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

#include <QHash>
#include <QList>
#include <QString>

#include "Directory.h"

namespace launchy {

// Proxy settings as stored, without QtNetwork: the type is a plain int holding
// a QNetworkProxy::ProxyType value. Naming the enumerator here would put a
// Qt5Network reference into every consumer of the core library, the tests
// included.
struct ProxyConfig {
    ProxyConfig()
        : type(0), port(0), requirePassword(false) {
    }

    int type;
    QString host;
    quint16 port;
    bool requirePassword;
    QString user;
    QString password;
};

// The ini values whose effect the core cannot apply: the widget style, the
// language, the application proxy and whether debug logging is on. Core
// decides what they are, the application applies them -- applying needs
// QtWidgets, QtNetwork and the translation manager, none of which core links.
struct RuntimeSettings {
    RuntimeSettings()
        : debugLog(false) {
    }

    bool debugLog;
    QString appStyle;
    QString language;
    ProxyConfig proxy;
};

class SettingsManager {
public:
    static SettingsManager& instance();

public:
    // Read the ini file behind the given directory roots and default catalog
    // list. The caller supplies both, so the same code runs against the real
    // platform and against a temporary profile in the tests -- which is why
    // this file knows nothing about the platform library.
    void load(const QHash<QString, QList<QString>>& roots,
              const QList<Directory>& defaultCatalogDirectories);

    // Values the application still has to apply after load()
    const RuntimeSettings& runtime() const;

    bool isPortable() const;

    // Convert between portable and installed mode. Returns false when the
    // files could not be moved; reporting that is the caller's job, because
    // core has no way to show a message box. On failure the mode is left
    // unchanged -- flipping it while the files are still in the old directory
    // would make the app read a profile that does not exist.
    bool setPortable(bool makePortable);

    // Directory that holds the settings files, it follows the current mode
    // (portable or installed) and the active profile
    QString configDirectory() const;

    QList<QString> directory(const QString& name) const;
    QString catalogFilename() const;
    QString historyFilename() const;
    QString skinPath(const QString& skinName) const;
    void removeAll();

    void setProfileName(const QString& name);

    QList<Directory> readCatalogDirectories();
    void writeCatalogDirectories(QList<Directory>& directories);

    // Fix up catalog directories that point into another user's profile, add
    // the built-in ones and move them to the front. The list is only modified
    // in memory, returns true when 'directories' was modified
    bool mergeDefaultCatalogDirectories(QList<Directory>& directories);

private:
    SettingsManager();
    Q_DISABLE_COPY(SettingsManager)

private:
    QString configDirectory(bool portable) const;

private:
    bool m_portable;
    QString m_profileName;
    QHash<QString, QList<QString>> m_roots;
    QList<Directory> m_defaultDirs;
    RuntimeSettings m_runtime;
};
} // namespace launchy
