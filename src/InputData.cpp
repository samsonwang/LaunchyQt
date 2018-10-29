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

#include "precompiled.h"
#include "InputData.h"
#include "GlobalVar.h"

QSet<uint> InputData::getLabels() {
    return labels;
}

void InputData::setLabel(uint l) {
    labels.insert(l);
}

void InputData::removeLabel(uint l) {
    labels.remove(l);
}

bool InputData::hasLabel(uint l) {
    return labels.contains(l);
}

void InputData::setID(uint i) {
    id = i;
}

uint InputData::getID() const {
    return id;
}

const QString& InputData::getText() const {
    return text;
}

void InputData::setText(const QString& t) {
    text = t;
}

bool InputData::hasText() const {
    return !text.isEmpty();
}

CatItem& InputData::getTopResult() {
    return const_cast<CatItem&>(static_cast<const InputData*>(this)->getTopResult());
}

const CatItem& InputData::getTopResult() const {
    return topResult;
}

void InputData::setTopResult(const CatItem& sr) {
    topResult = sr;
}

InputData::InputData() {
    id = 0;
}

InputData::InputData(const QString& str)
    : text(str) {
    id = 0;
}

QDataStream &operator<<(QDataStream &out, const InputData &inputData) {
    out << inputData.text;
    out << inputData.labels;
    out << inputData.topResult;
    out << inputData.id;
    return out;
}

QDataStream &operator>>(QDataStream &in, InputData &inputData) {
    in >> inputData.text;
    in >> inputData.labels;
    in >> inputData.topResult;
    in >> inputData.id;
    return in;
}

void InputDataList::parse(const QString& text) {
	if (text.isEmpty()) {
		clear();
	}
	else {
		QStringList split = text.split(QString(" ") + QChar(0x25ba) + " ");

		// Truncate inputData to the same length as the new input text
		if (split.count() < count()) {
			erase(begin() + split.count(), end());
		}

		// Truncate to the first different entry
		for (int i = 0; i < count(); ++i) {
			(*this)[i].removeLabel(LABEL_HISTORY);
			if (at(i).getText() != split[i]) {
				erase(begin() + i, end());
				break;
			}
		}

		// And add anything new
		for (int i = count(); i < split.count(); i++) {
			InputData data(split[i]);
			push_back(data);
		}
	}
}


QString InputDataList::toString(bool omitLast) const {
	QString result = "";
	for (int i = 0; i < count(); ++i) {
		if (i > 0)
			result += QString(" ") + QChar(0x25ba) + " ";
		if (!omitLast || i < count()-1)
			result += at(i).getText();
	}
	return result;
}
