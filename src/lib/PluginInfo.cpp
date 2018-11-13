/*
Launchy
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

#include "PluginInfo.h"
#include <QPluginLoader>
#include "PluginInterface.h"

PluginInfo::PluginInfo()
    : id(0),
      obj(nullptr),
      loaded(false) {
}

PluginInfo::~PluginInfo() {
    QPluginLoader loader(path);
    loader.unload();
}

bool PluginInfo::isValid() const {
    return obj && !name.isNull() && id > 0;
}

int PluginInfo::sendMsg(int msgId, void* wParam, void* lParam) {
    // This should have some kind of exception guard to prevent
    // Launchy from crashing when a plugin is misbehaving.
    // This would consist of a try/catch block to handle C++ exceptions
    // and on Windows would also include a structured exception handler
    return obj->msg(msgId, wParam, lParam);
}