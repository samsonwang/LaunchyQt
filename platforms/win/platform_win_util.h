/*
Launchy: Application Launcher
Copyright (C) 2007-2009  Josh Karlin, Simon Capewell

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

#ifndef __PLATFORM_WIN_UTIL_H
#define __PLATFORM_WIN_UTIL_H


void UpdateEnvironment();
QString GetShellDirectory(int type);
bool EnumerateNetworkServers(QStringList& items, DWORD serverType, const wchar_t* domain = NULL);


class LimitSingleInstance
{
public:
	LimitSingleInstance(TCHAR *strMutexName)
	{
		//Make sure that you use a name that is unique for this application otherwise
		//two apps may think they are the same if they are using same name for
		//3rd parm to CreateMutex
		mutex = CreateMutex(NULL, FALSE, strMutexName); //do early
		lastError = GetLastError(); //save for use later...
	}

	~LimitSingleInstance() 
	{
		if (mutex)  //Do not forget to close handles.
		{
			CloseHandle(mutex); //Do as late as possible.
			mutex = NULL; //Good habit to be in.
		}
	}

	bool IsAnotherInstanceRunning() 
	{
		return (ERROR_ALREADY_EXISTS == lastError);
	}

private:
	HANDLE mutex;
	DWORD  lastError;
};


#endif
