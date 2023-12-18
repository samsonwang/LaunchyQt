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

#include "OptionItem.h"

#include <Qt>
#include <QNetworkProxy>

namespace launchy {

const char*     OPTION_ALWAYSSHOW                             = "GenOps/alwaysshow";
const bool      OPTION_ALWAYSSHOW_DEFAULT                     = false;

const char*     OPTION_ALWAYSTOP                              = "GenOps/alwaystop";
const bool      OPTION_ALWAYSTOP_DEFAULT                      = false;

const char*     OPTION_LOGLEVEL                               = "GenOps/logLevel";
const int       OPTION_LOGLEVEL_DEFAULT                       = 2;

const char*     OPTION_DECORATETEXT                           = "GenOps/decoratetext";
const bool      OPTION_DECORATETEXT_DEFAULT                   = false;

const char*     OPTION_HIDEIFLOSTFOCUS                        = "GenOps/hideiflostfocus";
const bool      OPTION_HIDEIFLOSTFOCUS_DEFAULT                = true;

const char*     OPTION_ALWAYSCENTER                           = "GenOps/alwayscenter";
const int       OPTION_ALWAYSCENTER_DEFAULT                   = 3;

const char*     OPTION_DRAGMODE                               = "GenOps/dragmode";
const bool      OPTION_DRAGMODE_DEFAULT                       = false;

const char*     OPTION_APPSTYLE                                = "GenOps/appStyle";
const char*     OPTION_APPSTYLE_DEFAULT                        = "fusion";

const char*     OPTION_SHOWHIDDENFILES                        = "GenOps/showHiddenFiles";
const bool      OPTION_SHOWHIDDENFILES_DEFAULT                = false;

const char*     OPTION_SHOWNETWORK                            = "GenOps/showNetwork";
const bool      OPTION_SHOWNETWORK_DEFAULT                    = false;

const char*     OPTION_AUTOSUGGESTDELAY                       = "GenOps/autoSuggestDelay";
const int       OPTION_AUTOSUGGESTDELAY_DEFAULT               = 1000;

const char*     OPTION_REBUILDTIMER                           = "GenOps/rebuildTimer";
const int       OPTION_REBUILDTIMER_DEFAULT                   = 30;

const char*     OPTION_NUMVIEWABLE                            = "GenOps/numviewable";
const int       OPTION_NUMVIEWABLE_DEFAULT                    = 4;

const char*     OPTION_NUMRESULT                              = "GenOps/numresults";
const int       OPTION_NUMRESULT_DEFAULT                      = 10;

const char*     OPTION_MAXITEMSINHISTORY                      = "GenOps/maxitemsinhistory";
const int       OPTION_MAXITEMSINHISTORY_DEFAULT              = 20;

const char*     OPTION_OPAQUENESS                             = "GenOps/opaqueness";
const int       OPTION_OPAQUENESS_DEFAULT                     = 100;

const char*     OPTION_FADEIN                                 = "GenOps/fadein";
const int       OPTION_FADEIN_DEFAULT                         = 10;

const char*     OPTION_FADEOUT                                = "GenOps/fadeout";
const int       OPTION_FADEOUT_DEFAULT                        = 20;

const char*     OPTION_SKIN                                   = "GenOps/skin";
const char*     OPTION_SKIN_DEFAULT                           = "Default";

const char*     OPTION_CONDENSEDVIEW                          = "GenOps/condensedView";
const int       OPTION_CONDENSEDVIEW_DEFAULT                  = 2;

const char*     OPTION_HOTKEY                                 = "GenOps/hotkey";
const int       OPTION_HOTKEY_DEFAULT                         = -1;

const char*     OPTION_HOTKEYMOD                              = "GenOps/hotkeyModifier";
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
const int       OPTION_HOTKEYMOD_DEFAULT                      = Qt::AltModifier;
#elif defined(Q_OS_MAC)
const int       OPTION_HOTKEYMOD_DEFAULT                      = Qt::MetaModifier;
#endif

const char*     OPTION_HOTKEYKEY                              = "GenOps/hotkeyAction";
const int       OPTION_HOTKEYKEY_DEFAULT                      = Qt::Key_Space;

const char*     OPTION_VERSION                                = "version";
const int       OPTION_VERSION_DEFAULT                        = 0;

const char*     OPTION_POS                                    = "Display/pos";
const QPoint    OPTION_POS_DEFAULT                            = QPoint(0, 0);

const char*     OPTION_IGNORE_FULL_SCREEN                     = "General/IgnoreFullScreen";
const bool      OPTION_IGNORE_FULL_SCREEN_DEFAULT             = true;

// Update
const char*     OPTION_UPDATE_CHECK_ON_STARTUP                 = "Update/checkOnStartup";
const bool      OPTION_UPDATE_CHECK_ON_STARTUP_DEFAULT         = true;

const char*     OPTION_UPDATE_CHECK_ON_STARTUP_DELAY           = "Update/checkOnStartupDelay";
const int       OPTION_UPDATE_CHECK_ON_STARTUP_DELAY_DEFAULT   = 60;

const char*     OPTION_UPDATE_CHECK_INTERVAL                   = "Update/checkInterval";
const int       OPTION_UPDATE_CHECK_INTERVAL_DEFAULT           = 24;

const char*     OPTION_UPDATE_LAST_CHECK                       = "Update/lastCheck";
const char*     OPTION_UPDATE_LAST_CHECK_DEFAULT               = "1970-01-01 00:00:00";

// Proxy
const char*     OPTION_PROXY_TYPE                              = "Proxy/proxyType";
const int       OPTION_PROXY_TYPE_DEFAULT                      = QNetworkProxy::NoProxy;

const char*     OPTION_PROXY_SERVER_NAME                       = "Proxy/serverName";
const char*     OPTION_PROXY_SERVER_NAME_DEFAULT               = "";

const char*     OPTION_PROXY_SERVER_PORT                       = "Proxy/serverPort";
const char*     OPTION_PROXY_SERVER_PORT_DEFAULT               = "";

const char*     OPTION_PROXY_REQUIRE_PASSWORD                  = "Proxy/requirePassword";
const bool      OPTION_PROXY_REQUIRE_PASSWORD_DEFAULT          = false;

const char*     OPTION_PROXY_USERNAME                          = "Proxy/username";
const char*     OPTION_PROXY_USERNAME_DEFAULT                  = "";

const char*     OPTION_PROXY_PASSWORD                          = "Proxy/password";
const char*     OPTION_PROXY_PASSWORD_DEFAULT                  = "";

// System
const char*     OPTION_LANGUAGE                                = "System/Language";
const char*     OPTION_LANGUAGE_DEFAULT                        = "";

} // namespace launchy
