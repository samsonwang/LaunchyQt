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

#ifndef controly_H
#define controly_H

#include "plugin_interface.h"
#include "gui.h"
#include "fhoicon.h"


class controlyPlugin : public QObject, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface)

private:
	uint HASH_controly;

	FhoIconCreator iconCreator;

	#ifdef WITH_GUI
		Gui* gui;
	#endif

public:
	controlyPlugin();
	~controlyPlugin();

	int msg(int msgId, void* wParam = NULL, void* lParam = NULL); 

private:
	void getID(uint*);
	void setPath(QString * path);
	void getName(QString*);
	void getCatalog(QList<CatItem>*);
	void init();
	QString getIcon();
	QString getIconPath() const;
	void getApps(QList<CatItem>* items);
	bool isMatch(QString text1, QString text2);
	void addCatItem(QString text, QList<CatItem>* results, QString fullName, QString shortName);
	void updateUsage(CatItem& item);
	void getResults(QList<InputData>* inputData, QList<CatItem>* results);
	int launchItem(QList<InputData>* inputData, CatItem* item);

	#ifdef WITH_GUI
		void doDialog(QWidget* parent, QWidget**);
		void endDialog(bool accept);
	#endif

	QString libPath;
};

extern controlyPlugin* gControlyInstance;

#endif