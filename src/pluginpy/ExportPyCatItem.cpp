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
#include "CatalogItem.h"

namespace py = pybind11;

namespace exportpy {

CatItem::CatItem(const std::string& full,
                 const std::string& shortN,
                 unsigned int pluginId,
                 const std::string& iconPath)
    : fullPath(full),
    shortName(shortN),
    icon(iconPath),
    id(pluginId),
    usage(0),
    data(nullptr) {

    m_data.fullPath = QString::fromStdString(full);
}

CatItem::CatItem()
    : usage(0),
      data(nullptr),
      id(0) {

}

CatItem::CatItem(const launchy::CatItem& item)
    : fullPath(item.fullPath.toStdString()),
      shortName(item.shortName.toStdString()),
      lowName(item.lowName.toStdString()),
      icon(item.icon.toStdString()),
      usage(item.usage),
      data(item.data),
      id(item.id) {

}

CatItem& CatItem::operator=(const launchy::CatItem& item) {
    fullPath = item.fullPath.toStdString();
    shortName = item.shortName.toStdString();
    lowName = item.lowName.toStdString();
    icon = item.icon.toStdString();
    usage = item.usage;
    data = item.data;
    id = item.id;
    return *this;
}

void ExportCatItem(const py::module& m) {

    py::class_<exportpy::CatItem>(m, "CatItem")
        .def(py::init<const std::string&, const std::string&, unsigned int, const std::string&>())
        .def_readwrite("fullPath", &exportpy::CatItem::fullPath)
        .def_readwrite("shortName", &exportpy::CatItem::shortName)
        .def_readwrite("icon", &exportpy::CatItem::icon);

    py::class_<exportpy::CatItemList>(m, "CatItemList")
        //.def(py::init<>())
        .def("append", &exportpy::CatItemList::append)
        .def("prepend", &exportpy::CatItemList::prepend)
        .def("push_front", &exportpy::CatItemList::push_front)
        .def("push_back", &exportpy::CatItemList::push_back);
}

CatItemList::CatItemList(QList<launchy::CatItem>* data)
    : m_data(data) {
}

void CatItemList::append(const CatItem& item) {
//     launchy::CatItem temp;
//     temp.fullPath = item.m_data;
//     //m_head->push_front(item);
    m_data->push_back(item.m_data);
}

void CatItemList::prepend(const CatItem& item) {
    m_data->push_front(item.m_data);
    //m_rear->push_back(item);
}

void CatItemList::push_front(const CatItem& item) {
    m_data->push_front(item.m_data);
    //m_head->push_front(item);
}

void CatItemList::push_back(const CatItem& item) {
    m_data->push_back(item.m_data);
    //m_rear->push_back(item);
}

}

