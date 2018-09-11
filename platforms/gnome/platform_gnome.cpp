/*
  Launchy: Application Launcher
  Copyright (C) 2007  Josh Karlin
  
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

#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnome/gnome-desktop-item.h>
#include <QtGui>
#include <QApplication>
#include <QX11Info>
#include "platform_gnome.h"
#include "platform_gnome_util.h"

PlatformGnome::PlatformGnome() : PlatformUnix () 		
{
//    if (icons)
//		delete icons;
	icons.reset((QFileIconProvider*) new GnomeIconProvider());
//    icons = (QFileIconProvider*) new GnomeIconProvider();
}

QApplication* PlatformGnome::init(int* argc, char** argv)
{
    // MUST CALL GTK BEFORE QAPP!  Otherwise things get confused
    g_thread_init(NULL); // necessary since gtk called from catalog thread
    gdk_threads_init();
    gtk_init(argc, &argv);
    gnome_vfs_init();
    
        
    QApplication * app = new QApplication(*argc, argv);
    icons.reset((QFileIconProvider*) new GnomeIconProvider());
    return app;
}

PlatformGnome::~PlatformGnome()
{ 
    GlobalShortcutManager::clear();
    gnome_vfs_shutdown();
//    delete icons;
}


void PlatformGnome::alterItem(CatItem* item) {
    
    if (!item->fullPath.endsWith(".desktop", Qt::CaseInsensitive))
	return;
    GError * error = NULL;

    gdk_threads_enter();
    GnomeDesktopItem* ditem = gnome_desktop_item_new_from_file(item->fullPath.toLocal8Bit().data(),
							       (GnomeDesktopItemLoadFlags) 0,//GNOME_DESKTOP_ITEM_LOAD_ONLY_IF_EXISTS,
							       &error);
    if (error) {
	g_error_free(error);
	gdk_threads_leave();
	return;
    }

    if (!ditem) {
	gdk_threads_leave();
	return;
    }

    // The gnome errors are coming from here, because it falls back to
    // finding the file if it can't find the icon and spits out a
    // nasty error.  Nothing I can do but reimplement get_icon, pretty
    // annoying behavior on gnome's part
   
    GtkIconTheme* thm = gtk_icon_theme_get_for_screen(gdk_screen_get_default());
    //    item->icon = gnome_desktop_item_get_icon (ditem, gtk_icon_theme_get_default());
    item->icon = gnome_desktop_item_get_icon(ditem, thm);
    
    //item->fullPath = gnome_desktop_item_get_localestring(ditem, "Exec");
    QString name = gnome_desktop_item_get_localestring(ditem, "Name");
    if (name.size() >= item->shortName.size() - 8) {
	item->shortName = name;
	item->lowName = item->shortName.toLower();
    }
    
	
    gnome_desktop_item_unref (ditem);    
    gdk_threads_leave();
    return;
}

bool PlatformGnome::Execute(QString path, QString args)
{
    if (!path.endsWith(".desktop", Qt::CaseInsensitive))
	return false;

    gdk_threads_enter();

    GError * error = NULL;
    GnomeDesktopItem* ditem = gnome_desktop_item_new_from_file(path.toLocal8Bit().data(),
							       (GnomeDesktopItemLoadFlags) 0,
							       &error);
    if (error) {
	g_error_free(error);
	gdk_threads_leave();
	return false;
    }
    if (!ditem) return false;
    
    // Get the args
    GList * list = NULL;
    args = args.trimmed();
    if (args.size() > 0) {
	foreach(QString s, args.split(" ")) {
	    list = g_list_append(list, s.toLocal8Bit().data());
	}
    }

    
    gnome_desktop_item_launch(ditem, list, (GnomeDesktopItemLaunchFlags) 0, &error);

    g_list_free(list);

    if (error) {
	g_error_free(error);
	gnome_desktop_item_unref(ditem);
	gdk_threads_leave();
	return false;
    }
			      
    gnome_desktop_item_unref(ditem);
    gdk_threads_leave();
    return true;
}

Q_EXPORT_PLUGIN2(platform_gnome, PlatformGnome)
