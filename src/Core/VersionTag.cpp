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

#include "VersionTag.h"

namespace launchy {

// The encoding has to match LAUNCHY_VERSION in LaunchyVersion.h; a test pins
// the two together by round-tripping that macro through this function.
unsigned versionFromTag(const QString& tag) {
    QString version = tag.trimmed();
    if (version.startsWith(QLatin1Char('v')) || version.startsWith(QLatin1Char('V'))) {
        version = version.mid(1);
    }

    // Exactly three components: a shorter tag ("3.1") is not a release, and a
    // suffix ("3.1.9-beta") fails the numeric conversions below.
    if (version.count(QLatin1Char('.')) != 2) {
        return 0;
    }

    bool okMajor = false;
    bool okMinor = false;
    bool okPatch = false;
    const unsigned major = version.section(QLatin1Char('.'), 0, 0).toUInt(&okMajor);
    const unsigned minor = version.section(QLatin1Char('.'), 1, 1).toUInt(&okMinor);
    const unsigned patch = version.section(QLatin1Char('.'), 2, 2).toUInt(&okPatch);
    if (!okMajor || !okMinor || !okPatch) {
        return 0;
    }

    return major * 10000 + minor * 100 + patch;
}

} // namespace launchy
