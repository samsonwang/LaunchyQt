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

#include "PluginLoader.h"
#include <QHash>
#include "PluginInterface.h"
#include "PluginMgr.h"

namespace pluginpy {

PluginLoader::PluginLoader(const QString& pluginName, const QString& pluginPath)
    : m_pluginName(pluginName),
      m_pluginPath(pluginPath),
      m_interface(nullptr) {
    load();
}

void PluginLoader::setFileName(const QString& fileName) {
    // split fileName to pluginName and pluginPath
    // m_fileName = fileName;
}

launchy::PluginInterface* PluginLoader::instance() {
    return m_interface;
}

bool PluginLoader::unload() {
    bool ret = false;
    try {
        // finally get m_interface
        PluginMgr& mgr = PluginMgr::instance();
        m_interface = nullptr;
        mgr.unloadPlugin(qHash(m_pluginName));
    }
    catch (const py::error_already_set& e) {
        PyErr_Print();
        PyErr_Clear();
        const char* errInfo = e.what();
        qWarning() << "PluginLoader::load, exception catched on load plugin"
            << "error info:" << errInfo;
    }

    return ret;
}

bool PluginLoader::load() {
    try {
        // finally get m_interface
        PluginMgr& mgr = PluginMgr::instance();
        m_interface = mgr.loadPlugin(m_pluginName, m_pluginPath);
    }
    catch (const py::error_already_set& e) {
        PyErr_Print();
        PyErr_Clear();
        const char* errInfo = e.what();
        qWarning() << "PluginLoader::load, exception catched on load plugin"
            << "error info:" << errInfo;
    }

    return m_interface != nullptr;
}

}

