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
#include "gui.h"
#include "InputData.h"

class Calcy : public QObject, public PluginInterface {
	Q_OBJECT
    Q_PLUGIN_METADATA(IID "net.launchy.PluginInterface/1.0")
	Q_INTERFACES(PluginInterface)

public:
	Calcy();
	virtual ~Calcy();

    virtual int msg(int msgId, void* wParam = NULL, void* lParam = NULL);

	void setPath(QString* path);
	void getLabels(QList<InputData>* inputData);
	void getID(uint*);
	void getName(QString*);
	void getResults(QList<InputData>* id, QList<CatItem>* results);
	void launchItem(QList<InputData>* inputData, CatItem* item);
	void doDialog(QWidget* parent, QWidget**);
	void endDialog(bool accept);
	void init();
	QString getIcon();

private:
	uint HASH_CALCY;
	QString m_libPath;
	QRegExp m_reg;
	QScopedPointer<Gui> m_gui;
};

extern Calcy* g_plugin;

