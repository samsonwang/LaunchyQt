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

#include "IconExtractor.h"

#include <QDir>

#include "AppBase.h"
#include "GlobalVar.h"

namespace launchy {

IconExtractor::IconExtractor() {
}

void IconExtractor::processIcon(const CatItem& item, bool highPriority) {
    m_mutex.lock();

    if (highPriority) {
        m_items.push_front(item);
    }
    else {
        m_items.push_back(item);
    }

    m_mutex.unlock();

    if (!isRunning()) {
        start(LowPriority);
    }
}

void IconExtractor::processIcons(const QList<CatItem>& newItems, bool reset) {
    m_mutex.lock();

    // int itemCount = m_items.size();

    /*
    if (reset && itemCount > 0 && isRunning()) {
        // reset the queue, but keep the most recent high priority item
        // CatItem item = m_items.dequeue();
        m_items.clear();
        // if (item.pluginName == (uint)-1)
        // m_items.append(item);
        itemCount = m_items.size();
    }
    */

    m_items += newItems;
    // for (int i = itemCount; i < m_items.size(); ++i)
        // m_items[i].pluginName = i - itemCount;

    m_mutex.unlock();

    if (!isRunning()) {
        start(IdlePriority);
    }
}

void IconExtractor::stop() {
    m_mutex.lock();
    m_items.clear();
    m_mutex.unlock();
}

void IconExtractor::run() {

    bool itemsRemaining = true;

    do {
        CatItem item;

        m_mutex.lock();
        itemsRemaining = m_items.size() > 0;
        if (itemsRemaining)
            item = m_items.dequeue();
        m_mutex.unlock();

        if (itemsRemaining) {
            QIcon icon = getIcon(item);
            emit iconExtracted(item.pluginName, item.fullPath, icon);
        }
    } while (itemsRemaining);
}

QIcon IconExtractor::getIcon(const CatItem& item) {
    qDebug() << "IconExtractor::getIcon, Fetching icon for" << item.fullPath;

#ifdef Q_OS_MAC
    if (item.iconPath.endsWith(".png") || item.iconPath.endsWith(".ico"))
        return QIcon(item.iconPath);
#endif

    if (item.iconPath.isNull()) {
#ifdef Q_OS_LINUX
        QFileInfo info(item.fullPath);
        if (info.isDir())
            return g_app->icon(QFileIconProvider::Folder);
#endif
        if (item.fullPath.isEmpty())
            return QIcon();
        return g_app->icon(QDir::toNativeSeparators(item.fullPath));
    }
    else {
#ifdef Q_OS_LINUX
        if (QFile::exists(item.iconPath)) {
            return QIcon(item.iconPath);
        }
#endif
        return g_app->icon(QDir::toNativeSeparators(item.iconPath));
    }
}

} // namespace launchy
