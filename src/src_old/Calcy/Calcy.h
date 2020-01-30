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

#pragma once

#include <QRegExp>
#include "PluginInterface.h"
#include "InputData.h"

class Gui;
class Calcy : public QObject, public launchy::PluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PLUGININTERFACE_IID)
    Q_INTERFACES(launchy::PluginInterface)
public:
    Calcy();
    virtual ~Calcy();

    virtual int msg(int msgId, void* wParam = NULL, void* lParam = NULL);

private:
    void getID(uint* id);
    void getName(QString* name);
    void setPath(const QString* path);
    void getLabels(QList<launchy::InputData>* inputData);
    void getResults(QList<launchy::InputData>* inputData, QList<launchy::CatItem>* result);
    int launchItem(QList<launchy::InputData>* inputData, launchy::CatItem* item);
    void doDialog(QWidget* parent, QWidget** dialog);
    void endDialog(bool accept);
    void init();
    QString getIcon();

private:
    uint HASH_CALCY;
    QString m_libPath;
    QRegExp m_reg;
    QPointer<Gui> m_gui;
};

extern Calcy* g_plugin;

