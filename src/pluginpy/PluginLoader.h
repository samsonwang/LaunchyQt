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

#include "PluginPyLib.h"

namespace launchy { class PluginInterface; }

namespace pluginpy {

class PLUGINPY_EXPORT PluginLoader {
public:
    PluginLoader(const QString& pluginName, const QString& pluginPath);

    launchy::PluginInterface* instance();

    bool unload();

    static void initSettings(QSettings* setting);

private:
    void setFileName(const QString& fileName);
    bool load();

private:
    QString m_pluginName;
    QString m_pluginPath;
    launchy::PluginInterface* m_interface;
};

}
