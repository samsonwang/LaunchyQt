/*
LaunchyQt
Copyright (C) 2019 Samson Wang

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "InputDataList.h"
#include "GlobalVar.h"

namespace launchy {

QString InputDataList::s_separator = QString(" ") + QChar(0x25ba) + QString(" ");

void InputDataList::setSeparator(const QString& sepa) {
    s_separator = sepa;
}

void InputDataList::parse(const QString& text) {
    if (text.isEmpty()) {
        clear();
    }
    else {
        QStringList split = text.split(s_separator);

        // Truncate inputData to the same length as the new input text
        if (split.count() < count()) {
            erase(begin() + split.count(), end());
        }

        // Truncate to the first different entry
        for (int i = 0; i < count(); ++i) {
            (*this)[i].clearLabel();
            if (at(i).getText() != split[i]) {
                erase(begin() + i, end());
                break;
            }
        }

        // And add anything new
        for (int i = count(); i < split.count(); ++i) {
            InputData data(split[i]);
            push_back(data);
        }
    }
}

QString InputDataList::toString(bool omitLast) const {
    QString result;
    for (int i = 0; i < count(); ++i) {
        if (i > 0) {
//             if (at(i).getText().isEmpty()) {
//                 continue;
//             }
            result += s_separator;
        }
        if (!omitLast || i < count()-1) {
            result += at(i).getText();
        }
    }
    return result;
}

} // namespace launchy

