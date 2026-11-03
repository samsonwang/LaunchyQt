/*
Launchy: Application Launcher
Copyright (C) 2007-2010  Josh Karlin, Simon Capewell

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

#include <QList>
#include <QMap>
#include <QString>

#include "Core/Directory.h"

namespace launchy {

class CatItem;

// Everything the domain layer has to ask the host platform, and nothing else.
//
// Deliberately small and Qt Core only: no QIcon, no QWidget, no QApplication.
// This file belongs to the platform library, next to the implementations it
// describes (Windows/PlatformWin, Linux/PlatformLinux): one object gets created
// by initPlatform() before anything reads settings or builds a catalog, and
// everyone else reaches it through platform(), never through a concrete class.
// That is what keeps CatalogBuilder and the settings code testable without a
// GUI -- and keeping the seam in the same library as the factory is what lets
// the installed pointer stay private to Platform.cpp.
class PlatformBase {
public:
    virtual ~PlatformBase() {}

    // Roots of every runtime directory, keyed by role: "config",
    // "portableConfig", "skins", "plugins". SettingsManager resolves all of
    // its paths from these. A QMap rather than a QHash: the roles are a small
    // fixed set, the lookup cost is irrelevant, and a sorted, repeatable
    // iteration order makes anything that walks the roles deterministic.
    virtual QMap<QString, QList<QString>> directories() const = 0;

    // Directories indexed when the catalog is created for the first time
    virtual QList<Directory> defaultCatalogDirectories() const = 0;

    // Expand %VAR% / $VAR references in a configured path
    virtual QString expandEnvironmentVars(const QString& vars) const = 0;

    // Let the platform rewrite an item as it is indexed (e.g. .desktop files)
    virtual void alterItem(CatItem* item) const = 0;

    // Whether the window manager allows a per-pixel alpha border
    virtual bool supportsAlphaBorder() const = 0;

    // Network neighbourhood servers, false when the platform cannot list them
    virtual bool getComputers(QStringList& computers) const = 0;

    // Whether a notification would disturb the user right now
    virtual bool allowNotification() const = 0;

    // Seconds since the last keyboard/mouse input, negative when unknown
    virtual int systemIdleSeconds() const = 0;

    // Run a program with arguments, platform-independent of how that is done
    // (ShellExecuteEx on Windows, xdg-open/kstart on Linux). translateSeparators
    // rewrites '/' into the native separator first.
    virtual void runProgram(const QString& file, const QString& args,
                            bool translateSeparators = true) const = 0;
};

// The installed platform: installed once by initPlatform(), read by everything
// else through here. Falls back to the inert platform while nothing is
// installed, so a core consumer never has to check.
//
// A process-wide seam rather than a constructor parameter because the
// consumers (SettingsManager, CatalogBuilder, the catalog worker thread) are
// created before there is anyone to pass it in from, and threading it through
// every call site would touch far more code than the dependency does. That is
// also its limit: nothing outside this library implements this interface, so it
// can never grow into a general service locator.
PlatformBase& platform();

// Create this build's platform and install it. Call it once, first thing in
// main(), before anything reads settings or builds a catalog.
//
// The installed pointer stays a plain file-local in Platform.cpp: this header
// declares no variable, and nothing outside this library can replace what is
// installed. Which class gets built is decided there too, by the one conditional
// that selects between the platform implementations.
void initPlatform();

// Delete what initPlatform() created and fall back to the inert platform. Every
// consumer has to be gone by now: anything still asking a question afterwards
// gets the inert answers, not the real ones. Fine to call when nothing was
// installed, which is what lets a test reset the seam between cases.
void cleanupPlatform();

} // namespace launchy
