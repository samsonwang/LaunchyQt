
#pragma once

#include <QSharedPointer>
#include <QSettings>
#include <QAtomicInt>

#if defined(_MSC_VER)
#pragma warning(disable : 4251)
#endif

#if defined LAUNCHY_LIB
#define LAUNCHY_EXPORT Q_DECL_EXPORT
#else
#define LAUNCHY_EXPORT Q_DECL_IMPORT
#endif

namespace launchy {

/* The setting manager in which you store your persistent data.
More data can be found in QT's documentation for QSettings. */
LAUNCHY_EXPORT extern QSharedPointer<QSettings> g_settings;

LAUNCHY_EXPORT extern QAtomicInt g_needRebuildCatalog;

/* A convienience run function
This function will run the program along with arguments and is platform independent.
param file The location of the file to run
param args The arguments to the command
param translateSeparators Whether to translate / into native separators */
LAUNCHY_EXPORT void runProgram(const QString& file, const QString& args, bool translateSeparators = true);

LAUNCHY_EXPORT int getDesktop();

}
