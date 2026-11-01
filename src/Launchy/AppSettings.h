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

namespace launchy {

// Apply the values SettingsManager read but could not act on: the widget
// style, the application proxy, the language and debug logging.
//
// The split is deliberate. Core decides what the settings *are* (it can be
// tested without a GUI), the application decides what they *do*, because
// applying them needs QStyleFactory, QNetworkProxy and the translation
// manager -- three things core must not link.
void applyRuntimeSettings();

} // namespace launchy
