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

#include <QProcess>
#include <QObject>

#include "AppBase.h"
#include "AppSettings.h"
#include "SettingsManager.h"
#include "LaunchyWidget.h"
#include "Logger.h"
#include "MemProfiler.h"
#include "GlobalVar.h"
#include "Platform/Platform.h"
#include "PluginInterface/Runtime.h"

namespace {

// Launchy is a launcher: the environment it was started in must not leak into
// the programs it launches, because some of those variables decide how a child
// starts. ELECTRON_RUN_AS_NODE is the worst of them - it turns every Electron
// application (VS Code, Cursor, Slack, ...) into a windowless node process that
// exits in about half a second, so the launch looks like it silently did
// nothing. A launcher started from some other application's shell inherits it
// without anyone asking, hence the drop. Runs after logging is up (so the reason
// is recorded in launchy.log) and before any program can be launched.
void dropHostEnvironment() {
    static const char* const vars[] = { "ELECTRON_RUN_AS_NODE" };

    for (const char* name : vars) {
        if (qEnvironmentVariableIsSet(name)) {
            qWarning() << "main: dropping inherited" << name
                       << "from the child environment, value:" << qEnvironmentVariable(name);
            qunsetenv(name);
        }
    }
}

} // namespace

int main(int argc, char* argv[]) {

    // The application object (QApplication) and the platform are separate now:
    // the application lives in the executable, the platform answers the
    // platform questions and lives in the platform library. Installed before
    // anything reads settings or builds a catalog, taken down at the end of
    // main() -- after everything that could still ask it a question is gone.
    launchy::AppBase* app = launchy::createApplication(argc, argv);
    launchy::initPlatform();

    // A plugin cannot run a program itself -- the platform is what does, and it
    // lives in this executable. Hand the plugin-facing request straight to it:
    // no queue, no timer, no UI object in the middle. Wired here, next to the
    // platform it points at, so the two cannot get out of step.
    launchy::setRunProgramHandler([](const QString& file, const QString& args) {
        launchy::platform().runProgram(file, args);
    });

    launchy::memprof::record("main:after-createApplication");

    // Load settings (core reads them), then apply the effects the core cannot
    // (style, proxy, language, logging).
    launchy::SettingsManager::instance().load(launchy::platform().directories(),
                                              launchy::platform().defaultCatalogDirectories());
    launchy::applyRuntimeSettings();
    launchy::memprof::record("main:after-settings-load");

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
            else if (arg.compare("debug", Qt::CaseInsensitive) == 0) {
                launchy::log::setDebugLogEnabled(true);
                // [MEMPROF] output goes through qInfo(), which only lands in
                // launchy.log under -debug, so turn on sampling at the same time.
                launchy::memprof::setEnabled(true);
            }
            else if (arg.compare("profile", Qt::CaseInsensitive) == 0) {
                if (++i < args.length()) {
                    launchy::SettingsManager::instance().setProfileName(args[i]);
                }
            }
        }
    }

    launchy::memprof::record("main:after-parse-args");

    // After the argument loop on purpose: -debug is what opens the log file, so
    // this warning would be lost if it ran any earlier. Still long before the
    // first program can be launched.
    dropHostEnvironment();

    launchy::createLaunchyWidget(command);

    int exitCode = qApp->exec();
    launchy::memprof::record("main:after-exec");

    qDebug() << "app exec exit code:" << exitCode;

    launchy::cleanupGlobalVar();
    launchy::memprof::record("main:after-cleanup");

    // Everything that can still reach the platform is gone by now except the
    // plugin seam, so unhook that first -- a request arriving from a straggling
    // worker would otherwise call into a platform that is on its way out. Only
    // then take the platform itself down.
    launchy::setRunProgramHandler(nullptr);
    launchy::cleanupPlatform();

    if (exitCode == launchy::Restart) {
        qInfo() << "app restarted" << args;
        QString program = args[0];
        args.pop_front();
        QProcess::startDetached(program, args);
    }

    launchy::log::stopLogging();

    return exitCode;
}
