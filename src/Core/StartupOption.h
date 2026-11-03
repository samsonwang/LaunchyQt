/*
LaunchyQt
Copyright (C) 2019 Samson Wang

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <QString>
#include <QStringList>

namespace launchy {

// What the command line asked for, read once at the top of main() before a
// QApplication, the settings or the platform exist. Two of these decisions have
// to be known that early: -multiple decides whether the application object lets
// a second process run at all, and -profile decides which ini file the settings
// are read from.

struct StartupOption {
    StartupOption();

    bool rescue;      // -rescue: reset the skin and the position, then show
    bool show;        // -show: show the launcher
    bool options;     // -options: open the options dialog
    bool rescan;      // -rescan: rebuild the catalog on start
    bool exit;        // -exit: quit instead of running
    bool debug;       // -debug: write debug output to launchy.log
    bool multiple;    // -multiple: start even if another instance is running
    QString profile;  // -profile <name>, empty when not given
};

// Parse the command line main() received, program path included at index 0.
//
// Only the "-name" spelling is an option: a leading '/' is not an option prefix
// and such an argument is left as a positional one (ignored). Names are matched
// case-sensitively, so "-Show" is not "-show".
//
// A command line the parser does not understand is reported through qWarning and
// the launcher still starts with whatever it did understand: it comes from a
// shortcut or a script, where failing to start looks like a hung launch.
StartupOption parseStartupOption(const QStringList& arguments);

} // namespace launchy
