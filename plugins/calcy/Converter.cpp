/*
Calcy - plugin for LaunchyQt
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

#include "Converter.h"

bool Converter::octStr(qlonglong num, QString& reslut) {
    reslut = "0" + QString::number(num, 8);
    return true;
}

bool Converter::decStr(qlonglong num, QString& reslut) {
    reslut = QString::number(num, 10);
    return true;
}

bool Converter::hexStr(qlonglong num, QString& reslut) {
    reslut = "0x" + QString::number(num, 16);
    return true;
}

bool Converter::binStr(qlonglong num, QString& reslut) {
    reslut = QString::number(num, 2);
    return true;
}