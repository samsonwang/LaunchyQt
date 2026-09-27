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
#include <QMutex>

#include "LaunchyLib/CatalogItem.h"

// These classes do not pertain to plugins

namespace launchy {
// Catalog provides methods to search and manage the indexed items
class Catalog {
public:
    Catalog();
    virtual ~Catalog();
    bool load(const QString& filename);
    bool save(const QString& filename);
    void incrementTimestamp();
    void searchCatalogs(const QString& text, QList<CatItem>& result);
    void promoteRecentlyUsedItems(const QString& text, QList<CatItem>& list);

    virtual int count() = 0;
    virtual void clear() = 0;
    virtual void addItem(const CatItem& item) = 0;
    virtual void purgeOldItems() = 0;

    virtual void incrementUsage(const CatItem& item) = 0;
    virtual void demoteItem(const CatItem& item) = 0;

    static bool matches(CatItem* item, const QString& match);
    static QString decorateText(const QString& text, const QString& match, bool outputRichText = false);

protected:
    virtual const CatItem& getItem(int) = 0;
    virtual QList<CatItem*> search(const QString&) = 0;

    int m_timestamp;
    QMutex m_mutex;
};


// CatalogItem is used internally to store additional
class CatalogItem : public CatItem {
public:
    CatalogItem();
    CatalogItem(const CatItem& item, int time);

    int m_timestamp;
};


// QSet<CatalogItem> needs a hash that is consistent with the equality
// used by the set, see CatItem::operator== (fullPath and shortName)
inline uint qHash(const CatalogItem& item, uint seed = 0) {
    return qHash(item.shortName, qHash(item.fullPath, seed));
}


/** This class does not pertain to plugins */
// The slow catalog searches slowly but
// adding items is fast and uses less memory
// than FastCatalog
class SlowCatalog : public Catalog {
public:
    SlowCatalog();
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
    QVector<CatalogItem> m_catalogItems;
};


/** This class does not pertain to plugins */
// The fast catalog keeps the items themselves in a QSet, so addItem()
// finds and replaces duplicates in O(1) without SlowCatalog's separate
// key -> position index. QSet hands out its elements unordered and const
// only, therefore a flat copy of the items is kept for getItem() and
// search(); it is rebuilt on demand after a change and makes the catalog
// use more memory than SlowCatalog
class FastCatalog : public Catalog {
public:
    FastCatalog();
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

private:
    QSet<CatalogItem> m_catalogItems;
    // Flat copy of the set: save() needs getItem(i) and search() hands
    // out pointers into the catalog, neither of which a QSet supports.
    // The pointers stay valid until the catalog changes again, callers
    // use them with m_mutex held
    QVector<CatalogItem> m_snapshot;
    bool m_snapshotDirty;
};

bool CatLessPtr(CatItem* left, CatItem* right);
bool CatLessRef(CatItem& left, CatItem& right);

} // namespace launchy
