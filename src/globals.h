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

#ifndef GLOBALS_H
#define GLOBALS_H


#include "SettingsManager.h"
#include <QSettings> // need this for os x
#define LAUNCHY_VERSION 300
#define LAUNCHY_VERSION_STRING "3.0.0 alpha"

#define HASH_LAUNCHY 0
#define HASH_HISTORY 1
#define HASH_LAUNCHYFILE 2
#define LABEL_FILE 0
#define LABEL_AUTOSUGGEST 1
#define LABEL_HISTORY 2

class LaunchyWidget;
class PlatformBase;
class CatalogBuilder;

extern PlatformBase* g_platform;
extern LaunchyWidget* g_mainWidget;
extern QSettings* g_settings;
extern SettingsManager g_settingMgr;
extern CatalogBuilder* g_builder;
extern QString g_searchText;


#endif
