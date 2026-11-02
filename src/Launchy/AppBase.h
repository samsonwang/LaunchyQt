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

#pragma once

#include <SingleApplication/SingleApplication>

namespace launchy {

// The application object: the QApplication (via SingleApplication, which also
// gives the single-instance channel).
//
// It holds no platform knowledge at all. The platform questions -- including
// the icons, which used to be cached here -- are answered by a separate platform
// object (PlatformWin / PlatformLinux) that lives in the LaunchyPlatform library
// and knows nothing about QApplication; main() installs it through
// initPlatform(). Keeping the application object in the executable is what lets
// the platform library stay free of QApplication.
class AppBase : public SingleApplication {
public:
    AppBase(int& argc, char** argv);
    virtual ~AppBase();

public:
    static void cleanup();

    // The application object. This exists for the call sites that need the
    // QApplication itself (its signals, its modal widgets).
    static AppBase* instance();

public:
    virtual bool isAlreadyRunning() const;
    virtual void sendInstanceCommand(int command);
};

// Create the application object. Returns the existing one when there is
// already a QApplication.
AppBase* createApplication(int& argc, char** argv);

} // namespace launchy
