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

#pragma once

#include "catalog.h"
#include <QVector>
#include <QMutex> // os x

// These classes do not pertain to plugins

// Catalog provides methods to search and manage the indexed items
class Catalog
{
public:
	Catalog() : timestamp(0) {}
	virtual ~Catalog() {}
	bool load(const QString& filename);
	bool save(const QString& filename);
	void incrementTimestamp() { ++timestamp; }
	void searchCatalogs(const QString&, QList<CatItem>&);
	void promoteRecentlyUsedItems(const QString& text, QList<CatItem> & list);

	virtual int count() = 0;
	virtual void clear() = 0;
	virtual void addItem(const CatItem& item) = 0;
	virtual void purgeOldItems() = 0;

	virtual void incrementUsage(const CatItem& item) = 0;
	virtual void demoteItem(const CatItem& item) = 0;

	static bool matches(CatItem* item, const QString& match);
	static QString decorateText(const QString& text, const QString& match, bool outputRichText = false);

protected:	
	virtual const CatItem& getItem(int) = 0;
	virtual QList<CatItem*> search(const QString&) = 0;

	int timestamp;
	QMutex mutex;
};


// CatalogItem is used internally to store additional 
class CatalogItem : public CatItem
{
public:
	CatalogItem() : 
	  timestamp(0)
	{
	}

	CatalogItem(const CatItem& item, int time) :
	  timestamp(time)
	{
		fullPath = item.fullPath;
		shortName = item.shortName;
		lowName = item.lowName;
		icon = item.icon;
		usage = item.usage;
		data = item.data;
		id = item.id;
	}

	int timestamp;
};


/** This class does not pertain to plugins */
// The slow catalog searches slowly but
// adding items is fast and uses less memory
// than FastCatalog
class SlowCatalog : public Catalog
{
public:
	SlowCatalog() : Catalog() {}
	virtual int count() { return catalogItems.count(); }
	virtual void clear() { catalogItems.clear(); }
	virtual void addItem(const CatItem& item);
	virtual void purgeOldItems();

	virtual void incrementUsage(const CatItem& item);
	virtual void demoteItem(const CatItem& item);

protected:
	virtual const CatItem& getItem(int i) { return catalogItems[i]; }
	virtual QList<CatItem*> search(const QString&);

private:
	QVector<CatalogItem> catalogItems;
};
