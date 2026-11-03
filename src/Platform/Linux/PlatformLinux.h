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
// QApplication, no QWidget -- only X11, QtGui (QX11Info) and Qt Core. The only
// icons here are the ones alterItem() has to resolve while indexing; the icons
// the UI shows are extracted by IconExtractor (see Platform.h).
class PlatformLinux : public PlatformBase {
public:
    PlatformLinux();
    virtual ~PlatformLinux();

    virtual QMap<QString, QList<QString>> directories() const;
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
    // Resolving a .desktop entry's Icon= name to a file is an XDG lookup with
    // its own caches, so indexing needs a provider even though extraction no
    // longer goes through this class. It is deliberately NOT shared with the
    // one IconExtractor owns: indexing runs on the catalog thread and
    // extraction on its own, and these caches are only safe from one thread.
    IconProviderLinux* m_iconProvider;
};

} // namespace launchy
