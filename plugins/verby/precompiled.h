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

#ifndef PRECOMPILED_H
#define PRECOMPILED_H

#ifdef _MSC_VER
#pragma warning(push,3)
#endif

#include <QtGui>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <stdlib.h>


#ifdef Q_WS_WIN
#include <windows.h>
#include <shlobj.h>
#include <tchar.h>
#endif


#endif // PRECOMPILED_H
