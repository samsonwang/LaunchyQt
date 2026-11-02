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

#include <QApplication>

#include "CrashDumper.h"

namespace launchy {

AppWin::AppWin(int& argc, char** argv)
    : AppBase(argc, argv),
      m_crashDumper(new CrashDumper(L"Launchy")) {

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

void AppWin::sendInstanceCommand(int command) {
    UINT commandMessageId = RegisterWindowMessage(_T("LaunchyCommand"));
    PostMessage(HWND_BROADCAST, commandMessageId, command, 0);
}

// Create the application object
AppBase* createApplication(int& argc, char** argv) {
    if (qApp) {
        return AppBase::instance();
    }
    return new AppWin(argc, argv);
}

} // namespace launchy
