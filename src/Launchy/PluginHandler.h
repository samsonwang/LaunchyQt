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
#include <QString>

#include <memory>

#include "PluginInterface/CatalogItem.h"
#include "PluginInterface/InputData.h"

#include "PluginInfo.h"
#include "PluginRegistry.h"

class QWidget;

namespace launchy {

class PluginFactory;

// The application's plugin service, and the whole of what is left of it here:
// the registry in the core library owns the plugin table and does the message
// dispatch, this class only wires the factories into it and adapts the one call
// that carries a QWidget. It keeps no plugin state.
//
// It stays a class with a singleton accessor because the call sites read better
// as PluginHandler::instance().launchItem(...) than as a global function, and
// because doDialog() is the only place that has to know about widgets: making
// that a free function would spread the QWidget typing over the option dialog.
class PluginHandler {
public:
    static PluginHandler& instance();

public:
    // Scan the plugin directories and rebuild the table. Called once at
    // startup, and again whenever the option dialog's plugin page asks for it.
    void loadPlugins();

    QHash<QString, PluginInfo> getPlugins() const {
        return PluginRegistry::instance().getPlugins();
    }

    void showLaunchy() {
        PluginRegistry::instance().showLaunchy();
    }

    void hideLaunchy() {
        PluginRegistry::instance().hideLaunchy();
    }

    void getLabels(QList<InputData>* inputData) {
        PluginRegistry::instance().getLabels(inputData);
    }

    void getResults(QList<InputData>* inputData, QList<CatItem>* results) {
        PluginRegistry::instance().getResults(inputData, results);
    }

    void getCatalogs(Catalog* catalog, INotifyProgressStep* progressStep) {
        PluginRegistry::instance().getCatalogs(catalog, progressStep);
    }

    int launchItem(QList<InputData>* inputData, CatItem* item) {
        return PluginRegistry::instance().launchItem(inputData, item);
    }

    void endDialog(const QString& name, bool accept) {
        PluginRegistry::instance().endDialog(name, accept);
    }

    // The registry hands the dialog back as an opaque handle, see
    // PluginRegistry::doDialog().
    QWidget* doDialog(QWidget* parent, const QString& name);

private:
    PluginHandler();
    Q_DISABLE_COPY(PluginHandler)

private:
    std::unique_ptr<PluginFactory> m_factory;
};

} // namespace launchy
