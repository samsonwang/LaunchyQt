/*
Launchy: Application Launcher
Copyright (C) 2009  Simon Capewell

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

#include "precompiled.h"
#include "CommandHistory.h"
#include "globals.h"


CommandHistory::CommandHistory()
{
}


bool CommandHistory::load(const QString& filename)
{
	QFile inFile(filename);
	if (!inFile.open(QIODevice::ReadOnly))
		return false;

	QByteArray ba = inFile.readAll();
	QDataStream in(&ba, QIODevice::ReadOnly);
	in.setVersion(LAUNCHY_VERSION);
	while (!in.atEnd())
	{
		InputDataList item;
		in >> item;
		history.push_back(item);
	}

	return true;
}


void CommandHistory::save(const QString& filename) const
{
	QByteArray ba;
	QDataStream out(&ba, QIODevice::ReadWrite); 
	out.setVersion(LAUNCHY_VERSION);

	foreach(InputDataList item, history)
	{
		out << item;
	}

	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly))
	{
		qWarning("Could not open history for writing");
		return;
	}
	file.write(ba);
}


// Add an item or promote an existing matching item to the top of the list
void CommandHistory::addItem(const InputDataList& item)
{
	if (item.count() == 0)
		return;

	// Look for a matching history entry
	QString itemText = item.toString().toLower();
	for (int i = 0; i < history.size(); ++i)
	{
		if (history[i].toString().toLower() == itemText)
		{
			// Found a match, remove it and add a replacement
			history.removeAt(i);
			break;
		}
	}

	InputDataList historyItem = item;
	historyItem.first().setLabel(LABEL_HISTORY);
	history.push_front(historyItem);

	if (history.size() > gSettings->value("GenOps/maxitemsinhistory", 20).toInt())
	{
		history.pop_back();
	}
}


InputDataList& CommandHistory::getItem(int index)
{
	return history[index];
}


void CommandHistory::removeAt(int index)
{
	return history.removeAt(index);
}


// Populate the searchresults with items from the command history
void CommandHistory::search(const QString& /*searchText*/, QList<CatItem>& searchResults) const
{
	int index = 0;
	foreach(InputDataList historyItem, history)
	{
		CatItem item = historyItem.first().getTopResult();
		item.id = HASH_HISTORY;
		item.data = (void*)index++;
		searchResults.push_back(item);
	}
}
