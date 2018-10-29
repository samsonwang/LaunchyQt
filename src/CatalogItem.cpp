/*
Launchy: Application Launcher
Copyright (C) 2007-2009  Josh Karlin, Simon Capewell

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
#include "CatalogItem.h"
#include "GlobalVar.h"

bool CatLessNoPtr(CatItem & a, CatItem & b)
{
	bool less = CatLess(&a, &b);

/*	if (less)
		qDebug() << a.lowName << "(" << a.usage << ") < " << b.lowName << " (" << b.usage << ")";
	else
		qDebug() << b.lowName << "(" << b.usage << ") < " << a.lowName << " (" << a.usage << ")";
*/
	return less;
}


bool CatLess(CatItem* a, CatItem* b) {
	// Items with negative usage are lowest priority
	if (a->usage < 0 && b->usage >= 0)
		return false;
	if (b->usage < 0 && a->usage >= 0)
		return true;

	bool localEqual = a->lowName == g_searchText;
	bool otherEqual = b->lowName == g_searchText;

	// Exact match between search text and item name has higest priority
	if (localEqual && !otherEqual)
		return true;
	if (!localEqual && otherEqual)
		return false;

	int localFind = a->lowName.indexOf(g_searchText);
	int otherFind = b->lowName.indexOf(g_searchText);

	if (g_searchText.count() == 1)
	{
		// Match at the start
		if (localFind == 0 && otherFind != 0)
			return true;
		else if (localFind != 0 && otherFind == 0)
			return false;

		// Higher usage
		if (a->usage > b->usage)
			return true;
		if (a->usage < b->usage)
			return false;
	}

	// Contiguous text anywhere in the item name
	if (localFind != -1 && otherFind == -1)
		return true;
	else if (localFind == -1 && otherFind != -1)
		return false;

	if (localFind != -1 && otherFind != -1)
	{
		// Both have word matches
		// Higher usage
		if (a->usage > b->usage)
			return true;
		if (a->usage < b->usage)
			return false;

		// Contiguous text nearer the start of the item name
		if (localFind < otherFind)
			return true;
		else if (otherFind < localFind)
			return false;
	}
	else
	{
		// Higher usage
		if (a->usage > b->usage)
			return true;
		if (a->usage < b->usage)
			return false;
	}

	int localLen = a->lowName.count();
	int otherLen = b->lowName.count();

	// Favour shorter item names
	if (localLen < otherLen)
		return true;
	if (localLen > otherLen)
		return false;

	// Absolute tiebreaker to prevent loops
	return a->fullPath < b->fullPath;
}

CatItem::CatItem()
    : usage(0),
      data(NULL),
      id(0) {

}

CatItem::CatItem(const QString& full, bool isDir /*= false*/)
    : fullPath(full) {
    int last = fullPath.lastIndexOf("/");
    if (last == -1) {
        shortName = fullPath;

    }
    else {
        shortName = fullPath.mid(last+1);
        if (!isDir)
            shortName = shortName.mid(0, shortName.lastIndexOf("."));
    }

    lowName = shortName.toLower();
    data = NULL;
    usage = 0;
    id = 0;
}

CatItem::CatItem(const QString& full, const QString& shortN)
    : fullPath(full),
      shortName(shortN) {
    lowName = shortName.toLower();
    data = NULL;
    usage = 0;
    id = 0;
}

CatItem::CatItem(const QString& full, const QString& shortN, uint i_d)
    : fullPath(full),
      shortName(shortN),
      id(i_d) {
    lowName = shortName.toLower();
    data = NULL;
    usage = 0;
}

CatItem::CatItem(const QString& full, const QString& shortN, uint i_d, const QString& iconPath)
    : fullPath(full),
      shortName(shortN),
      icon(iconPath),
      id(i_d) {
    lowName = shortName.toLower();
    data = NULL;
    usage = 0;
}

bool CatItem::operator!=(const CatItem& other) const {
    return !(*this == other);
}

bool CatItem::operator==(const CatItem& other) const {
    return fullPath == other.fullPath && shortName == other.shortName;
}


