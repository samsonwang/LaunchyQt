/*
Launchy: Application Launcher
Copyright (C) 2007-2010  Josh Karlin, Simon Capewell

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

#include "Platform.h"

// The implementation this build installs. The include and the type have to
// agree, so they are selected together; each platform folder is on the include
// path of this library (Platform/Windows on Windows, Platform/Linux on Linux),
// which is what makes the bare header name resolve.
//
// This is the only piece of platform knowledge left outside the platform
// specific files. It used to sit behind a createPlatform() factory per platform,
// which wrapped a single new-expression and needed a hook to hand the pointer
// back here; a typedef says the same thing without either.
#if defined(Q_OS_WIN)
#include "Windows/PlatformWin.h"
#elif defined(Q_OS_LINUX)
#include "Linux/PlatformLinux.h"
#else
#error Launchy has no platform implementation for this target
#endif

namespace launchy {

namespace {

#if defined(Q_OS_WIN)
typedef PlatformWin InstalledPlatform;
#elif defined(Q_OS_LINUX)
typedef PlatformLinux InstalledPlatform;
#endif

// What the core sees when no platform is installed: every question is answered
// with "this platform has nothing to offer", never with a crash. Tests and any
// core-only consumer run against this.
class PlatformNull : public PlatformBase {
public:
    virtual QMap<QString, QList<QString>> directories() const {
        return QMap<QString, QList<QString>>();
    }

    virtual QList<Directory> defaultCatalogDirectories() const {
        return QList<Directory>();
    }

    // Returning the input unchanged is the honest answer: a platform that does
    // not know the OS cannot expand anything, and the value is still usable.
    virtual QString expandEnvironmentVars(const QString& vars) const {
        return vars;
    }

    virtual void alterItem(CatItem* item) const {
        Q_UNUSED(item);
    }

    virtual bool supportsAlphaBorder() const {
        return false;
    }

    virtual bool getComputers(QStringList& computers) const {
        Q_UNUSED(computers);
        return false;
    }

    virtual bool allowNotification() const {
        return true;
    }

    virtual int systemIdleSeconds() const {
        return -1;
    }

    virtual void runProgram(const QString& file, const QString& args,
                            bool translateSeparators) const {
        Q_UNUSED(file);
        Q_UNUSED(args);
        Q_UNUSED(translateSeparators);
    }
};

PlatformNull& platformNull() {
    static PlatformNull instance;
    return instance;
}

// The installed platform, nullptr while none. Private to this file: it is the
// whole reason the seam lives in the platform library rather than in the core,
// where it would have to be reachable from a header to be written.
PlatformBase* g_platform = nullptr;

} // namespace

PlatformBase& platform() {
    return g_platform != nullptr ? *g_platform : platformNull();
}

void initPlatform() {
    g_platform = new InstalledPlatform;
}

void cleanupPlatform() {
    delete g_platform;
    g_platform = nullptr;
}

} // namespace launchy
