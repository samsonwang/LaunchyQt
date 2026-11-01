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

#include "CatalogRanking.h"

#include <algorithm>

namespace launchy {

// Case-insensitive subsequence test: every character of 'word' must appear, in
// order, somewhere inside 'name'. Both strings are expected to be lower case
// already.
bool matchWordHelper(const QString& name, const QString& word) {
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
int countMatchWords(const CatItem* item, const QStringList& lowerWords) {
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

QStringList splitQueryWords(const QString& text) {
    return text.split(' ', QString::SkipEmptyParts);
}

// Precomputed ranking keys for one matched item. Building these ONCE per item
// (instead of recomputing them on every std::sort comparison) is the whole point
// of the sort-key cache: a single sort compares each item O(log n) times,
// so without caching the same indexOf()/matchWordHelper() results are recomputed
// dozens of times per item.
CatalogRank makeRank(CatItem* item, const QString& searchText,
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

bool rankBefore(const CatalogRank& a, const CatalogRank& b, int matchLen) {
    // Items with negative usage are lowest priority
    if (a.usage < 0 && b.usage >= 0)
        return false;
    if (b.usage < 0 && a.usage >= 0)
        return true;

    // Exact match between search text and item name has highest priority
    if (a.exactEqual && !b.exactEqual)
        return true;
    if (!a.exactEqual && b.exactEqual)
        return false;

    const int localFind = a.findPos;
    const int otherFind = b.findPos;

    if (matchLen == 1) {
        // Match at the start
        if (localFind == 0 && otherFind != 0)
            return true;
        else if (localFind != 0 && otherFind == 0)
            return false;

        // Higher usage
        if (a.usage != b.usage)
            return a.usage > b.usage;
    }

    // Contiguous text anywhere in the item name
    if (localFind != -1 && otherFind == -1)
        return true;
    else if (localFind == -1 && otherFind != -1)
        return false;

    if (localFind != -1 && otherFind != -1) {
        // Both contain the query as a contiguous substring:
        // Higher usage
        if (a.usage != b.usage)
            return a.usage > b.usage;

        // Contiguous text nearer the start of the item name
        if (localFind < otherFind)
            return true;
        else if (otherFind < localFind)
            return false;
    }
    else {
        // Neither item contains the whole query as a contiguous substring
        // (typical for out-of-order / multi-word queries).
        //
        // Usage is compared before the word count. This is the only branch a
        // plugin's dynamic result can ever reach: such an item carries the
        // answer instead of the typed query in its name (CalcyPy answers
        // "12+3" with an item named "15"), so it matches none of the query
        // words and every catalog match would outrank it on the word count
        // alone. A high usage is all a plugin has to say "put me first", so
        // it has to be honoured here. Items with equal usage - the common
        // case for catalog entries that were never launched - still get the
        // multi-word ranking below.
        //
        // Swapping these two comparisons was the v3.1.9 regression (b4c9392):
        // see test/TestCatalogSearch.cpp, usageBeatsWordCount().
        if (a.usage != b.usage)
            return a.usage > b.usage;

        // Rank by how many query words match, so closer matches surface first
        if (a.wordMatches != b.wordMatches)
            return a.wordMatches > b.wordMatches;
    }

    // Favour shorter item names
    if (a.nameLen < b.nameLen)
        return true;
    if (a.nameLen > b.nameLen)
        return false;

    // Absolute tiebreaker to prevent loops
    return a.item->fullPath < b.item->fullPath;
}

} // namespace launchy
