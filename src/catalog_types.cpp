/*
Launchy: Application Launcher
Copyright (C) 2007-2010  Josh Karlin, Simon Capewell

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
#include "catalog_types.h"
#include "globals.h"


// Load the catalog from the specified filename
bool Catalog::load(const QString& filename)
{
	QFile inFile(filename);
	if (!inFile.open(QIODevice::ReadOnly))
	{
		qWarning("Could not open catalog file for reading");
		return false;
	}

	// Remove any existing catalog contents
	timestamp = 0;
	clear();

	QByteArray ba = inFile.readAll();
	QByteArray unzipped = qUncompress(ba);
	QDataStream in(&unzipped, QIODevice::ReadOnly);
	in.setVersion(QDataStream::Qt_4_2);

	while (!in.atEnd())
	{
		CatItem item;
		in >> item;
		addItem(item);
	}

	return true;
}


// Save the catalog to the specified filename
bool Catalog::save(const QString& filename)
{
	// Prevent other threads accessing the catalog
	QMutexLocker locker(&mutex);

	QByteArray ba;
	QDataStream out(&ba, QIODevice::ReadWrite); 
	out.setVersion(QDataStream::Qt_4_2);

	for (int i = 0; i < count(); i++)
	{
		CatItem item = getItem(i);
		out << item;
	}

	// Compress and write the catalog to the specified file
	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly))
	{
		qWarning("Could not open catalog file for writing");
		return false;
	}
	file.write(qCompress(ba));
	return true;
}


// Return true if the specified catalog item matches the specified string
bool Catalog::matches(CatItem* item, const QString& match)
{
	int matchLength = match.count();
	int curChar = 0;

	foreach(QChar c, item->lowName)
	{
		if (c == match[curChar])
		{
			++curChar;
			if (curChar >= matchLength)
			{
				return true;
			}
		} 
	}

	return false;
}


// Search the catalog, for items matching the text parameter and 
// populate the out parameter
void Catalog::searchCatalogs(const QString& text, QList<CatItem>& out)
{
	// Prevent other threads accessing the catalog
	QMutexLocker locker(&mutex);

	QList<CatItem*> catMatches = search(text);

	// Now prioritize the catalog items
	qSort(catMatches.begin(), catMatches.end(), CatLess);

	// Check for history matches
	QString location = "History/" + text;
	QStringList hist;
	hist = gSettings->value(location, hist).toStringList();
	if (hist.count() == 2)
	{
		for (int i = 0; i < catMatches.count(); i++)
		{
			if (catMatches[i]->lowName == hist[0] &&
				catMatches[i]->fullPath == hist[1])
			{
				CatItem* tmp = catMatches[i];
				catMatches.removeAt(i);
				catMatches.push_front(tmp);
			}
		}
	}

	// Load up the results
	int max = gSettings->value("GenOps/numresults", 10).toInt();
	for (int i = 0; i < max && i < catMatches.count(); i++)
	{
		out.push_back(*catMatches[i]);
	}
}


void Catalog::promoteRecentlyUsedItems(const QString& text, QList<CatItem> & list)
{
	// Check for history matches
	QString location = "History/" + text;
	QStringList hist;
	hist = gSettings->value(location, hist).toStringList();
	if (hist.count() == 2)
	{
		for (int i = 0; i < list.count(); i++)
		{
			if (list[i].lowName == hist[0] &&
				list[i].fullPath == hist[1])
			{
				CatItem tmp = list[i];
				list.removeAt(i);
				list.push_front(tmp);
			}
		}
	}
}


QString Catalog::decorateText(const QString& text, const QString& match, bool outputRichText)
{
	if (!gSettings->value("GenOps/decoratetext", false).toBool())
		return text;
	QString decoratedText;
	int matchLength = match.count();
	int curChar = 0;

	int index = text.toLower().indexOf(match);
	if (index > 0)
		decoratedText = text.left(index);
	else
		index = 0;
	bool highlighted = false;
	for (; index < text.count(); ++index)
	{
		QChar c = text[index];
		// prefix based rendering is buggy with lots of underlines limit it to 15
		// until we get round to replacing the list widget delegate with a rich text delegate
		if (curChar < matchLength && c.toLower() == match[curChar].toLower() &&
			(outputRichText || curChar < 15))
		{
			if (outputRichText)
			{
				if (!highlighted)
				{
					decoratedText += "<u>";
					highlighted = true;
				}
				decoratedText += c;
			}
			else
				decoratedText += QString("&") + c;
			++curChar;
		}
		else
		{
			if (outputRichText && highlighted)
			{
				decoratedText += "</u>";
				highlighted = false;
			}
			decoratedText += c;
		}
	}

	if (outputRichText && highlighted)
	{
		decoratedText += "</u>";
		highlighted = false;
	}

	return decoratedText;
}


void SlowCatalog::addItem(const CatItem& item)
{
	// Prevent other threads accessing the catalog
	QMutexLocker locker(&mutex);

	bool replaced = false;

	if (timestamp > 0)
	{
		// If we're not loading the catalog, search for an existing matching catalog item
		// and replace it if it exists
		for (int i = 0; i < catalogItems.size(); ++i)
		{
			if (item == catalogItems[i])
			{
				int usage = catalogItems[i].usage;
				catalogItems[i] = CatalogItem(item, timestamp);
				catalogItems[i].usage = usage;
				replaced = true;
				break;
			}
		}
	}

	if (!replaced)
	{
		// If no match found, append the item to the catalog
		qDebug() << "Adding" << item.fullPath;
		catalogItems.push_back(CatalogItem(item, timestamp));
	}
}


void SlowCatalog::purgeOldItems()
{
	// Prevent other threads accessing the catalog
	QMutexLocker locker(&mutex);

	for (int i = catalogItems.size() - 1; i >= 0; --i)
	{
		if (catalogItems.at(i).timestamp < timestamp)
		{
			qDebug() << "Removing" << catalogItems.at(i).fullPath;
			catalogItems.remove(i);
		}
	}
}


void SlowCatalog::incrementUsage(const CatItem& item)
{
	// Prevent other threads accessing the catalog
	QMutexLocker locker(&mutex);

	for (int i = 0; i < catalogItems.size(); ++i)
	{
		if (item == catalogItems[i])
		{
			// If an item is currently demoted, return it to a usage count of 1
			if (catalogItems[i].usage < 0)
				catalogItems[i].usage = 1;
			else
				++catalogItems[i].usage;
			break;
		}
	}
}


void SlowCatalog::demoteItem(const CatItem& item)
{
	// Prevent catalog refreshes whilst searching
	QMutexLocker locker(&mutex);

	for (int i = 0; i < catalogItems.size(); ++i)
	{
		if (item == catalogItems[i])
		{
			// If an item is not demoted, demote it
			if (catalogItems[i].usage > 0)
				catalogItems[i].usage = -1;
			else // otherwise demote it further
				--catalogItems[i].usage;
			break;
		}
	}
}


// Return a list of catalog items that match searchText
// this method should only be called from within a QMutexLocker protected section
QList<CatItem*> SlowCatalog::search(const QString& searchText)
{
	QList<CatItem*> result;

	if (searchText.count() > 0)
	{
		QString lowSearch = searchText.toLower();

		for (int i = 0; i < catalogItems.count(); ++i)
		{
			if (matches(&catalogItems[i], lowSearch))
			{
				result.push_back(&catalogItems[i]);
			}
		}
	}

	return result;
}
