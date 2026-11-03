/*
LaunchyQt: CatalogFast unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include <QSettings>
#include <QSharedPointer>
#include <QTemporaryDir>

#include "Core/CatalogFast.h"
#include "Core/CatalogSlow.h"
#include "Core/CoreGlobals.h"
#include "Core/OptionItem.h"

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

// getItem() is protected on purpose (only save() and search() need it), but the
// tests have to read single fields back out of the catalog. Exposing it through
// a probe is cheaper and clearer than searching for the item and hoping the
// ranking surfaces it.
class CatalogProbe : public CatalogFast {
public:
    using CatalogFast::getItem;

    CatItem find(const QString& shortName) {
        for (int i = 0; i < count(); ++i) {
            if (getItem(i).shortName == shortName) {
                return getItem(i);
            }
        }
        return CatItem();
    }

    QStringList allShortNames() {
        QStringList names;
        for (int i = 0; i < count(); ++i) {
            names.append(getItem(i).shortName);
        }
        names.sort();
        return names;
    }
};

} // namespace

// CatalogFast keeps the items in a QSet plus a flat snapshot and a trigram
// index. The cases below cover what that structure adds on top of the shared
// ranking policy: replace-on-add, usage bookkeeping, purge, and the index
// having to stay in step with the snapshot.
class TestCatalogFast : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    // --- adding items ---
    void loadingPhaseKeepsTheFirstEqualItem();
    void duplicateWhileLoadingDiffersFromSlowCatalog();
    void rebuildRefreshesFieldsAndKeepsUsage();
    void usageChangeIgnoresAnUnknownItem();

    // --- usage bookkeeping ---
    void incrementUsageRevivesADemotedItem();
    void demoteItemGoesNegativeAndKeepsGoing();

    // --- purge ---
    void purgeDropsItemsTheRebuildDidNotSee();

    // --- search ---
    void searchAgreesWithSlowCatalog();
    void singleCharacterQueryScansEveryItem();
    void multiWordQueryNeedsEveryWord();
    void mutationInvalidatesTheIndex();
    void demotedItemRanksAfterItsSibling();
    void subsequenceWithoutATrigramIsNotReturned();

    // --- lifecycle ---
    void clearDropsEverything();
    void saveAndLoadRoundTrip();

private:
    QScopedPointer<QTemporaryDir> m_dir;
};

void TestCatalogFast::initTestCase() {
    m_dir.reset(new QTemporaryDir);
    QVERIFY(m_dir->isValid());

    // searchCatalogs() reads the result cap from the settings store
    g_settings = QSharedPointer<QSettings>(
        new QSettings(m_dir->path() + "/catalog_fast_test.ini", QSettings::IniFormat));
    QVERIFY(!g_settings.isNull());
    g_settings->setValue(OPTION_NUMRESULT, 100);
}

void TestCatalogFast::init() {
    g_settings->setValue(OPTION_NUMRESULT, 100);
}

void TestCatalogFast::cleanupTestCase() {
    // Same shutdown order as the application: release the shared QSettings
    // while the exe is still alive (a test that skips this dies with
    // 0xC0000409 after passing).
    if (!g_settings.isNull()) {
        g_settings->sync();
    }
    g_settings.clear();
    m_dir.reset();
}

// While the catalog is being loaded (timestamp 0) the first entry of an equal
// pair wins. That is where CatalogFast and CatalogSlow deliberately differ:
// the slow catalog appends the duplicate, the set cannot hold two equal items
// and keeps the one it already has.
void TestCatalogFast::loadingPhaseKeepsTheFirstEqualItem() {
    CatalogProbe catalog;

    CatItem first = makeItem("C:/x/tool.exe", "tool");
    first.iconPath = "first.ico";
    catalog.addItem(first);

    CatItem second = makeItem("C:/x/tool.exe", "tool");
    second.iconPath = "second.ico";
    catalog.addItem(second);

    QCOMPARE(catalog.count(), 1);
    QCOMPARE(catalog.find("tool").iconPath, QString("first.ico"));
}

// The counterweight to the case above: the slow catalog really does append the
// duplicate while loading. The difference is deliberate (a set cannot hold two
// equal items) and only matters for a file that was saved with duplicates in
// it; locked so nobody "fixes" one side to match the other by accident.
void TestCatalogFast::duplicateWhileLoadingDiffersFromSlowCatalog() {
    const CatItem item = makeItem("C:/x/tool.exe", "tool", 0);

    CatalogFast fast;
    CatalogSlow slow;
    fast.addItem(item);
    fast.addItem(item);
    slow.addItem(item);
    slow.addItem(item);

    QCOMPARE(fast.count(), 1);
    QCOMPARE(slow.count(), 2);
}

// During a rebuild the item is refreshed in place: every field comes from the
// new copy except the usage count, which survives. Losing it would reset every
// learned ranking on each index refresh.
void TestCatalogFast::rebuildRefreshesFieldsAndKeepsUsage() {
    CatalogProbe catalog;
    catalog.addItem(makeItem("C:/x/tool.exe", "tool", 7));

    catalog.incrementTimestamp();

    CatItem refreshed = makeItem("C:/x/tool.exe", "tool", 0);
    refreshed.iconPath = "new.ico";
    refreshed.pluginName = "Launcher";
    catalog.addItem(refreshed);

    QCOMPARE(catalog.count(), 1);
    const CatItem stored = catalog.find("tool");
    QCOMPARE(stored.usage, 7);
    QCOMPARE(stored.iconPath, QString("new.ico"));
    QCOMPARE(stored.pluginName, QString("Launcher"));
}

// incrementUsage/demoteItem only look the item up; they never insert. A path
// that is not in the catalog must leave it untouched instead of adding a
// half-built entry.
void TestCatalogFast::usageChangeIgnoresAnUnknownItem() {
    CatalogProbe catalog;
    catalog.addItem(makeItem("C:/x/known.exe", "known", 0));

    catalog.incrementUsage(makeItem("C:/x/unknown.exe", "unknown", 0));
    catalog.demoteItem(makeItem("C:/x/unknown.exe", "unknown", 0));

    QCOMPARE(catalog.count(), 1);
    QCOMPARE(catalog.allShortNames(), QStringList({"known"}));
    QCOMPARE(catalog.find("known").usage, 0);
}

void TestCatalogFast::incrementUsageRevivesADemotedItem() {
    CatalogProbe catalog;
    catalog.addItem(makeItem("C:/x/calc.exe", "calc", 0));

    catalog.incrementUsage(makeItem("C:/x/calc.exe", "calc", 0));
    QCOMPARE(catalog.find("calc").usage, 1);

    // A demoted item comes back at 1, not at 0: launching it again is a vote
    // for it, and 0 would leave it indistinguishable from never launched
    catalog.demoteItem(makeItem("C:/x/calc.exe", "calc", 0));
    QCOMPARE(catalog.find("calc").usage, -1);
    catalog.incrementUsage(makeItem("C:/x/calc.exe", "calc", 0));
    QCOMPARE(catalog.find("calc").usage, 1);
}

void TestCatalogFast::demoteItemGoesNegativeAndKeepsGoing() {
    CatalogProbe catalog;
    catalog.addItem(makeItem("C:/x/calc.exe", "calc", 3));

    catalog.demoteItem(makeItem("C:/x/calc.exe", "calc", 0));
    QCOMPARE(catalog.find("calc").usage, -1);

    // Demoting an already demoted item keeps pushing it down
    catalog.demoteItem(makeItem("C:/x/calc.exe", "calc", 0));
    QCOMPARE(catalog.find("calc").usage, -2);
}

// purgeOldItems() drops everything the last rebuild did not touch: an item
// whose file disappeared is never re-added, so its timestamp stays behind.
void TestCatalogFast::purgeDropsItemsTheRebuildDidNotSee() {
    CatalogProbe catalog;
    catalog.addItem(makeItem("C:/x/alpha.txt", "alpha", 0));
    catalog.addItem(makeItem("C:/x/beta.txt", "beta", 0));

    catalog.incrementTimestamp();
    catalog.addItem(makeItem("C:/x/beta.txt", "beta", 0)); // seen again
    catalog.purgeOldItems();

    QCOMPARE(catalog.count(), 1);
    QCOMPARE(catalog.allShortNames(), QStringList({"beta"}));
    QCOMPARE(runSearch(catalog, "alpha").count(), 0);
}

// The trigram index is an optimization, it must not change the answer. Same
// items, same queries, both catalogs: identical results.
void TestCatalogFast::searchAgreesWithSlowCatalog() {
    const QList<CatItem> items = {
        makeItem("C:/apps/firefox.exe", "firefox", 5),
        makeItem("C:/docs/campfire.jpg", "campfire", 50),
        makeItem("C:/docs/book reviews of Nassim Taleb.txt",
                 "book reviews of Nassim Taleb", 0),
        makeItem("C:/apps/notepad.exe", "notepad", 0),
        makeItem("C:/apps/notepad++.exe", "notepad++", 100),
        makeItem("C:/x/abcdef.txt", "abcdef", 0),
    };

    CatalogFast fast;
    CatalogSlow slow;
    for (const CatItem& item : items) {
        fast.addItem(item);
        slow.addItem(item);
    }

    const QStringList queries = {
        "fire",          // prefix + usage
        "camp",          // contiguous in the middle
        "notepad",       // exact name
        "book reviews",  // two words, both contiguous
        "tale",          // trigram inside a word
        "campf",         // trigram at the end of a word
        "abcdef",        // exact, long
        "zzz",           // no match at all
    };

    for (const QString& query : queries) {
        // Compare the result SETS, not the order: searchCatalogs() only ranks
        // when the cap cuts the match set, and these queries fit inside it. The
        // two catalogs then hand back whatever their own walk produced
        // (insertion order vs. index order). Ordering the merged list is the
        // caller's job - LaunchyWidget::doSearch() sorts it.
        QStringList fromFast = shortNames(runSearch(fast, query));
        QStringList fromSlow = shortNames(runSearch(slow, query));
        fromFast.sort();
        fromSlow.sort();
        QCOMPARE(fromFast, fromSlow);
    }
}

// Queries shorter than one trigram cannot use the index at all. The fallback
// has to be a full scan, not "return nothing".
void TestCatalogFast::singleCharacterQueryScansEveryItem() {
    CatalogFast catalog;
    catalog.addItem(makeItem("C:/x/qgit.exe", "qgit", 0));
    catalog.addItem(makeItem("C:/x/abcq.txt", "abcq", 0));
    catalog.addItem(makeItem("C:/x/plain.txt", "plain", 0));

    QStringList found = shortNames(runSearch(catalog, "q"));
    found.sort();
    QCOMPARE(found, QStringList({"abcq", "qgit"}));
}

// The index intersects the candidate sets of all query words, so a word that
// matches nothing must produce no result even when the other word matches.
void TestCatalogFast::multiWordQueryNeedsEveryWord() {
    CatalogFast catalog;
    catalog.addItem(makeItem("C:/apps/firefox.exe", "firefox", 0));
    catalog.addItem(makeItem("C:/docs/book.txt", "book", 0));

    QCOMPARE(runSearch(catalog, "fire book").count(), 0);
    QCOMPARE(runSearch(catalog, "fire").count(), 1);
    QCOMPARE(runSearch(catalog, "book").count(), 1);
}

// Adding an item invalidates both the snapshot and the index. If either were
// left stale, an item added after the first search would never be found.
void TestCatalogFast::mutationInvalidatesTheIndex() {
    CatalogFast catalog;
    catalog.addItem(makeItem("C:/x/alpha.txt", "alpha", 0));
    QCOMPARE(runSearch(catalog, "alpha").count(), 1);

    catalog.addItem(makeItem("C:/x/alphabet.txt", "alphabet", 0));
    QStringList found = shortNames(runSearch(catalog, "alpha"));
    found.sort();
    QCOMPARE(found, QStringList({"alpha", "alphabet"}));

    // The same through a usage change, which replaces the stored item. One slot,
    // so the catalog ranks and the demoted item has to lose it.
    catalog.demoteItem(makeItem("C:/x/alpha.txt", "alpha", 0));
    g_settings->setValue(OPTION_NUMRESULT, 1);
    QCOMPARE(shortNames(runSearch(catalog, "alpha")), QStringList({"alphabet"}));
}

// A demoted item has negative usage, which the ranking policy pushes below
// every non-demoted item - even when its own name is the better match.
void TestCatalogFast::demotedItemRanksAfterItsSibling() {
    // One slot for two matches: the cap has to cut, so the catalog ranks
    g_settings->setValue(OPTION_NUMRESULT, 1);

    CatalogFast catalog;
    catalog.addItem(makeItem("C:/win/calc.exe", "calc", 0));
    catalog.addItem(makeItem("C:/win/calculator.exe", "calculator", 0));

    QCOMPARE(shortNames(runSearch(catalog, "calc")), QStringList({"calc"}));

    catalog.demoteItem(makeItem("C:/win/calc.exe", "calc", 0));
    QCOMPARE(shortNames(runSearch(catalog, "calc")), QStringList({"calculator"}));
}

// Known trade-off of the trigram index (documented in CatalogFast.h): a query
// word is only looked up through its contiguous 3-char substrings, so a match
// that exists as a subsequence but shares no trigram is not returned. Catalog
// Slow, which scans every item, does find it. Locked here so the difference
// stays a deliberate choice rather than a silent regression.
void TestCatalogFast::subsequenceWithoutATrigramIsNotReturned() {
    const CatItem item = makeItem("C:/docs/a-b-c.txt", "a-b-c", 0);

    CatalogFast fast;
    CatalogSlow slow;
    fast.addItem(item);
    slow.addItem(item);

    // "abc" is a subsequence of "a-b-c" but no contiguous "abc" exists
    QCOMPARE(runSearch(slow, "abc").count(), 1);
    QCOMPARE(runSearch(fast, "abc").count(), 0);
}

void TestCatalogFast::clearDropsEverything() {
    CatalogProbe catalog;
    catalog.addItem(makeItem("C:/x/alpha.txt", "alpha", 0));
    catalog.addItem(makeItem("C:/x/beta.txt", "beta", 0));

    catalog.clear();

    QCOMPARE(catalog.count(), 0);
    QCOMPARE(catalog.allShortNames(), QStringList());
    QCOMPARE(runSearch(catalog, "alpha").count(), 0);
}

// save() walks getItem(i), i.e. the snapshot, so the round trip also proves the
// snapshot is rebuilt on demand after a change.
void TestCatalogFast::saveAndLoadRoundTrip() {
    const QString filename = m_dir->path() + "/catalog_fast.json";

    CatalogFast saved;
    saved.addItem(makeItem("C:/apps/Firefox.exe", "Firefox", 42));
    saved.addItem(makeItem("C:/docs/notes.txt", "notes", 0));
    QVERIFY(saved.save(filename));

    CatalogFast loaded;
    QVERIFY(loaded.load(filename));
    QCOMPARE(loaded.count(), 2);

    const QList<CatItem> result = runSearch(loaded, "firefox");
    QCOMPARE(result.count(), 1);
    QCOMPARE(result.first().fullPath, QString("C:/apps/Firefox.exe"));
    QCOMPARE(result.first().usage, 42);
}

QTEST_GUILESS_MAIN(TestCatalogFast)
#include "TestCatalogFast.moc"
