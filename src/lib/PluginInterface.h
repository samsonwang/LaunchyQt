/*
Launchy: Application Launcher
Copyright (C) 2007-2009  Josh Karlin

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

#pragma once

#include <QScopedPointer>
#include <QSettings>
#include <QString>
#include "LaunchyLib.h"

namespace launchy {
/**
    \brief This is the class that a plugin must implement.

    The PluginInterface is the interface between Launchy and your plugin.
    Launchy communicates with your plugin through the msg function.  To store
    any settings in Launchy's permanent configuration file, please
    refer to the settings member variable.
*/
class LAUNCHY_EXPORT PluginInterface {
public:
    PluginInterface();
    virtual ~PluginInterface();


    /**
        \brief This is how messages are passed to the plugin

        Here you handle the subset of messages that your plugin requires.  At minimum, your plugin must
            handle MSG_GET_ID and MSG_GET_NAME.  For unhandled messages, return 0.  For handled messages, return 1.
        \param msgId The type of the message, such as show dialog, initialize, or get results
        \param wParam If this message type requires any parameters, this will be the first and casted to void*
        \param lParam If this message requires two parameters, this is the second, casted to void*
        \return 1 means the message was handled, 0 means it was ignored

        \verbatim
        int WebyPlugin::msg(int msgId, void* wParam, void* lParam)
        {
            bool handled = false;
            switch (msgId)
            {
                case MSG_INIT:
                    init();
                    handled = true;
                    break;
                case MSG_GET_LABELS:
                    getLabels((QList<InputData>*) wParam);
                    handled = true;
                    break;
                case MSG_GET_ID:
                    getID((uint*) wParam);
                    handled = true;
                    break;
                case MSG_GET_NAME:
                    getName((QString*) wParam);
                    handled = true;
                    break;
                case MSG_GET_RESULTS:
                    getResults((QList<InputData>*) wParam, (QList<CatItem>*) lParam);
                    handled = true;
                    break;
                case MSG_GET_CATALOG:
                    getCatalog((QList<CatItem>*) wParam);
                    handled = true;
                    break;
                case MSG_LAUNCH_ITEM:
                    launchItem((QList<InputData>*) wParam, (CatItem*) lParam);
                    handled = true;
                    break;
                case MSG_HAS_DIALOG:
                    handled = true;
                    break;
                case MSG_DO_DIALOG:
                    doDialog((QWidget*) wParam, (QWidget**) lParam);
                    break;
                case MSG_END_DIALOG:
                    endDialog((bool) wParam);
                    break;

                default:
                    break;
            }

            return handled;
        }
        \endverbatim

    */
    virtual int msg(int msgId, void* wParam = NULL, void* lParam = NULL) = 0;


    /** The setting manager in which you store your persistent data.
    More data can be found in QT's documentation for QSettings

    Following is an example of using the settings variable:

    \verbatim
    QSettings* set = *settings;

    // Write a value to the config file
    set->setValue("weby/version", 1.0);

    // Read a value from the config value
    double value = set->value("weby/version", 0.0);
    \endverbatim
    */
//    QPointer<QSettings>* settings;
};


/*
    \brief A convienience run function

     This function will run the program along with arguments and is platform independent.
    \param file The location of the file to run
    \param args The arguments to the command
    \param translateSeparators Whether to translate / into native separators
*/
LAUNCHY_EXPORT void runProgram(const QString& file, const QString& args, bool translateSeparators = true);
LAUNCHY_EXPORT int getDesktop();
}

#define PLUGININTERFACE_IID "com.launchyqt.PluginInterface/1.0"
Q_DECLARE_INTERFACE(launchy::PluginInterface, PLUGININTERFACE_IID)
