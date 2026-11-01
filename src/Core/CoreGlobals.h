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

// Process-wide state that the core algorithms read. Kept in one header so the
// remaining globals are visible in a single place instead of being spread over
// the application sources -- every entry here is one that a refactor still
// owes an injection seam.

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

// Lower-cased text of the query being searched. The UI writes it before
// ranking, CatItemCompare() reads it. The comparator variant that takes the
// query as a parameter (CatItemCompareWithMatch) exists precisely so new code
// does not have to touch this.
extern QString g_searchText;

} // namespace launchy
