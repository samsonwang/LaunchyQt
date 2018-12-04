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
      m_repeatInterval(OPTION_UPDATE_CHECK_REPEAT_INTERVAL_DEFAULT * 1000 * 3600),
      m_timerRepeat(new QTimer(this)) {

    connect(m_mgr, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));

    m_timerStartup->setSingleShot(true);
    connect(m_timerStartup, SIGNAL(timeout()),
            this, SLOT(checkUpdate()));

    m_timerRepeat->setSingleShot(false);
    connect(m_timerRepeat, SIGNAL(timeout()),
            this, SLOT(checkUpdate()));
}

UpdateChecker& UpdateChecker::instance() {
    static UpdateChecker s_obj;
    return s_obj;
}

void UpdateChecker::startup() {
//     QNetworkProxy::ProxyType proxyType
//         = g_settings->value(OPTION_PROXY_TYPE,
//                             OPTION_PROXY_TYPE_DEFAULT).value<QNetworkProxy::ProxyType>();

    bool checkOnStartup = g_settings->value(OPTION_UPDATE_CHECK_ON_STARTUP,
                                            OPTION_UPDATE_CHECK_ON_STARTUP_DEFAULT).toBool();
    if (!checkOnStartup) {
        qDebug() << "UpdateChecker::startup, no startup check";
        return;
    }

    int delay = g_settings->value(OPTION_UPDATE_CHECK_ON_STARTUP_DELAY,
                                  OPTION_UPDATE_CHECK_ON_STARTUP_DELAY_DEFAULT).toInt();

    qDebug() << "UpdateChecker::startup, check delay (second):" << delay;
    m_timerStartup->start(delay * 1000);

    bool checkRepeat = g_settings->value(OPTION_UPDATE_CHECK_REPEAT,
                                         OPTION_UPDATE_CHECK_REPEAT_DEFAULT).toBool();
    if (checkRepeat) {
        qDebug() << "UpdateChecker::startup, no repeat check";
        return;
    }

    int checkInterval = g_settings->value(OPTION_UPDATE_CHECK_REPEAT_INTERVAL,
                                          OPTION_UPDATE_CHECK_REPEAT_INTERVAL_DEFAULT).toInt();
    qDebug() << "UpdateChecker::startup, repeat check interval (hour):" << checkInterval;
    
    m_repeatInterval = checkInterval * 1000 * 3600;
    m_timerRepeat->start(m_repeatInterval);
}

void UpdateChecker::reloadConfig() {
    bool checkRepeat = g_settings->value(OPTION_UPDATE_CHECK_REPEAT,
                                         OPTION_UPDATE_CHECK_REPEAT_DEFAULT).toBool();
    if (checkRepeat) {
        qDebug() << "UpdateChecker::reloadConfig, no repeat check";
        return;
    }

    int checkInterval = g_settings->value(OPTION_UPDATE_CHECK_REPEAT_INTERVAL,
                                          OPTION_UPDATE_CHECK_REPEAT_INTERVAL_DEFAULT).toInt();
    qDebug() << "UpdateChecker::reloadConfig, repeat check interval (hour):" << checkInterval;

    m_repeatInterval = checkInterval * 1000 * 3600;
    m_timerRepeat->start(m_repeatInterval);
}

void UpdateChecker::checkUpdate() {
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
                    qDebug() << "UpdateChecker::replyFinished, latest version:" << lastestVersion;
                }
            }
        }
    }

    if (lastestVersion > LAUNCHY_VERSION) {
        g_mainWidget->trayNotify(tr("A new version of Launchy is available!"));
    }

    if (reader.hasError()) {
        qWarning() << "UpdateChecker::replyFinished, xml error:"
            << reader.errorString();
    }
    reader.clear();
}

}