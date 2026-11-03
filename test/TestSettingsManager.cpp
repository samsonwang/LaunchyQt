/*
LaunchyQt: settings manager unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QTemporaryDir>
#include <QSharedPointer>

#include "Core/Directory.h"
#include "Core/OptionItem.h"
#include "Core/SettingsManager.h"

#include "PluginInterface/Runtime.h"

using namespace launchy;

namespace {

// The directory roots a platform would report, resolved under one
// temporary directory. This is the whole seam: no QApplication, no platform
// code, the manager just needs to be told where the files live.
QMap<QString, QList<QString>> makeRoots(const QString& base,
                                        const QString& portableRoot) {
    QMap<QString, QList<QString>> roots;
    roots["config"] << base + "/config";
    roots["portableConfig"] << portableRoot;
    roots["skins"] << base + "/skins";
    roots["plugins"] << base + "/plugins";
    return roots;
}

QList<Directory> makeDefaults(const QStringList& names) {
    QList<Directory> dirs;
    for (const QString& name : names) {
        dirs.append(Directory(name));
    }
    return dirs;
}

QStringList namesOf(const QList<Directory>& dirs) {
    QStringList names;
    for (const Directory& dir : dirs) {
        names.append(dir.name);
    }
    return names;
}

// QSettings may hand the path back in native separators
QString normalized(const QString& path) {
    return QDir::fromNativeSeparators(path);
}

} // namespace

class TestSettingsManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();

    void installedModeWithoutPortableConfig();
    void portableModeWhenIniExistsButNoMarker();
    void installedMarkerWinsOverIni();
    void profileIsAppendedToEveryPath();
    void defaultsAreWrittenOnFirstLoad();
    void mergePutsDefaultsFirstAndDropsDuplicates();
    void writtenCatalogDirectoriesReadBack();
    void setPortableMovesTheFiles();
    void setPortableFailureKeepsModeAndReopensSettings();

private:
    // One temporary root for the whole run, one subdirectory per case. The
    // cases share the SettingsManager singleton and the global QSettings, so
    // keeping every directory alive until the end avoids a settings object
    // syncing into a directory that a per-case temporary had already removed.
    QString caseDir(const QString& name);

private:
    QTemporaryDir m_dir;
};

QString TestSettingsManager::caseDir(const QString& name) {
    const QString path = m_dir.path() + "/" + name;
    QDir().mkpath(path);
    // QSettings writes the file but does not create its directory
    QDir().mkpath(path + "/config");
    return path;
}

void TestSettingsManager::initTestCase() {
    QVERIFY(m_dir.isValid());
}

void TestSettingsManager::init() {
    // The singleton carries the profile between cases otherwise
    SettingsManager::instance().setProfileName(QString());
}

void TestSettingsManager::cleanupTestCase() {
    // Assigning g_settings in a test means it must be cleared here, or the
    // process exits 0xC0000409 in Launchy.dll's static teardown after passing.
    g_settings.clear();
}

void TestSettingsManager::installedModeWithoutPortableConfig() {
    const QString base = caseDir("installed");

    // The portable root does not exist at all: installed mode
    SettingsManager& settings = SettingsManager::instance();
    settings.load(makeRoots(base, base + "/portable"), makeDefaults({"A"}));

    QVERIFY(!settings.isPortable());
    QCOMPARE(settings.configDirectory(), base + "/config");
    QCOMPARE(settings.catalogFilename(), base + "/config/catalog.json");
    QCOMPARE(settings.historyFilename(), base + "/config/history.json");
}

void TestSettingsManager::portableModeWhenIniExistsButNoMarker() {
    const QString base = caseDir("portable");
    QDir().mkpath(base + "/portable");
    QFile ini(base + "/portable/launchy.ini");
    QVERIFY(ini.open(QIODevice::WriteOnly));
    ini.close();

    SettingsManager& settings = SettingsManager::instance();
    settings.load(makeRoots(base, base + "/portable"), makeDefaults({"A"}));

    QVERIFY(settings.isPortable());
    QCOMPARE(settings.configDirectory(), base + "/portable");
}

void TestSettingsManager::installedMarkerWinsOverIni() {
    const QString base = caseDir("marker");
    QDir().mkpath(base + "/portable");

    // Both files present = an installed copy that happens to sit next to an ini
    QFile ini(base + "/portable/launchy.ini");
    QVERIFY(ini.open(QIODevice::WriteOnly));
    ini.close();
    QFile marker(base + "/portable/.installed");
    QVERIFY(marker.open(QIODevice::WriteOnly));
    marker.close();

    SettingsManager& settings = SettingsManager::instance();
    settings.load(makeRoots(base, base + "/portable"), makeDefaults({"A"}));

    QVERIFY(!settings.isPortable());
    QCOMPARE(settings.configDirectory(), base + "/config");
}

void TestSettingsManager::profileIsAppendedToEveryPath() {
    const QString base = caseDir("profile");

    SettingsManager& settings = SettingsManager::instance();
    settings.load(makeRoots(base, base + "/portable"), makeDefaults({"A"}));

    settings.setProfileName("work");

    QCOMPARE(settings.configDirectory(), base + "/config/profiles/work");
    QCOMPARE(settings.catalogFilename(), base + "/config/profiles/work/catalog.json");
    QCOMPARE(settings.historyFilename(), base + "/config/profiles/work/history.json");
}

void TestSettingsManager::defaultsAreWrittenOnFirstLoad() {
    const QString base = caseDir("defaults");

    SettingsManager& settings = SettingsManager::instance();
    settings.load(makeRoots(base, base + "/portable"), makeDefaults({"A", "B"}));

    // The write goes through the shared instance, the read through a private
    // one bound to the same file, exactly like the catalog worker does it.
    g_settings->sync();

    QCOMPARE(namesOf(settings.readCatalogDirectories()), QStringList({"A", "B"}));
}

void TestSettingsManager::mergePutsDefaultsFirstAndDropsDuplicates() {
    const QString base = caseDir("merge");

    SettingsManager& settings = SettingsManager::instance();
    settings.load(makeRoots(base, base + "/portable"), makeDefaults({"A", "B"}));

    // The stored list is out of order, misses B and repeats A
    QList<Directory> stored = makeDefaults({"C", "A"});
    QVERIFY(settings.mergeDefaultCatalogDirectories(stored));

    QCOMPARE(namesOf(stored), QStringList({"A", "B", "C"}));

    // Running it again on the merged list changes nothing
    QVERIFY(!settings.mergeDefaultCatalogDirectories(stored));
}

void TestSettingsManager::writtenCatalogDirectoriesReadBack() {
    const QString base = caseDir("roundtrip");

    SettingsManager& settings = SettingsManager::instance();
    settings.load(makeRoots(base, base + "/portable"), makeDefaults({"A"}));

    Directory custom("D:/custom");
    custom.types << "*.lnk" << "*.exe";
    custom.indexDirs = true;
    custom.indexExe = false;
    custom.depth = 7;

    QList<Directory> toWrite;
    toWrite.append(custom);
    settings.writeCatalogDirectories(toWrite);
    g_settings->sync();

    const QList<Directory> read = settings.readCatalogDirectories();
    QCOMPARE(read.count(), 1);
    QCOMPARE(read[0].name, QString("D:/custom"));
    QCOMPARE(read[0].types, QStringList({"*.lnk", "*.exe"}));
    QVERIFY(read[0].indexDirs);
    QVERIFY(!read[0].indexExe);
    QCOMPARE(read[0].depth, 7);
}

void TestSettingsManager::setPortableMovesTheFiles() {
    const QString base = caseDir("convert");

    SettingsManager& settings = SettingsManager::instance();
    settings.load(makeRoots(base, base + "/portable"), makeDefaults({"A"}));

    // load() created the ini through QSettings, make sure it is on disk before
    // it is copied
    g_settings->sync();
    QVERIFY(QFile::exists(base + "/config/launchy.ini"));

    QVERIFY(settings.setPortable(true));

    QVERIFY(settings.isPortable());
    QCOMPARE(settings.configDirectory(), base + "/portable");
    QVERIFY(QFile::exists(base + "/portable/launchy.ini"));
    // The settings object has to follow the files it now owns
    QCOMPARE(normalized(g_settings->fileName()), base + "/portable/launchy.ini");
}

void TestSettingsManager::setPortableFailureKeepsModeAndReopensSettings() {
    const QString base = caseDir("blocked");

    // A file where a directory would have to be: the destination can never be
    // created, so the copy fails. The path itself does not exist, which is what
    // keeps the initial mode installed.
    QFile blocker(base + "/blocker");
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.close();

    SettingsManager& settings = SettingsManager::instance();
    settings.load(makeRoots(base, base + "/blocker/sub"), makeDefaults({"A"}));
    QVERIFY(!settings.isPortable());

    QVERIFY(!settings.setPortable(true));

    // The failure must not flip the mode: the files never left the old
    // directory, so reporting portable would make the next run read an empty
    // profile.
    QVERIFY(!settings.isPortable());
    QCOMPARE(settings.configDirectory(), base + "/config");

    // setPortable() destroys the shared QSettings before it copies. If it is
    // not reopened, the next caller dereferences a null pointer.
    QVERIFY(!g_settings.isNull());
    QCOMPARE(normalized(g_settings->fileName()), base + "/config/launchy.ini");
}

QTEST_APPLESS_MAIN(TestSettingsManager)

#include "TestSettingsManager.moc"
