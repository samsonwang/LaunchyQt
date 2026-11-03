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
class LaunchyApp : public SingleApplication {
public:
    // allowSecondary is -multiple: by default a second process hands its
    // request to the first one and exits, with this it runs alongside it
    // (different profiles, typically, each with its own hotkey).
    LaunchyApp(int& argc, char** argv, bool allowSecondary);
    virtual ~LaunchyApp();

    // The application object. This exists for the call sites that need the
    // QApplication itself (its signals, its modal widgets).
    static LaunchyApp* instance();
};

// Create the application object, if there is none yet. 
// allowSecondary comes from the command line (-multiple), so it has to be
// decided before this call: the single-instance channel is set up by the
// application's constructor and cannot be changed afterwards.
void initApplication(int& argc, char** argv, bool allowSecondary);

// Delete it again, next to the function that created it. A free function rather
// than a static member: the object is on its way out by then, and asking it to
// delete itself reads backwards.
void cleanupApplication();

} // namespace launchy
