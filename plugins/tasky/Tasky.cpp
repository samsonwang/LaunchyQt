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

#include <QtGui>
#include <QUrl>
#include <QFile>
#include <QRegExp>
#include <QTextCodec>
#include <QPixmap>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#include <shlobj.h>
#include <tchar.h>
#endif

#include "PluginMsg.h"

using namespace launchy;

taskyPlugin* gtaskyInstance = NULL;

QList<QString> windowTitles;
QList<QString> iconPaths;
QString selectedCatItemText;
HWND selectedWindow;

static QImage qt_fromWinHBITMAP(HDC hdc, HBITMAP bitmap, int w, int h)
{
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage = w * h * 4;

    QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
    if (image.isNull())
        return image;

    // Get bitmap bits
    uchar *data = (uchar *)qMalloc(bmi.bmiHeader.biSizeImage);

    if (GetDIBits(hdc, bitmap, 0, h, data, &bmi, DIB_RGB_COLORS)) {
        // Create image and copy data into image.
        for (int y = 0; y<h; ++y) {
            void *dest = (void *)image.scanLine(y);
            void *src = data + y * image.bytesPerLine();
            memcpy(dest, src, image.bytesPerLine());
        }
    }
    else {
        qWarning("qt_fromWinHBITMAP(), failed to get bitmap bits");
    }
    qFree(data);

    return image;
}

QPixmap convertHIconToPixmap(const HICON icon)
{
    bool foundAlpha = false;
    HDC screenDevice = GetDC(0);
    HDC hdc = CreateCompatibleDC(screenDevice);
    ReleaseDC(0, screenDevice);

    ICONINFO iconinfo;
    bool result = GetIconInfo(icon, &iconinfo); //x and y Hotspot describes the icon center
    if (!result)
        qWarning("convertHIconToPixmap(), failed to GetIconInfo()");

    int w = iconinfo.xHotspot * 2;
    int h = iconinfo.yHotspot * 2;

    BITMAPINFOHEADER bitmapInfo;
    bitmapInfo.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.biWidth = w;
    bitmapInfo.biHeight = h;
    bitmapInfo.biPlanes = 1;
    bitmapInfo.biBitCount = 32;
    bitmapInfo.biCompression = BI_RGB;
    bitmapInfo.biSizeImage = 0;
    bitmapInfo.biXPelsPerMeter = 0;
    bitmapInfo.biYPelsPerMeter = 0;
    bitmapInfo.biClrUsed = 0;
    bitmapInfo.biClrImportant = 0;
    DWORD* bits;

    HBITMAP winBitmap = CreateDIBSection(hdc, (BITMAPINFO*)&bitmapInfo, DIB_RGB_COLORS, (VOID**)&bits, NULL, 0);
    HGDIOBJ oldhdc = (HBITMAP)SelectObject(hdc, winBitmap);
    DrawIconEx(hdc, 0, 0, icon, iconinfo.xHotspot * 2, iconinfo.yHotspot * 2, 0, 0, DI_NORMAL);
    QImage image = qt_fromWinHBITMAP(hdc, winBitmap, w, h);

    for (int y = 0; y < h && !foundAlpha; y++) {
        QRgb *scanLine = reinterpret_cast<QRgb *>(image.scanLine(y));
        for (int x = 0; x < w; x++) {
            if (qAlpha(scanLine[x]) != 0) {
                foundAlpha = true;
                break;
            }
        }
    }
    if (!foundAlpha) {
        //If no alpha was found, we use the mask to set alpha values
        DrawIconEx(hdc, 0, 0, icon, w, h, 0, 0, DI_MASK);
        QImage mask = qt_fromWinHBITMAP(hdc, winBitmap, w, h);

        for (int y = 0; y < h; y++) {
            QRgb *scanlineImage = reinterpret_cast<QRgb *>(image.scanLine(y));
            QRgb *scanlineMask = mask.isNull() ? 0 : reinterpret_cast<QRgb *>(mask.scanLine(y));
            for (int x = 0; x < w; x++) {
                if (scanlineMask && qRed(scanlineMask[x]) != 0)
                    scanlineImage[x] = 0; //mask out this pixel
                else
                    scanlineImage[x] |= 0xff000000; // set the alpha channel to 255
            }
        }
    }
    //dispose resources created by iconinfo call
    DeleteObject(iconinfo.hbmMask);
    DeleteObject(iconinfo.hbmColor);

    SelectObject(hdc, oldhdc); //restore state
    DeleteObject(winBitmap);
    DeleteDC(hdc);
    return QPixmap::fromImage(image);
}

HICON getHIconFromExe(HWND hWnd)
{
    DWORD procId = 0;

    GetWindowThreadProcessId(hWnd, &procId);

    HICON hIcon = NULL;

    if (procId != 0)
    {
        HANDLE hndl = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, procId);

        if (hndl != 0)
        {
            TCHAR filename[2048];

            if (GetModuleFileNameEx(hndl, NULL, filename, sizeof(filename) / sizeof(TCHAR)))
            {
                ExtractIconEx(filename, 0, &hIcon, 0, 1);
            }

            CloseHandle(hndl);
        }
    }

    return hIcon;
}

QString getIconFromHwnd(HWND hWnd)
{
    QString iconPath = qApp->applicationDirPath() + "/plugins/icons/tasky/" + QString::number((int)hWnd) + ".png";

    //no need to get icon if it already exists
    if (!QFile::exists(iconPath))
    {
        HICON hIcon = NULL;

        //try to get large icon from window
        hIcon = (HICON)GetClassLong(hWnd, GCL_HICON);

        //if not possible, get large icon from exe
        if (hIcon == NULL)
        {
            hIcon = getHIconFromExe(hWnd);
        }

        if (hIcon != NULL)
        {
            QPixmap qpix = convertHIconToPixmap(hIcon);

            if (qpix.width() < 32)
            {
                hIcon = getHIconFromExe(hWnd);

                if (hIcon != NULL)
                {
                    qpix = convertHIconToPixmap(hIcon);
                }
            }

            qpix.save(iconPath);
        }
    }

    return iconPath;
}

void initIconDir()
{
    QDir iconDir(qApp->applicationDirPath() + "/plugins/icons/tasky/");
    if (!iconDir.exists())
    {
        iconDir.mkpath(qApp->applicationDirPath() + "/plugins/icons/tasky/");
    }
    QDirIterator iconDirIt(iconDir);
    while (iconDirIt.hasNext())
    {
        iconDir.remove(iconDirIt.next());
    }
}

QString fromWCharArray(const wchar_t *string)
{
    if (sizeof(wchar_t) == sizeof(QChar)) {
        return QString::fromUtf16((ushort *)string, -1);
    }
    else {
        return QString::fromUcs4((uint *)string, -1);
    }
}

BOOL windowHasTaskbarButton(HWND hWnd)
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

BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
    wchar_t *title;

    if (!windowHasTaskbarButton(hWnd))
        return TRUE;

    int len = static_cast<int>(SendMessageW(hWnd, WM_GETTEXTLENGTH, 0, 0));
    title = new wchar_t[len+1];
    if (!SendMessageW(hWnd, WM_GETTEXT, len+1, reinterpret_cast<LPARAM>(title)))
        return TRUE; // No Title

    windowTitles.push_back(fromWCharArray(title));
    iconPaths.push_back(getIconFromHwnd(hWnd));

    return TRUE;
}

BOOL CALLBACK FindWindowTitle(HWND hWnd, LPARAM lParam) {
    wchar_t *title;

    if (!hWnd)
        return TRUE; // Not a window

    if (!::IsWindowVisible(hWnd))
        return TRUE; // Not visible

    int len = static_cast<int>(SendMessageW(hWnd, WM_GETTEXTLENGTH, 0, 0));
    title = new wchar_t[len+1];
    if (!SendMessageW(hWnd, WM_GETTEXT, len+1, reinterpret_cast<LPARAM>(title)))
        return TRUE; // No Title

    if (selectedCatItemText == fromWCharArray(title)) {
        selectedWindow = hWnd;
        return FALSE;
    }

    return TRUE;
}

BOOL searchWindowTitle(QString search, QString windowTitle)
{
    for (int i = 0; i < search.size(); ++i) {
        int titleCount = windowTitle.count(search[i], Qt::CaseInsensitive);
        int searchCount = search.count(search[i], Qt::CaseInsensitive);

        if (titleCount < searchCount)
        {
            return false;
        }
    }

    return true;
}

void taskyPlugin::getID(uint* id)
{
    *id = HASH_tasky;
}

void taskyPlugin::getName(QString* str)
{
    *str = PLUGIN_NAME;
}

void taskyPlugin::init()
{
    initIconDir();
}

void taskyPlugin::getLabels(QList<InputData>* id)
{
}

void taskyPlugin::getResults(QList<InputData>* id, QList<CatItem>* results)
{
    QString text = id->first().getText();

    if (id->count() < 3)
    {
        if (id->count() == 1)
        {
            if (text == "")
                return;
        }
        else
        {
            if (!text.contains("tasky", Qt::CaseInsensitive))
                return;

            text = id->last().getText();
        }

        windowTitles.clear();
        iconPaths.clear();
        initIconDir();

        EnumWindows(EnumWindowsProc, NULL);

        for (int i = 0; i < windowTitles.size(); ++i) {
            if (searchWindowTitle(text, windowTitles[i]))
                results->push_front(CatItem(windowTitles[i] + "." + PLUGIN_NAME, windowTitles[i], HASH_tasky, iconPaths[i]));
        }
    }
}

QString taskyPlugin::getIcon()
{
#ifdef Q_OS_WIN
    return qApp->applicationDirPath() + "/plugins/icons/tasky.png";
#endif
}

void taskyPlugin::getCatalog(QList<CatItem>* items)
{
    items->push_back(CatItem("Tasky.tasky", "Tasky", HASH_tasky, getIcon()));
}

void taskyPlugin::launchItem(QList<InputData>* id, CatItem* item)
{
    CatItem* base = (id->count() == 1) ? &id->first().getTopResult() : &id->last().getTopResult();

    selectedCatItemText = base->shortName;

    EnumWindows(FindWindowTitle, NULL);

    WINDOWPLACEMENT wp;

    if (!GetWindowPlacement(selectedWindow, &wp))
        return;

    switch (wp.showCmd) {
    case SW_SHOWMINIMIZED:
        ShowWindow(selectedWindow, SW_RESTORE);
    case SW_SHOWNORMAL:
    case SW_SHOWMAXIMIZED:
    default:
        BringWindowToTop(selectedWindow);
        break;
    }

    SetForegroundWindow(selectedWindow);
    SetActiveWindow(selectedWindow);
    return;
}

void taskyPlugin::doDialog(QWidget* parent, QWidget** newDlg)
{
}

void taskyPlugin::endDialog(bool accept)
{
}

void taskyPlugin::hide()
{
}

void taskyPlugin::show()
{
}

taskyPlugin::taskyPlugin()
{
    HASH_tasky = qHash(QString(PLUGIN_NAME));
}

taskyPlugin::~taskyPlugin()
{

}

int taskyPlugin::msg(int msgId, void* wParam, void* lParam)
{
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
    case MSG_LAUNCHY_HIDE:
        hide();
        break;
    case MSG_LAUNCHY_SHOW:
        show();
    default:
        break;
    }

    return handled;
}

Q_EXPORT_PLUGIN2(tasky, taskyPlugin)
