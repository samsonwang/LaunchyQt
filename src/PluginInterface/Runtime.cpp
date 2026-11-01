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

#include "Runtime.h"

#include <QDebug>
#include <QDir>
#include <QMutex>
#include <QMutexLocker>

namespace launchy {

// Process-wide runtime state shared across the application, the core library
// and every plugin. g_settings is the one QSettings instance (exported through
// the DLL so every module sees the same object), g_needRebuildCatalog is a
// lock-free flag a plugin sets when it wants the catalog re-indexed.
QSharedPointer<QSettings> g_settings;
QAtomicInt g_needRebuildCatalog;

QString getAppTempPath(bool toNative) {
    QString path = QDir::tempPath() + QString("/Launchy");

    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    if (toNative) {
        path = QDir::toNativeSeparators(path);
    }

    return path;
}

namespace {

// The application's launch path, reached through a function pointer because this
// library is the shared bottom layer: it cannot link the platform library (that
// one links it), and neither can a plugin. main() sets this once, before any
// plugin exists, and a request only reads it.
//
// A mutex instead of a bare pointer: the write happens on the GUI thread while a
// plugin thread may already be reading. It only ever covers the copy of the
// pointer -- the launch itself runs unlocked.
QMutex g_handlerMutex;
RunProgramHandler g_handler = nullptr;

} // namespace

void setRunProgramHandler(RunProgramHandler handler) {
    QMutexLocker locker(&g_handlerMutex);
    g_handler = handler;
}

void runProgram(const QString& file, const QString& args) {
    RunProgramHandler handler = nullptr;
    {
        QMutexLocker locker(&g_handlerMutex);
        handler = g_handler;
    }

    if (!handler) {
        // Nothing is hooked up: a test run, or a process that forgot to wire the
        // seam. The request cannot be honoured, so say so instead of returning
        // as if it had been.
        qWarning() << "launchy::runProgram, no handler installed, dropping:"
                   << file << args;
        return;
    }

    // Straight through to the platform; there is nothing in between to
    // queue, schedule or drain.
    handler(file, args);
}

} // namespace launchy
