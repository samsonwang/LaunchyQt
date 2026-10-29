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
#include <QJsonObject>
#include "UnicodeTable.h"

namespace launchy {

QJsonObject catItemToJson(const CatItem& item) {
    QJsonObject obj;
    obj["fullPath"] = item.fullPath;
    obj["shortName"] = item.shortName;
    obj["iconPath"] = item.iconPath;
    obj["pluginName"] = item.pluginName;
    obj["usage"] = item.usage;
    return obj;
}

// Rebuild a CatItem from the stored fields. Returns false for entries that
// can never match anything (missing or empty path/name).
bool catItemFromJson(const QJsonObject& obj, CatItem& item) {
    const QJsonValue fullPathValue = obj["fullPath"];
    const QJsonValue shortNameValue = obj["shortName"];
    if (!fullPathValue.isString() || !shortNameValue.isString()) {
        return false;
    }

    const QString fullPath = fullPathValue.toString();
    const QString shortName = shortNameValue.toString();
    if (fullPath.isEmpty() || shortName.isEmpty()) {
        return false;
    }

    // The 4-argument constructor recomputes searchName / searchNameTrans
    // from shortName, matching what every other creation path does
    item = CatItem(fullPath, shortName,
                   obj["pluginName"].toString(),
                   obj["iconPath"].toString());
    // Usage may be negative: a demoted item has usage -1
    item.usage = obj["usage"].toInt();
    return true;
}

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
            // A leading dot (".bashrc") or a name with no extension must keep
            // the full name; lastIndexOf(".") would otherwise yield an empty
            // string (mid(0, 0)) or, for no dot at all, mid(0, -1).
            const int dot = shortName.lastIndexOf(".");
            if (dot > 0) {
                shortName = shortName.mid(0, dot);
            }
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
    // Logging here would emit one line per constructed item, which flooded
    // the log (and the disk when debug logging is on) during a rebuild
    return result;
}

} // namespace launchy

