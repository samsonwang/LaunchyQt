/*
LaunchyQt
Copyright (C) 2018 Samson Wang

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <QtGlobal>

namespace launchy {

// Pinyin initial of one UTF-16 code unit, for CatItem::convertSearchName().
//
// This header deliberately exposes a FUNCTION and no objects. The table and its
// bounds are `const` objects at namespace scope, which default to INTERNAL
// linkage in C++ -- a header can only hand those out while its `extern`
// declaration happens to precede the definition inside the same translation
// unit. That coupling is invisible: reorder the includes in UnicodeTable.cpp
// and every consumer starts failing to link with an undefined-symbol error that
// points at the consumer, not at the reorder. A function has external linkage by
// its own declaration, so nothing here can rot that way, and the data itself
// never leaves UnicodeTable.cpp.
//
// Returns false and leaves `initial` untouched when `code` is outside the
// covered simplified Chinese block (U+4E00..U+9FA5).
//
// Internal to PluginInterface: not exported from Launchy.dll, callers outside
// this DLL must go through CatItem::convertSearchName().
bool zhCNInitial(ushort code, char& initial);

} // namespace launchy
