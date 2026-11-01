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
#include <strsafe.h>

WCHAR* CrashDumper::m_appName;
MINIDUMPWRITEDUMP CrashDumper::m_dumpFunction;
HMODULE CrashDumper::m_dbgHelpDll;

CrashDumper::CrashDumper(const WCHAR* appName) {
    if (m_appName == NULL) {
        m_appName = _wcsdup(appName);
#ifdef NDEBUG
        SetUnhandledExceptionFilter(TopLevelFilter);
#endif
    }
}

CrashDumper::~CrashDumper() {
    free(m_appName);
    m_appName = NULL;
    if (m_dbgHelpDll) {
        FreeLibrary(m_dbgHelpDll);
        m_dbgHelpDll = NULL;
    }
    m_dumpFunction = NULL;
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
        // Build the dump path in a fixed-size buffer using the bounds-checked
        // StringCch* family so no segment can ever overflow the stack. Every
        // append is length-checked; on any overflow the path is discarded and
        // the dump is simply skipped rather than corrupting the stack.
        WCHAR dumpPath[MAX_PATH];

        // work out a good place for the dump file
        if (!GetTempPathW(_countof(dumpPath), dumpPath))
            StringCchCopyW(dumpPath, _countof(dumpPath), L"c:\\");

        if (FAILED(StringCchCatW(dumpPath, _countof(dumpPath), m_appName)))
            return false;
        if (DirectoryExists(dumpPath) == TRUE
            || CreateDirectoryW(dumpPath, NULL) == TRUE) {
            if (FAILED(StringCchCatW(dumpPath, _countof(dumpPath), L"\\")))
                return false;
        }
        else {
            if (FAILED(StringCchCatW(dumpPath, _countof(dumpPath), L"_")))
                return false;
        }
        if (FAILED(StringCchCatW(dumpPath, _countof(dumpPath), postfix)))
            return false;
        if (FAILED(StringCchCatW(dumpPath, _countof(dumpPath), L"CrashDump")))
            return false;

        SYSTEMTIME time;
        GetLocalTime(&time);
        WCHAR timeStr[256];
        GetDateFormatW(LOCALE_USER_DEFAULT, 0,
                       &time, L"yyyyMMdd", timeStr, _countof(timeStr));
        if (FAILED(StringCchCatW(dumpPath, _countof(dumpPath), timeStr)))
            return false;

        GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, 0,
                        &time, L"HHmmss", timeStr, _countof(timeStr));
        if (FAILED(StringCchCatW(dumpPath, _countof(dumpPath), timeStr)))
            return false;

        if (FAILED(StringCchCatW(dumpPath, _countof(dumpPath), L".dmp")))
            return false;

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

        m_dbgHelpDll = hDll;
        m_dumpFunction = hDll ? (MINIDUMPWRITEDUMP)GetProcAddress(hDll, "MiniDumpWriteDump") : NULL;
    }

    return m_dumpFunction;
}

BOOL CrashDumper::DirectoryExists(LPCWSTR szPath) {
    DWORD dwAttrib = GetFileAttributesW(szPath);

    return (dwAttrib != INVALID_FILE_ATTRIBUTES
            && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
