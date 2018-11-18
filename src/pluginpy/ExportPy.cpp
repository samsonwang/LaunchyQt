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

#include <pybind11/pybind11.h>
#include "ExportPyPlugin.h"
#include "ExportPyCatItem.h"

static int add_five(int x) {
    return x+5;
}

PYBIND11_MODULE(launchy, m) {
    m.doc() = "launchy plugin module for python";
    // Export our basic testing function
    m.def("add_five", &add_five, "function which increase 5");

    exportpy::ExportCatItem(m);
    exportpy::ExportPlugin(m);
    //python_export::export_QString();
    //python_export::export_pylaunchy();
    //python_export::export_catalog();
    //python_export::export_ScriptPlugin();
}

// pybind11 demo module
static int add(int i, int j) {
    return i + j;
}

PYBIND11_MODULE(example, m) {
    m.doc() = "pybind11 example plugin"; // optional module docstring

    m.def("add", &add, "A function which adds two numbers");
}