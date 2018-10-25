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


#include "precompiled.h"
#include "AppWin.h"
#include "LaunchyWidget.h"
#include "WinIconProvider.h"
#include "minidump.h"

// Override the main widget to handle incoming system messages. We could have done this in the QApplication
// event handler, but then we'd have to filter out the duplicates for messages like WM_SETTINGCHANGE.
class LaunchyWidgetWin : public LaunchyWidget {
public:
    LaunchyWidgetWin(CommandFlags command)
        : LaunchyWidget(command) {
        commandMessageId = RegisterWindowMessage(_T("LaunchyCommand"));
    }

    virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result) {
        MSG* msg = (MSG*)message;
        switch (msg->message) {
        case WM_SETTINGCHANGE:
            // Refresh Launchy's environment on settings changes
            if (msg->lParam && _tcscmp((TCHAR*)msg->lParam, _T("Environment")) == 0) {
                UpdateEnvironment();
            }
            break;

        case WM_ENDSESSION:
            // Ensure settings are saved
            saveSettings();
            break;

            // Might need to capture these two messages if Vista gives any problems with alpha borders
            // when restoring from standby
        case WM_POWERBROADCAST:
            break;
        case WM_WTSSESSION_CHANGE:
            break;

        default:
            if (msg->message == commandMessageId) {
                // A Launchy startup command
                executeStartupCommand((int)msg->wParam);
            }
            break;
        }
        return LaunchyWidget::nativeEvent(eventType, message, result);
    }

private:
    UINT commandMessageId;
};

// Create the main widget for the application
LaunchyWidget* createLaunchyWidget(CommandFlags command) {
    return new LaunchyWidgetWin(command);
}

AppWin::AppWin(int& argc, char** argv)
    : AppBase(argc, argv),
    minidumper(_T("Launchy")) {
    instance = new LimitSingleInstance(_T("Local\\{ASDSAD0-DCC6-49b5-9C61-ASDSADIIIJJL}"));

    // Create local and global application mutexes so that installer knows when
    // Launchy is running
    localMutex = CreateMutex(NULL, 0, _T("LaunchyMutex"));
    globalMutex = CreateMutex(NULL, 0, _T("Global\\LaunchyMutex"));

    icons = (QFileIconProvider*)new WinIconProvider();
}

AppWin::~AppWin() {
    if (localMutex)
        CloseHandle(localMutex);
    if (globalMutex)
        CloseHandle(globalMutex);
    delete instance;
    instance = NULL;
}

void AppWin::setPreferredIconSize(int size)
{
    ((WinIconProvider*)icons)->setPreferredIconSize(size);
}

QHash<QString, QList<QString> > AppWin::getDirectories() {
    QHash<QString, QList<QString> > out;
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Launchy", "Launchy");
    QString iniFilename = settings.fileName();
    QFileInfo info(iniFilename);
    QString userDataPath = info.absolutePath();

    out["config"] << userDataPath;
    out["portableConfig"] << qApp->applicationDirPath();
    out["skins"] << qApp->applicationDirPath() + "/skins"
        << userDataPath + "/skins";
    out["plugins"] << qApp->applicationDirPath() + "/plugins"
        << userDataPath + "/plugins";
    out["defSkin"] << "Default";

    return out;
}


QList<Directory> AppWin::getDefaultCatalogDirectories() {
    QList<Directory> list;

    Directory tmp;

    tmp.name = GetShellDirectory(CSIDL_COMMON_STARTMENU);
    tmp.types << "*.lnk";
    list.append(tmp);

    tmp.name = GetShellDirectory(CSIDL_STARTMENU);
    list.append(tmp);

    tmp.name = "Utilities\\";
    tmp.indexDirs = false;
    list.append(tmp);

    Directory tmp2;
    tmp2.name = "%appdata%\\Microsoft\\Internet Explorer\\Quick Launch";
    tmp2.types << "*.*";
    list.append(tmp2);

    return list;
}


QString AppWin::expandEnvironmentVars(QString txt) {
    QString result;

    DWORD size = ExpandEnvironmentStrings((LPCWSTR)txt.utf16(), NULL, 0);
    if (size > 0) {
        TCHAR* buffer = new TCHAR[size];
        ExpandEnvironmentStrings((LPCWSTR)txt.utf16(), buffer, size);
        result = QString::fromUtf16((const ushort*)buffer);
        delete[] buffer;
    }

    return result;
}

void AppWin::sendInstanceCommand(int command) {
    UINT commandMessageId = RegisterWindowMessage(_T("LaunchyCommand"));
    PostMessage(HWND_BROADCAST, commandMessageId, command, 0);
}

bool AppWin::isAlreadyRunning() const {
    return instance->IsAnotherInstanceRunning();
}

bool AppWin::supportsAlphaBorder() const {
    return true;
}

bool AppWin::getComputers(QStringList& computers) const
{
    Q_UNUSED(computers)
        /*
        // Get a list of domains. This should return nothing or fail when we're on a workgroup
        QStringList domains;
        if (EnumerateNetworkServers(domains, SV_TYPE_DOMAIN_ENUM))
        {
            foreach(QString domain, domains)
            {
                EnumerateNetworkServers(computers, SV_TYPE_WORKSTATION | SV_TYPE_SERVER, (const TCHAR*)domain.utf16());
            }

            // If names have been retrieved from more than one domain, they'll need sorting
            if (domains.length() > 1)
            {
                computers.sort();
            }

            return true;
        }

        return EnumerateNetworkServers(computers, SV_TYPE_WORKSTATION | SV_TYPE_SERVER);
        */
        return true;
}

// Create the application object
QApplication* createApplication(int& argc, char** argv) {
    return new AppWin(argc, argv);
}
