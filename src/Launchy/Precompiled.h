/*
Launchy: Application Launcher
Copyright (C) 2007-2009  Simon Capewell

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

#if !defined(PRECOMPILED_H)
#define PRECOMPILED_H

#ifdef _MSC_VER
#pragma warning (disable : 4091)
#pragma warning (disable : 4099)
#pragma warning (push, 1)
#define NOMINMAX
#endif

#include <QtGlobal> // for OS detection

#ifdef Q_OS_WIN
#include <Windows.h>
#include <tchar.h>
#include <shellapi.h>
#include <ShObjIdl.h>
#include <ShlObj.h>
#include <ShlGuid.h>
#include <comdef.h>
#include <CommCtrl.h>
#include <objbase.h>
#include <commoncontrols.h>
#include <UserEnv.h>
#include <LM.h>
// #include <QtWin>
#endif

#include <QtGui>
#include <QtWidgets>
#include <QtNetwork>
#include <QFuture>

// deps, 3rd party modules
#include "SingleApplication/singleapplication.h"
#include "QHotkey/QHotkey.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

//#include <stdlib.h>

// #include <boost/shared_ptr.hpp>
// using namespace boost;

//#ifdef Q_OS_WIN
//#define _CRT_SECURE_NO_WARNINGS

//#define _WIN32_WINNT 0x0600
//#define _WIN32_IE 0x0700


#endif /* PRECOMPILED */
