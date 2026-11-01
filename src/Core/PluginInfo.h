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

// An entry of the plugin table: one loaded plugin, its name, where it came
// from, and the object the messages are dispatched to. It lives next to the
// registry that owns the table rather than in the plugin ABI: nothing outside
// this library names the type, a plugin only ever sees the PluginInterface it
// implements.
#pragma once

#include <QString>

namespace launchy {

class PluginInterface;

// A plain value type on purpose: the registry copies it in and out of the
// table, and the option dialog reads a copy of that table. No virtual methods,
// no ownership -- the PluginInterface* belongs to the factory that made it.
struct PluginInfo {
    QString name;
    QString path;
    bool loaded;
    PluginInterface* obj;

public:
    PluginInfo();
    ~PluginInfo();

public:
    bool isValid() const;

    int sendMsg(int msgId, void* wParam = nullptr, void* lParam = nullptr);
};

} // namespace launchy
