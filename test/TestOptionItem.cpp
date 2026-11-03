/*
LaunchyQt: option key and default value unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include <QNetworkProxy>
#include <QString>
#include <QStringList>
#include <Qt>

#include "Core/CoreGlobals.h"
#include "Core/OptionItem.h"

using namespace launchy;

namespace {

// Every key declared in OptionItem.h, in the order the header lists them.
//
// The platform has no way to enumerate the externs, so this list is written by
// hand: it is the one place that can see all keys at once, and it has to grow
// with the header. What it buys is the check no compiler makes -- that two
// options never end up on the same string. A copied block with an unrenamed key
// still compiles, still reads and writes, and silently shares the other
// option's value in the ini.
QStringList allOptionKeys() {
    return QStringList()
        // general
        << OPTION_VERSION << OPTION_POS << OPTION_HOTKEY
        << OPTION_HOTKEYMOD << OPTION_HOTKEYKEY
        << OPTION_IGNORE_FULL_SCREEN << OPTION_ALWAYSSHOW << OPTION_ALWAYSTOP
        << OPTION_HIDEIFLOSTFOCUS << OPTION_ALWAYSCENTER << OPTION_DRAGMODE
        << OPTION_SCREEN_INDEX << OPTION_HIDE_TRAY_ICON << OPTION_APPSTYLE
        << OPTION_DECORATETEXT << OPTION_SEARCH_MODE << OPTION_AUTOSUGGESTDELAY
        << OPTION_NUMVIEWABLE << OPTION_NUMRESULT << OPTION_MAXITEMSINHISTORY
        << OPTION_CONDENSEDVIEW << OPTION_OPAQUENESS << OPTION_FADEIN
        << OPTION_FADEOUT << OPTION_RESOLVE_SYM_LINK << OPTION_SKIN
        // plugin
        << OPTION_PLUGIN_EXTRA_DIRECTORY
        // update
        << OPTION_UPDATE_CHECK_ON_STARTUP << OPTION_UPDATE_CHECK_ON_STARTUP_DELAY
        << OPTION_UPDATE_CHECK_INTERVAL << OPTION_UPDATE_LAST_CHECK
        // proxy
        << OPTION_PROXY_TYPE << OPTION_PROXY_SERVER_NAME
        << OPTION_PROXY_SERVER_PORT << OPTION_PROXY_REQUIRE_PASSWORD
        << OPTION_PROXY_USERNAME << OPTION_PROXY_PASSWORD
        // system
        << OPTION_REBUILDTIMER << OPTION_SHOWHIDDENFILES << OPTION_SHOWNETWORK
        << OPTION_DEBUG_LOG << OPTION_LANGUAGE;
}

} // namespace

class TestOptionItem : public QObject {
    Q_OBJECT

private slots:
    // --- key hygiene ---
    void keysAreGroupedAndWellFormed();
    void keysAreUnique();
    void keysStayInsideTheKnownGroups();

    // --- defaults the window manager/UI arithmetic depends on ---
    void resultCapIsAtLeastTheVisibleRows();
    void numericDefaultsAreInRange();
    void proxyDefaultIsTheEnumeratorNotTheComboIndex();
    void hotkeyDefaultsAreSet();

    // --- catalog name constants ---
    void catalogNamesAreDistinct();
};

void TestOptionItem::keysAreGroupedAndWellFormed() {
    const QStringList keys = allOptionKeys();
    QVERIFY(keys.size() >= 40);

    for (const QString& key : keys) {
        // "group/name": QSettings nests on the first slash, so a key without
        // one lands at the top of the ini next to unrelated values
        QCOMPARE(key.count(QLatin1Char('/')), 1);
        QVERIFY(!key.startsWith(QLatin1Char('/')));
        QVERIFY(!key.endsWith(QLatin1Char('/')));

        // Whitespace is a silent trap: "GenOps/ appStyle" is a different key
        // from "GenOps/appStyle", and the option would simply never be read
        for (const QChar ch : key) {
            QVERIFY2(!ch.isSpace(), qPrintable("key has whitespace: '" + key + "'"));
        }
        QCOMPARE(key, key.trimmed());
    }
}

void TestOptionItem::keysAreUnique() {
    const QStringList keys = allOptionKeys();
    for (int i = 0; i < keys.size(); ++i) {
        for (int j = i + 1; j < keys.size(); ++j) {
            QVERIFY2(keys[i] != keys[j],
                     qPrintable("duplicate option key: " + keys[i]));
        }
    }
}

void TestOptionItem::keysStayInsideTheKnownGroups() {
    // An unexpected group is not a bug by itself, but it means a typo in the
    // prefix ("Genopts/...") which would strand the value in its own group.
    const QStringList groups = {"GenOps", "Display", "Plugin", "Update",
                                "Proxy", "System"};
    for (const QString& key : allOptionKeys()) {
        const QString group = key.section(QLatin1Char('/'), 0, 0);
        QVERIFY2(groups.contains(group),
                 qPrintable("unknown option group: " + key));
    }
}

// The catalog fills at most numresult entries and the list widget shows
// numviewable of them. A default the other way round leaves the window unable
// to fill the rows it was built for.
void TestOptionItem::resultCapIsAtLeastTheVisibleRows() {
    QVERIFY(OPTION_NUMRESULT_DEFAULT > 0);
    QVERIFY(OPTION_NUMVIEWABLE_DEFAULT > 0);
    QVERIFY(OPTION_NUMRESULT_DEFAULT >= OPTION_NUMVIEWABLE_DEFAULT);
}

void TestOptionItem::numericDefaultsAreInRange() {
    // search mode is the combo index in OptionDialog (0 = none, 1 = fast,
    // 2 = fuzzy); a value outside the range makes CatalogBuilder fall back to
    // fast without telling anyone
    QVERIFY(OPTION_SEARCH_MODE_DEFAULT >= 0);
    QVERIFY(OPTION_SEARCH_MODE_DEFAULT <= 2);

    // Fader divides opaqueness by 100: 0 would make the window invisible and
    // the launcher look dead on first start
    QVERIFY(OPTION_OPAQUENESS_DEFAULT > 0);
    QVERIFY(OPTION_OPAQUENESS_DEFAULT <= 100);

    // Negative timeouts are passed straight to QTimer::start()
    QVERIFY(OPTION_FADEIN_DEFAULT >= 0);
    QVERIFY(OPTION_FADEOUT_DEFAULT >= 0);
    QVERIFY(OPTION_AUTOSUGGESTDELAY_DEFAULT >= 0);
    QVERIFY(OPTION_UPDATE_CHECK_ON_STARTUP_DELAY_DEFAULT >= 0);

    // A zero rebuild timer would ask for a rebuild on every pass
    QVERIFY(OPTION_REBUILDTIMER_DEFAULT > 0);
    // Interval is in hours; zero would pass the "next check" test on every start
    QVERIFY(OPTION_UPDATE_CHECK_INTERVAL_DEFAULT > 0);
    // A zero history limit disables the usage ranking that ranking relies on
    QVERIFY(OPTION_MAXITEMSINHISTORY_DEFAULT > 0);
}

// The ini stores the QNetworkProxy enumerator, not the combo index: the dialog
// reads currentData() and keeps the order NoProxy/DefaultProxy/HTTP/SOCKS5 on
// purpose. Anyone who "fixes" the default to match the combo order turns
// NoProxy (2) into an HTTP proxy for every user.
void TestOptionItem::proxyDefaultIsTheEnumeratorNotTheComboIndex() {
    QCOMPARE(OPTION_PROXY_TYPE_DEFAULT, static_cast<int>(QNetworkProxy::NoProxy));
    QVERIFY(OPTION_PROXY_SERVER_NAME_DEFAULT != nullptr);
    QVERIFY(OPTION_PROXY_SERVER_PORT_DEFAULT != nullptr);
    QVERIFY(OPTION_PROXY_USERNAME_DEFAULT != nullptr);
    QVERIFY(OPTION_PROXY_PASSWORD_DEFAULT != nullptr);
}

void TestOptionItem::hotkeyDefaultsAreSet() {
    // Key 0 is Qt::Key_unknown: the global hotkey would be registered for a
    // key that never arrives, i.e. the launcher stops opening
    QVERIFY(OPTION_HOTKEYKEY_DEFAULT > 0);
    QVERIFY(OPTION_HOTKEYMOD_DEFAULT != 0);

    QVERIFY(OPTION_APPSTYLE_DEFAULT != nullptr);
    QVERIFY(QString(OPTION_APPSTYLE_DEFAULT).length() > 0);
    QVERIFY(OPTION_SKIN_DEFAULT != nullptr);
    QVERIFY(QString(OPTION_SKIN_DEFAULT).length() > 0);
}

// CommandHistory stamps its rows with NAME_HISTORY and FileSearch with
// NAME_LAUNCHYFILE; pages of the option dialog compare against them. Equal
// constants would make a history row look like a file row.
void TestOptionItem::catalogNamesAreDistinct() {
    QVERIFY(!NAME_LAUNCHY.isEmpty());
    QVERIFY(!NAME_HISTORY.isEmpty());
    QVERIFY(!NAME_LAUNCHYFILE.isEmpty());

    QVERIFY(NAME_LAUNCHY != NAME_HISTORY);
    QVERIFY(NAME_LAUNCHY != NAME_LAUNCHYFILE);
    QVERIFY(NAME_HISTORY != NAME_LAUNCHYFILE);

    QVERIFY(!LABEL_FILE.isEmpty());
    QVERIFY(!LABEL_HISTORY.isEmpty());
    QVERIFY(!LABEL_AUTOSUGGEST.isEmpty());
    QVERIFY(LABEL_FILE != LABEL_HISTORY);
    QVERIFY(LABEL_FILE != LABEL_AUTOSUGGEST);
    QVERIFY(LABEL_HISTORY != LABEL_AUTOSUGGEST);
}

QTEST_APPLESS_MAIN(TestOptionItem)

#include "TestOptionItem.moc"
