#include "precompiled.h"
#include "platform_mac.h"


PlatformMac::PlatformMac(int& argc, char** argv) :
        PlatformBase(argc,argv)
{

    icons = new QFileIconProvider();
}

PlatformMac::~PlatformMac()
{

}


bool PlatformMac::setHotkey(const QKeySequence & key, QObject* receiver, const char* slot)
{
    GlobalShortcutManager::disconnect(oldKey, receiver, slot);
    GlobalShortcutManager::connect(key, receiver, slot);
    oldKey = key;
    qDebug() << key << GlobalShortcutManager::isConnected(key);
    return GlobalShortcutManager::isConnected(key);
}


void PlatformMac::alterItem(CatItem* item) {
    if (!item->fullPath.endsWith(".app", Qt::CaseInsensitive))
        return;
//    item->shortName.chop(4);
//    item->lowName.chop(4);
}


QHash<QString, QList<QString> > PlatformMac::getDirectories()
{
    QHash<QString, QList<QString> > out;
    QDir d;
    QString home = QDir::homePath() + "/Library/Launchy";


    d.mkdir(home);
    out["skins"] += qApp->applicationDirPath() + "/../Resources/skins";
    out["skins"] += home + "/skins";
    //out["skins"] += SKINS_PATH;

    out["plugins"] += qApp->applicationDirPath() + "/plugins";
    out["plugins"] += home + "/plugins";
    //out["plugins"] += PLUGINS_PATH;

    out["config"] += home;
    out["portableConfig"] += qApp->applicationDirPath();

    if (QFile::exists(out["skins"].last() + "/Default"))
        out["defSkin"] += out["skins"].last() + "/Default";
    else
      out["defSkin"] += out["skins"].first() + "/Default";

    out["platforms"] += qApp->applicationDirPath();
    //out["platforms"] += PLATFORMS_PATH;

    return out;

}

QList<Directory> PlatformMac::getDefaultCatalogDirectories()
{
    QList<Directory> list;
    QStringList types;
    types << "*.app";

    list.append(Directory("/Applications", types, false, false, 5));
    list.append(Directory("~/Applications", types, false, false, 5));
    list.append(Directory("/System/Library/CoreServices", types, false, false, 5));
    list.append(Directory("~", QStringList(), true, false, 0));    

    return list;
}

QString PlatformMac::expandEnvironmentVars(QString txt)
{
    QStringList list = QProcess::systemEnvironment();
    txt.replace('~', "$HOME$");
    QString delim("$");
    QString out = "";
    int curPos = txt.indexOf(delim, 0);
    if (curPos == -1) return txt;

    while(curPos != -1)
    {
            int nextPos = txt.indexOf("$", curPos+1);
            if (nextPos == -1)
            {
                    out += txt.mid(curPos+1);
                    break;
            }
            QString var = txt.mid(curPos+1, nextPos-curPos-1);
            bool found = false;
            foreach(QString s, list)
            {
                    if (s.startsWith(var, Qt::CaseInsensitive))
                    {
                            found = true;
                            out += s.mid(var.length()+1);
                            break;
                    }
            }
            if (!found)
                    out += "$" + var;
            curPos = nextPos;
    }
    return out;
}

bool PlatformMac::isAlreadyRunning() const
{
    return false;
}

// Create the application object
QApplication* createApplication(int& argc, char** argv)
{
        return new PlatformMac(argc, argv);
}

