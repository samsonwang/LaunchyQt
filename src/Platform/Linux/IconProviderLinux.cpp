
#include "IconProviderLinux.h"
#include <QDir>
#include <QPixmap>
#include <QIcon>
#include <QDebug>
#include <QPainter>
#include <QProcess>

namespace launchy {

IconProviderLinux::IconProviderLinux() {
    foreach(QString line, QProcess::systemEnvironment()) {
        if (!line.startsWith("XDG_DATA_DIRS", Qt::CaseInsensitive)) {
            continue;
        }

        QStringList spl = line.split("=");
        m_xdgDataDirs = spl[1].split(":");
    }
    m_xdgDataDirs += "/usr/share/icons/";
}

IconProviderLinux::~IconProviderLinux() {

}

QIcon IconProviderLinux::icon(const QFileInfo& info) {
    QString name = info.fileName();

    if (name.endsWith(".png", Qt::CaseInsensitive)) {
        return QIcon(info.absoluteFilePath());
    }

    if (name.endsWith(".ico", Qt::CaseInsensitive)) {
        return QIcon(info.absoluteFilePath());
    }

    if (!name.contains(".")) {
        return QFileIconProvider::icon(QFileIconProvider::File);
    }

    QString end = name.mid(name.lastIndexOf(".")+1);
    if (!m_file2mime.contains(end.toLower())) {
        QProcess proc;
        QStringList args;
        args += "query";
        args += "filetype";
        args += info.absoluteFilePath();
        proc.setReadChannel(QProcess::StandardOutput);
        proc.start(QString("xdg-mime"), args);
        proc.waitForFinished(10000);
        QString mimeType = proc.readAll().trimmed();
        proc.close();
        m_file2mime.insert(end.toLower(), mimeType);
    }

    QString mimeType = m_file2mime[end.toLower()];

    if (!m_mime2desktop.contains(mimeType)) {
        QProcess proc2;
        QStringList args;
        args += "query";
        args += "default";
        args += mimeType;
        proc2.start(QString("xdg-mime"),args);
        proc2.waitForFinished(10000);
        QString desk = proc2.readAll().trimmed();
        proc2.close();

        m_mime2desktop[mimeType] = desk;
    }


    QString desktop = m_mime2desktop[mimeType];

    if (desktop.isEmpty()) {
        return QFileIconProvider::icon(QFileIconProvider::File);
    }

    return QIcon(getDesktopIcon(desktop));
}

QString IconProviderLinux::getDesktopIcon(QString desktopFile, QString iconName) {
    if (QFile::exists(desktopFile)) {
        desktopFile = desktopFile.mid(desktopFile.lastIndexOf("/")+1);
    }

    /*
      if (desktopFile.contains("dolphin")) {
      int x = 1;
      x += 1;
      }
    */

    if (m_desktop2icon.contains(desktopFile) && iconName.isEmpty()) {
        iconName = m_desktop2icon[desktopFile];
    }

    if (iconName.isEmpty()) {
        const char *dirs[] = { "/usr/share/applications/",
                               "/usr/local/share/applications/",
                               "/usr/share/gdm/applications/",
                               "/usr/share/applications/kde/",
                               "~/.local/share/applications/" };
        for(int i = 0; i < 5; i++) {
            QString dir = dirs[i];
            QString path = dir + desktopFile;

            if (QFile::exists(path)) {
                QFile file(path);
                if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    return "";
                }

                while (!file.atEnd()) {
                    QString line = file.readLine();
                    if (line.startsWith("Icon", Qt::CaseInsensitive)) {
                        m_desktop2icon[desktopFile] = line.split("=")[1].trimmed();
                    }
                }
                break;
            }
        }
    }

    if (iconName.isEmpty()) {
        iconName = m_desktop2icon[desktopFile];
    }

    if (iconName.isEmpty()) {
        return "";
    }

    // Find the icon path
    QString iconPath;
    if (m_icon2path.contains(iconName)) {
        iconPath = m_icon2path[iconName];
    }
    else if (QFile::exists(iconName)) {
        iconPath = iconName;
    }
    else {
        QStringList inames;
        if (iconName.endsWith(".png") || iconName.endsWith(".xpm") || iconName.endsWith(".svg")) {
            inames += iconName;
        }
        else {
            inames += iconName + ".png";
            inames += iconName + ".xpm";
            inames += iconName + ".svg";
        }

        bool ifound = false;
        QStringList themes;
        themes += "/hicolor/32x32";
        themes += "/hicolor/48x48";
        themes += "/hicolor/64x64";
        themes += "/oxygen/32x32";
        themes += "/gnome/32x32";

        QStringList dirs;
        dirs += QDir::homePath() + "/.icons" + themes[0];
        dirs += "/usr/share/pixmaps";
        foreach (QString dir, m_xdgDataDirs) {
            foreach (QString thm, themes) {
                dirs += dir + "/icons" + thm;
            }
        }

        foreach(QString dir, dirs) {
            QDir d(dir);
            QStringList sdirs;
            if (!dir.endsWith("pixmaps")) {
                sdirs = d.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
            }
            sdirs += ".";

            foreach (QString subdir, sdirs) {
                foreach (QString iname, inames) {
                    if (QFile::exists(dir + "/" + subdir + "/" +  iname)) {
                        iconPath = dir + "/" + subdir + "/" + iname;
                        m_icon2path[iconName] = iconPath;
                        ifound = true;
                        break;
                    }
                }
            }

            if (ifound) {
                break;
            }
        }
    }

    return iconPath;
}

}
