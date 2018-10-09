/*
Launchy: Application Launcher
Copyright (C) 2010  Josh Karlin, Simon Capewell

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

#include <QMessageBox>
#include "precompiled.h"
#include "SettingsManager.h"
#include "LaunchyWidget.h"
#include "globals.h"
#include "platform_base.h"

SettingsManager g_settingMgr;

const char* iniName = "/launchy.ini";
const char* dbName = "/launchy.db";
const char* historyName = "/history.db";


SettingsManager::SettingsManager()
{
	portable = false;
}


void SettingsManager::load()
{
	if (g_settings != NULL)
	{
		delete g_settings;
		g_settings = NULL;
	}

	// Load settings
	dirs = g_platform->getDirectories();
	portable = QFile::exists(dirs["portableConfig"][0] + iniName);

	qDebug("Loading settings in %s mode from %s", portable ? "portable" : "installed", qPrintable(configDirectory(portable)));
	QString iniPath = configDirectory(portable) + iniName;
	g_settings = new QSettings(configDirectory(portable) + iniName, QSettings::IniFormat);
	if (!QFile::exists(iniPath))
	{
		// Ini file doesn't exist, create some defaults and save them to disk
		QList<Directory> directories = g_platform->getDefaultCatalogDirectories();
		writeCatalogDirectories(directories);
	}
}


bool SettingsManager::isPortable() const
{
	return portable;
}


QList<QString> SettingsManager::directory(QString name) const
{
	return dirs[name];
}


QString SettingsManager::catalogFilename() const
{
	return configDirectory(portable) + dbName;
}


QString SettingsManager::historyFilename() const
{
	return configDirectory(portable) + historyName;
}


// Find the skin with the specified name ensuring that it contains at least a stylesheet
QString SettingsManager::skinPath(const QString& skinName) const
{
	QString directory;

    foreach(QString dir, dirs["skins"])
	{
		dir += QString("/") + skinName + "/";
        if (QFile::exists(dir + "style.qss"))
		{
			directory = dir;
			break;
		}
	}

    return directory;
}


// Switch between portable and installed mode
void SettingsManager::setPortable(bool makePortable)
{
	if (makePortable != portable)
	{
		qDebug("Converting to %s mode", makePortable ? "portable" : "installed");

		// Destroy the QSettings object first so it writes any changes to disk
		if (g_settings != NULL)
		{
			delete g_settings;
			g_settings = NULL;
		}

		QString oldDir = configDirectory(portable);
		QString oldIniName = oldDir + iniName;
		QString oldDbName = oldDir + dbName;
		QString oldHistoryName = oldDir + historyName;

		// Copy the settings to the new location
		// and delete the original settings if they are copied successfully
		QString newDir = configDirectory(makePortable);
		if (QFile::copy(oldIniName, newDir + iniName) &&
			QFile::copy(oldDbName, newDir + dbName) &&
			QFile::copy(oldHistoryName, newDir + historyName))
		{
			QFile::remove(oldIniName);
			QFile::remove(oldDbName);
			QFile::remove(oldHistoryName);
		}
		else
		{
			qWarning("Could not convert to %s mode", makePortable ? "portable" : "installed");
			if (makePortable)
			{
				QMessageBox::warning(g_mainWidget, tr("Launchy"),
                                     tr("Could not convert to portable mode."
                                     " Please check you have write access to the %1 directory.")
                                     .arg(newDir));
			}
		}

		load();
	}
}


// Delete all settings files in both installed and portable directories
void SettingsManager::removeAll()
{
	QFile::remove(configDirectory(false) + iniName);
	QFile::remove(configDirectory(false) + dbName);
	QFile::remove(configDirectory(false) + historyName);

	QFile::remove(configDirectory(true) + iniName);
	QFile::remove(configDirectory(true) + dbName);
	QFile::remove(configDirectory(true) + historyName);
}


// Get the configuration directory
QString SettingsManager::configDirectory(bool portable) const
{
	QString result = dirs[portable ? "portableConfig" : "config"][0];
	if (profileName.length() > 0)
	{
		result += "/profiles/" + profileName;
	}
	return result;
}


void SettingsManager::setProfileName(const QString& name)
{
	profileName = name;
}


QList<Directory> SettingsManager::readCatalogDirectories()
{
	QList<Directory> result;
	int size = g_settings->beginReadArray("directories");
	for (int i = 0; i < size; ++i)
	{
		g_settings->setArrayIndex(i);
		Directory tmp;
		tmp.name = g_settings->value("name").toString();
		if (tmp.name.length() > 0)
		{
			tmp.types = g_settings->value("types").toStringList();
			tmp.indexDirs = g_settings->value("indexDirs", false).toBool();
			tmp.indexExe = g_settings->value("indexExes", false).toBool();
			tmp.depth = g_settings->value("depth", 100).toInt();
			result.append(tmp);
		}
	}
	g_settings->endArray();

	return result;
}


void SettingsManager::writeCatalogDirectories(QList<Directory>& directories)
{
	g_settings->beginWriteArray("directories");
	for (int i = 0; i < directories.count(); ++i)
	{
		if (directories[i].name.length() > 0)
		{
			g_settings->setArrayIndex(i);
			g_settings->setValue("name", directories[i].name);
			g_settings->setValue("types", directories[i].types);
			g_settings->setValue("indexDirs", directories[i].indexDirs);
			g_settings->setValue("indexExes", directories[i].indexExe);
			g_settings->setValue("depth", directories[i].depth);
		}
	}
	g_settings->endArray();
}
