/*
Launchy: Application Launcher
Copyright (C) 2007-2009  Josh Karlin

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "PluginHandler.h"

#include <QPluginLoader>
#include <QDir>
#include <QDebug>
#include <QMutexLocker>

#include <utility>

#include "LaunchyLib/PluginInterface.h"
#include "LaunchyLib/PluginMsg.h"
#include "PluginPy/PluginLoader.h"

#include "Catalog.h"
#include "SettingsManager.h"

namespace launchy {

PluginHandler& PluginHandler::instance() {
    static PluginHandler s_obj;
    return s_obj;
}

void PluginHandler::loadPlugins() {
    // The catalog builder reads m_plugins from its worker thread while the
    // option dialog calls loadPlugins() from the GUI thread, so the whole
    // reload runs under the lock and the finished table is published with a
    // single assignment at the end. A concurrent reader therefore always sees
    // a complete table instead of a half rebuilt one, and it can walk the copy
    // afterwards without holding the lock while the plugins are called.
    QMutexLocker locker(&m_mutex);

    QHash<QString, PluginInfo> plugins;

    // Get the list of loadable plugins
    m_loadable.clear();
    int size = g_settings->beginReadArray("Plugin");
    for (int i = 0; i < size; ++i) {
        g_settings->setArrayIndex(i);
        QString name = g_settings->value("name").toString();
        bool toLoad = g_settings->value("load").toBool();
        m_loadable[name] = toLoad;
    }
    g_settings->endArray();

    // init QSetting for python plugin
    pluginpy::initSettings(g_settings.data());

    foreach(QString directory, SettingsManager::instance().directory("plugins")) {
        // Load up the plugins in the plugins/ directory
        QDir pluginsDir(directory);
        foreach(QString pluginName, pluginsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QString pluginLibDir = QDir::cleanPath(directory + "/" + pluginName);
            if (QLibrary(pluginLibDir + "/" + pluginName).load()) {
                loadCppPlugin(pluginName, pluginLibDir, &plugins);
            }
            else if (QFile::exists(pluginLibDir + "/" + pluginName + ".py")) {
                loadPythonPlugin(pluginName, pluginLibDir, &plugins);
            }
            else {
                qWarning() << "PluginHandler::loadPlugins, unknown plugin type, plugin name: "
                    << pluginName << ", dir: " << pluginLibDir;
            }
        }
    }

    m_plugins = std::move(plugins);
}

QHash<QString, launchy::PluginInfo> PluginHandler::getPlugins() const {
    QMutexLocker locker(&m_mutex);
    // Copy: the table may be rebuilt right after the lock is dropped, the
    // caller keeps working on this snapshot
    return m_plugins;
}

void PluginHandler::showLaunchy() {
    QHash<QString, PluginInfo> plugins = getPlugins();
    foreach(PluginInfo info, plugins) {
        if (info.loaded)
            info.sendMsg(MSG_LAUNCHY_SHOW);
    }
}

void PluginHandler::hideLaunchy() {
    QHash<QString, PluginInfo> plugins = getPlugins();
    foreach(PluginInfo info, plugins) {
        if (info.loaded)
            info.sendMsg(MSG_LAUNCHY_HIDE);
    }
}

void PluginHandler::getLabels(QList<InputData>* inputData) {
    if (!inputData->isEmpty()) {
        QHash<QString, PluginInfo> plugins = getPlugins();
        foreach(PluginInfo info, plugins) {
            if (info.loaded)
                info.sendMsg(MSG_GET_LABELS, (void*)inputData);
        }
    }
}

void PluginHandler::getResults(QList<InputData>* inputData, QList<CatItem>* results) {
    if (!inputData->isEmpty()) {
        QHash<QString, PluginInfo> plugins = getPlugins();
        foreach(PluginInfo info, plugins) {
            if (info.loaded)
                info.sendMsg(MSG_GET_RESULTS, (void*)inputData, (void*)results);
        }
    }
}

void PluginHandler::getCatalogs(Catalog* pCatalog, INotifyProgressStep* progressStep) {
    // Called on the catalog builder worker thread, so take a snapshot and let
    // the plugin callbacks run without the lock: the GUI thread must stay free
    // to reload the plugin list while the catalog is being built
    QHash<QString, PluginInfo> plugins = getPlugins();
    int index = 0;

    foreach(PluginInfo info, plugins) {
        if (info.loaded) {
            QList<CatItem> items;
            info.sendMsg(MSG_GET_CATALOG, &items);
            foreach(CatItem item, items) {
                pCatalog->addItem(item);
            }
            if (progressStep) {
                progressStep->progressStep(index);
            }
            ++index;
        }
    }
}

int PluginHandler::launchItem(QList<InputData>* inputData, CatItem* result) {
    assert(inputData);
    assert(result);

    QHash<QString, PluginInfo> plugins = getPlugins();
    auto it = plugins.find(result->pluginName);
    if (it == plugins.end()) {
        return MSG_CONTROL_LAUNCHITEM;
    }

    if (!it->loaded) {
        return MSG_CONTROL_LAUNCHITEM;
    }

    return it->sendMsg(MSG_LAUNCH_ITEM, inputData, result);
}

QWidget* PluginHandler::doDialog(QWidget* parent, const QString& name) {
    QHash<QString, PluginInfo> plugins = getPlugins();
    if (!plugins.contains(name) || !plugins[name].loaded) {
        return nullptr;
    }
    QWidget* newBox = nullptr;
    plugins[name].sendMsg(MSG_DO_DIALOG, parent, &newBox);
    return newBox;
}

void PluginHandler::endDialog(const QString& name, bool accept) {
    QHash<QString, PluginInfo> plugins = getPlugins();
    if (!plugins.contains(name) || !plugins[name].loaded) {
        return;
    }
    plugins[name].sendMsg(MSG_END_DIALOG, (void*)accept);
}

void PluginHandler::loadPythonPlugin(const QString& pluginName, const QString& pluginPath,
                                     QHash<QString, PluginInfo>* plugins) {
    qDebug() << "PluginHandler::loadPythonPlugin, plugin:" << pluginName << "(" << pluginPath << ")";

    // this function gets correct PluginInfo and puts it into the table passed in
    // (the caller publishes it as m_plugins once the whole reload is done)
    QString pluginFullPath = pluginPath + "/" + pluginName + ".py";
    pluginpy::PluginLoader loader(pluginName, pluginPath);
    PluginInterface* plugin = loader.instance();
    if (!plugin) {
        qWarning() << "PluginHandler::loadPythonPlugin, " << pluginFullPath << "is not a Launchy plugin";
        return;
    }
    qDebug() << "PluginHandler::loadPythonPlugin, plugin loaded:" << pluginFullPath;

    PluginInfo info;
    info.path = pluginPath;
    info.loaded = false;
    info.obj = plugin;

    if (!info.sendMsg(MSG_GET_NAME, &info.name)) {
        qWarning() << "PluginHandler::loadPythonPlugin, fail to get plugin name,"
            << " plugin path:" << pluginFullPath;
    }
    else if (info.name != pluginName) {
        qWarning() << "PluginHandler::loadPythonPlugin, plugin name not match:"
            << pluginName << ", " << info.name;
    }
    else if (!m_loadable.contains(pluginName) || m_loadable[pluginName]) {
        qDebug() << "PluginHandler::loadPythonPlugin, plugin loaded:" << pluginName;
        info.loaded = true;
        info.sendMsg(MSG_INIT);
        info.sendMsg(MSG_PATH, &info.path);
    }
    else {
        qDebug() << "PluginHandler::loadPythonPlugin, plugin configured not to load:"
            << pluginName;
        loader.unload();
    }

    (*plugins)[pluginName] = info;
}

void PluginHandler::loadCppPlugin(const QString& pluginName, const QString& pluginPath,
                                  QHash<QString, PluginInfo>* plugins) {
    QString pluginFullPath = pluginPath + "/" + pluginName;
    QPluginLoader loader(pluginFullPath);
    qDebug() << "PluginHandler::loadCppPlugin, plugin:" << pluginFullPath;
    PluginInterface* plugin = qobject_cast<PluginInterface*>(loader.instance());
    if (!plugin) {
        qWarning() << "PluginHandler::loadCppPlugin, " << pluginFullPath
            << "is not a valid plugin";
        return;
    }
    qDebug() << "PluginHandler::loadCppPlugin, plugin loaded:" << pluginFullPath;

    PluginInfo info;
    info.path = pluginPath;
    info.loaded = false;
    info.obj = plugin;

    if (!info.sendMsg(MSG_GET_NAME, &info.name)) {
        qWarning() << "PluginHandler::loadCppPlugin, fail to get plugin name,"
            << " plugin path:" << pluginFullPath;
    }
    else if (!m_loadable.contains(pluginName) || m_loadable[pluginName]) {
        info.loaded = true;
        info.sendMsg(MSG_INIT);
        info.sendMsg(MSG_PATH, &info.path);

        // Load any of the plugin's plugins of its own
        QList<PluginInfo> additionalPlugins;
        info.sendMsg(MSG_LOAD_PLUGINS, &additionalPlugins);

        foreach(PluginInfo pluginInfo, additionalPlugins) {
            if (!pluginInfo.isValid()) {
                continue;
            }

            bool isPluginLoadable =
                !m_loadable.contains(pluginInfo.name) || m_loadable[pluginInfo.name];

            if (isPluginLoadable) {
                pluginInfo.sendMsg(MSG_INIT);
                pluginInfo.loaded = true;
            }
            else {
                pluginInfo.sendMsg(MSG_UNLOAD_PLUGIN, &pluginInfo.name);
                pluginInfo.loaded = false;
            }
            (*plugins)[pluginInfo.name] = pluginInfo;
        }
    }
    else {
        info.loaded = false;
        loader.unload();
    }
    (*plugins)[pluginName] = info;
}

PluginHandler::PluginHandler() {
}

} // namespace launchy
