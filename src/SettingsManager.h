/*
Launchy: Application Launcher
Copyright (C) 2010  Josh Karlin, Simon Capewell

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

#include <QString>
#include <QHash>
#include <QList>
#include "Directory.h"

namespace launchy {
class SettingsManager {
public:
    static SettingsManager& instance();

public:
    void load();

    bool isPortable() const;
    QList<QString> directory(QString name) const;
    QString catalogFilename() const;
    QString historyFilename() const;
    QString skinPath(const QString& skinName) const;
    void setPortable(bool makePortable);
    void removeAll();
    void setProfileName(const QString& name);
    QList<Directory> readCatalogDirectories();
    void writeCatalogDirectories(QList<Directory>& directories);

private:
    SettingsManager();
    Q_DISABLE_COPY(SettingsManager)

private:
    QString configDirectory(bool portable) const;

private:
    bool m_portable;
    QString m_profileName;
    QHash<QString, QList<QString>> m_dirs;
};
}
