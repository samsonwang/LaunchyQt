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

#include "Core/StartupOption.h"
#include "Core/SettingsManager.h"

#include "Platform/Platform.h"

#include "PluginInterface/Runtime.h"

#include "Logger.h"
#include "MemProfiler.h"
#include "LaunchyApp.h"
#include "LaunchyWidget.h"

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

    // The command line is read first, while nothing that could depend on it
    // exists yet: no QApplication, no settings, no platform. Two of its answers
    // have to be known this early -- -multiple decides how the application
    // object is built (one instance or several allowed), -profile decides which
    // ini file the settings are read from.
    //
    // The arguments are taken as ANSI (local 8-bit) on purpose: argv is all a
    // program has before Qt builds an application object, and the only
    // free-form value -- the profile name -- is a directory name, which the
    // system code page covers.
    QStringList arguments;
    arguments.reserve(argc);
    for (int i = 0; i < argc; ++i) {
        arguments << QString::fromLocal8Bit(argv[i]);
    }
    const launchy::StartupOption option = launchy::parseStartupOption(arguments);

    // -debug comes before everything else: it opens the log file, and every step
    // below wants to be able to explain a failure there.
    if (option.debug) {
        launchy::log::setDebugLogEnabled(true);
        // [MEMPROF] output goes through qInfo(), which only lands in
        // launchy.log under -debug, so turn on sampling at the same time.
        launchy::memprof::setEnabled(true);
    }

    launchy::memprof::record("main:after-parse-args");

    // After -debug on purpose: the warning below would be lost if it ran any
    // earlier. Still long before the first program can be launched.
    dropHostEnvironment();

    // Where the settings live is part of the settings, so the profile has to be
    // known before they are loaded -- with -profile the ini file read below is
    // the one in profiles/<name>/.
    if (!option.profile.isEmpty()) {
        launchy::SettingsManager::instance().setProfileName(option.profile);
    }

    // The application object (QApplication) and the platform are separate now:
    // the application lives in the executable, the platform answers the
    // platform questions and lives in the platform library. Both are installed
    // before anything reads settings or builds a catalog: the settings are
    // where the application's directories come from, so they have to be asked
    // of a real platform, not of the inert fallback core otherwise answers with.
    launchy::initApplication(argc, argv, option.multiple);
    launchy::initPlatform();

    launchy::memprof::record("main:after-initApplication");

    // A plugin cannot run a program itself -- the platform is what does, and it
    // lives in this executable. Hand the plugin-facing request straight to it:
    // no queue, no timer, no UI object in the middle. Wired here, next to the
    // platform it points at, so the two cannot get out of step.
    launchy::setRunProgramHandler([](const QString& file, const QString& args) {
        launchy::platform().runProgram(file, args);
    });

    // Load settings into the core.
    launchy::SettingsManager::instance().load(launchy::platform().directories(),
                                              launchy::platform().defaultCatalogDirectories());
    launchy::memprof::record("main:after-settings-load");

    launchy::initLaunchyWidget(option);

    int exitCode = qApp->exec();
    launchy::memprof::record("main:after-exec");

    qDebug() << "app exec exit code:" << exitCode;

    // Tear the process-wide objects down in dependency order, now that the event
    // loop is gone. The five calls below are ordered like this on purpose, each
    // one removes something the calls after it still read; the order is
    // widget -> plugin seam -> platform -> settings -> application.
    //
    // The widget goes first: deleting a QWidget needs a live QApplication, and
    // its destructor is what stops the catalog builder thread (it waits for a
    // running scan). That thread calls platform() while it scans, so it has to
    // be joined before the platform can be dropped.
    launchy::cleanupLaunchyWidget();

    // Everything that can still reach the platform is gone by now except the
    // plugin seam, so unhook that first -- a request arriving from a straggling
    // worker would otherwise call into a platform that is on its way out. Only
    // then take the platform itself down.
    launchy::setRunProgramHandler(nullptr);
    launchy::cleanupPlatform();

    // The settings go only now: the teardown above dereferences g_settings
    // without a null check (the option dialog, savePosition). A cleared
    // g_settings is a null pointer, so anything still reading it would crash.
    launchy::g_settings.clear();

    // Last, because it invalidates qApp: no widget may be deleted and no code
    // connected to the application object (PluginPy returns the GIL from
    // aboutToQuit) may run after this returns.
    launchy::cleanupApplication();

    qInfo("main: cleanup finished");
    launchy::memprof::record("main:after-cleanup");

    if (exitCode == launchy::EXIT_CODE_RESTART) {
        // The arguments this process was started with, still in the list they
        // were parsed from -- no second read of the command line, and none from
        // an application object that has already been deleted above.
        qInfo() << "app restarted" << arguments;
        QString program = arguments.front();
        arguments.pop_front();
        QProcess::startDetached(program, arguments);
    }

    launchy::log::stopLogging();

    return exitCode;
}
