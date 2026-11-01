/*
LaunchyQt
Copyright (C) 2019 Samson Wang

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "AppSettings.h"

#include <QApplication>
#include <QLocale>
#include <QNetworkProxy>
#include <QStyleFactory>

#include "SettingsManager.h"
#include "Logger.h"
#include "TranslationManager.h"
#include "LaunchyVersion.h"

namespace launchy {

void applyRuntimeSettings() {
    SettingsManager& settings = SettingsManager::instance();
    const RuntimeSettings& runtime = settings.runtime();

    log::setDebugLogEnabled(runtime.debugLog);

    qInfo() << "Launchy version:" << LAUNCHY_VERSION_STRING
        << "(git " << LAUNCHY_GIT_HASH
        << "build " << LAUNCHY_BUILD_TIME_STRING << ")";
    qInfo("Loading settings in %s mode from %s",
          settings.isPortable() ? "portable" : "installed",
          qPrintable(settings.configDirectory()));

    // set application style
    qApp->setStyle(QStyleFactory::create(runtime.appStyle));

    // load proxy. RuntimeSettings carries the type as a plain int so that core
    // does not have to name a QtNetwork enum; this is the one place it becomes
    // a real QNetworkProxy::ProxyType again.
    QNetworkProxy proxy;
    proxy.setType(static_cast<QNetworkProxy::ProxyType>(runtime.proxy.type));
    proxy.setHostName(runtime.proxy.host);
    proxy.setPort(runtime.proxy.port);
    if (runtime.proxy.requirePassword) {
        proxy.setUser(runtime.proxy.user);
        proxy.setPassword(runtime.proxy.password);
    }
    QNetworkProxy::setApplicationProxy(proxy);

    // load language translation
    if (runtime.language.isEmpty()) {
        TranslationManager::instance().setLocale(QLocale::system());
    }
    else {
        TranslationManager::instance().setLocale(QLocale(runtime.language));
    }
}

} // namespace launchy
