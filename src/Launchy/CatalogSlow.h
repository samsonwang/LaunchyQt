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

#include <QVector>
#include <QHash>
#include <QMutex>

#include "Catalog.h"
#include "LaunchyLib/CatalogItem.h"

// These classes do not pertain to plugins

namespace launchy {
/** This class does not pertain to plugins */
// The slow catalog searches slowly but
// adding items is fast and uses less memory
// than CatalogFast
class CatalogSlow : public Catalog {
public:
    CatalogSlow();
    virtual int count();
    virtual void clear();
    virtual void addItem(const CatItem& item);
    virtual void purgeOldItems();

    virtual void incrementUsage(const CatItem& item);
    virtual void demoteItem(const CatItem& item);

protected:
    virtual const CatItem& getItem(int i);
    virtual QList<CatItem*> search(const QString& searchText);

private:
    QVector<CatalogItem> m_catalogItems;
    // fullPath + shortName (the fields compared by CatItem::operator==) ->
    // position in m_catalogItems. Lets addItem() find and replace an existing
    // item without scanning the whole catalog during a rebuild, which used to
    // take seconds for tens of thousands of items. Must be updated whenever
    // m_catalogItems changes (see purgeOldItems)
    QHash<QString, int> m_index;
};

} // namespace launchy
