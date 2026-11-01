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

#include "CatalogFast.h"

#include <QDebug>
#include <QStringList>
#include <utility>

namespace launchy {

// Extract all contiguous 3-character substrings (trigrams) from text.
// Returns an empty list if text has fewer than 3 characters.
// Used by CatalogFast::search() to look up candidate items via the
// trigram inverted index.
static QStringList extractTrigrams(const QString& text) {
    QStringList trigrams;
    int len = text.size();
    if (len < 3)
        return trigrams;
    trigrams.reserve(len - 2);
    for (int i = 0; i + 3 <= len; ++i) {
        trigrams.append(text.mid(i, 3));
    }
    return trigrams;
}

CatalogFast::CatalogFast()
    : Catalog(),
      m_snapshotDirty(true),
      m_trigramIndexDirty(true) {

}

int CatalogFast::count() {
    QMutexLocker locker(&m_mutex);

    return m_catalogItems.count();
}

void CatalogFast::clear() {
    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    m_catalogItems.clear();
    m_snapshot.clear();
    m_snapshotDirty = true;
    m_trigramIndex.clear();
    m_trigramIndexDirty = true;
}

void CatalogFast::addItem(const CatItem& item) {
    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    // The set itself provides the duplicate detection that CatalogSlow
    // needs a key -> position index for: only fullPath and shortName are
    // hashed and compared (see qHash in Catalog.h), so an equal item is
    // found in O(1) even while a rebuild adds tens of thousands of items
    const CatalogItem stored(item, m_timestamp);
    auto it = m_catalogItems.find(stored);

    if (it != m_catalogItems.end()) {
        if (m_timestamp > 0) {
            // Rebuild phase: refresh every field of the existing item but
            // keep its usage count, exactly like CatalogSlow does.
            // QSet::insert keeps the old key object of an equal item, so
            // the stale entry has to be removed before inserting again
            CatalogItem refreshed = stored;
            refreshed.usage = it->usage;
            m_catalogItems.erase(it);
            m_catalogItems.insert(refreshed);
            m_snapshotDirty = true;
            m_trigramIndexDirty = true;
        }
        // While loading the catalog (timestamp == 0) the first entry of
        // an equal pair wins, the set cannot keep the duplicates that
        // CatalogSlow appends in this phase
        return;
    }

    m_catalogItems.insert(stored);
    m_snapshotDirty = true;
    m_trigramIndexDirty = true;
}

void CatalogFast::purgeOldItems() {
    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    // Collect the stale items first, removing entries while iterating
    // the set would invalidate the iterator
    QSet<CatalogItem> stale;
    for (const CatalogItem& item : m_catalogItems) {
        if (item.m_timestamp < m_timestamp) {
            // Don't log every removed path: a changed directory would emit
            // tens of thousands of messages while the mutex is held,
            // blocking searches in the GUI thread
            stale.insert(item);
        }
    }

    const int removed = stale.count();
    if (removed == 0) {
        return;
    }
    for (const CatalogItem& item : stale) {
        m_catalogItems.remove(item);
    }
    // Give the memory of the stale entries back to the system
    m_catalogItems.squeeze();
    m_snapshotDirty = true;
    m_trigramIndexDirty = true;
    qInfo() << "CatalogFast::purgeOldItems, removed" << removed
        << "stale items";
}

void CatalogFast::incrementUsage(const CatItem& item) {
    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    // Only fullPath and shortName take part in the comparison
    auto it = m_catalogItems.find(CatalogItem(item, 0));
    if (it == m_catalogItems.end()) {
        return;
    }

    // The set hands out its elements const only, so replace the entry
    // with an updated copy instead of modifying it in place
    CatalogItem updated = *it;
    // If an item is currently demoted, return it to a usage count of 1
    if (updated.usage < 0) {
        updated.usage = 1;
    }
    else {
        ++updated.usage;
    }
    m_catalogItems.erase(it);
    m_catalogItems.insert(updated);
    m_snapshotDirty = true;
    m_trigramIndexDirty = true;
}

void CatalogFast::demoteItem(const CatItem& item) {
    // Prevent catalog refreshes whilst searching
    QMutexLocker locker(&m_mutex);

    // Only fullPath and shortName take part in the comparison
    auto it = m_catalogItems.find(CatalogItem(item, 0));
    if (it == m_catalogItems.end()) {
        return;
    }

    CatalogItem updated = *it;
    // If an item is not demoted, demote it
    if (updated.usage > 0) {
        updated.usage = -1;
    }
    else { // otherwise demote it further
        --updated.usage;
    }
    m_catalogItems.erase(it);
    m_catalogItems.insert(updated);
    m_snapshotDirty = true;
    m_trigramIndexDirty = true;
}

const CatItem& CatalogFast::getItem(int i) {
    QMutexLocker locker(&m_mutex);

    rebuildSnapshot();
    return m_snapshot[i];
}

QList<CatItem*> CatalogFast::search(const QString& searchText) {
    QMutexLocker locker(&m_mutex);

    QList<CatItem*> result;
    if (searchText.isEmpty()) {
        return result;
    }

    // Search the flat copy: the set only offers const elements but the
    // caller reads through CatItem* pointers (and sorts them)
    rebuildSnapshot();
    rebuildTrigramIndex();

    QString lowSearch = searchText.toLower();
    QStringList words = lowSearch.split(' ', QString::SkipEmptyParts);

    // Collect candidate indices via trigram intersection.
    // For each query word, find items whose searchName or searchNameTrans
    // contain ALL of the word's trigrams as contiguous substrings, then
    // intersect across words (every word must match).
    QSet<int> candidates;
    bool useIndex = true;

    for (const QString& word : words) {
        QStringList trigrams = extractTrigrams(word);

        if (trigrams.isEmpty()) {
            // Word is shorter than 3 chars, trigram index can't help.
            // Fall back to linear scan for this search.
            useIndex = false;
            break;
        }

        QSet<int> wordCandidates;
        bool firstTrigram = true;
        for (const QString& tg : trigrams) {
            auto it = m_trigramIndex.find(tg);
            if (it == m_trigramIndex.end()) {
                // No item contains this trigram, no match possible
                wordCandidates.clear();
                break;
            }
            if (firstTrigram) {
                wordCandidates = *it;
                firstTrigram = false;
            }
            else {
                wordCandidates.intersect(*it);
                if (wordCandidates.isEmpty()) {
                    break;
                }
            }
        }

        if (wordCandidates.isEmpty()) {
            // No candidates for this word, no overall match
            return result;
        }

        if (candidates.isEmpty()) {
            candidates = std::move(wordCandidates);
        }
        else {
            candidates.intersect(wordCandidates);
            if (candidates.isEmpty()) {
                return result;
            }
        }
    }

    if (useIndex) {
        // Run the full subsequence test only on candidates.
        // words are already pre-split and pre-lowered above
        for (int idx : candidates) {
            if (matchWords(&m_snapshot[idx], words)) {
                result.push_back(&m_snapshot[idx]);
            }
        }
    }
    else {
        // Fall back to linear scan (query word shorter than 3 chars)
        for (int i = 0; i < m_snapshot.count(); ++i) {
            if (matchWords(&m_snapshot[i], words)) {
                result.push_back(&m_snapshot[i]);
            }
        }
    }

    return result;
}

// Rebuild the trigram index from the current snapshot.
// Must be called with m_mutex held.
void CatalogFast::rebuildTrigramIndex() {
    if (!m_trigramIndexDirty) {
        return;
    }

    // Snapshot must be up to date since the index maps to snapshot positions
    rebuildSnapshot();

    m_trigramIndex.clear();

    int n = m_snapshot.count();
    m_trigramIndex.reserve(n);

    for (int i = 0; i < n; ++i) {
        const CatalogItem& item = m_snapshot[i];

        // Index trigrams from searchName
        const QString& sn = item.searchName;
        for (int j = 0; j + 3 <= sn.size(); ++j) {
            m_trigramIndex[sn.mid(j, 3)].insert(i);
        }

        // Index trigrams from searchNameTrans
        // (duplicate insertions into QSet<int> are harmless)
        const QString& st = item.searchNameTrans;
        for (int j = 0; j + 3 <= st.size(); ++j) {
            m_trigramIndex[st.mid(j, 3)].insert(i);
        }
    }

    m_trigramIndexDirty = false;

    qInfo() << "CatalogFast::rebuildTrigramIndex, indexed"
            << n << "items into" << m_trigramIndex.size() << "trigram buckets";
}

// Rebuild the flat copy of the set, must be called with m_mutex held
void CatalogFast::rebuildSnapshot() {
    if (!m_snapshotDirty) {
        return;
    }

    m_snapshot.resize(m_catalogItems.count());
    int i = 0;
    for (const CatalogItem& item : m_catalogItems) {
        m_snapshot[i] = item;
        ++i;
    }
    m_snapshotDirty = false;
}

} // namespace launchy
