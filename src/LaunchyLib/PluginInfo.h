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

#include <QString>

#include "LaunchyLib.h"

namespace launchy {

class PluginInterface;

// This structure is used by plugins such as PyLaunchy, so it must not be extended
// with virtual methods or additional data members
struct LAUNCHY_EXPORT PluginInfo {
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
