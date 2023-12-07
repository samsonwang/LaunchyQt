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

#include <iostream>

#include <Python.h>

#include <pybind11/pybind11.h>
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include <pybind11/embed.h>
#include <pybind11/eval.h>

#include <QDebug>
#include <QApplication>
#include <QDir>

#include "LaunchyLib/PluginInterface.h"
#include "LaunchyLib/LaunchyLib.h"

#include "ExportPyPlugin.h"
#include "ExportPyCatItem.h"
#include "PluginMgr.h"

namespace py = pybind11;

static int add_five(int x) {
    return x+5;
}

namespace exportpy {

std::string getAppPath(bool toNative) {
    QString path = qApp->applicationDirPath();
    if (toNative) {
        path = QDir::toNativeSeparators(path);
    }
    return path.toStdString();
}

std::string getAppTempPath(bool toNative) {
    QString path = QDir::tempPath() + QString("/Launchy");
    if (toNative) {
        path = QDir::toNativeSeparators(path);
    }
    return path.toStdString();
}

void runProgram(const std::string& file, const std::string& args) {
    qDebug() << "exportpy::runProgram, file:" << file.c_str()
        << "args:" << args.c_str();

    launchy::runProgram(QString::fromStdString(file),
                        QString::fromStdString(args));
}

void setNeedRebuildCatalog() {
    ++launchy::g_needRebuildCatalog;
}

void objectReceiver(py::object obj) {
    if (py::isinstance<exportpy::CatItem>(obj)) {
        std::cout << "objectReceiver, got CatItem" << std::endl;
        CatItem item = py::cast<CatItem>(obj);
//         std::cout << "fullPath:" << item.fullPath << std::endl;
//         std::cout << "shortName" << item.shortName << std::endl;
//         std::cout << "icon" << item.icon << std::endl;
    }

    else if (py::isinstance<exportpy::Plugin*>(obj)) {
        std::cout << "objectReceiver, got Plugin ptr" << std::endl;
    }

    else if (py::isinstance<exportpy::Plugin>(obj)) {
        std::cout << "objectReceiver, got Plugin" << std::endl;
    }

    else {
        std::cout << "objectReceiver, got unknown" << std::endl;
    }
}

} // namespace exportpy


PYBIND11_MODULE(launchy, m) {
    m.doc() = "launchy plugin module for python";

    // Export our basic testing function
    m.def("add_five", &add_five, "function which increase number by 5");

    m.def("getAppPath", &exportpy::getAppPath,
          "get launchy application path",
          py::arg("toNative") = true);

    m.def("getAppTempPath", &exportpy::getAppTempPath,
          "get launchy application temp path",
          py::arg("toNative") = true);

    m.def("runProgram", &exportpy::runProgram,
          "run program by launchy");

    m.def("setNeedRebuildCatalog", &exportpy::setNeedRebuildCatalog,
          "set need rebuild catalog after settings changed");

    // for testing
    // m.def("objectReceiver", &exportpy::objectReceiver);

    py::class_<exportpy::Plugin, exportpy::PluginHelper>(m, "Plugin")
        .def(py::init<>())
        .def("init", &exportpy::Plugin::init)
        .def("getName", &exportpy::Plugin::getName)
        .def("setPath", &exportpy::Plugin::setPath)
        .def("getLabels", &exportpy::Plugin::getLabels)
        .def("getResults", &exportpy::Plugin::getResults)
        .def("getCatalog", &exportpy::Plugin::getCatalog)
        .def("launchItem", &exportpy::Plugin::launchItem)
        .def("hasDialog", &exportpy::Plugin::hasDialog)
        .def("doDialog", &exportpy::Plugin::doDialog)
        .def("endDialog", &exportpy::Plugin::endDialog)
        .def("launchyShow", &exportpy::Plugin::launchyShow)
        .def("launchyHide", &exportpy::Plugin::launchyHide);

    py::class_<exportpy::CatItem>(m, "CatItem")
        .def(py::init<const std::string&, const std::string&,
             const std::string&, const std::string&>())
        .def("fullPath", &exportpy::CatItem::fullPath)
        .def("shortName", &exportpy::CatItem::shortName)
        .def("iconPath", &exportpy::CatItem::iconPath)
        .def("setUsage", &exportpy::CatItem::setUsage);

    py::class_<exportpy::CatItemList>(m, "CatItemList")
        //.def(py::init<>())
        .def("append", &exportpy::CatItemList::append)
        .def("prepend", &exportpy::CatItemList::prepend)
        .def("push_front", &exportpy::CatItemList::push_front)
        .def("push_back", &exportpy::CatItemList::push_back);

    py::class_<exportpy::InputData>(m, "InputData")
        .def("setLabel", &exportpy::InputData::setLabel)
        .def("removeLabel", &exportpy::InputData::removeLabel)
        .def("hasLabel", &exportpy::InputData::hasLabel)
        .def("setPlugin", &exportpy::InputData::setPlugin)
        .def("getPlugin", &exportpy::InputData::getPlugin)
        .def("getText", &exportpy::InputData::getText)
        .def("setText", &exportpy::InputData::setText)
        .def("getTopResult", &exportpy::InputData::getTopResult);
}