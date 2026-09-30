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
#include <QElapsedTimer>

#include "Catalog.h"
#include "CatalogFast.h"
#include "AppBase.h"
#include "Directory.h"
#include "SettingsManager.h"
#include "MemProfiler.h"

#define CATALOG_PROGRESS_MIN 0
#define CATALOG_PROGRESS_MAX 100
// The scan runs in a duty cycle: after CATALOG_THROTTLE_WORK_MS of continuous
// work it briefly sleeps, so a rebuild never hogs a whole core and other
// applications stay responsive
#define CATALOG_THROTTLE_WORK_MS 10
#define CATALOG_THROTTLE_SLEEP_MS 1
// Check the duty cycle every this many entries while iterating large dirs
#define CATALOG_THROTTLE_ENTRIES 1024

namespace launchy {

CatalogBuilder* CatalogBuilder::s_instance = nullptr;

CatalogBuilder::CatalogBuilder()
    : m_catalog(new CatalogFast),
      m_thread(new QThread),
      m_progress(CATALOG_PROGRESS_MAX) {
    moveToThread(m_thread);
    m_thread->start(QThread::IdlePriority);
}

void CatalogBuilder::requestBuild() {
    // Ignore the request when a rebuild is already queued or running
    if (m_rebuildScheduled.testAndSetOrdered(0, 1)) {
        QMetaObject::invokeMethod(this, "buildCatalog", Qt::QueuedConnection);
    }
}

void CatalogBuilder::buildCatalog() {
    m_throttleTimer.start();

    m_progress = CATALOG_PROGRESS_MIN;
    emit catalogIncrement(m_progress);
    m_catalog->incrementTimestamp();
    m_indexed.clear();

    launchy::memprof::record("buildCatalog:start");
    // Sample the memory at most once per second while scanning so we can see
    // which catalog directory causes the private working set to jump.
    QElapsedTimer memLogTimer;
    memLogTimer.start();

    PluginHandler& pluginHandler = PluginHandler::instance();
    QList<Directory> catDirs = SettingsManager::instance().readCatalogDirectories();
    const QHash<QString, PluginInfo>& pluginsInfo = pluginHandler.getPlugins();
    m_totalItems = catDirs.count() + pluginsInfo.count();
    m_currentItem = 0;

    while (m_currentItem < catDirs.count()) {

        qDebug() << "CatalogBuilder::buildCatalog, total items:"
                 << m_totalItems << "current item:" << m_currentItem;

        QString currentDir = g_app->expandEnvironmentVars(catDirs[m_currentItem].name);
        indexDirectory(currentDir,
                       catDirs[m_currentItem].types,
                       catDirs[m_currentItem].indexDirs,
                       catDirs[m_currentItem].indexExe,
                       catDirs[m_currentItem].depth);

        progressStep(m_currentItem);

        if (memLogTimer.elapsed() >= 1000) {
            launchy::memprof::record(
                QString("buildCatalog:scan %1").arg(currentDir));
            memLogTimer.restart();
        }

        ++m_currentItem;
    }

    // The file scan is done, free the scan-time path set right away
    m_indexed.clear();
    launchy::memprof::record("buildCatalog:after-dir-scan");

    // Don't call the pluginhandler to request catalog because we need to track progress
    pluginHandler.getCatalogs(m_catalog, this);
    launchy::memprof::record("buildCatalog:after-plugins");

    qDebug() << "CatalogBuilder::buildCatalog, purget old item";

    m_catalog->purgeOldItems();
    launchy::memprof::record("buildCatalog:after-purge");

    qDebug() << "CatalogBuilder::buildCatalog, saving catalog";
    // Save the catalog here on the worker thread: serializing and
    // compressing it in the GUI thread froze the interface and caused
    // a large memory spike there
    {
        launchy::memprof::ScopedMem saveScope("buildCatalog:save");
        m_catalog->save(SettingsManager::instance().catalogFilename());
    }
    launchy::memprof::record("buildCatalog:after-save");

    qDebug() << "CatalogBuilder::buildCatalog, catalog rebuild finished";

    m_progress = CATALOG_PROGRESS_MAX;
    emit catalogFinished();
    launchy::memprof::record("buildCatalog:finished");

    // The rebuild is complete, requests received while it was running
    // were ignored
    m_rebuildScheduled.storeRelease(0);
}


// Briefly give the CPU back so other applications stay responsive while
// the catalog is being rebuilt
void CatalogBuilder::throttle() {
    if (!m_throttleTimer.isValid()) {
        m_throttleTimer.start();
        return;
    }
    if (m_throttleTimer.elapsed() >= CATALOG_THROTTLE_WORK_MS) {
        QThread::msleep(CATALOG_THROTTLE_SLEEP_MS);
        m_throttleTimer.restart();
    }
}

void CatalogBuilder::indexDirectory(const QString& directory,
                                    const QStringList& filters,
                                    bool fDirs,
                                    bool fBin,
                                    int depth) {
    throttle();

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
            if ((i % CATALOG_THROTTLE_ENTRIES) == 0) {
                throttle();
            }
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
            if ((i % CATALOG_THROTTLE_ENTRIES) == 0) {
                throttle();
            }
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
        if ((i % CATALOG_THROTTLE_ENTRIES) == 0) {
            throttle();
        }
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

    int newProgress = (int)(CATALOG_PROGRESS_MAX * (float)m_currentItem / m_totalItems);
    if (newProgress != m_progress) {
        m_progress = newProgress;
        emit catalogIncrement(m_progress);
    }

    return true;
}
}
