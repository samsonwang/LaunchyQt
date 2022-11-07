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

namespace exportpy {

CatItem::CatItem(const std::string& full,
                 const std::string& shortN,
                 const std::string& plugin,
                 const std::string& iconPath) {

    m_data = launchy::CatItem(QString::fromStdString(full),
                              QString::fromStdString(shortN),
                              QString::fromStdString(plugin),
                              QString::fromStdString(iconPath));
}

CatItem::CatItem(const launchy::CatItem& item)
    : m_data(item) {
}

const launchy::CatItem& CatItem::getData() const {
    return m_data;
}

std::string CatItem::fullPath() const {
    return m_data.fullPath.toStdString();
}

std::string CatItem::shortName() const {
    return m_data.shortName.toStdString();
}

std::string CatItem::iconPath() const {
    return m_data.iconPath.toStdString();
}

void CatItem::setUsage(int usage) {
    m_data.usage = usage;
}

CatItemList::CatItemList(QList<launchy::CatItem>* data)
    : m_data(data) {
}

void CatItemList::append(const CatItem& item) {
    m_data->push_back(item.getData());
}

void CatItemList::prepend(const CatItem& item) {
    m_data->push_front(item.getData());
}

void CatItemList::push_front(const CatItem& item) {
    m_data->push_front(item.getData());
}

void CatItemList::push_back(const CatItem& item) {
    m_data->push_back(item.getData());
}

} // namespace exportpy

