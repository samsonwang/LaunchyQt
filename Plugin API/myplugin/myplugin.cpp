/*
Launchy: Application Launcher
Copyright (C) 2007-2010  Josh Karlin

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

#include "myplugin.h"
#include "gui.h"

mypluginPlugin* gmypluginInstance = NULL;


mypluginPlugin::mypluginPlugin()
{
	HASH_myplugin = qHash(QString(PLUGIN_NAME));
	gmypluginInstance = this;
	gui = NULL;
}


mypluginPlugin::~mypluginPlugin()
{
}


void mypluginPlugin::getID(uint* id)
{
	*id = HASH_myplugin;
}


void mypluginPlugin::getName(QString* str)
{
	*str = PLUGIN_NAME;
}


void mypluginPlugin::init()
{
}



void mypluginPlugin::getLabels(QList<InputData>* id)
{
}


void mypluginPlugin::getResults(QList<InputData>* id, QList<CatItem>* results)
{
}


QString mypluginPlugin::getIcon()
{
        return qApp->applicationDirPath() + "/plugins/icons/myplugin.png";
}


void mypluginPlugin::getCatalog(QList<CatItem>* items)
{
}


void mypluginPlugin::launchItem(QList<InputData>* id, CatItem* item)
{
}


void mypluginPlugin::doDialog(QWidget* parent, QWidget** newDlg) 
{
	if (gui != NULL)
		return;
	gui = new Gui(parent);
	*newDlg = gui;
}


void mypluginPlugin::endDialog(bool accept) 
{
	if (accept)
	{
		gui->writeOptions();
		init();
	}
	delete gui;
	gui = NULL;
}


int mypluginPlugin::msg(int msgId, void* wParam, void* lParam)
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
			// Set to true if you provide a gui
			handled = false;
			break;
		case MSG_DO_DIALOG:
			// This isn't called unless you return true to MSG_HAS_DIALOG
			doDialog((QWidget*) wParam, (QWidget**) lParam);
			break;
		case MSG_END_DIALOG:
			// This isn't called unless you return true to MSG_HAS_DIALOG
			endDialog((bool) wParam);
			break;

		default:
			break;
	}
		
	return handled;
}

Q_EXPORT_PLUGIN2(myplugin, mypluginPlugin) 
