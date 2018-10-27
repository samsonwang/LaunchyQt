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
#include <QDataStream>
#include <QSet>

/**
\brief CatItem (Catalog Item) stores a single item in the index
*/
class CatItem {
public:
    /** The full path of the indexed item */
    QString fullPath;
    /** The abbreviated name of the indexed item */
    QString shortName;
    /** The lowercase name of the indexed item */
    QString lowName;
    /** A path to an icon for the item */
    QString icon;
    /** How many times this item has been called by the user */
    int usage;
    /** This is unused, and meant for plugin writers and future extensions */
    void* data;
    /** The plugin id of the creator of this CatItem */
    int id;

    CatItem();

    CatItem(const QString& full, bool isDir = false);

    CatItem(const QString& full, const QString& shortN);

    CatItem(const QString& full, const QString& shortN, uint i_d);
    /** This is the constructor most used by plugins
    \param full The full path of the file to execute
    \param shortN The abbreviated name for the entry
    \param i_d Your plugin id (0 for Launchy itself)
    \param iconPath The path to the icon for this entry
    \warning It is usually a good idea to append ".your_plugin_name" to the end of the full parameter
    so that there are not multiple items in the index with the same full path.
    */
    CatItem(const QString& full, const QString& shortN, uint i_d, const QString& iconPath);

    bool operator==(const CatItem& other) const;
    bool operator!=(const CatItem& other) const;
};


/** InputData shows one segment (between tabs) of a user's query
    A user's query is typically represented by List<InputData>
    and each element of the list represents a segment of the query.

    E.g.  query = "google <tab> this is my search" will have 2 InputData segments
    in the list.  One for "google" and one for "this is my search"
*/
class InputData {
public:
    InputData();
    InputData(const QString& str);

    /** Get the labels applied to this query segment */
    QSet<uint> getLabels();
    /** Apply a label to this query segment */
    void setLabel(uint l);
    /** Remove a label from this query segment */
    void removeLabel(uint l);
    /** Check if it has the given label applied to it */
    bool hasLabel(uint l);

    /** Set the id of this query

    This can be used to override the owner of the selected catalog item, so that
    no matter what item is chosen from the catalog, the given plugin will be the one
    to execute it.

    \param i The plugin id of the plugin to execute the query's best match from the catalog
    */
    void setID(uint i);

    /** Returns the current owner id of the query */
    uint getID() const;

    /** Get the text of the query segment */
    const QString& getText() const;

    /** Set the text of the query segment */
    void setText(const QString& t);

    /** Get the text of the query segment */
    bool hasText() const;

    /** Get a pointer to the best catalog match for this segment of the query */
    CatItem&  getTopResult();
    const CatItem& getTopResult() const;

    /** Change the best catalog match for this segment */
    void setTopResult(const CatItem& sr);

    friend QDataStream &operator<<(QDataStream &out, const InputData &inputData);
    friend QDataStream &operator>>(QDataStream &in, InputData &inputData);

private:
    /** The user's entry */
    QString text;
    /** Any assigned labels to this query segment */
    QSet<uint> labels;
    /** A pointer to the best catalog match for this segment of the query */
    CatItem topResult;
    /** The plugin id of this query's owner */
    uint id;
};

bool CatLess(CatItem* left, CatItem* right);
bool CatLessNoPtr(CatItem& a, CatItem& b);

inline QDataStream &operator<<(QDataStream &out, const CatItem &item) {
    out << item.fullPath;
    out << item.shortName;
    out << item.lowName;
    out << item.icon;
    out << item.usage;
    out << item.id;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, CatItem &item) {
    in >> item.fullPath;
    in >> item.shortName;
    in >> item.lowName;
    in >> item.icon;
    in >> item.usage;
    in >> item.id;
    return in;
}

inline QDataStream &operator<<(QDataStream &out, const InputData &inputData) {
    out << inputData.text;
    out << inputData.labels;
    out << inputData.topResult;
    out << inputData.id;
    return out;
}

inline QDataStream &operator>>(QDataStream &in, InputData &inputData) {
    in >> inputData.text;
    in >> inputData.labels;
    in >> inputData.topResult;
    in >> inputData.id;
    return in;
}
