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
    // Nothing is unloaded here on purpose. The plugin objects this process
    // created are still in the registry's table, and they are code living
    // inside those modules; unloading a module out from under them would take
    // their vtable with it. The loader objects are left to the OS, which is
    // the same deal the pre-split code made by letting its stack loader die
    // (and the python bridge makes at shutdown too).
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
    QPluginLoader* loader = new QPluginLoader(libPath);
    PluginInterface* plugin = qobject_cast<PluginInterface*>(loader->instance());
    if (!plugin) {
        qWarning() << "PluginFactory::loadCpp," << libPath << "is not a valid plugin";
        delete loader;
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
        loader->unload();
        delete loader;
        return PluginInfo();
    }

    // Deliberately no check that the reported name equals the directory name:
    // C++ plugins have never been held to that and some do not match.
    m_cpp.insert(name, loader);
    return info;
}

PluginInfo PluginFactory::loadPython(const QString& name, const QString& path) {
    const QString pluginFullPath = QDir::cleanPath(path + "/" + name + ".py");

    // The bridge hands the ini to python through launchy.__settings, so the
    // store has to be published before the first module is imported. Doing it
    // here rather than at startup keeps the interpreter out of a process that
    // has no python plugin installed.
    pluginpy::initSettings(g_settings.data());

    pluginpy::PluginLoader* loader = new pluginpy::PluginLoader(name, path);
    PluginInterface* plugin = loader->instance();
    if (!plugin) {
        qWarning() << "PluginFactory::loadPython," << pluginFullPath
            << "is not a Launchy plugin";
        delete loader;
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
        loader->unload();
        delete loader;
        return PluginInfo();
    }

    // A python module is imported by name from the plugin's own directory, so
    // the reported name is also how the bridge addresses the instance. Letting
    // a mismatch through would make unloadPlugin() miss the object it created.
    if (info.name != name) {
        qWarning() << "PluginFactory::loadPython, plugin name not match:"
            << name << ", " << info.name;
        loader->unload();
        delete loader;
        return PluginInfo();
    }

    m_python.insert(name, loader);
    return info;
}

void PluginFactory::release(const QString& name) {
    if (QPluginLoader* loader = m_cpp.take(name)) {
        loader->unload();
        delete loader;
    }
    if (pluginpy::PluginLoader* loader = m_python.take(name)) {
        loader->unload();
        delete loader;
    }
}

} // namespace launchy
