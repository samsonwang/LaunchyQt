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

#pragma once

#include <string>
#include <pybind11/pybind11.h>

namespace py = pybind11;

namespace exportpy {

void registerPlugin(py::object pluginClass);

unsigned int hash(const std::string& str);

std::string getAppPath(bool toNative = true);

void runProgram(const std::string& file, const std::string& args);

void setNeedRebuildCatalog();

// for debu and test use
void objectReceiver(py::object obj);
}
