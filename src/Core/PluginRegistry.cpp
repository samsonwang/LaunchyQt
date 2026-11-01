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

#include "PluginRegistry.h"

#include <QDebug>
#include <QDir>
#include <QMutexLocker>

#include <cassert>
#include <utility>

#include "PluginInterface/Runtime.h"
#include "PluginInterface/PluginMsg.h"

#include "Catalog.h"
#include "SettingsManager.h"

namespace launchy {

namespace {

// The array the option dialog maintains: directory name -> should this plugin
// be loaded. A plugin with no entry at all is loadable, that is how a freshly
// installed one starts enabled.
QHash<QString, bool> readLoadableTable() {
    QHash<QString, bool> table;
    if (g_settings.isNull()) {
        qWarning("PluginRegistry::loadPlugins, no settings, treating every plugin as enabled");
        return table;
    }

    const int size = g_settings->beginReadArray("Plugin");
    for (int i = 0; i < size; ++i) {
        g_settings->setArrayIndex(i);
        table[g_settings->value("name").toString()] = g_settings->value("load").toBool();
    }
    g_settings->endArray();
    return table;
}

} // namespace

PluginRegistry& PluginRegistry::instance() {
    static PluginRegistry s_obj;
    return s_obj;
}

PluginRegistry::PluginRegistry() {
}

void PluginRegistry::loadPlugins(IPluginFactory& factory) {
    // The catalog builder reads the table from its worker thread while the
    // option dialog calls loadPlugins() from the GUI thread, so the whole
    // reload runs under the lock and the finished table is published with a
    // single assignment at the end. A concurrent reader therefore always sees
    // a complete table instead of a half rebuilt one, and it can walk the copy
    // afterwards without holding the lock while the plugins are called.
    QMutexLocker locker(&m_mutex);

    m_loadable = readLoadableTable();

    QHash<QString, PluginInfo> plugins;
    const QList<QString> roots = SettingsManager::instance().directory("plugins");
    for (const QString& root : roots) {
        const QDir pluginsDir(root);
        const QStringList names = pluginsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& name : names) {
            const QString path = QDir::cleanPath(root + "/" + name);
            PluginInfo info = factory.instantiate(name, path);
            if (info.obj == nullptr) {
                // Nothing this process can load. The factory already said what
                // was wrong with the directory, so this stays silent.
                continue;
            }
            install(name, info, factory, &plugins);
        }
    }

    m_plugins = std::move(plugins);
}

void PluginRegistry::install(const QString& key, PluginInfo info, IPluginFactory& factory,
                             QHash<QString, PluginInfo>* plugins) {
    // A disabled plugin still has to be in the table: the option dialog builds
    // its list from it, so a plugin left out here disappears from the list and
    // can never be re-enabled, and its "load=false" entry is silently dropped
    // the next time the dialog rewrites the array.
    if (!isLoadable(key)) {
        qDebug() << "PluginRegistry::loadPlugins, plugin configured not to load:" << key;
        factory.release(key);
        info.obj = nullptr;
        info.loaded = false;
        (*plugins)[key] = info;
        return;
    }

    qDebug() << "PluginRegistry::loadPlugins, plugin loaded:" << key;
    info.loaded = true;
    info.sendMsg(MSG_INIT);
    info.sendMsg(MSG_PATH, &info.path);

    (*plugins)[key] = info;
}

bool PluginRegistry::isLoadable(const QString& name) const {
    return !m_loadable.contains(name) || m_loadable[name];
}

QHash<QString, PluginInfo> PluginRegistry::getPlugins() const {
    QMutexLocker locker(&m_mutex);
    // Copy: the table may be rebuilt right after the lock is dropped, the
    // caller keeps working on this snapshot
    return m_plugins;
}

PluginInfo PluginRegistry::plugin(const QString& name) const {
    QMutexLocker locker(&m_mutex);
    return m_plugins.value(name);
}

bool PluginRegistry::isLoaded(const QString& name) const {
    QMutexLocker locker(&m_mutex);
    const auto it = m_plugins.constFind(name);
    return it != m_plugins.constEnd() && it->loaded;
}

void PluginRegistry::showLaunchy() {
    QHash<QString, PluginInfo> plugins = getPlugins();
    for (PluginInfo info : plugins) {
        if (info.loaded) {
            info.sendMsg(MSG_LAUNCHY_SHOW);
        }
    }
}

void PluginRegistry::hideLaunchy() {
    QHash<QString, PluginInfo> plugins = getPlugins();
    for (PluginInfo info : plugins) {
        if (info.loaded) {
            info.sendMsg(MSG_LAUNCHY_HIDE);
        }
    }
}

void PluginRegistry::getLabels(QList<InputData>* inputData) {
    if (inputData->isEmpty()) {
        return;
    }

    QHash<QString, PluginInfo> plugins = getPlugins();
    for (PluginInfo info : plugins) {
        if (info.loaded) {
            info.sendMsg(MSG_GET_LABELS, (void*)inputData);
        }
    }
}

void PluginRegistry::getResults(QList<InputData>* inputData, QList<CatItem>* results) {
    if (inputData->isEmpty()) {
        return;
    }

    QHash<QString, PluginInfo> plugins = getPlugins();
    for (PluginInfo info : plugins) {
        if (info.loaded) {
            info.sendMsg(MSG_GET_RESULTS, (void*)inputData, (void*)results);
        }
    }
}

void PluginRegistry::getCatalogs(Catalog* pCatalog, INotifyProgressStep* progressStep) {
    // Called on the catalog builder worker thread, so take a snapshot and let
    // the plugin callbacks run without the lock: the GUI thread must stay free
    // to reload the plugin list while the catalog is being built
    QHash<QString, PluginInfo> plugins = getPlugins();
    int index = 0;

    for (PluginInfo info : plugins) {
        if (!info.loaded) {
            continue;
        }

        QList<CatItem> items;
        info.sendMsg(MSG_GET_CATALOG, &items);
        for (const CatItem& item : items) {
            pCatalog->addItem(item);
        }
        if (progressStep) {
            progressStep->progressStep(index);
        }
        ++index;
    }
}

int PluginRegistry::launchItem(QList<InputData>* inputData, CatItem* result) {
    assert(inputData);
    assert(result);

    PluginInfo info = plugin(result->pluginName);
    if (info.obj == nullptr || !info.loaded) {
        // Nobody claims the item: the caller then falls back to its own launch
        return MSG_CONTROL_LAUNCHITEM;
    }

    return info.sendMsg(MSG_LAUNCH_ITEM, inputData, result);
}

void* PluginRegistry::doDialog(void* parent, const QString& name) {
    PluginInfo info = plugin(name);
    if (info.obj == nullptr || !info.loaded) {
        return nullptr;
    }

    void* dialog = nullptr;
    info.sendMsg(MSG_DO_DIALOG, parent, &dialog);
    return dialog;
}

void PluginRegistry::endDialog(const QString& name, bool accept) {
    PluginInfo info = plugin(name);
    if (info.obj == nullptr || !info.loaded) {
        return;
    }

    info.sendMsg(MSG_END_DIALOG, (void*)accept);
}

} // namespace launchy
