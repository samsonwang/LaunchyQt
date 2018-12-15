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

#include "GlobalVar.h"
#include "AppBase.h"
#include "LaunchyWidget.h"
#include "SettingsManager.h"
#include "Logger.h"

int main(int argc, char* argv[]) {

    launchy::createApplication(argc, argv);

    // Load settings
    launchy::SettingsManager::instance().load();

    QTranslator translator;
    if (translator.load(QLocale(),
                        QString("launchy"),
                        QString("_"),
                        QString("translation"))) {
        qApp->installTranslator(&translator);
    }

    // improve code below with QCommandlinePareser
    QStringList args = qApp->arguments();
    launchy::CommandFlags command = launchy::Default;
    bool allowMultipleInstances = false;
    for (int i = 1; i < args.size(); ++i) {
        QString arg = args[i];
        if (arg.startsWith("-") || arg.startsWith("/")) {
            arg = arg.mid(1);
            if (arg.compare("rescue", Qt::CaseInsensitive) == 0) {
                command = launchy::ResetSkin | launchy::ResetPosition | launchy::ShowLaunchy;
            }
            else if (arg.compare("show", Qt::CaseInsensitive) == 0) {
                command |= launchy::ShowLaunchy;
            }
            else if (arg.compare("options", Qt::CaseInsensitive) == 0) {
                command |= launchy::ShowOptions;
            }
            else if (arg.compare("multiple", Qt::CaseInsensitive) == 0) {
                allowMultipleInstances = true;
            }
            else if (arg.compare("rescan", Qt::CaseInsensitive) == 0) {
                command |= launchy::Rescan;
            }
            else if (arg.compare("exit", Qt::CaseInsensitive) == 0) {
                command |= launchy::Exit;
            }
            else if (arg.compare("log", Qt::CaseInsensitive) == 0) {
                launchy::Logger::setLogLevel(QtDebugMsg);
            }
            else if (arg.compare("profile", Qt::CaseInsensitive) == 0) {
                if (++i < args.length()) {
                    launchy::SettingsManager::instance().setProfileName(args[i]);
                }
            }
        }
    }

    if (!allowMultipleInstances && launchy::g_app->isAlreadyRunning()) {
        launchy::g_app->sendInstanceCommand(command);
        exit(1);
    }

    launchy::createLaunchyWidget(command);

    qApp->exec();
}
