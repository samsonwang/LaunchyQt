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

#include "AppBase.h"
#include <QFileInfo>
#include "Directory.h"
#include "CatalogItem.h"
#include "IconProviderBase.h"

namespace launchy {

static auto HIDPI = []() {
    // hidpi detecttion
    static const char ENV_VAR_QT_DEVICE_PIXEL_RATIO[] = "QT_DEVICE_PIXEL_RATIO";
    if (!qEnvironmentVariableIsSet(ENV_VAR_QT_DEVICE_PIXEL_RATIO)
        && !qEnvironmentVariableIsSet("QT_AUTO_SCREEN_SCALE_FACTOR")
        && !qEnvironmentVariableIsSet("QT_SCALE_FACTOR")
        && !qEnvironmentVariableIsSet("QT_SCREEN_SCALE_FACTORS")) {
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    }
    return 0;
}();

AppBase::AppBase(int& argc, char** argv)
    : SingleApplication(argc, argv, false, Mode::User),
      m_iconProvider(nullptr) {
    setQuitOnLastWindowClosed(false);
    setApplicationName("LaunchyQt");
    setOrganizationName("LaunchyQt");
    setOrganizationDomain("launchyqt.com");

}

AppBase::~AppBase() {
    if (m_iconProvider) {
        delete m_iconProvider;
        m_iconProvider = NULL;
    }
}

void AppBase::cleanup() {
    if (g_app) {
        delete g_app;
    }
};

QIcon AppBase::icon(const QFileInfo& info) {
    if (m_iconProvider) {
        return m_iconProvider->icon(info);
    }
    return QIcon();
}

QIcon AppBase::icon(QFileIconProvider::IconType type) {
    if (m_iconProvider) {
        return m_iconProvider->icon(type);
    }
    return QIcon();
}

void AppBase::setPreferredIconSize(int size) {
    if (m_iconProvider) {
        m_iconProvider->setPreferredIconSize(size);
    }
}

bool AppBase::isAlreadyRunning() const {
    return this->isSecondary();
}

void AppBase::sendInstanceCommand(int command) {
    Q_UNUSED(command)
}

void AppBase::alterItem(CatItem* item) {
    Q_UNUSED(item)
}

bool AppBase::supportsAlphaBorder() const {
    return false;
}

bool AppBase::getComputers(QStringList& computers) const {
    Q_UNUSED(computers);
    return false;
}
}
