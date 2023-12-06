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

#include <QMessageBox>
#include <QNetworkProxy>
#include <QStyleFactory>
#include <QDir>

#include "LaunchyWidget.h"
#include "GlobalVar.h"
#include "AppBase.h"
#include "Logger.h"
#include "OptionItem.h"
#include "TranslationManager.h"
#include "LaunchyVersion.h"

static const char* iniName = "/launchy.ini";
static const char* dbName = "/launchy.db";
static const char* historyName = "/history.db";
static const char* installedName = "/.installed";

// for QNetworkProxy::ProxyType in QVariant
Q_DECLARE_METATYPE(QNetworkProxy::ProxyType)

namespace launchy {
SettingsManager::SettingsManager()
    : m_portable(false) {
}

SettingsManager& SettingsManager::instance() {
    static SettingsManager s_obj;
    return s_obj;
}

void SettingsManager::load() {
    if (!g_app) {
        return;
    }

    // load settings
    m_dirs = g_app->getDirectories();

    if (m_dirs.contains("portableConfig")) {
        QList<QString>& lst = m_dirs["portableConfig"];
        if (!lst.empty()) {
            QString strName = lst.front();
            m_portable |= QFile::exists(strName);
            m_portable |= QFile::exists(strName + iniName);
            m_portable &= !QFile::exists(strName + installedName);
        }
        else {
            m_portable = false;
        }
    }
    else {
        m_portable = false;
    }

    QString iniPath = configDirectory(m_portable) + iniName;
    g_settings.reset(new QSettings(iniPath, QSettings::IniFormat));
    int nVersion = g_settings->value(OPTION_VERSION, OPTION_VERSION_DEFAULT).toInt();

    if (!QFile::exists(iniPath) || nVersion == OPTION_VERSION_DEFAULT) {
        // Ini file doesn't exist, create some defaults and save them to disk
        g_settings->setValue(OPTION_VERSION, OPTION_VERSION_DEFAULT);
        QList<Directory> directories = g_app->getDefaultCatalogDirectories();
        writeCatalogDirectories(directories);
    }

    int logLevel = g_settings->value(OPTION_LOGLEVEL, OPTION_LOGLEVEL_DEFAULT).toInt();
    Logger::setLogLevel(logLevel);

    qInfo() << "Launchy version:" << LAUNCHY_VERSION_STRING
        << "( build" << __DATE__ << __TIME__ << ")";
    qInfo("Loading settings in %s mode from %s",
          m_portable ? "portable" : "installed", qPrintable(configDirectory(m_portable)));

    // set application style
    QString appStyle = g_settings->value(OPTION_APPSTYLE, OPTION_APPSTYLE_DEFAULT).toString();
    qApp->setStyle(QStyleFactory::create(appStyle));

    // load proxy
    QNetworkProxy::ProxyType proxyType
        = g_settings->value(OPTION_PROXY_TYPE,
                            OPTION_PROXY_TYPE_DEFAULT).value<QNetworkProxy::ProxyType>();

    QString serverName = g_settings->value(OPTION_PROXY_SERVER_NAME,
                                           OPTION_PROXY_SERVER_NAME_DEFAULT).toString();
    QString serverPort = g_settings->value(OPTION_PROXY_SERVER_PORT,
                                           OPTION_PROXY_SERVER_PORT_DEFAULT).toString();
    bool requirePassword = g_settings->value(OPTION_PROXY_REQUIRE_PASSWORD,
                                             OPTION_PROXY_REQUIRE_PASSWORD_DEFAULT).toBool();

    QNetworkProxy proxy;
    proxy.setType(proxyType);
    proxy.setHostName(serverName);
    proxy.setPort(serverPort.toUInt());
    if (requirePassword) {
        QString username = g_settings->value(OPTION_PROXY_USERNAME,
                                             OPTION_PROXY_USERNAME_DEFAULT).toString();
        QString password = g_settings->value(OPTION_PROXY_PASSWORD,
                                             OPTION_PROXY_PASSWORD_DEFAULT).toString();
        proxy.setUser(username);
        proxy.setPassword(password);
    }

    QNetworkProxy::setApplicationProxy(proxy);

    // load language translation
    QString lang = g_settings->value(OPTION_LANGUAGE, OPTION_LANGUAGE_DEFAULT).toString();
    if (lang.isEmpty()) {
        TranslationManager::instance().setLocale(QLocale::system());
    }
    else {
        TranslationManager::instance().setLocale(QLocale(lang));
    }
}

bool SettingsManager::isPortable() const {
    return m_portable;
}

QList<QString> SettingsManager::directory(QString name) const {
    return m_dirs[name];
}

QString SettingsManager::catalogFilename() const {
    return configDirectory(m_portable) + dbName;
}

QString SettingsManager::historyFilename() const {
    return configDirectory(m_portable) + historyName;
}

// Find the skin with the specified name ensuring that it contains at least a stylesheet
QString SettingsManager::skinPath(const QString& skinName) const {
    QString directory;

    foreach(QString dir, m_dirs["skins"]) {
        dir += QString("/") + skinName + "/";
        if (QFile::exists(dir + "style.qss")) {
            directory = dir;
            break;
        }
    }

    return directory;
}

// Switch between portable and installed mode
void SettingsManager::setPortable(bool makePortable) {
    if (makePortable == m_portable) {
        return;
    }

    qInfo("SettingsManager::setPortable, Converting to %s mode",
          makePortable ? "portable" : "installed");

    // Destroy the QSettings object first so it writes every changes to disk
    g_settings.clear();

    QString oldDir = configDirectory(m_portable);
    QString oldIniName = oldDir + iniName;
    QString oldDbName = oldDir + dbName;
    QString oldHistoryName = oldDir + historyName;

    // Copy the settings to the new location
    // and delete the original settings if they are copied successfully
    QString newDir = configDirectory(makePortable);
    QDir(newDir).mkpath(".");
    if (QFile::copy(oldIniName, newDir + iniName)
        && QFile::copy(oldDbName, newDir + dbName)
        && QFile::copy(oldHistoryName, newDir + historyName)) {
        QFile::remove(oldIniName);
        QFile::remove(oldDbName);
        QFile::remove(oldHistoryName);
        QFile::remove(newDir + installedName);

        if (!makePortable && !QDir(oldDir).rmdir(".")) {
            // if converting to installed mode,
            // try to remove portable mode config directory if it is empty.
            // !! This may be dangerous if the old directory could contain other files.
            // !! MUST be careful when deleting files and directories.
            QFile file(oldDir + installedName);
            file.open(QIODevice::WriteOnly);
        }
    }
    else {
        qWarning("Could not convert to %s mode", makePortable ? "portable" : "installed");

        QMessageBox::warning(g_app->activeModalWidget(),
                             QString("Launchy"),
                             qApp->translate("launchy::OptionDialog",
                                             "Fail to convert to %1 mode.\n"
                                             "Please check directory:\n %2")
                             .arg(makePortable ? "portable" : "installed")
                             .arg(newDir));


    }

    m_portable = makePortable;

    load();
}

// Delete all settings files in both installed and portable directories
void SettingsManager::removeAll() {
    QFile::remove(configDirectory(false) + iniName);
    QFile::remove(configDirectory(false) + dbName);
    QFile::remove(configDirectory(false) + historyName);

    QFile::remove(configDirectory(true) + iniName);
    QFile::remove(configDirectory(true) + dbName);
    QFile::remove(configDirectory(true) + historyName);
}

// Get the configuration directory
QString SettingsManager::configDirectory(bool portable) const {
    QString result = m_dirs[portable ? "portableConfig" : "config"][0];
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
    int size = g_settings->beginReadArray("directories");
    for (int i = 0; i < size; ++i) {
        g_settings->setArrayIndex(i);
        Directory tmp;
        tmp.name = g_settings->value("name").toString();
        if (tmp.name.length() > 0) {
            tmp.types = g_settings->value("types").toStringList();
            tmp.indexDirs = g_settings->value("indexDirs", false).toBool();
            tmp.indexExe = g_settings->value("indexExes", false).toBool();
            tmp.depth = g_settings->value("depth", 100).toInt();
            result.append(tmp);
        }
    }
    g_settings->endArray();

    return result;
}

void SettingsManager::writeCatalogDirectories(QList<Directory>& directories) {
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
}
