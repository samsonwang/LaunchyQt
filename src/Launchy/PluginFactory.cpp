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

#include "PluginFactory.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QLibrary>
#include <QPluginLoader>

#include "PluginInterface/Runtime.h"
#include "PluginInterface/PluginInterface.h"
#include "PluginInterface/PluginMsg.h"
#include "PluginPy/PluginLoader.h"

namespace launchy {

PluginFactory::PluginFactory() {
}

PluginFactory::~PluginFactory() {
    // Nothing to release: every loader this class builds is a stack object that
    // dies inside loadCpp()/loadPython(), and what it mapped stays mapped.
}

PluginInfo PluginFactory::instantiate(const QString& name, const QString& path) {
    const QString libPath = QDir::cleanPath(path + "/" + name);

    // Order matters and matches the loader this replaced: a directory that
    // resolves as a module is a C++ plugin, one holding <name>.py is a python
    // plugin, anything else is neither.
    if (QLibrary(libPath).load()) {
        return loadCpp(name, path);
    }
    if (QFile::exists(path + "/" + name + ".py")) {
        return loadPython(name, path);
    }

    qWarning() << "PluginFactory::instantiate, unknown plugin type, plugin name:"
        << name << ", dir: " << path;
    return PluginInfo();
}

PluginInfo PluginFactory::loadCpp(const QString& name, const QString& path) {
    const QString libPath = QDir::cleanPath(path + "/" + name);

    // The loader is a stack object: something has to map the module and hand
    // back the instance, and once it has, nothing else is wanted from it. This
    // process never unloads a plugin, so the loader has no reason to outlive
    // this call (measured: the module stays mapped and the instance keeps
    // answering after the loader is destroyed).
    QPluginLoader loader(libPath);
    PluginInterface* plugin = qobject_cast<PluginInterface*>(loader.instance());
    if (!plugin) {
        qWarning() << "PluginFactory::loadCpp," << libPath << "is not a valid plugin";
        return PluginInfo();
    }
    qDebug() << "PluginFactory::loadCpp, plugin loaded:" << libPath;

    PluginInfo info;
    info.path = path;
    info.loaded = false;
    info.obj = plugin;

    if (!info.sendMsg(MSG_GET_NAME, &info.name) || info.name.isEmpty()) {
        qWarning() << "PluginFactory::loadCpp, fail to get plugin name,"
            << " plugin path:" << libPath;
        // A module that is not a Launchy plugin does not deserve to stay mapped
        loader.unload();
        return PluginInfo();
    }

    // Make "this module is never unmapped" an explicit contract instead of an
    // observed side effect: the plugin object is handed to a table that copies
    // itself across threads, and a vtable that goes away with a loader object
    // is a crash nobody would trace back to this line.
    QLibrary::LoadHints hints = loader.loadHints();
    hints |= QLibrary::PreventUnloadHint;
    loader.setLoadHints(hints);

    // Deliberately no check that the reported name equals the directory name:
    // C++ plugins have never been held to that and some do not match.
    return info;
}

PluginInfo PluginFactory::loadPython(const QString& name, const QString& path) {
    const QString pluginFullPath = QDir::cleanPath(path + "/" + name + ".py");

    // The bridge hands the ini to python through launchy.__settings, so the
    // store has to be published before the first module is imported. Doing it
    // here rather than at startup keeps the interpreter out of a process that
    // has no python plugin installed.
    pluginpy::initSettings(g_settings.data());

    // Also a stack object. It only addresses the plugin, it does not own it:
    // the bridge keeps the python object and the wrapper it hands out, so this
    // loader can die as soon as it has said where the plugin lives.
    pluginpy::PluginLoader loader(name, path);
    PluginInterface* plugin = loader.instance();
    if (!plugin) {
        qWarning() << "PluginFactory::loadPython," << pluginFullPath
            << "is not a Launchy plugin";
        return PluginInfo();
    }
    qDebug() << "PluginFactory::loadPython, plugin loaded:" << pluginFullPath;

    PluginInfo info;
    info.path = path;
    info.loaded = false;
    info.obj = plugin;

    if (!info.sendMsg(MSG_GET_NAME, &info.name)) {
        qWarning() << "PluginFactory::loadPython, fail to get plugin name,"
            << " plugin path:" << pluginFullPath;
        loader.unload();
        return PluginInfo();
    }

    // A python module is imported by name from the plugin's own directory, so
    // the reported name is also how the bridge addresses the instance. Letting
    // a mismatch through would make unloadPlugin() miss the object it created.
    if (info.name != name) {
        qWarning() << "PluginFactory::loadPython, plugin name not match:"
            << name << ", " << info.name;
        loader.unload();
        return PluginInfo();
    }
    return info;
}

} // namespace launchy
