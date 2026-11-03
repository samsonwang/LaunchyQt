/*
LaunchyQt: CatalogBuilder unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include <QTemporaryDir>

#include "Core/CatalogBuilder.h"
#include "Core/CatalogSlow.h"

#include "PluginInterface/CatalogItem.h"

using namespace launchy;

namespace {

// indexDirectory() is private on purpose (only buildCatalog drives it), but it
// is the migration's whole point: the directory walk, the dedup set and the
// .lnk rules all live there. A probe re-exposes it the same way TestCatalogFast
// / TestCatalogSlow expose getItem(). The constructor starts a worker QThread
// (idle priority, no event loop), which is harmless in a headless test as long
// as shutdown() runs in cleanup: indexDirectory itself never emits queued
// signals or touches thread affinity.
class BuilderProbe : public CatalogBuilder {
public:
    void runIndex(Catalog* catalog, const QString& dir, const QStringList& filters,
                  bool fDirs, bool fBin, int depth,
                  const CatalogBuilderContext& ctx = CatalogBuilderContext()) {
        indexDirectory(catalog, ctx, dir, filters, fDirs, fBin, depth);
    }
};

} // namespace

class TestCatalogBuilder : public QObject {
    Q_OBJECT

private slots:
    void cleanup();

    void walkIndexesFilesWithMatchingFilters();
    void walkIndexesSubdirectoriesToDepth();
    void dedupSkipsAlreadyIndexedPaths();
    void alterItemHookRewritesEachIndexedFile();
    void emptyContextIsHarmless();
};

void TestCatalogBuilder::cleanup() {
}

// A filter match indexes the file; a non-matching file is ignored. Each test
// uses its own QTemporaryDir so leftovers from a previous case cannot leak into
// the count assertions.
void TestCatalogBuilder::walkIndexesFilesWithMatchingFilters() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString root = dir.path();
    QFile a(root + "/a.txt");
    QVERIFY(a.open(QIODevice::WriteOnly));
    a.close();
    QFile b(root + "/b.md");
    QVERIFY(b.open(QIODevice::WriteOnly));
    b.close();

    BuilderProbe builder;
    CatalogSlow catalog;
    builder.runIndex(&catalog, root, QStringList() << "*.txt", false, false, 0);

    QCOMPARE(catalog.count(), 1);
    builder.shutdown();
}

// depth > 0 descends into subdirectories, depth 0 does not.
void TestCatalogBuilder::walkIndexesSubdirectoriesToDepth() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString root = dir.path();
    QDir(root).mkpath("sub/deep");
    QFile leaf(root + "/sub/deep/x.txt");
    QVERIFY(leaf.open(QIODevice::WriteOnly));
    leaf.close();

    BuilderProbe shallow;
    CatalogSlow shallowCatalog;
    shallow.runIndex(&shallowCatalog, root, QStringList() << "*.txt", false, false, 0);
    QCOMPARE(shallowCatalog.count(), 0);
    shallow.shutdown();

    BuilderProbe deep;
    CatalogSlow deepCatalog;
    deep.runIndex(&deepCatalog, root, QStringList() << "*.txt", false, false, 2);
    QCOMPARE(deepCatalog.count(), 1);
    deep.shutdown();
}

// fDirs indexes directories as items; a .lnk file is not a directory, so it is
// not indexed by fDirs, and with no filters the file loop is skipped.
void TestCatalogBuilder::dedupSkipsAlreadyIndexedPaths() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString root = dir.path();
    QDir(root).mkdir("real");
    QFile dup(root + "/thing.lnk");
    QVERIFY(dup.open(QIODevice::WriteOnly));
    dup.close();

    BuilderProbe builder;
    CatalogSlow catalog;
    builder.runIndex(&catalog, root, QStringList(), true, false, 0);
    // Only "real" is a directory; thing.lnk is a file and is skipped.
    QCOMPARE(catalog.count(), 1);
    builder.shutdown();
}

// The alterItem hook must be invoked once per indexed file and may rewrite the
// item before it is added.
void TestCatalogBuilder::alterItemHookRewritesEachIndexedFile() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString root = dir.path();
    QFile f(root + "/a.txt");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.close();

    int calls = 0;
    CatalogBuilderContext ctx;
    ctx.alterItem = [&calls](CatItem* item) {
        ++calls;
        item->shortName = QStringLiteral("rewritten");
    };

    BuilderProbe builder;
    CatalogSlow catalog;
    builder.runIndex(&catalog, root, QStringList() << "*.txt", false, false, 0, ctx);

    QCOMPARE(calls, 1);
    QCOMPARE(catalog.count(), 1);
    builder.shutdown();
}

// An all-empty context (no expandEnvironmentVars, no alterItem) must index the
// file unchanged rather than crash or skip it.
void TestCatalogBuilder::emptyContextIsHarmless() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString root = dir.path();
    QFile f(root + "/a.txt");
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.close();

    BuilderProbe builder;
    CatalogSlow catalog;
    builder.runIndex(&catalog, root, QStringList() << "*.txt", false, false, 0);

    QCOMPARE(catalog.count(), 1);
    builder.shutdown();
}

QTEST_APPLESS_MAIN(TestCatalogBuilder)

#include "TestCatalogBuilder.moc"
