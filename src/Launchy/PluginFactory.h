/*
Launchy: Application Launcher
Copyright (C) 2007-2009  Josh Karlin

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#pragma once

#include <QHash>
#include <QString>

#include "PluginRegistry.h"

class QPluginLoader;

namespace pluginpy {
class PluginLoader;
}

namespace launchy {

// The two ways a plugin can be built, behind the one interface the registry
// knows about.
//
//   * a directory that resolves as a module is a C++ plugin, loaded through
//     QPluginLoader
//   * a directory holding <name>.py is a python plugin, loaded through the
//     PluginPy bridge
//
// Both mechanisms used to be two private methods of PluginHandler, which meant
// the enable/disable policy was duplicated for them and neither could be tested
// without a real DLL. They live here now, and nothing above this class mentions
// .dll or .py.
class PluginFactory : public IPluginFactory {
public:
    PluginFactory();
    ~PluginFactory() override;

public:
    PluginInfo instantiate(const QString& name, const QString& path) override;
    void release(const QString& name) override;

private:
    PluginInfo loadCpp(const QString& name, const QString& path);
    PluginInfo loadPython(const QString& name, const QString& path);

private:
    // The loaders have to outlive instantiate(): the module behind a plugin
    // object must stay loaded for as long as the registry keeps that object,
    // and release() arrives after instantiate() has returned.
    QHash<QString, QPluginLoader*> m_cpp;
    QHash<QString, pluginpy::PluginLoader*> m_python;
};

} // namespace launchy
