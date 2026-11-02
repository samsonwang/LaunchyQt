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

#include "AppBase.h"

#include <QApplication>

namespace launchy {

AppBase::AppBase(int& argc, char** argv)
    : SingleApplication(argc, argv, false, Mode::SecondaryNotification) {
    setQuitOnLastWindowClosed(false);
    setApplicationName("LaunchyQt");
    setOrganizationName("LaunchyQt");
    // setOrganizationDomain("launchyqt.github.io");
}

AppBase::~AppBase() {
}

AppBase* AppBase::instance() {
    return static_cast<AppBase*>(qApp);
}

void AppBase::cleanup() {
    // The platform object is owned by main(), which drops and deletes it after
    // this returns -- see main.cpp. Deleting the application object first lets
    // the teardown run while the platform is still installed.
    delete AppBase::instance();
}

bool AppBase::isAlreadyRunning() const {
    return this->isSecondary();
}

void AppBase::sendInstanceCommand(int command) {
    Q_UNUSED(command)
}

} // namespace launchy
