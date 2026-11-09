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
QMap<QString, bool> readLoadableTable() {
    QMap<QString, bool> table;
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

    QMap<QString, PluginInfo> plugins;
    const QList<QString> roots = SettingsManager::instance().directory("plugins");
    for (const QString& root : roots) {
        const QDir pluginsDir(root);
        const QStringList names = pluginsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& name : names) {
            const QString path = QDir::cleanPath(root + "/" + name);

            // A plugin is built once and then lives as long as the process, so
            // a reload -- one per click on a checkbox -- takes the entry it
            // already has instead of instantiating that directory again. The
            // path has to match as well: the same directory name under a
            // different root is a different plugin.
            const PluginInfo existing = m_plugins.value(name);
            if (existing.path == path && existing.obj != nullptr) {
                install(name, existing, &plugins);
                continue;
            }

            // A previous root has already claimed this plugin name. The later
            // entry silently overwrites the earlier one (QMap assignment), so
            // inform the user which copy wins and which one is ignored.
            if (plugins.contains(name) && plugins[name].path != path) {
                qInfo() << "PluginRegistry::loadPlugins, duplicate plugin name" << name
                        << "from" << path
                        << "overrides earlier entry at" << plugins[name].path;
            }

            // Configured off and never built: it stays unbuilt. This is the
            // whole point of checking the ini before instantiating -- building
            // a plugin maps its DLL and, for the first python one, starts the
            // interpreter, and none of that is worth paying for a plugin the
            // registry will not dispatch to anyway.
            //
            // The entry still goes into the table, just without an object: the
            // option dialog builds its list from there, so a plugin left out is
            // one the user could never switch back on (and its "load=false"
            // line would be dropped the next time the dialog rewrites the
            // array). Name and path are all a skipped plugin can say about
            // itself; its own MSG_GET_NAME answer only exists once it runs.
            if (!isLoadable(name)) {
                qDebug() << "PluginRegistry::loadPlugins, plugin configured not to load:" << name;
                PluginInfo info;
                info.name = name;
                info.path = path;
                plugins[name] = info;
                continue;
            }

            PluginInfo info = factory.instantiate(name, path);
            if (info.obj == nullptr) {
                // Nothing this process can load. The factory already said what
                // was wrong with the directory, so this stays silent.
                qInfo() << "PluginRegistry::loadPlugins, instantiate failed:" << name << path;
                continue;
            }
            install(name, info, &plugins);
        }
    }

    // An entry whose directory has disappeared simply falls out here. Nothing
    // is unloaded on the way: its code stays mapped until the process ends,
    // exactly like every other plugin's.
    m_plugins = std::move(plugins);

    // DIAG only: the dispatch table is rebuilt exactly here, so this is the one
    // place that can show a plugin which will never answer (a directory the
    // factory could not load leaves no entry at all). Run with -debug.
    for (auto it = m_plugins.constBegin(); it != m_plugins.constEnd(); ++it) {
        qInfo() << "PluginRegistry::loadPlugins, plugin entry:" << it.key()
                << "loaded" << it->loaded
                << "obj" << (it->obj != nullptr);
    }
}

void PluginRegistry::install(const QString& key, PluginInfo info,
                             QMap<QString, PluginInfo>* plugins) {
    // Reached only for an entry that already had an instance (the caller keeps
    // loadPlugins() from building a plugin that is configured off), so this is
    // the "switch off a plugin that is running" case.
    //
    // It keeps its instance. Disabling is a decision about dispatch, not a
    // statement about the module on disk: dropping the code here is what used
    // to leave a catalog build holding a vtable that had just been unmapped,
    // and what made re-enabling build the plugin all over again.
    if (!isLoadable(key)) {
        qDebug() << "PluginRegistry::loadPlugins, plugin configured not to load:" << key;
        info.loaded = false;
        (*plugins)[key] = info;
        return;
    }

    // Carried over from the previous table: it was initialised when it first
    // loaded, and a reload keeps a plugin, it does not restart it (a plugin
    // that gets MSG_INIT on every checkbox would re-read its directory and
    // rebuild its timers each time).
    if (info.loaded) {
        (*plugins)[key] = info;
        return;
    }

    qDebug() << "PluginRegistry::loadPlugins, plugin loaded:" << key;
    info.loaded = true;
    info.sendMsg(MSG_INIT);
    info.sendMsg(MSG_SET_PATH, &info.path);

    (*plugins)[key] = info;
}

bool PluginRegistry::isLoadable(const QString& name) const {
    return !m_loadable.contains(name) || m_loadable[name];
}

QMap<QString, PluginInfo> PluginRegistry::getPlugins() const {
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
    QMap<QString, PluginInfo> plugins = getPlugins();
    for (PluginInfo info : plugins) {
        if (info.loaded) {
            info.sendMsg(MSG_LAUNCHY_SHOW);
        }
    }
}

void PluginRegistry::hideLaunchy() {
    QMap<QString, PluginInfo> plugins = getPlugins();
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

    QMap<QString, PluginInfo> plugins = getPlugins();
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

    QMap<QString, PluginInfo> plugins = getPlugins();

    // DIAG only: what the dispatch layer actually handed to the plugins, and
    // how many items each one added. A plugin that adds nothing here either did
    // not run or chose not to answer, which is the distinction the bug report
    // needs. Run with -debug.
    qInfo() << "PluginRegistry::getResults, input segments" << inputData->count()
            << "text" << inputData->last().getText()
            << "plugin entries" << plugins.size();

    for (PluginInfo info : plugins) {
        if (info.loaded) {
            const int before = results->count();
            info.sendMsg(MSG_GET_RESULTS, (void*)inputData, (void*)results);
            qInfo() << "PluginRegistry::getResults, plugin:" << info.name
                    << "items" << before << "->" << results->count();
        }
    }
}

void PluginRegistry::getCatalogs(Catalog* pCatalog, INotifyProgressStep* progressStep) {
    // Called on the catalog builder worker thread, so take a snapshot and let
    // the plugin callbacks run without the lock: the GUI thread must stay free
    // to reload the plugin list while the catalog is being built
    QMap<QString, PluginInfo> plugins = getPlugins();
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
