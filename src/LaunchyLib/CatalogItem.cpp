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
#include <QDataStream>
#include <QDebug>
#include "UnicodeTable.h"

namespace launchy {

CatItem::CatItem()
    : usage(0),
      data(NULL) {

}

CatItem::CatItem(const QString& full, bool isDir)
    : fullPath(full),
      usage(0),
      data(NULL) {
    int last = fullPath.lastIndexOf("/");
    if (last == -1) {
        shortName = fullPath;
    }
    else {
        shortName = fullPath.mid(last+1);
        if (!isDir) {
            shortName = shortName.mid(0, shortName.lastIndexOf("."));
        }
    }

    searchName = shortName.toLower();
    searchNameTrans = convertSearchName(searchName);
}

CatItem::CatItem(const QString& full, const QString& shortN)
    : fullPath(full),
      shortName(shortN),
      usage(0),
      data(NULL) {

    searchName = shortName.toLower();
    searchNameTrans = convertSearchName(searchName);
}

CatItem::CatItem(const QString& full, const QString& shortN, const QString& plugin)
    : fullPath(full),
      shortName(shortN),
      usage(0),
      pluginName(plugin),
      data(NULL) {

    searchName = shortName.toLower();
    searchNameTrans = convertSearchName(searchName);
}

CatItem::CatItem(const QString& full, const QString& shortN,
                 const QString& plugin, const QString& iconPath)
    : fullPath(full),
      shortName(shortN),
      iconPath(iconPath),
      usage(0),
      pluginName(plugin),
      data(NULL) {

    searchName = shortName.toLower();
    searchNameTrans = convertSearchName(searchName);
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
    out << item.searchName;
    out << item.searchNameTrans;
    out << item.iconPath;
    out << item.pluginName;
    out << item.usage;
    return out;
}

QDataStream& operator>>(QDataStream& in, CatItem &item) {
    in >> item.fullPath;
    in >> item.shortName;
    in >> item.searchName;
    in >> item.searchNameTrans;
    in >> item.iconPath;
    in >> item.pluginName;
    in >> item.usage;
    return in;
}

QString CatItem::convertSearchName(const QString& shortName) {
    QString result = shortName.toLower();

    for (int i = 0; i<result.size(); ++i) {
        ushort code = result[i].unicode();

        // zhCN simplified Chinese (pin yin)
        if (code >= zhCN_min && code <= zhCN_max) {
            result[i] = zhCN_table[code - zhCN_min];
        }

    }
    qDebug() << "CatItem::convertSearchName, shortName:" << shortName
        << ", result:" << result;
    return result;
}

} // namespace launchy

