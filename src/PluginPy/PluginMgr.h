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

// qt defines "slots" macro
// python.h must include before qt header files
#include <Python.h>
#include <pybind11/pybind11.h>

#include <QString>
#include <QSettings>

namespace launchy { class PluginInterface; }
namespace py = pybind11;

namespace pluginpy {

class PluginMgr {
public:
    static PluginMgr& instance();

    launchy::PluginInterface* loadPlugin(const QString& pluginName, const QString& pluginPath);
    bool unloadPlugin(const QString& pluginName);

    void initSettings(QSettings* setting);

    void registerPlugin(py::object pluginClass);

private:
    PluginMgr();
    ~PluginMgr();
    PluginMgr(const PluginMgr&) = delete;
    PluginMgr& operator=(const PluginMgr&) = delete;

private:
    QSettings* m_pSettings;
    QMap<QString, py::object> m_pluginObject;
    QMap<QString, launchy::PluginInterface*> m_pluginInterface;
};

} // namespace pluginpy
