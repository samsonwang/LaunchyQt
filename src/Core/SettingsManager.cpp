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

#include "SettingsManager.h"

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QRegularExpression>
#include <QDebug>

#include "PluginInterface/Runtime.h"

#include "OptionItem.h"

static const char* iniName = "/launchy.ini";
// The catalog index, stored as JSON (the legacy binary /launchy.db is
// intentionally ignored, no migration: a fresh index is rebuilt on load)
static const char* catalogName = "/catalog.json";
// Command history and query promotion, both in a single json file.
// The legacy binary /history.db is intentionally ignored (no migration).
static const char* historyName = "/history.json";

// a marker file to indicate that the app are installed and not portable
// (portable mode is used when the ini file is present but the marker file is not)
static const char* installedName = "/.installed";

namespace launchy {

// Compare catalog directory names ignoring separator style and, where the file
// system ignores it, case. The core has no platform header, so the case rule
// comes from Qt itself.
static bool isSameDirectoryPath(const QString& a, const QString& b) {
    QString pathA = QDir::fromNativeSeparators(a);
    QString pathB = QDir::fromNativeSeparators(b);

    while (pathA.length() > 1 && pathA.endsWith('/')) {
        pathA.chop(1);
    }
    while (pathB.length() > 1 && pathB.endsWith('/')) {
        pathB.chop(1);
    }

    const Qt::CaseSensitivity pathCase =
#if defined(Q_OS_WIN)
        Qt::CaseInsensitive;
#else
        Qt::CaseSensitive;
#endif

    return pathA.compare(pathB, pathCase) == 0;
}

// Detect catalog directory paths that point into another user's profile
// (account renamed or settings migrated to another machine): extract the
// user name from each path itself and compare it with the current user name.
// The list is only modified in memory, returns true when it was modified.
static bool fixCatalogUserPaths(QList<Directory>& directories) {
#if defined(Q_OS_WIN)
    const Qt::CaseSensitivity userCase = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity userCase = Qt::CaseSensitive;
#endif

    const QString currentHome = QDir::fromNativeSeparators(QDir::homePath());
    const QString currentUser = currentHome.section('/', -1);

    qDebug("SettingsManager::fixCatalogUserPaths, current user: %s, home: %s",
           qPrintable(currentUser), qPrintable(currentHome));

    // Match profile roots like C:/Users/name/..., /home/name/... or /Users/name/...
    static const QRegularExpression userPathRe(
        QStringLiteral("^((?:[A-Za-z]:)?/(?:Users|home)/)([^/]+)(/.*)?$"),
        QRegularExpression::CaseInsensitiveOption);

    bool changed = false;

    for (int i = 0; i < directories.count(); ++i) {
        const QString name = directories[i].name;
        const QString path = QDir::fromNativeSeparators(name);
        const QRegularExpressionMatch match = userPathRe.match(path);

        // Skip paths without a profile segment, paths of the current user
        // and paths that still exist (e.g. C:/Users/Public or other accounts)
        if (!match.hasMatch()
            || match.captured(2).compare(currentUser, userCase) == 0
            || QFile::exists(path)) {
            continue;
        }

        const QString fixed = currentHome + match.captured(3);
        qInfo("SettingsManager::fixCatalogUserPaths, %s -> %s",
              qPrintable(name),
              qPrintable(QDir::toNativeSeparators(fixed)));
        directories[i].name = name.contains('\\')
            ? QDir::toNativeSeparators(fixed) : fixed;
        changed = true;
    }

    return changed;
}

SettingsManager::SettingsManager()
    : m_portable(false) {
}

SettingsManager& SettingsManager::instance() {
    static SettingsManager s_obj;
    return s_obj;
}

void SettingsManager::load(const QMap<QString, QList<QString>>& roots,
                           const QList<Directory>& defaultCatalogDirectories) {
    m_roots = roots;
    m_defaultDirs = defaultCatalogDirectories;

    if (m_roots.isEmpty()) {
        qWarning("SettingsManager::load, no directory roots, keeping the current settings");
        return;
    }

    // Portable mode: the ini file (or its directory) exists but the installed
    // marker does not.
    m_portable = false;
    const QList<QString> portableRoots = m_roots.value("portableConfig");
    if (!portableRoots.isEmpty()) {
        const QString& root = portableRoots.front();
        m_portable = (QFile::exists(root) || QFile::exists(root + iniName))
            && !QFile::exists(root + installedName);
    }

    const QString iniPath = configDirectory(m_portable) + iniName;
    if (g_settings.isNull() || iniPath != g_settings->fileName()) {
        g_settings.reset(new QSettings(iniPath, QSettings::IniFormat));
    }

    const int nVersion = g_settings->value(OPTION_VERSION, OPTION_VERSION_DEFAULT).toInt();
    if (!QFile::exists(iniPath) || nVersion == OPTION_VERSION_DEFAULT) {
        // Ini file doesn't exist, create some defaults and save them to disk
        g_settings->setValue(OPTION_VERSION, OPTION_VERSION_DEFAULT);
        QList<Directory> directories = m_defaultDirs;
        writeCatalogDirectories(directories);
    }

    // An extra plugin directory is just another entry in the resolved roots,
    // so everything that walks directory("plugins") picks it up.
    const QString pluginExtraDir = g_settings->value(OPTION_PLUGIN_EXTRA_DIRECTORY).toString();
    if (!pluginExtraDir.isEmpty()) {
        m_roots["plugins"].push_back(pluginExtraDir);
    }
}

// Add the built-in catalog directories and list them first in a fixed order,
// stale user profile paths are fixed up first.
// The list is only modified in memory, the caller decides when to persist it
// and when to rebuild the catalog. Returns true when the list was modified.
bool SettingsManager::mergeDefaultCatalogDirectories(QList<Directory>& directories) {

    bool changed = fixCatalogUserPaths(directories);

    if (m_defaultDirs.isEmpty()) {
        return changed;
    }

    const QList<Directory>& required = m_defaultDirs;

    QList<Directory> rest = directories;
    QList<Directory> ordered;

    // Move the required directories to the front, keeping their own settings
    foreach(const Directory& req, required) {
        int found = -1;
        for (int i = 0; i < rest.count(); ++i) {
            if (isSameDirectoryPath(rest[i].name, req.name)) {
                found = i;
                break;
            }
        }

        if (found >= 0) {
            ordered.append(rest.takeAt(found));
        }
        else {
            qInfo("SettingsManager::mergeDefaultCatalogDirectories, adding %s",
                  qPrintable(req.name));
            ordered.append(req);
        }
    }

    // Keep the other directories, but drop duplicates of the required ones
    foreach(const Directory& dir, rest) {
        bool duplicate = false;
        foreach(const Directory& req, required) {
            if (isSameDirectoryPath(dir.name, req.name)) {
                duplicate = true;
                break;
            }
        }

        if (!duplicate) {
            ordered.append(dir);
        }
    }

    bool same = ordered.count() == directories.count();
    for (int i = 0; same && i < ordered.count(); ++i) {
        same = ordered[i].name == directories[i].name;
    }

    if (same) {
        return changed;
    }

    directories = ordered;
    return true;
}

bool SettingsManager::isPortable() const {
    return m_portable;
}

// Get the configuration directory of the current mode
QString SettingsManager::configDirectory() const {
    return configDirectory(m_portable);
}

QList<QString> SettingsManager::directory(const QString& name) const {
    return m_roots.value(name);
}

QString SettingsManager::catalogFilename() const {
    return configDirectory(m_portable) + catalogName;
}

QString SettingsManager::historyFilename() const {
    return configDirectory(m_portable) + historyName;
}

// Find the skin with the specified name ensuring that it contains at least a stylesheet
QString SettingsManager::skinPath(const QString& skinName) const {
    QString found;

    foreach(QString dir, directory("skins")) {
        dir += QString("/") + skinName + "/";
        if (QFile::exists(dir + "style.qss")) {
            found = dir;
            break;
        }
    }

    return found;
}

// Switch between portable and installed mode
bool SettingsManager::setPortable(bool makePortable) {
    if (makePortable == m_portable) {
        return true;
    }

    qInfo("SettingsManager::setPortable, Converting to %s mode",
          makePortable ? "portable" : "installed");

    // Destroy the QSettings object first so it writes every changes to disk
    g_settings.clear();

    QString oldDir = configDirectory(m_portable);
    QString oldIniPath = oldDir + iniName;
    QString oldCatalogPath = oldDir + catalogName;
    QString oldHistoryPath = oldDir + historyName;

    // Copy the settings to the new location
    // and delete the original settings if they are copied successfully
    // The history and catalog files are optional: the history only exists
    // once something was launched or promoted, and the catalog only exists
    // once a directory has been indexed, so a fresh profile may have
    // neither to copy.
    QString newDir = configDirectory(makePortable);
    QDir(newDir).mkpath(".");

    const bool copied =
        QFile::copy(oldIniPath, newDir + iniName)
        && (!QFile::exists(oldCatalogPath) || QFile::copy(oldCatalogPath, newDir + catalogName))
        && (!QFile::exists(oldHistoryPath) || QFile::copy(oldHistoryPath, newDir + historyName));

    if (!copied) {
        qWarning("SettingsManager::setPortable, could not convert to %s mode (new directory: %s)",
                 makePortable ? "portable" : "installed", qPrintable(newDir));
        // Nothing moved, so the mode stays as it was. Reopen the old ini file:
        // g_settings was cleared above and every caller dereferences it.
        load(m_roots, m_defaultDirs);
        return false;
    }

    QFile::remove(oldIniPath);
    QFile::remove(oldCatalogPath);
    QFile::remove(oldHistoryPath);
    QFile::remove(newDir + installedName);

    if (!makePortable && !QDir(oldDir).rmdir(".")) {
        // if converting to installed mode,
        // try to remove portable mode config directory if it is empty.
        // !! This may be dangerous if the old directory could contain other files.
        // !! MUST be careful when deleting files and directories.
        QFile file(oldDir + installedName);
        file.open(QIODevice::WriteOnly);
    }

    m_portable = makePortable;
    load(m_roots, m_defaultDirs);
    return true;
}

// Delete all settings files in both installed and portable directories
void SettingsManager::removeAll() {
    QFile::remove(configDirectory(false) + iniName);
    QFile::remove(configDirectory(false) + catalogName);
    QFile::remove(configDirectory(false) + historyName);

    QFile::remove(configDirectory(true) + iniName);
    QFile::remove(configDirectory(true) + catalogName);
    QFile::remove(configDirectory(true) + historyName);
}

// Get the configuration directory
QString SettingsManager::configDirectory(bool portable) const {
    const QList<QString> roots = m_roots.value(portable ? "portableConfig" : "config");
    if (roots.isEmpty()) {
        return QString();
    }

    QString result = roots.front();
    if (!m_profileName.isEmpty()) {
        result += "/profiles/" + m_profileName;
    }
    return result;
}

void SettingsManager::setProfileName(const QString& name) {
    m_profileName = name;
}

QList<Directory> SettingsManager::readCatalogDirectories() {
    QList<Directory> result;
    if (g_settings.isNull()) {
        return result;
    }

    // Use a local copy instead of the shared g_settings instance: this method
    // is called from the catalog builder worker thread while the GUI thread may
    // read/write the same QSettings. QSettings keeps group/array state, so two
    // threads sharing one instance is not safe. A separate instance bound to the
    // same file reads independently and never disturbs the shared one.
    QSettings settings(g_settings->fileName(), QSettings::IniFormat);
    int size = settings.beginReadArray("directories");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        Directory tmp;
        tmp.name = settings.value("name").toString();
        if (tmp.name.length() > 0) {
            tmp.types = settings.value("types").toStringList();
            tmp.indexDirs = settings.value("indexDirs", false).toBool();
            tmp.indexExe = settings.value("indexExes", false).toBool();
            tmp.depth = settings.value("depth", 100).toInt();
            result.append(tmp);
        }
    }
    settings.endArray();

    return result;
}

void SettingsManager::writeCatalogDirectories(QList<Directory>& directories) {
    if (g_settings.isNull()) {
        return;
    }

    g_settings->beginWriteArray("directories", directories.count());
    for (int i = 0; i < directories.count(); ++i) {
        if (directories[i].name.length() > 0) {
            g_settings->setArrayIndex(i);
            g_settings->setValue("name", directories[i].name);
            g_settings->setValue("types", directories[i].types);
            g_settings->setValue("indexDirs", directories[i].indexDirs);
            g_settings->setValue("indexExes", directories[i].indexExe);
            g_settings->setValue("depth", directories[i].depth);
        }
    }
    g_settings->endArray();
}
} // namespace launchy
