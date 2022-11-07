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

#include "LaunchyLib/InputData.h"

#include "ExportPyCatItem.h"

namespace exportpy {

InputData::InputData(launchy::InputData* data)
    : m_data(data) {

}

void InputData::setLabel(const std::string& label) {
    m_data->setLabel(QString::fromStdString(label));
}

void InputData::removeLabel(const std::string& label) {
    m_data->removeLabel(QString::fromStdString(label));
}

bool InputData::hasLabel(const std::string& label) {
    return m_data->hasLabel(QString::fromStdString(label));
}

void InputData::setPlugin(const std::string& plugin) {
    m_data->setPlugin(QString::fromStdString(plugin));
}

std::string InputData::getPlugin() const {
    return m_data->getPlugin().toStdString();
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

} // namespace exportpy
