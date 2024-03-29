/*
Launchy: Application Launcher
Copyright (C) 2009-2010  Simon Capewell, Josh Karlin

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

#include "FileSearch.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include <QDir>
#include <QRegularExpression>
#include <QDebug>

#include "AppBase.h"
#include "GlobalVar.h"
#include "Catalog.h"
#include "OptionItem.h"

namespace launchy {

void FileSearch::search(const QString& searchText,
                        QList<CatItem>& searchResults,
                        InputDataList& inputData) {
    qDebug() << "Searching file system for" << searchText;

    QString searchPath = QDir::fromNativeSeparators(searchText);
    g_searchText = searchPath;

    if (searchPath.startsWith("~"))
        searchPath.replace("~", QDir::homePath());

#ifdef Q_OS_WIN
    if (searchPath == "/") {
        // Special case for Windows: list available drives
        QFileInfoList driveList = QDir::drives();
        for (int i = driveList.length()-1; i >= 0; --i) {
            QFileInfo info = driveList[i];
            // Retrieve volume name
            QString volumeName;
            WCHAR volName[MAX_PATH];
            if (GetVolumeInformationW((WCHAR*)info.filePath().utf16(), volName,
                                     MAX_PATH, NULL, NULL, NULL, NULL, 0)) {
                volumeName = QString::fromWCharArray(volName);
            }
            else {
                volumeName = QDir::toNativeSeparators(info.filePath());
            }
            CatItem item(QDir::toNativeSeparators(info.filePath()), volumeName);
            item.pluginName = NAME_LAUNCHYFILE;
            searchResults.push_front(item);
        }
        return;
    }
    if (searchPath.size() == 2 && searchText[0].isLetter() && searchPath[1] == ':')
        searchPath += "/";
#endif

    // Split the string on the last slash
    QString directoryPart = searchPath.section("/", 0, -2);
    if (directoryPart == "")
        directoryPart = "/";
    QString filePart = searchPath.section("/", -1);
    bool isDirectory = filePart.length() == 0;
    bool sort = true;
    bool listPopulated = false;
    QStringList itemList;
    QDir dir(directoryPart);

#ifdef Q_OS_WIN
    // This is a windows network search
    if (searchPath.startsWith("//")) {
        // Exit if the user doesn't want to browse networks
        if (!g_settings->value(OPTION_SHOWNETWORK, OPTION_SHOWNETWORK_DEFAULT).toBool())
            return;

        // Check for a search against just the network name
        QRegularExpression re(QStringLiteral("//([a-z0-9\\-]+)?$"),
                              QRegularExpression::CaseInsensitiveOption);
        if (re.match(searchPath).hasMatch()) {
            // Get a list of devices on the network. This will be filtered and sorted later.
            g_app->getComputers(itemList);
            isDirectory = false;
            listPopulated = true;
            sort = false;
        }
    }
#endif
    if (!listPopulated) {
        // Exit if the path doesn't exist
        if (!dir.exists())
            return;

        // We have a directory, get a list of files and directories within the directory
        QDir::Filters filters = QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot;
#ifndef Q_OS_WIN
        filters |= QDir::CaseSensitive;
#else
        filePart = filePart.toLower();
#endif

        if (g_settings->value(OPTION_SHOWHIDDENFILES, OPTION_SHOWHIDDENFILES_DEFAULT).toBool()) {
            filters |= QDir::Hidden;
        }

        itemList = dir.entryList(filters, QDir::DirsLast | QDir::IgnoreCase | QDir::LocaleAware);
    }

    for (int i = itemList.length()-1; i >= 0; --i) {
        QString fileName = itemList[i];
        QString filePath = QDir::cleanPath(dir.absolutePath() + "/" + fileName);
        CatItem item(QDir::toNativeSeparators(filePath), fileName);
        if (filePart.length() == 0 || Catalog::matches(&item, filePart.toLower())) {
            item.pluginName = NAME_LAUNCHYFILE;
            searchResults.push_front(item);
        }
    }

    // Set the sort and underline global to just the filename
    g_searchText = filePart;

    if (isDirectory) {
        // We're showing a directory, add it as the top result
        if (!directoryPart.endsWith("/"))
            directoryPart += "/";
        QString fullPath = QDir::toNativeSeparators(directoryPart);
        QString name = dir.dirName();
        CatItem item(fullPath, name.length() == 0 ? fullPath : name);
        item.pluginName = NAME_LAUNCHYFILE;
        searchResults.push_front(item);
    }
    else if (sort) {
        // If we're not matching exactly and there's a filename then do a priority sort
        std::sort(searchResults.begin(), searchResults.end(), CatLessRef);
    }

    inputData.last().setLabel(LABEL_FILE);
}
}
