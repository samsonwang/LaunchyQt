/*
LaunchyQt: command history unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include <QSettings>
#include <QTemporaryDir>
#include <QSharedPointer>

#include "PluginInterface/PluginLabels.h"
#include "PluginInterface/Runtime.h"

#include "Core/CommandHistory.h"
#include "Core/InputDataList.h"
#include "Core/OptionItem.h"


using namespace launchy;

namespace {

// One query segment, exactly as the widget builds it: "plugin ► argument" with
// the chosen result attached to the first segment.
InputDataList makeCommand(const QString& plugin, const QString& argument,
                          const QString& target) {
    InputDataList list;
    list.parse(plugin + " " + QString(QChar(0x25ba)) + " " + argument);

    // Every segment carries the result it was resolved to. The store rejects a
    // command holding an empty one, since it could never be relaunched.
    for (int i = 0; i < list.count(); ++i) {
        const QString text = list.at(i).getText();
        list[i].setTopResult(CatItem(target + "/" + text, text));
    }
    return list;
}

} // namespace

class TestCommandHistory : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    void addItemKeepsNewestFirst();
    void addItemPromotesExistingEntry();
    void addItemHonoursHistoryLimit();
    void removeAtDropsOnlyThatEntry();
    void getItemOutOfRangeIsEmpty();
    void promotionMovesToFront();
    void saveLoadRoundTrip();
    void searchSkipsSingleSegmentCommands();

private:
    // Every case writes its own file: QSettings and CommandHistory both cache
    // per file name, so sharing one would make the cases order dependent.
    QString historyPath(const QString& name) const;

private:
    QTemporaryDir m_dir;
};

QString TestCommandHistory::historyPath(const QString& name) const {
    return m_dir.path() + "/" + name + ".json";
}

void TestCommandHistory::initTestCase() {
    QVERIFY(m_dir.isValid());

    // addItem() reads the history cap from the shared settings object, which
    // the application installs. Give the test its own file.
    g_settings.reset(new QSettings(m_dir.path() + "/test.ini", QSettings::IniFormat));
    g_settings->setValue(OPTION_MAXITEMSINHISTORY, OPTION_MAXITEMSINHISTORY_DEFAULT);
}

void TestCommandHistory::init() {
    g_settings->setValue(OPTION_MAXITEMSINHISTORY, OPTION_MAXITEMSINHISTORY_DEFAULT);
}

void TestCommandHistory::cleanupTestCase() {
    // Without this the process dies in Launchy.dll's static teardown after all
    // tests pass (0xC0000409). See the note in test/README or the project log.
    g_settings.clear();
}

void TestCommandHistory::addItemKeepsNewestFirst() {
    CommandHistory history;
    history.load(historyPath("order"));

    history.addItem(makeCommand("google", "first", "C:/first"));
    history.addItem(makeCommand("google", "second", "C:/second"));

    QCOMPARE(history.getItemCount(), 2);
    // Newest first: that is what the history search rows assume
    QCOMPARE(history.getItem(0).toString(), QString("google \u25ba second"));
    QCOMPARE(history.getItem(1).toString(), QString("google \u25ba first"));
}

void TestCommandHistory::addItemPromotesExistingEntry() {
    CommandHistory history;
    history.load(historyPath("dedupe"));

    history.addItem(makeCommand("google", "first", "C:/first"));
    history.addItem(makeCommand("google", "second", "C:/second"));
    // Same text again: it must move up, not be stored twice
    history.addItem(makeCommand("google", "first", "C:/first"));

    QCOMPARE(history.getItemCount(), 2);
    QCOMPARE(history.getItem(0).toString(), QString("google \u25ba first"));
}

void TestCommandHistory::addItemHonoursHistoryLimit() {
    CommandHistory history;
    history.load(historyPath("limit"));

    g_settings->setValue(OPTION_MAXITEMSINHISTORY, 2);

    history.addItem(makeCommand("google", "one", "C:/1"));
    history.addItem(makeCommand("google", "two", "C:/2"));
    history.addItem(makeCommand("google", "three", "C:/3"));

    QCOMPARE(history.getItemCount(), 2);
    QCOMPARE(history.getItem(0).toString(), QString("google \u25ba three"));
    QCOMPARE(history.getItem(1).toString(), QString("google \u25ba two"));
}

void TestCommandHistory::removeAtDropsOnlyThatEntry() {
    CommandHistory history;
    history.load(historyPath("remove"));

    history.addItem(makeCommand("google", "one", "C:/1"));
    history.addItem(makeCommand("google", "two", "C:/2"));
    history.removeAt(1);

    QCOMPARE(history.getItemCount(), 1);
    QCOMPARE(history.getItem(0).toString(), QString("google \u25ba two"));

    // Out of range is ignored, not an assert: the caller may hold a stale index
    history.removeAt(5);
    QCOMPARE(history.getItemCount(), 1);
}

void TestCommandHistory::getItemOutOfRangeIsEmpty() {
    CommandHistory history;
    history.load(historyPath("bounds"));

    QVERIFY(history.getItem(-1).isEmpty());
    QVERIFY(history.getItem(0).isEmpty());
}

void TestCommandHistory::promotionMovesToFront() {
    CommandHistory history;
    history.load(historyPath("promotion"));

    history.setPromotedItem("goo", "google", "C:/google.exe");
    history.setPromotedItem("cal", "calc", "C:/calc.exe");
    // Re-promoting the first query must move it back in front, not duplicate it
    history.setPromotedItem("goo", "google", "C:/google.exe");

    QString shortName;
    QString fullPath;
    QVERIFY(history.promotedItem("cal", shortName, fullPath));
    QCOMPARE(shortName, QString("calc"));
    QCOMPARE(fullPath, QString("C:/calc.exe"));

    // An empty query or item is refused
    history.setPromotedItem("", "x", "y");
    QVERIFY(!history.promotedItem("", shortName, fullPath));
}

void TestCommandHistory::saveLoadRoundTrip() {
    const QString path = historyPath("roundtrip");

    {
        CommandHistory history;
        history.load(path);
        history.addItem(makeCommand("google", "one", "C:/1"));
        history.addItem(makeCommand("google", "two", "C:/2"));
        history.setPromotedItem("goo", "google", "C:/1");
    }

    CommandHistory reloaded;
    QVERIFY(reloaded.load(path));
    QCOMPARE(reloaded.getItemCount(), 2);
    QCOMPARE(reloaded.getItem(0).toString(), QString("google \u25ba two"));

    QString shortName;
    QString fullPath;
    QVERIFY(reloaded.promotedItem("goo", shortName, fullPath));
    QCOMPARE(shortName, QString("google"));
    QCOMPARE(fullPath, QString("C:/1"));
}

void TestCommandHistory::searchSkipsSingleSegmentCommands() {
    CommandHistory history;
    history.load(historyPath("search"));

    // A bare alias belongs to the catalog, so a one-segment entry must never
    // come back as a history row.
    InputDataList bare;
    bare.parse("google");
    history.addItem(bare);
    history.addItem(makeCommand("google", "query", "C:/1"));

    QList<CatItem> results;
    history.search("query", results);
    QCOMPARE(results.count(), 1);
    QCOMPARE(results.first().pluginName, NAME_HISTORY);

    results.clear();
    history.search("google", results);
    // Only the two-segment command matches; the bare one is skipped
    QCOMPARE(results.count(), 1);
    QCOMPARE(results.first().shortName, QString("google \u25ba query"));
}

QTEST_APPLESS_MAIN(TestCommandHistory)

#include "TestCommandHistory.moc"
