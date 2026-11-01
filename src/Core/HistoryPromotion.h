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

namespace launchy {

// The catalog needs exactly one fact from the history store: which item the
// user picked the last time this exact query ran, so it can move that item
// back to the top of the results.
//
// CommandHistory (the persistent store) lives in the application and knows
// about input parsing, settings and files. This interface is the seam that
// lets the catalog ask for a promotion without depending on any of that --
// which is what keeps the ranking code linkable into a headless test.
// The application installs its CommandHistory with
// Catalog::setHistoryPromotion().
class IHistoryPromotion {
public:
    virtual ~IHistoryPromotion() {}

    // True when a promotion is stored for this exact query text.
    // 'shortName' and 'fullPath' identify the item to move to the front.
    virtual bool promotedItem(const QString& text, QString& shortName,
                              QString& fullPath) const = 0;
};

} // namespace launchy
