/*
Tasky
Copyright (C) 2010  Daniel Ossipoff

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

#include <QList>
#include "PluginInterface.h"
#include "InputData.h"
#include "CatalogItem.h"

class Tasky : public QObject, public launchy::PluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PLUGININTERFACE_IID)
    Q_INTERFACES(launchy::PluginInterface)
public:
	Tasky();
	virtual ~Tasky();

	virtual int msg(int msgId, void* wParam = NULL, void* lParam = NULL); 

private:
    void setPath(const QString* path);
	void getLabels(QList<launchy::InputData>* inputList);
	void getID(uint* id);
	void getName(QString* name);
	void getResults(QList<launchy::InputData>* inputList, QList<launchy::CatItem>* result);
	void getCatalog(QList<launchy::CatItem>* item);
	void launchItem(QList<launchy::InputData>* inputList, launchy::CatItem* item);
	void doDialog(QWidget* parent, QWidget** dialog);
	void endDialog(bool accept);
	void init();
	QString getIcon();

    void initIconDir();

private:
    uint HASH_TASKY;
    QString m_libPath;
};

extern Tasky* g_taskyInstance;

