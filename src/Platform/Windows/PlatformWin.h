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

#include <memory>

#include "Platform.h"

// Global namespace, like the class itself: only the ownership is declared here,
// the definition (windows.h + dbghelp.h) stays in the .cpp.
class CrashDumper;

namespace launchy {

// The Windows answers to the platform questions. Pure PlatformBase: no
// QApplication, no QWidget -- only Windows APIs and Qt Core.
//
// It also owns the crash dumper: a mini dump is an OS facility, and owning it
// here means the handler covers everything the process does once the platform
// is installed, instead of only what happens after the QApplication exists.
class PlatformWin : public PlatformBase {
public:
    PlatformWin();
    virtual ~PlatformWin();

    // PlatformBase, the Windows answers
    virtual QMap<QString, QList<QString>> directories() const;
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

private:
    // unique_ptr, not a member by value: the header would otherwise have to
    // pull in windows.h and dbghelp.h for everyone who only asks a platform
    // question. Complete type at the destructor, which is defined in the .cpp.
    std::unique_ptr<CrashDumper> m_crashDumper;
};

} // namespace launchy
