/*
Verby: Plugin for Launchy
Copyright (C) 2009  Simon Capewell

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

#include "precompiled.h"
#include "Verby.h"
#include "gui.h"


void VerbyPlugin::init()
{
}


void VerbyPlugin::setPath(QString * path)
{
	libPath = *path;
}


void VerbyPlugin::getID(uint* id)
{
	*id = HASH_VERBY;
}


void VerbyPlugin::getName(QString* str)
{
	*str = "Verby";
}


QString VerbyPlugin::getIcon()
{
	return getIconPath() + "verby.png";
}


void VerbyPlugin::getLabels(QList<InputData>* inputData)
{
	if (inputData->count() == 1)
	{
		// If it's not an item from Launchy's built in catalog, i.e. a file or directory or something added 
		// by a plugin, don't add verbs.
		if (inputData->first().getTopResult().id != 0)
			return;

		QString path = inputData->first().getTopResult().fullPath;
		QFileInfo info(path);

		if (info.isSymLink())
		{
			inputData->first().setLabel(HASH_LINK);
		}
		else if (info.isDir())
		{
			inputData->first().setLabel(HASH_DIR);
		}
		else if (info.isFile()) 
		{
			inputData->first().setLabel(HASH_FILE);
		}
	}
}


QString VerbyPlugin::getIconPath() const
{
	return libPath + "/icons/";
}


bool VerbyPlugin::isMatch(QString text1, QString text2)
{
	int text2Length = text2.count();
	int curChar = 0;

	foreach(QChar c, text1)
	{
		if (c.toLower() == text2[curChar].toLower())
		{
			++curChar;
			if (curChar >= text2Length)
			{
				return true;
			}
		}
	}

	return false;
}


void VerbyPlugin::addCatItem(QString text, QList<CatItem>* results, QString fullName, QString shortName, QString iconName)
{
	if (text.length() == 0 || isMatch(shortName, text))
	{
        CatItem item = CatItem(fullName, shortName, HASH_VERBY, getIconPath() + iconName);
		item.usage = (*settings)->value("verby/" + shortName.replace(" ", "") , 0).toInt();
		results->push_back(item);
	}
}


void VerbyPlugin::updateUsage(CatItem& item)
{
	(*settings)->setValue("verby/" + item.shortName.replace(" ", ""), item.usage + 1);
}


void VerbyPlugin::getResults(QList<InputData>* inputData, QList<CatItem>* results)
{
	if (inputData->count() == 2)
	{
		QString text = inputData->at(1).getText();

		if (inputData->first().hasLabel(HASH_DIR))
		{
			addCatItem(text, results, "Properties", "Directory properties", "properties.png");
		}
		else if (inputData->first().hasLabel(HASH_FILE))
		{
			addCatItem(text, results, "Open containing folder", "Open containing folder", "opencontainer.png");
			addCatItem(text, results, "Properties", "File properties", "properties.png");
		}
		else if (inputData->first().hasLabel(HASH_LINK))
		{
			addCatItem(text, results, "Run as", "Run as a different user", "run.png");
			addCatItem(text, results, "Open containing folder", "Open containing folder", "opencontainer.png");
			addCatItem(text, results, "Open shortcut folder", "Open shortcut folder", "opencontainer.png");
			addCatItem(text, results, "Copy path", "Copy path to clipboard", "copy.png");
			addCatItem(text, results, "Properties", "File properties", "properties.png");
		}
		else
		{
			return;
		}

		// Mark the item as a Verby item so that Verby has a chance to process it before Launchy
		inputData->first().setID(HASH_VERBY);
		inputData->first().getTopResult().id = HASH_VERBY;

		// ensure there's always an item at the top of the list for launching with parameters.
		results->push_front(CatItem(
			"Run",
			inputData->last().getText(), INT_MAX,
			getIconPath() + "run.png"));
	}
}


int VerbyPlugin::launchItem(QList<InputData>* inputData, CatItem* item)
{
	item = item; // Compiler Warning

	if (inputData->count() != 2)
	{
		// Tell Launchy to handle the command
		return MSG_CONTROL_LAUNCHITEM;
	}

	QString noun = inputData->first().getTopResult().fullPath;
	CatItem& verbItem = inputData->last().getTopResult();
	QString verb = verbItem.shortName;

	qDebug() << "Verby launchItem" << verb;
	if (verb == "Run")
	{
		runProgram(noun, "");
	}
	else if (verb == "Open containing folder")
	{
		QFileInfo info(noun);
		if (info.isSymLink())
		{
			info.setFile(info.symLinkTarget());
		}

#ifdef Q_WS_WIN
		runProgram("explorer.exe", "\"" + QDir::toNativeSeparators(info.absolutePath()) + "\"");
#endif
	}
	else if (verb == "Open shortcut folder")
	{
		QFileInfo info(noun);

#ifdef Q_WS_WIN
		runProgram("explorer.exe", "\"" + QDir::toNativeSeparators(info.absolutePath()) + "\"");
#endif
	}
	else if (verb == "Run as")
	{
#ifdef Q_WS_WIN
		SHELLEXECUTEINFO shellExecInfo;

		shellExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shellExecInfo.fMask = SEE_MASK_FLAG_NO_UI;
		shellExecInfo.hwnd = NULL;
		shellExecInfo.lpVerb = L"runas";
		shellExecInfo.lpFile = (LPCTSTR)noun.utf16();
		shellExecInfo.lpParameters = NULL;
		QDir dir(noun);
		QFileInfo info(noun);
		if (!info.isDir() && info.isFile())
			dir.cdUp();
		QString filePath = QDir::toNativeSeparators(dir.absolutePath());
		shellExecInfo.lpDirectory = (LPCTSTR)filePath.utf16();
		shellExecInfo.nShow = SW_NORMAL;
		shellExecInfo.hInstApp = NULL;

		ShellExecuteEx(&shellExecInfo);
#endif
	}
	else if (verb == "File properties")
	{
#ifdef Q_WS_WIN
		SHELLEXECUTEINFO shellExecInfo;

		shellExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		shellExecInfo.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_INVOKEIDLIST;
		shellExecInfo.hwnd = NULL;
		shellExecInfo.lpVerb = L"properties";
		QString filePath = QDir::toNativeSeparators(noun);
		shellExecInfo.lpFile = (LPCTSTR)filePath.utf16();
		shellExecInfo.lpIDList = NULL; 
		shellExecInfo.lpParameters = NULL;
		shellExecInfo.lpDirectory = NULL;
		shellExecInfo.nShow = SW_NORMAL;
		shellExecInfo.hInstApp = NULL;

		ShellExecuteEx(&shellExecInfo);
#endif
	}
	else if (verb == "Copy path to clipboard")
	{
		QFileInfo info(noun);
		if (info.isSymLink())
		{
			info.setFile(info.symLinkTarget());
		}
		QClipboard *clipboard = QApplication::clipboard();
		clipboard->setText(QDir::toNativeSeparators(info.canonicalFilePath()));
	}
	else
	{
		// Tell Launchy to handle the command
		return MSG_CONTROL_LAUNCHITEM;
	}

	updateUsage(verbItem);
	return true;
}


void VerbyPlugin::doDialog(QWidget* parent, QWidget** newDlg)
{
	if (gui == NULL)
	{
		gui = new Gui(parent);
		*newDlg = gui;
	}
}


void VerbyPlugin::endDialog(bool accept)
{
	if (accept)
	{
		gui->writeOptions();
		init();
	}
	if (gui != NULL) 
		delete gui;

	gui = NULL;
}


int VerbyPlugin::msg(int msgId, void* wParam, void* lParam)
{
	int handled = 0;
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
		handled = launchItem((QList<InputData>*) wParam, (CatItem*) lParam);
		break;
	case MSG_HAS_DIALOG:
		handled = true;
		break;
	case MSG_DO_DIALOG:
		doDialog((QWidget*) wParam, (QWidget**) lParam);
		break;
	case MSG_END_DIALOG:
		endDialog(wParam != 0);
		break;
	case MSG_PATH:
		setPath((QString *) wParam);
		break;

	default:
		break;
	}

	return handled;
}


Q_EXPORT_PLUGIN2(Verby, VerbyPlugin)
