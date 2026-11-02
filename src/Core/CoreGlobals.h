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

// Process-wide model constants. Kept in one header so they are visible in a
// single place instead of being spread over the application sources. Nothing
// here is mutable state any more: the query text used for result decoration is
// owned by LaunchyWidget and injected into the delegate (IconDelegate::
// setSearchText), because a writable global in a STATIC library would give every
// linker its own copy.

namespace launchy {

// Catalog item provenance. CommandHistory stamps the rows it injects with
// these, and the UI decides what to do with a row from their values, so they
// belong to the model rather than to either side.
extern const QString NAME_LAUNCHY;
extern const QString NAME_HISTORY;
extern const QString NAME_LAUNCHYFILE;

// Labels carried by an input segment to say "this segment is a file" or
// "this segment came from the history".
extern const QString LABEL_FILE;
extern const QString LABEL_AUTOSUGGEST;
extern const QString LABEL_HISTORY;

} // namespace launchy
