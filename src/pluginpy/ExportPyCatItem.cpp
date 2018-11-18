/*
LaunchyQt
Copyright (C) 2018 Samson Wang

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

#include "ExportPyCatItem.h"

namespace py = pybind11;

namespace exportpy {

CatItem::CatItem(const std::string& full,
                 const std::string& shortN,
                 int pluginId,
                 const std::string& iconPath)
    : fullPath(full),
    shortName(shortN),
    icon(iconPath),
    id(pluginId) {
}

void ExportCatItem(const pybind11::module& m) {

    py::class_<exportpy::CatItem>(m, "CatItem")
        .def(py::init<const std::string&, const std::string&, int, const std::string&>())
        .def_readwrite("fullPath", &exportpy::CatItem::fullPath)
        .def_readwrite("shortName", &exportpy::CatItem::shortName)
        .def_readwrite("icon", &exportpy::CatItem::icon);

}

}

