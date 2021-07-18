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

#include <Python.h>
#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include <pybind11/embed.h>
#include <pybind11/eval.h>

namespace py = pybind11;

namespace exportpy {

unsigned int hash(const std::string& str);

std::string getAppPath(bool toNative = true);

std::string getAppTempPath(bool toNative = true);

void runProgram(const std::string& file, const std::string& args);

void setNeedRebuildCatalog();

// for debug and test
void objectReceiver(py::object obj);

} // namespace exportpy
