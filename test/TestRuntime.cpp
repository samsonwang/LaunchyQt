/*
LaunchyQt: runtime free functions and globals unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include <QDir>
#include <QSettings>
#include <QThread>

#include "PluginInterface/Runtime.h"

using namespace launchy;

namespace {

// What the installed handler saw, in call order. File scope rather than a
// parameter because the seam deliberately carries no user data: it is a straight
// hand-off to the platform, not a general callback bus.
struct Probe {
    QStringList seen;           // "file|args"
    QThread* thread = nullptr;  // thread the last call ran on
};

Probe g_probe;

void recordCall(const QString& file, const QString& args) {
    g_probe.seen.append(file + "|" + args);
    g_probe.thread = QThread::currentThread();
}

void ignoreCall(const QString&, const QString&) {
}

// Behaves like a handler that is done after this request. The runtime must stay
// callable from inside a delivery -- i.e. it may not hold its lock while the
// handler runs -- otherwise this deadlocks.
void recordAndUnhook(const QString& file, const QString& args) {
    recordCall(file, args);
    setRunProgramHandler(nullptr);
}

} // namespace

class TestRuntime : public QObject {
    Q_OBJECT

private slots:
    void cleanup();
    void cleanupTestCase();

    void appTempPathExists();
    void appTempPathNativeSeparators();
    void appTempPathNonNativeSeparators();
    void settingsGlobalDefaultNull();
    void settingsGlobalAssignable();
    void needRebuildCatalogDefault();
    void runRequestReachesTheHandler();
    void runRequestWithoutHandlerIsDropped();
    void handlerIsReplacedNotStacked();
    void handlerMayUnhookItself();
};

void TestRuntime::cleanup() {
    // No state leaks between tests: the probe starts empty and nothing is
    // installed.
    setRunProgramHandler(nullptr);
    g_probe.seen.clear();
    g_probe.thread = nullptr;
}

void TestRuntime::cleanupTestCase() {
    // g_settings points into the DLL and survives the test. Leaving a QSettings
    // in it makes the DLL delete it during teardown, after Qt is gone.
    g_settings.clear();
}

void TestRuntime::appTempPathExists() {
    QString path = getAppTempPath();
    // The path must exist on disk after the call (it is created on demand).
    QVERIFY(QDir(path).exists());
    // It is a Launchy-specific directory under the system temp root.
    QVERIFY(path.contains("Launchy"));
}

void TestRuntime::appTempPathNativeSeparators() {
    QString path = getAppTempPath(true);
    QCOMPARE(path, QDir::toNativeSeparators(path));
}

void TestRuntime::appTempPathNonNativeSeparators() {
    QString path = getAppTempPath(false);
    // With toNative=false the path keeps forward slashes on every platform.
    QVERIFY(!path.contains('\\'));
    QVERIFY(path.endsWith("/Launchy"));
}

void TestRuntime::settingsGlobalDefaultNull() {
    // Before any plugin/plugin-mgr assigns it, g_settings is a null pointer.
    // The shared pointer itself is non-null but points to nothing.
    QVERIFY(g_settings.isNull());
}

void TestRuntime::settingsGlobalAssignable() {
    // g_settings is a shared pointer; assigning a fresh QSettings works and
    // is observable. Use an .ini path so the test never touches the real
    // Launchy configuration.
    QSharedPointer<QSettings> settings(
        new QSettings("unittest.ini", QSettings::IniFormat));
    g_settings = settings;
    QVERIFY(!g_settings.isNull());
    QVERIFY(g_settings->fileName().contains("unittest.ini"));
}

void TestRuntime::needRebuildCatalogDefault() {
    // The global starts at 0 (no rebuild requested).
    QCOMPARE(g_needRebuildCatalog.load(), 0);
    g_needRebuildCatalog.store(1);
    QCOMPARE(g_needRebuildCatalog.load(), 1);
    // Leave the global back at its default so other tests observe a clean state.
    g_needRebuildCatalog.store(0);
}

void TestRuntime::runRequestReachesTheHandler() {
    // Each request is forwarded once, in order, with both strings intact -- and
    // it is a plain call: there is no queue to drain and no event loop to run, so
    // the launch has already happened when runProgram() returns.
    setRunProgramHandler(&recordCall);

    runProgram("C:/some/file.exe", "-a -b");
    QCOMPARE(g_probe.seen, QStringList() << "C:/some/file.exe|-a -b");

    runProgram("/usr/bin/app", "--help");
    QCOMPARE(g_probe.seen,
             QStringList() << "C:/some/file.exe|-a -b" << "/usr/bin/app|--help");

    // The handler runs in the caller's thread, which is the GUI thread for every
    // plugin in this tree: that is what lets the platform open a window.
    QCOMPARE(g_probe.thread, QThread::currentThread());
}

void TestRuntime::runRequestWithoutHandlerIsDropped() {
    // Nothing installed (a process that never wired the seam): the request is
    // dropped with a warning instead of crashing or silently piling up.
    runProgram("nobody-home.exe", "");
    QVERIFY(g_probe.seen.isEmpty());
}

void TestRuntime::handlerIsReplacedNotStacked() {
    // One seam, one owner: installing again replaces the previous handler, it
    // does not add a second destination.
    setRunProgramHandler(&recordCall);
    setRunProgramHandler(&ignoreCall);

    runProgram("replaced.exe", "-r");
    QVERIFY(g_probe.seen.isEmpty());
}

void TestRuntime::handlerMayUnhookItself() {
    // The runtime copies the pointer and releases its lock before calling, so a
    // handler is free to reinstall or remove itself. This hangs (CTest timeout)
    // if that ever regresses into calling under the lock.
    setRunProgramHandler(&recordAndUnhook);

    runProgram("last-one.exe", "");
    QCOMPARE(g_probe.seen, QStringList() << "last-one.exe|");

    runProgram("too-late.exe", "");
    QCOMPARE(g_probe.seen, QStringList() << "last-one.exe|");
}

QTEST_APPLESS_MAIN(TestRuntime)
#include "TestRuntime.moc"
