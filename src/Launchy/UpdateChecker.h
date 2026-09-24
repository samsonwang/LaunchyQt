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

#pragma once

#include <QObject>
#include <QUrl>

class QNetworkAccessManager;
class QTimer;
class QNetworkReply;

namespace launchy {

class UpdateChecker : public QObject {
    Q_OBJECT
public:
    static UpdateChecker& instance();
    void startup();
    void reloadConfig();
    void manualCheck();

    // latest version detected by update check
    bool hasNewVersion() const;
    QString latestVersionTag() const;
    QUrl releaseUrl() const;

signals:
    // emitted when a version newer than the current one is detected
    void updateAvailable(const QString& versionTag, const QUrl& releaseUrl);

private:
    UpdateChecker();
    Q_DISABLE_COPY(UpdateChecker)

private slots:
    void getVersionInfo();
    void replyFinished(QNetworkReply* reply);

private:
    QNetworkAccessManager* m_mgr;
    QTimer* m_timerStartup;
    bool m_manualCheck;
    bool m_hasNewVersion;
    QString m_latestVersionTag;
    QUrl m_releaseUrl;
};

} // namespace launchy
