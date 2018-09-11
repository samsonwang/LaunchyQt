#include "platform_unix_util.h"


#include <QPixmap>
#include <QIcon>
#include <QDebug>
#include <QPainter>
#include <QProcess>

UnixIconProvider::UnixIconProvider() {
    foreach(QString line, QProcess::systemEnvironment()) {
	if (!line.startsWith("XDG_DATA_DIRS", Qt::CaseInsensitive))
	    continue;
	QStringList spl = line.split("=");
	xdgDataDirs = spl[1].split(":");	
    }
    xdgDataDirs += "/usr/share/icons/";
}


QIcon UnixIconProvider::icon(const QFileInfo& info)
{
    QString name = info.fileName();

    if (name.endsWith(".png", Qt::CaseInsensitive))
	return QIcon(info.absoluteFilePath());    
    if (name.endsWith(".ico", Qt::CaseInsensitive))
	return QIcon(info.absoluteFilePath());
    if (!name.contains("."))
        return QFileIconProvider::icon(QFileIconProvider::File);


    QString end = name.mid(name.lastIndexOf(".")+1);
    if (!file2mime.contains(end.toLower())) {
	
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
	file2mime.insert(end.toLower(), mimeType);
    }

    QString mimeType = file2mime[end.toLower()];

    if (!mime2desktop.contains(mimeType)) {
	QProcess proc2;
	QStringList args;
	args += "query";
	args += "default";
	args += mimeType;
	proc2.start(QString("xdg-mime"),args);
	proc2.waitForFinished(10000);
	QString desk = proc2.readAll().trimmed();
	proc2.close();
	
	mime2desktop[mimeType] = desk;
    }

    
    QString desktop = mime2desktop[mimeType];

    if (desktop == "")
        return QFileIconProvider::icon(QFileIconProvider::File);

    return QIcon(getDesktopIcon(desktop));
}

QString UnixIconProvider::getDesktopIcon(QString desktopFile, QString IconName) {    
    if (QFile::exists(desktopFile)) 
	desktopFile = desktopFile.mid(desktopFile.lastIndexOf("/")+1);	
    
    if (desktopFile.contains("dolphin")) {
            int x = 1;
            x += 1;
    }

    if (desktop2icon.contains(desktopFile) && IconName == "")
	IconName = desktop2icon[desktopFile];    
    if (IconName == "") {
	const char *dirs[] = { "/usr/share/applications/",
			       "/usr/local/share/applications/",
			       "/usr/share/gdm/applications/",
			       "/usr/share/applications/kde/",
			       "~/.local/share/applications/"};
	for(int i = 0; i < 5; i++) {
	    QString dir = dirs[i];
	    QString path = dir + "/" + desktopFile;

	    if (QFile::exists(path)) {
		QFile file(path);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		    return "";
		while(!file.atEnd()) {
		    QString line = file.readLine();
		    if (line.startsWith("Icon", Qt::CaseInsensitive)) {
			desktop2icon[desktopFile] = line.split("=")[1].trimmed();
		    }
		}
		break;
	    }
	}
    }

    if (IconName == "")
	IconName = desktop2icon[desktopFile];

    if (IconName == "")
	return "";
    
    // Find the icon path
    QString iconPath;
    if (icon2path.contains(IconName)) {
	iconPath = icon2path[IconName];
    }
    else if (QFile::exists(IconName)) {
	iconPath = IconName;
    }
    else {
	QStringList inames;
	if (IconName.endsWith(".png") || IconName.endsWith(".xpm") || IconName.endsWith(".svg"))
	    inames += IconName;
	else {
	    inames += IconName + ".png";
	    inames += IconName + ".xpm";
	    inames += IconName + ".svg";
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

	foreach(QString dir, xdgDataDirs) {
	    foreach(QString thm, themes) {
		dirs += dir + "/icons" + thm;
	    }
	}

	dirs += "/usr/share/pixmaps";

	
	foreach(QString dir, dirs) {
	    QDir d(dir);
	    QStringList sdirs;
	    if (!dir.endsWith("pixmaps"))
		sdirs = d.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
	    sdirs += "."; 
	    
	    foreach (QString subdir, sdirs) {
		foreach(QString iname, inames) {
		    if (QFile::exists(dir + "/" + subdir + "/" +  iname)) {
			iconPath = dir + "/" + subdir + "/" + iname;
			icon2path[IconName] = iconPath;
			ifound = true;
			break;
		    }
		}
	    }
	    if (ifound)
		break;
	}
    }	


    return iconPath;
}
