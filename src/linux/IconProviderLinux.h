

#pragma once

#include <QIcon>
#include <QString>
#include <QHash>
#include "IconProviderBase.h"
class QFileInfo;

namespace launchy {

class IconProviderLinux : public IconProviderBase {
public:
    IconProviderLinux();
    virtual ~IconProviderLinux();
    virtual QIcon icon(const QFileInfo& info);
    QString getDesktopIcon(QString desktopFile, QString IconName = "");

private:
    QHash<QString, QString> file2mime;
    QHash<QString, QString> mime2desktop;
    QHash<QString, QString> desktop2icon;
    QHash<QString, QString> icon2path;
    QStringList xdgDataDirs;
};

}
