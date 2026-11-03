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

#include <QString>

#include "Core/PluginRegistry.h"

namespace launchy {

// The two ways a plugin can be built, behind the one interface the registry
// knows about.
//
//   * a directory that resolves as a module is a C++ plugin, loaded through
//     QPluginLoader
//   * a directory holding <name>.py is a python plugin, loaded through the
//     PluginPy bridge
//
// Both mechanisms used to be two private methods of one loader class that also
// owned the table and the policy, which meant the enable/disable rule was
// duplicated for them and neither could be tested without a real DLL. They live
// here now, behind the single seam the registry knows about, and nothing above
// this class mentions .dll or .py.
//
// A factory is a short-lived stack object: build one for a single
// PluginRegistry::loadPlugins() pass and let it die. It owns nothing and it
// unloads nothing -- the loaders it builds are stack objects too, and a plugin
// that has been mapped stays mapped for the rest of the process. A second pass
// reuses the entries already in the table and asks this class for nothing.
class PluginFactory : public IPluginFactory {
public:
    PluginFactory();
    ~PluginFactory() override;

public:
    PluginInfo instantiate(const QString& name, const QString& path) override;

private:
    PluginInfo loadCpp(const QString& name, const QString& path);
    PluginInfo loadPython(const QString& name, const QString& path);
};

} // namespace launchy
