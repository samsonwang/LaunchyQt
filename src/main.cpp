/*
Launchy: Application Launcher
Copyright (C) 2007-2009  Josh Karlin, Simon Capewell
Copyright (C) 2018  Samson Wang

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

#include "AppBase.h"
#include "LaunchyWidget.h"
#include "SettingsManager.h"
#include "QLogger.h"

int main(int argc, char* argv[]) {

    createApplication(argc, argv);
    
    // Load settings
    SettingsManager::instance().load();

    QTranslator translator;
    if (translator.load(QLocale(),
                        QString("launchy"),
                        QString("_"),
                        QString("translation"))) {
        qApp->installTranslator(&translator);
    }

    // improve code below with QCommandlinePareser
    QStringList args = qApp->arguments();
    CommandFlags command = Default;
    bool allowMultipleInstances = false;
    for (int i = 1; i < args.size(); ++i) {
        QString arg = args[i];
        if (arg.startsWith("-") || arg.startsWith("/")) {
            arg = arg.mid(1);
            if (arg.compare("rescue", Qt::CaseInsensitive) == 0) {
                command = ResetSkin | ResetPosition | ShowLaunchy;
            }
            else if (arg.compare("show", Qt::CaseInsensitive) == 0) {
                command |= ShowLaunchy;
            }
            else if (arg.compare("options", Qt::CaseInsensitive) == 0) {
                command |= ShowOptions;
            }
            else if (arg.compare("multiple", Qt::CaseInsensitive) == 0) {
                allowMultipleInstances = true;
            }
            else if (arg.compare("rescan", Qt::CaseInsensitive) == 0) {
                command |= Rescan;
            }
            else if (arg.compare("exit", Qt::CaseInsensitive) == 0) {
                command |= Exit;
            }
            else if (arg.compare("log", Qt::CaseInsensitive) == 0) {
                QLogger::setLogLevel(QtDebugMsg);
            }
            else if (arg.compare("profile", Qt::CaseInsensitive) == 0) {
                if (++i < args.length()) {
                    SettingsManager::instance().setProfileName(args[i]);
                }
            }
        }
    }

    if (!allowMultipleInstances && g_app->isAlreadyRunning()) {
        g_app->sendInstanceCommand(command);
        exit(1);
    }

    createLaunchyWidget(command);

    qApp->exec();
}
