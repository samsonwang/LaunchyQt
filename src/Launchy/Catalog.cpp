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

#include "Catalog.h"

#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <QStringList>

#include "GlobalVar.h"
#include "OptionItem.h"

namespace launchy {

Catalog::Catalog()
    : m_timestamp(0) {

}

Catalog::~Catalog() {

}

// Load the catalog from the specified filename
bool Catalog::load(const QString& filename) {
    QFile inFile(filename);
    if (!inFile.open(QIODevice::ReadOnly)) {
        qInfo() << "Catalog::load, fail to open catalog file:"
                << filename;
        return false;
    }

    // Remove any existing catalog contents
    m_timestamp = 0;
    clear();

    QByteArray ba = inFile.readAll();
    QByteArray unzipped = qUncompress(ba);
    QDataStream in(&unzipped, QIODevice::ReadOnly);
    in.setVersion(QDataStream::Qt_4_2);

    while (!in.atEnd()) {
        CatItem item;
        in >> item;
        addItem(item);
    }

    return true;
}

// Save the catalog to the specified filename
bool Catalog::save(const QString& filename) {

    QByteArray ba;
    {
        // Snapshot the catalog under the lock; compression and file I/O
        // happen outside so that searches are only blocked for the short
        // serialization step instead of the whole save
        QMutexLocker locker(&m_mutex);

        QDataStream out(&ba, QIODevice::ReadWrite);
        out.setVersion(QDataStream::Qt_4_2);

        for (int i = 0; i < count(); i++) {
            CatItem item = getItem(i);
            out << item;
        }
    }

    // Compress and write the catalog to the specified file
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("Catalog::save, Could not open catalog file for writing");
        return false;
    }
    file.write(qCompress(ba));
    return true;
}

void Catalog::incrementTimestamp() {
    ++m_timestamp;
}

// Case-insensitive subsequence test: every character of `word` must appear, in
// order, somewhere inside `name`. Both strings are expected to be lower case
// already, but `word` is lowered defensively so the helper is self contained.
static bool wordMatches(const QString& name, const QString& word) {
    if (word.isEmpty())
        return true;
    int cur = 0;
    int wlen = word.size();
    for (QChar c : name) {
        if (c == word[cur]) {
            if (++cur >= wlen)
                return true;
        }
    }
    return false;
}

// Count how many whitespace-separated words of `searchText` are subsequences of
// the item's search name (or its transliterated form). Used to rank items when
// the whole query is not a contiguous substring (e.g. out-of-order, multi-word).
static int countMatchingWords(const CatItem* item, const QString& searchText) {
    QStringList words = searchText.split(' ', QString::SkipEmptyParts);
    int count = 0;
    for (QString word : words) {
        word = word.toLower();
        if (word.isEmpty())
            continue;
        if (wordMatches(item->searchName, word) ||
            wordMatches(item->searchNameTrans, word))
            ++count;
    }
    return count;
}

// Return true if the specified catalog item matches the specified string.
//
// The query is split on whitespace into individual words. Every non-empty word
// must be a subsequence of the item's search name (or its transliterated form),
// in ANY order. This lets a user type any subset of a file name's words, in any
// order, and still find the file, e.g. "nassim reviews book" matches
// "book reviews of Nassim Taleb.txt".
bool Catalog::matches(CatItem* item, const QString& match) {
    QStringList words = match.split(' ', QString::SkipEmptyParts);
    if (words.isEmpty())
        return false;

    for (QString word : words) {
        word = word.toLower();
        if (!wordMatches(item->searchName, word) &&
            !wordMatches(item->searchNameTrans, word)) {
            return false;
        }
    }

    return true;
}

// Search the catalog, for items matching the text parameter and
// populate the out parameter
void Catalog::searchCatalogs(const QString& text, QList<CatItem>& result) {
    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    QList<CatItem*> catMatches = search(text);
    qDebug() << "Catalog::searchCatalogs, search matched count:" << catMatches.count();
    // Now prioritize the catalog items
    std::sort(catMatches.begin(), catMatches.end(), CatLessPtr);

    // Check for history matches, and put them in the front
    QString location = "History/" + text;
    QStringList hist = g_settings->value(location).toStringList();
    if (hist.count() == 2) {
        for (int i = 0; i < catMatches.count(); ++i) {
            if (catMatches[i]->shortName == hist[0] && catMatches[i]->fullPath == hist[1]) {
                CatItem* tmp = catMatches[i];
                catMatches.removeAt(i);
                catMatches.push_front(tmp);
            }
        }
    }

    // Load up the results
    int max = g_settings->value(OPTION_NUMRESULT, OPTION_NUMRESULT_DEFAULT).toInt();
    for (int i = 0; i < max && i < catMatches.count(); ++i) {
        result.push_back(*catMatches[i]);
    }
}

void Catalog::promoteRecentlyUsedItems(const QString& text, QList<CatItem>& list) {
    // Check for history matches
    QString location = "History/" + text;
    QStringList hist = g_settings->value(location).toStringList();
    qDebug() << "Catalog::promoteRecentlyUsedItems, text:"
        << text << "hist" << hist << "hist count:" << hist.count();

    if (hist.count() != 2) {
        return;
    }

    for (int i = 0; i < list.count(); i++) {
        if (list[i].shortName == hist[0] && list[i].fullPath == hist[1]) {
            CatItem tmp = list[i];
            qDebug() << "Catalog::promoteRecentlyUsedItems, promoted:" << tmp.fullPath;
            list.removeAt(i);
            list.push_front(tmp);
            break;
        }
    }
}

QString Catalog::decorateText(const QString& text, const QString& match, bool outputRichText) {
    if (!g_settings->value(OPTION_DECORATETEXT, OPTION_DECORATETEXT_DEFAULT).toBool())
        return text;
    QStringList words = match.split(' ', QString::SkipEmptyParts);
    if (words.isEmpty())
        return text;

    // Mark every character of `text` that is consumed by the subsequence match
    // of at least one query word. This keeps the highlight correct when the
    // query words appear out of order or in multiple places.
    QString lowerText = text.toLower();
    QVector<bool> marked(lowerText.size(), false);
    for (QString word : words) {
        word = word.toLower();
        if (word.isEmpty())
            continue;
        int cur = 0;
        int wlen = word.size();
        for (int i = 0; i < lowerText.size() && cur < wlen; ++i) {
            if (lowerText[i] == word[cur]) {
                marked[i] = true;
                ++cur;
            }
        }
    }

    QString decoratedText;
    bool highlighted = false;
    for (int i = 0; i < text.size(); ++i) {
        QChar c = text[i];
        if (marked[i]) {
            if (outputRichText) {
                if (!highlighted) {
                    decoratedText += "<u>";
                    highlighted = true;
                }
                decoratedText += c;
            }
            else
                decoratedText += QString("&") + c;
        }
        else {
            if (outputRichText && highlighted) {
                decoratedText += "</u>";
                highlighted = false;
            }
            decoratedText += c;
        }
    }

    if (outputRichText && highlighted) {
        decoratedText += "</u>";
        highlighted = false;
    }

    return decoratedText;
}

// Key of an item in SlowCatalog's position index. Items are equal when both
// fullPath and shortName match (CatItem::operator==), a control character
// separates the fields since it can not occur in a path or a file name
static QString indexKey(const CatItem& item) {
    return item.fullPath + QChar(0x1f) + item.shortName;
}

SlowCatalog::SlowCatalog()
    : Catalog() {

}

int SlowCatalog::count() {
    // Recursive lock: safe on its own (options dialog) and when save() calls
    // this while already holding m_mutex
    QMutexLocker locker(&m_mutex);

    return m_catalogItems.count();
}

void SlowCatalog::clear() {
    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    m_catalogItems.clear();
    m_index.clear();
}

void SlowCatalog::addItem(const CatItem& item) {
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

void SlowCatalog::purgeOldItems() {
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

    qInfo() << "SlowCatalog::purgeOldItems, removed" << removed
        << "stale items";
}


void SlowCatalog::incrementUsage(const CatItem& item) {
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

void SlowCatalog::demoteItem(const CatItem& item) {
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

const CatItem& SlowCatalog::getItem(int i) {
    // Recursive lock: save() calls this while already holding m_mutex
    QMutexLocker locker(&m_mutex);

    return m_catalogItems[i];
}

// Return a list of catalog items that match searchText.
// searchCatalogs() already holds m_mutex, the recursive lock keeps direct
// callers safe as well
QList<CatItem*> SlowCatalog::search(const QString& searchText) {
    QMutexLocker locker(&m_mutex);

    QList<CatItem*> result;
    if (!searchText.isEmpty()) {
        QString lowSearch = searchText.toLower();
        for (int i = 0; i < m_catalogItems.count(); ++i) {
            if (matches(&m_catalogItems[i], lowSearch)) {
                result.push_back(&m_catalogItems[i]);
            }
        }
    }

    return result;
}


FastCatalog::FastCatalog()
    : Catalog(),
      m_snapshotDirty(true) {

}

int FastCatalog::count() {
    return m_catalogItems.count();
}

void FastCatalog::clear() {
    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    m_catalogItems.clear();
    m_snapshot.clear();
    m_snapshotDirty = true;
}

void FastCatalog::addItem(const CatItem& item) {
    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    // The set itself provides the duplicate detection that SlowCatalog
    // needs a key -> position index for: only fullPath and shortName are
    // hashed and compared (see qHash in Catalog.h), so an equal item is
    // found in O(1) even while a rebuild adds tens of thousands of items
    const CatalogItem stored(item, m_timestamp);
    auto it = m_catalogItems.find(stored);

    if (it != m_catalogItems.end()) {
        if (m_timestamp > 0) {
            // Rebuild phase: refresh every field of the existing item but
            // keep its usage count, exactly like SlowCatalog does.
            // QSet::insert keeps the old key object of an equal item, so
            // the stale entry has to be removed before inserting again
            CatalogItem refreshed = stored;
            refreshed.usage = it->usage;
            m_catalogItems.erase(it);
            m_catalogItems.insert(refreshed);
            m_snapshotDirty = true;
        }
        // While loading the catalog (timestamp == 0) the first entry of
        // an equal pair wins, the set cannot keep the duplicates that
        // SlowCatalog appends in this phase
        return;
    }

    m_catalogItems.insert(stored);
    m_snapshotDirty = true;
}

void FastCatalog::purgeOldItems() {
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
    qInfo() << "FastCatalog::purgeOldItems, removed" << removed
        << "stale items";
}

void FastCatalog::incrementUsage(const CatItem& item) {
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
}

void FastCatalog::demoteItem(const CatItem& item) {
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
}

const CatItem& FastCatalog::getItem(int i) {
    rebuildSnapshot();
    return m_snapshot[i];
}

// Return a list of catalog items that match searchText
// this method should only be called from within a QMutexLocker protected section
QList<CatItem*> FastCatalog::search(const QString& searchText) {
    QList<CatItem*> result;
    if (searchText.isEmpty()) {
        return result;
    }

    // Search the flat copy: the set only offers const elements but the
    // caller reads through CatItem* pointers (and sorts them)
    rebuildSnapshot();

    QString lowSearch = searchText.toLower();
    for (int i = 0; i < m_snapshot.count(); ++i) {
        if (matches(&m_snapshot[i], lowSearch)) {
            result.push_back(&m_snapshot[i]);
        }
    }

    return result;
}

// Rebuild the flat copy of the set, must be called with m_mutex held
void FastCatalog::rebuildSnapshot() {
    if (!m_snapshotDirty) {
        return;
    }

    // resize() keeps the allocated capacity of the previous snapshot
    m_snapshot.resize(m_catalogItems.count());
    int i = 0;
    for (const CatalogItem& item : m_catalogItems) {
        m_snapshot[i] = item;
        ++i;
    }
    m_snapshotDirty = false;
}

bool CatLessRef(CatItem& a, CatItem& b) {
    bool less = CatLessPtr(&a, &b);
    /*	if (less)
    qDebug() << a.lowName << "(" << a.usage << ") < " << b.lowName << " (" << b.usage << ")";
    else
    qDebug() << b.lowName << "(" << b.usage << ") < " << a.lowName << " (" << a.usage << ")";
    */
    return less;
}

bool CatLessPtr(CatItem* a, CatItem* b) {
    // Items with negative usage are lowest priority
    if (a->usage < 0 && b->usage >= 0)
        return false;
    if (b->usage < 0 && a->usage >= 0)
        return true;

    bool localEqual = (a->searchName == g_searchText
                       || a->searchNameTrans == g_searchText);

    bool otherEqual = (b->searchName == g_searchText
                       || b->searchNameTrans == g_searchText);

    // Exact match between search text and item name has higest priority
    if (localEqual && !otherEqual)
        return true;
    if (!localEqual && otherEqual)
        return false;

    int localFind = std::min(a->searchName.indexOf(g_searchText),
                             a->searchNameTrans.indexOf(g_searchText));
    int otherFind = std::min(b->searchName.indexOf(g_searchText),
                             b->searchNameTrans.indexOf(g_searchText));

    if (g_searchText.size() == 1) {
        // Match at the start
        if (localFind == 0 && otherFind != 0)
            return true;
        else if (localFind != 0 && otherFind == 0)
            return false;

        // Higher usage
        if (a->usage > b->usage)
            return true;
        if (a->usage < b->usage)
            return false;
    }

    // Contiguous text anywhere in the item name
    if (localFind != -1 && otherFind == -1)
        return true;
    else if (localFind == -1 && otherFind != -1)
        return false;

    if (localFind != -1 && otherFind != -1) {
        // Both have word matches
        // Higher usage
        if (a->usage > b->usage)
            return true;
        if (a->usage < b->usage)
            return false;

        // Contiguous text nearer the start of the item name
        if (localFind < otherFind)
            return true;
        else if (otherFind < localFind)
            return false;
    }
    else {
        // Neither item contains the whole query as a contiguous substring
        // (typical for out-of-order / multi-word queries). Rank by how many
        // query words match, then by usage, so closer matches surface first.
        int localWords = countMatchingWords(a, g_searchText);
        int otherWords = countMatchingWords(b, g_searchText);
        if (localWords != otherWords)
            return localWords > otherWords;

        // Higher usage
        if (a->usage > b->usage)
            return true;
        if (a->usage < b->usage)
            return false;
    }

    int localLen = a->shortName.size();
    int otherLen = b->shortName.size();

    // Favour shorter item names
    if (localLen < otherLen)
        return true;
    if (localLen > otherLen)
        return false;

    // Absolute tiebreaker to prevent loops
    return a->fullPath < b->fullPath;
}

CatalogItem::CatalogItem()
    : m_timestamp(0) {

}

CatalogItem::CatalogItem(const CatItem& item, int time)
    : CatItem(item),
      m_timestamp(time) {

}

} // namespace launchy
