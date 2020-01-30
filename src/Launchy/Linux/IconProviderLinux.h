

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
    QString getDesktopIcon(QString desktopFile, QString iconName = "");

private:
    QHash<QString, QString> m_file2mime;
    QHash<QString, QString> m_mime2desktop;
    QHash<QString, QString> m_desktop2icon;
    QHash<QString, QString> m_icon2path;
    QStringList m_xdgDataDirs;
};

}
