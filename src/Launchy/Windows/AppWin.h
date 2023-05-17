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

#pragma once

#include <windows.h>

#include "AppBase.h"

class CrashDumper;

namespace launchy {

class AppWin : public AppBase {
public:
    AppWin(int& argc, char** argv);
    virtual ~AppWin();

    virtual QHash<QString, QList<QString>> getDirectories();
    virtual QList<Directory> getDefaultCatalogDirectories();
    virtual QString expandEnvironmentVars(QString);
    virtual bool supportsAlphaBorder() const;
    virtual void sendInstanceCommand(int command);
    virtual bool getComputers(QStringList& computers) const;

private:
    HANDLE localMutex;
    HANDLE globalMutex;
    CrashDumper* m_crashDumper;
};

} // namespace launchy

