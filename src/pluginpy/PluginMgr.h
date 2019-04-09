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

#include <pybind11/pybind11.h>

namespace launchy { class PluginInterface; }
namespace py = pybind11;

namespace pluginpy {

class PluginMgr {
public:
    static PluginMgr& instance();

    launchy::PluginInterface* loadPlugin(const QString& pluginName, const QString& pluginPath);
    bool unloadPlugin(uint pluginId);

    void initSettings(QSettings* setting);

    void registerPlugin(py::object pluginClass);

private:
    PluginMgr();
    ~PluginMgr();
    PluginMgr(const PluginMgr&) = delete;
    PluginMgr& operator=(const PluginMgr&) = delete;

private:
    // type of python (embedable or installed)
    // QVector<py::object> m_pluginClass;
    QSettings* m_pSettings;
    QHash<uint, py::object> m_pluginObject;
    QHash<uint, launchy::PluginInterface*> m_pluginInterface;
    //QVector<py::object> m_pluginObject;
};

}
