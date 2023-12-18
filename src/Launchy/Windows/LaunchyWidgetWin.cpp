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

#include "LaunchyWidgetWin.h"

#include "UtilWin.h"

namespace launchy {

LaunchyWidgetWin::LaunchyWidgetWin(CommandFlags command)
    : LaunchyWidget(command) {
    commandMessageId = RegisterWindowMessageW(L"LaunchyCommand");
}

bool LaunchyWidgetWin::nativeEvent(const QByteArray& eventType,
                                   void* message,
                                   intptr_t* result) {
    MSG* msg = (MSG*)message;
    switch (msg->message) {
    case WM_SETTINGCHANGE:
        // Refresh Launchy's environment on settings changes
        if (msg->lParam && wcscmp((WCHAR*)msg->lParam, L"Environment") == 0) {
            UpdateEnvironment();
        }
        break;

    case WM_ENDSESSION:
        // Ensure settings are saved
        saveSettings();
        break;

        // Might need to capture these two messages if Vista gives any problems with alpha borders
        // when restoring from standby
    case WM_POWERBROADCAST:
        break;
    case WM_WTSSESSION_CHANGE:
        break;

    default:
        if (msg->message == commandMessageId) {
            // A Launchy startup command
            executeStartupCommand((int)msg->wParam);
        }
        break;
    }
    return LaunchyWidget::nativeEvent(eventType, message, result);
}

void LaunchyWidgetWin::focusLaunchy() {
    // need to use this method in Windows to ensure that keyboard focus is set when
    // being activated via a hook or message from another instance of Launchy
    SetForegroundWindowEx((HWND)winId());
    LaunchyWidget::focusLaunchy();
}

// Create the main widget for the application
void createLaunchyWidget(CommandFlags command) {
    if (!LaunchyWidget::s_instance) {
        LaunchyWidget::s_instance = new LaunchyWidgetWin(command);
    }
}

} // namespace launchy
