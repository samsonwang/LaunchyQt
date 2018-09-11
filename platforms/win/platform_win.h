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

#ifndef PLATFORM_WIN
#define PLATFORM_WIN


#include "platform_base.h"
#include "platform_win_util.h"
#include "platform_base_hotkey.h"
#include "minidump.h"


class PlatformWin : public PlatformBase 
{
	Q_OBJECT

public:
	PlatformWin(int& argc, char** argv);
	~PlatformWin();

	virtual void setPreferredIconSize(int size);
	virtual QKeySequence getHotkey() const;
	virtual bool setHotkey(const QKeySequence& newHotkey, QObject* receiver, const char* slot);
	virtual QHash<QString, QList<QString> > getDirectories();
	virtual QList<Directory> getDefaultCatalogDirectories();
	virtual QString expandEnvironmentVars(QString);
	virtual bool supportsAlphaBorder() const;
	virtual bool isAlreadyRunning() const;
	virtual void sendInstanceCommand(int command);
	virtual bool getComputers(QStringList& computers) const;

private:
	HANDLE localMutex, globalMutex;
	LimitSingleInstance* instance;
	MiniDumper minidumper;
};


#endif
