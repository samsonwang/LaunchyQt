/*
Launchy: Application Launcher
Copyright (C) 2007 Josh Karlin, Simon Capewell

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

#include "UtilWin.h"

#include <UserEnv.h>
#include <ShlObj.h>
#include <LM.h>

#include "AppWin.h"
#include "GlobalVar.h"

namespace launchy {

// Replace this process' environment with the current system environment
void UpdateEnvironment() {
    // Fetch the current environment for the user
    HANDLE accessToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY, &accessToken))
        return;

    wchar_t* environment;
    if (!CreateEnvironmentBlock((LPVOID*)&environment, accessToken, FALSE))
        return;

    // Empty the current environment
    QStringList variables;
    wchar_t* currentEnvironment = GetEnvironmentStringsW();
    for (WCHAR* p = currentEnvironment; *p != 0;)
    {
        QString variable = QString::fromWCharArray(p);
        QString name = variable.section("=", 0, 0);
        // Ignore entries for drive current directory entries that have no name
        if (name.size() > 0)
            variables.append(name);
        p += wcslen(p) + 1;
    }
    if (currentEnvironment)
        FreeEnvironmentStringsW(currentEnvironment);

    // Now we've finished enumerating the current environment, we can safely delete variables
    foreach(QString name, variables)
    {
        SetEnvironmentVariableW((LPCWSTR)name.utf16(), NULL);
    }

    // Recreate the environment using the fresh system copy
    for (wchar_t* p = environment; *p != 0;)
    {
        wchar_t* name = p;
        wchar_t* value = wcschr(p, L'=');
        p += wcslen(p) + 1;
        if (value)
        {
            *value = L'\0';
            SetEnvironmentVariableW(name, value + 1);
        }
    }

    DestroyEnvironmentBlock(environment);
    CloseHandle(accessToken);
}

QString GetShellDirectory(int type) {
    wchar_t buffer[_MAX_PATH];
    SHGetFolderPathW(NULL, type, NULL, 0, buffer);
    return QString::fromWCharArray(buffer);
}

bool EnumerateNetworkServers(QStringList& items, DWORD serverType, const wchar_t* domain) {
    SERVER_INFO_100* serverInfo = 0;
    DWORD read, totalOnNetwork;

    NET_API_STATUS result = NetServerEnum(NULL, 100, (BYTE**)&serverInfo, MAX_PREFERRED_LENGTH,
                                          &read, &totalOnNetwork, serverType, domain, 0);
    if (result == NERR_Success) {
        for (DWORD i = 0; i < read; ++i) {
            QString name = QString::fromWCharArray(serverInfo[i].sv100_name);
            items.push_back(name);
        }
    }

    // Possible error codes
    // ERROR_ACCESS_DENIED: "The user does not have access to the requested information."
    // NERR_InvalidComputer: "The computer name is invalid."
    // ERROR_NO_BROWSER_SERVERS_FOUND: "No browser servers found."
    // ERROR_MORE_DATA: "More entries are available with subsequent calls."

    if (serverInfo) {
        NetApiBufferFree(serverInfo);
    }

    return result == NERR_Success;
}

void SetForegroundWindowEx(HWND hWnd) {
    // Attach foreground window thread to our thread
    const DWORD foreGroundID = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    const DWORD currentID = GetCurrentThreadId();

    AttachThreadInput(foreGroundID, currentID, TRUE);
    // Do our stuff here
    HWND lastActivePopupWnd = GetLastActivePopup(hWnd);
    SetForegroundWindow(lastActivePopupWnd);

    // Detach the attached thread
    AttachThreadInput(foreGroundID, currentID, FALSE);
}

} // namespace launchy
