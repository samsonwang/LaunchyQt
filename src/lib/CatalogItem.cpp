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


#include "CatalogItem.h"
namespace launchy {
CatItem::CatItem()
    : usage(0),
      data(NULL),
      id(0) {

}

CatItem::CatItem(const QString& full, bool isDir)
    : fullPath(full),
      usage(0),
      data(NULL), 
      id(0) {
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
}

CatItem::CatItem(const QString& full, const QString& shortN)
    : fullPath(full),
      shortName(shortN),
      usage(0),
      data(NULL),
      id(0) {
    lowName = shortName.toLower();
}

CatItem::CatItem(const QString& full, const QString& shortN, uint i)
    : fullPath(full),
      shortName(shortN),
      usage(0),
      data(NULL),
      id(i) {
    lowName = shortName.toLower();
}

CatItem::CatItem(const QString& full, const QString& shortN, uint i, const QString& iconPath)
    : fullPath(full),
      shortName(shortN),
      icon(iconPath),
      usage(0),
      data(NULL),
      id(i) {
    lowName = shortName.toLower();
}

bool CatItem::operator!=(const CatItem& other) const {
    return !(*this == other);
}

bool CatItem::operator==(const CatItem& other) const {
    return fullPath == other.fullPath && shortName == other.shortName;
}

QDataStream& operator<<(QDataStream& out, const CatItem &item) {
    out << item.fullPath;
    out << item.shortName;
    out << item.lowName;
    out << item.icon;
    out << item.usage;
    out << item.id;
    return out;
}

QDataStream& operator>>(QDataStream& in, CatItem &item) {
    in >> item.fullPath;
    in >> item.shortName;
    in >> item.lowName;
    in >> item.icon;
    in >> item.usage;
    in >> item.id;
    return in;
}
}