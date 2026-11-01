
#pragma once

#include <QSharedPointer>
#include <QSettings>
#include <QAtomicInt>
#include <QString>

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif

#if defined(LAUNCHY_LIB) || defined(PluginInterface_EXPORTS)
#define LAUNCHY_EXPORT Q_DECL_EXPORT
#else
#define LAUNCHY_EXPORT Q_DECL_IMPORT
#endif

namespace launchy {

/* The setting manager in which you store your persistent data.
More data can be found in QT's documentation for QSettings. */
LAUNCHY_EXPORT extern QSharedPointer<QSettings> g_settings;

LAUNCHY_EXPORT extern QAtomicInt g_needRebuildCatalog;

/* The app's scratch directory (created on demand). Pure Qt Core and identical
   on every platform, so it stays in this shared library where the python
   plugin binding can reach it without linking the platform seam. */
LAUNCHY_EXPORT QString getAppTempPath(bool toNative = true);

/* Ask the application to run a program. A plugin (native or python) cannot run
   a program itself without reaching into the platform implementation, which
   lives in the application. This call forwards the request to the handler the
   application installed (see setRunProgramHandler) and returns; the handler is
   what runs it through the platform. That indirection is what keeps the
   plugin ABI free of the platform seam -- from here it still looks like the
   plain runProgram() call a plugin wants to make. */
LAUNCHY_EXPORT void runProgram(const QString& file, const QString& args);

/* The application end of runProgram(): point it at the platform once at
   start up. A plugin never calls this.

   Delivery is a plain call in the requesting thread -- there is no queue, no
   timer and no event hop, so whatever the handler does has happened by the time
   runProgram() returns. That is enough here because every plugin in this tree
   requests a launch as the result of an item the user picked, i.e. from the GUI
   thread, which is the thread a new window has to belong to anyway.

   Pass nullptr to unhook; requests are then dropped. */
typedef void (*RunProgramHandler)(const QString& file, const QString& args);
LAUNCHY_EXPORT void setRunProgramHandler(RunProgramHandler handler);

} // namespace launchy
