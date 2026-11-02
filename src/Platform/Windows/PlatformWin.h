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

#include "Platform.h"

namespace launchy {

// The Windows answers to the platform questions. Pure PlatformBase: no
// QApplication, no QWidget -- only Windows APIs and Qt Core.
class PlatformWin : public PlatformBase {
public:
    PlatformWin();
    virtual ~PlatformWin();

    // PlatformBase, the Windows answers
    virtual QHash<QString, QList<QString> > directories() const;
    virtual QList<Directory> defaultCatalogDirectories() const;
    virtual QString expandEnvironmentVars(const QString& vars) const;

    virtual bool supportsAlphaBorder() const;
    virtual bool getComputers(QStringList& computers) const;
    virtual bool allowNotification() const;
    virtual int systemIdleSeconds() const;

    virtual void runProgram(const QString& file, const QString& args,
                            bool translateSeparators = true) const;

    // Nothing to rewrite on Windows: .lnk files are read by the shortcut
    // handler and already carry their name, icon and target.
    virtual void alterItem(CatItem* item) const;
};

} // namespace launchy
