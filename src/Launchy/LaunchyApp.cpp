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

#include "LaunchyApp.h"

#include <QApplication>

namespace launchy {

LaunchyApp::LaunchyApp(int& argc, char** argv)
    : SingleApplication(argc, argv, false, Mode::SecondaryNotification) {
    setQuitOnLastWindowClosed(false);
    setApplicationName("LaunchyQt");
    setOrganizationName("LaunchyQt");
    // setOrganizationDomain("launchyqt.github.io");
}

LaunchyApp::~LaunchyApp() {
}

LaunchyApp* LaunchyApp::instance() {
    return static_cast<LaunchyApp*>(qApp);
}

void cleanupApplication() {
    // The platform object is owned by main(), which drops and deletes it after
    // this returns -- see main.cpp. Deleting the application object first lets
    // the teardown run while the platform is still installed. Deleting a null
    // qApp is a no-op, so this is safe even when initApplication() never ran.
    delete LaunchyApp::instance();
}

// No per-platform application class any more: AppWin/AppLinux were empty
// subclasses once the crash dumper moved to PlatformWin and the installer
// mutexes and the "LaunchyCommand" broadcast went away. What is left is the
// QApplication itself, which is the same on every platform, so it is built
// here and main() does not have to care which OS it is on.
//
// The new object installs itself as qApp (QApplication's constructor does that),
// so nothing is returned: every call site reaches it through qApp or
// instance() anyway. Already having one is not an error -- initApplication()
// simply leaves it alone.
void initApplication(int& argc, char** argv) {
    if (qApp) {
        return;
    }
    new LaunchyApp(argc, argv);
}

} // namespace launchy
