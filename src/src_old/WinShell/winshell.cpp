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

#include <QtGui>
#include <QFile>
#include <QDir>

#ifdef Q_WS_WIN
#include <windows.h>
#include <shlobj.h>
#include <tchar.h>

#endif

#include "winshell.h"
#include "gui.h"

winshellPlugin* gwinshellInstance = NULL;

void winshellPlugin::init()
{
	if (gwinshellInstance == NULL)
		gwinshellInstance = this;

}

void winshellPlugin::getID(uint* id)
{
	*id = HASH_winshell;
}

void winshellPlugin::getName(QString* str)
{
	*str = "Winshell";
}


QString winshellPlugin::getIcon()
{
#ifdef Q_WS_WIN
	return qApp->applicationDirPath() + "/plugins/icons/winshell.ico";
#endif
}


void winshellPlugin::getLabels(QList<InputData>* id)
{
	QString & text = id->last().getText();

	// Is it a file?
	if (!text.contains("\\") && !text.contains("/"))
		return;
	text = QDir::fromNativeSeparators(text);

	// Network searches are too slow
	if (text.startsWith("//")) return;


	QDir qd;
	QFile qf;
	// Has the user typed in a file or directory?
	if (qd.exists(text)) {
		id->last().setLabel(HASH_DIR);
		return;
	}
	if (qf.exists(text)) {
		id->last().setLabel(HASH_FILE);
		return;
	}

	QString path = id->last().getTopResult().fullPath;

	if (qd.exists(path)) {
		id->last().setLabel(HASH_DIR);
		return;
	}
	if (qf.exists(path)) {
		id->last().setLabel(HASH_FILE);
		return;
	}




}


void winshellPlugin::getResults(QList<InputData>* id, QList<CatItem>* results)
{
	if (id->count() == 2) {
		if (id->first().hasLabel(HASH_DIR) || id->first().hasLabel(HASH_FILE)) {
			results->push_back(CatItem("Open With", "Open With", HASH_winshell, getIcon()));
			results->push_back(CatItem("Copy to", "Copy to", HASH_winshell, getIcon()));
			results->push_back(CatItem("Move to", "Move to", HASH_winshell, getIcon()));
		}
	}

	if (id->count() == 3) {
		if (id->first().hasLabel(HASH_DIR) || id->first().hasLabel(HASH_FILE)) {
			CatItem* it = &((InputData)id->at(1)).getTopResult();
			if (it->id == HASH_winshell) {
				if (id->last().hasLabel(HASH_DIR) || id->last().hasLabel(HASH_FILE)) {
					id->last().setID(HASH_winshell);
				}
			}
		}
	}
}

void CopyDir(QString f1, QString f2) {
#ifdef Q_WS_WIN
	runProgram("utilities/winshell/copydir.bat", f1 + " " + f2);
#endif
}

void CopyFile(QString f1, QString f2) {
#ifdef Q_WS_WIN
	runProgram("utilities/winshell/copyfile.bat", f1 + " " + f2);
#endif
}

void Move(QString f1, QString f2) {
#ifdef Q_WS_WIN
	runProgram("utilities/winshell/move.bat", f1 + " " + f2);
#endif
}


void winshellPlugin::launchItem(QList<InputData>* id, CatItem* item)
{
	item = item; // Compiler Warning
	QString file;
	QString args;
	
	if (id->count() != 3)
		return;

	CatItem* it = &((InputData)id->at(1)).getTopResult();

	QString noun1 = id->first().getTopResult().fullPath;
	QString verb = it->shortName;
	QString noun2 = id->last().getTopResult().fullPath;


	if (verb == "Copy to") {
		if (!id->last().hasLabel(HASH_DIR))
			return;

		if (id->first().hasLabel(HASH_DIR)) {
			CopyDir(noun1, noun2);
		}
		else {
			CopyFile(noun1, noun2);
		}
	}
	else if (verb == "Move to") {
		if (!id->last().hasLabel(HASH_DIR))
			return;
		Move(noun1, noun2);
	}

//	runProgram("hi", "");
//	runProgram(file, args);
}

void winshellPlugin::doDialog(QWidget* parent, QWidget** newDlg) {
	if (gui != NULL) return;
	gui = new Gui(parent);
	*newDlg = gui;
}

void winshellPlugin::endDialog(bool accept) {
	if (accept) {
		gui->writeOptions();
		init();
	}
	if (gui != NULL) 
		delete gui;
	
	gui = NULL;
}

int winshellPlugin::msg(int msgId, void* wParam, void* lParam)
{
	bool handled = false;
	switch (msgId)
	{		
		case MSG_INIT:
			init();
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
		case MSG_GET_LABELS:
			getLabels((QList<InputData>*) wParam);
			handled = true;
			break;
		case MSG_GET_RESULTS:
			getResults((QList<InputData>*) wParam, (QList<CatItem>*) lParam);
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

		default:
			break;
	}
		
	return handled;
}

Q_EXPORT_PLUGIN2(winshell, winshellPlugin) 