/*
  Launchy: Application Launcher
  Copyright (C) 2008  Josh Karlin

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

#include "PlatformLinux.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <QtGui>
#include <QCoreApplication>
#include <QFileInfo>
#include <QRegularExpression>
#include <private/qtx11extras_p.h>

#include "IconProviderLinux.h"
#include "Core/Directory.h"
#include "PluginInterface/CatalogItem.h"

namespace launchy {

PlatformLinux::PlatformLinux()
    : m_iconProvider(new IconProviderLinux()) {
}

PlatformLinux::~PlatformLinux() {
    delete m_iconProvider;
    m_iconProvider = nullptr;
}

// Network neighbourhood browsing has no equivalent here (it was the Windows
// domain enumeration), so the honest answer is "this platform cannot list them".
bool PlatformLinux::getComputers(QStringList& computers) const {
    Q_UNUSED(computers);
    return false;
}

// Nothing queries the session for a "do not disturb" state, so a notification
// is never treated as intrusive.
bool PlatformLinux::allowNotification() const {
    return true;
}

// Idle time is unknown; callers fall back to the behaviour that existed before
// idle detection was introduced.
int PlatformLinux::systemIdleSeconds() const {
    return -1;
}

QList<Directory> PlatformLinux::defaultCatalogDirectories() const {
    QList<Directory> list;
    const char *dirs[] = {"/usr/share/applications/",
                          "/usr/local/share/applications/",
                          "/usr/share/gdm/applications/",
                          "/usr/share/applications/kde/",
                          "~/.local/share/applications/"};
    QStringList l;
    l << "*.desktop";

    for(int i = 0; i < 5; i++) {
        list.append(Directory(dirs[i], l, false, true, 10));
    }

    list.append(Directory("~", QStringList(), true, false, 0));

    return list;
}

QMap<QString, QList<QString>> PlatformLinux::directories() const {
    QMap<QString, QList<QString>> out;
    QDir d;
    d.mkdir(QDir::homePath() + "/.launchy");

    // applicationDirPath() is Qt Core (QCoreApplication), not QApplication.
    const QString appDir = QCoreApplication::applicationDirPath();

    out["skins"] += appDir + "/skins";
    out["skins"] += QDir::homePath() + "/.launchy/skins";
    out["skins"] += "/usr/share/launchy/skins";

    out["plugins"] += appDir + "/plugins";
    out["plugins"] += QDir::homePath() + "/.launchy/plugins";
    out["plugins"] += "/usr/lib/launchy/plugins";

    out["config"] += QDir::homePath() + "/.launchy";
    out["portableConfig"] += appDir;

    if (QFile::exists(out["skins"].last() + "/Default"))
        out["defSkin"] += out["skins"].last() + "/Default";
    else
        out["defSkin"] += out["skins"].first() + "/Default";

    out["platforms"] += appDir;
    out["platforms"] += "/usr/lib/launchy";

    return out;
}

bool PlatformLinux::supportsAlphaBorder() const {
    return QX11Info::isCompositingManagerRunning();
}

void PlatformLinux::alterItem(CatItem* item) const {
    if (!item->fullPath.endsWith(".desktop", Qt::CaseInsensitive)) {
        return;
    }

    QFile file(item->fullPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QString locale = QLocale::system().name();

    QString name;
    QString icon;
    QString exe;
    while(!file.atEnd()) {
        QString line = QString::fromUtf8(file.readLine());

        if (line.startsWith("Name[" + locale, Qt::CaseInsensitive)) {
            name = line.split("=")[1].trimmed();
        }
        else if (line.startsWith("Name=", Qt::CaseInsensitive)) {
            name = line.split("=")[1].trimmed();
        }
        else if (line.startsWith("Icon", Qt::CaseInsensitive)) {
            icon = line.split("=")[1].trimmed();
        }
        else if (line.startsWith("Exec", Qt::CaseInsensitive)) {
            exe = line.split("=")[1].trimmed();
        }
    }

    if (name.size() >= item->shortName.size() - 8) {
        item->shortName = name;
    }

    // Don't index desktop items wthout icons
    if (icon.trimmed().isEmpty()) {
        return;
    }

    /* fill in some specifiers while we have the info */
    exe.replace("%i", "--icon " + icon);
    exe.replace("%c", name);
    exe.replace("%k", item->fullPath);

    QStringList allExe = exe.trimmed().split(" ", Qt::SkipEmptyParts);
    if (allExe.isEmpty() || allExe[0].isEmpty()) {
        return;
    }

    exe = allExe[0];
    allExe.removeFirst();

    /* if an absolute or relative path is supplied we can just skip this
       everything else should be checked to avoid picking up [unwanted]
       stuff from the working directory - if it doesnt exsist, use it anyway */
    if(!exe.contains(QRegularExpression("^.?.?/"))) {
        foreach(QString line, QProcess::systemEnvironment()) {
            if (!line.startsWith("Path", Qt::CaseInsensitive)) {
                continue;
            }

            QStringList spl = line.split("=");
            QStringList spl2 = spl[1].split(":");
            foreach(QString dir, spl2) {
                QString tmp = dir + "/" + exe;
                if (QFile::exists(tmp)) {
                    exe = tmp;
                    break;
                }
            }
            break;
        }
    }

    item->fullPath = exe + " " + allExe.join(" ");

    icon = m_iconProvider->getDesktopIcon(file.fileName(), icon);

    QFileInfo inf(icon);
    if (!inf.exists()) {
        qDebug() << "couldn't find icon for" << icon << item->fullPath;
        return;
    }

    item->iconPath = icon;
}

QString PlatformLinux::expandEnvironmentVars(const QString& txt) const {
    QStringList list = QProcess::systemEnvironment();
    QString in = txt;
    in.replace('~', "$HOME$");
    QString delim("$");
    QString out = "";
    int curPos = in.indexOf(delim, 0);
    if (curPos == -1) return in;

    while(curPos != -1) {
        int nextPos = in.indexOf("$", curPos+1);
        if (nextPos == -1) {
            out += in.mid(curPos+1);
            break;
        }
        QString var = in.mid(curPos+1, nextPos-curPos-1);
        bool found = false;
        foreach(QString s, list) {
            if (s.startsWith(var, Qt::CaseInsensitive)) {
                found = true;
                out += s.mid(var.length()+1);
                break;
            }
        }
        if (!found) {
            out += "$" + var;
        }
        curPos = nextPos;
    }
    return out;
}

namespace {

// Which desktop environment is running. Only the Linux runProgram needs this
// (KDE windows can start behind others, so they need kstart --activate).
enum {
    DESKTOP_WINDOWS = 0,
    DESKTOP_GNOME = 1,
    DESKTOP_KDE = 2,
    DESKTOP_MAC = 3,
};

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

} // namespace

void PlatformLinux::runProgram(const QString& file, const QString& args,
                                    bool translateSeparators) const {
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
