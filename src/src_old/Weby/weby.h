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

#ifndef WEBY_H
#define WEBY_H


#include "precompiled.h"
#include "plugin_interface.h"
#include "globals.h"
#include "gui.h"
#include "IconCache.h"


class Suggest : public QObject
{
	Q_OBJECT
public:
	Suggest();
	void run(QString url, QString query);

	QStringList results;

public slots:
	void httpGetFinished(bool error);

private:
	QString query;
	QHttp http;
	QEventLoop loop;
	int id;
	static int currentId;	

};


using namespace boost;

class WebyPlugin : public QObject, public PluginInterface
{
	Q_OBJECT
	Q_INTERFACES(PluginInterface)


public:
	uint HASH_WEBSITE;
	uint HASH_DEFAULTSEARCH;
	uint HASH_WEBY;
	QList<WebySite> sites;
	QList<Bookmark> marks;

private:
	shared_ptr<Gui> gui;
	QString iconCachePath;
	IconCache* iconCache;
public:
	QString libPath;
	WebyPlugin() {
//		gui = NULL;

		HASH_WEBSITE = qHash(QString("website"));
		HASH_DEFAULTSEARCH = qHash(QString("defaultsearch"));
		HASH_WEBY = qHash(QString("weby"));
	}
	~WebyPlugin() {}
	int msg(int msgId, void* wParam = NULL, void* lParam = NULL); 
	void setPath(QString * path);
	void getLabels(QList<InputData>*);
	void getID(uint*);
	void getName(QString*);
	void getResults(QList<InputData>* inputData, QList<CatItem>* results);
	void getCatalog(QList<CatItem>* items);
	void launchItem(QList<InputData>* inputData, CatItem*);
	void doDialog(QWidget* parent, QWidget**);
	void endDialog(bool accept);
	WebySite getDefault();
	void init();
	QString getFirefoxPath();
	void indexFirefox(QString path, QList<CatItem>* items);
	QString getIcon();
#ifdef Q_WS_WIN
	void indexIE(QString path, QList<CatItem>* items);
#endif
};


#endif
