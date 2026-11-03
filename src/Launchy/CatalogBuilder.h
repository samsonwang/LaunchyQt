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

#pragma once

#include <QObject>
#include <QAtomicInt>
#include <QElapsedTimer>
#include "Core/PluginRegistry.h"
class QThread;

namespace launchy {

class Catalog;

// Not a singleton: LaunchyWidget owns exactly one by value (m_catalogBuilder).
// The builder does not own the catalog either -- LaunchyWidget owns that and
// passes it in for every rebuild, so ownership stays in one place. What lives
// here is only the scan state, which belongs to the worker thread the
// constructor moves this object to.
class CatalogBuilder : public QObject, public INotifyProgressStep {
    Q_OBJECT
public:
    CatalogBuilder();
    // Stops the worker thread, so a builder can be a plain value member of its
    // owner.
    virtual ~CatalogBuilder();
    Q_DISABLE_COPY(CatalogBuilder)

    int getProgress() const;
    int isRunning() const;
    virtual bool progressStep(int newStep);

    // Stops the worker thread; idempotent, the destructor calls it too. Owners
    // that have to release something the scan touches before the builder itself
    // goes away call it explicitly -- LaunchyWidget does, it owns the catalog
    // and deletes it from its own destructor body.
    void shutdown();

    // Thread-safe. Request a rebuild of `catalog`, owned by the caller and
    // required to outlive this builder; requests that arrive while a rebuild is
    // queued or running are ignored.
    void requestBuild(Catalog* catalog);

signals:
    void catalogIncrement(int);
    void catalogFinished();

private:
    void buildCatalog(Catalog* catalog);
    void indexDirectory(Catalog* catalog, const QString& dir, const QStringList& filters,
                        bool fdirs, bool fbin, int depth);
    // Briefly yield the CPU so the scan does not starve other applications
    void throttle();
private:
    QThread* m_thread;

    QSet<QString> m_indexed;
    // Read on the GUI thread, written on the worker thread
    QAtomicInt m_progress;
    int m_currentItem;
    int m_totalItems;

    QElapsedTimer m_throttleTimer;
    QAtomicInt m_rebuildScheduled{0}; // a rebuild is queued or running
};

} // namespace launchy
