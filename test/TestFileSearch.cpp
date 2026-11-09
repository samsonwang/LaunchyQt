/*
LaunchyQt: file system search unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QSharedPointer>
#include <QStringList>
#include <QTemporaryDir>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "PluginInterface/PluginLabels.h"
#include "PluginInterface/Runtime.h"

#include "Core/InputDataList.h"
#include "Core/OptionItem.h"

#include "FileSearch.h"

using namespace launchy;

namespace {

QStringList shortNames(const QList<CatItem>& items) {
    QStringList names;
    for (const CatItem& item : items) {
        names.append(item.shortName);
    }
    return names;
}

// FileSearch::search() ends by labelling the input segment, so it needs a list
// with at least one segment -- exactly what the widget hands it.
InputDataList inputFor(const QString& text) {
    InputDataList inputData;
    inputData.parse(text);
    return inputData;
}

QString join(const QString& dir, const QString& name) {
    return dir + "/" + name;
}

void writeFile(const QString& path, const QString& content = QString("x")) {
    QFile file(path);
    QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(path));
    file.write(content.toUtf8());
    file.close();
}

} // namespace

class TestFileSearch : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // --- looksLikePath: decides whether the file system is probed at all ---
    void pathSeparatorLooksLikeAPath();
    void nativeSeparatorLooksLikeAPath();
    void bareDriveLooksLikeAPath();
    void homeReferenceLooksLikeAPath();
    void urlIsNotAPath();
    void bareWordIsNotAPath();
    void emptyTextIsNotAPath();

    // --- search(): directory listing ---
    void trailingSeparatorListsTheDirectory();
    void listingExcludesHiddenFilesByDefault();
    void listingIncludesHiddenFilesWhenEnabled();
    void wordFilterKeepsOnlyMatchingEntries();
    void multiWordQueryRequiresEveryWord();
    void missingDirectoryYieldsNothing();
    void resultsAreRankedByTheFileNamePart();
    void fileResultsAreStampedWithTheFilePlugin();

private:
    QString pathInTree(const QString& name) const {
        return join(m_tree.path(), name);
    }

private:
    QTemporaryDir m_settingsDir;
    QTemporaryDir m_tree;
};

void TestFileSearch::initTestCase() {
    QVERIFY(m_settingsDir.isValid());
    QVERIFY(m_tree.isValid());

    // search() reads the hidden-file switch through the global settings store
    g_settings = QSharedPointer<QSettings>(
        new QSettings(m_settingsDir.path() + "/file_search_test.ini",
                      QSettings::IniFormat));
    QVERIFY(!g_settings.isNull());
    g_settings->setValue(OPTION_SHOWHIDDENFILES, false);

    QDir().mkpath(pathInTree("sub"));
    writeFile(pathInTree("Alpha Report.txt"));
    writeFile(pathInTree("beta.txt"));
    writeFile(pathInTree("gamma.log"));
    writeFile(pathInTree("sub/nested.txt"));

    // A dotfile is hidden on Unix but not on Windows, where "hidden" means the
    // file attribute. Set it so the case means the same thing on both.
    writeFile(pathInTree(".hidden.txt"));
#ifdef Q_OS_WIN
    QVERIFY(SetFileAttributesW(
        reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(pathInTree(".hidden.txt")).utf16()),
        FILE_ATTRIBUTE_HIDDEN));
#endif
}

void TestFileSearch::cleanupTestCase() {
    // Releasing the shared QSettings while the exe is alive: a test that leaves
    // it set dies in Launchy.dll's static teardown after passing (0xC0000409).
    if (!g_settings.isNull()) {
        g_settings->sync();
    }
    g_settings.clear();
}

void TestFileSearch::pathSeparatorLooksLikeAPath() {
    QVERIFY(FileSearch::looksLikePath("C:/Windows"));
    QVERIFY(FileSearch::looksLikePath("folder/sub"));
    QVERIFY(FileSearch::looksLikePath("docs/"));
}

// The user can type the separator of the platform, and the path split in
// search() normalises it first -- the test has to accept the same thing.
void TestFileSearch::nativeSeparatorLooksLikeAPath() {
    QVERIFY(FileSearch::looksLikePath(QDir::toNativeSeparators("C:/Windows")));
    QVERIFY(FileSearch::looksLikePath(QString("\\\\server\\share")));
}

void TestFileSearch::bareDriveLooksLikeAPath() {
    // "C:" alone lists the root of the drive
    QVERIFY(FileSearch::looksLikePath("C:"));
    QVERIFY(FileSearch::looksLikePath("c:"));
    // ...but "C" or "C:something" is an ordinary query, not a drive
    QVERIFY(!FileSearch::looksLikePath("C"));
    QVERIFY(!FileSearch::looksLikePath("C:report"));
}

void TestFileSearch::homeReferenceLooksLikeAPath() {
    QVERIFY(FileSearch::looksLikePath("~"));
    QVERIFY(FileSearch::looksLikePath("~/documents"));
}

// A URL has slashes but belongs to the catalog and the plugins. Probing the
// file system for it would also restart the query with a non-lowercased text.
void TestFileSearch::urlIsNotAPath() {
    QVERIFY(!FileSearch::looksLikePath("http://example.com"));
    QVERIFY(!FileSearch::looksLikePath("https://example.com/a/b"));
    QVERIFY(!FileSearch::looksLikePath("file:///C:/temp"));
}

void TestFileSearch::bareWordIsNotAPath() {
    QVERIFY(!FileSearch::looksLikePath("notepad"));
    QVERIFY(!FileSearch::looksLikePath("calc.exe"));
    QVERIFY(!FileSearch::looksLikePath("goo gle"));
}

void TestFileSearch::emptyTextIsNotAPath() {
    QVERIFY(!FileSearch::looksLikePath(QString()));
}

// A trailing separator means "show me what is in here": every entry is listed
// and the directory itself is the top result, so Enter opens it.
void TestFileSearch::trailingSeparatorListsTheDirectory() {
    InputDataList inputData = inputFor(m_tree.path() + "/");
    QList<CatItem> results;

    FileSearch::search(m_tree.path() + "/", results, inputData);

    // 4 visible entries (sub, Alpha Report.txt, beta.txt, gamma.log) plus the
    // directory row in front
    QCOMPARE(results.count(), 5);
    QCOMPARE(QDir::cleanPath(results.first().fullPath),
             QDir::cleanPath(m_tree.path()));

    const QStringList names = shortNames(results);
    QVERIFY(names.contains("sub"));
    QVERIFY(names.contains("Alpha Report.txt"));
    QVERIFY(names.contains("beta.txt"));
    QVERIFY(names.contains("gamma.log"));
}

void TestFileSearch::listingExcludesHiddenFilesByDefault() {
    g_settings->setValue(OPTION_SHOWHIDDENFILES, false);

    InputDataList inputData = inputFor(m_tree.path() + "/");
    QList<CatItem> results;
    FileSearch::search(m_tree.path() + "/", results, inputData);

    QVERIFY(!shortNames(results).contains(".hidden.txt"));
}

void TestFileSearch::listingIncludesHiddenFilesWhenEnabled() {
    g_settings->setValue(OPTION_SHOWHIDDENFILES, true);

    InputDataList inputData = inputFor(m_tree.path() + "/");
    QList<CatItem> results;
    FileSearch::search(m_tree.path() + "/", results, inputData);

    QVERIFY(shortNames(results).contains(".hidden.txt"));

    g_settings->setValue(OPTION_SHOWHIDDENFILES, false);
}

void TestFileSearch::wordFilterKeepsOnlyMatchingEntries() {
    InputDataList inputData = inputFor(m_tree.path() + "/beta");
    QList<CatItem> results;

    FileSearch::search(m_tree.path() + "/beta", results, inputData);

    QCOMPARE(shortNames(results), QStringList({"beta.txt"}));
}

// The part after the last separator is split on spaces and every word has to
// match: "alpha txt" keeps Alpha Report.txt and drops beta.txt, which only
// carries "txt".
void TestFileSearch::multiWordQueryRequiresEveryWord() {
    InputDataList inputData = inputFor(m_tree.path() + "/alpha txt");
    QList<CatItem> results;

    FileSearch::search(m_tree.path() + "/alpha txt", results, inputData);

    QCOMPARE(shortNames(results), QStringList({"Alpha Report.txt"}));
}

// A path that does not exist must leave the result list (and the segment's
// labels) alone instead of inventing entries.
void TestFileSearch::missingDirectoryYieldsNothing() {
    const QString missing = join(m_tree.path(), "no-such-directory/deeper");
    InputDataList inputData = inputFor(missing);
    QList<CatItem> results;

    FileSearch::search(missing, results, inputData);

    QVERIFY(results.isEmpty());
    QVERIFY(!inputData.last().hasLabel(LABEL_FILE));
}

// The ranking is scoped to the file name part: the shorter name that starts
// with the query comes first. Kept in its own directory: "report" is also a
// subsequence of "Alpha Report.txt" in the parent, which would be listed too.
void TestFileSearch::resultsAreRankedByTheFileNamePart() {
    const QString dir = pathInTree("rank");
    QVERIFY(QDir().mkpath(dir));
    writeFile(join(dir, "report.txt"));
    writeFile(join(dir, "report-a.txt"));
    writeFile(join(dir, "report-of-something.txt"));

    InputDataList inputData = inputFor(dir + "/report");
    QList<CatItem> results;

    FileSearch::search(dir + "/report", results, inputData);

    QCOMPARE(results.count(), 3);
    QCOMPARE(results.first().shortName, QString("report.txt"));
}

void TestFileSearch::fileResultsAreStampedWithTheFilePlugin() {
    InputDataList inputData = inputFor(m_tree.path() + "/gamma");
    QList<CatItem> results;

    FileSearch::search(m_tree.path() + "/gamma", results, inputData);

    QCOMPARE(results.count(), 1);
    QCOMPARE(results.first().pluginName, NAME_LAUNCHY);
    // The segment is tagged so the UI knows the list came from the file system
    QVERIFY(inputData.last().hasLabel(LABEL_FILE));
}

QTEST_APPLESS_MAIN(TestFileSearch)

#include "TestFileSearch.moc"
