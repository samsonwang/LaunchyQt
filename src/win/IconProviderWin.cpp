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

#if 0
// Temporary work around to avoid having to install the latest Windows SDK
//#ifndef __IShellItemImageFactory_INTERFACE_DEFINED__
#define __IShellItemImageFactory_INTERFACE_DEFINED__

// ** marcro redefination
// #define SHIL_JUMBO 0x4

// IShellItemImageFactory::GetImage() flags
enum _SIIGB {
    SIIGBF_RESIZETOFIT = 0x00000000,
    SIIGBF_BIGGERSIZEOK = 0x00000001,
    SIIGBF_MEMORYONLY = 0x00000002,
    SIIGBF_ICONONLY = 0x00000004,
    SIIGBF_THUMBNAILONLY = 0x00000008,
    SIIGBF_INCACHEONLY = 0x00000010
};
typedef int SIIGBF;


const GUID IID_IShellItemImageFactory = {0xbcc18b79,0xba16,0x442f,{0x80,0xc4,0x8a,0x59,0xc3,0x0c,0x46,0x3b}};

class IShellItemImageFactory : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetImage(SIZE size, SIIGBF flags, /*__RPC__deref_out_opt*/ HBITMAP *phbm) = 0;
};

#endif


namespace launchy {

// This also exists in plugin_interface, need to remove both if I make a 64 build
static QString wicon_aliasTo64(QString path) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString pf32 = env.value("PROGRAMFILES");
    QString pf64 = env.value("PROGRAMW6432");

    // On 64 bit windows, 64 bit shortcuts don't resolve correctly from 32 bit executables, fix it here
    QFileInfo pInfo(path);

    if (env.contains("PROGRAMW6432") && pInfo.isSymLink() && pf32 != pf64) {
        if (QDir::toNativeSeparators(pInfo.symLinkTarget()).contains(pf32)) {
            QString path64 = QDir::toNativeSeparators(pInfo.symLinkTarget());
            path64.replace(pf32, pf64);
            if (QFileInfo(path64).exists()) {
                path = path64;
            }
        }
        else if (pInfo.symLinkTarget().contains("system32")) {
            QString path32 = QDir::toNativeSeparators(pInfo.symLinkTarget());
            if (!QFileInfo(path32).exists()) {
                path = path32.replace("system32", "sysnative");
            }
        }
    }
    return path;
}

IconProviderWin::IconProviderWin() {
//     // Load Vista/7 specific API pointers
//     HMODULE hLib = GetModuleHandleW(L"shell32");
//     if (hLib) {
//         (FARPROC&)fnSHCreateItemFromParsingName = GetProcAddress(hLib, "SHCreateItemFromParsingName");
//     }
}

IconProviderWin::~IconProviderWin() {

}

// void IconProviderWin::setPreferredIconSize(int size) {
//     m_preferredSize = size;
// }

QIcon IconProviderWin::icon(const QFileInfo& info) const {
    QIcon retIcon;
    QString fileExtension = info.suffix().toLower();

    if (fileExtension == "png"
        || fileExtension == "bmp"
        || fileExtension == "jpg"
        || fileExtension == "jpeg") {
        retIcon = QIcon(info.filePath());
    }
    else if (fileExtension == "cpl") {
        HICON hIcon;
        QString filePath = QDir::toNativeSeparators(info.filePath());
        ExtractIconEx((LPCTSTR)filePath.utf16(), 0, &hIcon, NULL, 1);
        retIcon = QIcon(QtWin::fromHICON(hIcon));
        DestroyIcon(hIcon);
    }
    else {
        // This 64 bit mapping needs to go away if we produce a 64 bit build of launchy
        QString filePath = wicon_aliasTo64(QDir::toNativeSeparators(info.filePath()));

        // Get the icon index using SHGetFileInfo
        SHFILEINFO sfi = {0};

        QRegExp re("\\\\\\\\([a-z0-9\\-]+\\\\?)?$", Qt::CaseInsensitive);
        if (re.exactMatch(filePath)) {
            // To avoid network hangs, explicitly fetch the My Computer icon for UNCs
            LPITEMIDLIST pidl;
            if (SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidl) == S_OK) {
                SHGetFileInfo((LPCTSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_SYSICONINDEX);
                // Set the file path to the My Computer GUID for any later fetches
                filePath = "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}";
            }
        }
        if (sfi.iIcon == 0) {
            SHGetFileInfo((LPCTSTR)filePath.utf16(), 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX);
        }

        // An icon index of 3 is the generic file icon
        if (sfi.iIcon > 0 && sfi.iIcon != 3) {
            // Retrieve the system image list.
            // To get the 48x48 icons, use SHIL_EXTRALARGE
            // To get the 256x256 icons (Vista only), use SHIL_JUMBO
            int imageListIndex;
            if (m_preferredSize <= 16) {
                imageListIndex = SHIL_SMALL;
            }
            else if (m_preferredSize <= 32) {
                imageListIndex = SHIL_LARGE;
            }
            else if (m_preferredSize <= 48) {
                imageListIndex = SHIL_EXTRALARGE;
            }
            else {
                imageListIndex = SHIL_JUMBO;
            }

            // If the OS supports SHCreateItemFromParsingName, get a 256x256 icon
            if (!addIconFromShellFactory(filePath, retIcon)) {
                // otherwise get the largest appropriate size
                if (!addIconFromImageList(imageListIndex, sfi.iIcon, retIcon)
                    && imageListIndex == SHIL_JUMBO) {
                    addIconFromImageList(SHIL_EXTRALARGE, sfi.iIcon, retIcon);
                }
            }

            // Ensure there's also a 32x32 icon - extralarge and above often only contain
            // a large frame with the 32x32 icon in the middle or looks blurry
            if (imageListIndex == SHIL_EXTRALARGE || imageListIndex == SHIL_JUMBO) {
                addIconFromImageList(SHIL_LARGE, sfi.iIcon, retIcon);
            }
        }
        // isSymLink is case sensitive when it perhaps shouldn't be
        else if (info.isSymLink() || fileExtension == "lnk") {
            QFileInfo targetInfo(info.symLinkTarget());
            retIcon = icon(targetInfo);
        }
        else {
            retIcon = QFileIconProvider::icon(info);
        }
    }

    return retIcon;
}

bool IconProviderWin::addIconFromImageList(int imageListIndex, int iconIndex, QIcon& icon) const {
    HICON hIcon = 0;
    IImageList* imageList;
    HRESULT hResult = SHGetImageList(imageListIndex, IID_IImageList, (void**)&imageList);
    if (hResult == S_OK) {
        hResult = imageList->GetIcon(iconIndex, ILD_TRANSPARENT, &hIcon);
        imageList->Release();
    }
    if (hResult == S_OK && hIcon) {
        icon.addPixmap(QtWin::fromHICON(hIcon));
        DestroyIcon(hIcon);
    }

    return SUCCEEDED(hResult);
}


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