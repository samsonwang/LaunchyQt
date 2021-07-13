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

#include "CrashDumper.h"

#include <stdlib.h>

WCHAR* CrashDumper::m_appName;
MINIDUMPWRITEDUMP CrashDumper::m_dumpFunction;

CrashDumper::CrashDumper(const WCHAR* appName) {
    if (m_appName == NULL) {
        m_appName = _wcsdup(appName);
#ifdef NDEBUG
        SetUnhandledExceptionFilter(TopLevelFilter);
#endif
    }
}

LONG CrashDumper::TopLevelFilter(struct _EXCEPTION_POINTERS *exceptionInfo) {
    LONG result = EXCEPTION_CONTINUE_SEARCH;

    if (CreateMiniDump(L"Normal", exceptionInfo, MiniDumpNormal) &&
        CreateMiniDump(L"PRWMem", exceptionInfo, MiniDumpWithPrivateReadWriteMemory)) {
        MessageBoxW(NULL,
                    L"Sorry, Launchy seems to have crashed. "
                    "To help us work out what went wrong, "
                    "crash dumps have been created in your temp directory. "
                    "Please contact me via Launchy github issue.",
                    m_appName, MB_OK | MB_ICONEXCLAMATION);
        result = EXCEPTION_EXECUTE_HANDLER;
    }

    return result;
}


bool CrashDumper::CreateMiniDump(const WCHAR* postfix,
                                struct _EXCEPTION_POINTERS *exceptionInfo,
                                MINIDUMP_TYPE dumpType) {
    bool result = false;

    MINIDUMPWRITEDUMP pDump = GetMiniDumpWriteFunction();
    if (pDump) {
        WCHAR dumpPath[_MAX_PATH];

        // work out a good place for the dump file
        if (!GetTempPathW(_MAX_PATH, dumpPath))
            wcscpy(dumpPath, L"c:\\");

        wcscat(dumpPath, m_appName);
        if (DirectoryExists(dumpPath) == TRUE
            || CreateDirectoryW(dumpPath, NULL) == TRUE) {
            wcscat(dumpPath, L"\\");
        }
        else {
            wcscat(dumpPath, L"_");
        }
        wcscat(dumpPath, postfix);
        wcscat(dumpPath, L"CrashDump");

        SYSTEMTIME time;
        GetLocalTime(&time);
        WCHAR timeStr[256];
        GetDateFormatW(LOCALE_USER_DEFAULT, 0,
                       &time, L"yyyyMMdd", timeStr, sizeof(timeStr));
        wcscat(dumpPath, timeStr);

        GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0,
                        &time, L"HHmmss", timeStr, sizeof(timeStr));
        wcscat(dumpPath, timeStr);

        wcscat(dumpPath, L".dmp");

        // create the file
        HANDLE hFile = CreateFileW(dumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL, NULL);

        if (hFile != INVALID_HANDLE_VALUE) {
            _MINIDUMP_EXCEPTION_INFORMATION ExInfo;
            ExInfo.ThreadId = GetCurrentThreadId();
            ExInfo.ExceptionPointers = exceptionInfo;
            ExInfo.ClientPointers = NULL;

            // write the dump
            if (pDump(GetCurrentProcess(), GetCurrentProcessId(),
                      hFile, dumpType, &ExInfo, NULL, NULL)) {
                result = true;
            }
            CloseHandle(hFile);
        }
    }

    return result;
}


MINIDUMPWRITEDUMP CrashDumper::GetMiniDumpWriteFunction() {
    if (!m_dumpFunction) {
        // Find dbghelp.dll and check it exports the MiniDumpWriteDump function
        HMODULE hDll = NULL;
        WCHAR szDbgHelpPath[_MAX_PATH];

        if (GetModuleFileNameW(NULL, szDbgHelpPath, _MAX_PATH)) {
            WCHAR *pSlash = wcsrchr(szDbgHelpPath, L'\\');
            if (pSlash) {
                wcscpy(pSlash+1, L"DBGHELP.DLL");
                hDll = LoadLibraryW(szDbgHelpPath);
            }
        }

        if (hDll == NULL) {
            // load any version we can
            hDll = LoadLibraryW(L"DBGHELP.DLL");
        }

        m_dumpFunction = hDll ? (MINIDUMPWRITEDUMP)GetProcAddress(hDll, "MiniDumpWriteDump") : NULL;
    }

    return m_dumpFunction;
}

BOOL CrashDumper::DirectoryExists(LPCWSTR szPath) {
    DWORD dwAttrib = GetFileAttributesW(szPath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES
            && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
