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

#include <QVector>
#include <QSet>
#include <QHash>
#include <QMutex>

#include "Catalog.h"
#include "LaunchyLib/CatalogItem.h"

// These classes do not pertain to plugins

namespace launchy {
/** This class does not pertain to plugins */
// The fast catalog keeps the items themselves in a QSet, so addItem()
// finds and replaces duplicates in O(1) without CatalogSlow's separate
// key -> position index. QSet hands out its elements unordered and const
// only, therefore a flat copy of the items is kept for getItem() and
// search(); it is rebuilt on demand after a change and makes the catalog
// use more memory than CatalogSlow
class CatalogFast : public Catalog {
public:
    CatalogFast();
    virtual int count();
    virtual void clear();
    virtual void addItem(const CatItem& item);
    virtual void purgeOldItems();

    virtual void incrementUsage(const CatItem& item);
    virtual void demoteItem(const CatItem& item);

protected:
    virtual const CatItem& getItem(int i);
    virtual QList<CatItem*> search(const QString& searchText);

private:
    // Refresh m_snapshot from m_catalogItems when it is stale,
    // must be called with m_mutex held
    void rebuildSnapshot();

    // Rebuild the trigram index from the current snapshot.
    // Must be called with m_mutex held.
    void rebuildTrigramIndex();

private:
    QSet<CatalogItem> m_catalogItems;
    // Flat copy of the set: save() needs getItem(i) and search() hands
    // out pointers into the catalog, neither of which a QSet supports.
    // The pointers stay valid until the catalog changes again, callers
    // use them with m_mutex held
    QVector<CatalogItem> m_snapshot;
    bool m_snapshotDirty;

    // Trigram inverted index: 3-char substring -> set of snapshot indices.
    // Built from searchName and searchNameTrans of every item. Lets
    // search() narrow the candidate set before running the full
    // subsequence test, turning an O(n) full scan into O(candidates)
    // where candidates << n for typical queries of 3+ characters.
    //
    // The index is keyed by snapshot position, so it must be rebuilt
    // whenever the snapshot changes (m_snapshotDirty implies
    // m_trigramIndexDirty).
    //
    // Trade-off: the index finds items that contain every query trigram
    // as a contiguous substring. Subsequence-only matches whose query
    // characters are spread too far apart to share any contiguous trigram
    // are not returned. In practice these are extremely rare and rank
    // lowest anyway (CatItemComparePtr ranks contiguous-substring matches far
    // above subsequence-only matches).
    QHash<QString, QSet<int>> m_trigramIndex;
    bool m_trigramIndexDirty;
};

// QSet<CatalogItem> needs a hash that is consistent with the equality
// used by the set, see CatItem::operator== (fullPath and shortName)
inline uint qHash(const CatalogItem& item, uint seed = 0) {
    return qHash(item.shortName, qHash(item.fullPath, seed));
}

} // namespace launchy
