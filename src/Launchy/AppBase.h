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

#include <QList>
#include <QHash>
#include <QString>
#include <QFileIconProvider>

#include <SingleApplication/SingleApplication>

#include "Directory.h"

class QFileInfo;

namespace launchy {

class CatItem;
class IconProviderBase;

class AppBase : public SingleApplication {
public:
    AppBase(int& argc, char** argv);
    virtual ~AppBase();

public:
    static void cleanup();

public:
    QIcon icon(const QFileInfo& info);
    QIcon icon(const QString& path);
    QIcon icon(QFileIconProvider::IconType type);
    void setPreferredIconSize(int size);

public:
    virtual QHash<QString, QList<QString>> getDirectories() const = 0;
    virtual QList<Directory> getDefaultCatalogDirectories() const = 0;
    virtual QString expandEnvironmentVars(const QString& vars) const = 0;

public:
    virtual bool isAlreadyRunning() const;
    virtual void sendInstanceCommand(int command);

    // Need to alter an indexed item?  e.g. .desktop files
    virtual void alterItem(CatItem* item) const;

    virtual bool supportsAlphaBorder() const;

    virtual bool getComputers(QStringList& computers) const;

    // detect current fullscreen mode
    virtual bool allowNotification() const;

protected:
    IconProviderBase* m_iconProvider;
};

AppBase* createApplication(int& argc, char** argv);

#define g_app static_cast<launchy::AppBase*>(qApp)

} // namespace launchy
