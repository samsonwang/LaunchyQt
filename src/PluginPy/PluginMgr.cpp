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

#include <QDebug>
#include <QDir>
#include <QApplication>

#include "LaunchyLib/LaunchyLib.h"
#include "LaunchyLib/InputData.h"

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

    if (m_pluginInterface.contains(pluginName)) {
        qDebug() << "pluginpy::PluginMgr::loadPlugin, plugin alread loaded, name:"
            << pluginName << "path:" << pluginPath;
        return m_pluginInterface[pluginName];
    }

    if (!m_pluginObject.contains(pluginName)) {
        py::list pathObj = py::module::import("sys").attr("path").cast<py::list>();
        pathObj.append(qUtf8Printable(QDir::toNativeSeparators(pluginPath)));
        py::object module = py::module::import(qUtf8Printable(pluginName));
        py::object pluginClass = module.attr("getPlugin")();

        m_pluginObject.insert(pluginName, pluginClass());
    }

    py::object& pluginObject = m_pluginObject[pluginName];

    if (py::isinstance<exportpy::Plugin>(pluginObject)) {
        qDebug() << "pluginpy::PluginMgr::loadPlugin, plugin load succeed, plugin name:"
            << pluginName;
        exportpy::Plugin* pluginPtr = pluginObject.cast<exportpy::Plugin*>();
        if (pluginPtr) {
            std::string name = pluginPtr->getName();
            qDebug() << "pluginpy::PluginMgr::loadPlugin, plugin name:"
                << name.c_str() << pluginName;
            launchy::PluginInterface* intf = new pluginpy::PluginWrapper(pluginPtr, pluginName);
            // store this pointer in manager
            m_pluginInterface.insert(pluginName, intf);
            return intf;
        }
    }

    return nullptr;
}

bool PluginMgr::unloadPlugin(const QString& pluginName) {
    qDebug() << "pluginpy::PluginMgr::unloadPlugin, name:" << pluginName;

    launchy::PluginInterface* plugin = m_pluginInterface[pluginName];
    if (plugin) {
        delete plugin;
        plugin = nullptr;
    }

    m_pluginInterface.remove(pluginName);
    m_pluginObject.remove(pluginName);

    return true;
}

void PluginMgr::initSettings(QSettings* setting) {
    if (!setting) {
        qWarning("pluginpy::PluginMgr::initSettings, setting is nullptr");
        return;
    }

    // avoid multiple pybind11 import
    if (setting == m_pSettings) {
        qDebug("pluginpy::PluginMgr::initSettings, setting is already set");
        return;
    }
    m_pSettings = setting;

    try {
        // init qsetting
        py::object launchyModule = py::module::import("launchy");
        py::object launchyDict = launchyModule.attr("__dict__");
        PyObject* settingPyObj = PyLong_FromVoidPtr(m_pSettings);
        launchyDict["__settings"] = py::handle(settingPyObj);

        // run launchy_util initSettings
        qDebug() << "pluginpy::PluginMgr::initSettings, import launchy_util";
        py::object launchyUtilModule = py::module::import("launchy_util");
        py::object launchyUtilInitSettings = launchyUtilModule.attr("initSettings");
        launchyUtilInitSettings();
    }
    catch (const py::error_already_set& e) {
        PyErr_Print();
        PyErr_Clear();
        qWarning() << "pluginpy::PluginMgr::initSettings, fail to init QSetting,"
            << e.what();
    }
}

void PluginMgr::registerPlugin(py::object pluginClass) {
    qDebug() << "pluginpy::PluginMgr::registerPlugin, register plugin called";
    //m_pluginClass.push_back(pluginClass);
}

PluginMgr::PluginMgr()
    : m_pSettings(nullptr) {

    py::initialize_interpreter();

    QString pythonPkgPath = qApp->applicationDirPath() + "/python";
    py::list pathObj = py::module::import("sys").attr("path").cast<py::list>();
    pathObj.append(qUtf8Printable(QDir::toNativeSeparators(pythonPkgPath)));

    // load system pypi package path
    QString externalPkgPath = launchy::g_settings->value("Python/ExternalPackagePath", "").toString();
    if (!externalPkgPath.isEmpty()) {
        pathObj.append(qUtf8Printable(QDir::toNativeSeparators(externalPkgPath)));
        qDebug() << "pluginpy::PluginMgr::PluginMgr, external package path:"
            << externalPkgPath;
    }

    try {
        // run launchy_util main (init logging and pypi package)
        qDebug() << "pluginpy::PluginMgr::PluginMgr, import launchy_util";
        py::object launchyUtilModule = py::module::import("launchy_util");
        py::object launchyUtilMain = launchyUtilModule.attr("main");
        launchyUtilMain();
    }
    catch (const py::error_already_set& e) {
        PyErr_Print();
        PyErr_Clear();
        qWarning() << "pluginpy::PluginMgr::PluginMgr, fail to init QSetting,"
            << e.what();
    }
}

PluginMgr::~PluginMgr() {
    for (auto plugin : m_pluginInterface)
    {
        delete plugin;
    }

    m_pSettings = nullptr;
    m_pluginInterface.clear();
    m_pluginObject.clear();

    // !! can not call this function,
    // python is still running in this destructor
    // py::finalize_interpreter();
}

} // namesapce pluginpy

