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

#ifndef runner_H
#define runner_H

#include "gui.h"
#include "globals.h"
#include "plugin_interface.h"
#include <boost/shared_ptr.hpp>

class RunnerPlugin : public QObject, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface)

public:
	RunnerPlugin()
	{
		gui.reset();
		HASH_runner = qHash(QString("runner"));
	}
	~RunnerPlugin() {}
	
	int msg(int msgId, void* wParam = NULL, void* lParam = NULL); 

private:
	void setPath(QString * path);
	void getID(uint*);
	void getName(QString*);
	void getCatalog(QList<CatItem>* items);
	void getResults(QList<InputData>* inputData, QList<CatItem>* results);
	void launchItem(QList<InputData>* inputData, CatItem* item);
	void doDialog(QWidget* parent, QWidget**);
	void endDialog(bool accept);
	void init();
	QString getIcon();
	QString getIcon(QString file);

	uint HASH_WEBSITE;
	uint HASH_runner;
	QList<runnerCmd> cmds;
	QString libPath;
    boost::shared_ptr<Gui> gui;
};


#endif
