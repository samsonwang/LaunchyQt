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

#include "precompiled.h"
#include "minidump.h"


TCHAR* MiniDumper::m_appName;
MINIDUMPWRITEDUMP MiniDumper::m_dumpFunction;


MiniDumper::MiniDumper(const TCHAR* appName)
{
	if (m_appName == NULL)
	{
		m_appName = _tcsdup(appName);
#ifndef DEBUG
		SetUnhandledExceptionFilter(TopLevelFilter);
#endif
	}
}


LONG MiniDumper::TopLevelFilter(struct _EXCEPTION_POINTERS *exceptionInfo)
{
	LONG result = EXCEPTION_CONTINUE_SEARCH;

	if (CreateMiniDump(_T("Mini"), exceptionInfo, MiniDumpNormal) &&
		CreateMiniDump(_T("Midi"), exceptionInfo, MiniDumpWithPrivateReadWriteMemory))
	{
		MessageBox(NULL, 
			_T("Sorry, Launchy seems to have crashed. To help us work out what went wrong, crash dumps have been created in your temp directory. Please send them to us via the SourceForge forums."),
			m_appName, MB_OK | MB_ICONEXCLAMATION);
		result = EXCEPTION_EXECUTE_HANDLER;
	}

	return result;
}


bool MiniDumper::CreateMiniDump(const TCHAR* postfix, struct _EXCEPTION_POINTERS *exceptionInfo, MINIDUMP_TYPE dumpType)
{
	bool result = false;

	MINIDUMPWRITEDUMP pDump = GetMiniDumpWriteFunction();
	if (pDump)
	{
		TCHAR dumpPath[_MAX_PATH];

		// work out a good place for the dump file
		if (!GetTempPath(_MAX_PATH, dumpPath))
			_tcscpy(dumpPath, _T("c:\\"));

		_tcscat(dumpPath, m_appName);
		_tcscat(dumpPath, _T("_"));
		_tcscat(dumpPath, postfix);
		_tcscat(dumpPath, _T("CrashDump.dmp"));

		// create the file
		HANDLE hFile = CreateFile(dumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
								  FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			_MINIDUMP_EXCEPTION_INFORMATION ExInfo;
			ExInfo.ThreadId = GetCurrentThreadId();
			ExInfo.ExceptionPointers = exceptionInfo;
			ExInfo.ClientPointers = NULL;

			// write the dump
			if (pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, dumpType, &ExInfo, NULL, NULL))
			{
				result = true;
			}
			CloseHandle(hFile);
		}
	}

	return result;
}


MINIDUMPWRITEDUMP MiniDumper::GetMiniDumpWriteFunction()
{
	if (!m_dumpFunction)
	{
		// Find dbghelp.dll and check it exports the MiniDumpWriteDump function
		HMODULE hDll = NULL;
		TCHAR szDbgHelpPath[_MAX_PATH];

		if (GetModuleFileName(NULL, szDbgHelpPath, _MAX_PATH))
		{
			TCHAR *pSlash = _tcsrchr(szDbgHelpPath, _T('\\'));
			if (pSlash)
			{
				_tcscpy(pSlash+1, _T("DBGHELP.DLL"));
				hDll = LoadLibrary(szDbgHelpPath);
			}
		}

		if (hDll == NULL)
		{
			// load any version we can
			hDll = LoadLibrary(_T("DBGHELP.DLL"));
		}

		m_dumpFunction = hDll ? (MINIDUMPWRITEDUMP)GetProcAddress(hDll, "MiniDumpWriteDump") : NULL;
	}

	return m_dumpFunction;
}