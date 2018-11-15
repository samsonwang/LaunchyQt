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
struct Pet {
    Pet(const std::string &name) : name(name) { }
    void setName(const std::string &name_) { name = name_; }
    const std::string &getName() const { return name; }

    std::string name;
};
PYBIND11_MODULE(example, m) {
    py::class_<Pet>(m, "Pet")
        .def(py::init<const std::string &>())
        .def("setName", &Pet::setName)
        .def("getName", &Pet::getName);
}


namespace exportpy {
void ExportCatItem(const pybind11::module& m) {

    py::class_<CatItem>(m, "Pet")
        .def(py::init<const std::string &>())
        .def("setName", &Pet::setName)
        .def("getName", &Pet::getName);

    py::class_<CatItem>(m, "CatItem")
        .def(py::init<const QString&, const QString&, uint, const QString&>())

        BOOST_PYTHON_EXPORT_CUSTOM_TYPE("fullPath", &CatItem::fullPath)
        BOOST_PYTHON_EXPORT_CUSTOM_TYPE("shortName", &CatItem::shortName)
        BOOST_PYTHON_EXPORT_CUSTOM_TYPE("lowName", &CatItem::lowName)
        BOOST_PYTHON_EXPORT_CUSTOM_TYPE("icon", &CatItem::icon)

        .def_readwrite("usage", &CatItem::usage)
        .def_readwrite("id", &CatItem::id);


    class_<CatItem>("CatItem", init<>())
        .def(init<QString, QString, unsigned int, QString>())

        BOOST_PYTHON_EXPORT_CUSTOM_TYPE("fullPath", &CatItem::fullPath)
        BOOST_PYTHON_EXPORT_CUSTOM_TYPE("shortName", &CatItem::shortName)
        BOOST_PYTHON_EXPORT_CUSTOM_TYPE("lowName", &CatItem::lowName)
        BOOST_PYTHON_EXPORT_CUSTOM_TYPE("icon", &CatItem::icon)

        .def_readwrite("usage", &CatItem::usage)
        .def_readwrite("id", &CatItem::id)
        ;
}

}

