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
#include <QDir>

#include "Catalog.h"
#include "AppBase.h"
#include "Directory.h"
#include "SettingsManager.h"

#define CATALOG_PROGRESS_MIN 0
#define CATALOG_PROGRESS_MAX 100

namespace launchy {

CatalogBuilder* CatalogBuilder::s_instance = nullptr;

CatalogBuilder::CatalogBuilder()
    : m_catalog(new SlowCatalog),
      m_thread(new QThread),
      m_progress(CATALOG_PROGRESS_MAX) {
    moveToThread(m_thread);
    m_thread->start(QThread::IdlePriority);
}

void CatalogBuilder::buildCatalog() {
    m_progress = CATALOG_PROGRESS_MIN;
    emit catalogIncrement(m_progress);
    m_catalog->incrementTimestamp();
    m_indexed.clear();

    PluginHandler& pluginHandler = PluginHandler::instance();
    QList<Directory> catDirs = SettingsManager::instance().readCatalogDirectories();
    const QHash<QString, PluginInfo>& pluginsInfo = pluginHandler.getPlugins();
    m_totalItems = catDirs.count() + pluginsInfo.count();
    m_currentItem = 0;

    while (m_currentItem < catDirs.count()) {
        QString currentDir = g_app->expandEnvironmentVars(catDirs[m_currentItem].name);
        indexDirectory(currentDir,
                       catDirs[m_currentItem].types,
                       catDirs[m_currentItem].indexDirs,
                       catDirs[m_currentItem].indexExe,
                       catDirs[m_currentItem].depth);
        progressStep(m_currentItem);
    }

    // Don't call the pluginhandler to request catalog because we need to track progress
    pluginHandler.getCatalogs(m_catalog, this);

    m_catalog->purgeOldItems();
    m_indexed.clear();
    m_progress = CATALOG_PROGRESS_MAX;
    emit catalogFinished();
}

void CatalogBuilder::indexDirectory(const QString& directory,
                                    const QStringList& filters,
                                    bool fDirs,
                                    bool fBin,
                                    int depth) {
    QString dir = QDir::toNativeSeparators(directory);
    QDir qDir(dir);
    dir = qDir.absolutePath();
    QStringList dirs = qDir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);

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
                        indexDirectory(dir + "/" + dirs[i], filters, fDirs, fBin, depth-1);
                }
            }
        }
    }

    if (fDirs) {
        for (int i = 0; i < dirs.count(); ++i) {
            if (!dirs[i].startsWith(".") && !m_indexed.contains(dir + "/" + dirs[i])) {
                bool isShortcut = dirs[i].endsWith(".lnk", Qt::CaseInsensitive);

                CatItem item(dir + "/" + dirs[i], !isShortcut);
                m_catalog->addItem(item);
                m_indexed.insert(dir + "/" + dirs[i]);
            }
        }
    }
    else {
        // Grab any shortcut directories
        // This is to work around a QT weirdness that treats shortcuts to directories as actual directories
        for (int i = 0; i < dirs.count(); ++i) {
            if (!dirs[i].startsWith(".")
                && dirs[i].endsWith(".lnk", Qt::CaseInsensitive)) {
                if (!m_indexed.contains(dir + "/" + dirs[i])) {
                    CatItem item(dir + "/" + dirs[i], true);
                    m_catalog->addItem(item);
                    m_indexed.insert(dir + "/" + dirs[i]);
                }
            }
        }
    }

    if (fBin) {
        QStringList bins = qDir.entryList(QDir::Files | QDir::Executable);
        for (int i = 0; i < bins.count(); ++i) {
            if (!m_indexed.contains(dir + "/" + bins[i])) {
                CatItem item(dir + "/" + bins[i]);
                m_catalog->addItem(item);
                m_indexed.insert(dir + "/" + bins[i]);
            }
        }
    }

    // Don't want a null file filter, that matches everything..
    if (filters.empty()) {
        return;
    }

    QStringList files = qDir.entryList(filters, QDir::Files | QDir::System, QDir::Unsorted);
    for (int i = 0; i < files.count(); ++i) {
        if (!m_indexed.contains(dir + "/" + files[i])) {
            CatItem item(dir + "/" + files[i]);
            g_app->alterItem(&item);
#ifdef Q_OS_LINUX
            if (item.fullPath.endsWith(".desktop") && item.iconPath.isEmpty()) {
                continue;
            }
#endif
            m_catalog->addItem(item);

            m_indexed.insert(dir + "/" + files[i]);
        }
    }
}

CatalogBuilder::~CatalogBuilder() {
    s_instance = nullptr;
    qDebug() << "CatalogBuilder::~CatalogBuilder, exit thread";
    if (m_thread) {
        m_thread->exit();
        m_thread->wait();
        m_thread->deleteLater();
        m_thread = nullptr;
    }

    if (m_catalog) {
        delete m_catalog;
        m_catalog = nullptr;
    }
}

CatalogBuilder* CatalogBuilder::instance() {
    if (!s_instance) {
        s_instance = new CatalogBuilder;
    }
    return s_instance;
}

void CatalogBuilder::cleanup() {
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

Catalog* CatalogBuilder::getCatalog() {
    return instance()->m_catalog;
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
