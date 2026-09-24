/*
Launchy
Copyright (C) 2018 Samson Wang

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "UpdateChecker.h"

#include <QMetaType>
#include <QNetworkProxy>
#include <QNetworkAccessManager>
#include <QTimer>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <QDebug>

#include "GlobalVar.h"
#include "OptionItem.h"
#include "LaunchyWidget.h"
#include "LaunchyVersion.h"

// for QNetworkProxy::ProxyType in QVariant
Q_DECLARE_METATYPE(QNetworkProxy::ProxyType)

namespace launchy {

// convert github release tag "v3.1.8" to LAUNCHY_VERSION format,
// returns 0 if tag is not in "vMAJOR.MINOR.PATCH" format
static unsigned parseReleaseTag(const QString& tag) {
    QString version = tag.trimmed();
    if (version.startsWith(QLatin1Char('v')) || version.startsWith(QLatin1Char('V'))) {
        version = version.mid(1);
    }
    if (version.count(QLatin1Char('.')) != 2) {
        return 0;
    }

    bool okMajor = false;
    bool okMinor = false;
    bool okPatch = false;
    const unsigned major = version.section(QLatin1Char('.'), 0, 0).toUInt(&okMajor);
    const unsigned minor = version.section(QLatin1Char('.'), 1, 1).toUInt(&okMinor);
    const unsigned patch = version.section(QLatin1Char('.'), 2, 2).toUInt(&okPatch);
    if (!okMajor || !okMinor || !okPatch) {
        return 0;
    }
    return major * 10000 + minor * 100 + patch;
}

UpdateChecker::UpdateChecker()
    : m_mgr(new QNetworkAccessManager(this)),
      m_timerStartup(new QTimer(this)),
      m_manualCheck(false),
      m_hasNewVersion(false) {

    connect(m_mgr, &QNetworkAccessManager::finished,
            this, &UpdateChecker::replyFinished);

    m_timerStartup->setSingleShot(true);
    connect(m_timerStartup, &QTimer::timeout,
            this, &UpdateChecker::getVersionInfo);
}

UpdateChecker& UpdateChecker::instance() {
    static UpdateChecker s_obj;
    return s_obj;
}

bool UpdateChecker::hasNewVersion() const {
    return m_hasNewVersion;
}

QString UpdateChecker::latestVersionTag() const {
    return m_latestVersionTag;
}

QUrl UpdateChecker::releaseUrl() const {
    return m_releaseUrl;
}

void UpdateChecker::startup() {
    bool checkOnStartup = g_settings->value(OPTION_UPDATE_CHECK_ON_STARTUP,
                                            OPTION_UPDATE_CHECK_ON_STARTUP_DEFAULT).toBool();
    if (!checkOnStartup) {
        qInfo() << "UpdateChecker::startup, no startup check";
        return;
    }

    int interval = g_settings->value(OPTION_UPDATE_CHECK_INTERVAL,
                                     OPTION_UPDATE_CHECK_INTERVAL_DEFAULT).toInt();

    QString lastUpdate = g_settings->value(OPTION_UPDATE_LAST_CHECK,
                                           OPTION_UPDATE_LAST_CHECK_DEFAULT).toString();

    QDateTime last = QDateTime::fromString(lastUpdate, "yyyy-MM-dd HH:mm:ss");
    QDateTime now = QDateTime::currentDateTime();

    if (now < last.addSecs(interval * 3600)) {
        qInfo() << "UpdateChecker::startup, last update time:" << lastUpdate
            << ", wait for next startup";
        return;
    }

    int delay = g_settings->value(OPTION_UPDATE_CHECK_ON_STARTUP_DELAY,
                                  OPTION_UPDATE_CHECK_ON_STARTUP_DELAY_DEFAULT).toInt();

    qInfo() << "UpdateChecker::startup, will update in" << delay << "seconds";

    m_timerStartup->start(delay * 1000);
}

void UpdateChecker::reloadConfig() {
    bool checkOnStartup = g_settings->value(OPTION_UPDATE_CHECK_ON_STARTUP,
                                            OPTION_UPDATE_CHECK_ON_STARTUP_DEFAULT).toBool();

    if (!checkOnStartup && m_timerStartup->isActive()) {
        qDebug() << "UpdateChecker::reloadConfig, no update check";
        m_timerStartup->stop();
        return;
    }
}

void UpdateChecker::manualCheck() {
    if (m_manualCheck) {
        return;
    }
    qInfo() << "UpdateChecker::manualCheck, doing manual check";
    m_manualCheck = true;
    getVersionInfo();
}

void UpdateChecker::getVersionInfo() {
    qDebug() << "UpdateChecker::getVersionInfo, ssl supported:" << QSslSocket::supportsSsl()
             << ", build ssl version:" << QSslSocket::sslLibraryBuildVersionString()
             << ", runtime ssl version:" << QSslSocket::sslLibraryVersionString();

    if (!QSslSocket::supportsSsl()) {
        qWarning() << "UpdateChecker::getVersionInfo, SSL runtime unavailable,"
                   << "https request will fail with TLS initialization failed,"
                   << "deploy OpenSSL 1.1 x64 runtime DLLs (libssl-1_1-x64.dll,"
                   << "libcrypto-1_1-x64.dll) next to Launchy.exe";
    }

    // https://docs.github.com/en/rest/releases/releases#get-the-latest-release
    QNetworkRequest request(
        QUrl("https://api.github.com/repos/samsonwang/LaunchyQt/releases/latest"));
    // github api requires a user agent, otherwise it responds 403
    request.setRawHeader("User-Agent", "Launchy/" LAUNCHY_VERSION_STRING);
    request.setRawHeader("Accept", "application/vnd.github+json");
    m_mgr->get(request);
}

void UpdateChecker::replyFinished(QNetworkReply* reply) {
    if (!reply) {
        return;
    }
    reply->deleteLater();

    // report failure, manual user feedback on manual check only
    auto checkFailed = [this](const QString& reason) {
        qWarning() << "UpdateChecker::replyFinished," << reason;
        if (m_manualCheck) {
            m_manualCheck = false;
            g_mainWidget->trayNotify(tr("Failed to check for updates."));
        }
    };

    if (reply->error() != QNetworkReply::NoError) {
        const int status =
            reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString reason = QString("network error %1, http status %2: %3")
                         .arg(static_cast<int>(reply->error()))
                         .arg(status)
                         .arg(reply->errorString());
        if (!QSslSocket::supportsSsl()) {
            reason += QLatin1String(
                " (SSL runtime unavailable, check OpenSSL DLL deployment)");
        }
        checkFailed(reason);
        // do not record check time on failure, so next startup retries
        return;
    }

    const QByteArray info = reply->readAll();
    qDebug() << "UpdateChecker::replyFinished, content:" << info;

    // parse github api response
    QJsonParseError jsonError;
    const QJsonDocument doc = QJsonDocument::fromJson(info, &jsonError);
    if (jsonError.error != QJsonParseError::NoError) {
        checkFailed(QString("json parse error: %1").arg(jsonError.errorString()));
        return;
    }

    // https://docs.github.com/en/rest/releases/releases#get-the-latest-release
    const QJsonObject release = doc.object();
    const QString tagName = release.value(QLatin1String("tag_name")).toString();
    const QString releaseUrl = release.value(QLatin1String("html_url")).toString();

    const unsigned latestVersion = parseReleaseTag(tagName);
    if (latestVersion == 0) {
        checkFailed(QString("unexpected release tag: %1").arg(tagName));
        return;
    }

    qInfo() << "UpdateChecker::replyFinished, latest version:" << tagName
        << "(" << latestVersion << ")" << ", url:" << releaseUrl;

    if (latestVersion > LAUNCHY_VERSION) {
        m_hasNewVersion = true;
        m_latestVersionTag = tagName;
        m_releaseUrl = QUrl(releaseUrl);

        g_mainWidget->trayNotify(tr("A new version is available."),
                                 m_releaseUrl);
        emit updateAvailable(m_latestVersionTag, m_releaseUrl);
    }
    else if (m_manualCheck) {
        g_mainWidget->trayNotify(tr("No update is available."));
    }

    // reset flag so future manual checks are not blocked
    m_manualCheck = false;

    // write to setting
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    g_settings->setValue(OPTION_UPDATE_LAST_CHECK, now);
}

} // namespace launchy

