/*
Launchy: Application Launcher
Copyright (C) 2009  Simon Capewell

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

#include <QList>
#include <QSet>

#include "LaunchyLib.h"
#include "CatalogItem.h"

class QDataStream;

/** InputData shows one segment (between tabs) of a user's query
A user's query is typically represented by List<InputData>
and each element of the list represents a segment of the query.

E.g.  query = "google <tab> this is my search" will have 2 InputData segments
in the list.  One for "google" and one for "this is my search"
*/
namespace launchy {

class LAUNCHY_EXPORT InputData {
public:
    InputData();
    InputData(const QString& str);

public:
    /** Get the labels applied to this query segment */
    const QSet<QString>& getLabels() const;
    /** Apply a label to this query segment */
    void setLabel(const QString& label);
    /** Remove label from this query segment */
    void removeLabel(const QString& label);
    void clearLabel();
    /** Check if it has the given label applied to it */
    bool hasLabel(const QString& label);

    /** Set plugin name of this query
    This can be used to override the owner of the selected catalog item, so that
    no matter what item is chosen from the catalog, the given plugin will be the one
    to execute it.
    \param i The name of the plugin to execute the query's best match from the catalog
    */
    void setPlugin(const QString& plugin);
    /** Returns the current owner of the query */
    const QString& getPlugin() const;

    /** Get the text of the query segment */
    const QString& getText() const;

    /** Set the text of the query segment */
    void setText(const QString& t);

    /** Check if the text is empty */
    bool hasText() const;

    /** Get a pointer to the best catalog match for this segment of the query */
    CatItem& getTopResult();
    const CatItem& getTopResult() const;

    /** Change the best catalog match for this segment */
    void setTopResult(const CatItem& sr);

    friend LAUNCHY_EXPORT QDataStream& operator<<(QDataStream& out,
                                                  const InputData& inputData);
    friend LAUNCHY_EXPORT QDataStream& operator>>(QDataStream& in,
                                                  InputData& inputData);

private:
    /** The user's entry */
    QString m_text;
    /** Any assigned labels to this query segment */
    QSet<QString> m_labels;
    /** A pointer to the best catalog match for this segment of the query */
    CatItem m_topResult;
    /** The plugin name of this query's owner */
    QString m_pluginName;
};

} // namespace launchy

