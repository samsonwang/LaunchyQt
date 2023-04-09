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

#include <windows.h>

#include <shellapi.h>
#include <ShlObj.h>
#include <CommCtrl.h>
#include <commoncontrols.h>

#include <QDir>
#include <QFileInfo>
#include <QDebug>
// #include <QtWin>
#include <QProcessEnvironment>

#include "UtilWin.h"

namespace launchy {

IconProviderWin::IconProviderWin() {

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
        ExtractIconExW((LPCWSTR)filePath.utf16(), 0, &hIcon, NULL, 1);
        retIcon = QIcon(QPixmap::fromImage(QImage::fromHICON(hIcon)));
        DestroyIcon(hIcon);
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

        qDebug() << "IconProviderWin::icon, file path:" << filePath
            << ", flags:" << flags;

        SHFILEINFOW sfi;
        ZeroMemory(&sfi, sizeof(sfi));

        // Get the icon index using SHGetFileInfo
        SHGetFileInfoW((LPCWSTR)filePath.utf16(), 0, &sfi, sizeof(sfi), flags);
        if (sfi.hIcon) {
            retIcon.addPixmap(QPixmap::fromImage(QImage::fromHICON(sfi.hIcon)));
            // extra large icon
            if (m_preferredSize >= 48) {
                addIconFromImageList(SHIL_EXTRALARGE, sfi.iIcon, retIcon);
            }
        }
        else {
            qDebug() << "IconProviderWin::icon, fail to extract by SHGetFileInfo, use qt default";
            retIcon = QFileIconProvider::icon(info);
        }
    }

    return retIcon;
}

QString IconProviderWin::linkTargetPathTo64(const QFileInfo& info) const {
    // On 64 bit windows, 64 bit shortcuts don't resolve correctly from 32 bit executables, fix it here
    QString strPath = QDir::toNativeSeparators(info.symLinkTarget());

    if (QFileInfo(strPath).exists()) {
        return strPath;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!env.contains("PROGRAMW6432")) {
        return strPath;
    }

    QString strDir32 = env.value("PROGRAMFILES"); // C:\Program Files (x86)
    QString strDir64 = env.value("PROGRAMW6432"); // C:\Program Files

    if (strDir32 != strDir64) {
        if (strPath.contains(strDir32)) {
            QString strPath64 = strPath;
            strPath64.replace(strDir32, strDir64);
            if (QFileInfo(strPath64).exists()) {
                strPath = strPath64;
            }
        }
        else if (strPath.contains("system32")) {
            QString strPath32 = strPath;
            if (!QFileInfo(strPath32).exists()) {
                strPath = strPath32.replace("system32", "sysnative");
            }
        }
    }

    return strPath;
}

bool IconProviderWin::addIconFromImageList(int imageListIndex, int iconIndex, QIcon& icon) const {
    IImageList* pImageList = NULL;
    HRESULT hResult = SHGetImageList(imageListIndex, IID_IImageList, (void**)&pImageList);
    if (hResult == S_OK && pImageList != NULL) {
        HICON hIcon = NULL;
        hResult = pImageList->GetIcon(iconIndex, ILD_TRANSPARENT, &hIcon);
        if (hResult == S_OK && hIcon != NULL) {
            icon.addPixmap(QPixmap::fromImage(QImage::fromHICON(hIcon)));
            DestroyIcon(hIcon);
        }
        pImageList->Release();
    }

    return SUCCEEDED(hResult);
}

// !! OBSOLETED function (samson 2020-4-12)
// On Vista or 7 we could use SHIL_JUMBO to get a 256x256 icon,
// but we'll use SHCreateItemFromParsingName as it'll give an identical
// icon to the one shown in explorer and it scales automatically.
bool IconProviderWin::addIconFromShellFactory(const QString& filePath, QIcon& icon) const {
    CoInitialize(NULL);
    HRESULT hResult = S_FALSE;
    IShellItemImageFactory* pSIIF = NULL;
    hResult = SHCreateItemFromParsingName((PCWSTR)filePath.utf16(), NULL, IID_PPV_ARGS(&pSIIF));
    if (hResult == S_OK && pSIIF) {
        HBITMAP hBitmap = NULL;
        SIZE iconSize = {m_preferredSize, m_preferredSize};
        hResult = pSIIF->GetImage(iconSize, SIIGBF_RESIZETOFIT | SIIGBF_ICONONLY , &hBitmap);
        if (hResult == S_OK && hBitmap != NULL) {
            QPixmap iconPixmap = QPixmap::fromImage(QImage::fromHBITMAP(hBitmap));
            icon.addPixmap(iconPixmap);
            DeleteObject(hBitmap);
        }
        pSIIF->Release();
    }
    CoUninitialize();
    return hResult == S_OK;
}

} // namespace launchy
