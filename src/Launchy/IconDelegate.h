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

#pragma once

#include <QStyledItemDelegate>
#include <QLabel>
#include <QString>

#define ROLE_SHORT Qt::DisplayRole
#define ROLE_FULL Qt::ToolTipRole
#define ROLE_ICON Qt::DecorationRole

namespace launchy {
class IconDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    IconDelegate(QObject *parent = 0);

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option,
                       const QModelIndex &index) const;

    virtual QSize sizeHint(const QStyleOptionViewItem &option,
                           const QModelIndex &index) const;

    void setSize(int s);
    // Text the drawn row is decorated against (Catalog::decorateText). It is a
    // drawing input like the size and the alternative-path label, so it arrives
    // through a setter instead of a global: paint() runs on any repaint, long
    // after the query that produced the rows.
    void setSearchText(const QString& text);
    void setAlternativePathWidget(QLabel* label);

private:
    int m_size;
    QString m_searchText;
    QLabel* m_alternativesPath;
};
}
