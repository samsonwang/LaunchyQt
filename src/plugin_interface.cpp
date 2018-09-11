/*
Launchy: Application Launcher
Copyright (C) 2007-2009  Josh Karlin

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
#include "plugin_interface.h"
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QLocale>

/*! \file
    \brief A Documented file.
    
    Details.
*/


#ifdef Q_WS_WIN
// This doesn't exist until qt 4.6 (currently 4.5 in ubuntu 10.04)
#include <QProcessEnvironment>

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#ifndef UNICODE
#define UNICODE
#endif

#define _WIN32_WINNT 0x0600	
#define _WIN32_IE 0x0700

#include <windows.h>
#include <shlobj.h>

// This is also defined in WinIconProvider, remove from both locations if 64 bit build is produced
QString aliasTo64(QString path) 
{
	QProcessEnvironment env = QProcessEnvironment::systemEnvironment ();
	QString pf32 = env.value("PROGRAMFILES");
	QString pf64 = env.value("PROGRAMW6432");

	// On 64 bit windows, 64 bit shortcuts don't resolve correctly from 32 bit executables, fix it here
	QFileInfo pInfo(path);

	if (env.contains("PROGRAMW6432") && pInfo.isSymLink() && pf32 != pf64) {
		if (QDir::toNativeSeparators(pInfo.symLinkTarget()).contains(pf32)) {
			QString path64 = QDir::toNativeSeparators(pInfo.symLinkTarget());
			path64.replace(pf32, pf64);
			if (QFileInfo(path64).exists()) {
				path = path64;
			}
		}
		else if (pInfo.symLinkTarget().contains("system32")) {
			QString path32 = QDir::toNativeSeparators(pInfo.symLinkTarget());
			if (!QFileInfo(path32).exists()) {
				path = path32.replace("system32", "sysnative");
			}
		}
	}
	return path;
}

int getDesktop() { return DESKTOP_WINDOWS; }

/*
void runProgram(QString path, QString args) {

	SHELLEXECUTEINFO ShExecInfo;
	bool elevated = (GetKeyState(VK_SHIFT) & 0x80000000) != 0 && (GetKeyState(VK_CONTROL) & 0x80000000) != 0;

	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_FLAG_NO_UI;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = elevated ? L"runas" : NULL;
	ShExecInfo.lpFile = (LPCTSTR)path.utf16();
	if (args != "") {
		ShExecInfo.lpParameters = (LPCTSTR)args.utf16();
	} else {
		ShExecInfo.lpParameters = NULL;
	}
	QDir dir(path);
	QFileInfo info(path);
	if (!info.isDir() && info.isFile())
		dir.cdUp();	
	QString filePath = QDir::toNativeSeparators(dir.absolutePath());
	ShExecInfo.lpDirectory = (LPCTSTR)filePath.utf16();
	ShExecInfo.nShow = SW_NORMAL;
	ShExecInfo.hInstApp = NULL;

	ShellExecuteEx(&ShExecInfo);	
}
*/
void runProgram(QString path, QString args, bool translateSeparators) {

	// This 64 bit aliasing needs to be gotten rid of if we have a 64 bit build
	path = aliasTo64(path);


	SHELLEXECUTEINFO ShExecInfo;
	bool elevated = (GetKeyState(VK_SHIFT) & 0x80000000) != 0 && (GetKeyState(VK_CONTROL) & 0x80000000) != 0;

	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_FLAG_NO_UI;
	ShExecInfo.fMask = NULL;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = elevated ? L"runas" : NULL;
	QString filePath = translateSeparators ? QDir::toNativeSeparators(path) : path;
	ShExecInfo.lpFile = (LPCTSTR)filePath.utf16();

	if (args != "") {
		ShExecInfo.lpParameters = (LPCTSTR)args.utf16();
	} else {
		ShExecInfo.lpParameters = NULL;
	}


	QDir dir(path);
	QFileInfo info(path);
	if (!info.isDir() && info.isFile())
		dir.cdUp();
	QString directory = QDir::toNativeSeparators(dir.absolutePath());
	ShExecInfo.lpDirectory = (LPCTSTR)directory.utf16();
	ShExecInfo.nShow = SW_NORMAL;
	ShExecInfo.hInstApp = NULL;

	ShellExecuteEx(&ShExecInfo);	
}

#endif

#ifdef Q_WS_MAC

int getDesktop() { return DESKTOP_MAC; }

void runProgram(QString path, QString args, bool translateSeparators)
{
    translateSeparators = translateSeparators; // kill warning
    QString cmd;
    cmd = "open \"" + QDir::toNativeSeparators(path) + "\" --args " + args.trimmed();
    QProcess::startDetached(cmd.trimmed());

    return;
}

#endif


#ifdef Q_WS_X11


int getDesktop()
{
    QStringList list = QProcess::systemEnvironment();
    foreach(QString s, list)
	{
	    if (s.startsWith("GNOME_DESKTOP_SESSION"))
		return DESKTOP_GNOME;
	    else if (s.startsWith("KDE_FULL_SESSION"))
		return DESKTOP_KDE;
	}
    return -1;
}



void runProgram(QString path, QString args, bool translateSeparators) {

    QString fullname = path.split(" ")[0];
    QFileInfo info(fullname);

    /* I would argue that launchy does not need to fully
       support the desktop entry specification yet/ever.
       NOTE: %c, %k, and %i are handled during loading */
    if( path.contains("%") ){
        path.replace("%U", args);
        path.replace("%u", args);
        path.replace("%F", args);
        path.replace("%f", args);
        /* remove specifiers either not yet supported or depricated */
        path.remove(QRegExp("%."));
        args = "";
   }

    QString cmd;

    if( !info.isExecutable() || info.isDir() ){
        /* if more then one file is passed, then xdg-open will fail.. */
        cmd = "xdg-open \"" + path.trimmed() + "\"";
    }else if(getDesktop() == DESKTOP_KDE) {
        /* special case for KDE since some apps start behind other windows */
        cmd = "kstart --activate " + path.trimmed() + " " + args.trimmed();
    } else /* gnome, xfce, etc */ {
        path.replace("\"", "\\\"");
        args.replace("\"", "\\\"");
        cmd = "sh -c \"" + path.trimmed() + " " + args.trimmed() + "\"";
     }


    QProcess::startDetached(cmd);

    return;
}


#endif

