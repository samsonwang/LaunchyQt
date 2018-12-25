
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

LAUNCHY_EXPORT extern QSharedPointer<QSettings> g_settings;
LAUNCHY_EXPORT extern QAtomicInt g_needRebuildCatalog;

}