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
#include "GlobalVar.h"
#include "OptionItem.h"
#include "LaunchyWidget.h"

// for QNetworkProxy::ProxyType in QVariant
Q_DECLARE_METATYPE(QNetworkProxy::ProxyType)

namespace launchy {

UpdateChecker::UpdateChecker()
    : m_mgr(new QNetworkAccessManager(this)),
      m_timerStartup(new QTimer(this)),
      m_manualCheck(false) {

    connect(m_mgr, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));

    m_timerStartup->setSingleShot(true);
    connect(m_timerStartup, SIGNAL(timeout()),
            this, SLOT(getVersionInfo()));
}

UpdateChecker& UpdateChecker::instance() {
    static UpdateChecker s_obj;
    return s_obj;
}

void UpdateChecker::startup() {
    bool checkOnStartup = g_settings->value(OPTION_UPDATE_CHECK_ON_STARTUP,
                                            OPTION_UPDATE_CHECK_ON_STARTUP_DEFAULT).toBool();
    if (!checkOnStartup) {
        qDebug() << "UpdateChecker::startup, no startup check";
        return;
    }

    int interval = g_settings->value(OPTION_UPDATE_CHECK_INTERVAL,
                                     OPTION_UPDATE_CHECK_INTERVAL_DEFAULT).toInt();

    QString lastUpdate = g_settings->value(OPTION_UPDATE_LAST_CHECK,
                                           OPTION_UPDATE_LAST_CHECK_DEFAULT).toString();

    QDateTime last = QDateTime::fromString(lastUpdate, "yyyy-MM-dd HH:mm:ss");
    QDateTime now = QDateTime::currentDateTime();

    if (now < last.addSecs(interval * 3600)) {
        qDebug() << "UpdateChecker::startup, last update time:" << lastUpdate
            << ", wait for next startup";
        return;
    }

    int delay = g_settings->value(OPTION_UPDATE_CHECK_ON_STARTUP_DELAY,
                                  OPTION_UPDATE_CHECK_ON_STARTUP_DELAY_DEFAULT).toInt();

    qDebug() << "UpdateChecker::startup, will update in" << delay << "seconds";

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
    qDebug() << "UpdateChecker::manualCheck, doing manual check";
    m_manualCheck = true;
    getVersionInfo();
}

void UpdateChecker::getVersionInfo() {
    m_mgr->get(QNetworkRequest(QUrl("https://launchyqt.com/version.xml")));
    qDebug() << "UpdateChecker::checkUpdate, getting version info";
}

void UpdateChecker::replyFinished(QNetworkReply* reply) {
    QByteArray info = reply->readAll();
    qDebug() << "UpdateChecker::replyFinished, content:" << info;

    // parse xml
    // https://stackoverflow.com/questions/3092387/parse-a-xml-file-in-qt
    int lastestVersion = LAUNCHY_VERSION;
    QXmlStreamReader reader(info);
    while (!reader.atEnd() && !reader.hasError()) {
        QXmlStreamReader::TokenType token = reader.readNext();
        if (token == QXmlStreamReader::StartDocument) {
            continue;
        }
        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == "latest") {
                QXmlStreamAttributes attr = reader.attributes();
                if (attr.hasAttribute("version")) {
                    lastestVersion = attr.value("version").toInt();
                    qInfo() << "UpdateChecker::replyFinished, latest version:" << lastestVersion;
                }
            }
        }
    }

    if (reader.hasError()) {
        qWarning() << "UpdateChecker::replyFinished, xml error:"
            << reader.errorString();
    }

    if (lastestVersion > LAUNCHY_VERSION) {
        g_mainWidget->trayNotify(tr("A new version is available."));
    }
    else if (m_manualCheck) {
        m_manualCheck = false;
        g_mainWidget->trayNotify(tr("No update is available."));
    }

    // write to setting
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    g_settings->setValue(OPTION_UPDATE_LAST_CHECK, now);

    reader.clear();
}

}
