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

namespace py = pybind11;

namespace exportpy {

InputData::InputData()
    : m_id(0) {

}

InputData::InputData(const std::string& str)
    : m_text(str),
      m_id(0){

}

InputData::InputData(const launchy::InputData& data) {
    m_text = data.getText().toStdString();
    foreach(uint l, data.getLabels()) {
        m_labels.insert(l);
    }
    m_topResult = data.getTopResult();
    m_id = data.getID();
}

const std::set<unsigned int>& InputData::getLabels() const {
    return m_labels;
}

void InputData::setLabel(unsigned int l) {
    m_labels.insert(l);
}

void InputData::removeLabel(unsigned int l) {
    m_labels.erase(l);
}

bool InputData::hasLabel(unsigned int l) {
    return m_labels.count(l) > 0;
}

void InputData::setID(unsigned int i) {
    m_id = i;
}

unsigned int InputData::getID() const {
    return m_id;
}

const std::string& InputData::getText() const {
    return m_text;
}

void InputData::setText(const std::string& t) {
    m_text = t;
}

bool InputData::hasText() const {
    return !m_text.empty();
}

CatItem& InputData::getTopResult() {
    return m_topResult;
    //return const_cast<CatItem&>(static_cast<const InputData*>(this)->getTopResult());
}

// const CatItem& InputData::getTopResult() const {
//     return m_topResult;
// }

void InputData::setTopResult(const CatItem& sr) {
    m_topResult = sr;
}


void ExportInputData(const py::module& m) {

    py::class_<exportpy::InputData>(m, "InputData")
        .def(py::init<>())
        .def(py::init<const std::string&>())
        .def("getLabels", &exportpy::InputData::getLabels)
        .def("setLabel", &exportpy::InputData::setLabel)
        .def("hasLabel", &exportpy::InputData::hasLabel)
        .def("setID", &exportpy::InputData::setID)
        .def("getID", &exportpy::InputData::getID)
        .def("getText", &exportpy::InputData::getText)
        .def("setText", &exportpy::InputData::setText)
        .def("getTopResult", &exportpy::InputData::getTopResult)
        .def("setTopResult", &exportpy::InputData::setTopResult);

}

}
