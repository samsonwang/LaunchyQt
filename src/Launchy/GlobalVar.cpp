/*
Launchy: Application Launcher
Copyright (C) 2007-2010  Josh Karlin, Simon Capewell

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

#include "GlobalVar.h"
#include "AppBase.h"
#include "LaunchyWidget.h"
#include "CatalogBuilder.h"

#include "LaunchyLib/LaunchyLib.h"

namespace launchy {

const QString NAME_LAUNCHY = QStringLiteral("launchy");
const QString NAME_HISTORY = QStringLiteral("launchy_history");
const QString NAME_LAUNCHYFILE = QStringLiteral("launchy_file");

const QString LABEL_FILE = QStringLiteral("launchy_file");
const QString LABEL_AUTOSUGGEST = QStringLiteral("launchy_auto_suggest");
const QString LABEL_HISTORY = QStringLiteral("launchy_history");

QString g_searchText;

void cleanupGlobalVar() {

    CatalogBuilder::cleanup();
    LaunchyWidget::cleanup();
    AppBase::cleanup();
    g_settings.clear();

    qInfo("cleanupGlobalVar, cleanup finished");
}

} // namespace launchy
