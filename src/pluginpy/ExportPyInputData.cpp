/*
PluginPy
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

#include "ExportPyInputData.h"
#include "InputData.h"
#include "ExportPyCatItem.h"

namespace py = pybind11;

namespace exportpy {

InputData::InputData(launchy::InputData* data)
    : m_data(data) {

}

void InputData::setLabel(unsigned int l) {
    m_data->setLabel(l);
}

void InputData::removeLabel(unsigned int l) {
    m_data->removeLabel(l);
}

bool InputData::hasLabel(unsigned int l) {
    return m_data->hasLabel(l);
}

void InputData::setID(unsigned int i) {
    m_data->setID(i);
}

unsigned int InputData::getID() const {
    return m_data->getID();
}

std::string InputData::getText() const {
    return m_data->getText().toStdString();
}

void InputData::setText(const std::string& t) {
    m_data->setText(QString::fromStdString(t));
}

bool InputData::hasText() const {
    return m_data->hasText();
}

CatItem InputData::getTopResult() {
    launchy::CatItem& item = m_data->getTopResult();
    return CatItem(item);
}

void ExportInputData(const py::module& m) {

    py::class_<exportpy::InputData>(m, "InputData")
        .def("setLabel", &exportpy::InputData::setLabel)
        .def("removeLabel", &exportpy::InputData::removeLabel)
        .def("hasLabel", &exportpy::InputData::hasLabel)
        .def("setID", &exportpy::InputData::setID)
        .def("getID", &exportpy::InputData::getID)
        .def("getText", &exportpy::InputData::getText)
        .def("setText", &exportpy::InputData::setText)
        .def("getTopResult", &exportpy::InputData::getTopResult);

}

}
