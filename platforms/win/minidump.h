/*
Launchy: Application Launcher
Copyright (C) 2010  Simon Capewell

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

#ifndef MINIDUMP
#define MINIDUMP


#include "dbghelp.h"


// from dbghelp.h
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
									CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
									CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
									CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
									);

class MiniDumper
{
public:
	MiniDumper(const TCHAR* appName);
private:
	static LONG WINAPI TopLevelFilter(struct _EXCEPTION_POINTERS *exceptionInfo);
	static MINIDUMPWRITEDUMP GetMiniDumpWriteFunction();
	static bool CreateMiniDump(const TCHAR* postfix, struct _EXCEPTION_POINTERS *exceptionInfo, MINIDUMP_TYPE dumpType);

	static MINIDUMPWRITEDUMP m_dumpFunction;
	static TCHAR* m_appName;
};


#endif
