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

#include "CatalogItem.h"
#include "InputDataList.h"

class CommandHistory {
public:
    CommandHistory();

    bool load(const QString& filename);
    void save(const QString& filename) const;

    void addItem(const InputDataList& item);
    void removeAt(int index);
    const InputDataList& getItem(int index);
    void search(const QString& searchText, QList<CatItem>& searchResults) const;

private:
    QLinkedList<InputDataList> m_history;
};
