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

#include "ExportPy.h"
#include <iostream>
#include <QHash>
#include "ExportPyPlugin.h"
#include "ExportPyCatItem.h"
#include "PluginMgr.h"
#include "PluginInterface.h"
#include "LaunchyLib.h"

static int add_five(int x) {
    return x+5;
}

PYBIND11_MODULE(launchy, m) {
    m.doc() = "launchy plugin module for python";
    // Export our basic testing function
    m.def("add_five", &add_five, "function which increase number by 5");

    m.def("hash", &exportpy::hash, "hash function from qt");

    m.def("registerPlugin", &exportpy::registerPlugin);

    //
    m.def("getAppPath", &exportpy::getAppPath,
          "get launchy application path",
          py::arg("toNative") = true);

    m.def("runProgram", &exportpy::runProgram,
          "run program by launchy");

    m.def("setNeedRebuildCatalog", &exportpy::setNeedRebuildCatalog,
          "set need rebuild catalog after settings changed");

    // for testing
    m.def("objectReceiver", &exportpy::objectReceiver);

    exportpy::ExportPlugin(m);
    exportpy::ExportCatItem(m);
    exportpy::ExportInputData(m);
}

namespace exportpy {

void registerPlugin(py::object pluginClass) {
    //std::cout << "registerPlugin called" << std::endl;
    qDebug() << "exportpy::registerPlugin, register plugin called";

    pluginpy::PluginMgr& mgr = pluginpy::PluginMgr::instance();
    mgr.registerPlugin(pluginClass);

    /*
    py::object pluginObj = pluginClass();

    if (py::isinstance<Plugin>(pluginObj)) {
        std::cout << "plugin register succeeded" << std::endl;
        // qDebug() << "exportpy::registerPlugin, plugin register succeed";
        Plugin* pluginPtr = pluginObj.cast<Plugin*>();
        if (pluginPtr) {
            std::string name = pluginPtr->getName();
            std::cout << "registered plugin name:" << name << std::endl;
         //   qDebug() << "exportpy::registerPlugin, plugin name:" << name.c_str();
        }
  //      pluginpy::PluginMgr& mgr = pluginpy::PluginMgr::instance();
//        mgr.registerPlugin(pluginClass);
    }
    else {
        std::cout << "plugin register failed" << std::endl;
        // qDebug() << "exportpy::registerPlugin, plugin register failed";
    }
    */
}

unsigned int hash(const std::string& str) {
    QString qstr = QString::fromStdString(str);
    return qHash(qstr);
}

std::string getAppPath(bool toNative) {
    QString path = qApp->applicationDirPath();
    if (toNative) {
        path = QDir::toNativeSeparators(path);
    }
    return path.toStdString();
}

void runProgram(const std::string& file, const std::string& args) {
    QString fileQ = QString::fromStdString(file);
    QString argsQ = QString::fromStdString(args);
    qDebug() << "exportpy::runProgram, file:" << file.c_str()
        << "args:" << args.c_str();
    launchy::runProgram(fileQ, argsQ);
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

}
