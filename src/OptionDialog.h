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

#pragma once

#include <QDialog>
#include "FileBrowserDelegate.h"
#include "Directory.h"
class QListWidgetItem;
class QVBoxLayout;
namespace Ui { class OptionDialog; }

namespace launchy {
class OptionDialog : public QDialog {
    Q_OBJECT
public:
    OptionDialog(QWidget* parent = nullptr);
    virtual ~OptionDialog();

public:
    virtual void setVisible(bool visible);

public slots:
    virtual void accept();
    virtual void reject();

protected:
    virtual void showEvent(QShowEvent* event);

private:
    // catalog
    void initCatalogWidget();
    void saveCatalogSettings();
    // plugins
    void initPluginsWidget();
    void savePluginsSettings();
    // update
    void initUpdateWidget();
    void saveUpdateSettings();
    // proxy
    void initProxyWidget();
    void saveProxySettings();
    // about
    void initAboutWidget();

    void addDirectory(const QString& directory, bool edit = false);
    void loadPluginDialog(QListWidgetItem* item);
    // void connectCatalogBuilderEvents();

private slots:
    void logLevelChanged(int index);
    void autoUpdateCheckChanged(int state);
    void tabChanged(int tab);
    // skins
    void skinChanged(const QString& newSkin);
    // catalog
    void dirRowChanged(int row);
    void catDirItemChanged(QListWidgetItem* item);
    void catDirDragEnter(QDragEnterEvent* event);
    void catDirDrop(QDropEvent *event);
    void catDirPlusClicked(bool c);
    void catDirMinusClicked(bool c);
    void catTypesItemChanged(QListWidgetItem* item);
    void catTypesPlusClicked(bool c);
    void catTypesMinusClicked(bool c);
    void catTypesDirChanged(int);
    void catTypesExeChanged(int);
    void catDepthChanged(int);
    void catalogProgressUpdated(int);
    void catalogBuilt();
    void catRescanClicked(bool);
    // plugins
    void pluginChanged(int row);
    void pluginItemChanged(QListWidgetItem* item);
    // proxy
    void onProxyTypeChanged(int index);
    void onProxyRequiresPasswordToggled(bool checked);

private:
    Ui::OptionDialog* m_pUi;

    FileBrowserDelegate m_directoryItemDelegate;
    int m_currentPlugin;
    bool m_needRescan;
    QStringList metaKeys;
    QStringList actionKeys;
    QList<int> iMetaKeys;
    QList<int> iActionKeys;
    QList<Directory> m_memDirs;

    //QList<QPair<QString, uint>> pluginNames;
    //QVBoxLayout* pluginLayout;
    //QString lastDir;

    //static QByteArray s_windowGeometry;
    //static int s_currentTab;
    //static int s_currentPlugin;
};
}
