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


#include "AppWin.h"

#include <tchar.h>
#include <ShlObj.h>
#include <LM.h>

#include "LaunchyWidgetWin.h"
#include "UtilWin.h"
#include "LaunchyWidget.h"
#include "IconProviderWin.h"
#include "CrashDumper.h"

namespace launchy {

AppWin::AppWin(int& argc, char** argv)
    : AppBase(argc, argv),
      m_crashDumper(new CrashDumper(L"Launchy")) {

    m_iconProvider = new IconProviderWin();

    // Create local and global application mutexes so that installer knows when
    // Launchy is running
    localMutex = CreateMutexW(NULL, 0, L"LaunchyMutex");
    globalMutex = CreateMutexW(NULL, 0, L"Global\\LaunchyMutex");
}

AppWin::~AppWin() {
    if (localMutex) {
        CloseHandle(localMutex);
    }
    if (globalMutex) {
        CloseHandle(globalMutex);
    }
    if (m_crashDumper) {
        delete m_crashDumper;
        m_crashDumper = nullptr;
    }
}

QHash<QString, QList<QString> > AppWin::getDirectories() {
    QHash<QString, QList<QString> > out;
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Launchy", "Launchy");
    QString iniFilename = settings.fileName();
    QFileInfo info(iniFilename);
    QString userDataPath = info.absolutePath();

    out["config"] << userDataPath;
    out["portableConfig"] << qApp->applicationDirPath() + "/config";
    out["skins"] << qApp->applicationDirPath() + "/skins"
        << userDataPath + "/skins";
    out["plugins"] << qApp->applicationDirPath() + "/plugins"
        << userDataPath + "/plugins";
    out["defSkin"] << "Default";

    return out;
}


QList<Directory> AppWin::getDefaultCatalogDirectories() {
    QList<Directory> list;

    Directory dir1;
    dir1.name = GetShellDirectory(CSIDL_COMMON_STARTMENU);
    dir1.types << "*.lnk";
    dir1.indexDirs = false;
    list.append(dir1);

    Directory dir2;
    dir2.name = GetShellDirectory(CSIDL_STARTMENU);
    dir2.types << "*.lnk";
    dir2.indexDirs = false;
    list.append(dir2);

    Directory dir3;
    dir3.name = "utilities\\";
    dir3.types << "*.lnk";
    dir3.types << "*.cmd";
    dir3.types << "*.vbs";
    dir3.indexDirs = false;
    list.append(dir3);

    /*
    Directory dir4;
    dir4.name = "%appdata%\\Microsoft\\Internet Explorer\\Quick Launch";
    dir4.types << "*.*";
    list.append(dir4);
    */

    return list;
}


QString AppWin::expandEnvironmentVars(QString txt) {
    QString result;

    DWORD size = ExpandEnvironmentStringsW((LPCWSTR)txt.utf16(), NULL, 0);
    if (size > 0) {
        WCHAR* buffer = new WCHAR[size];
        ExpandEnvironmentStringsW((LPCWSTR)txt.utf16(), buffer, size);
        result = QString::fromWCharArray(buffer);
        delete[] buffer;
        buffer = nullptr;
    }

    return result;
}

void AppWin::sendInstanceCommand(int command) {
    UINT commandMessageId = RegisterWindowMessage(_T("LaunchyCommand"));
    PostMessage(HWND_BROADCAST, commandMessageId, command, 0);
}

bool AppWin::supportsAlphaBorder() const {
    return true;
}

bool AppWin::getComputers(QStringList& computers) const {
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

// Create the application object
AppBase* createApplication(int& argc, char** argv) {
    if (qApp) {
        return g_app;
    }
    return new AppWin(argc, argv);
}

} // namespace launchy
