/*
Launchy: Application Launcher
Copyright (C) 2007-2009  Josh Karlin

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

#include <QString>
#include "LaunchyLib.h"

class QDataStream;

/**
\brief CatItem (Catalog Item) stores a single item in the index
*/
namespace launchy {

class LAUNCHY_EXPORT CatItem {
public:
    enum SearchNameType {
        LOWER = 0,
        TRANS,
        CAPACITY
    };

public:
    /** The full path of the indexed item */
    QString fullPath;
    /** The abbreviated name of the indexed item
        It is a name without path and suffix */
    QString shortName;
    /** The transformed name of the indexed item,
        searchName[LOWER] is the lower form of shortName
        searchName[TRANS] is prepared to search and match non-English letters */
    QString searchName[CAPACITY];
    /** A path to an icon for the item */
    QString iconPath;
    /** How many times this item has been called by the user */
    int usage;
    /** This is unused, and meant for plugin writers and future extensions */
    void* data;
    /** The plugin id of the creator of this CatItem */
    uint pluginId;

    CatItem();

    CatItem(const QString& full, bool isDir = false);

    CatItem(const QString& full, const QString& shortN);

    CatItem(const QString& full, const QString& shortN, uint id);

    /** This is the constructor most used by plugins
    \param full The full path of the file to execute
    \param shortN The abbreviated name for the entry
    \param id Your plugin id (0 for Launchy itself)
    \param iconPath The path to the icon for this entry
    \warning It is usually a good idea to append ".your_plugin_name" to the end of the full parameter
    so that there are not multiple items in the index with the same full path.
    */
    CatItem(const QString& full, const QString& shortN, uint id, const QString& iconPath);

    bool operator==(const CatItem& other) const;
    bool operator!=(const CatItem& other) const;

    friend LAUNCHY_EXPORT QDataStream& operator<<(QDataStream& out, const CatItem& item);
    friend LAUNCHY_EXPORT QDataStream& operator>>(QDataStream& in, CatItem& item);

    /** Convert short name to search name */
    static QString convertSearchName(const QString& shortName);
};

}
