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

#include "Precompiled.h"
#include "Catalog.h"
#include "GlobalVar.h"
#include "OptionItem.h"

namespace launchy {

Catalog::Catalog()
    : m_timestamp(0) {

}

Catalog::~Catalog() {

}

// Load the catalog from the specified filename
bool Catalog::load(const QString& filename) {
    QFile inFile(filename);
    if (!inFile.open(QIODevice::ReadOnly)) {
        qWarning("Catalog::load, Could not open catalog file for reading");
        return false;
    }

    // Remove any existing catalog contents
    m_timestamp = 0;
    clear();

    QByteArray ba = inFile.readAll();
    QByteArray unzipped = qUncompress(ba);
    QDataStream in(&unzipped, QIODevice::ReadOnly);
    in.setVersion(QDataStream::Qt_4_2);

    while (!in.atEnd()) {
        CatItem item;
        in >> item;
        addItem(item);
    }

    return true;
}


// Save the catalog to the specified filename
bool Catalog::save(const QString& filename) {
    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    QByteArray ba;
    QDataStream out(&ba, QIODevice::ReadWrite);
    out.setVersion(QDataStream::Qt_4_2);

    for (int i = 0; i < count(); i++) {
        CatItem item = getItem(i);
        out << item;
    }

    // Compress and write the catalog to the specified file
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("Catalog::save, Could not open catalog file for writing");
        return false;
    }
    file.write(qCompress(ba));
    return true;
}


void Catalog::incrementTimestamp() {
    ++m_timestamp;
}

// Return true if the specified catalog item matches the specified string
bool Catalog::matches(CatItem* item, const QString& match) {
    int matchLength = match.count();
    int curChar = 0;

    foreach(QChar c, item->searchName[CatItem::LOWER]) {
        if (c == match[curChar]) {
            ++curChar;
            if (curChar >= matchLength) {
                return true;
            }
        }
    }

    foreach(QChar c, item->searchName[CatItem::TRANS]) {
        if (c == match[curChar]) {
            ++curChar;
            if (curChar >= matchLength) {
                return true;
            }
        }
    }

    return false;
}


// Search the catalog, for items matching the text parameter and
// populate the out parameter
void Catalog::searchCatalogs(const QString& text, QList<CatItem>& result) {
    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    QList<CatItem*> catMatches = search(text);
    qDebug() << "Catalog::searchCatalogs, search matched count:" << catMatches.count();
    // Now prioritize the catalog items
    qSort(catMatches.begin(), catMatches.end(), CatLessPtr);

    // Check for history matches, and put them in the front
    QString location = "History/" + text;
    QStringList hist = g_settings->value(location).toStringList();
    if (hist.count() == 2) {
        for (int i = 0; i < catMatches.count(); ++i) {
            if (catMatches[i]->shortName == hist[0] && catMatches[i]->fullPath == hist[1]) {
                CatItem* tmp = catMatches[i];
                catMatches.removeAt(i);
                catMatches.push_front(tmp);
            }
        }
    }

    // Load up the results
    int max = g_settings->value(OPSTION_NUMRESULT, OPSTION_NUMRESULT_DEFAULT).toInt();
    for (int i = 0; i < max && i < catMatches.count(); i++) {
        result.push_back(*catMatches[i]);
    }
}

void Catalog::promoteRecentlyUsedItems(const QString& text, QList<CatItem>& list) {
    // Check for history matches
    QString location = "History/" + text;
    QStringList hist = g_settings->value(location).toStringList();
    qDebug() << "Catalog::promoteRecentlyUsedItems, text:"
        << text << "hist" << hist << "hist count:" << hist.count();

    if (hist.count() != 2) {
        return;
    }

    for (int i = 0; i < list.count(); i++) {
        if (list[i].shortName == hist[0] && list[i].fullPath == hist[1]) {
            CatItem tmp = list[i];
            qDebug() << "Catalog::promoteRecentlyUsedItems, promoted:" << tmp.fullPath;
            list.removeAt(i);
            list.push_front(tmp);
            break;
        }
    }
}

QString Catalog::decorateText(const QString& text, const QString& match, bool outputRichText) {
    if (!g_settings->value(OPSTION_DECORATETEXT, OPSTION_DECORATETEXT_DEFAULT).toBool())
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
    for (; index < text.count(); ++index) {
        QChar c = text[index];
        // prefix based rendering is buggy with lots of underlines limit it to 15
        // until we get round to replacing the list widget delegate with a rich text delegate
        if (curChar < matchLength && c.toLower() == match[curChar].toLower()
            && (outputRichText || curChar < 15)) {
            if (outputRichText) {
                if (!highlighted) {
                    decoratedText += "<u>";
                    highlighted = true;
                }
                decoratedText += c;
            }
            else
                decoratedText += QString("&") + c;
            ++curChar;
        }
        else {
            if (outputRichText && highlighted) {
                decoratedText += "</u>";
                highlighted = false;
            }
            decoratedText += c;
        }
    }

    if (outputRichText && highlighted) {
        decoratedText += "</u>";
        highlighted = false;
    }

    return decoratedText;
}


SlowCatalog::SlowCatalog()
    : Catalog() {

}

int SlowCatalog::count() {
    return m_catalogItems.count();
}


void SlowCatalog::clear() {
    m_catalogItems.clear();
}

void SlowCatalog::addItem(const CatItem& item) {
    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    bool replaced = false;

    if (m_timestamp > 0) {
        // If we're not loading the catalog, search for an existing matching catalog item
        // and replace it if it exists
        for (int i = 0; i < m_catalogItems.size(); ++i) {
            if (item == m_catalogItems[i]) {
                int usage = m_catalogItems[i].usage;
                m_catalogItems[i] = CatalogItem(item, m_timestamp);
                m_catalogItems[i].usage = usage;
                replaced = true;
                break;
            }
        }
    }

    if (!replaced) {
        // If no match found, append the item to the catalog
        // qDebug() << "SlowCatalog::addItem, Adding" << item.fullPath;
        m_catalogItems.push_back(CatalogItem(item, m_timestamp));
    }
}


void SlowCatalog::purgeOldItems() {
    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    for (int i = m_catalogItems.size() - 1; i >= 0; --i) {
        if (m_catalogItems.at(i).m_timestamp < m_timestamp) {
            qDebug() << "SlowCatalog::purgeOldItems, Removing" << m_catalogItems.at(i).fullPath;
            m_catalogItems.remove(i);
        }
    }
}


void SlowCatalog::incrementUsage(const CatItem& item) {
    // Prevent other threads accessing the catalog
    QMutexLocker locker(&m_mutex);

    for (int i = 0; i < m_catalogItems.size(); ++i) {
        if (item == m_catalogItems[i]) {
            // If an item is currently demoted, return it to a usage count of 1
            if (m_catalogItems[i].usage < 0) {
                m_catalogItems[i].usage = 1;
            }
            else {
                ++m_catalogItems[i].usage;
            }
            break;
        }
    }
}


void SlowCatalog::demoteItem(const CatItem& item) {
    // Prevent catalog refreshes whilst searching
    QMutexLocker locker(&m_mutex);

    for (int i = 0; i < m_catalogItems.size(); ++i) {
        if (item == m_catalogItems[i]) {
            // If an item is not demoted, demote it
            if (m_catalogItems[i].usage > 0) {
                m_catalogItems[i].usage = -1;
            }
            else { // otherwise demote it further
                --m_catalogItems[i].usage;
            }
            break;
        }
    }
}


const CatItem& SlowCatalog::getItem(int i) {
    return m_catalogItems[i];
}

// Return a list of catalog items that match searchText
// this method should only be called from within a QMutexLocker protected section
QList<CatItem*> SlowCatalog::search(const QString& searchText) {
    QList<CatItem*> result;
    if (!searchText.isEmpty()) {
        QString lowSearch = searchText.toLower();
        for (int i = 0; i < m_catalogItems.count(); ++i) {
            if (matches(&m_catalogItems[i], lowSearch)) {
                result.push_back(&m_catalogItems[i]);
            }
        }
    }

    return result;
}

bool CatLessRef(CatItem& a, CatItem& b) {
    bool less = CatLessPtr(&a, &b);
    /*	if (less)
    qDebug() << a.lowName << "(" << a.usage << ") < " << b.lowName << " (" << b.usage << ")";
    else
    qDebug() << b.lowName << "(" << b.usage << ") < " << a.lowName << " (" << a.usage << ")";
    */
    return less;
}

bool CatLessPtr(CatItem* a, CatItem* b) {
    // Items with negative usage are lowest priority
    if (a->usage < 0 && b->usage >= 0)
        return false;
    if (b->usage < 0 && a->usage >= 0)
        return true;

    bool localEqual = (a->searchName[CatItem::LOWER] == g_searchText
        || a->searchName[CatItem::TRANS] == g_searchText);

    bool otherEqual = (b->searchName[CatItem::LOWER] == g_searchText
        || b->searchName[CatItem::TRANS] == g_searchText);

    // Exact match between search text and item name has higest priority
    if (localEqual && !otherEqual)
        return true;
    if (!localEqual && otherEqual)
        return false;

    int localFind = std::min(a->searchName[CatItem::LOWER].indexOf(g_searchText),
                             a->searchName[CatItem::TRANS].indexOf(g_searchText));
    int otherFind = std::min(b->searchName[CatItem::LOWER].indexOf(g_searchText),
                             b->searchName[CatItem::TRANS].indexOf(g_searchText));

    if (g_searchText.count() == 1) {
        // Match at the start
        if (localFind == 0 && otherFind != 0)
            return true;
        else if (localFind != 0 && otherFind == 0)
            return false;

        // Higher usage
        if (a->usage > b->usage)
            return true;
        if (a->usage < b->usage)
            return false;
    }

    // Contiguous text anywhere in the item name
    if (localFind != -1 && otherFind == -1)
        return true;
    else if (localFind == -1 && otherFind != -1)
        return false;

    if (localFind != -1 && otherFind != -1) {
        // Both have word matches
        // Higher usage
        if (a->usage > b->usage)
            return true;
        if (a->usage < b->usage)
            return false;

        // Contiguous text nearer the start of the item name
        if (localFind < otherFind)
            return true;
        else if (otherFind < localFind)
            return false;
    }
    else {
        // Higher usage
        if (a->usage > b->usage)
            return true;
        if (a->usage < b->usage)
            return false;
    }

    int localLen = a->shortName.count();
    int otherLen = b->shortName.count();

    // Favour shorter item names
    if (localLen < otherLen)
        return true;
    if (localLen > otherLen)
        return false;

    // Absolute tiebreaker to prevent loops
    return a->fullPath < b->fullPath;
}

CatalogItem::CatalogItem()
    : m_timestamp(0) {

}

CatalogItem::CatalogItem(const CatItem& item, int time)
    : CatItem(item),
      m_timestamp(time) {

}

}
