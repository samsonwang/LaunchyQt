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
#include <QDebug>

#include "PluginInterface.h"
#include "PluginMsg.h"

namespace launchy {

PluginInfo::PluginInfo()
    : loaded(false),
      obj(nullptr) {
}

PluginInfo::~PluginInfo() {

}

bool PluginInfo::isValid() const {
    return obj && !name.isEmpty();
}

int PluginInfo::sendMsg(int msgId, void* wParam, void* lParam) {
    // This should have some kind of exception guard to prevent
    // Launchy from crashing when a plugin is misbehaving.
    // This would consist of a try/catch block to handle C++ exceptions
    // and on Windows would also include a structured exception handler

    int ret = MSG_CONTROL_LAUNCHITEM;

    try {
        ret = obj->msg(msgId, wParam, lParam);
    }
    catch (const std::exception& e) {
        qWarning() << "PluginInfo::sendMsg, exception catched:" << e.what();
    }

    return ret;
}

} // namespace launchy
