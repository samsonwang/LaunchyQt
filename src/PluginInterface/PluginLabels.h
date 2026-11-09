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

#include <QString>

// Public label / owner constants shared by Launchy core and every plugin.
//
// They live here (PluginInterface, Qt Core only) because PluginInterface is the
// plugin ABI layer -- every plugin, every native one, every python one via
// launchy.pyd, every unit test in test/ links PluginInterface -- so this single
// header is reachable from all of them without pulling in a GUI or platform
// dependency. The values are string literals, so const QString with
// QStringLiteral in the header gives every consumer its own internal-linkage
// copy; the strings are identical and can never diverge.
//
// Plugin-private labels (e.g. Verby's "verby_file", WebSearchPy's "LABEL_WEBSITE")
// stay inside their own plugin source -- they are not shared model constants
// and should not be public.

namespace launchy {

// CatItem::pluginName owner identifiers. Launchy stamps the items it owns
// (file browse-as-you-type, history) with these names so the UI and downstream
// plugins can tell them apart from items added by other plugins. The indexed
// catalog deliberately leaves pluginName empty, so anything that wants to match
// all Launchy-owned items must check both the empty string and NAME_LAUNCHY.
// Verby::isLaunchyOwnedItem() used to hardcode the values here; plugins should
// always prefer these constants so they stay in sync.

/// Owner of file browse-as-you-type items produced by FileSearch.
const QString NAME_LAUNCHY = QStringLiteral("launchy");

/// Owner of history rows injected by CommandHistory.
const QString NAME_HISTORY = QStringLiteral("launchy_history");


// InputData label strings. Plugins tag an InputData segment with a label in
// MSG_GET_LABELS and act on that label in MSG_GET_RESULTS -- they are the
// communication channel between plugins on a single keystroke pass. These
// three are Launchy-internal labels that plugins may want to read (e.g. Verby
// looks for LABEL_FILE on the first segment), so they belong here and not in
// Core. A segment's labels are transient: InputDataList::parse() clears them
// on the next keystroke, so they never outlive one MSG_GET_LABELS /
// MSG_GET_RESULTS pair.

/// Label set by FileSearch when a segment is a file path.
const QString LABEL_FILE = QStringLiteral("launchy_file");

/// Label set by LaunchyWidget for the auto-suggest first segment.
const QString LABEL_AUTOSUGGEST = QStringLiteral("launchy_auto_suggest");

/// Label set by CommandHistory when replaying a history entry.
const QString LABEL_HISTORY = QStringLiteral("launchy_history");

} // namespace launchy
