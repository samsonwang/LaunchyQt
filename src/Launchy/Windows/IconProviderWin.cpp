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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QtWin>
#endif // QT_VERSION

#include <QProcessEnvironment>

#include "LaunchyLib/LaunchyLib.h"

#include "OptionItem.h"
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
        || fileExtension == QStringLiteral("jpeg")
        || fileExtension == QStringLiteral("ico")) {
        retIcon = QIcon(info.filePath());
    }
    else if (fileExtension == QStringLiteral("cpl")) {
        HICON hIcon;
        QString filePath = QDir::toNativeSeparators(info.filePath());
        ExtractIconExW((LPCWSTR)filePath.utf16(), 0, &hIcon, NULL, 1);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        retIcon = QIcon(QtWin::fromHICON(hIcon));
#else
        retIcon = QIcon(QPixmap::fromImage(QImage::fromHICON(hIcon)));
#endif // QT_VERSION

        DestroyIcon(hIcon);
    }
    else {

        QString filePath;
        // Whether to extract the icon from file attributes alone
        // (SHGFI_USEFILEATTRIBUTES) instead of letting SHGetFileInfo resolve and
        // access the path. It is turned on whenever touching the path would be
        // slow or could block: a shortcut whose target lives on the network, or a
        // path that itself is on the network (non-lnk file/folder, or a real
        // symlink/junction resolved onto a network drive). In that case we avoid
        // any remote lookup and just get the cached type icon.
        bool useFileAttributes = false;

        // resolve sym link target
        if (info.isSymLink()
            && g_settings->value(OPTION_RESOLVE_SYM_LINK,
                                 OPTION_RESOLVE_SYM_LINK_DEFAULT).toBool()) {

            // Read the path stored inside the shortcut itself first. Unlike
            // QFileInfo::symLinkTarget(), which asks the shell to resolve the
            // link and can therefore probe the target, this is a local read of
            // the .lnk file.
            QString rawTarget = rawLinkTarget(info);

            // A shortcut whose target lives on the network (UNC path or a drive
            // mapped to a network share) is deliberately left unresolved even
            // when OPTION_RESOLVE_SYM_LINK is on: querying the remote target for
            // its icon blocks for seconds when the host is slow or offline, and
            // the catalog build/UI would freeze with it. For those links the
            // icon is taken from the .lnk itself, which is a local, cached
            // lookup that never touches the network.
            if (!rawTarget.isEmpty() && isNetworkPath(rawTarget)) {
                filePath = QDir::toNativeSeparators(info.filePath());
                useFileAttributes = true;

                qDebug() << "IconProviderWin::icon, sym link target is a network path,"
                    << "keep the link itself, target path:" << rawTarget
                    << "file path:" << filePath;
            }
            else {
                filePath = QDir::toNativeSeparators(info.symLinkTarget());

                qDebug() << "IconProviderWin::icon, sym link, target path:" << filePath
                    << "file path:" << info.filePath();
            }
        }
        else {
            filePath = QDir::toNativeSeparators(info.filePath());
        }

        // The network-exemption above only covered .lnk shortcuts whose stored
        // target is remote. A non-lnk file or folder that already lives on the
        // network (e.g. \\server\share\app.exe passed straight in, or a real
        // NTFS symlink/junction that symLinkTarget() resolved onto a network
        // drive) would still reach SHGetFileInfo with a remote path and stall in
        // exactly the same way. Catch those here too, using the same cheap local
        // check, and fall back to the offline lookup.
        if (!useFileAttributes && isNetworkPath(filePath)) {
            useFileAttributes = true;

            qDebug() << "IconProviderWin::icon, path is a network path,"
                << "use offline icon, file path:" << filePath;
        }

        unsigned int flags = SHGFI_ICON | SHGFI_SYSICONINDEX | SHGFI_ICONLOCATION;

        DWORD attributes = FILE_ATTRIBUTE_NORMAL;

        // When the path must not be touched (its target is on the network, or the
        // path itself is on the network), tell SHGetFileInfo not to touch the
        // remote location: with SHGFI_USEFILEATTRIBUTES the icon is picked from
        // the extension alone, so resolving a shortcut to an unreachable network
        // location costs nothing. Tell it whether the item is a directory so the
        // offline lookup returns a folder icon for network folders instead of a
        // file icon.
        if (useFileAttributes) {
            flags |= SHGFI_USEFILEATTRIBUTES;
            attributes = info.isDir() ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
        }

        // Pick the smallest system image list that satisfies the requested
        // size. Fetching SHIL_JUMBO (256x256) only when a large icon is truly
        // needed avoids allocating a ~256KB pixmap for every 32-48px UI icon.
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


        qDebug() << "IconProviderWin::icon, file path:" << filePath
            << ", flags:" << flags;

        SHFILEINFOW sfi;
        ZeroMemory(&sfi, sizeof(sfi));

        // Get the icon index using SHGetFileInfo
        SHGetFileInfoW((LPCWSTR)filePath.utf16(), attributes, &sfi, sizeof(sfi), flags);
        if (sfi.hIcon) {

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            retIcon.addPixmap(QtWin::fromHICON(sfi.hIcon));
#else
            retIcon.addPixmap(QPixmap::fromImage(QImage::fromHICON(sfi.hIcon)));
#endif // QT_VERSION

            // FIX: SHGetFileInfoW hands us a freshly allocated icon handle.
            // We must DestroyIcon it, otherwise every extracted icon leaks a
            // GDI handle plus its backing bitmap (grows unbounded per session).
            DestroyIcon(sfi.hIcon);
            sfi.hIcon = NULL;
        }
        else {
            qDebug() << "IconProviderWin::icon, fail to extract by SHGetFileInfo, use qt default";
            retIcon = QFileIconProvider::icon(info);
        }
    }

    return retIcon;
}

// Reads the target path stored inside a .lnk file without letting the shell
// resolve or probe it. IShellLink::GetPath with SLGP_RAWPATH returns the literal
// string kept in the link file, whereas QFileInfo::symLinkTarget() uses
// SLGP_UNCPRIORITY and may ask the network for a preferred UNC form. Returns an
// empty string when the file is not a readable shortcut, in which case callers
// should fall back to QFileInfo::symLinkTarget().
QString IconProviderWin::rawLinkTarget(const QFileInfo& info) const {
    QString ret;

    IShellLinkW* pShellLink = NULL;
    bool neededCoInit = false;

    HRESULT hResult = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                       IID_PPV_ARGS(&pShellLink));
    if (hResult == CO_E_NOTINITIALIZED) {
        neededCoInit = true;
        CoInitialize(NULL);
        hResult = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                   IID_PPV_ARGS(&pShellLink));
    }

    if (SUCCEEDED(hResult) && pShellLink != NULL) {
        IPersistFile* pPersistFile = NULL;
        hResult = pShellLink->QueryInterface(IID_PPV_ARGS(&pPersistFile));
        if (SUCCEEDED(hResult) && pPersistFile != NULL) {
            QString linkPath = QDir::toNativeSeparators(info.filePath());
            hResult = pPersistFile->Load((LPCOLESTR)linkPath.utf16(), STGM_READ);
            if (SUCCEEDED(hResult)) {
                wchar_t szTarget[MAX_PATH] = { 0 };
                WIN32_FIND_DATAW wfd;
                ZeroMemory(&wfd, sizeof(wfd));
                if (SUCCEEDED(pShellLink->GetPath(szTarget, MAX_PATH, &wfd, SLGP_RAWPATH))) {
                    ret = QString::fromWCharArray(szTarget);
                }
            }
            pPersistFile->Release();
        }
        pShellLink->Release();
    }

    if (neededCoInit) {
        CoUninitialize();
    }

    return ret;
}

// Returns true when the path lives on the network. Only cheap checks are used
// here (a string prefix test and a local query for the drive type of "X:\"),
// because the whole point is to avoid touching a remote host: any API that is
// handed a full network path may stall for seconds when the server is slow or
// unreachable (GetDriveTypeW on "\\server\share\" alone blocks for ~20s in a
// lab test against a black-holed address).
bool IconProviderWin::isNetworkPath(const QString& path) {
    if (path.isEmpty()) {
        return false;
    }

    // UNC path, e.g. \\server\share\file.lnk or \server\share\file.lnk
    if (path.startsWith(QStringLiteral("\\")) || path.startsWith(QStringLiteral("//"))) {
        return true;
    }

    // Drive letter: ask the mount manager for the type of the volume root. The
    // root is what is passed in, never the full path, so a dead remote server is
    // not contacted: the drive type comes from the local connection table.
    if (path.length() >= 2 && path.at(1) == QLatin1Char(':')) {
        QString root = path.left(2) + QLatin1Char('\\');
        return GetDriveTypeW((LPCWSTR)root.utf16()) == DRIVE_REMOTE;
    }

    return false;
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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            icon.addPixmap(QtWin::fromHICON(hIcon));
#else
            icon.addPixmap(QPixmap::fromImage(QImage::fromHICON(hIcon)));
#endif // QT_VERSION

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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            QPixmap iconPixmap = QtWin::fromHBITMAP(hBitmap);
#else
            QPixmap iconPixmap = QPixmap::fromImage(QImage::fromHBITMAP(hBitmap));
#endif // QT_VERSION

            icon.addPixmap(iconPixmap);
            DeleteObject(hBitmap);
        }
        pSIIF->Release();
    }
    CoUninitialize();
    return hResult == S_OK;
}

} // namespace launchy
