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

#include "CatalogSlow.h"

#include <QStringList>
#include <QDebug>

namespace launchy {

// Key of an item in CatalogSlow's position index. Items are equal when both
// fullPath and shortName match (CatItem::operator==), a control character
// separates the fields since it can not occur in a path or a file name
static QString indexKey(const CatItem& item) {
    return item.fullPath + QChar(0x1f) + item.shortName;
}

CatalogSlow::CatalogSlow()
    : Catalog() {

}

int CatalogSlow::count() {
    // Recursive lock: safe on its own (options dialog) and when save() calls
    // this while already holding m_mutex
    QMutexLocker locker(&m_mutex);

    return m_catalogItems.count();
}

void CatalogSlow::clear() {
    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    m_catalogItems.clear();
    m_index.clear();
}

void CatalogSlow::addItem(const CatItem& item) {
    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    const QString key = indexKey(item);
    auto it = m_index.find(key);

    if (it != m_index.end()) {
        if (m_timestamp > 0) {
            // Rebuild phase: refresh the item in place and keep its usage count
            const int usage = m_catalogItems[it.value()].usage;
            m_catalogItems[it.value()] = CatalogItem(item, m_timestamp);
            m_catalogItems[it.value()].usage = usage;
        }
        else {
            // While loading the catalog (timestamp 0) an already indexed item
            // is appended again, exactly like the previous linear scan did.
            // The index keeps pointing at the first occurrence
            m_catalogItems.push_back(CatalogItem(item, m_timestamp));
        }
        return;
    }

    // A new item: append it and remember where it is
    m_index.insert(key, m_catalogItems.size());
    m_catalogItems.push_back(CatalogItem(item, m_timestamp));
}

void CatalogSlow::purgeOldItems() {
    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    int removed = 0;
    for (int i = m_catalogItems.size() - 1; i >= 0; --i) {
        if (m_catalogItems.at(i).m_timestamp < m_timestamp) {
            // Don't log every removed path: a changed directory would emit
            // tens of thousands of messages while the mutex is held,
            // blocking searches in the GUI thread
            m_catalogItems.remove(i);
            ++removed;
        }
    }

    if (removed == 0) {
        return;
    }

    // Removing entries shifted the positions, rebuild the index. The first
    // occurrence wins, exactly like the linear scan used to find it
    m_index.clear();
    for (int i = 0; i < m_catalogItems.size(); ++i) {
        const QString key = indexKey(m_catalogItems[i]);
        if (!m_index.contains(key)) {
            m_index.insert(key, i);
        }
    }

    qInfo() << "CatalogSlow::purgeOldItems, removed" << removed
        << "stale items";
}

void CatalogSlow::incrementUsage(const CatItem& item) {
    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    auto it = m_index.find(indexKey(item));
    if (it == m_index.end()) {
        return;
    }

    CatalogItem& found = m_catalogItems[it.value()];
    // If an item is currently demoted, return it to a usage count of 1
    if (found.usage < 0) {
        found.usage = 1;
    }
    else {
        ++found.usage;
    }
}

void CatalogSlow::demoteItem(const CatItem& item) {
    // Prevent catalog refreshes whilst searching
    QMutexLocker locker(&m_mutex);

    auto it = m_index.find(indexKey(item));
    if (it == m_index.end()) {
        return;
    }

    CatalogItem& found = m_catalogItems[it.value()];
    // If an item is not demoted, demote it
    if (found.usage > 0) {
        found.usage = -1;
    }
    else { // otherwise demote it further
        --found.usage;
    }
}

const CatItem& CatalogSlow::getItem(int i) {
    // Recursive lock: save() calls this while already holding m_mutex
    QMutexLocker locker(&m_mutex);

    return m_catalogItems[i];
}

// Return a list of catalog items that match searchText.
// searchCatalogs() already holds m_mutex, the recursive lock keeps direct
// callers safe as well
QList<CatItem*> CatalogSlow::search(const QString& searchText) {
    QMutexLocker locker(&m_mutex);

    QList<CatItem*> result;
    if (!searchText.isEmpty()) {
        // pre-split the query once, not per item
        QStringList words = searchText.split(' ', QString::SkipEmptyParts);
        for (int i = 0; i < m_catalogItems.count(); ++i) {
            if (matchWords(&m_catalogItems[i], words)) {
                result.push_back(&m_catalogItems[i]);
            }
        }
    }

    return result;
}

} // namespace launchy
