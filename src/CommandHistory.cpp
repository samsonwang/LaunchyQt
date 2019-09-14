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

#include "CommandHistory.h"
#include "GlobalVar.h"
#include "OptionItem.h"

namespace launchy {
CommandHistory::CommandHistory() {
}

bool CommandHistory::load(const QString& filename) {
    QFile inFile(filename);
    if (!inFile.open(QIODevice::ReadOnly))
        return false;

    QByteArray ba = inFile.readAll();
    QDataStream in(&ba, QIODevice::ReadOnly);
    in.setVersion(LAUNCHY_VERSION);
    while (!in.atEnd()) {
        InputDataList item;
        in >> item;
        m_history.push_back(item);
    }

    return true;
}

void CommandHistory::save(const QString& filename) const {
    QByteArray ba;
    QDataStream out(&ba, QIODevice::ReadWrite);
    out.setVersion(LAUNCHY_VERSION);

    foreach(InputDataList item, m_history) {
        out << item;
    }

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("Could not open history for writing");
        return;
    }
    file.write(ba);
}

// Add an item or promote an existing matching item to the top of the list
void CommandHistory::addItem(const InputDataList& item) {
    if (item.isEmpty()) {
        return;
    }

    int historyMax = g_settings->value(OPSTION_MAXITEMSINHISTORY,
                                       OPSTION_MAXITEMSINHISTORY_DEFAULT).toInt();
    if (historyMax <= 0) {
        m_history.clear();
        return;
    }

    // Look for a matching history entry
    QString itemText = item.toString().toLower();
    for (auto it = m_history.begin(); it != m_history.end(); ++it) {
        if (it->toString().toLower() == itemText) {
            // Found a match, remove it and add a replacement
            m_history.erase(it);
            break;
        }
    }

    //InputDataList historyItem = item;
    //historyItem.first().setLabel(LABEL_HISTORY);
    m_history.push_front(item);
    m_history.front().front().setLabel(LABEL_HISTORY);

    while (m_history.size() > historyMax) {
        m_history.pop_back();
    }
}

const InputDataList& CommandHistory::getItem(int index) {
    Q_ASSERT(index >= 0 && index < m_history.size());
    return *(m_history.begin() + index);
}

void CommandHistory::removeAt(int index) {
    if (index >= 0 && index < m_history.size()) {
        m_history.erase(m_history.begin() + index);
    }
}

void CommandHistory::getAllItem(QList<CatItem>& searchResults) const {
    int index = 0;
    for(InputDataList historyItem: m_history) {
        if(historyItem.isEmpty())
            continue;
        CatItem& item = historyItem.first().getTopResult();
        item.pluginId = HASH_HISTORY;
        item.data = (void*)index++; // use this when switching alternative list
        searchResults.push_back(item);
    }
}

// Populate the search results with items from the command history
void CommandHistory::search(const QString& text, QList<CatItem>& searchResults) const {
    if (text.isEmpty()) {
        return;
    }

    int64_t index = -1;
    foreach (InputDataList historyCmd, m_history) {
        ++index;
        // ignore history commands which have only one input segment
        if (historyCmd.count() == 1) {
            continue;
        }
        // each InputDataList is a whole input and invoke action
        foreach (InputData data, historyCmd) {
            if (data.getText().contains(text, Qt::CaseInsensitive)) {
                // history matched
                CatItem item = historyCmd.first().getTopResult();
                item.pluginId = HASH_HISTORY;
                item.data = (void*)index;
                item.shortName = historyCmd.toString();

                // search for duplicates
                bool duplicated = false;
                foreach (CatItem retItem, searchResults) {
                    if (retItem.shortName == item.shortName && retItem.fullPath == item.fullPath) {
                        duplicated = true;
                        break;
                    }
                }

                if (!duplicated) {
                    qDebug() << "CommandHistory::search, matched item:" << item.shortName
                        << item.fullPath << item.data;
                    searchResults.push_back(item);
                }
            }
        }
    }

}
}
