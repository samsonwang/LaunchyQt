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

#include <functional>

#include <QList>
#include <QString>
#include <QStringList>

#include "PluginInterface/CatalogItem.h"

// The ranking policy for catalog results, in one place.
//
// There used to be two implementations of it: the comparator inside
// Catalog::searchCatalogs() (which sorts a list of precomputed keys) and
// CatItemComparePtrWithMatch() (which recomputes the keys on every call).
// They had to rank identically or the result cap in searchCatalogs() would
// silently drop different candidates than an uncached sort, and nothing
// checked that. rankBefore() is now the only decision function; the two
// callers differ only in where the keys come from.
//
// The two things built on top of the policy live here as well, so Catalog.h
// carries no ranking or promotion code at all: the uncached comparator
// (CatItemCompareWithMatch, used where the keys are not worth caching) and the
// merged-list comparator (CatItemCompareWithPromotion, which makes "the command this
// query ran last time" the most significant key).
//
// Pure logic: no QWidget, no settings, no globals -- which is what makes it
// testable (see test/TestCatalogSearch.cpp).

namespace launchy {

// Ranking keys of one candidate, computed once per item.
struct CatalogRank {
    const CatItem* item = nullptr;
    bool exactEqual = false;   // searchName == query || searchNameTrans == query
    int findPos = -1;          // min(indexOf in searchName, indexOf in searchNameTrans)
    int wordMatches = 0;       // countMatchWords(), valid only when findPos == -1
    int usage = 0;
    int nameLen = 0;
};

// True when 'word' is a case-insensitive subsequence of 'name'.
// Both strings must already be lower case.
bool matchWordHelper(const QString& name, const QString& word);

// Number of the pre-split, pre-lowered query words that are subsequences of
// the item's search name (or its transliterated form).
int countMatchWords(const CatItem& item, const QStringList& lowerWords);

// Query words as the ranking expects them: split on whitespace, empty parts
// dropped. Case folding is the caller's job, the query is lower-cased once
// before searching.
QStringList splitQueryWords(const QString& text);

// Builds the keys for one item. 'searchText' must be lower case, 'lowerWords'
// its splitQueryWords() result.
CatalogRank makeRank(const CatItem& item, const QString& searchText,
                     const QStringList& lowerWords);

// THE ranking policy: true when 'a' ranks before 'b'.
// 'matchLen' is the query length (the single-character rule depends on it).
// The order is total (the fullPath tiebreaker makes every pair decidable),
// so partial_sort and stable_sort agree.
bool rankBefore(const CatalogRank& a, const CatalogRank& b, int matchLen);

// Uncached comparator: builds the keys rankBefore() wants on every call. Used
// where the list is small or the query changes per call (FileSearch ranks the
// filename part of a path, which is not the query the caller searched for).
// Rank-cached callers (Catalog::searchCatalogs) must agree with it --
// catalogAndComparatorAgree() is the guard on that.
bool CatItemCompareWithMatch(CatItem& left, CatItem& right, const QString& match);

// "Which item did this exact query pick last time" -- the only fact promotion
// needs from the command history. The application builds one from its
// CommandHistory (see promotedItem()); a default-constructed one means "no
// promotion", which is what a headless test and any history-less caller use.
using PromotionLookup = std::function<bool(const QString& query,
                                           QString& shortName,
                                           QString& fullPath)>;

// Comparator for the list the user actually sees: the catalog, history and
// plugin hits merged together (LaunchyWidget::doSearch).
//
// Two keys, in this order:
//   1. the item this exact query picked last time comes first;
//   2. everything else by CatItemCompareWithMatch, i.e. by rankBefore().
//
// Promotion is a comparison key rather than a step that runs after the sort,
// so there is no window in which a later sort could undo it, and ordering the
// merged list happens in exactly one place. The lookup runs once, in the
// constructor -- the comparator only compares afterwards. An empty lookup, or
// one that knows nothing about this query, leaves the list ordered by rank.
//
// Consequence: an item that ranks past the catalog's result cap never reaches
// this comparator, so it can only be promoted while it is among the returned
// matches.
class CatItemCompareWithPromotion {
public:
    CatItemCompareWithPromotion(const QString& query, const PromotionLookup& promote);

    bool operator()(CatItem& left, CatItem& right) const;

private:
    bool isPromoted(const CatItem& item) const;

    QString m_query;
    QString m_promotedShort;
    QString m_promotedFull;
    bool m_hasPromoted = false;
};

} // namespace launchy
