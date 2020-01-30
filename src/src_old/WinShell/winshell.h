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

#ifndef winshell_H
#define winshell_H
#include "gui.h"
#include "globals.h"

#include "plugin_interface.h"



class winshellPlugin : public QObject, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface)


public:
	uint HASH_winshell;
	uint HASH_FILE;
	uint HASH_DIR;

private:
	Gui* gui;
public:
	winshellPlugin() {
		gui = NULL;
		HASH_winshell = qHash(QString("winshell"));
		HASH_DIR = qHash(QString("directory"));
		HASH_FILE = qHash(QString("file"));
	}
	~winshellPlugin() {}
	int msg(int msgId, void* wParam = NULL, void* lParam = NULL); 

	void getID(uint*);
	void getName(QString*);
	void getCatalog(QList<CatItem>* items);
	void getResults(QList<InputData>* id, QList<CatItem>* results);
	void launchItem(QList<InputData>*, CatItem*);
	void doDialog(QWidget* parent, QWidget**);
	void endDialog(bool accept);
	void getLabels(QList<InputData>* id);
	void init();
	QString getIcon();

};

extern winshellPlugin* gwinshellInstance;



#endif