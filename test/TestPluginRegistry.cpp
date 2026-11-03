/*
LaunchyQt: plugin registry unit tests

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
*/

#include <QtTest>

#include <QDir>
#include <QMap>
#include <QSettings>
#include <QSharedPointer>
#include <QTemporaryDir>

#include "Core/CatalogSlow.h"
#include "Core/PluginRegistry.h"
#include "Core/SettingsManager.h"

#include "PluginInterface/Runtime.h"
#include "PluginInterface/PluginInterface.h"
#include "PluginInterface/PluginMsg.h"

using namespace launchy;

namespace {

// A plugin that does nothing but write down what it was sent. The registry has
// to route messages without ever assuming what a plugin does with them, so this
// is all the fidelity the policy needs.
class FakePlugin : public PluginInterface {
public:
    explicit FakePlugin(const QString& name)
        : m_name(name),
          m_dialogHandle(nullptr) {
    }

    int msg(int msgId, void* wParam, void* lParam) override {
        sent << msgId;

        switch (msgId) {
        case MSG_GET_NAME:
            *static_cast<QString*>(wParam) = m_name;
            return 1;

        case MSG_PATH:
            path = *static_cast<const QString*>(wParam);
            return 0;

        case MSG_GET_RESULTS:
            ++resultQueries;
            if (lParam) {
                *static_cast<QList<CatItem>*>(lParam)
                    << CatItem(m_name + "/result", m_name + " result");
            }
            return 1;

        case MSG_GET_LABELS:
            ++labelQueries;
            return 1;

        case MSG_GET_CATALOG:
            if (wParam) {
                *static_cast<QList<CatItem>*>(wParam)
                    << CatItem(m_name + "/catalog", m_name + " catalog");
            }
            return 1;

        case MSG_LAUNCH_ITEM:
            launchedItem = static_cast<CatItem*>(lParam)->shortName;
            return kLaunchHandled;

        case MSG_DO_DIALOG:
            if (lParam) {
                *static_cast<void**>(lParam) = m_dialogHandle;
            }
            return 0;

        case MSG_END_DIALOG:
            dialogClosedWith = (wParam != nullptr);
            return 0;

        default:
            return 0;
        }
    }

public:
    // Code the registry must hand back untouched from MSG_LAUNCH_ITEM
    static const int kLaunchHandled = 42;

    QString m_name;
    QList<int> sent;
    QString path;
    int resultQueries = 0;
    int labelQueries = 0;
    QString launchedItem;
    bool dialogClosedWith = false;
    void* m_dialogHandle = nullptr;
};

// The factory half of the test: says which directories hold a plugin and what
// the plugin calls itself. Everything the real factory does with a DLL or a
// python module is irrelevant to the policy under test.
class FakeFactory : public IPluginFactory {
public:
    PluginInfo instantiate(const QString& name, const QString& path) override {
        instantiated << name;

        const QString reported = names.value(name);
        if (reported.isEmpty()) {
            // "this directory is not a plugin" -- the registry has to skip it
            return PluginInfo();
        }

        FakePlugin* plugin = new FakePlugin(reported);
        plugin->m_dialogHandle = dialogs.value(name);
        owned << plugin;

        PluginInfo info;
        info.path = path;
        info.obj = plugin;
        info.sendMsg(MSG_GET_NAME, &info.name);
        return info;
    }

    // Directory name -> the name the plugin reports; missing means "not a
    // plugin at all"
    QMap<QString, QString> names;
    QMap<QString, void*> dialogs;

    QStringList instantiated;
    QList<FakePlugin*> owned;
};

// Counts the progress reports the registry makes while collecting catalogs
class ProgressRecorder : public INotifyProgressStep {
public:
    bool progressStep(int newStep) override {
        steps << newStep;
        return true;
    }

    QList<int> steps;
};

} // namespace

class TestPluginRegistry : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void loadsEnabledPlugins();
    void skipsDirectoriesThatHoldNoPlugin();
    void disabledPluginStaysInTheTable();
    void enablingAPluginThatWasNeverBuiltLoadsIt();
    void dispatchSkipsDisabledPlugins();
    void dispatchFollowsTheTableOrder();
    void getLabelsIgnoresEmptyInput();
    void launchItemRoutesToTheOwningPlugin();
    void launchItemOnDisabledPluginFallsBack();
    void getCatalogsCollectsItemsAndReportsProgress();
    void dialogHandleTravelsAsAnOpaqueValue();
    void reloadReplacesTheTable();
    void reloadReusesTheInstance();
    void disableThenEnableKeepsTheInstance();

private:
    // A clean profile per case: the plugin array lives in the ini, so sharing
    // one file would let one case disable a plugin for another.
    QString loadProfile(const QString& name);
    QString addPluginDirectory(const QString& base, const QString& name);
    void disable(const QStringList& names);
    void enable(const QStringList& names);
    void setLoadState(const QMap<QString, bool>& states);

private:
    QTemporaryDir m_dir;
};

void TestPluginRegistry::initTestCase() {
    QVERIFY(m_dir.isValid());
}

void TestPluginRegistry::cleanupTestCase() {
    // Assigning g_settings in a test means it must be cleared here, or the
    // process exits 0xC0000409 in Launchy.dll's static teardown after passing.
    g_settings.clear();
}

QString TestPluginRegistry::loadProfile(const QString& name) {
    const QString base = m_dir.path() + "/" + name;
    QDir().mkpath(base + "/config");
    QDir().mkpath(base + "/plugins");

    QMap<QString, QList<QString>> roots;
    roots["config"] << base + "/config";
    roots["plugins"] << base + "/plugins";
    SettingsManager::instance().load(roots, QList<Directory>());

    return base;
}

QString TestPluginRegistry::addPluginDirectory(const QString& base, const QString& name) {
    const QString path = base + "/plugins/" + name;
    QDir().mkpath(path);
    return path;
}

namespace {

// The array the option dialog maintains, written straight into the profile
void writeLoadArray(const QMap<QString, bool>& states) {
    g_settings->beginWriteArray("Plugin", states.count());
    int index = 0;
    for (auto it = states.constBegin(); it != states.constEnd(); ++it, ++index) {
        g_settings->setArrayIndex(index);
        g_settings->setValue("name", it.key());
        g_settings->setValue("load", it.value());
    }
    g_settings->endArray();
    g_settings->sync();
}

void writeLoadArray(const QStringList& names, bool load) {
    QMap<QString, bool> states;
    for (const QString& name : names) {
        states[name] = load;
    }
    writeLoadArray(states);
}

// How many result queries the plugin called 'name' has seen. A plugin that was
// never built is not in 'owned' at all, and that counts as zero too.
int resultQueries(const QString& name, const FakeFactory& factory) {
    for (const FakePlugin* plugin : factory.owned) {
        if (plugin->m_name == name) {
            return plugin->resultQueries;
        }
    }
    return 0;
}

} // namespace

void TestPluginRegistry::disable(const QStringList& names) {
    writeLoadArray(names, false);
}

void TestPluginRegistry::enable(const QStringList& names) {
    writeLoadArray(names, true);
}

void TestPluginRegistry::setLoadState(const QMap<QString, bool>& states) {
    writeLoadArray(states);
}

void TestPluginRegistry::loadsEnabledPlugins() {
    const QString base = loadProfile("enabled");
    addPluginDirectory(base, "Alpha");
    addPluginDirectory(base, "Beta");

    FakeFactory factory;
    factory.names["Alpha"] = "Alpha";
    factory.names["Beta"] = "Beta";

    PluginRegistry::instance().loadPlugins(factory);

    const QMap<QString, PluginInfo> plugins = PluginRegistry::instance().getPlugins();
    QCOMPARE(plugins.count(), 2);

    for (const QString& name : {QString("Alpha"), QString("Beta")}) {
        QVERIFY2(plugins.contains(name), qPrintable(name));
        const PluginInfo info = plugins.value(name);
        QCOMPARE(info.name, name);
        QVERIFY(info.loaded);
    }

    // Both were told to initialise and where they live, exactly once
    const FakePlugin* plugin = factory.owned.first();
    QCOMPARE(plugin->sent.count(MSG_INIT), 1);
    QCOMPARE(plugin->sent.count(MSG_PATH), 1);
    QCOMPARE(plugin->path, QDir::cleanPath(base + "/plugins/Alpha"));
}

void TestPluginRegistry::skipsDirectoriesThatHoldNoPlugin() {
    const QString base = loadProfile("notaplugin");
    addPluginDirectory(base, "Alpha");
    addPluginDirectory(base, "Empty");

    FakeFactory factory;
    factory.names["Alpha"] = "Alpha";
    // "Empty" is left out: the factory says there is nothing it can load

    PluginRegistry::instance().loadPlugins(factory);

    const QMap<QString, PluginInfo> plugins = PluginRegistry::instance().getPlugins();
    QCOMPARE(plugins.count(), 1);
    QVERIFY(plugins.contains("Alpha"));
    QVERIFY(!plugins.contains("Empty"));
    // The directory was never claimed by the factory, so there is nothing to
    // release either
}

void TestPluginRegistry::disabledPluginStaysInTheTable() {
    const QString base = loadProfile("disabled");
    addPluginDirectory(base, "Alpha");
    addPluginDirectory(base, "Beta");

    FakeFactory factory;
    factory.names["Alpha"] = "Alpha";
    factory.names["Beta"] = "Beta";
    disable({"Alpha"});

    PluginRegistry::instance().loadPlugins(factory);

    // Nothing was built for it: the factory was not even asked about the
    // directory. Mapping a DLL (or importing a module) for a plugin that never
    // gets a message is the cost this check exists to avoid.
    QVERIFY(!factory.instantiated.contains("Alpha"));
    QCOMPARE(factory.owned.count(), 1);

    const QMap<QString, PluginInfo> plugins = PluginRegistry::instance().getPlugins();
    // It has to stay in the table: the option dialog builds its list from there,
    // so a plugin dropped here could never be enabled again
    QCOMPARE(plugins.count(), 2);
    const PluginInfo disabled = plugins.value("Alpha");
    QVERIFY(!disabled.loaded);
    QVERIFY(disabled.obj == nullptr);
    // A skipped plugin can still describe itself by where it lives -- its own
    // MSG_GET_NAME answer only exists once it has been built
    QCOMPARE(disabled.name, QString("Alpha"));
    QCOMPARE(disabled.path, QDir::cleanPath(base + "/plugins/Alpha"));
    QVERIFY(plugins.value("Beta").loaded);
}

void TestPluginRegistry::enablingAPluginThatWasNeverBuiltLoadsIt() {
    const QString base = loadProfile("enable-later");
    addPluginDirectory(base, "Alpha");
    disable({"Alpha"});

    FakeFactory first;
    first.names["Alpha"] = "Alpha";
    PluginRegistry::instance().loadPlugins(first);
    QVERIFY(first.owned.isEmpty());

    // Ticking the checkbox has to build it now -- the entry it left behind is a
    // placeholder, not a loaded plugin
    enable({"Alpha"});
    FakeFactory second;
    second.names["Alpha"] = "Alpha";
    PluginRegistry::instance().loadPlugins(second);

    const PluginInfo info = PluginRegistry::instance().getPlugins().value("Alpha");
    QVERIFY(info.loaded);
    QVERIFY(info.obj != nullptr);
    QCOMPARE(info.name, QString("Alpha"));
    QCOMPARE(second.owned.count(), 1);
    // Built once, and initialised on the way in
    QCOMPARE(second.owned.first()->sent.count(MSG_INIT), 1);
    QCOMPARE(second.owned.first()->sent.count(MSG_PATH), 1);
}

void TestPluginRegistry::dispatchSkipsDisabledPlugins() {
    const QString base = loadProfile("dispatch");
    addPluginDirectory(base, "Alpha");
    addPluginDirectory(base, "Beta");

    FakeFactory factory;
    factory.names["Alpha"] = "Alpha";
    factory.names["Beta"] = "Beta";
    disable({"Beta"});

    PluginRegistry::instance().loadPlugins(factory);

    QList<InputData> input;
    input << InputData("query");
    QList<CatItem> results;
    PluginRegistry::instance().getResults(&input, &results);

    // Exactly one plugin answered, and its item ended up in the list. Beta was
    // never even built, so nothing there could have answered for it.
    QCOMPARE(results.count(), 1);
    QCOMPARE(results.first().shortName, QString("Alpha result"));
    QCOMPARE(resultQueries("Alpha", factory), 1);
    QCOMPARE(resultQueries("Beta", factory), 0);

    // And the same rule for a plugin switched off after it has been running:
    // the instance stays (nothing is ever unloaded), the messages stop
    setLoadState({{"Alpha", false}, {"Beta", true}});
    PluginRegistry::instance().loadPlugins(factory);

    results.clear();
    PluginRegistry::instance().getResults(&input, &results);
    QCOMPARE(results.count(), 1);
    QCOMPARE(results.first().shortName, QString("Beta result"));
    QCOMPARE(resultQueries("Alpha", factory), 1);
    QCOMPARE(resultQueries("Beta", factory), 1);
}

void TestPluginRegistry::dispatchFollowsTheTableOrder() {
    const QString base = loadProfile("order");
    // Created out of alphabetical order on purpose: what the plugins answer in
    // must come from the table, not from the order the directory listing
    // happens to return
    addPluginDirectory(base, "Charlie");
    addPluginDirectory(base, "Alpha");
    addPluginDirectory(base, "Beta");

    FakeFactory factory;
    factory.names["Alpha"] = "Alpha";
    factory.names["Beta"] = "Beta";
    factory.names["Charlie"] = "Charlie";

    PluginRegistry::instance().loadPlugins(factory);

    QStringList keys;
    for (const PluginInfo& info : PluginRegistry::instance().getPlugins()) {
        keys << info.name;
    }
    QCOMPARE(keys, QStringList({"Alpha", "Beta", "Charlie"}));

    // The table is a QMap, so every dispatch walks it in directory-name order:
    // a run is reproducible, and two plugins cannot swap places from one start
    // to the next (a QHash shuffled them per process)
    QList<InputData> input;
    input << InputData("query");
    QList<CatItem> results;
    PluginRegistry::instance().getResults(&input, &results);

    QCOMPARE(results.count(), 3);
    QCOMPARE(results.at(0).shortName, QString("Alpha result"));
    QCOMPARE(results.at(1).shortName, QString("Beta result"));
    QCOMPARE(results.at(2).shortName, QString("Charlie result"));
}

void TestPluginRegistry::getLabelsIgnoresEmptyInput() {
    const QString base = loadProfile("labels");
    addPluginDirectory(base, "Alpha");

    FakeFactory factory;
    factory.names["Alpha"] = "Alpha";

    PluginRegistry::instance().loadPlugins(factory);

    QList<InputData> empty;
    PluginRegistry::instance().getLabels(&empty);
    QCOMPARE(factory.owned.first()->labelQueries, 0);

    QList<InputData> input;
    input << InputData("query");
    PluginRegistry::instance().getLabels(&input);
    QCOMPARE(factory.owned.first()->labelQueries, 1);
}

void TestPluginRegistry::launchItemRoutesToTheOwningPlugin() {
    const QString base = loadProfile("launch");
    addPluginDirectory(base, "Alpha");

    FakeFactory factory;
    factory.names["Alpha"] = "Alpha";

    PluginRegistry::instance().loadPlugins(factory);

    CatItem item("c:/elsewhere/tool.exe", "tool");
    item.pluginName = "Alpha";

    QList<InputData> input;
    input << InputData("tool");

    // The plugin's own return code has to survive: it is how a plugin says
    // whether it consumed the launch or the platform should
    QCOMPARE(PluginRegistry::instance().launchItem(&input, &item),
             FakePlugin::kLaunchHandled);
    QCOMPARE(factory.owned.first()->launchedItem, item.shortName);
}

void TestPluginRegistry::launchItemOnDisabledPluginFallsBack() {
    const QString base = loadProfile("launch-disabled");
    addPluginDirectory(base, "Alpha");

    FakeFactory factory;
    factory.names["Alpha"] = "Alpha";
    disable({"Alpha"});

    PluginRegistry::instance().loadPlugins(factory);

    CatItem item("c:/elsewhere/tool.exe", "tool");
    item.pluginName = "Alpha";

    QList<InputData> input;
    input << InputData("tool");

    // Unknown and disabled look the same to the caller: fall back to Launchy's
    // own launching
    QCOMPARE(PluginRegistry::instance().launchItem(&input, &item),
             int(MSG_CONTROL_LAUNCHITEM));

    CatItem stranger("c:/elsewhere/other.exe", "other");
    stranger.pluginName = "Nobody";
    QCOMPARE(PluginRegistry::instance().launchItem(&input, &stranger),
             int(MSG_CONTROL_LAUNCHITEM));
}

void TestPluginRegistry::getCatalogsCollectsItemsAndReportsProgress() {
    const QString base = loadProfile("catalogs");
    addPluginDirectory(base, "Alpha");
    addPluginDirectory(base, "Beta");

    FakeFactory factory;
    factory.names["Alpha"] = "Alpha";
    factory.names["Beta"] = "Beta";

    PluginRegistry::instance().loadPlugins(factory);

    CatalogSlow catalog;
    ProgressRecorder progress;
    PluginRegistry::instance().getCatalogs(&catalog, &progress);

    QCOMPARE(catalog.count(), 2);
    // One step per loaded plugin, numbered from zero
    QCOMPARE(progress.steps.count(), 2);
    QCOMPARE(progress.steps.first(), 0);
    QCOMPARE(progress.steps.last(), 1);

    // A null recorder is legal, the builder does not always want progress
    CatalogSlow second;
    PluginRegistry::instance().getCatalogs(&second, nullptr);
    QCOMPARE(second.count(), 2);
}

void TestPluginRegistry::dialogHandleTravelsAsAnOpaqueValue() {
    const QString base = loadProfile("dialog");
    addPluginDirectory(base, "Alpha");

    // Core must not know what this is, so it is only ever compared, never
    // dereferenced -- the same way the application passes a QWidget*
    void* const dialog = reinterpret_cast<void*>(static_cast<quintptr>(0x1234));

    FakeFactory factory;
    factory.names["Alpha"] = "Alpha";
    factory.dialogs["Alpha"] = dialog;

    PluginRegistry::instance().loadPlugins(factory);

    QCOMPARE(PluginRegistry::instance().doDialog(nullptr, "Alpha"), dialog);
    QVERIFY(PluginRegistry::instance().doDialog(nullptr, "Nobody") == nullptr);

    PluginRegistry::instance().endDialog("Alpha", true);
    QVERIFY(factory.owned.first()->dialogClosedWith);
}

void TestPluginRegistry::reloadReplacesTheTable() {
    const QString base = loadProfile("reload");
    addPluginDirectory(base, "Alpha");

    FakeFactory first;
    first.names["Alpha"] = "Alpha";
    PluginRegistry::instance().loadPlugins(first);
    QCOMPARE(PluginRegistry::instance().getPlugins().count(), 1);

    // Second scan with a different directory: the table is rebuilt, not merged
    QDir(base + "/plugins/Alpha").removeRecursively();
    addPluginDirectory(base, "Beta");

    FakeFactory second;
    second.names["Beta"] = "Beta";

    PluginRegistry::instance().loadPlugins(second);

    const QMap<QString, PluginInfo> plugins = PluginRegistry::instance().getPlugins();
    QCOMPARE(plugins.count(), 1);
    QVERIFY(plugins.contains("Beta"));
    QVERIFY(!plugins.contains("Alpha"));
}

void TestPluginRegistry::reloadReusesTheInstance() {
    const QString base = loadProfile("reuse");
    addPluginDirectory(base, "Alpha");

    FakeFactory first;
    first.names["Alpha"] = "Alpha";
    PluginRegistry::instance().loadPlugins(first);
    QCOMPARE(first.owned.count(), 1);

    // A second pass over the same directory -- one click on the checkbox in the
    // option dialog -- must not build the plugin again: one ini entry means one
    // instance, and a second load would leave a module (or a python object) that
    // nothing in the table points at any more.
    FakeFactory second;
    second.names["Alpha"] = "Alpha";

    PluginRegistry::instance().loadPlugins(second);

    QVERIFY(second.instantiated.isEmpty());
    QVERIFY(second.owned.isEmpty());
    QCOMPARE(first.owned.count(), 1);

    // Nor is it restarted: a reload keeps a plugin, it does not re-initialise
    // one that was already running
    QCOMPARE(first.owned.first()->sent.count(MSG_INIT), 1);
    QVERIFY(PluginRegistry::instance().getPlugins().value("Alpha").loaded);
}

void TestPluginRegistry::disableThenEnableKeepsTheInstance() {
    const QString base = loadProfile("toggle");
    addPluginDirectory(base, "Alpha");

    FakeFactory factory;
    factory.names["Alpha"] = "Alpha";
    PluginRegistry::instance().loadPlugins(factory);
    QCOMPARE(factory.owned.count(), 1);
    FakePlugin* const plugin = factory.owned.first();

    disable({"Alpha"});
    PluginRegistry::instance().loadPlugins(factory);
    QVERIFY(!PluginRegistry::instance().getPlugins().value("Alpha").loaded);
    QCOMPARE(plugin->sent.count(MSG_INIT), 1);

    // Re-enabling hands back the very same instance, initialised again because
    // it is on its way out of the disabled state
    enable({"Alpha"});
    PluginRegistry::instance().loadPlugins(factory);

    const PluginInfo info = PluginRegistry::instance().getPlugins().value("Alpha");
    QVERIFY(info.loaded);
    QVERIFY(info.obj == plugin);
    QCOMPARE(factory.instantiated.count("Alpha"), 1);
    QCOMPARE(plugin->sent.count(MSG_INIT), 2);
}

QTEST_APPLESS_MAIN(TestPluginRegistry)

#include "TestPluginRegistry.moc"
