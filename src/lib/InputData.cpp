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

#include "InputData.h"
#include <QDataStream>

namespace launchy {
const QSet<uint>& InputData::getLabels() const {
    return m_labels;
}

void InputData::setLabel(uint l) {
    m_labels.insert(l);
}

void InputData::removeLabel(uint l) {
    m_labels.remove(l);
}

void InputData::clearLabel() {
    m_labels.clear();
}

bool InputData::hasLabel(uint l) {
    return m_labels.contains(l);
}

void InputData::setID(uint i) {
    m_id = i;
}

uint InputData::getID() const {
    return m_id;
}

const QString& InputData::getText() const {
    return m_text;
}

void InputData::setText(const QString& t) {
    m_text = t;
}

bool InputData::hasText() const {
    return !m_text.isEmpty();
}

CatItem& InputData::getTopResult() {
    return const_cast<CatItem&>(static_cast<const InputData*>(this)->getTopResult());
}

const CatItem& InputData::getTopResult() const {
    return m_topResult;
}

void InputData::setTopResult(const CatItem& sr) {
    m_topResult = sr;
}

InputData::InputData()
    : m_id(0) {
}

InputData::InputData(const QString& str)
    : m_text(str),
    m_id(0) {
}

QDataStream& operator<<(QDataStream& out, const InputData& inputData) {
    out << inputData.m_text;
    out << inputData.m_labels;
    out << inputData.m_topResult;
    out << inputData.m_id;
    return out;
}

QDataStream& operator>>(QDataStream& in, InputData& inputData) {
    in >> inputData.m_text;
    in >> inputData.m_labels;
    in >> inputData.m_topResult;
    in >> inputData.m_id;
    return in;
}
}