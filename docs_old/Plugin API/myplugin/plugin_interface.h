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

#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H


#include <QObject>
#include <QSettings>
#include "catalog.h"

#ifdef Q_WS_WIN
#define VC_EXTRALEAN
#ifndef WINVER
#define WINVER 0x05100
#define _WIN32_WINNT 0x0510	
#define _WIN32_WINDOWS 0x0510 
#define _WIN32_IE 0x0600
#endif

#include <windows.h>
#include <shlobj.h>
#endif

#ifndef UNICODE
#define UNICODE
#endif


#define DESKTOP_WINDOWS 0
#define DESKTOP_GNOME 1
#define DESKTOP_KDE 2

/**
	\brief This message asks the Plugin for its ID Number.

	 Launchy needs an unsigned int identification value for each loaded plugin.  You supply your own here. 
	Typically, this is the result of hashing a string, as shown in the example below.
	\param wParam (uint*) That stores the resulting ID number.
	\warning Because we're hashing strings to integers.. it is theoretically possible that two plugin names will collide to the same plugin id.  

	\verbatim
	int WebyPlugin::msg(int msgId, void* wParam, void* lParam)
	{
		bool handled = false;
		switch (msgId)
		{
			case MSG_GET_ID:
				*wParam = qHash(QString("Weby_Plugin"))
				handled = true;
				break;
		}
		
		return handled;
	}
	\endverbatim
*/
#define MSG_GET_ID 0

/**
	\brief This message asks the plugin if it would like to apply a label to the current search query.
	
	 It is sometimes useful to label user queries with plugin-defined tags.  For instance, the weby plugin
		will tag input that contains "www" or ".com" or ".net" with the hash value of the string "HASH_WEBSITE".  Then,
		other plugins that see the query can know that the current search is for a website.\n\n
		The InputData class stores the current user's query.  It is in a List structure because each time "tab" is pressed by the user
		a new InputData is formed and appended to the list.  In other words, if the user typed "google <tab> this is my query" then
		wParam would represent a list of 2 InputData classes, with the first representing "google", and the second, "this is my query". Each InputData
		can be tagged individually.

	\warning This is called each time the user changes a character in his or her query, so make sure it's fast.

	\param wParam QList<InputData>* id
	\param lParam NULL

	\verbatim
	void WebyPlugin::getLabels(QList<InputData>* id)
	{
		if (id->count() > 1)
			return;

		// Apply a "website" label if we think it's a website
		QString & text = id->last().getText();

		if (text.contains("http://", Qt::CaseInsensitive))
			id->last().setLabel(HASH_WEBSITE);
		else if (text.contains("https://", Qt::CaseInsensitive)) 
			id->last().setLabel(HASH_WEBSITE);
		else if (text.contains(".com", Qt::CaseInsensitive)) 
			id->last().setLabel(HASH_WEBSITE);
		else if (text.contains(".net", Qt::CaseInsensitive))
			id->last().setLabel(HASH_WEBSITE);
		else if (text.contains(".org", Qt::CaseInsensitive))
			id->last().setLabel(HASH_WEBSITE);
		else if (text.contains("www.", Qt::CaseInsensitive))
			id->last().setLabel(HASH_WEBSITE);
	}

	int WebyPlugin::msg(int msgId, void* wParam, void* lParam)
	{
		bool handled = false;
		switch (msgId)
		{
			case MSG_GET_LABELS:
				getLabels((QList<InputData>*) wParam);
				handled = true;
				break;
		}
		
		return handled;
	}
	\endverbatim
*/
#define MSG_GET_LABELS 1



/**
	\brief Asks the plugin for any results to a query

	 If your plugin returns catalog results on the fly to a query 
		(e.g. a website query for weby or a calculator result), then this is the place to do so.
		The existing results are stored in the list of CatItem's (short for Catalog Items) passed in as
		lParam and you can append your own results to it.

	\param wParam (QList<InputData>*): The user's query
	\param lParam (QList<CatItem>*): The list of existing results for the query, you can append your own

	\verbatim
	void WebyPlugin::getResults(QList<InputData>* id, QList<CatItem>* results)
	{
		// Is this query a website?  If so create a new result for the website
		if (id->last().hasLabel(HASH_WEBSITE)) {
			QString & text = id->last().getText();
			// This is a website, create an entry for it
			results->push_front(CatItem(text + ".weby", text, HASH_WEBY, getIcon()));
		}
		
		// Is this query of form:  website <tab> search_term?
		if (id->count() > 1 && id->first().getTopResult().id == HASH_WEBY) {
			QString & text = id->last().getText();
			// This is user search text, create an entry for it
			results->push_front(CatItem(text + ".weby", text, HASH_WEBY, getIcon()));
		}
	}

	int WebyPlugin::msg(int msgId, void* wParam, void* lParam)
	{
		bool handled = false;
		switch (msgId)
		{
			case MSG_GET_RESULTS:
				getResults((QList<InputData>*) wParam, (QList<CatItem>*) lParam);
				handled = true;
				break;
		}
		
		return handled;
	}
	\endverbatim
*/
#define MSG_GET_RESULTS 2

/**
	\brief Asks the plugin for a static catalog to be added to the primary catalog
	 Some plugins will add permanent entries to Launchy's primary catalog (until the catalog is rebuilt).  
	For instance, weby adds firefox bookmarks into the primary catalog.  This is the function in which that is done.
	\param wParam (QList<CatItem>*): The catalog that you append your new entries to (these will be copied over to the primary catalog)
	\verbatim
	void WebyPlugin::getCatalog(QList<CatItem>* items)
	{
		// Add the default websites to the catalog "google/amazon/etc.."
		foreach(WebySite site, sites) {
			items->push_back(CatItem(site.name + ".weby", site.name, HASH_WEBY, getIcon()));
		}

		// If we're supposed to index firefox, then do that there
		if ((*settings)->value("weby/firefox", true).toBool()) {
			QString path = getFirefoxPath();
			indexFirefox(path, items);
		}
	}

	int WebyPlugin::msg(int msgId, void* wParam, void* lParam)
	{
		bool handled = false;
		switch (msgId)
		{
			case MSG_GET_CATALOG:
				getCatalog((QList<CatItem>*) wParam);
				handled = true;
				break;
		}
		
		return handled;
	}
	\endverbatim
*/

#define MSG_GET_CATALOG 3


/**
	\brief This message instructs the plugin that one of its own catalog items was selected by the user and should now be executed

	 If the plugin adds items to the catalog via MSG_GET_RESULTS or MSG_GET_CATALOG and one of those items is selected by the user
		then it is up to the plugin to execute it when the user presses "enter".  This is where you perform the action.
	\param wParam (QList<InputData>*): The user's query
	\param lParam (CatItem*): The user selected catalog item

	\verbatim
	void WebyPlugin::launchItem(QList<InputData>* id, CatItem* item)
	{
		QString file = "";
		QString args = "";

		if (id->count() == 2) {
			args = id->last().getText();
			args = QUrl::toPercentEncoding(args);
			item = &id->first().getTopResult();
		}

		// Is it a Firefox shortcut?
		if (item->fullPath.contains(".shortcut")) {
			file = item->fullPath.mid(0, item->fullPath.count()-9);
			file.replace("%s", args);
		}
		else { // It's a user-specific site
			bool found = false;
			foreach(WebySite site, sites) {
				if (item->shortName == site.name) {
					found = true;
					file = site.base;
					if (args != "") {
						QString tmp = site.query;
						tmp.replace("%s", args);
						file += tmp;
					}
					break;
				}
			}

			if (!found) {
				file = item->shortName;	
				if (!file.contains("http://")) {
					file = "http://" + file;
				}
			}
		}
		QUrl url(file);
		runProgram(url.toEncoded(), "");
	}

	int WebyPlugin::msg(int msgId, void* wParam, void* lParam)
	{
		bool handled = false;
		switch (msgId)
		{
			case MSG_LAUNCH_ITEM:
				launchItem((QList<InputData>*) wParam, (CatItem*) lParam);
				handled = true;
				break;
		}
		
		return handled;
	}
	\endverbatim
*/
#define MSG_LAUNCH_ITEM 4


/**
	\brief This message informs the plugin that it's being loaded.  This is a good time to do any initialization work.

	\param wParam NULL
	\param lParam NULL

	\verbatim
	void calcyPlugin::init()
	{
	    // Do some initialization here if necessary..
	}

	int CalcyPlugin::msg(int msgId, void* wParam, void* lParam)
	{
		bool handled = false;
		switch (msgId)
		{
			case MSG_INIT:
				init();
				handled = true;
				break;
		}
		
		return handled;
	}
	\endverbatim
*/


#define MSG_INIT 5


/**
	\brief This message asks the plugin if it has a dialog to display in the options menu.

	 The result of the request is returned via the result of the msg function (handled).

	\param wParam NULL
	\param lParam NULL

	\verbatim
	int WebyPlugin::msg(int msgId, void* wParam, void* lParam)
	{
		bool handled = false;
		switch (msgId)
		{
		case MSG_HAS_DIALOG:
			handled = true;
			break;
		}
		
		return handled;
	}
	\endverbatim
*/
#define MSG_HAS_DIALOG 6

/**
	\brief This message tells the plugin that it's time to show its user interface

	\param wParam (QWidget*): The parent of the dialog to create
	\param lParam (QWidget**): Your plugin's new dialog is returned here for Launchy to keep tabs on it

	\verbatim
	void WebyPlugin::doDialog(QWidget* parent, QWidget** newDlg) {
		// gui is a private member variable of class WebyPlugin
		if (gui != NULL) return;

		// class Gui is weby's user interface class
		gui = new Gui(parent);
		*newDlg = gui;
	}

	int WebyPlugin::msg(int msgId, void* wParam, void* lParam)
	{
		bool handled = false;
		switch (msgId)
		{
			case MSG_DO_DIALOG:
				doDialog((QWidget*) wParam, (QWidget**) lParam);
				break;
		}
		
		return handled;
	}
	\endverbatim
*/

#define MSG_DO_DIALOG 7


/**
	\brief This message asks the plugin for its string name

	\param wParam (QString*) The destination for the name of your plugin
	\param lParam NULL

	\verbatim
	void WebyPlugin::getName(QString* str)
	{
		*str = "Weby";
	}

	int WebyPlugin::msg(int msgId, void* wParam, void* lParam)
	{
		bool handled = false;
		switch (msgId)
		{
			case MSG_GET_NAME:
				getName((QString*) wParam);
				handled = true;
				break;
		}
		
		return handled;
	}
	\endverbatim
*/
#define MSG_GET_NAME 8

/**
	\brief This message informs the plugin that it should close its dialog

	\param wParam (bool): Whether the plugin should accept changes made by the user while the dialog was open
	\param lParam NULL

	\verbatim
	void WebyPlugin::endDialog(bool accept) {
		if (accept) {
			gui->writeOptions();
			init();
		}
		if (gui != NULL) 
			delete gui;
		
		gui = NULL;
	}

	int WebyPlugin::msg(int msgId, void* wParam, void* lParam)
	{
		bool handled = false;
		switch (msgId)
		{
			case MSG_END_DIALOG:
				endDialog((bool) wParam);
				break;
		}
		
		return handled;
	}
	\endverbatim
*/
#define MSG_END_DIALOG 9


/**
	\brief This message informs the plugin that Launchy is now visible on the screen.

	\param wParam NULL
	\param lParam NULL
*/
#define MSG_LAUNCHY_SHOW 10

/**
	\brief This message informs the plugin that Launchy is no longer visible on the screen.

	\param wParam NULL
	\param lParam NULL
*/
#define MSG_LAUNCHY_HIDE 11



#define MSG_CONTROL_OPTIONS 500
#define MSG_CONTROL_EXIT 501
#define MSG_CONTROL_REBUILD 502

/**
	\brief This is the class that a plugin must implement.

	The PluginInterface is the interface between Launchy and your plugin.  
	Launchy communicates with your plugin through the msg function.  To store
	any settings in Launchy's permanent configuration file, please 
	refer to the settings member variable.
*/
class PluginInterface
{
public:
	virtual ~PluginInterface() {};


	/**
		\brief This is how messages are passed to the plugin

		Here you handle the subset of messages that your plugin requires.  At minimum, your plugin must
			handle MSG_GET_ID and MSG_GET_NAME.  For unhandled messages, return 0.  For handled messages, return 1.
		\param msgId The type of the message, such as show dialog, initialize, or get results 
		\param wParam If this message type requires any parameters, this will be the first and casted to void*
		\param lParam If this message requires two parameters, this is the second, casted to void*
		\return 1 means the message was handled, 0 means it was ignored

		\verbatim
		int WebyPlugin::msg(int msgId, void* wParam, void* lParam)
		{
			bool handled = false;
			switch (msgId)
			{		
				case MSG_INIT:
					init();
					handled = true;
					break;
				case MSG_GET_LABELS:
					getLabels((QList<InputData>*) wParam);
					handled = true;
					break;
				case MSG_GET_ID:
					getID((uint*) wParam);
					handled = true;
					break;
				case MSG_GET_NAME:
					getName((QString*) wParam);
					handled = true;
					break;
				case MSG_GET_RESULTS:
					getResults((QList<InputData>*) wParam, (QList<CatItem>*) lParam);
					handled = true;
					break;
				case MSG_GET_CATALOG:
					getCatalog((QList<CatItem>*) wParam);
					handled = true;
					break;
				case MSG_LAUNCH_ITEM:
					launchItem((QList<InputData>*) wParam, (CatItem*) lParam);
					handled = true;
					break;
				case MSG_HAS_DIALOG:
					handled = true;
					break;
				case MSG_DO_DIALOG:
					doDialog((QWidget*) wParam, (QWidget**) lParam);
					break;
				case MSG_END_DIALOG:
					endDialog((bool) wParam);
					break;

				default:
					break;
			}
				
			return handled;
		}
		\endverbatim

	*/
	virtual int msg(int msgId, void* wParam = NULL, void* lParam = NULL) = 0;


	/** The setting manager in which you store your persistent data.  More data can be found in QT's documentation for QSettings

	Following is an example of using the settings variable:

	\verbatim
	QSettings* set = *settings;

	// Write a value to the config file
	set->setValue("weby/version", 1.0);

	// Read a value from the config value
	double value = set->value("weby/version", 0.0);
	\endverbatim
	*/
	QSettings** settings;
};


/**
	\brief A convienience run function

	 This function will run the program along with arguments and is platform independent.
	\param file The location of the file to run
	\param args The arguments to the command
	*/
void runProgram(QString file, QString args);
int getDesktop();


 Q_DECLARE_INTERFACE(PluginInterface,
                     "net.launchy.PluginInterface/1.0")



#endif
