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
#include <QWaitCondition>

#include "PluginInterface/CatalogItem.h"

namespace launchy {

class IconProviderBase;

// The background worker that turns indexed items into icons, and the only place
// in the tree that produces a QIcon out of the file system.
//
// It lives in the platform library because extracting an icon is the one thing
// here that is really an OS question: which file to look at, and what to do
// when a .desktop entry points at a name instead of a path, differs per
// platform (see getIcon()). The class itself is pure scheduling -- a queue, one
// low-priority thread and a signal -- so it owns no state that a second copy per
// linker could disagree about: the widget that runs it holds the only instance.
//
// The icon provider is owned here rather than by PlatformBase on purpose.
// Naming QFileIconProvider on the platform seam would put a QtWidgets type in
// Platform.h and drag the GUI into every consumer of it, the headless tests
// included; an icon is only ever wanted by the widget, so the provider belongs
// to the thing that extracts them.
//
// Callers use it as a plain QObject: processIcon(s) to queue, stop() to drop
// what has not started yet, and iconExtracted() for the result. The thread is
// started in the constructor and waits on a condition variable between requests.
class IconExtractor : public QThread {
    Q_OBJECT
public:
    IconExtractor();
    ~IconExtractor() override;

public:
    void processIcon(const CatItem& item, bool highPriority = false);
    void processIcons(const QList<CatItem>& items, bool reset = true);
    void stop();

    // Edge length the icons are extracted at. Called when the skin has decided
    // how large the output icon is; the provider picks its shell image list
    // from this, so it has to be set before the first request.
    void setPreferredIconSize(int size);

protected:
    void run() override;

signals:
    void iconExtracted(const QString& pluginName,
                       const QString& path,
                       const QIcon& icon);

private:
    QIcon getIcon(const CatItem& item);

    // The cache behind every answer getIcon() gives: created with the object,
    // used only from the worker thread, destroyed with it.
    IconProviderBase* m_provider;

    QMutex m_mutex;
    QWaitCondition m_condition;
    QQueue<CatItem> m_items;
    // Set to true when a stop is requested; run() ends the thread after the
    // current request (if any) has been processed.
    bool m_exiting = false;
};

} // namespace launchy
