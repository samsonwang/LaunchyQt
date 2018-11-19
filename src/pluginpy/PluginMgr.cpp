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

#include "PluginMgr.h"
#include <pybind11/embed.h>
#include <pybind11/eval.h>
#include "ExportPyPlugin.h"
#include "PluginWrapper.h"

namespace py = pybind11;

namespace pluginpy {

pluginpy::PluginMgr& PluginMgr::instance() {
    static PluginMgr s_mgr;
    return s_mgr;
}

launchy::PluginInterface* PluginMgr::loadPlugin(const QString& pluginName, const QString& pluginPath) {
    qDebug() << "pluginpy::PluginMgr::loadPlugin, name:" << pluginName
        << "path:" << pluginPath;

    // add dir to os path
    QString pluginFileName = pluginPath + "/" + pluginName + ".py";
    pluginFileName = QDir::toNativeSeparators(pluginFileName);
    std::string fileName = pluginFileName.toLocal8Bit().data();
    
    //py::scoped_interpreter guard{};
    py::initialize_interpreter();
    // import plugin module or execute plugin file
    py::module sys = py::module::import("sys");
    sys.attr("path");
    //py::object mainModule = py::module::import("__main__");
    //py::object mainScope = mainModule.attr("__dict__");
    //py::eval_file(fileName, mainScope);

    py::object mod = py::module::import("CalcyPy");
    py::object pluginClass = mod.attr("getPlugin")();

    uint id = qHash(pluginName);
    m_pluginClass.insert(id, pluginClass());


    py::object& pluginObject = m_pluginClass[id];

    if (py::isinstance<exportpy::Plugin>(pluginObject)) {
        qDebug() << "PluginMgr::loadPlugin, plugin load succeed";
        exportpy::Plugin* pluginPtr = pluginObject.cast<exportpy::Plugin*>();
        if (pluginPtr) {
            std::string name = pluginPtr->getName();
            uint id = pluginPtr->getID();
            //std::cout << "registered plugin name:" << name << std::endl;
            qDebug() << "exportpy::registerPlugin, plugin name:" << name.c_str();
            launchy::PluginInterface* intf = new pluginpy::PluginWrapper(pluginPtr);
            return intf;
        }
        //pluginpy::PluginMgr& mgr = pluginpy::PluginMgr::instance();
        //mgr.registerPlugin(pluginClass);
    }


    //py::scoped_interpreter guard{};
    //py::module sys = py::module::import("sys");
    //py::module plugin = py::module::import("CalcyPy");
    //auto syspath = sys.attr("path");
    //py::print(py::str(syspath));
//    py::print(sys.attr("path"));
    //auto syspath = sys.attr("path");
    

    return nullptr;
}

bool PluginMgr::unloadPlugin(uint pluginId) {
    qDebug() << "pluginpy::PluginMgr::unloadPlugin, id:" << pluginId;

    return true;
}

void PluginMgr::registerPlugin(py::object pluginClass) {
    qDebug() << "PluginMgr::registerPlugin, register plugin called";


}

PluginMgr::PluginMgr() {

}

}
