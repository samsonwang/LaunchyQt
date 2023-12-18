/*
LaunchyQt
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
#include <QPoint>

namespace launchy {

// general
extern const char*      OPTION_VERSION;
extern const int        OPTION_VERSION_DEFAULT;

extern const char*      OPTION_POS;
extern const QPoint     OPTION_POS_DEFAULT;

extern const char*      OPTION_HOTKEY;
extern const int        OPTION_HOTKEY_DEFAULT;

extern const char*      OPTION_HOTKEYMOD;
extern const int        OPTION_HOTKEYMOD_DEFAULT;

extern const char*      OPTION_HOTKEYKEY;
extern const int        OPTION_HOTKEYKEY_DEFAULT;

extern const char*      OPTION_IGNORE_FULL_SCREEN;
extern const bool       OPTION_IGNORE_FULL_SCREEN_DEFAULT;

extern const char*      OPTION_ALWAYSSHOW;
extern const bool       OPTION_ALWAYSSHOW_DEFAULT;

extern const char*      OPTION_ALWAYSTOP;
extern const bool       OPTION_ALWAYSTOP_DEFAULT;

extern const char*      OPTION_HIDEIFLOSTFOCUS;
extern const bool       OPTION_HIDEIFLOSTFOCUS_DEFAULT;

extern const char*      OPTION_ALWAYSCENTER;
extern const int        OPTION_ALWAYSCENTER_DEFAULT;

extern const char*      OPTION_DRAGMODE;
extern const bool       OPTION_DRAGMODE_DEFAULT;

static const char*      OPTION_SCREEN_INDEX = "General/ScreenIndex";
static const int        OPTION_SCREEN_INDEX_DEFAULT = 0;

static const char*      OPTION_HIDE_TRAY_ICON = "GenOps/HideTrayIcon";
static const bool       OPTION_HIDE_TRAY_ICON_DEFAULT = false;

extern const char*      OPTION_APPSTYLE;
extern const char*      OPTION_APPSTYLE_DEFAULT;

extern const char*      OPTION_DECORATETEXT;
extern const bool       OPTION_DECORATETEXT_DEFAULT;

extern const char*      OPTION_AUTOSUGGESTDELAY;
extern const int        OPTION_AUTOSUGGESTDELAY_DEFAULT;

extern const char*      OPTION_NUMVIEWABLE;
extern const int        OPTION_NUMVIEWABLE_DEFAULT;

extern const char*      OPTION_NUMRESULT;
extern const int        OPTION_NUMRESULT_DEFAULT;

extern const char*      OPTION_MAXITEMSINHISTORY;
extern const int        OPTION_MAXITEMSINHISTORY_DEFAULT;

extern const char*      OPTION_CONDENSEDVIEW;
extern const int        OPTION_CONDENSEDVIEW_DEFAULT;

extern const char*      OPTION_OPAQUENESS;
extern const int        OPTION_OPAQUENESS_DEFAULT;

extern const char*      OPTION_FADEIN;
extern const int        OPTION_FADEIN_DEFAULT;

extern const char*      OPTION_FADEOUT;
extern const int        OPTION_FADEOUT_DEFAULT;

// skin
extern const char*      OPTION_SKIN;
extern const char*      OPTION_SKIN_DEFAULT;

// update
extern const char*      OPTION_UPDATE_CHECK_ON_STARTUP;
extern const bool       OPTION_UPDATE_CHECK_ON_STARTUP_DEFAULT;

extern const char*      OPTION_UPDATE_CHECK_ON_STARTUP_DELAY;
extern const int        OPTION_UPDATE_CHECK_ON_STARTUP_DELAY_DEFAULT;

extern const char*      OPTION_UPDATE_CHECK_INTERVAL;
extern const int        OPTION_UPDATE_CHECK_INTERVAL_DEFAULT;

extern const char*      OPTION_UPDATE_LAST_CHECK;
extern const char*      OPTION_UPDATE_LAST_CHECK_DEFAULT;

// proxy
extern const char*      OPTION_PROXY_TYPE;
extern const int        OPTION_PROXY_TYPE_DEFAULT;

extern const char*      OPTION_PROXY_SERVER_NAME;
extern const char*      OPTION_PROXY_SERVER_NAME_DEFAULT;

extern const char*      OPTION_PROXY_SERVER_PORT;
extern const char*      OPTION_PROXY_SERVER_PORT_DEFAULT;

extern const char*      OPTION_PROXY_REQUIRE_PASSWORD;
extern const bool       OPTION_PROXY_REQUIRE_PASSWORD_DEFAULT;

extern const char*      OPTION_PROXY_USERNAME;
extern const char*      OPTION_PROXY_USERNAME_DEFAULT;

extern const char*      OPTION_PROXY_PASSWORD;
extern const char*      OPTION_PROXY_PASSWORD_DEFAULT;

// system
extern const char*      OPTION_REBUILDTIMER;
extern const int        OPTION_REBUILDTIMER_DEFAULT;

extern const char*      OPTION_SHOWHIDDENFILES;
extern const bool       OPTION_SHOWHIDDENFILES_DEFAULT;

extern const char*      OPTION_SHOWNETWORK;
extern const bool       OPTION_SHOWNETWORK_DEFAULT;

extern const char*      OPTION_LOGLEVEL;
extern const int        OPTION_LOGLEVEL_DEFAULT;

extern const char*      OPTION_LANGUAGE;
extern const char*      OPTION_LANGUAGE_DEFAULT;

} // namespace launchy
