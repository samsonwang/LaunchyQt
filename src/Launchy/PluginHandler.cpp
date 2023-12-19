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
    pluginpy::PluginLoader::initSettings(g_settings.data());

    foreach(QString directory, SettingsManager::instance().directory("plugins")) {
        // Load up the plugins in the plugins/ directory
        QDir pluginsDir(directory);
        foreach(QString pluginName, pluginsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QString pluginLibDir = QDir::cleanPath(directory + "/" + pluginName);
            if (QLibrary(pluginLibDir + "/" + pluginName).load()) {
                loadCppPlugin(pluginName, pluginLibDir);
            }
            else if (QFile::exists(pluginLibDir + "/" + pluginName + ".py")) {
                loadPythonPlugin(pluginName, pluginLibDir);
            }
            else {
                qWarning() << "PluginHandler::loadPlugins, unknown plugin type, plugin name: "
                    << pluginName << ", dir: " << pluginLibDir;
            }
        }
    }
}

void PluginHandler::showLaunchy() {
    foreach(PluginInfo info, m_plugins) {
        if (info.loaded)
            info.sendMsg(MSG_LAUNCHY_SHOW);
    }
}

void PluginHandler::hideLaunchy() {
    foreach(PluginInfo info, m_plugins) {
        if (info.loaded)
            info.sendMsg(MSG_LAUNCHY_HIDE);
    }
}

void PluginHandler::getLabels(QList<InputData>* inputData) {
    if (!inputData->isEmpty()) {
        foreach(PluginInfo info, m_plugins) {
            if (info.loaded)
                info.sendMsg(MSG_GET_LABELS, (void*)inputData);
        }
    }
}

void PluginHandler::getResults(QList<InputData>* inputData, QList<CatItem>* results) {
    if (!inputData->isEmpty()) {
        foreach(PluginInfo info, m_plugins) {
            if (info.loaded)
                info.sendMsg(MSG_GET_RESULTS, (void*)inputData, (void*)results);
        }
    }
}

void PluginHandler::getCatalogs(Catalog* pCatalog, INotifyProgressStep* progressStep) {
    int index = 0;

    foreach(PluginInfo info, m_plugins) {
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

    auto it = m_plugins.find(result->pluginName);
    if (it == m_plugins.end()) {
        return MSG_CONTROL_LAUNCHITEM;
    }

    if (!it->loaded) {
        return MSG_CONTROL_LAUNCHITEM;
    }

    return it->sendMsg(MSG_LAUNCH_ITEM, inputData, result);
}

QWidget* PluginHandler::doDialog(QWidget* parent, const QString& name) {
    if (!m_plugins.contains(name) || !m_plugins[name].loaded) {
        return nullptr;
    }
    QWidget* newBox = nullptr;
    m_plugins[name].sendMsg(MSG_DO_DIALOG, parent, &newBox);
    return newBox;
}

void PluginHandler::endDialog(const QString& name, bool accept) {
    if (!m_plugins.contains(name) || !m_plugins[name].loaded) {
        return;
    }
    m_plugins[name].sendMsg(MSG_END_DIALOG, (void*)accept);
}

const QHash<QString, launchy::PluginInfo> & PluginHandler::getPlugins() const {
    return m_plugins;
}

void PluginHandler::loadPythonPlugin(const QString& pluginName, const QString& pluginPath) {
    qDebug() << "PluginHandler::loadPythonPlugin, plugin:" << pluginName << "(" << pluginPath << ")";

    // this function gets correct PluginInfo and put it in member variable m_plugins
    // consider dynamic load the PluginPy library
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

    m_plugins[pluginName] = info;
}

void PluginHandler::loadCppPlugin(const QString& pluginName, const QString& pluginPath) {
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
            m_plugins[pluginInfo.name] = pluginInfo;
        }
    }
    else {
        info.loaded = false;
        loader.unload();
    }
    m_plugins[pluginName] = info;
}

PluginHandler::PluginHandler() {
}

} // namespace launchy
