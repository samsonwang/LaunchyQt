/*
Launchy: Application Launcher
Copyright (C) 2009  Simon Capewell

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

#include "LaunchyLib/CatalogItem.h"

#include "InputDataList.h"

namespace launchy {

// Persistent history store, everything lives in a single json file:
//   - "commands":  the launched inputs (InputDataList entries) that make up
//     the history rows and the history search
//   - "promotion": "query -> item chosen last time the exact query ran",
//     used by Catalog to move that item back to the top of the results.
//     Front of the list is the most recently used entry, the list is capped
//     (LRU) and validated on load.
class CommandHistory {
public:
    CommandHistory();

public:
    bool load(const QString& filename);
    void save(const QString& filename) const;

    void addItem(const InputDataList& item);
    void removeAt(int index);

    // Number of stored entries, use it to validate an index before calling getItem()
    int getItemCount() const;

    // Returns the entry stored at index, or an empty InputDataList when index is out
    // of range. An assert used to guard this, but it is compiled out under NDEBUG
    // while the callers still reach here with a stale index, so the check has to be
    // a real runtime one. Never trust a CatItem::data index that came from an older
    // population of the alternatives list: the history can shrink in between.
    InputDataList getItem(int index) const;

    void getAllItem(QList<CatItem>& searchResults) const;
    void search(const QString& text, QList<CatItem>& searchResults) const;

    // Look up the item that was chosen the last time the exact query ran.
    // Returns false when there is no promotion entry for the query.
    bool promotedItem(const QString& query,
                      QString& shortName, QString& fullPath) const;

    // Record that `shortName`/`fullPath` was chosen for `query`, moving the
    // entry to the front of the LRU list. Persists immediately.
    void setPromotedItem(const QString& query,
                         const QString& shortName, const QString& fullPath);

private:
    struct PromotedItem {
        QString query;
        QString shortName;
        QString fullPath;
    };

    // Write the store to the file last passed to load(). Every mutation
    // persists immediately so a crash cannot lose data.
    void persist() const;

private:
    // Front is the most recently launched input
    QList<InputDataList> m_command;

    // Front is the most recently used promotion entry
    QList<PromotedItem> m_promotion;

    // File last passed to load(), used by persist()
    QString m_filename;
};

// The widget owned history store, set in the LaunchyWidget constructor and
// cleared in its destructor. Catalog needs the promotion data and has no
// other way to reach the store (it used to go through g_settings).
extern CommandHistory* g_history;

} // namespace launchy
