/*
Launchy: Application Launcher
Copyright (C) 2007-2010  Josh Karlin, Simon Capewell

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
#include "catalog_types.h"
#include "catalog_builder.h"
#include "globals.h"
#include "main.h"
#include "Directory.h"


CatalogBuilder::CatalogBuilder(PluginHandler* plugs) :
	plugins(plugs),
	progress(CATALOG_PROGRESS_MAX)
{
	catalog = new SlowCatalog();
}


void CatalogBuilder::buildCatalog()
{
	progress = CATALOG_PROGRESS_MIN;
	emit catalogIncrement(progress);
	catalog->incrementTimestamp();
	indexed.clear();

	QList<Directory> memDirs = SettingsManager::readCatalogDirectories();
	QHash<uint, PluginInfo> pluginsInfo = plugins->getPlugins();
	totalItems = memDirs.count() + pluginsInfo.count();
	currentItem = 0;

	while (currentItem < memDirs.count())
	{
		QString cur = platform->expandEnvironmentVars(memDirs[currentItem].name);
		indexDirectory(cur, memDirs[currentItem].types, memDirs[currentItem].indexDirs, memDirs[currentItem].indexExe, memDirs[currentItem].depth);
		progressStep(currentItem);
	}

	// Don't call the pluginhandler to request catalog because we need to track progress
	plugins->getCatalogs(catalog, this);

	catalog->purgeOldItems();
	indexed.clear();
	progress = CATALOG_PROGRESS_MAX;
	emit catalogFinished();
}


void CatalogBuilder::indexDirectory(const QString& directory, const QStringList& filters, bool fdirs, bool fbin, int depth)
{
	QString dir = QDir::toNativeSeparators(directory);
	QDir qd(dir);
	dir = qd.absolutePath();
	QStringList dirs = qd.entryList(QDir::AllDirs);

	if (depth > 0)
	{
		for (int i = 0; i < dirs.count(); ++i)
		{
			if (!dirs[i].startsWith("."))
			{
				QString cur = dirs[i];
				if (!cur.contains(".lnk"))
				{
#ifdef Q_WS_MAC
                                    // Special handling of app directories
                                    if (cur.endsWith(".app", Qt::CaseInsensitive)) {
                                        CatItem item(dir + "/" + cur);
                                        platform->alterItem(&item);
                                        catalog->addItem(item);
                                    }
                                    else
#endif
					indexDirectory(dir + "/" + dirs[i], filters, fdirs, fbin, depth-1);
				}
			}
		}
	}

	if (fdirs)
	{
		for (int i = 0; i < dirs.count(); ++i)
		{
			if (!dirs[i].startsWith(".") && !indexed.contains(dir + "/" + dirs[i]))
			{
				bool isShortcut = dirs[i].endsWith(".lnk", Qt::CaseInsensitive);

				CatItem item(dir + "/" + dirs[i], !isShortcut);
				catalog->addItem(item);
				indexed.insert(dir + "/" + dirs[i]);
			}
		}
	}
	else
	{
		// Grab any shortcut directories 
		// This is to work around a QT weirdness that treats shortcuts to directories as actual directories
		for (int i = 0; i < dirs.count(); ++i)
		{
			if (!dirs[i].startsWith(".") && dirs[i].endsWith(".lnk",Qt::CaseInsensitive))
			{
				if (!indexed.contains(dir + "/" + dirs[i]))
				{
					CatItem item(dir + "/" + dirs[i], true);
					catalog->addItem(item);
					indexed.insert(dir + "/" + dirs[i]);
				}
			}
		}
	}

	if (fbin)
	{
		QStringList bins = qd.entryList(QDir::Files | QDir::Executable);
		for (int i = 0; i < bins.count(); ++i)
		{
			if (!indexed.contains(dir + "/" + bins[i]))
			{
				CatItem item(dir + "/" + bins[i]);
				catalog->addItem(item);
				indexed.insert(dir + "/" + bins[i]);
			}
		}
	}

	// Don't want a null file filter, that matches everything..
	if (filters.count() == 0)
		return;

	QStringList files = qd.entryList(filters, QDir::Files | QDir::System, QDir::Unsorted );
	for (int i = 0; i < files.count(); ++i)
	{
		if (!indexed.contains(dir + "/" + files[i]))
		{
			CatItem item(dir + "/" + files[i]);
			platform->alterItem(&item);
#ifdef Q_WS_X11
                        if(item.fullPath.endsWith(".desktop") && item.icon == "")
                            continue;
#endif
			catalog->addItem(item);

			indexed.insert(dir + "/" + files[i]);
		}
	}
}


bool CatalogBuilder::progressStep(int newStep)
{
	newStep = newStep;

	++currentItem;
	int newProgress = (int)(CATALOG_PROGRESS_MAX * (float)currentItem / totalItems);
	if (newProgress != progress)
	{
		progress = newProgress;
		emit catalogIncrement(progress);
	}

	return true;
}
