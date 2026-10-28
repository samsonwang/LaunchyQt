
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

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>
#include <QDebug>

#include "GlobalVar.h"
#include "OptionItem.h"

namespace launchy {

// Json file layout:
// {
//   "version": 1,
//   "command": [
//     [ { "text": "...", "labels": ["..."], "top": { <CatItem> } },
//       { "text": "...", "labels": ["..."], "top": { <CatItem> } } ],
//     ...
//   ],
//   "promotion": [
//     { "query": "...", "shortName": "...", "fullPath": "..." },
//     ...   // front = most recently used, capped at PROMOTION_MAX_ITEMS
//   ]
// }
//
// Each command is an array of its segments in input order (InputDataList is a
// QList<InputData>, one array per command is the natural mapping).
// CatItem only stores the fields that cannot be recomputed: searchName and
// searchNameTrans are derived from shortName exactly like every CatItem
// constructor does. The promotion list is an array (not an object) because
// QJsonObject sorts its keys, which would destroy the LRU order.

static const int HISTORY_FILE_VERSION = 1;
static const int PROMOTION_MAX_ITEMS = 100;

CommandHistory* g_history = nullptr;

namespace {

QJsonObject inputDataToJson(const InputData& data) {
    QJsonObject segment;
    segment["text"] = data.getText();
    segment["top"] = catItemToJson(data.getTopResult());

    // Sort the labels so the file stays diff-friendly
    QStringList labels = data.getLabels().toList();
    labels.sort();
    QJsonArray labelArray;
    for (const QString& label : labels) {
        labelArray.append(label);
    }
    segment["labels"] = labelArray;

    return segment;
}

// Returns false when the segment is malformed; the whole command entry is
// then dropped (a partial entry could never be relaunched anyway).
bool inputDataFromJson(const QJsonObject& segment, InputData& data) {
    const QJsonValue textValue = segment["text"];
    const QJsonValue topValue = segment["top"];
    if (!textValue.isString() || !topValue.isObject()) {
        return false;
    }

    CatItem topResult;
    if (!catItemFromJson(topValue.toObject(), topResult)) {
        return false;
    }

    data = InputData(textValue.toString());
    data.setTopResult(topResult);

    const QJsonValue labelsValue = segment["labels"];
    if (labelsValue.isArray()) {
        const QJsonArray labels = labelsValue.toArray();
        for (int i = 0; i < labels.count(); ++i) {
            const QJsonValue label = labels.at(i);
            if (label.isString()) {
                data.setLabel(label.toString());
            }
        }
    }

    return true;
}

} // namespace

CommandHistory::CommandHistory() {
}

bool CommandHistory::load(const QString& filename) {
    m_filename = filename;
    m_command.clear();
    m_promotion.clear();

    QFile inFile(filename);
    if (!inFile.open(QIODevice::ReadOnly)) {
        // No file yet -> empty history. The legacy history.db (binary
        // QDataStream) is intentionally not read, there is no migration.
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(inFile.readAll(),
                                                      &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "CommandHistory::load, invalid history file:"
                   << filename << parseError.errorString();
        return false;
    }

    const QJsonObject root = doc.object();
    const int version = root["version"].toInt();
    if (version > HISTORY_FILE_VERSION) {
        qWarning() << "CommandHistory::load, history file version" << version
                   << "is newer than supported" << HISTORY_FILE_VERSION;
    }

    const QJsonArray commands = root["command"].toArray();
    int droppedCommands = 0;
    for (int i = 0; i < commands.count(); ++i) {
        const QJsonValue command = commands.at(i);
        if (!command.isArray()) {
            ++droppedCommands;
            continue;
        }

        const QJsonArray segments = command.toArray();
        InputDataList entry;
        bool valid = true;
        for (int j = 0; j < segments.count(); ++j) {
            const QJsonValue segment = segments.at(j);
            InputData data;
            if (!segment.isObject()
                || !inputDataFromJson(segment.toObject(), data)) {
                valid = false;
                break;
            }
            entry.push_back(data);
        }

        if (valid && !entry.isEmpty()) {
            m_command.push_back(entry);
        }
        else {
            ++droppedCommands;
        }
    }

    const QJsonArray promotion = root["promotion"].toArray();
    int droppedPromotions = 0;
    for (int i = 0; i < promotion.count(); ++i) {
        const QJsonValue entry = promotion.at(i);
        if (!entry.isObject()) {
            ++droppedPromotions;
            continue;
        }

        const QJsonObject obj = entry.toObject();
        const QJsonValue queryValue = obj["query"];
        const QJsonValue shortNameValue = obj["shortName"];
        const QJsonValue fullPathValue = obj["fullPath"];
        if (!queryValue.isString() || !shortNameValue.isString()
            || !fullPathValue.isString()) {
            ++droppedPromotions;
            continue;
        }

        PromotedItem item;
        item.query = queryValue.toString();
        item.shortName = shortNameValue.toString();
        item.fullPath = fullPathValue.toString();
        if (item.query.isEmpty() || item.shortName.isEmpty()
            || item.fullPath.isEmpty()) {
            ++droppedPromotions;
            continue;
        }

        m_promotion.push_back(item);
    }

    // Keep the most recently used entries only (the file order is the LRU
    // order), also caps hand-edited files that grew past the limit
    while (m_promotion.count() > PROMOTION_MAX_ITEMS) {
        m_promotion.removeLast();
        ++droppedPromotions;
    }

    if (droppedCommands > 0 || droppedPromotions > 0) {
        qWarning() << "CommandHistory::load, dropped malformed entries:"
                   << droppedCommands << "commands,"
                   << droppedPromotions << "promotions";
    }

    return true;
}

void CommandHistory::save(const QString& filename) const {
    QJsonObject root;
    root["version"] = HISTORY_FILE_VERSION;

    QJsonArray commands;
    for (const InputDataList& item : m_command) {
        QJsonArray segments;
        for (const InputData& data : item) {
            segments.append(inputDataToJson(data));
        }
        commands.append(segments);
    }
    root["command"] = commands;

    QJsonArray promotion;
    for (const PromotedItem& item : m_promotion) {
        QJsonObject entry;
        entry["query"] = item.query;
        entry["shortName"] = item.shortName;
        entry["fullPath"] = item.fullPath;
        promotion.append(entry);
    }
    root["promotion"] = promotion;

    // QSaveFile commits atomically, a crash mid-write cannot corrupt the file
    QSaveFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "CommandHistory::save, could not open history file"
                   << filename;
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        qWarning() << "CommandHistory::save, could not commit history file"
                   << filename;
    }
}

void CommandHistory::persist() const {
    if (!m_filename.isEmpty()) {
        save(m_filename);
    }
}

// Add an item or promote an existing matching item to the top of the list
void CommandHistory::addItem(const InputDataList& item) {
    if (item.isEmpty()) {
        return;
    }

    int historyMax = g_settings->value(OPTION_MAXITEMSINHISTORY,
                                       OPTION_MAXITEMSINHISTORY_DEFAULT).toInt();
    if (historyMax <= 0) {
        if (!m_command.isEmpty()) {
            m_command.clear();
            persist();
        }
        return;
    }

    // Look for a matching history entry
    QString itemText = item.toString().toLower();
    for (auto it = m_command.begin(); it != m_command.end(); ++it) {
        if (it->toString().toLower() == itemText) {
            // Found a match, remove it and add a replacement
            m_command.erase(it);
            break;
        }
    }

    //InputDataList historyItem = item;
    //historyItem.first().setLabel(LABEL_HISTORY);
    m_command.push_front(item);
    m_command.front().front().setLabel(LABEL_HISTORY);

    while (m_command.size() > historyMax) {
        m_command.pop_back();
    }

    persist();
}

int CommandHistory::getItemCount() const {
    return m_command.size();
}

// An empty list signals "no such entry" to the caller instead of reading past the
// end of the history.
InputDataList CommandHistory::getItem(int index) const {
    if (index < 0 || index >= m_command.size()) {
        qWarning() << "CommandHistory::getItem, index out of range:" << index
                   << ", history size:" << m_command.size();
        return InputDataList();
    }

    return m_command.at(index);
}

void CommandHistory::removeAt(int index) {
    if (index >= 0 && index < m_command.size()) {
        m_command.erase(m_command.begin() + index);
        persist();
    }
}

void CommandHistory::getAllItem(QList<CatItem>& searchResults) const {
    long long index = 0; // long long in x86 is 4 bytes, and in x64 is 8 bytes
                         // use this to avoid VS C4312 warning
    for (const InputDataList& historyItem: m_command) {
        if (historyItem.isEmpty()) {
            continue;
        }
        CatItem item = historyItem.first().getTopResult();
        item.pluginName = NAME_HISTORY;
        item.data = (void*)index++; // Need this when switching alternative list
        searchResults.push_back(item);
    }
}

// Populate the search results with items from the command history
void CommandHistory::search(const QString& text, QList<CatItem>& searchResults) const {
    if (text.isEmpty()) {
        return;
    }

    long long index = -1;
    for (const InputDataList& historyCmd : m_command) {
        ++index;
        // ignore history commands which have only one input segment
        if (historyCmd.empty() || historyCmd.count() == 1) {
            continue;
        }
        // each InputDataList is a whole input and invoke action
        for (const InputData& data : historyCmd) {
            if (data.getText().contains(text, Qt::CaseInsensitive)) {
                // history matched
                CatItem item = historyCmd.first().getTopResult();
                item.pluginName = NAME_HISTORY;
                item.data = (void*)index;
                item.shortName = historyCmd.toString();

                // search for duplicates
                bool duplicated = false;
                for (const CatItem& retItem : searchResults) {
                    if (retItem.shortName == item.shortName
                        && retItem.fullPath == item.fullPath) {
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

bool CommandHistory::promotedItem(const QString& query,
                                  QString& shortName, QString& fullPath) const {
    for (const PromotedItem& item : m_promotion) {
        if (item.query == query) {
            shortName = item.shortName;
            fullPath = item.fullPath;
            return true;
        }
    }
    return false;
}

void CommandHistory::setPromotedItem(const QString& query,
                                     const QString& shortName,
                                     const QString& fullPath) {
    if (query.isEmpty() || shortName.isEmpty() || fullPath.isEmpty()) {
        return;
    }

    // Refresh an existing entry in place (exact match, the old ini storage
    // was case sensitive too)
    for (int i = 0; i < m_promotion.count(); ++i) {
        if (m_promotion[i].query == query) {
            m_promotion.removeAt(i);
            break;
        }
    }

    PromotedItem item;
    item.query = query;
    item.shortName = shortName;
    item.fullPath = fullPath;
    m_promotion.push_front(item);

    while (m_promotion.count() > PROMOTION_MAX_ITEMS) {
        m_promotion.removeLast();
    }

    persist();
}

} // namespace launchy
