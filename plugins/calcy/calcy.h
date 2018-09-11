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

#ifndef CALCY_H
#define CALCY_H


#include "plugin_interface.h"
#include "gui.h"
#include <QRegExp>
#include <boost/shared_ptr.hpp>

using namespace boost;


class calcyPlugin : public QObject, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface)

public:
	calcyPlugin();
	~calcyPlugin();
	int msg(int msgId, void* wParam = NULL, void* lParam = NULL); 
	void setPath(QString * path);
	void getLabels(QList<InputData>*);
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
	QString libPath;
	QRegExp reg;
	shared_ptr<Gui> gui;
};


extern calcyPlugin* gPlugin;


#endif
