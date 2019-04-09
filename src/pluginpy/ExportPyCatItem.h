/*
SmartLaunch: multi-function app launcher.
Copyright (C) 2017 Samson Wang

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <pybind11/pybind11.h>
#include "CatalogItem.h"

//namespace launchy { class CatItem; }

namespace py = pybind11;

namespace exportpy {

void ExportCatItem(const py::module& m);

class CatItem {
public:
    CatItem() = default;
    CatItem(const launchy::CatItem& item);

    /** This is the constructor most used by plugins
    \param full The full path of the file to execute
    \param shortN The abbreviated name for the entry
    \param i_d Your plugin id (0 for Launchy itself)
    \param iconPath The path to the icon for this entry
    \warning It is usually a good idea to append ".your_plugin_name" to the end of the full parameter
    so that there are not multiple items in the index with the same full path.
    */
    CatItem(const std::string& full,
            const std::string& shortN,
            unsigned int pluginId,
            const std::string& iconPath);

    const launchy::CatItem& getData() const;

    std::string fullPath() const;
    std::string shortName() const;
    std::string iconPath() const;
    void setUsage(int usage);

private:
    launchy::CatItem m_data;
};


class CatItemList {
public:
    CatItemList(QList<launchy::CatItem>* data);

    void append(const CatItem& item);
    void prepend(const CatItem& item);
    void push_front(const CatItem& item);
    void push_back(const CatItem& item);

private:
    QList<launchy::CatItem>* m_data;
};

}
