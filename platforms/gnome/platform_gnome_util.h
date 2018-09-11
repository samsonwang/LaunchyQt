#ifndef PLATFORM_GNOME_UTIL_H
#define PLATFORM_GNOME_UTIL_H
//#include <qdatastream.h>
//#include <
#include <QtGui>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>


#include <QFileIconProvider>

#include <QWidget>

class GnomeIconProvider : QFileIconProvider
{
 public:
    GnomeIconProvider() {}
    ~GnomeIconProvider() {}
    QIcon icon(const QFileInfo& info) const;
};


#endif
