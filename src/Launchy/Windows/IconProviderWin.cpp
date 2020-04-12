/*
  Launchy: Application Launcher
  Copyright (C) 2007-2009  Josh Karlin, Simon Capewell

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

#include "IconProviderWin.h"
#include "UtilWin.h"

namespace launchy {

IconProviderWin::IconProviderWin() {
//     // Load Vista/7 specific API pointers
//     HMODULE hLib = GetModuleHandleW(L"shell32");
//     if (hLib) {
//         (FARPROC&)fnSHCreateItemFromParsingName = GetProcAddress(hLib, "SHCreateItemFromParsingName");
//     }
}

IconProviderWin::~IconProviderWin() {

}

QIcon IconProviderWin::icon(const QFileInfo& info) const {
    QIcon retIcon;

    QString fileExtension = info.suffix().toLower();

    if (fileExtension == QStringLiteral("png")
        || fileExtension == QStringLiteral("bmp")
        || fileExtension == QStringLiteral("jpg")
        || fileExtension == QStringLiteral("jpeg")) {
        retIcon = QIcon(info.filePath());
    }
    else if (fileExtension == QStringLiteral("cpl")) {
        HICON hIcon;
        QString filePath = QDir::toNativeSeparators(info.filePath());
        ExtractIconEx((LPCTSTR)filePath.utf16(), 0, &hIcon, NULL, 1);
        retIcon = QIcon(QtWin::fromHICON(hIcon));
        DestroyIcon(hIcon);
    }
    else if (fileExtension == QStringLiteral("lnk") || info.isSymLink()) {

        QString targetPath = linkTargetPathTo64(info);

        qDebug() << "IconProviderWin::icon, lnk, target path:" << targetPath
                 << ", info target path:" << info.symLinkTarget();

        retIcon = icon(targetPath);
    }
    else {

        unsigned int flags = SHGFI_ICON | SHGFI_SYSICONINDEX | SHGFI_ICONLOCATION;

        if (m_preferredSize <= 16) {
            flags |= SHIL_SMALL;
        }
        else if (m_preferredSize <= 32) {
            flags |= SHIL_LARGE;
        }
        else if (m_preferredSize <= 48) {
            flags |= SHIL_EXTRALARGE;
        }
        else {
            flags |= SHIL_JUMBO;
        }

        QString filePath = QDir::toNativeSeparators(info.filePath());

        qDebug() << "IconProviderWin::icon, exe, file path:" << filePath
                 << ", flags:" << flags;

        SHFILEINFO sfi;
        ZeroMemory(&sfi, sizeof(sfi));

        // Get the icon index using SHGetFileInfo
        SHGetFileInfo((LPCTSTR)filePath.utf16(), 0, &sfi, sizeof(sfi), flags);
        if (sfi.hIcon) {
            retIcon.addPixmap(QtWin::fromHICON(sfi.hIcon));
            // extra large icon
            if (m_preferredSize >= 48) {
                addIconFromImageList(SHIL_EXTRALARGE, sfi.iIcon, retIcon);
            }
        }
        else {
            qDebug() << "IconProviderWin::icon, exe, fail to extract by SHGetFileInfo";
            retIcon = QFileIconProvider::icon(info);
        }
    }

    return retIcon;
}

QString IconProviderWin::linkTargetPathTo64(const QFileInfo& info) const {
    // On 64 bit windows, 64 bit shortcuts don't resolve correctly from 32 bit executables, fix it here
    QString path = QDir::toNativeSeparators(info.symLinkTarget());

    if (QFileInfo(path).exists()) {
        return path;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!env.contains("PROGRAMW6432")) {
        return path;
    }

    QString pf32 = env.value("PROGRAMFILES"); // C:\Program Files (x86)
    QString pf64 = env.value("PROGRAMW6432"); // C:\Program Files

    if (pf32 != pf64) {
        if (path.contains(pf32)) {
            QString path64 = path;
            path64.replace(pf32, pf64);
            if (QFileInfo(path64).exists()) {
                path = path64;
            }
        }
        else if (path.contains("system32")) {
            QString path32 = path;
            if (!QFileInfo(path32).exists()) {
                path = path32.replace("system32", "sysnative");
            }
        }
    }

    return path;
}

bool IconProviderWin::addIconFromImageList(int imageListIndex, int iconIndex, QIcon& icon) const {
    IImageList* imageList;
    HRESULT hResult = SHGetImageList(imageListIndex, IID_IImageList, (void**)&imageList);
    if (hResult == S_OK && imageList != NULL) {
        HICON hIcon = NULL;
        hResult = imageList->GetIcon(iconIndex, ILD_TRANSPARENT, &hIcon);
        imageList->Release();

        if (hResult == S_OK && hIcon != NULL) {
            icon.addPixmap(QtWin::fromHICON(hIcon));
            DestroyIcon(hIcon);
        }
    }

    return SUCCEEDED(hResult);
}

// !! OBSOLETED function (samson 2020-4-12)
// On Vista or 7 we could use SHIL_JUMBO to get a 256x256 icon,
// but we'll use SHCreateItemFromParsingName as it'll give an identical
// icon to the one shown in explorer and it scales automatically.
bool IconProviderWin::addIconFromShellFactory(const QString& filePath, QIcon& icon) const {
    HRESULT hResult = S_FALSE;
    IShellItemImageFactory* pSIIF = NULL;
    CoInitialize(NULL);
    hResult = SHCreateItemFromParsingName((LPCTSTR)filePath.utf16(), NULL, IID_PPV_ARGS(&pSIIF));
    if (hResult == S_OK) {
        HBITMAP hBitmap = NULL;
        SIZE iconSize = {m_preferredSize, m_preferredSize};
        hResult = pSIIF->GetImage(iconSize, SIIGBF_RESIZETOFIT | SIIGBF_ICONONLY , &hBitmap);
        if (hResult == S_OK) {
            QPixmap iconPixmap = QtWin::fromHBITMAP(hBitmap, QtWin::HBitmapPremultipliedAlpha);
            icon.addPixmap(iconPixmap);
            DeleteObject(hBitmap);
        }
        pSIIF->Release();
    }
    CoUninitialize();
    return hResult == S_OK;
}

}
