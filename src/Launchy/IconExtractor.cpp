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
#include <QMutexLocker>

#include "AppBase.h"
#include "GlobalVar.h"

namespace launchy {

IconExtractor::IconExtractor() {
    // The worker thread runs for the lifetime of this object: it is started
    // here and then blocks on the condition variable inside run() waiting for
    // requests.
    start(LowPriority);
}

IconExtractor::~IconExtractor() {
    // Ask the thread to exit and wait for it to finish, so the object is never
    // destroyed while the thread is still running.
    {
        QMutexLocker locker(&m_mutex);
        m_exiting = true;
    }
    m_condition.wakeAll();
    wait();
}

void IconExtractor::processIcon(const CatItem& item, bool highPriority) {
    int totalCount = 0;

    {
        QMutexLocker locker(&m_mutex);
        if (highPriority) {
            m_items.push_front(item);
        }
        else {
            m_items.push_back(item);
        }
        totalCount = m_items.size();
    }

    qDebug() << "IconExtractor::processIcon, item path:" << item.fullPath
             << "total item count:" << totalCount;

    // Wake the worker thread to process the request.
    // If the thread is currently busy (mutex released while extracting an
    // icon) this wakeOne() is lost, but run() re-checks the queue on its next
    // loop iteration, so the request is never dropped.
    m_condition.wakeOne();
}

void IconExtractor::processIcons(const QList<CatItem>& items, bool reset) {
    Q_UNUSED(reset);

    int totalCount = 0;

    {
        QMutexLocker locker(&m_mutex);
        if (reset) {
            m_items.clear();
        }
        m_items += items;
        totalCount = m_items.size();
    }

    qDebug() << "IconExtractor::processIcons, new item count:" << items.size()
             << "total item count:" << totalCount;

    m_condition.wakeOne();
}

void IconExtractor::stop() {
    // Only clear the pending queue: drop icon requests that have not started
    // extracting yet. The worker thread itself keeps running and waits for
    // future requests; it is not terminated here.

    qDebug() << "IconExtractor::stop, clearing pending icon requests";

    QMutexLocker locker(&m_mutex);
    m_items.clear();
}

void IconExtractor::run() {
    forever {
        CatItem item;

        {
            QMutexLocker locker(&m_mutex);
            // Block while the queue is empty and no exit has been requested,
            // avoiding a busy-wait.
            while (m_items.isEmpty() && !m_exiting) {
                m_condition.wait(&m_mutex);
            }

            // Exit requested: end the thread (clear any leftovers first so we
            // don't keep extracting after destruction begins).
            if (m_exiting) {
                m_items.clear();
                break;
            }

            item = m_items.dequeue();

            qDebug() << "IconExtractor::run, item path:" << item.fullPath
                 << "items remaining:" << m_items.size();
        }

        QIcon icon = getIcon(item);
        emit iconExtracted(item.pluginName, item.fullPath, icon);
    }
}

QIcon IconExtractor::getIcon(const CatItem& item) {
    qDebug() << "IconExtractor::getIcon, fetching icon for" << item.fullPath;

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
