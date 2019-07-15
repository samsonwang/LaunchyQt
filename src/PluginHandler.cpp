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
#include "PluginInterface.h"
#include "PluginMsg.h"
#include "Catalog.h"
#include "SettingsManager.h"
#include "PluginLoader.h"

#if defined(Q_OS_WIN)
#define LIB_EXT ".dll"
#elif defined(Q_OS_LINUX) || defined(Q_OS_MAC)
#define LIB_EXT ".so"
#endif

namespace launchy {

PluginHandler& PluginHandler::instance() {
    static PluginHandler s_obj;
    return s_obj;
}

PluginHandler::PluginHandler() {
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

void PluginHandler::getCatalogs(Catalog* catalog, INotifyProgressStep* progressStep) {
    int index = 0;

    foreach(PluginInfo info, m_plugins) {
        if (info.loaded) {
            QList<CatItem> items;
            info.sendMsg(MSG_GET_CATALOG, (void*)&items);
            foreach(CatItem item, items) {
                catalog->addItem(item);
            }
            if (progressStep) {
                progressStep->progressStep(index);
            }
            ++index;
        }
    }
}

int PluginHandler::launchItem(QList<InputData>* inputData, CatItem* result) {
    if (!m_plugins.contains(result->pluginId) || !m_plugins[result->pluginId].loaded) {
        return MSG_CONTROL_LAUNCHITEM;
    }
    return m_plugins[result->pluginId].sendMsg(MSG_LAUNCH_ITEM, (void*)inputData, (void*)result);
}

QWidget* PluginHandler::doDialog(QWidget* parent, uint id) {
    if (!m_plugins.contains(id) || !m_plugins[id].loaded) {
        return NULL;
    }
    QWidget* newBox = NULL;
    m_plugins[id].sendMsg(MSG_DO_DIALOG, (void*)parent, (void*)&newBox);
    return newBox;
}

void PluginHandler::endDialog(uint id, bool accept) {
    if (!m_plugins.contains(id) || !m_plugins[id].loaded) {
        return;
    }
    m_plugins[id].sendMsg(MSG_END_DIALOG, (void*)accept);
}

const QHash<uint, launchy::PluginInfo> & PluginHandler::getPlugins() const {
    return m_plugins;
}

void PluginHandler::loadPlugins() {
    // Get the list of loadable plugins
    m_loadable.clear();
    int size = g_settings->beginReadArray("plugins");
    for (int i = 0; i < size; ++i) {
        g_settings->setArrayIndex(i);
        uint id = g_settings->value("id").toUInt();
        bool toLoad = g_settings->value("load").toBool();
        m_loadable[id] = toLoad;
    }
    g_settings->endArray();

    // init QSetting for python plugin
    pluginpy::PluginLoader::initSettings(g_settings.data());

    foreach(QString directory, SettingsManager::instance().directory("plugins")) {
        // Load up the plugins in the plugins/ directory
        QDir pluginsDir(directory);
        foreach(QString pluginName, pluginsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            QString pluginLibDir = QDir::cleanPath(directory + "/" + pluginName);
            if (QFile::exists(pluginLibDir + "/" + pluginName + ".py")) {
                loadPythonPlugin(pluginName, pluginLibDir);
            }
            else if (QFile::exists(pluginLibDir + "/" + pluginName + LIB_EXT)) {
                loadCppPlugin(pluginName, pluginLibDir);
            }
        }
    }
}

void PluginHandler::loadPythonPlugin(const QString& pluginName, const QString& pluginPath) {
    qDebug() << "PluginHandler::loadPythonPlugin, plugin:" << pluginName << "(" << pluginPath << ")";

    // this function gets correct PluginInfo and put it in member variable m_plugins
    // consider dynamic load the PluginPy library
    QString pluginFullPath = pluginPath + "/" + pluginName + ".py";
    pluginpy::PluginLoader loader(pluginName, pluginPath);
    PluginInterface* plugin = loader.instance();
    if (!plugin) {
        qWarning() << pluginFullPath << "is not a Launchy plugin";
        return;
    }
    qDebug() << "PluginHandler::loadPythonPlugin, plugin loaded:" << pluginFullPath;

    PluginInfo info;
    info.obj = plugin;
    info.path = pluginFullPath;
    bool handled = (info.sendMsg(MSG_GET_ID, (void*)&info.id) != 0);
    info.sendMsg(MSG_GET_NAME, (void*)&info.name);

    // configured not to load
    if (handled && (!m_loadable.contains(info.id) || m_loadable[info.id])) {
        info.loaded = true;
        info.sendMsg(MSG_INIT);
        info.sendMsg(MSG_PATH, (void*)&pluginPath);
    }
    else {
        // set not load
        info.loaded = false;
        loader.unload();
    }

    m_plugins[info.id] = info;
}

void PluginHandler::loadCppPlugin(const QString& pluginName, const QString& pluginPath) {
    QString pluginFullPath = pluginPath + "/" + pluginName + LIB_EXT;
    QPluginLoader loader(pluginFullPath);
    qDebug() << "PluginHandler::loadCppPlugin, plugin:" << pluginFullPath;
    PluginInterface* plugin = qobject_cast<PluginInterface*>(loader.instance());
    if (!plugin) {
        qWarning() << pluginFullPath << "is not a Launchy plugin";
        return;
    }
    qDebug() << "Plugin loaded:" << pluginFullPath;

    PluginInfo info;
    info.obj = plugin;
    info.path = pluginFullPath;
    bool handled = info.sendMsg(MSG_GET_ID, (void*)&info.id) != 0;
    info.sendMsg(MSG_GET_NAME, (void*)&info.name);

    if (handled && (!m_loadable.contains(info.id) || m_loadable[info.id])) {
        info.loaded = true;
        info.sendMsg(MSG_INIT);
        info.sendMsg(MSG_PATH, (void*)&pluginPath);

        // Load any of the plugin's plugins of its own
        QList<PluginInfo> additionalPlugins;
        info.sendMsg(MSG_LOAD_PLUGINS, &additionalPlugins);

        foreach(PluginInfo pluginInfo, additionalPlugins) {
            if (!pluginInfo.isValid()) {
                continue;
            }

            bool isPluginLoadable =
                !m_loadable.contains(pluginInfo.id) || m_loadable[pluginInfo.id];

            if (isPluginLoadable) {
                pluginInfo.sendMsg(MSG_INIT);
                pluginInfo.loaded = true;
            }
            else {
                pluginInfo.sendMsg(MSG_UNLOAD_PLUGIN, (void*)(int64_t)pluginInfo.id);
                pluginInfo.loaded = false;
            }
            m_plugins[pluginInfo.id] = pluginInfo;
        }
    }
    else {
        info.loaded = false;
        loader.unload();
    }
    m_plugins[info.id] = info;
}


}
