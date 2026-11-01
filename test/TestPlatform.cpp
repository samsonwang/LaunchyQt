/*
LaunchyQt: PlatformBase seam unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include <QStringList>

#include "Platform/Platform.h"

#include "PluginInterface/CatalogItem.h"

using namespace launchy;

// Nothing installs a platform in the tests, so every call reaches the inert
// one. That is the half worth locking down here: a core consumer must get
// "this platform has nothing to offer" instead of a crash, and the answers must
// stay honest rather than pretending a capability exists. The real answers are
// the platform library's business, checked by running the application.
class TestPlatform : public QObject {
    Q_OBJECT

private slots:
    void cleanup();

    void inertPlatformSaysNoPlatform();
    void inertPlatformLeavesPathsAlone();
};

void TestPlatform::cleanup() {
    // Nothing is installed here, but a leaked platform would poison the next
    // test: the seam is process wide.
    cleanupPlatform();
}

void TestPlatform::inertPlatformSaysNoPlatform() {
    cleanupPlatform();

    QVERIFY(platform().directories().isEmpty());
    QVERIFY(platform().defaultCatalogDirectories().isEmpty());
    QVERIFY(!platform().supportsAlphaBorder());
    // A notification is the safe default: refusing one silently would make a
    // missing platform look like "do not disturb"
    QVERIFY(platform().allowNotification());
    QCOMPARE(platform().systemIdleSeconds(), -1);

    QStringList computers;
    QVERIFY(!platform().getComputers(computers));
    QVERIFY(computers.isEmpty());
}

// No platform means "cannot expand", not "expand to nothing": the configured
// value is still the best answer available.
void TestPlatform::inertPlatformLeavesPathsAlone() {
    cleanupPlatform();

    QCOMPARE(platform().expandEnvironmentVars("%TEMP%/Launchy"),
             QString("%TEMP%/Launchy"));

    // Letting a null platform rewrite an item would corrupt the index
    CatItem item("C:/x/tool.exe", "tool");
    platform().alterItem(&item);
    QCOMPARE(item.pluginName, QString());
}

QTEST_GUILESS_MAIN(TestPlatform)
#include "TestPlatform.moc"
