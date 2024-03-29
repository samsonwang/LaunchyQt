/*
Launchy: Application Launcher
Copyright (C) 2009 Simon Capewell, Josh Karlin

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

#pragma once

#include <QThread>
#include <QList>
#include <QQueue>
#include <QString>
#include <QIcon>
#include <QMutex>

#include "LaunchyLib/CatalogItem.h"

namespace launchy {

class IconExtractor : public QThread {
    Q_OBJECT
public:
    IconExtractor();

public:
    void processIcon(const CatItem& item, bool highPriority = false);
    void processIcons(const QList<CatItem>& newItems, bool reset = true);
    void stop();

protected:
    virtual void run();

signals:
    void iconExtracted(const QString& pluginName,
                       const QString& path,
                       const QIcon& icon);

private:
    QIcon getIcon(const CatItem& item);

    QMutex m_mutex;
    QQueue<CatItem> m_items;
};

} // namespace launchy
