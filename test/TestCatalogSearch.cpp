/*
LaunchyQt: Catalog ranking unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QTemporaryDir>
#include <QSharedPointer>

#include <algorithm>

#include "Catalog.h"
#include "CatalogSlow.h"
#include "CatalogRanking.h"
#include "CoreGlobals.h"
#include "OptionItem.h"

#include "PluginInterface/Runtime.h"

using namespace launchy;

namespace {

CatItem makeItem(const QString& path, const QString& shortName, int usage = 0) {
    CatItem item(path, shortName);
    item.usage = usage;
    return item;
}

QList<CatItem> runSearch(Catalog& catalog, const QString& query) {
    QList<CatItem> result;
    catalog.searchCatalogs(query, result);
    return result;
}

QStringList shortNames(const QList<CatItem>& items) {
    QStringList names;
    for (const CatItem& item : items) {
        names.append(item.shortName);
    }
    return names;
}

// Stands in for CommandHistory: promotion only ever asks "which item did this
// exact query pick last time", so a callable is all it takes.
PromotionLookup promoteTo(const QString& query, const QString& shortName,
                          const QString& fullPath) {
    return [=](const QString& text, QString& promotedShort, QString& promotedFull) {
        if (text != query) {
            return false;
        }
        promotedShort = shortName;
        promotedFull = fullPath;
        return true;
    };
}

} // namespace

// Ranking policy tests. The catalog is created per test so no state leaks
// between cases.
class TestCatalogSearch : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    // --- the policy itself (CatalogRanking::rankBefore) ---
    void usageBeatsWordCount();
    void wordCountBreaksUsageTies();

    // --- the same policy driven through a catalog ---
    void exactMatchFirst();
    void prefixFirstForSingleCharQuery();
    void contiguousBeatsNonContiguous();
    void negativeUsageRanksLast();
    void shorterNameWins();
    void resultCapHonoured();
    void matchSetBelowCapIsUnordered();
    void promotedItemSortsFirst();
    void catalogAndComparatorAgree();
    void saveAndLoadRoundTrip();

    // --- the catalog file format ---
    void loadRejectsABrokenFile();
    void loadDropsMalformedItems();
    void loadAcceptsANewerFileVersion();
    void loadReplacesExistingContent();

    // --- result decoration ---
    void decorateTextMarksEveryMatchedCharacter();
    void decorateTextProducesRichText();
    void decorateTextDisabledBySetting();

private:
    QScopedPointer<QTemporaryDir> m_dir;
};

void TestCatalogSearch::initTestCase() {
    m_dir.reset(new QTemporaryDir);
    QVERIFY(m_dir->isValid());

    // The catalog reads its result cap from the settings store; the plugin ABI
    // exports the same object, so assigning here is all a test has to do.
    g_settings = QSharedPointer<QSettings>(
        new QSettings(m_dir->path() + "/catalog_test.ini", QSettings::IniFormat));
    QVERIFY(!g_settings.isNull());
}

void TestCatalogSearch::init() {
    // Every case starts with a cap large enough to see the whole result set.
    // searchCatalogs() only ranks when the cap has to cut the match set, so the
    // cases that assert an ORDER shrink the cap below the number of matches
    // first; the ones that only count results can leave it alone.
    g_settings->setValue(OPTION_NUMRESULT, 100);
}

void TestCatalogSearch::cleanupTestCase() {
    // Mirror what the application does at shutdown (main()):
    // release the shared QSettings before the libraries are unloaded, so the
    // object is not destroyed from Launchy.dll's static teardown.
    if (!g_settings.isNull()) {
        g_settings->sync();
    }
    g_settings.clear();
    m_dir.reset();
}

// Regression for b4c9392 (v3.1.9): the branch used by plugin answers compares
// usage BEFORE the word count. A plugin result ("15" answering "12+3") matches
// no query word at all, so if the word count went first every catalog match
// would outrank it and setUsage() could no longer push a plugin to the top.
void TestCatalogSearch::usageBeatsWordCount() {
    const QString query = "12 3"; // two words, so the word count can differ
    CatItem pluginItem = makeItem("calcy:15", "15", 50000);
    CatItem catalogItem = makeItem("C:/tmp/123.txt", "123", 0);

    const QStringList words = splitQueryWords(query);
    CatalogRank pluginRank = makeRank(pluginItem, query, words);
    CatalogRank catalogRank = makeRank(catalogItem, query, words);

    // Neither name contains the query contiguously: this is the branch that
    // decides between a plugin answer and a file match.
    QCOMPARE(pluginRank.findPos, -1);
    QCOMPARE(catalogRank.findPos, -1);
    // The file matches both query words, the plugin answer matches none
    QCOMPARE(pluginRank.wordMatches, 0);
    QCOMPARE(catalogRank.wordMatches, 2);

    // Usage decides anyway - this is the assertion the v3.1.9 order failed
    QVERIFY(rankBefore(pluginRank, catalogRank, query.size()));
    QVERIFY(!rankBefore(catalogRank, pluginRank, query.size()));
}

// The other half of the same branch: with equal usage the closer multi-word
// match still wins, so an unlaunched file is not outranked by a worse match.
void TestCatalogSearch::wordCountBreaksUsageTies() {
    const QString query = "12 3";
    CatItem weak = makeItem("calcy:15", "15", 0);
    CatItem closer = makeItem("C:/tmp/123.txt", "123", 0);

    const QStringList words = splitQueryWords(query);

    QVERIFY(rankBefore(makeRank(closer, query, words),
                       makeRank(weak, query, words), query.size()));
    QVERIFY(!rankBefore(makeRank(weak, query, words),
                        makeRank(closer, query, words), query.size()));
}

void TestCatalogSearch::exactMatchFirst() {
    // One slot, two matches: the cap has to cut, so the catalog ranks
    g_settings->setValue(OPTION_NUMRESULT, 1);

    CatalogSlow catalog;
    catalog.addItem(makeItem("C:/win/notepad.exe", "notepad", 0));      // exact
    catalog.addItem(makeItem("C:/win/notepad++.exe", "notepad++", 100)); // used more

    // Exact name match outranks usage
    QCOMPARE(shortNames(runSearch(catalog, "notepad")), QStringList({"notepad"}));
}

void TestCatalogSearch::prefixFirstForSingleCharQuery() {
    g_settings->setValue(OPTION_NUMRESULT, 1);

    CatalogSlow catalog;
    catalog.addItem(makeItem("C:/apps/chrome.exe", "chrome", 0));
    catalog.addItem(makeItem("C:/apps/acrobat.exe", "acrobat", 100));

    // For a one-character query a match at the start wins over usage
    QCOMPARE(shortNames(runSearch(catalog, "c")), QStringList({"chrome"}));
}

void TestCatalogSearch::contiguousBeatsNonContiguous() {
    g_settings->setValue(OPTION_NUMRESULT, 1);

    CatalogSlow catalog;
    catalog.addItem(makeItem("C:/apps/firefox.exe", "firefox", 0));
    catalog.addItem(makeItem("C:/docs/f-i-r-e.txt", "f-i-r-e", 100));

    // "f-i-r-e" matches the query as a subsequence only; a contiguous match
    // ranks above it even with a much lower usage count
    QCOMPARE(shortNames(runSearch(catalog, "fire")), QStringList({"firefox"}));
}

void TestCatalogSearch::negativeUsageRanksLast() {
    g_settings->setValue(OPTION_NUMRESULT, 1);

    CatalogSlow catalog;
    catalog.addItem(makeItem("C:/win/calc.exe", "calc", -1)); // demoted
    catalog.addItem(makeItem("C:/win/calculator.exe", "calculator", 0));

    // Negative usage (demoted) is lowest priority, even when the name is an
    // exact match: it does not get the only slot
    QCOMPARE(shortNames(runSearch(catalog, "calc")), QStringList({"calculator"}));
}

void TestCatalogSearch::shorterNameWins() {
    g_settings->setValue(OPTION_NUMRESULT, 1);

    CatalogSlow catalog;
    catalog.addItem(makeItem("C:/x/abzzz.txt", "abzzz", 0));
    catalog.addItem(makeItem("C:/x/abz.txt", "abz", 0));

    // Same usage, same match position: the shorter name comes first, so it is
    // the one the cap keeps
    QCOMPARE(shortNames(runSearch(catalog, "ab")), QStringList({"abz"}));
}

// The other half of the contract: a match set that fits inside the cap is
// returned whole and in search() order. Nothing has to be chosen, and the
// caller ranks the merged list anyway (LaunchyWidget::doSearch()).
void TestCatalogSearch::matchSetBelowCapIsUnordered() {
    CatalogSlow catalog;
    catalog.addItem(makeItem("C:/x/abzzz.txt", "abzzz", 0));
    catalog.addItem(makeItem("C:/x/abz.txt", "abz", 0));

    // Cap 2, two matches: no ranking, CatalogSlow's insertion order comes back
    g_settings->setValue(OPTION_NUMRESULT, 2);
    QCOMPARE(shortNames(runSearch(catalog, "ab")), QStringList({"abzzz", "abz"}));

    // Cap 1: now the set has to be cut, and the ranking decides what survives
    g_settings->setValue(OPTION_NUMRESULT, 1);
    QCOMPARE(shortNames(runSearch(catalog, "ab")), QStringList({"abz"}));
}

void TestCatalogSearch::resultCapHonoured() {
    g_settings->setValue(OPTION_NUMRESULT, 2);

    CatalogSlow catalog;
    catalog.addItem(makeItem("C:/x/ab1.txt", "ab1", 0));
    catalog.addItem(makeItem("C:/x/ab2.txt", "ab2", 0));
    catalog.addItem(makeItem("C:/x/ab3.txt", "ab3", 0));

    // Three matches, cap 2: the two best survive, in rank order
    QCOMPARE(shortNames(runSearch(catalog, "ab")), QStringList({"ab1", "ab2"}));
}

// The comparator behind the list the user sees: "the command this exact query
// ran last time" is its most significant key, ranking is the second one.
void TestCatalogSearch::promotedItemSortsFirst() {
    // campfire wins on usage (50 vs 0), so it ranks first without promotion
    const QList<CatItem> merged = {
        makeItem("C:/apps/firefox.exe", "firefox", 0),
        makeItem("C:/docs/campfire.jpg", "campfire", 50),
    };

    const auto sorted = [](QList<CatItem> list, const PromotionLookup& promote) {
        std::sort(list.begin(), list.end(), CatItemCompareWithPromotion("fire", promote));
        return shortNames(list);
    };

    // Plain ranking
    QCOMPARE(sorted(merged, PromotionLookup()), QStringList({"campfire", "firefox"}));

    // The item this exact query picked last time comes first, even though it
    // ranks below the other one
    QCOMPARE(sorted(merged, promoteTo("fire", "firefox", "C:/apps/firefox.exe")),
             QStringList({"firefox", "campfire"}));

    // No entry for this query, or an entry for an item that is not in the list:
    // the ranking decides everything
    QCOMPARE(sorted(merged, promoteTo("other", "firefox", "C:/apps/firefox.exe")),
             QStringList({"campfire", "firefox"}));
    QCOMPARE(sorted(merged, promoteTo("fire", "ghost", "C:/x/ghost.exe")),
             QStringList({"campfire", "firefox"}));
}

// The cached sort in searchCatalogs() and the uncached CatItemCompareWithMatch()
// path must rank identically - they used to be two copies of the same policy.
// This is the guard that keeps them from drifting apart again. The cap is below
// the match count so the catalog actually ranks, and it also proves the cut
// keeps the head of the ranking rather than a different subset.
void TestCatalogSearch::catalogAndComparatorAgree() {
    const int cap = 4;
    g_settings->setValue(OPTION_NUMRESULT, cap);

    CatalogSlow catalog;
    const QList<CatItem> items = {
        makeItem("C:/x/ab.exe", "ab", 3),
        makeItem("C:/x/abc.txt", "abc", 0),
        makeItem("C:/x/xab.txt", "xab", 9),
        makeItem("C:/x/a-b.txt", "a-b", 1),
        makeItem("C:/x/zzab.txt", "zzab", 0),
        makeItem("C:/x/ab-c.txt", "ab-c", 2),
    };
    for (const CatItem& item : items) {
        catalog.addItem(item);
    }

    const QString query = "ab";
    const QStringList cachedOrder = shortNames(runSearch(catalog, query));
    QCOMPARE(cachedOrder.count(), cap);

    QList<CatItem> uncached = items;
    // std::sort, not stable_sort: the comparator ends in a fullPath tiebreaker,
    // so the order is total and neither algorithm can disagree on it.
    std::sort(uncached.begin(), uncached.end(),
              [&query](CatItem& left, CatItem& right) {
                  return CatItemCompareWithMatch(left, right, query);
              });

    QCOMPARE(cachedOrder, shortNames(uncached.mid(0, cap)));
}

// The catalog file format moved into the core library, keep the round-trip
// covered: load() must rebuild the items that save() wrote.
void TestCatalogSearch::saveAndLoadRoundTrip() {
    const QString filename = m_dir->path() + "/catalog.json";

    CatalogSlow saved;
    saved.addItem(makeItem("C:/apps/Firefox.exe", "Firefox", 42));
    saved.addItem(makeItem("C:/docs/notes.txt", "notes", 0));
    QVERIFY(saved.save(filename));

    CatalogSlow loaded;
    QVERIFY(loaded.load(filename));
    QCOMPARE(loaded.count(), 2);

    QList<CatItem> result = runSearch(loaded, "firefox");
    QCOMPARE(result.count(), 1);
    QCOMPARE(result.first().fullPath, QString("C:/apps/Firefox.exe"));
    // Usage counts are preserved, they are what the ranking reads
    QCOMPARE(result.first().usage, 42);
}

// A missing or unreadable file is the normal first run, not an error: load()
// reports it and leaves the catalog empty.
void TestCatalogSearch::loadRejectsABrokenFile() {
    CatalogSlow missing;
    QVERIFY(!missing.load(m_dir->path() + "/does-not-exist.json"));

    const QString broken = m_dir->path() + "/broken.json";
    QFile file(broken);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("this is not json");
    file.close();

    CatalogSlow catalog;
    QVERIFY(!catalog.load(broken));
    QCOMPARE(catalog.count(), 0);
}

// Entries that could never match anything (no path, no name, not even an
// object) are dropped individually: one corrupt entry must not cost the user
// the rest of the catalog.
void TestCatalogSearch::loadDropsMalformedItems() {
    const QString filename = m_dir->path() + "/partly-broken.json";

    QJsonArray items;
    items.append(catItemToJson(makeItem("C:/x/ok.exe", "ok", 3)));

    QJsonObject noName;
    noName["fullPath"] = "C:/x/noname.exe";
    items.append(noName);

    QJsonObject emptyName;
    emptyName["fullPath"] = "C:/x/empty.exe";
    emptyName["shortName"] = "";
    items.append(emptyName);

    items.append(QString("not an object"));

    QJsonObject root;
    root["version"] = 1;
    root["items"] = items;

    QFile file(filename);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QJsonDocument(root).toJson());
    file.close();

    CatalogSlow catalog;
    QVERIFY(catalog.load(filename));
    QCOMPARE(catalog.count(), 1);

    QList<CatItem> result = runSearch(catalog, "ok");
    QCOMPARE(result.count(), 1);
    QCOMPARE(result.first().usage, 3);
}

// A file from a newer release still loads: only the fields this build knows
// about are read, so downgrading costs ranking history, not the whole catalog.
void TestCatalogSearch::loadAcceptsANewerFileVersion() {
    const QString filename = m_dir->path() + "/future.json";

    QJsonArray items;
    items.append(catItemToJson(makeItem("C:/x/future.exe", "future", 1)));

    QJsonObject root;
    root["version"] = 999;
    root["items"] = items;

    QFile file(filename);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QJsonDocument(root).toJson());
    file.close();

    CatalogSlow catalog;
    QVERIFY(catalog.load(filename));
    QCOMPARE(catalog.count(), 1);
}

void TestCatalogSearch::loadReplacesExistingContent() {
    const QString first = m_dir->path() + "/first.json";
    const QString second = m_dir->path() + "/second.json";

    CatalogSlow a;
    a.addItem(makeItem("C:/x/alpha.txt", "alpha", 0));
    QVERIFY(a.save(first));

    CatalogSlow b;
    b.addItem(makeItem("C:/x/beta.txt", "beta", 0));
    QVERIFY(b.save(second));

    CatalogSlow catalog;
    QVERIFY(catalog.load(first));
    QCOMPARE(catalog.count(), 1);

    QVERIFY(catalog.load(second));
    QCOMPARE(catalog.count(), 1);
    QCOMPARE(runSearch(catalog, "beta").count(), 1);
    QCOMPARE(runSearch(catalog, "alpha").count(), 0);
}

// Highlighting works per character of the subsequence match, not per word: a
// query of "np" lights up the n and the p of "notepad" and nothing between.
void TestCatalogSearch::decorateTextMarksEveryMatchedCharacter() {
    g_settings->setValue(OPTION_DECORATETEXT, true);

    QCOMPARE(Catalog::decorateText("notepad", "np"), QString("&note&pad"));

    // Multi-word queries in any order mark every consumed character
    QCOMPARE(Catalog::decorateText("book reviews", "reviews book"),
             QString("&b&o&o&k &r&e&v&i&e&w&s"));
}

// The item view draws the result as rich text, where a match is underlined
// instead of prefixed with '&'.
void TestCatalogSearch::decorateTextProducesRichText() {
    g_settings->setValue(OPTION_DECORATETEXT, true);

    QCOMPARE(Catalog::decorateText("notepad", "np", true),
             QString("<u>n</u>ote<u>p</u>ad"));
    // Adjacent matches share one underline run, a gap closes it
    QCOMPARE(Catalog::decorateText("book reviews", "reviews book", true),
             QString("<u>book</u> <u>reviews</u>"));
}

void TestCatalogSearch::decorateTextDisabledBySetting() {
    g_settings->setValue(OPTION_DECORATETEXT, false);

    QCOMPARE(Catalog::decorateText("notepad", "np"), QString("notepad"));
    QCOMPARE(Catalog::decorateText("notepad", "np", true), QString("notepad"));
}

QTEST_GUILESS_MAIN(TestCatalogSearch)
#include "TestCatalogSearch.moc"
