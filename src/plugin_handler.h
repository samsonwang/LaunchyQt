/*
Launchy: Application Launcher
Copyright (C) 2007-2009  Josh Karlin

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

#ifndef PLUGIN_HANDLER
#define PLUGIN_HANDLER


#include <QPluginLoader>
#include <QHash>
#include "plugin_interface.h"
#include "catalog.h"


class Catalog;


// This structure is used by plugins such as PyLaunchy, so it must not be extended
// with virtual methods or additional data members
struct PluginInfo
{
	uint id;
	QString name;
	QString path;
	PluginInterface* obj;
	bool loaded;

	PluginInfo()
	{
		id = 0;
		obj = NULL;
		loaded = false;
	}
	~PluginInfo()
	{
		QPluginLoader loader(path);
		loader.unload();
	}

	bool isValid() const
	{
		return obj && !name.isNull() && id > 0;
	}

	int sendMessage(int msgId, void* wParam = NULL, void* lParam = NULL);
};


// This interface is used to notify clients when a step in a long running process occurs
class INotifyProgressStep
{
public:
	virtual bool progressStep(int newStep) = 0;
};


class PluginHandler
{
public:
	PluginHandler();
	~PluginHandler();

	void loadPlugins();
	void showLaunchy();
	void hideLaunchy();
	void getLabels(QList<InputData>* inputData);
	void getResults(QList<InputData>* inputData, QList<CatItem>* results);
	void getCatalogs(Catalog* catalog, INotifyProgressStep* progressStep);
	int execute(QList<InputData>*, CatItem*);
	QWidget* doDialog(QWidget* parent, uint id);
	void endDialog(uint id, bool accept);
	QHash<uint, PluginInfo> & getPlugins() { return plugins; }

private:
	QHash<uint, PluginInfo> plugins;
};


#endif
