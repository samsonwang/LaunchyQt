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

#include <QCoreApplication>

namespace launchy {

LaunchyApp::LaunchyApp(int& argc, char** argv, bool allowSecondary)
    : SingleApplication(argc, argv, allowSecondary, Mode::SecondaryNotification) {
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

void initApplication(int& argc, char** argv, bool allowSecondary) {
    if (qApp) {
        return;
    }

    // High DPI support: must be set before QApplication is constructed.
    // AA_EnableHighDpiScaling makes Qt read the system DPI scale factor (125%,
    // 150%, ...) instead of running in the old virtualized 96-DPI mode;
    // AA_UseHighDpiPixmaps makes QIcon/pixmap use their high-DPI variants.
    // Qt 5.12 does not expose setHighDpiScaleFactorRoundingPolicy() (that is
    // Qt 5.14+), so the rounding strategy falls back to the compile-time
    // default (Round). 125% DPI therefore rounds to 100% by default. If that
    // is later found insufficient, try `QT_AUTO_SCREEN_SCALE_FACTOR=1` or
    // a qt.conf [Platforms] WindowsArguments override instead of changing
    // anything else here.
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    new LaunchyApp(argc, argv, allowSecondary);
}

void cleanupApplication() {
    // main() owns the platform object and the settings, and drops both before
    // calling this -- deleting the application object is the last step of the
    // shutdown, see the teardown block at the end of main.cpp. Nothing may
    // touch qApp or a widget after this returns. Deleting a null qApp is a
    // no-op, so this is safe even when initApplication() never ran.
    delete LaunchyApp::instance();
}

} // namespace launchy
