/*
LaunchyQt: CatalogSlow unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include <QSettings>
#include <QSharedPointer>
#include <QTemporaryDir>

#include "Core/CatalogSlow.h"

#include "PluginInterface/Runtime.h"

using namespace launchy;

namespace {

CatItem makeItem(const QString& path, const QString& shortName, int usage = 0) {
    CatItem item(path, shortName);
    item.usage = usage;
    return item;
}

// getItem() is protected (only save() and search() need it), the tests read
// single fields back out through a probe.
class CatalogProbe : public CatalogSlow {
public:
    using CatalogSlow::getItem;

    CatItem find(const QString& shortName) {
        for (int i = 0; i < count(); ++i) {
            if (getItem(i).shortName == shortName) {
                return getItem(i);
            }
        }
        return CatItem();
    }
};

} // namespace

// CatalogFast has its own suite; this one covers the parts of CatalogSlow that
// its linear scan plus position index implement on their own -- in particular
// that the index survives a purge, which is where the positions it stores stop
// matching the item they pointed at.
class TestCatalogSlow : public QObject {
    Q_OBJECT

private slots:
    void rebuildRefreshesFieldsAndKeepsUsage();
    void incrementUsageTouchesOnlyTheFirstOfTwoEqualItems();
    void incrementUsageRevivesADemotedItem();
    void demoteItemGoesNegativeAndKeepsGoing();
    void purgeReindexesTheItemsThatSurvived();
    void purgeWithoutRemovalsKeepsTheIndexUsable();
};

// During a rebuild the copy already in the catalog is refreshed in place: every
// field comes from the new item except the usage count.
void TestCatalogSlow::rebuildRefreshesFieldsAndKeepsUsage() {
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

// While loading, a duplicated entry is appended instead of replaced, but the
// index keeps pointing at the first one: every later usage change lands there.
// This is the documented difference from CatalogFast, which cannot hold two
// equal items at all.
void TestCatalogSlow::incrementUsageTouchesOnlyTheFirstOfTwoEqualItems() {
    CatalogProbe catalog;
    const CatItem item = makeItem("C:/x/tool.exe", "tool", 0);
    catalog.addItem(item);
    catalog.addItem(item);

    QCOMPARE(catalog.count(), 2);

    catalog.incrementUsage(item);

    QCOMPARE(catalog.getItem(0).usage, 1);
    QCOMPARE(catalog.getItem(1).usage, 0);
}

void TestCatalogSlow::incrementUsageRevivesADemotedItem() {
    CatalogProbe catalog;
    const CatItem item = makeItem("C:/x/calc.exe", "calc", 0);
    catalog.addItem(item);

    catalog.incrementUsage(item);
    QCOMPARE(catalog.find("calc").usage, 1);

    // A demoted item comes back at 1, not 0: launching it again is a vote for
    // it, and 0 would look like "never launched"
    catalog.demoteItem(item);
    QCOMPARE(catalog.find("calc").usage, -1);
    catalog.incrementUsage(item);
    QCOMPARE(catalog.find("calc").usage, 1);
}

void TestCatalogSlow::demoteItemGoesNegativeAndKeepsGoing() {
    CatalogProbe catalog;
    const CatItem item = makeItem("C:/x/calc.exe", "calc", 3);
    catalog.addItem(item);

    catalog.demoteItem(item);
    QCOMPARE(catalog.find("calc").usage, -1);

    catalog.demoteItem(item);
    QCOMPARE(catalog.find("calc").usage, -2);
}

// The case the position index can get wrong: removing an earlier item shifts
// every later one, so without the rebuild the index would point at the wrong
// entry and the usage count of a surviving item would go to a neighbour.
void TestCatalogSlow::purgeReindexesTheItemsThatSurvived() {
    CatalogProbe catalog;
    catalog.addItem(makeItem("C:/x/alpha.exe", "alpha", 3));
    catalog.addItem(makeItem("C:/x/beta.exe", "beta", 4));
    catalog.addItem(makeItem("C:/x/gamma.exe", "gamma", 5));

    // Rebuild: only gamma is still on disk
    catalog.incrementTimestamp();
    catalog.addItem(makeItem("C:/x/gamma.exe", "gamma", 0));

    catalog.purgeOldItems();

    QCOMPARE(catalog.count(), 1);
    QCOMPARE(catalog.find("gamma").usage, 5);

    // gamma now sits at position 0; the index has to have followed it
    catalog.incrementUsage(makeItem("C:/x/gamma.exe", "gamma", 0));
    QCOMPARE(catalog.count(), 1);
    QCOMPARE(catalog.getItem(0).shortName, QString("gamma"));
    QCOMPARE(catalog.getItem(0).usage, 6);
}

// A rebuild that saw every item removes nothing, and purgeOldItems() returns
// before rebuilding the index. The index is still valid then -- but only
// because nothing was removed, which is what this case pins down.
void TestCatalogSlow::purgeWithoutRemovalsKeepsTheIndexUsable() {
    CatalogProbe catalog;
    catalog.addItem(makeItem("C:/x/alpha.exe", "alpha", 1));
    catalog.addItem(makeItem("C:/x/beta.exe", "beta", 2));

    catalog.incrementTimestamp();
    catalog.addItem(makeItem("C:/x/alpha.exe", "alpha", 0));
    catalog.addItem(makeItem("C:/x/beta.exe", "beta", 0));

    catalog.purgeOldItems();

    QCOMPARE(catalog.count(), 2);
    QCOMPARE(catalog.find("alpha").usage, 1);
    QCOMPARE(catalog.find("beta").usage, 2);

    catalog.incrementUsage(makeItem("C:/x/beta.exe", "beta", 0));
    QCOMPARE(catalog.find("beta").usage, 3);
}

QTEST_APPLESS_MAIN(TestCatalogSlow)

#include "TestCatalogSlow.moc"
