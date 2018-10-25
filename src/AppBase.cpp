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
#include <QFileIconProvider>
#include "Directory.h"
#include "catalog.h"
#include "globals.h"

AppBase::AppBase(int& argc, char** argv)
    : SingleApplication(argc, argv, false, Mode::User, 100),
      m_iconProvider(nullptr) {
    g_app.reset(this);
    setQuitOnLastWindowClosed(false);
    setApplicationName("Launchy");
    setOrganizationDomain("Launchy");


}

AppBase::~AppBase() {
    if (m_iconProvider) {
        delete m_iconProvider;
        m_iconProvider = NULL;
    }
}

QIcon AppBase::icon(const QFileInfo& info) {
    return m_iconProvider->icon(info);
}

QIcon AppBase::icon(QFileIconProvider::IconType type) {
    return m_iconProvider->icon(type);
}

bool AppBase::isAlreadyRunning() const {
    return this->isSecondary();
}

void AppBase::sendInstanceCommand(int command) {
    Q_UNUSED(command);
}

void AppBase::alterItem(CatItem*) {
}

bool AppBase::supportsAlphaBorder() const {
    return false;
}

bool AppBase::getComputers(QStringList& computers) const {
    Q_UNUSED(computers);
    return false;
}
