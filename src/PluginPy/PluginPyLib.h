#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(PLUGINPY_LIB)
#  define PLUGINPY_EXPORT Q_DECL_EXPORT
# else
#  define PLUGINPY_EXPORT Q_DECL_IMPORT
# endif
#else
# define PLUGINPY_EXPORT
#endif
