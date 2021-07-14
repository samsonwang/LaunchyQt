#pragma once

#include <QtGlobal>

#ifndef BUILD_STATIC
# if defined(PLUGINPY_LIB) || defined(PluginPy_EXPORTS)
#  define PLUGINPY_EXPORT Q_DECL_EXPORT
# else
#  define PLUGINPY_EXPORT Q_DECL_IMPORT
# endif
#else
# define PLUGINPY_EXPORT
#endif
