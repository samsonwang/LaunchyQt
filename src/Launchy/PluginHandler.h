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

#include "LaunchyLib/PluginInterface.h"
#include "LaunchyLib/CatalogItem.h"
#include "LaunchyLib/InputData.h"
#include "LaunchyLib/PluginInfo.h"

namespace launchy {
class Catalog;
class INotifyProgressStep;

class PluginHandler {
public:
    static PluginHandler& instance();

public:
    void loadPlugins();

    void showLaunchy();
    void hideLaunchy();

    void getLabels(QList<InputData>* inputData);
    void getResults(QList<InputData>* inputData, QList<CatItem>* results);
    void getCatalogs(Catalog* catalog, INotifyProgressStep* progressStep);

    int launchItem(QList<InputData>* inputData, CatItem* item);

    QWidget* doDialog(QWidget* parent, const QString& name);
    void endDialog(const QString& name, bool accept);

    const QHash<QString, PluginInfo>& getPlugins() const;

private:
    // load plugin written in python
    void loadPythonPlugin(const QString& pluginName, const QString& pluginPath);
    // load plugin written in cpp
    void loadCppPlugin(const QString& pluginName, const QString& pluginPath);

private:
    PluginHandler();
    Q_DISABLE_COPY(PluginHandler)

private:
    QHash<QString, PluginInfo> m_plugins;
    QHash<QString, bool> m_loadable;
};

// This interface is used to notify clients when a step in a long running process occurs
class INotifyProgressStep {
public:
    virtual bool progressStep(int newStep) = 0;
};

} // namespace launchy
