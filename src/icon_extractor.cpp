/*
Launchy: Application Launcher
Copyright (C) 2009 Simon Capewell, Josh Karlin

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
#include "icon_extractor.h"
#include "globals.h"
#include "main.h"


IconExtractor::IconExtractor()
{
}


void IconExtractor::processIcon(const CatItem& item, bool highPriority)
{
	mutex.lock();

	if (highPriority)
	{
		// use an id of -1 to indicate high priority
		if (items.count() > 0 && items[0].id == -1)
		{
			items.replace(0, item);
		}
		else
		{
			items.push_front(item);
		}
		items[0].id = -1;
	}
	else
	{
		items.push_back(item);
	}

	mutex.unlock();
#ifdef Q_WS_MAC
        run();
#else
	if (!isRunning())
		start(LowPriority);
#endif
    }


void IconExtractor::processIcons(const QList<CatItem>& newItems, bool reset)
{
	mutex.lock();

	int itemCount = items.size();
	if (reset && itemCount > 0 && isRunning())
	{
		// reset the queue, but keep the most recent high priority item
		CatItem item = items.dequeue();
		items.clear();
		if (item.id == -1)
			items.append(item);
		itemCount = items.size();
	}

	items += newItems;
	for (int i = itemCount; i < items.size(); ++i)
		items[i].id = i - itemCount;

	mutex.unlock();

#ifdef Q_WS_MAC
run();
#else

	if (!isRunning())
		start(IdlePriority);
#endif
}


void IconExtractor::stop()
{
	mutex.lock();
	items.clear();
	mutex.unlock();
}


void IconExtractor::run()
{
#ifdef Q_WS_WIN
	CoInitialize(NULL);
#endif

	CatItem item;
	bool itemsRemaining = true;

	do
	{
		mutex.lock();
		itemsRemaining = items.size() > 0;
		if (itemsRemaining)
			item = items.dequeue();
		mutex.unlock();
		if (itemsRemaining)
		{
			QIcon icon = getIcon(item);
			emit iconExtracted(item.id, item.fullPath, icon);
		}
	}
	while (itemsRemaining);

#ifdef Q_WS_WIN
	CoUninitialize();
#endif
}


QIcon IconExtractor::getIcon(const CatItem& item)
{
	qDebug() << "Fetching icon for" << item.fullPath;

#ifdef Q_WS_MAC
    if (item.icon.endsWith(".png") || item.icon.endsWith(".ico"))
        return QIcon(item.icon);
#endif

	if (item.icon.isNull())
	{
#ifdef Q_WS_X11
		QFileInfo info(item.fullPath);
		if (info.isDir())
			return platform->icon(QFileIconProvider::Folder);
#endif
		if (item.fullPath.length() == 0)
			return QIcon();	
		return platform->icon(QDir::toNativeSeparators(item.fullPath));
	}
	else
	{
#ifdef Q_WS_X11
		if (QFile::exists(item.icon))
		{
			return QIcon(item.icon);
		}
#endif
		return platform->icon(QDir::toNativeSeparators(item.icon));
	}
}
