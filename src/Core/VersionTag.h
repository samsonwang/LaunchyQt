/*
LaunchyQt
Copyright (C) 2018 Samson Wang

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

namespace launchy {

// Encode a release tag into the integer form LAUNCHY_VERSION uses
// (MAJOR * 10000 + MINOR * 100 + PATCH), so a tag from the update feed can be
// compared with the running build's version directly.
//
// Returns 0 when the tag is not a plain "[v]MAJOR.MINOR.PATCH" release
// ("3.1", "v3.1.9-rc1", "release", ...). Callers treat 0 as "not a release we
// understand" rather than "older than us": a pre-release tag says nothing
// about the version the user is running.
unsigned versionFromTag(const QString& tag);

} // namespace launchy
