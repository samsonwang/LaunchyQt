/*
PluginPy
Copyright (C) 2017 Samson Wang

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

#include "PluginPy.h"

#include <QString>

namespace launchy { class PluginInterface; }

class QSettings;

namespace pluginpy {

// Publishes the ini to python as launchy.__settings (and runs
// launchy_util.initSettings). A free function on purpose: the caller does this
// before it knows which plugin it is about to load, so there is no loader yet
// to hang it on.
void PLUGINPY_EXPORT initSettings(QSettings* setting);

// A handle for one python plugin directory.
//
// It used to be a thin forwarder over a second class that owned the
// interpreter and the plugin table. That second class is gone: its state lives
// in PluginLoader.cpp as PluginCore, because a python interpreter is
// process-global and cannot belong to an object built once per plugin on the
// stack (see the comment on PluginCore).
//
// The handle addresses the plugin, it does not own it: PluginCore keeps the
// python object and the wrapper that instance() hands out, so this object can
// die as soon as it has said where the plugin lives.
class PLUGINPY_EXPORT PluginLoader {
public:
    PluginLoader(const QString& pluginName, const QString& pluginPath);

    launchy::PluginInterface* instance() const;

    bool unload();

private:
    bool load();

private:
    QString m_pluginName;
    QString m_pluginPath;
    launchy::PluginInterface* m_interface;
};

} // namespace pluginpy
