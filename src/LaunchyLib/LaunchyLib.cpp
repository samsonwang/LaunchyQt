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

#include "LaunchyLib.h"

#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QLocale>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>

#include "PluginMsg.h"

namespace launchy {
Q_DECL_EXPORT QSharedPointer<QSettings> g_settings;
Q_DECL_EXPORT QAtomicInt g_needRebuildCatalog;
} // namespace launchy

#if defined(Q_OS_WIN)

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif // VC_EXTRALEAN

#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <shellapi.h>

namespace launchy {
// This is also defined in WinIconProvider, remove from both locations if 64 bit build is produced
static QString aliasTo64(QString path) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString pf32 = env.value("PROGRAMFILES");
    QString pf64 = env.value("PROGRAMW6432");

    // On 64 bit windows, 64 bit shortcuts don't resolve correctly from 32 bit executables, fix it here
    QFileInfo pInfo(path);

    if (env.contains("PROGRAMW6432") && pInfo.isSymLink() && pf32 != pf64) {
        if (QDir::toNativeSeparators(pInfo.symLinkTarget()).contains(pf32)) {
            QString path64 = QDir::toNativeSeparators(pInfo.symLinkTarget());
            path64.replace(pf32, pf64);
            if (QFileInfo(path64).exists()) {
                path = path64;
            }
        }
        else if (pInfo.symLinkTarget().contains("system32")) {
            QString path32 = QDir::toNativeSeparators(pInfo.symLinkTarget());
            if (!QFileInfo(path32).exists()) {
                path = path32.replace("system32", "sysnative");
            }
        }
    }
    return path;
}

int getDesktop() {
    return DESKTOP_WINDOWS;
}

static void runProgram(const QString& path, const QString& args) {

    SHELLEXECUTEINFO ShExecInfo;
    bool elevated = (GetKeyState(VK_SHIFT) & 0x80000000) != 0 && (GetKeyState(VK_CONTROL) & 0x80000000) != 0;

    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = SEE_MASK_FLAG_NO_UI;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = elevated ? L"runas" : NULL;
    ShExecInfo.lpFile = (LPCTSTR)path.utf16();
    if (!args.isEmpty()) {
        ShExecInfo.lpParameters = (LPCTSTR)args.utf16();
    }
    else {
        ShExecInfo.lpParameters = NULL;
    }
    QDir dir(path);
    QFileInfo info(path);
    if (!info.isDir() && info.isFile())
        dir.cdUp();
    QString filePath = QDir::toNativeSeparators(dir.absolutePath());
    ShExecInfo.lpDirectory = (LPCTSTR)filePath.utf16();
    ShExecInfo.nShow = SW_NORMAL;
    ShExecInfo.hInstApp = NULL;

    ShellExecuteEx(&ShExecInfo);
}

void runProgram(const QString& path, const QString& args, bool translateSeparators) {
    // This 64 bit aliasing needs to be gotten rid of if we have a 64 bit build
    QString path64 = aliasTo64(path);

    SHELLEXECUTEINFO ShExecInfo;
    bool elevated = (GetKeyState(VK_SHIFT) & 0x80000000) != 0 && (GetKeyState(VK_CONTROL) & 0x80000000) != 0;

    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = SEE_MASK_FLAG_NO_UI;
    ShExecInfo.fMask = NULL;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = elevated ? L"runas" : NULL;
    QString filePath = translateSeparators ? QDir::toNativeSeparators(path64) : path64;
    ShExecInfo.lpFile = (LPCTSTR)filePath.utf16();

    if (!args.isEmpty()) {
        ShExecInfo.lpParameters = (LPCTSTR)args.utf16();
    }
    else {
        ShExecInfo.lpParameters = NULL;
    }

    QDir dir(path64);
    QFileInfo info(path64);
    if (!info.isDir() && info.isFile())
        dir.cdUp();
    QString directory = QDir::toNativeSeparators(dir.absolutePath());
    ShExecInfo.lpDirectory = (LPCTSTR)directory.utf16();
    ShExecInfo.nShow = SW_NORMAL;
    ShExecInfo.hInstApp = NULL;

    ShellExecuteEx(&ShExecInfo);
}

} // namespace launchy

#elif defined(Q_OS_MAC)

namespace launchy {

int getDesktop() {
    return DESKTOP_MAC;
}

void runProgram(QString path, QString args, bool translateSeparators) {
    translateSeparators = translateSeparators; // kill warning
    QString cmd;
    cmd = "open \"" + QDir::toNativeSeparators(path) + "\" --args " + args.trimmed();
    QProcess::startDetached(cmd.trimmed());

    return;
}

} // namespace launchy

#elif defined(Q_OS_LINUX)

namespace launchy {

int getDesktop() {
    QStringList list = QProcess::systemEnvironment();
    foreach(QString s, list) {
        if (s.startsWith("GNOME_DESKTOP_SESSION"))
            return DESKTOP_GNOME;
        else if (s.startsWith("KDE_FULL_SESSION"))
            return DESKTOP_KDE;
    }
    return -1;
}

void runProgram(const QString& file, const QString& args, bool translateSeparators) {
    Q_UNUSED(translateSeparators)
        QString path = file;
    QString arg = args;
    QString fullname = path.split(" ")[0];
    QFileInfo info(fullname);

    /* I would argue that launchy does not need to fully
    support the desktop entry specification yet/ever.
    NOTE: %c, %k, and %i are handled during loading */
    if (path.contains("%")) {
        path.replace("%U", arg);
        path.replace("%u", arg);
        path.replace("%F", arg);
        path.replace("%f", arg);
        /* remove specifiers either not yet supported or depricated */
        path.remove(QRegularExpression("%."));
        arg = "";
    }

    QString cmd;

    if (!info.isExecutable() || info.isDir()) {
        /* if more then one file is passed, then xdg-open will fail.. */
        cmd = "xdg-open \"" + path.trimmed() + "\"";
    }
    else if (getDesktop() == DESKTOP_KDE) {
        /* special case for KDE since some apps start behind other windows */
        cmd = "kstart --activate " + path.trimmed() + " " + args.trimmed();
    }
    else /* gnome, xfce, etc */ {
        path.replace("\"", "\\\"");
        arg.replace("\"", "\\\"");
        cmd = "sh -c \"" + path.trimmed() + " " + arg.trimmed() + "\"";
    }

    QProcess::startDetached(cmd);

    return;
}

} // namespace launchy

#endif
