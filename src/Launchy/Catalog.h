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

#include <QList>
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

    // Callers must pass words that are already split on whitespace and already lowercased.
    static bool matchWords(const CatItem* item, const QStringList& lowerWords);
    static QString decorateText(const QString& text, const QString& match, bool outputRichText = false);

protected:
    virtual const CatItem& getItem(int) = 0;
    virtual QList<CatItem*> search(const QString&) = 0;

    int m_timestamp;
    // Guards the catalog data. The lock is recursive: the compound operations
    // (searchCatalogs, save) hold it while calling the primitives (count,
    // getItem, search) and the primitives take it again on their own. With a
    // non-recursive mutex the calling thread deadlocked itself, which froze
    // the search window: a background rebuild died inside save() while still
    // holding the lock, so the next search blocked forever
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QRecursiveMutex m_mutex;
#else
    QMutex m_mutex{QMutex::Recursive};
#endif
};


// CatalogItem is used internally to store additional
class CatalogItem : public CatItem {
public:
    CatalogItem();
    CatalogItem(const CatItem& item, int time);

    int m_timestamp;
};


bool CatItemComparePtr(CatItem* left, CatItem* right);
bool CatItemCompareRef(CatItem& left, CatItem& right);

} // namespace launchy
