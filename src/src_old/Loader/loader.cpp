/*
Launchy: Application Launcher
Copyright (C) 2008  Josh Karlin

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

#include <QtGui>
#include <QUrl>
#include <QFile>
#include <QRegExp>
#include <QTextCodec>

#ifdef Q_WS_WIN
#include <windows.h>
#include <shlobj.h>
#include <tchar.h>

#endif

#include "loader.h"
#include "gui.h"

loaderPlugin* gloaderInstance = NULL;

void loaderPlugin::init()
{
	if (gloaderInstance == NULL)
		gloaderInstance = this;



//	QSettings* set = *settings;

}

void loaderPlugin::getID(uint* id)
{
	*id = HASH_loader;
}

void loaderPlugin::getName(QString* str)
{
	*str = "Plugin Loader";
}

void loaderPlugin::getLabels(QList<InputData>* id)
{
	if (id->count() > 1)
		return;


}



void loaderPlugin::getResults(QList<InputData>* id, QList<CatItem>* results)
{


}

QString loaderPlugin::getIcon()
{
	return libPath + "/icons/loader.png";
}



void loaderPlugin::getCatalog(QList<CatItem>* items)
{

}

void loaderPlugin::launchItem(QList<InputData>* id, CatItem* item)
{

}

void loaderPlugin::doDialog(QWidget* parent, QWidget** newDlg) {
	if (gui != NULL) return;
	gui = new Gui(parent);
	*newDlg = gui;
}

void loaderPlugin::endDialog(bool accept) {
	if (accept) {
	}
	if (gui != NULL) 
		delete gui;

	gui = NULL;
}

void loaderPlugin::setPath(QString * path) {
	libPath = *path;
}

int loaderPlugin::msg(int msgId, void* wParam, void* lParam)
{
	bool handled = false;
	switch (msgId)
	{		
	case MSG_INIT:
		init();
		handled = true;
		break;
	case MSG_GET_LABELS:
		getLabels((QList<InputData>*) wParam);
		handled = true;
		break;
	case MSG_GET_ID:
		getID((uint*) wParam);
		handled = true;
		break;
	case MSG_GET_NAME:
		getName((QString*) wParam);
		handled = true;
		break;
	case MSG_GET_RESULTS:
		getResults((QList<InputData>*) wParam, (QList<CatItem>*) lParam);
		handled = true;
		break;
	case MSG_GET_CATALOG:
		getCatalog((QList<CatItem>*) wParam);
		handled = true;
		break;
	case MSG_LAUNCH_ITEM:
		launchItem((QList<InputData>*) wParam, (CatItem*) lParam);
		handled = true;
		break;
	case MSG_HAS_DIALOG:
		handled = true;
		break;
	case MSG_DO_DIALOG:
		doDialog((QWidget*) wParam, (QWidget**) lParam);
		break;
	case MSG_END_DIALOG:
		endDialog((bool) wParam);
		break;
	case MSG_PATH:
		setPath((QString *) wParam);
	default:
		break;
	}

	return handled;
}

Q_EXPORT_PLUGIN2(loader, loaderPlugin) 
