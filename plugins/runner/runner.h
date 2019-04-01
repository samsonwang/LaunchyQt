/*
Launchy: Application Launcher
Copyright (C) 2007  Josh Karlin

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

#ifndef RUNNER_H
#define RUNNER_H

#include <QList>
#include "PluginInterface.h"
#include "CatalogItem.h"
#include "InputData.h"

#include "globals.h"

class Gui;

class Runner : public QObject, public launchy::PluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PLUGININTERFACE_IID)
    Q_INTERFACES(launchy::PluginInterface)

public:
    Runner();
    virtual ~Runner();

    virtual int msg(int msgId, void* wParam = NULL, void* lParam = NULL);

private:
    void getID(uint* id);
    void getName(QString* name);
    void init();

    void setPath(const QString* path);

    void getCatalog(QList<launchy::CatItem>* items);
    void getResults(QList<launchy::InputData>* inputData,
                    QList<launchy::CatItem>* results);
    void launchItem(QList<launchy::InputData>* inputData,
                    launchy::CatItem* item);
    void doDialog(QWidget* parent, QWidget** dialog);
    void endDialog(bool accept);

    QString getIcon() const;
    QString getIcon(QString file) const;

private:
    uint HASH_RUNNER;
    QList<runnerCmd> m_cmds;
    QString m_libPath;
    QSharedPointer<Gui> m_gui;
};

#endif // RUNNER_H
