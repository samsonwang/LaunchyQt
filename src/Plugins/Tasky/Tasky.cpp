/*
Tasky
Copyright (C) 2010  Daniel Ossipoff

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "Tasky.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#include <psapi.h>
#include <shlobj.h>
#include <tchar.h>
#include <WinUser.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

// DWMWA_CLOAKED arrived with the Windows 8 SDK, an older SDK misses the
// constant (the attribute query itself is Vista+ and answers E_INVALIDARG,
// so the filter below degrades to a no-op on Windows 7).
#ifndef DWMWA_CLOAKED
#define DWMWA_CLOAKED 14
#endif

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtWin>
#else

#endif // QT_VERSION

#endif

#include <QtGui>
#include <QUrl>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegExp>
#include <QTextCodec>
#include <QPixmap>

#include "LaunchyLib/LaunchyLib.h"
#include "LaunchyLib/PluginMsg.h"

#define PLUGIN_NAME "Tasky"

using namespace launchy;

static QList<QString> g_windowTitles;
static QList<QString> g_iconPaths;
// The window handles behind the titles above, filled in lockstep by the same
// enumeration: the launch must activate one of the windows the results
// offered, not the first window a fresh lookup happens to find.
static QList<HWND> g_windowHandles;
static HWND g_hSelectedWindow = NULL;

// The executable behind a window, the stable identity its icon is cached
// under. Returns false when the process cannot be opened (an elevated
// process, for instance), the caller then falls back to the window handle.
static bool getExePathOfWindow(HWND hWnd, QString* path) {
    DWORD procId = 0;
    GetWindowThreadProcessId(hWnd, &procId);

    if (procId == 0 || !path)
        return false;

    bool ok = false;
    HANDLE hndl = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, procId);
    if (hndl != 0) {
        WCHAR filename[2048];
        if (GetModuleFileNameExW(hndl, NULL, filename, sizeof(filename) / sizeof(WCHAR))) {
            *path = QString::fromWCharArray(filename);
            ok = true;
        }
        CloseHandle(hndl);
    }
    return ok;
}

// The returned icon is owned by the caller: it is extracted, not shared, and
// must be freed with DestroyIcon() once it is no longer needed.
static HICON getHIconFromExe(HWND hWnd) {
    QString exePath;
    HICON hIcon = NULL;
    if (getExePathOfWindow(hWnd, &exePath)) {
        ExtractIconExW((const wchar_t*)exePath.utf16(), 0, &hIcon, 0, 1);
    }
    return hIcon;
}

static BOOL windowHasTaskbarButton(HWND hWnd) {
    if (!hWnd)
        return FALSE; // Not a window

    if (!::IsWindowVisible(hWnd))
        return FALSE; // Not visible

    // UWP and other modern applications keep several top-level windows per
    // instance, all but the shown one sit "cloaked" behind the DWM: visible
    // by every Win32 flag below, yet never on screen. Without this check
    // they fill the results with entries no switch could ever show.
    DWORD cloaked = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED,
                                        &cloaked, sizeof(cloaked)))
        && cloaked != 0) {
        return FALSE; // Cloaked, off screen
    }

    LONG style = GetWindowLong(hWnd, GWL_STYLE);
    LONG exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

    if (style & WS_CHILD)
        return FALSE;

    if (exStyle & WS_EX_TOOLWINDOW)
        return FALSE;

    return TRUE;
}

// A window whose thread stopped responding would block the whole enumeration
// on its message queue forever: every message below is bounded by a short
// timeout, and Windows aborts the wait at once when the target thread is
// officially hung (SMTO_ABORTIFHUNG), so a dead application freezes the
// keystroke handling of launchy no longer than this. SMTO_BLOCK keeps the
// wait from dispatching messages on this thread, the enumeration stays
// re-entrancy free like a plain SendMessage would be.
static const UINT kTitleMsgFlags = SMTO_ABORTIFHUNG | SMTO_BLOCK;
static const UINT kTitleMsgTimeoutMs = 200;

// The title of a window, bounded by the timeout above. Returns false when
// the window has no title, or did not answer in time.
static bool windowTitleOf(HWND hWnd, QString* title) {
    DWORD_PTR len = 0;
    if (!::SendMessageTimeoutW(hWnd, WM_GETTEXTLENGTH, 0, 0,
                               kTitleMsgFlags, kTitleMsgTimeoutMs, &len)) {
        return false;
    }
    if (len == 0) {
        return false; // No Title
    }

    QScopedPointer<wchar_t> buffer(new wchar_t[len + 1]);
    DWORD_PTR copied = 0;
    if (!::SendMessageTimeoutW(hWnd, WM_GETTEXT, len + 1,
                               reinterpret_cast<LPARAM>(buffer.data()),
                               kTitleMsgFlags, kTitleMsgTimeoutMs, &copied)
        || copied == 0) {
        return false;
    }
    *title = QString::fromWCharArray(buffer.data(), static_cast<int>(copied));
    return true;
}

static BOOL CALLBACK enumWindowsProc(HWND hWnd, LPARAM lParam) {
    Tasky* pPlugin = (Tasky*)lParam;
    if (!pPlugin) {
        return TRUE;
    }

    if (!windowHasTaskbarButton(hWnd)) {
        return TRUE;
    }

    QString title;
    if (!windowTitleOf(hWnd, &title)) {
        return TRUE; // No Title, or the window is hung
    }

    g_windowTitles.push_back(title);
    g_iconPaths.push_back(pPlugin->getIconFromHWND(hWnd));
    g_windowHandles.push_back(hWnd);

    return TRUE;
}

// An ordered, case-insensitive subsequence match replaces the old character
// count test: "abc" no longer matches a title that merely holds the same
// letters in any order, every search character must be found after the
// previous one. The returned score ranks the matches, lower is better, and
// is meant to become the usage the final catalog sort orders by:
//   -1          no match
//   0..size-1   the title contains the search text, score is its position
//   >= size     scattered subsequence, score is size + span (last - first)
// so a contiguous match always outranks a scattered one at equal length.
static int matchWindowTitle(const QString& search, const QString& windowTitle) {
    if (search.isEmpty()) {
        return 0; // no filter: every window matches, at the same rank
    }

    const int noMatch = -1;
    const int at = windowTitle.indexOf(search, 0, Qt::CaseInsensitive);
    if (at >= 0) {
        return at; // contiguous match, the earlier the position the better
    }

    int first = -1;
    int last = -1;
    for (int i = 0; i < search.size(); ++i) {
        const int found = windowTitle.indexOf(search[i], last + 1, Qt::CaseInsensitive);
        if (found < 0) {
            return noMatch;
        }
        if (first < 0) {
            first = found;
        }
        last = found;
    }
    return windowTitle.size() + (last - first);
}

void Tasky::getName(QString* name) {
    *name = PLUGIN_NAME;
}

void Tasky::init() {
    initIconCache();
}

void Tasky::setPath(const QString* path) {
    Q_ASSERT(path);
    m_libPath = *path;
    qDebug() << "Tasky::setPath, m_libPath:" << m_libPath;
}

void Tasky::getLabels(QList<InputData>* inputList) {
    QString text = inputList->first().getText();
    //text.replace(" ", "");
    if (text.compare("tasky", Qt::CaseInsensitive) == 0) {
        inputList->first().setLabel(LABEL_TASKY);
    }
}

void Tasky::getResults(QList<InputData>* inputList, QList<CatItem>* result) {
    if (!inputList
        || inputList->isEmpty()
        || inputList->count() > 3
        || !inputList->first().hasLabel(LABEL_TASKY)) {
        return;
    }

    g_windowTitles.clear();
    g_iconPaths.clear();
    g_windowHandles.clear();
    // The icon cache is resolved once at init and never wiped per query,
    // this only covers a plugin that somehow missed MSG_INIT.
    if (m_iconCacheDir.isEmpty())
        initIconCache();

    EnumWindows(enumWindowsProc, (LPARAM)this);

    QString text = inputList->last().getText();
    for (int i = 0; i < g_windowTitles.size(); ++i) {
        const int score = matchWindowTitle(text, g_windowTitles[i]);
        if (score < 0) {
            continue;
        }

        CatItem item(g_windowTitles[i] + "." + PLUGIN_NAME,
                     g_windowTitles[i],
                     LABEL_TASKY,
                     g_iconPaths[i]);
        // Usage is the only lever a plugin has over the final catalog sort:
        // the tighter the match, the higher the usage. Equal scores share one
        // usage, so their relative order stays the Z-order of the push_backs.
        item.usage = 50000 - score;
        result->push_back(item);
    }

}

QString Tasky::getIcon() const {
    return m_libPath + "/tasky.png";
}

void Tasky::getCatalog(QList<CatItem>* items) {
    items->push_back(CatItem("Tasky.tasky", "Tasky", LABEL_TASKY, getIcon()));
}

void Tasky::launchItem(QList<InputData>* id, CatItem* item) {
    Q_UNUSED(item)

    CatItem* base = (id->count() == 1) ? &id->first().getTopResult() : &id->last().getTopResult();

    // Activate one of the windows the results offered, by title: the lookup
    // runs over the very enumeration that built the results, so child and
    // tool windows are filtered by the same rules and a duplicate title hits
    // the topmost window, the one shown first in the list. A fresh
    // enumeration by title could not promise either.
    g_hSelectedWindow = NULL;
    const int count = qMin(g_windowTitles.size(), g_windowHandles.size());
    for (int i = 0; i < count; ++i) {
        if (g_windowTitles[i] == base->shortName) {
            g_hSelectedWindow = g_windowHandles[i];
            break;
        }
    }

    // The handle was enumerated when the results were built and may be stale
    // by now: the window was closed, or the handle value was recycled since.
    if (g_hSelectedWindow == NULL || !IsWindow(g_hSelectedWindow)) {
        g_hSelectedWindow = NULL;
        return;
    }

    WINDOWPLACEMENT wp;
    if (!GetWindowPlacement(g_hSelectedWindow, &wp))
        return;

    switch (wp.showCmd) {
    case SW_SHOWMINIMIZED:
        ShowWindow(g_hSelectedWindow, SW_RESTORE);
    case SW_SHOWNORMAL:
    case SW_SHOWMAXIMIZED:
    default:
        BringWindowToTop(g_hSelectedWindow);
        break;
    }

    SetForegroundWindow(g_hSelectedWindow);
    SetActiveWindow(g_hSelectedWindow);
    return;
}

void Tasky::doDialog(QWidget* parent, QWidget** dialog) {
    Q_UNUSED(parent)
        Q_UNUSED(dialog)
}

void Tasky::endDialog(bool accept) {
    Q_UNUSED(accept)
}

// Resolve the icon cache directory once per Launchy run. The icons are
// throwaway data and the install dir is not writable for a normal user in a
// portable setup, so they live under the app temp path like WebSearchPy keeps
// its favicons: <temp>/Launchy/Tasky.
//
// Nothing is wiped here. The .png files are the cache itself, they are reused
// across queries and Launchy restarts; only .tmp leftovers of a save that was
// interrupted are removed, they are never useful.
void Tasky::initIconCache() {
    m_iconCacheDir = launchy::getAppTempPath(false) + "/Tasky";
    QDir iconDir(m_iconCacheDir);
    if (!iconDir.exists()) {
        iconDir.mkpath(".");
    }
    QDirIterator iconDirIt(iconDir);
    while (iconDirIt.hasNext()) {
        const QString filePath = iconDirIt.next();
        if (filePath.endsWith(".tmp", Qt::CaseInsensitive)) {
            QFile::remove(filePath);
        }
    }
}

// The cache file of a window. The window handle is recycled by the system and
// is no identity at all, so like one favicon file per host in WebSearchPy,
// there is one icon file per process image: every window of the same
// executable reuses it, across queries and restarts.
QString Tasky::iconCachePath(HWND hWnd) const {
    if (m_iconCacheDir.isEmpty())
        return QString();

    QString exePath;
    QString name;
    if (getExePathOfWindow(hWnd, &exePath)) {
        name = QFileInfo(exePath).completeBaseName().toLower();
    }
    // The process could not be opened, the window handle is all that is left.
    if (name.isEmpty()) {
        name = QString("hwnd_%1").arg((qint64)hWnd);
    }
    return m_iconCacheDir + "/" + name + ".png";
}

// The cache only holds png files, and a window query that arrives mid-save
// must never see a half written one: write to a temp name first, then rename.
static void saveIconPng(const QPixmap& pix, const QString& path) {
    QString tmpPath = path + ".tmp";
    QFile::remove(tmpPath);
    if (!pix.save(tmpPath, "PNG")) {
        QFile::remove(tmpPath);
        return;
    }
    QFile::remove(path);
    QFile::rename(tmpPath, path);
}

QString Tasky::getIconFromHWND(HWND hWnd) {
    QString iconPath = iconCachePath(hWnd);
    if (iconPath.isEmpty())
        return iconPath;

    // no need to extract anything while the cached icon exists
    if (QFile::exists(iconPath)) {
        return iconPath;
    }

    // try to get large icon from window
    //
    // A class icon is shared by the system, DestroyIcon() must never touch it;
    // an icon extracted from an exe is owned by this process and leaks unless
    // it is freed once the pixmap conversion is done, hence ownedIcon below.
    HICON hIcon = (HICON)GetClassLongPtr(hWnd, GCLP_HICON);
    bool ownedIcon = false;

    // if not possible, get large icon from exe
    if (hIcon == NULL) {
        hIcon = getHIconFromExe(hWnd);
        ownedIcon = (hIcon != NULL);
    }

    if (hIcon != NULL) {
        QPixmap qpix = QtWin::fromHICON(hIcon);

        if (qpix.width() < 32) {
            if (ownedIcon) {
                DestroyIcon(hIcon);
            }
            hIcon = getHIconFromExe(hWnd);
            ownedIcon = (hIcon != NULL);

            if (hIcon != NULL) {
                qpix = QtWin::fromHICON(hIcon);
            }
        }
        if (ownedIcon) {
            DestroyIcon(hIcon);
        }
        saveIconPng(qpix, iconPath);
    }

    return iconPath;
}

Tasky::Tasky()
    : LABEL_TASKY(PLUGIN_NAME) {
}

Tasky::~Tasky() {
}

int Tasky::msg(int msgId, void* wParam, void* lParam) {
    bool handled = false;
    switch (msgId) {
    case MSG_INIT:
        init();
        handled = true;
        break;

    case MSG_GET_LABELS:
        getLabels((QList<InputData>*) wParam);
        handled = true;
        break;

    case MSG_GET_NAME:
        getName((QString*)wParam);
        handled = true;
        break;

    case MSG_GET_RESULTS:
        getResults((QList<InputData>*) wParam, (QList<CatItem>*) lParam);
        handled = true;
        break;

    case MSG_GET_CATALOG:
        getCatalog((QList<CatItem>*) wParam);
        handled = true;
        break;

    case MSG_LAUNCH_ITEM:
        launchItem((QList<InputData>*) wParam, (CatItem*)lParam);
        handled = true;
        break;

    case MSG_DO_DIALOG:
        doDialog((QWidget*)wParam, (QWidget**)lParam);
        break;

    case MSG_END_DIALOG:
        endDialog((bool)wParam);
        break;

    case MSG_PATH:
        setPath((const QString*)wParam);
        break;

    default:
        break;
    }

    return handled;
}
