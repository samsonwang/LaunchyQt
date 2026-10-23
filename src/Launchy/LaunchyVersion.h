/*
Launchy: Application Launcher
Copyright (C) 2007-2010  Josh Karlin, Simon Capewell

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#pragma once

#define LAUNCHY_VERSION_MAJOR 3
#define LAUNCHY_VERSION_MINOR 1
#define LAUNCHY_VERSION_PATCH 9

// auto generate version number and string below
constexpr unsigned LAUNCHY_VERSION
    = LAUNCHY_VERSION_MAJOR * 10000
    + LAUNCHY_VERSION_MINOR * 100
    + LAUNCHY_VERSION_PATCH;

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define LAUNCHY_VERSION_STRING \
    STR(LAUNCHY_VERSION_MAJOR) "." \
    STR(LAUNCHY_VERSION_MINOR) "." \
    STR(LAUNCHY_VERSION_PATCH)


// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif // WIN32 or WIN64


// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif // __GNUC__


#if defined(ENVIRONMENT64)
const char LAUNCHY_BIT_STRING[] = "64";
#elif defined(ENVIRONMENT32)
const char LAUNCHY_BIT_STRING[] = "32";
#else
const char LAUNCHY_BIT_STRING[] = "N/A";
#endif // ENVIRONMENT64


static const char LAUNCHY_BUILD_TIME_STRING[] =
{
    // YY year
    __DATE__[7], __DATE__[8], __DATE__[9], __DATE__[10],
    '-',

    // First month letter, Oct Nov Dec = '1' otherwise '0'
    (__DATE__[0] == 'O' || __DATE__[0] == 'N' || __DATE__[0] == 'D') ? '1' : '0',

    // Second month letter
    (__DATE__[0] == 'J') ? ((__DATE__[1] == 'a') ? '1' :        // Jan, Jun or Jul
                             ((__DATE__[2] == 'n') ? '6' : '7')) :
    (__DATE__[0] == 'F') ? '2' :                                // Feb
    (__DATE__[0] == 'M') ? (__DATE__[2] == 'r') ? '3' : '5' :   // Mar or May
    (__DATE__[0] == 'A') ? (__DATE__[1] == 'p') ? '4' : '8' :   // Apr or Aug
    (__DATE__[0] == 'S') ? '9' :                                // Sep
    (__DATE__[0] == 'O') ? '0' :                                // Oct
    (__DATE__[0] == 'N') ? '1' :                                // Nov
    (__DATE__[0] == 'D') ? '2' :                                // Dec
    0,
    '-',

    // First day letter, replace space with digit
    __DATE__[4] == ' ' ? '0' : __DATE__[4],

    // Second day letter
    __DATE__[5],

    ' ',
    __TIME__[0], __TIME__[1],
    __TIME__[2],
    __TIME__[3], __TIME__[4],
    __TIME__[5],
    __TIME__[6], __TIME__[7],

   '\0'
};

// The commit this build came from.
//
// Resolved on every build by src/cmake/GenerateBuildInfo.cmake, which writes it
// into this generated header before anything here is compiled, and prints the
// value in the build log. It deliberately does not come from a configure time
// compile definition: committing does not re-run cmake, so such a value keeps
// naming the commit that was checked out when cmake last ran -- including a
// "-dirty" that survived the commit that cleaned the tree.
//
// The guard keeps a hand written compile working; it then reports "unknown".
#if defined(__has_include)
#if __has_include("LaunchyBuildInfo.h")
#include "LaunchyBuildInfo.h"
#endif
#endif // __has_include

#ifndef LAUNCHY_GIT_HASH
#define LAUNCHY_GIT_HASH "unknown"
#endif // LAUNCHY_GIT_HASH

