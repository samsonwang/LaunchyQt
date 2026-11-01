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

#include <QString>

// g_searchText and the NAME_*/LABEL_* item identities moved to LaunchyCore
// (see Core/CoreGlobals.h) because the catalog and the history store use them.
// Re-exported here so the application sources keep compiling without a second
// include.
#include "CoreGlobals.h"

namespace launchy {

// Tear the application's process-wide objects down in dependency order. Called
// once, after the event loop returns.
void cleanupGlobalVar();

} // namespace launchy
