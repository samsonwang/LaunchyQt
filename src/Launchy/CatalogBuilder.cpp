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
#include <QDebug>
#include <QElapsedTimer>

#include "Platform/Platform.h"

#include "Catalog.h"
#include "Directory.h"
#include "SettingsManager.h"
#include "MemProfiler.h"

#define CATALOG_PROGRESS_MIN 0
#define CATALOG_PROGRESS_MAX 100
// The scan runs in a duty cycle: after CATALOG_THROTTLE_WORK_MS of continuous
// work it briefly sleeps, so a rebuild never hogs a whole core and other
// applications stay responsive
#define CATALOG_THROTTLE_WORK_MS 100
#define CATALOG_THROTTLE_SLEEP_MS 5
// Check the duty cycle every this many entries while iterating large dirs
#define CATALOG_THROTTLE_ENTRIES 1024

namespace launchy {

CatalogBuilder::CatalogBuilder()
    : m_thread(new QThread),
      m_progress(CATALOG_PROGRESS_MAX) {
    moveToThread(m_thread);
    m_thread->start(QThread::IdlePriority);
}

void CatalogBuilder::requestBuild(Catalog* catalog) {
    // Ignore the request when a rebuild is already queued or running. The
    // catalog pointer travels with the queued call instead of being a member:
    // the builder owns no catalog, so nothing here has to be kept in sync with
    // the widget that does.
    if (m_rebuildScheduled.testAndSetOrdered(0, 1)) {
        QMetaObject::invokeMethod(this, [this, catalog] { buildCatalog(catalog); },
                                  Qt::QueuedConnection);
    }
}

void CatalogBuilder::buildCatalog(Catalog* catalog) {
    m_throttleTimer.start();

    m_progress.storeRelease(CATALOG_PROGRESS_MIN);
    emit catalogIncrement(m_progress.loadAcquire());
    catalog->incrementTimestamp();
    m_indexed.clear();

    launchy::memprof::record("buildCatalog:start");
    // Sample the memory at most once per second while scanning so we can see
    // which catalog directory causes the private working set to jump.
    QElapsedTimer memLogTimer;
    memLogTimer.start();

    PluginRegistry& pluginRegistry = PluginRegistry::instance();
    QList<Directory> catDirs = SettingsManager::instance().readCatalogDirectories();
    // Snapshot taken under the plugin handler lock, the table may be rebuilt
    // by the option dialog while the scan continues
    const QHash<QString, PluginInfo> pluginsInfo = pluginRegistry.getPlugins();
    m_totalItems = catDirs.count() + pluginsInfo.count();
    m_currentItem = 0;

    while (m_currentItem < catDirs.count()) {

        qDebug() << "CatalogBuilder::buildCatalog, total items:"
                 << m_totalItems << "current item:" << m_currentItem;

        QString currentDir = platform().expandEnvironmentVars(catDirs[m_currentItem].name);
        indexDirectory(catalog,
                       currentDir,
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
    pluginRegistry.getCatalogs(catalog, this);
    launchy::memprof::record("buildCatalog:after-plugins");

    qDebug() << "CatalogBuilder::buildCatalog, purget old item";

    catalog->purgeOldItems();
    launchy::memprof::record("buildCatalog:after-purge");

    qDebug() << "CatalogBuilder::buildCatalog, saving catalog";
    // Save the catalog here on the worker thread: serializing and
    // compressing it in the GUI thread froze the interface and caused
    // a large memory spike there
    {
        launchy::memprof::ScopedMem saveScope("buildCatalog:save");
        catalog->save(SettingsManager::instance().catalogFilename());
    }
    launchy::memprof::record("buildCatalog:after-save");

    qDebug() << "CatalogBuilder::buildCatalog, catalog rebuild finished";

    m_progress.storeRelease(CATALOG_PROGRESS_MAX);
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

void CatalogBuilder::indexDirectory(Catalog* catalog,
                                    const QString& directory,
                                    const QStringList& filters,
                                    bool fDirs,
                                    bool fBin,
                                    int depth) {
    throttle();

    QString dir = QDir::toNativeSeparators(directory);
    QDir qDir(dir);
    dir = qDir.absolutePath();
    QStringList dirs = qDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    if (depth > 0) {
        for (int i = 0; i < dirs.count(); ++i) {
            if (!dirs[i].startsWith(".")) {
                QString cur = dirs[i];
                if (!cur.endsWith(".lnk", Qt::CaseInsensitive)) {
#ifdef Q_OS_MAC
                    // Special handling of app directories
                    if (cur.endsWith(".app", Qt::CaseInsensitive)) {
                        CatItem item(dir + "/" + cur);
                        platform().alterItem(&item);
                        catalog->addItem(item);
                    }
                    else
#endif
                        indexDirectory(catalog, dir + "/" + dirs[i], filters,
                                       fDirs, fBin, depth-1);
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
                catalog->addItem(item);
                m_indexed.insert(dir + "/" + dirs[i]);
            }
        }
    }
    else {
        // Grab any shortcut directories
        // This is to work around a QT weirdness that treats shortcuts to directories as actual directories
        for (int i = 0; i < dirs.count(); ++i) {
            if ((i % CATALOG_THROTTLE_ENTRIES) == 0) {
                throttle();
            }
            if (!dirs[i].startsWith(".")
                && dirs[i].endsWith(".lnk", Qt::CaseInsensitive)) {
                if (!m_indexed.contains(dir + "/" + dirs[i])) {
                    CatItem item(dir + "/" + dirs[i], true);
                    catalog->addItem(item);
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
                catalog->addItem(item);
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
            platform().alterItem(&item);

#ifdef Q_OS_LINUX
            if (item.fullPath.endsWith(".desktop") && item.iconPath.isEmpty()) {
                continue;
            }
#endif
            catalog->addItem(item);

            m_indexed.insert(dir + "/" + files[i]);
        }
    }
}

void CatalogBuilder::shutdown() {
    if (!m_thread) {
        return;
    }

    // Return only once a running scan is done: the caller may delete the catalog
    // right after, and the scan writes to it.
    m_thread->exit();
    m_thread->wait();
    // wait() returned, so the thread is not running any more and the
    // QThread object itself can be deleted right here. No event loop is
    // guaranteed to be left at teardown time, so deleteLater() would leak.
    delete m_thread;
    m_thread = nullptr;
}

CatalogBuilder::~CatalogBuilder() {
    qDebug() << "CatalogBuilder::~CatalogBuilder, exit thread";
    shutdown();
}


int CatalogBuilder::getProgress() const {
    return m_progress.loadAcquire();
}

int CatalogBuilder::isRunning() const {
    return m_progress.loadAcquire() < CATALOG_PROGRESS_MAX;
}

bool CatalogBuilder::progressStep(int newStep) {
    Q_UNUSED(newStep)

    // Guard against division by zero: with no items to scan the progress is
    // already complete, so report the maximum rather than computing NaN/Inf.
    int newProgress = CATALOG_PROGRESS_MAX;
    if (m_totalItems > 0) {
        newProgress = (int)(CATALOG_PROGRESS_MAX * (float)m_currentItem / m_totalItems);
    }
    if (newProgress != m_progress.loadAcquire()) {
        m_progress.storeRelease(newProgress);
        emit catalogIncrement(m_progress.loadAcquire());
    }

    return true;
}
}
