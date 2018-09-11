/*
  Launchy: Application Launcher
  Copyright (C) 2008  Josh Karlin
  
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

#include "platform_unix_util.h"
#include "platform_base.h"
#include "platform_base_hotkey.h"
#include "platform_base_hottrigger.h"

#include <QX11Info>
#include <boost/shared_ptr.hpp>

#include <X11/X.h>
#include <X11/Xlib.h>

using namespace boost;

/*
  This QAPP scans all x events for keypresses
  and sends them off to the hotkey manager
*/

/*
class MyApp : public QApplication {
    Q_OBJECT
    public:
        MyApp(int argc, char** argv) : QApplication(argc,argv) {}
	bool x11EventFilter ( XEvent * event ) {
	    if (event->type == KeyPress) {
		emit xkeyPressed(event);
	    }
	return false;
    }    
signals:
    void xkeyPressed(XEvent*);
};
*/
class PlatformUnix :  public PlatformBase
{
    Q_OBJECT

    bool x11EventFilter ( XEvent * event ) {
        if (event->type == KeyPress) {
            emit xkeyPressed(event);
        }
        return false;
    }

    QKeySequence oldKey;
 public:
    PlatformUnix(int & argc, char** argv);
    ~PlatformUnix();
    
    void setPreferredIconSize(int size) { size = size; return; }

    //virtual shared_ptr<QApplication> init(int & argc, char** argv);
    // Mandatory functions
    // Mandatory functions
    bool setHotkey(const QKeySequence& key, QObject* receiver, const char* slot)
    {

	GlobalShortcutManager::disconnect(oldKey, receiver, slot);
	GlobalShortcutManager::connect(key, receiver, slot);
	oldKey = key;
        qDebug() << key << GlobalShortcutManager::isConnected(key);
	return GlobalShortcutManager::isConnected(key);
    }
    

    QKeySequence getHotkey() const
    {
        return oldKey;
    }

    QString GetSettingsDirectory() { 
	return "";
    }

    
    QList<Directory> getDefaultCatalogDirectories();
    
    
    void AddToNotificationArea() {};
    void RemoveFromNotificationArea() {};
    
    bool isAlreadyRunning() const {
	return false;
    }


    virtual QHash<QString, QList<QString> > getDirectories();
	virtual QString expandEnvironmentVars(QString txt);

    bool supportsAlphaBorder() const;
    /*
    QIcon icon(const QFileInfo& info) {
    	shared_ptr<UnixIconProvider> u(dynamic_pointer_cast<UnixIconProvider>(icons));
  	  	return u->getIcon(info);
//		return ((UnixIconProvider*) icons.get())->getIcon(info); 
    }
    */

    virtual void alterItem(CatItem*);
    signals:
    void xkeyPressed(XEvent*);

};




