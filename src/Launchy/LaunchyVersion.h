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
#define LAUNCHY_VERSION_PATCH 7

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
