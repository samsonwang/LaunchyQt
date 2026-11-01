/*
PluginPy: the C++ side of a python plugin.

launchy.pyd embeds an interpreter, so the only way to exercise PluginMgr,
PluginLoader and PluginWrapper::msg is to have the interpreter in this process
too. This test links PluginPy and drives a python plugin through the same
PluginInterface::msg() ABI the launcher uses:

    load  ->  MSG_GET_NAME / MSG_PATH / MSG_GET_RESULTS / MSG_LAUNCH_ITEM
          ->  MSG_DO_DIALOG / MSG_END_DIALOG  ->  unload

The plugin is written by the test into a temporary directory, so nothing here
depends on the plugins the app ships. What is checked is the marshalling in
both directions (InputData and CatItem), and the handled/not-handled return
values msg() promises: the setters and the dialog pair are fire-and-forget, so
the caller keeps dispatching them to the other plugins.

The test needs the python runtime next to it (python36.dll, python/launchy.pyd,
python/launchy_util.py) -- the POST_BUILD step in test/CMakeLists.txt copies
them, and PYTHONHOME is passed in as a definition so the interpreter finds its
standard library without relying on a registry entry.
*/

#include <QtTest>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QSharedPointer>
#include <QString>
#include <QTemporaryDir>

#include "PluginLoader.h"

#include "PluginInterface/CatalogItem.h"
#include "PluginInterface/InputData.h"
#include "PluginInterface/Runtime.h"
#include "PluginInterface/PluginInterface.h"
#include "PluginInterface/PluginMsg.h"

using namespace launchy;

namespace {

// One file per observable side effect: the C++ side cannot read a python
// object's attributes, so the fixture leaves its state on disk instead.
QString pathFile(const QString& pluginDir) {
    return pluginDir + "/path.txt";
}

QString queryFile(const QString& pluginDir) {
    return pluginDir + "/query.txt";
}

QString launchFile(const QString& pluginDir) {
    return pluginDir + "/launch.txt";
}

QString endDialogFile(const QString& pluginDir) {
    return pluginDir + "/end.txt";
}

QString readFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}

void writeFile(const QString& path, const QByteArray& content) {
    QFile file(path);
    QVERIFY2(file.open(QIODevice::WriteOnly), qPrintable(path));
    file.write(content);
    file.close();
}

// A plugin that implements every virtual, records what it was given, and hands
// back two catalog entries.
const char* FIXTURE_PLUGIN = R"PY(
import os
import launchy

HERE = os.path.dirname(__file__)


def _record(name, text):
    with open(os.path.join(HERE, name), "w") as handle:
        handle.write(text)


class PyFixture(launchy.Plugin):
    def __init__(self):
        launchy.Plugin.__init__(self)

    def init(self):
        pass

    def getName(self):
        return "PyFixture"

    def setPath(self, path):
        _record("path.txt", path)

    def getLabels(self, inputDataList):
        pass

    def getResults(self, inputDataList, resultsList):
        # The query has to arrive intact: this is the InputData conversion
        _record("query.txt", inputDataList[0].getText())
        resultsList.push_back(
            launchy.CatItem("C:/fixture/result-1", "result-1", "PyFixture", ""))
        resultsList.append(
            launchy.CatItem("C:/fixture/result-2", "result-2", "PyFixture", ""))

    def getCatalog(self, resultsList):
        pass

    def launchItem(self, inputDataList, item):
        _record("launch.txt", item.fullPath())

    def doDialog(self, parent):
        return 0x1234

    def endDialog(self, accept):
        _record("end.txt", "accept" if accept else "cancel")

    def launchyShow(self):
        pass

    def launchyHide(self):
        pass


def getPlugin():
    return PyFixture
)PY";

// A plugin that throws where a well behaved one fills the result list.
const char* RAISING_PLUGIN = R"PY(
import launchy


class PyRaising(launchy.Plugin):
    def __init__(self):
        launchy.Plugin.__init__(self)

    def init(self):
        pass

    def getName(self):
        return "PyRaising"

    def setPath(self, path):
        pass

    def getLabels(self, inputDataList):
        pass

    def getResults(self, inputDataList, resultsList):
        raise RuntimeError("fixture failure")

    def getCatalog(self, resultsList):
        pass

    def launchItem(self, inputDataList, item):
        pass


def getPlugin():
    return PyRaising
)PY";

} // namespace

class TestPluginPyLoader : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    // --- loading ---
    void fixturePluginLoads();
    void nameTravelsThroughTheOutParameter();
    void pathIsForwardedWithoutClaimingTheMessage();

    // --- marshalling ---
    void queryReachesThePlugin();
    void resultsTravelBackAsCatItems();
    void launchItemCountsAsAUsage();

    // --- dialog sequence ---
    void dialogHandleTravelsBothWays();
    void endDialogIsANotification();

    // --- robustness ---
    void unknownMessageIsNotHandled();
    void pythonExceptionIsContained();
    void unloadingDropsTheWrapper();

private:
    QString pluginDir() const {
        return m_dir->path() + "/plugins/PyFixture";
    }

private:
    QScopedPointer<QTemporaryDir> m_dir;
    pluginpy::PluginLoader* m_loader = nullptr;
    PluginInterface* m_plugin = nullptr;
};

void TestPluginPyLoader::initTestCase() {
    m_dir.reset(new QTemporaryDir);
    QVERIFY(m_dir->isValid());

    // The interpreter is created by PluginMgr on first use, which happens
    // below. Point it at the installation its python36.dll came from instead of
    // relying on a registry entry the interpreter would otherwise need.
#ifdef LAUNCHY_TEST_PYTHON_HOME
    qputenv("PYTHONHOME", QByteArray(LAUNCHY_TEST_PYTHON_HOME));
#endif

    // PluginMgr's constructor reads the external package path from the global
    // settings store, so it has to exist before the first plugin is loaded.
    g_settings = QSharedPointer<QSettings>(
        new QSettings(m_dir->path() + "/pluginpy_test.ini", QSettings::IniFormat));
    QVERIFY(!g_settings.isNull());

    QDir().mkpath(pluginDir());
    writeFile(pluginDir() + "/PyFixture.py", QByteArray(FIXTURE_PLUGIN));

    const QString raisingDir = m_dir->path() + "/plugins/PyRaising";
    QDir().mkpath(raisingDir);
    writeFile(raisingDir + "/PyRaising.py", QByteArray(RAISING_PLUGIN));
}

void TestPluginPyLoader::cleanupTestCase() {
    delete m_loader;
    m_loader = nullptr;
    m_plugin = nullptr;

    if (!g_settings.isNull()) {
        g_settings->sync();
    }
    g_settings.clear();
}

void TestPluginPyLoader::fixturePluginLoads() {
    m_loader = new pluginpy::PluginLoader("PyFixture", pluginDir());
    m_plugin = m_loader->instance();
    QVERIFY(m_plugin != nullptr);
}

void TestPluginPyLoader::nameTravelsThroughTheOutParameter() {
    QVERIFY(m_plugin != nullptr);
    QString name;
    QCOMPARE(m_plugin->msg(MSG_GET_NAME, &name), 1);
    QCOMPARE(name, QString("PyFixture"));
}

// MSG_PATH is a setter: it must reach the plugin and still report "not
// handled", or the launcher would stop offering the path to the other plugins.
void TestPluginPyLoader::pathIsForwardedWithoutClaimingTheMessage() {
    QVERIFY(m_plugin != nullptr);
    QString path = "C:/plugins/PyFixture";
    QCOMPARE(m_plugin->msg(MSG_PATH, &path), 0);
    QCOMPARE(readFile(pathFile(pluginDir())), path);
}

void TestPluginPyLoader::queryReachesThePlugin() {
    QVERIFY(m_plugin != nullptr);
    QList<InputData> inputData;
    inputData.append(InputData("hello from the query"));
    QList<CatItem> results;

    QCOMPARE(m_plugin->msg(MSG_GET_RESULTS, &inputData, &results), 1);
    QCOMPARE(readFile(queryFile(pluginDir())), QString("hello from the query"));
}

// Both bindings of CatItemList (push_back and append) have to end up in the
// same QList the launcher reads afterwards.
void TestPluginPyLoader::resultsTravelBackAsCatItems() {
    QVERIFY(m_plugin != nullptr);
    QList<InputData> inputData;
    inputData.append(InputData("query"));

    QList<CatItem> results;
    CatItem existing;
    existing.fullPath = "C:/fixture/already-there";
    existing.shortName = "already-there";
    results.append(existing);

    QCOMPARE(m_plugin->msg(MSG_GET_RESULTS, &inputData, &results), 1);

    // The plugin adds to the list it was given, it does not replace it
    QCOMPARE(results.count(), 3);
    QCOMPARE(results.at(0).fullPath, QString("C:/fixture/already-there"));
    QCOMPARE(results.at(1).fullPath, QString("C:/fixture/result-1"));
    QCOMPARE(results.at(2).fullPath, QString("C:/fixture/result-2"));
    QCOMPARE(results.at(1).pluginName, QString("PyFixture"));
}

// launchItem() counts the launch before it reaches the plugin: the usage count
// is what the ranking learns from.
void TestPluginPyLoader::launchItemCountsAsAUsage() {
    QVERIFY(m_plugin != nullptr);
    QList<InputData> inputData;
    inputData.append(InputData("query"));

    CatItem item;
    item.fullPath = "C:/fixture/result-1";
    item.shortName = "result-1";
    item.usage = 7;

    QCOMPARE(m_plugin->msg(MSG_LAUNCH_ITEM, &inputData, &item), 1);
    QCOMPARE(item.usage, 8);
    QCOMPARE(readFile(launchFile(pluginDir())), QString("C:/fixture/result-1"));
}

// The handle is opaque in both directions: the widget the launcher passes in
// reaches the plugin, and what the plugin returns is what the caller sees. A
// null result means "this plugin has no page".
void TestPluginPyLoader::dialogHandleTravelsBothWays() {
    QVERIFY(m_plugin != nullptr);
    void* dialog = nullptr;
    QCOMPARE(m_plugin->msg(MSG_DO_DIALOG, nullptr, &dialog), 0);
    QCOMPARE(reinterpret_cast<quintptr>(dialog), quintptr(0x1234));
}

void TestPluginPyLoader::endDialogIsANotification() {
    QVERIFY(m_plugin != nullptr);
    QCOMPARE(m_plugin->msg(MSG_END_DIALOG, reinterpret_cast<void*>(true)), 0);
    QCOMPARE(readFile(endDialogFile(pluginDir())), QString("accept"));
}

void TestPluginPyLoader::unknownMessageIsNotHandled() {
    QVERIFY(m_plugin != nullptr);
    QCOMPARE(m_plugin->msg(9999), 0);
}

// A plugin that throws must not take the launcher down: msg() catches, prints
// the traceback and reports "not handled".
void TestPluginPyLoader::pythonExceptionIsContained() {
    pluginpy::PluginLoader raising("PyRaising", m_dir->path() + "/plugins/PyRaising");
    QVERIFY(raising.instance() != nullptr);

    QList<InputData> inputData;
    inputData.append(InputData("query"));
    QList<CatItem> results;

    QCOMPARE(raising.instance()->msg(MSG_GET_RESULTS, &inputData, &results), 0);
    QVERIFY(results.isEmpty());

    // The plugin is still usable afterwards: the interpreter state was cleared
    QString name;
    QCOMPARE(raising.instance()->msg(MSG_GET_NAME, &name), 1);
    QCOMPARE(name, QString("PyRaising"));
}

void TestPluginPyLoader::unloadingDropsTheWrapper() {
    QVERIFY(m_loader != nullptr);
    QVERIFY(m_loader->unload());
    QVERIFY(m_loader->instance() == nullptr);
}

QTEST_GUILESS_MAIN(TestPluginPyLoader)

#include "TestPluginPyLoader.moc"
