/*
  Launchy: Application Launcher
  Copyright (C) 2008  Josh Karlin

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

class IconProviderLinux;

// The Linux answers to the platform questions. Pure PlatformBase: no
// QApplication, no QWidget -- only X11, QtGui (QX11Info) and Qt Core.
class PlatformLinux : public PlatformBase {
public:
    PlatformLinux();
    virtual ~PlatformLinux();

    virtual QHash<QString, QList<QString> > directories() const;
    virtual QList<Directory> defaultCatalogDirectories() const;
    virtual QString expandEnvironmentVars(const QString& vars) const;

    virtual bool supportsAlphaBorder() const;

    virtual bool getComputers(QStringList& computers) const;
    virtual bool allowNotification() const;
    virtual int systemIdleSeconds() const;

    virtual void runProgram(const QString& file, const QString& args,
                            bool translateSeparators = true) const;

    // Rewrite .desktop items as they are indexed (extract name/icon/exec)
    virtual void alterItem(CatItem* item) const;

private:
    // Owns the desktop-icon cache used by alterItem; this class must not reach
    // for the application object, so it carries its own provider.
    IconProviderLinux* m_iconProvider;
};

} // namespace launchy
