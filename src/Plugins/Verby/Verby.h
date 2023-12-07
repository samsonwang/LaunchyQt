/*
Verby: Plugin for Launchy
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

#pragma once

#include <QList>

#include "LaunchyLib/PluginInterface.h"
#include "LaunchyLib/CatalogItem.h"
#include "LaunchyLib/InputData.h"

class Gui;

class Verby : public QObject, public launchy::PluginInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID PLUGININTERFACE_IID)
    Q_INTERFACES(launchy::PluginInterface)
public:
    Verby();
    virtual ~Verby();

    virtual int msg(int msgId, void* wParam, void* lParam);

private:
    void getName(QString* name);
    void init();
    void setPath(const QString* path);
    void getLabels(QList<launchy::InputData>* inputData);
    void getResults(QList<launchy::InputData>* inputData, QList<launchy::CatItem>* results);
    int launchItem(QList<launchy::InputData>* inputData, launchy::CatItem* item);
    void doDialog(QWidget* parent, QWidget** dialog);
    void endDialog(bool accept);
    bool isMatch(const QString& text1, const QString& text2);
    void addCatItem(QString text, QList<launchy::CatItem>* results, QString fullName, QString shortName, QString iconName);
    void updateUsage(launchy::CatItem& item);
    QString getIcon();
    const QString& getIconPath() const;

    //void getCatalog(QList<CatItem>* items);
private:
    QString m_pluginName;
    QString m_libPath;
    Gui* m_gui;

    const QString LABEL_FILE;
    const QString LABEL_DIR;
    const QString LABEL_LINK;
    const QString LABEL_EXEC;
};

