/*
  Launchy: Application Launcher
  Copyright (C) 2007-2010  Josh Karlin, Simon Capewell

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

#include "CatalogBuilder.h"
#include <QThread>
#include "Catalog.h"
#include "GlobalVar.h"
#include "AppBase.h"
#include "Directory.h"
#include "SettingsManager.h"

#define CATALOG_PROGRESS_MIN 0
#define CATALOG_PROGRESS_MAX 100

namespace launchy {
CatalogBuilder::CatalogBuilder()
    : m_thread(new QThread),
      m_progress(CATALOG_PROGRESS_MAX) {
    g_catalog.reset(new SlowCatalog);
    moveToThread(m_thread);
    m_thread->start(QThread::IdlePriority);
}

void CatalogBuilder::buildCatalog() {
    m_progress = CATALOG_PROGRESS_MIN;
    emit catalogIncrement(m_progress);
    g_catalog->incrementTimestamp();
    m_indexed.clear();

    PluginHandler& pluginHandler = PluginHandler::instance();
    QList<Directory> memDirs = SettingsManager::instance().readCatalogDirectories();
    const QHash<uint, PluginInfo>& pluginsInfo = pluginHandler.getPlugins();
    m_totalItems = memDirs.count() + pluginsInfo.count();
    m_currentItem = 0;

    while (m_currentItem < memDirs.count()) {
        QString cur = g_app->expandEnvironmentVars(memDirs[m_currentItem].name);
        indexDirectory(cur, memDirs[m_currentItem].types, memDirs[m_currentItem].indexDirs,
                       memDirs[m_currentItem].indexExe, memDirs[m_currentItem].depth);
        progressStep(m_currentItem);
    }

    // Don't call the pluginhandler to request catalog because we need to track progress
    pluginHandler.getCatalogs(g_catalog.data(), this);

    g_catalog->purgeOldItems();
    m_indexed.clear();
    m_progress = CATALOG_PROGRESS_MAX;
    emit catalogFinished();
}

void CatalogBuilder::indexDirectory(const QString& directory,
                                    const QStringList& filters,
                                    bool fdirs,
                                    bool fbin,
                                    int depth) {
    QString dir = QDir::toNativeSeparators(directory);
    QDir qd(dir);
    dir = qd.absolutePath();
    QStringList dirs = qd.entryList(QDir::AllDirs);

    if (depth > 0) {
        for (int i = 0; i < dirs.count(); ++i) {
            if (!dirs[i].startsWith(".")) {
                QString cur = dirs[i];
                if (!cur.contains(".lnk")) {
#ifdef Q_OS_MAC
                    // Special handling of app directories
                    if (cur.endsWith(".app", Qt::CaseInsensitive)) {
                        CatItem item(dir + "/" + cur);
                        g_app->alterItem(&item);
                        g_catalog->addItem(item);
                    }
                    else
#endif
                        indexDirectory(dir + "/" + dirs[i], filters, fdirs, fbin, depth-1);
                }
            }
        }
    }

    if (fdirs) {
        for (int i = 0; i < dirs.count(); ++i) {
            if (!dirs[i].startsWith(".") && !m_indexed.contains(dir + "/" + dirs[i])) {
                bool isShortcut = dirs[i].endsWith(".lnk", Qt::CaseInsensitive);

                CatItem item(dir + "/" + dirs[i], !isShortcut);
                g_catalog->addItem(item);
                m_indexed.insert(dir + "/" + dirs[i]);
            }
        }
    }
    else {
        // Grab any shortcut directories
        // This is to work around a QT weirdness that treats shortcuts to directories as actual directories
        for (int i = 0; i < dirs.count(); ++i) {
            if (!dirs[i].startsWith(".") && dirs[i].endsWith(".lnk", Qt::CaseInsensitive)) {
                if (!m_indexed.contains(dir + "/" + dirs[i])) {
                    CatItem item(dir + "/" + dirs[i], true);
                    g_catalog->addItem(item);
                    m_indexed.insert(dir + "/" + dirs[i]);
                }
            }
        }
    }

    if (fbin) {
        QStringList bins = qd.entryList(QDir::Files | QDir::Executable);
        for (int i = 0; i < bins.count(); ++i) {
            if (!m_indexed.contains(dir + "/" + bins[i])) {
                CatItem item(dir + "/" + bins[i]);
                g_catalog->addItem(item);
                m_indexed.insert(dir + "/" + bins[i]);
            }
        }
    }

    // Don't want a null file filter, that matches everything..
    if (filters.count() == 0)
        return;

    QStringList files = qd.entryList(filters, QDir::Files | QDir::System, QDir::Unsorted);
    for (int i = 0; i < files.count(); ++i) {
        if (!m_indexed.contains(dir + "/" + files[i])) {
            CatItem item(dir + "/" + files[i]);
            g_app->alterItem(&item);
#ifdef Q_OS_LINUX
            if (item.fullPath.endsWith(".desktop") && item.iconPath == "")
                continue;
#endif
            g_catalog->addItem(item);

            m_indexed.insert(dir + "/" + files[i]);
        }
    }
}

CatalogBuilder::~CatalogBuilder() {
    qDebug() << "CatalogBuilder::~CatalogBuilder, exit thread";
    m_thread->exit();
    m_thread->deleteLater();
}

int CatalogBuilder::getProgress() const {
    return m_progress;
}

int CatalogBuilder::isRunning() const {
    return m_progress < CATALOG_PROGRESS_MAX;
}

bool CatalogBuilder::progressStep(int newStep) {
    newStep = newStep;

    ++m_currentItem;
    int newProgress = (int)(CATALOG_PROGRESS_MAX * (float)m_currentItem / m_totalItems);
    if (newProgress != m_progress) {
        m_progress = newProgress;
        emit catalogIncrement(m_progress);
    }

    return true;
}
}
