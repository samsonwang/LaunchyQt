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

#include "PlatformWin.h"

#include <windows.h>
#include <tchar.h>
#include <ShlObj.h>
#include <LM.h>
#include <shellapi.h>

#include <QSettings>
#include <QFileInfo>
#include <QCoreApplication>
#include <QProcessEnvironment>
#include <QDir>
#include <QDebug>

#include "UtilWin.h"
#include "Directory.h"
#include "CrashDumper.h"

namespace launchy {

// 64-bit shortcuts do not resolve correctly from a 32-bit executable; rewrite
// the "Program Files" symlink target to the 64-bit location when that exists.
static QString aliasTo64(const QString& path) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString pf32 = env.value("PROGRAMFILES");
    QString pf64 = env.value("PROGRAMW6432");

    QFileInfo pInfo(path);

    if (env.contains("PROGRAMW6432") && pInfo.isSymLink() && pf32 != pf64) {
        if (QDir::toNativeSeparators(pInfo.symLinkTarget()).contains(pf32)) {
            QString path64 = QDir::toNativeSeparators(pInfo.symLinkTarget());
            path64.replace(pf32, pf64);
            if (QFileInfo(path64).exists()) {
                return path64;
            }
        }
        else if (pInfo.symLinkTarget().contains("system32")) {
            QString path32 = QDir::toNativeSeparators(pInfo.symLinkTarget());
            if (!QFileInfo(path32).exists()) {
                return QString(path32).replace("system32", "sysnative");
            }
        }
    }
    return path;
}

// ShellExecuteEx reports failure only through its return value, so the message
// has to be built by hand: without it a failed launch is indistinguishable from
// a successful one where the target app dies on its own.
static QString win32ErrorText(DWORD error) {
    WCHAR* buffer = nullptr;
    DWORD length = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                  | FORMAT_MESSAGE_FROM_SYSTEM
                                  | FORMAT_MESSAGE_IGNORE_INSERTS,
                                  NULL, error,
                                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                  (LPWSTR)&buffer, 0, NULL);
    if (length == 0 || buffer == nullptr) {
        return QString();
    }

    QString text = QString::fromWCharArray(buffer, length).trimmed();
    LocalFree(buffer);
    return text;
}

// The crash dumper is installed here, in the constructor, so that the whole
// life of the process is covered from the moment the platform is installed --
// initPlatform() is called before any settings are read or any plugin is
// loaded. It is created even in a debug build, where CrashDumper decides on its
// own not to touch the exception filter.
//
// The name is the one the dump folder and the crash dialog have always used
// ("%TEMP%/Launchy/..."), deliberately not QCoreApplication::applicationName(),
// which is "LaunchyQt" and would send the dumps to a different directory.
PlatformWin::PlatformWin()
    : m_crashDumper(new CrashDumper(L"Launchy")) {
}

PlatformWin::~PlatformWin() {
}

void PlatformWin::runProgram(const QString& file, const QString& args,
                                  bool translateSeparators) const {
    qDebug() << "PlatformWin::runProgram, file:" << file
             << "args:" << args << "separator:" << translateSeparators;

    QString path = aliasTo64(file);

    // Explicit wide-char variant (ShellExecuteExW): this file does not rely on
    // the UNICODE macro (the rest of it already uses the ...W APIs directly),
    // so use the W struct and LPCWSTR casts to stay consistent.
    SHELLEXECUTEINFOW ShExecInfo;
    ZeroMemory(&ShExecInfo, sizeof(ShExecInfo));
    bool elevated = (GetKeyState(VK_SHIFT) & 0x80000000) != 0
        && (GetKeyState(VK_CONTROL) & 0x80000000) != 0;

    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
    // SEE_MASK_NOCLOSEPROCESS: the handle is the only proof the shell really
    // created a process, and its pid is what tells "the app was started but did
    // not show a window" apart from "nothing was started at all".
    ShExecInfo.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = elevated ? L"runas" : NULL;

    QString filePath = translateSeparators ? QDir::toNativeSeparators(path) : path;
    ShExecInfo.lpFile = (LPCWSTR)filePath.utf16();

    if (!args.isEmpty()) {
        ShExecInfo.lpParameters = (LPCWSTR)args.utf16();
    }
    else {
        ShExecInfo.lpParameters = NULL;
    }

    QDir dir(path);
    QFileInfo info(path);
    if (!info.isDir() && info.isFile())
        dir.cdUp();
    QString directory = QDir::toNativeSeparators(dir.absolutePath());
    ShExecInfo.lpDirectory = (LPCWSTR)directory.utf16();
    ShExecInfo.nShow = SW_NORMAL;
    ShExecInfo.hInstApp = NULL;

    if (!ShellExecuteExW(&ShExecInfo)) {
        DWORD error = GetLastError();
        qWarning() << "PlatformWin::runProgram, ShellExecuteEx failed,"
                   << "file:" << filePath
                   << "directory:" << directory
                   << "elevated:" << elevated
                   << "error:" << error
                   << "(" << win32ErrorText(error) << ")";
        return;
    }

    // A null handle is normal for a shell verb that hands over to an already
    // running instance, so it is logged rather than treated as a failure.
    DWORD pid = (ShExecInfo.hProcess != NULL) ? GetProcessId(ShExecInfo.hProcess) : 0;
    qDebug() << "PlatformWin::runProgram, launched, pid:" << pid
             << "file:" << filePath << "directory:" << directory;

    if (ShExecInfo.hProcess != NULL) {
        CloseHandle(ShExecInfo.hProcess);
    }
}

QList<Directory> PlatformWin::defaultCatalogDirectories() const {
    QList<Directory> list;

    Directory dir;
    dir.types << "*.lnk";
    dir.indexDirs = false;

    dir.name = GetShellDirectory(CSIDL_COMMON_STARTMENU);
    list.append(dir);

    dir.name = GetShellDirectory(CSIDL_STARTMENU);
    list.append(dir);

    dir.name = GetShellDirectory(CSIDL_COMMON_DESKTOPDIRECTORY);
    dir.indexExe = true;
    dir.depth = 1;
    list.append(dir);

    dir.name = GetShellDirectory(CSIDL_DESKTOPDIRECTORY);
    dir.indexExe = true;
    dir.depth = 1;
    list.append(dir);

    dir.name = GetShellDirectory(CSIDL_RECENT);
    dir.indexDirs = false;
    dir.indexExe = false;
    dir.depth = 1;
    list.append(dir);

    Directory dir2;
    dir2.name = "utilities\\";
    dir2.types << "*.lnk";
    dir2.types << "*.cmd";
    dir2.types << "*.vbs";
    dir2.indexDirs = false;
    list.append(dir2);

    return list;
}

QHash<QString, QList<QString> > PlatformWin::directories() const {
    QHash<QString, QList<QString> > out;
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Launchy", "Launchy");
    QString iniFilename = settings.fileName();
    QFileInfo info(iniFilename);
    QString userDataPath = info.absolutePath();

    // applicationDirPath() is Qt Core (QCoreApplication), not QApplication:
    // this class answers with where the executable lives without reaching for
    // the application object, which keeps this library free of QApplication.
    const QString appDir = QCoreApplication::applicationDirPath();

    out["config"] << userDataPath;
    out["portableConfig"] << appDir + "/config";
    out["skins"] << appDir + "/skins"
        << userDataPath + "/skins";
    out["plugins"] << appDir + "/plugins"
        << userDataPath + "/plugins";

    return out;
}

QString PlatformWin::expandEnvironmentVars(const QString& vars) const {
    QString result;

    DWORD size = ExpandEnvironmentStringsW((LPCWSTR)vars.utf16(), NULL, 0);
    if (size > 0) {
        WCHAR* buffer = new WCHAR[size];
        ExpandEnvironmentStringsW((LPCWSTR)vars.utf16(), buffer, size);
        result = QString::fromWCharArray(buffer);
        delete[] buffer;
        buffer = nullptr;
    }

    return result;
}

bool PlatformWin::supportsAlphaBorder() const {
    return true;
}

bool PlatformWin::getComputers(QStringList& computers) const {
    // Get a list of domains. This should return nothing or fail when we're on a workgroup
    QStringList domains;
    if (EnumerateNetworkServers(domains, SV_TYPE_DOMAIN_ENUM)) {
        foreach(QString domain, domains) {
            EnumerateNetworkServers(computers, SV_TYPE_WORKSTATION | SV_TYPE_SERVER, (const WCHAR*)domain.utf16());
        }

        // If names have been retrieved from more than one domain, they'll need sorting
        if (domains.length() > 1) {
            computers.sort();
        }

        return true;
    }

    return EnumerateNetworkServers(computers, SV_TYPE_WORKSTATION | SV_TYPE_SERVER);
}

bool PlatformWin::allowNotification() const {

    QUERY_USER_NOTIFICATION_STATE state;
    if (SHQueryUserNotificationState(&state) == S_OK) {
        return state == QUNS_ACCEPTS_NOTIFICATIONS;
    }

    return true;
}

int PlatformWin::systemIdleSeconds() const {
    LASTINPUTINFO lastInputInfo;
    lastInputInfo.cbSize = sizeof(LASTINPUTINFO);
    if (!GetLastInputInfo(&lastInputInfo)) {
        return -1;
    }

    // both values come from GetTickCount() and wrap around together,
    // so the unsigned subtraction stays correct after the 49.7 day wrap
    DWORD idleMillis = GetTickCount() - lastInputInfo.dwTime;
    return static_cast<int>(idleMillis / 1000);
}

// Nothing to rewrite on Windows: a .lnk already carries its name, icon and
// target, and the shortcut handler reads them as it indexes.
void PlatformWin::alterItem(CatItem* item) const {
    Q_UNUSED(item);
}

} // namespace launchy
