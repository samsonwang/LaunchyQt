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

#pragma once

#include <QHash>
#include <QList>
#include <QMutex>
#include <QString>

#include "PluginInterface/CatalogItem.h"
#include "PluginInterface/InputData.h"

#include "PluginInfo.h"

namespace launchy {

class Catalog;

// This interface is used to notify clients when a step in a long running process occurs
class INotifyProgressStep {
public:
    virtual ~INotifyProgressStep() {}
    virtual bool progressStep(int newStep) = 0;
};

// Where plugin objects come from. Everything the registry cannot know -- is a
// directory holding a C++ module or a python package, who unloads the module,
// when the python interpreter is started -- sits behind this interface. That is
// what keeps the registry on Qt Core alone, and what lets the tests drive the
// real loading policy with fake plugins.
class IPluginFactory {
public:
    virtual ~IPluginFactory() {}

    // Build the plugin stored in 'path' and ask it for its name (MSG_GET_NAME).
    // 'name' is the directory name, the key the ini array uses.
    //
    // Returns an entry whose obj is nullptr when the directory holds nothing
    // this factory can load -- it has logged why (unknown type, broken module,
    // wrong module name, ...). The instance otherwise stays owned by the
    // factory; the registry drops a rejected one through release().
    virtual PluginInfo instantiate(const QString& name, const QString& path) = 0;

    // Drop the instance created for the directory 'name', because the user
    // disabled the plugin or because the registry refused it. What that means
    // (unload the module, delete the python object) is the factory's business.
    virtual void release(const QString& name) = 0;
};

// The plugin table and the message dispatch.
//
// What lives here is the policy: which plugins are enabled, in what order they
// are initialised, which of them see a given message, and what a plugin that
// carries plugins of its own does to them. What does not live here is how a
// plugin is produced or unloaded -- that is IPluginFactory -- nor any QWidget:
// the options dialog is handed over as an opaque handle because core is not
// allowed to name it.
class PluginRegistry {
public:
    static PluginRegistry& instance();

public:
    // Rebuild the table: scan the plugin directories reported by
    // SettingsManager, ask 'factory' for each directory, apply the
    // enabled/disabled choice from the ini and publish the result.
    void loadPlugins(IPluginFactory& factory);

    // A snapshot by value, never a reference: loadPlugins() may rebuild the
    // table from the GUI thread (the plugins page of the option dialog) while
    // a worker thread walks the result, so the caller must own a copy.
    QHash<QString, PluginInfo> getPlugins() const;

    // One entry, default constructed when there is no such plugin. Handy for
    // the callers that route a single message (launchItem, endDialog, ...).
    PluginInfo plugin(const QString& name) const;
    bool isLoaded(const QString& name) const;

    void showLaunchy();
    void hideLaunchy();

    void getLabels(QList<InputData>* inputData);
    void getResults(QList<InputData>* inputData, QList<CatItem>* results);
    void getCatalogs(Catalog* catalog, INotifyProgressStep* progressStep);

    int launchItem(QList<InputData>* inputData, CatItem* item);

    // The dialog is a QWidget* for the plugin and for the caller, but core does
    // not include QtWidgets: it travels as an opaque handle and the application
    // -- the only side that owns the type -- casts it back.
    void* doDialog(void* parent, const QString& name);
    void endDialog(const QString& name, bool accept);

private:
    PluginRegistry();
    Q_DISABLE_COPY(PluginRegistry)

    // Put one instantiated plugin into the table under 'key' (its directory
    // name), running the enable/disable decision and the init sequence.
    void install(const QString& key, PluginInfo info, IPluginFactory& factory,
                 QHash<QString, PluginInfo>* plugins);

    // No entry in the ini array means "load": only the plugins the option
    // dialog has written have one.
    bool isLoadable(const QString& name) const;

private:
    // Guards m_plugins and m_loadable. The lock is recursive because a plugin
    // callback (MSG_INIT while a reload is in progress) may turn around and
    // read the registry again on the same thread.
    mutable QMutex m_mutex{QMutex::Recursive};
    QHash<QString, PluginInfo> m_plugins;
    QHash<QString, bool> m_loadable;
};

} // namespace launchy
