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

// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif


namespace launchy {

const int LAUNCHY_VERSION = 315;
const char* LAUNCHY_VERSION_STRING = "3.1.5";

#if defined(ENVIRONMENT64)
const char* LAUNCHY_BIT_STRING = "64";
#elif defined(ENVIRONMENT32)
const char* LAUNCHY_BIT_STRING = "32";
#else
const char* LAUNCHY_BIT_STRING = "N/A";
#endif

const QString NAME_LAUNCHY = QStringLiteral("launchy");
const QString NAME_HISTORY = QStringLiteral("launchy_history");
const QString NAME_LAUNCHYFILE = QStringLiteral("launchy_file");

const QString LABEL_FILE = "launchy_file";
const QString LABEL_AUTOSUGGEST = "launchy_auto_suggest";
const QString LABEL_HISTORY = "launchy_history";

QString g_searchText;

void cleanupGlobalVar() {

    CatalogBuilder::cleanup();
    LaunchyWidget::cleanup();
    AppBase::cleanup();
    g_settings.clear();

    qInfo("cleanupGlobalVar, cleanup finished");
}

} // namespace launchy
