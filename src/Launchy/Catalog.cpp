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
// order, somewhere inside `name`. Both strings are expected to be lower case already.
static bool matchWordHelper(const QString& name, const QString& word) {
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

// Count how many pre-split, pre-lowered query words are subsequences of the
// item's search name (or its transliterated form). Used to rank items when the
// whole query is not a contiguous substring (e.g. out-of-order, multi-word).
// The caller is responsible for splitting/toLowering the query once; passing the
// word list in avoids re-splitting on every rank comparison.
static int countMatchWords(const CatItem* item,
                              const QStringList& lowerWords) {
    int count = 0;
    for (const QString& word : lowerWords) {
        if (word.isEmpty())
            continue;
        if (matchWordHelper(item->searchName, word) ||
            matchWordHelper(item->searchNameTrans, word))
            ++count;
    }
    return count;
}

// Precomputed ranking keys for one matched item. Building these ONCE per item
// (instead of recomputing them on every std::sort comparison) is the whole point
// of the sort-key cache: a single sort compares each item O(log n) times,
// so without caching the same indexOf()/matchWordHelper() results are recomputed
// dozens of times per item.
struct CatalogRank {
    CatItem* item;
    bool exactEqual;   // searchName == query || searchNameTrans == query
    int findPos;       // min(indexOf in searchName, indexOf in searchNameTrans)
    int wordMatches;   // countMatchWords result (valid only when findPos == -1)
    int usage;
    int nameLen;
};

static CatalogRank makeRank(CatItem* item, const QString& searchText,
                            const QStringList& lowerWords) {
    CatalogRank r;
    r.item = item;
    r.exactEqual = (item->searchName == searchText ||
                    item->searchNameTrans == searchText);
    r.findPos = std::min(item->searchName.indexOf(searchText),
                         item->searchNameTrans.indexOf(searchText));
    r.usage = item->usage;
    r.nameLen = item->shortName.size();
    // Only needed when neither name contains the query as a contiguous substring;
    // the comparator never reads wordMatches otherwise, so skip the work.
    r.wordMatches = (r.findPos == -1)
        ? countMatchWords(item, lowerWords)
        : 0;
    return r;
}

// Return true if the specified catalog item matches the specified string.
//
// The query is split on whitespace into individual words. Every non-empty word
// must be a subsequence of the item's search name (or its transliterated form),
// in ANY order. This lets a user type any subset of a file name's words, in any
// order, and still find the file, e.g. "nassim reviews book" matches
// "book reviews of Nassim Taleb.txt".
// Subsequence test using pre-split, pre-lowered query words.
// Called in the hot search loop where the query has already been split and
// lowercased once, avoiding N redundant split()/toLower() calls.
bool Catalog::matchWords(const CatItem* item, const QStringList& lowerWords) {
    if (lowerWords.isEmpty())
        return false;

    for (const QString& word : lowerWords) {
        if (word.isEmpty())
            continue;
        if (!matchWordHelper(item->searchName, word)
            && !matchWordHelper(item->searchNameTrans, word)) {
            return false;
        }
    }

    return true;
}

// Search the catalog, for items matching the text parameter and
// populate the out parameter
void Catalog::searchCatalogs(const QString& text, QList<CatItem>& result) {
    // text paramater is expected to be lower case already

    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    QList<CatItem*> catMatches = search(text);
    qDebug() << "Catalog::searchCatalogs, search matched count:" << catMatches.count();

    // Prioritize the catalog items.
    // Precompute each item's ranking keys ONCE, then sort the cached view.
    // This replaces CatItemComparePtr (which recomputes indexOf()/matchWordHelper() on every
    // comparison) with a comparator that only reads the cached keys.
    // Read the result cap once; it bounds both the partial_sort and the output.
    int resultNum = g_settings->value(OPTION_NUMRESULT, OPTION_NUMRESULT_DEFAULT).toInt();
    if (catMatches.count() > 1) {
        QStringList words = text.split(' ', QString::SkipEmptyParts);

        QVector<CatalogRank> ranked;
        ranked.reserve(catMatches.count());
        for (CatItem* it : catMatches) {
            ranked.append(makeRank(it, text, words));
        }

        // Only the top `resultNum` items are ever displayed, so sort just those
        // instead of the whole match set. partial_sort drops the cost
        // from O(k log k) to O(k log resultNum) (resultNum is typically 10..20).
        int n = qMin(resultNum, ranked.count());
        std::partial_sort(ranked.begin(), ranked.begin() + n, ranked.end(),
            [&](const CatalogRank& a, const CatalogRank& b) {
                // Mirrors CatItemComparePtr, but reads precomputed keys.
                if (a.usage < 0 && b.usage >= 0)
                    return false;
                if (b.usage < 0 && a.usage >= 0)
                    return true;

                if (a.exactEqual && !b.exactEqual)
                    return true;
                if (!a.exactEqual && b.exactEqual)
                    return false;

                int localFind = a.findPos;
                int otherFind = b.findPos;

                if (text.size() == 1) {
                    // Match at the start
                    if (localFind == 0 && otherFind != 0)
                        return true;
                    else if (localFind != 0 && otherFind == 0)
                        return false;

                    // Higher usage
                    if (a.usage > b.usage)
                        return true;
                    if (a.usage < b.usage)
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
                    if (a.usage > b.usage)
                        return true;
                    if (a.usage < b.usage)
                        return false;

                    // Contiguous text nearer the start of the item name
                    if (localFind < otherFind)
                        return true;
                    else if (otherFind < localFind)
                        return false;
                }
                else {
                    // Neither item contains the whole query as a contiguous substring
                    int localWords = a.wordMatches;
                    int otherWords = b.wordMatches;
                    if (localWords != otherWords)
                        return localWords > otherWords;

                    // Higher usage
                    if (a.usage > b.usage)
                        return true;
                    if (a.usage < b.usage)
                        return false;
                }

                int localLen = a.nameLen;
                int otherLen = b.nameLen;

                // Favour shorter item names
                if (localLen < otherLen)
                    return true;
                if (localLen > otherLen)
                    return false;

                // Absolute tiebreaker to prevent loops
                return a.item->fullPath < b.item->fullPath;
            });

        // Map the cached, sorted order back onto the pointer list
        for (int i = 0; i < ranked.count(); ++i) {
            catMatches[i] = ranked[i].item;
        }
    }

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

    // Load up the results (first `resultNum`, already in priority order)
    for (int i = 0; i < resultNum && i < catMatches.count(); ++i) {
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


CatalogItem::CatalogItem()
    : m_timestamp(0) {

}

CatalogItem::CatalogItem(const CatItem& item, int time)
    : CatItem(item),
      m_timestamp(time) {

}


bool CatItemCompareRef(CatItem& a, CatItem& b) {
    return CatItemComparePtr(&a, &b);
}

bool CatItemComparePtr(CatItem* a, CatItem* b) {
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
        QStringList words = g_searchText.split(' ', QString::SkipEmptyParts);

        int localWords = countMatchWords(a, words);
        int otherWords = countMatchWords(b, words);
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

} // namespace launchy
