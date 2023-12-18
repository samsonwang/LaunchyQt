/*
LaunchyQt
Copyright (C) 2019 Samson Wang

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

#include <windows.h>

#include "LaunchyWidget.h"

namespace launchy {

// Override the main widget to handle incoming system messages.
// We could have done this in the QApplication
// event handler, but then we'd have to filter out the duplicates 
// for messages like WM_SETTINGCHANGE.

class LaunchyWidgetWin : public LaunchyWidget {
    friend void createLaunchyWidget(CommandFlags command);

protected:
    LaunchyWidgetWin(CommandFlags command);

protected:
    virtual bool nativeEvent(const QByteArray& eventType,
                             void* message,
                             intptr_t* result);

    virtual void focusLaunchy();

private:
    UINT commandMessageId;
};

} // namespace launchy
