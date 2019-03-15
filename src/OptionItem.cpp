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

namespace launchy {

const char*     OPSTION_ALWAYSSHOW                             = "GenOps/alwaysshow";
const bool      OPSTION_ALWAYSSHOW_DEFAULT                     = false;

const char*     OPSTION_ALWAYSTOP                              = "GenOps/alwaystop";
const bool      OPSTION_ALWAYSTOP_DEFAULT                      = false;

const char*     OPTION_LOGLEVEL                               = "GenOps/logLevel";
const int       OPTION_LOGLEVEL_DEFAULT                       = 2;

const char*     OPSTION_DECORATETEXT                           = "GenOps/decoratetext";
const bool      OPSTION_DECORATETEXT_DEFAULT                   = false;

const char*     OPSTION_HIDEIFLOSTFOCUS                        = "GenOps/hideiflostfocus";
const bool      OPSTION_HIDEIFLOSTFOCUS_DEFAULT                = true;

const char*     OPSTION_ALWAYSCENTER                           = "GenOps/alwayscenter";
const int       OPSTION_ALWAYSCENTER_DEFAULT                   = 3;

const char*     OPSTION_DRAGMODE                               = "GenOps/dragmode";
const bool      OPSTION_DRAGMODE_DEFAULT                       = false;

const char*     OPTION_APPSTYLE                                = "GenOps/appStyle";
const char*     OPTION_APPSTYLE_DEFAULT                        = "fusion";

const char*     OPTION_SHOWHIDDENFILES                        = "GenOps/showHiddenFiles";
const bool      OPTION_SHOWHIDDENFILES_DEFAULT                = false;

const char*     OPTION_SHOWNETWORK                            = "GenOps/showNetwork";
const bool      OPTION_SHOWNETWORK_DEFAULT                    = false;

const char*     OPSTION_AUTOSUGGESTDELAY                       = "GenOps/autoSuggestDelay";
const int       OPSTION_AUTOSUGGESTDELAY_DEFAULT               = 1000;

const char*     OPTION_REBUILDTIMER                           = "GenOps/rebuildTimer";
const int       OPTION_REBUILDTIMER_DEFAULT                   = 30;

const char*     OPSTION_NUMVIEWABLE                            = "GenOps/numviewable";
const int       OPSTION_NUMVIEWABLE_DEFAULT                    = 4;

const char*     OPSTION_NUMRESULT                              = "GenOps/numresults";
const int       OPSTION_NUMRESULT_DEFAULT                      = 10;

const char*     OPSTION_MAXITEMSINHISTORY                      = "GenOps/maxitemsinhistory";
const int       OPSTION_MAXITEMSINHISTORY_DEFAULT              = 20;

const char*     OPSTION_OPAQUENESS                             = "GenOps/opaqueness";
const int       OPSTION_OPAQUENESS_DEFAULT                     = 100;

const char*     OPSTION_FADEIN                                 = "GenOps/fadein";
const int       OPSTION_FADEIN_DEFAULT                         = 10;

const char*     OPSTION_FADEOUT                                = "GenOps/fadeout";
const int       OPSTION_FADEOUT_DEFAULT                        = 20;

const char*     OPSTION_SKIN                                   = "GenOps/skin";
const char*     OPSTION_SKIN_DEFAULT                           = "Default";

const char*     OPSTION_CONDENSEDVIEW                          = "GenOps/condensedView";
const int       OPSTION_CONDENSEDVIEW_DEFAULT                  = 2;

const char*     OPSTION_HOTKEY                                 = "GenOps/hotkey";
const int       OPSTION_HOTKEY_DEFAULT                         = -1;

const char*     OPSTION_HOTKEYMOD                              = "GenOps/hotkeyModifier";
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
const int       OPSTION_HOTKEYMOD_DEFAULT                      = Qt::AltModifier;
#elif defined(Q_OS_MAC)
const int       OPSTION_HOTKEYMOD_DEFAULT                      = Qt::MetaModifier;
#endif

const char*     OPSTION_HOTKEYKEY                              = "GenOps/hotkeyAction";
const int       OPSTION_HOTKEYKEY_DEFAULT                      = Qt::Key_Space;

const char*     OPSTION_VERSION                                = "version";
const int       OPSTION_VERSION_DEFAULT                        = 0;

const char*     OPSTION_POS                                    = "Display/pos";
const QPoint    OPSTION_POS_DEFAULT                            = QPoint(0, 0);

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

}
