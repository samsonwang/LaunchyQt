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
#include <psapi.h>
#include <shlobj.h>
#include <tchar.h>
#include <WinUser.h>
#include <QtWin>
#endif

#include <QtGui>
#include <QUrl>
#include <QFile>
#include <QRegExp>
#include <QTextCodec>
#include <QPixmap>
#include "PluginMsg.h"

#define PLUGIN_NAME "tasky"

using namespace launchy;

Tasky* g_taskyInstance = NULL;

static QList<QString> g_windowTitles;
static QList<QString> g_iconPaths;
static QString g_selectedCatItemText;
static HWND g_hSelectedWindow;

static HICON getHIconFromExe(HWND hWnd) {
    DWORD procId = 0;
    GetWindowThreadProcessId(hWnd, &procId);

    HICON hIcon = NULL;
    if (procId != 0) {
        HANDLE hndl = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, procId);

        if (hndl != 0) {
            TCHAR filename[2048];
            if (GetModuleFileNameEx(hndl, NULL, filename, sizeof(filename) / sizeof(TCHAR))) {
                ExtractIconEx(filename, 0, &hIcon, 0, 1);
            }
            CloseHandle(hndl);
        }
    }
    return hIcon;
}

static QString getIconFromHWND(HWND hWnd) {
    QString iconPath = qApp->applicationDirPath() + "/plugins/Tasky/cache/" + QString::number((int64_t)hWnd) + ".png";

    //no need to get icon if it already exists
    if (!QFile::exists(iconPath)) {
        //try to get large icon from window
        HICON hIcon = (HICON)GetClassLongPtr(hWnd, GCLP_HICON);

        //if not possible, get large icon from exe
        if (hIcon == NULL) {
            hIcon = getHIconFromExe(hWnd);
        }

        if (hIcon != NULL) {
            QPixmap qpix = QtWin::fromHICON(hIcon);

            if (qpix.width() < 32) {
                hIcon = getHIconFromExe(hWnd);

                if (hIcon != NULL) {
                    qpix = QtWin::fromHICON(hIcon);
                }
            }
            qpix.save(iconPath);
        }
    }

    return iconPath;
}

static QString fromWCharArray(const wchar_t *string) {
    if (sizeof(wchar_t) == sizeof(QChar)) {
        return QString::fromUtf16((ushort *)string, -1);
    }
    else {
        return QString::fromUcs4((uint *)string, -1);
    }
}

static BOOL windowHasTaskbarButton(HWND hWnd)
{
    if (!hWnd)
        return FALSE; // Not a window

    if (!::IsWindowVisible(hWnd))
        return FALSE; // Not visible

    LONG style = GetWindowLong(hWnd, GWL_STYLE);
    LONG exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

    if (style & WS_CHILD)
        return FALSE;

    if (exStyle & WS_EX_TOOLWINDOW)
        return FALSE;

    return TRUE;
}

static BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
    if (!windowHasTaskbarButton(hWnd))
        return TRUE;

    int len = static_cast<int>(::SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0));
    wchar_t* title = new wchar_t[len+1];
    if (!::SendMessage(hWnd, WM_GETTEXT, len+1, reinterpret_cast<LPARAM>(title)))
        return TRUE; // No Title

    g_windowTitles.push_back(fromWCharArray(title));
    g_iconPaths.push_back(getIconFromHWND(hWnd));

    return TRUE;
}

static BOOL CALLBACK FindWindowTitle(HWND hWnd, LPARAM lParam) {
    if (!hWnd)
        return TRUE; // Not a window

    if (!::IsWindowVisible(hWnd))
        return TRUE; // Not visible

    int len = static_cast<int>(::SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0));
    wchar_t* title = new wchar_t[len+1];
    if (!::SendMessage(hWnd, WM_GETTEXT, len+1, reinterpret_cast<LPARAM>(title)))
        return TRUE; // No Title

    if (g_selectedCatItemText == fromWCharArray(title)) {
        g_hSelectedWindow = hWnd;
        return FALSE;
    }

    return TRUE;
}

static BOOL searchWindowTitle(QString search, QString windowTitle) {
    for (int i = 0; i < search.size(); ++i) {
        int titleCount = windowTitle.count(search[i], Qt::CaseInsensitive);
        int searchCount = search.count(search[i], Qt::CaseInsensitive);

        if (titleCount < searchCount) {
            return false;
        }
    }

    return true;
}

void Tasky::getID(uint* id) {
    *id = HASH_TASKY;
}

void Tasky::getName(QString* name) {
    *name = PLUGIN_NAME;
}

void Tasky::init() {
    initIconDir();
}

void Tasky::setPath(const QString* path) {
    Q_ASSERT(path);
    m_libPath = *path;
    qDebug() << "Tasky::setPath, m_libPath:" << m_libPath;
}

void Tasky::getLabels(QList<InputData>* id) {
}

void Tasky::getResults(QList<InputData>* id, QList<CatItem>* results) {
    QString text = id->first().getText();

    if (id->count() < 3) {
        if (id->count() == 1) {
            if (text.isEmpty())
                return;
        }
        else {
            if (!text.contains("tasky", Qt::CaseInsensitive))
                return;

            text = id->last().getText();
        }

        g_windowTitles.clear();
        g_iconPaths.clear();
        initIconDir();

        EnumWindows(EnumWindowsProc, NULL);

        for (int i = 0; i < g_windowTitles.size(); ++i) {
            if (searchWindowTitle(text, g_windowTitles[i]))
                results->push_front(CatItem(g_windowTitles[i] + "." + PLUGIN_NAME, g_windowTitles[i], HASH_TASKY, g_iconPaths[i]));
        }
    }
}

QString Tasky::getIcon() {
    return m_libPath + "/tasky.png";
}

void Tasky::getCatalog(QList<CatItem>* items) {
    items->push_back(CatItem("Tasky.tasky", "Tasky", HASH_TASKY, getIcon()));
}

void Tasky::launchItem(QList<InputData>* id, CatItem* item) {
    CatItem* base = (id->count() == 1) ? &id->first().getTopResult() : &id->last().getTopResult();

    g_selectedCatItemText = base->shortName;

    EnumWindows(FindWindowTitle, NULL);

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
}

void Tasky::endDialog(bool accept) {
}

void Tasky::initIconDir() {
    QDir iconDir(m_libPath + "/cache/");
    if (!iconDir.exists()) {
        iconDir.mkpath(".");
    }
    QDirIterator iconDirIt(iconDir);
    while (iconDirIt.hasNext()) {
        iconDir.remove(iconDirIt.next());
    }
}

Tasky::Tasky() {
    HASH_TASKY = qHash(QString(PLUGIN_NAME));
}

Tasky::~Tasky() {

}

int Tasky::msg(int msgId, void* wParam, void* lParam) {
    bool handled = false;
    switch (msgId)
    {
    case MSG_INIT:
        init();
        handled = true;
        break;
    case MSG_GET_LABELS:
        getLabels((QList<InputData>*) wParam);
        handled = true;
        break;
    case MSG_GET_ID:
        getID((uint*)wParam);
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
    case MSG_HAS_DIALOG:
        // Set to true if you provide a gui
        handled = false;
        break;
    case MSG_DO_DIALOG:
        // This isn't called unless you return true to MSG_HAS_DIALOG
        doDialog((QWidget*)wParam, (QWidget**)lParam);
        break;
    case MSG_END_DIALOG:
        // This isn't called unless you return true to MSG_HAS_DIALOG
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
