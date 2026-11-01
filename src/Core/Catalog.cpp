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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>
#include <QDebug>
#include <QStringList>

#include <algorithm>

#include "CatalogRanking.h"
#include "CoreGlobals.h"
#include "OptionItem.h"

namespace launchy {

// Json file layout:
// {
//   "version": 1,
//   "items": [
//     { "fullPath": "...", "shortName": "...", "iconPath": "...",
//       "pluginName": "...", "usage": 0 },
//     ...
//   ]
// }
//
// Only the fields that cannot be recomputed are stored; searchName and
// searchNameTrans are derived from shortName by the CatItem constructor (see
// catItemFromJson). The legacy binary launchy.db is intentionally ignored.

static const int CATALOG_FILE_VERSION = 1;

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

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(inFile.readAll(),
                                                      &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Catalog::load, invalid catalog file:"
                   << filename << parseError.errorString();
        return false;
    }

    // Remove any existing catalog contents
    m_timestamp = 0;
    clear();

    const QJsonObject root = doc.object();
    const int version = root["version"].toInt();
    if (version > CATALOG_FILE_VERSION) {
        qWarning() << "Catalog::load, catalog file version" << version
                   << "is newer than supported" << CATALOG_FILE_VERSION;
    }

    const QJsonArray items = root["items"].toArray();
    int droppedItems = 0;
    for (int i = 0; i < items.count(); ++i) {
        const QJsonValue value = items.at(i);
        if (!value.isObject()) {
            ++droppedItems;
            continue;
        }

        CatItem item;
        if (catItemFromJson(value.toObject(), item)) {
            addItem(item);
        }
        else {
            ++droppedItems;
        }
    }

    if (droppedItems > 0) {
        qWarning() << "Catalog::load, dropped malformed items:" << droppedItems;
    }

    return true;
}

// Save the catalog to the specified filename
bool Catalog::save(const QString& filename) {

    QJsonArray items;
    {
        // Snapshot the catalog under the lock; JSON serialization and file
        // I/O happen outside so that searches are only blocked for the short
        // snapshot step instead of the whole save
        QMutexLocker locker(&m_mutex);

        for (int i = 0; i < count(); i++) {
            items.append(catItemToJson(getItem(i)));
        }
    }

    QJsonObject root;
    root["version"] = CATALOG_FILE_VERSION;
    root["items"] = items;

    // QSaveFile commits atomically, a crash mid-write cannot corrupt the file
    QSaveFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("Catalog::save, Could not open catalog file for writing");
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        qWarning("Catalog::save, Could not commit catalog file");
        return false;
    }
    return true;
}

void Catalog::incrementTimestamp() {
    ++m_timestamp;
}

void Catalog::setHistoryPromotion(const IHistoryPromotion* history) {
    m_history = history;
}

// The ranking keys and the word-matching helpers live in CatalogRanking.{h,cpp}:
// they are shared with the uncached comparator below, and having one copy is
// what keeps the two sort paths from drifting apart.

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
        const QStringList words = splitQueryWords(text);

        QVector<CatalogRank> ranked;
        ranked.reserve(catMatches.count());
        for (CatItem* it : catMatches) {
            ranked.append(makeRank(it, text, words));
        }

        // Only the top `resultNum` items are ever displayed, so sort just those
        // instead of the whole match set. partial_sort drops the cost
        // from O(k log k) to O(k log resultNum) (resultNum is typically 10..20).
        // The order itself comes from rankBefore(), the same decision function
        // the uncached CatItemCompare*() path runs, so the two cannot drift.
        const int n = qMin(resultNum, ranked.count());
        std::partial_sort(ranked.begin(), ranked.begin() + n, ranked.end(),
            [&](const CatalogRank& a, const CatalogRank& b) {
                return rankBefore(a, b, text.size());
            });

        // Map the cached, sorted order back onto the pointer list
        for (int i = 0; i < ranked.count(); ++i) {
            catMatches[i] = ranked[i].item;
        }
    }

    // Check for history matches, and put them in the front
    QString promotedShort;
    QString promotedFull;
    if (m_history != nullptr
        && m_history->promotedItem(text, promotedShort, promotedFull)) {
        for (int i = 0; i < catMatches.count(); ++i) {
            if (catMatches[i]->shortName == promotedShort
                && catMatches[i]->fullPath == promotedFull) {
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
    QString promotedShort;
    QString promotedFull;
    const bool promoted = (m_history != nullptr
                           && m_history->promotedItem(text, promotedShort,
                                                      promotedFull));
    qDebug() << "Catalog::promoteRecentlyUsedItems, text:"
        << text << "promoted:" << promoted;

    if (!promoted) {
        return;
    }

    for (int i = 0; i < list.count(); i++) {
        if (list[i].shortName == promotedShort
            && list[i].fullPath == promotedFull) {
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
    //
    // Lowercase the text character-by-character instead of as a whole string:
    // QString::toLower() can change a code point's length (e.g. U+0130 'İ'
    // folds to "i" + combining dot above), which would desynchronise `marked`
    // from `text` and cause an out-of-bounds or misaligned highlight. Per-char
    // folding keeps a 1:1 index mapping and is what QString uses for
    // case-insensitive substring comparison.
    QVector<bool> marked(text.size(), false);
    QVector<QChar> lowerChars;
    lowerChars.reserve(text.size());
    for (int i = 0; i < text.size(); ++i)
        lowerChars.append(text.at(i).toLower());
    for (QString word : words) {
        word = word.toLower();
        if (word.isEmpty())
            continue;
        int cur = 0;
        int wlen = word.size();
        for (int i = 0; i < text.size() && cur < wlen; ++i) {
            if (lowerChars[i] == word[cur]) {
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

// Uncached comparator: builds exactly the keys searchCatalogs() precomputes and
// runs the same rankBefore() policy on them. Only used for the small result
// lists FileSearch ranks itself, so rebuilding the keys per comparison is fine.
static bool CatItemComparePtrWithMatch(CatItem* a, CatItem* b,
                                       const QString& match) {
    const QStringList words = splitQueryWords(match);
    return rankBefore(makeRank(a, match, words), makeRank(b, match, words),
                      match.size());
}

bool CatItemCompare(CatItem& a, CatItem& b) {
    return CatItemComparePtrWithMatch(&a, &b, g_searchText);
}

bool CatItemCompareWithMatch(CatItem& a, CatItem& b, const QString& match) {
    return CatItemComparePtrWithMatch(&a, &b, match);
}

} // namespace launchy
