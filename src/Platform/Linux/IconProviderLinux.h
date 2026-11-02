

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

    // The path overload below hides the base one, which would also hide
    // icon(IconType): both overloads have to stay reachable through this type.
    // The signature matches QFileIconProvider::icon(const QFileInfo&) const
    // exactly -- a non-const overload would not override anything and would be
    // silently bypassed whenever this class is used through QFileIconProvider*
    // or IconProviderBase*, which is how IconExtractor holds it.
    using IconProviderBase::icon;
    virtual QIcon icon(const QFileInfo& info) const;
    QString getDesktopIcon(QString desktopFile, QString iconName = "") const;

private:
    // Memoised XDG lookups, filled on demand from the const extraction path:
    // const here means "does not change what the answer is", not "cannot cache".
    mutable QHash<QString, QString> m_file2mime;
    mutable QHash<QString, QString> m_mime2desktop;
    mutable QHash<QString, QString> m_desktop2icon;
    mutable QHash<QString, QString> m_icon2path;
    QStringList m_xdgDataDirs;
};

}
