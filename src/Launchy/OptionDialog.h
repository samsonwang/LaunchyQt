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

#include "Directory.h"

class QListWidgetItem;

namespace Ui { class OptionDialog; }

namespace launchy {

class FileBrowserDelegate;

class OptionDialog : public QDialog {
    Q_OBJECT
public:
    OptionDialog(QWidget* parent = nullptr);
    virtual ~OptionDialog();

public:
    void retranslateUi();

public slots:
    virtual void accept();
    virtual void reject();

protected:
    virtual void showEvent(QShowEvent* event);
    virtual void changeEvent(QEvent* event);

private:
    // general
    void initGeneralWidget();
    bool saveGeneralSettings();
    // skin
    void initSkinWidget();
    void saveSkinSettings();
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
    // system
    void initSystemWidget();
    void saveSystemSettings();
    // about
    void initAboutWidget();

    void addDirectory(const QString& directory, bool edit = false);
    void loadPluginDialog(QListWidgetItem* item);
    // void connectCatalogBuilderEvents();

private slots:
    void tabChanged(int tab);
    // general
    void onAppStyleChanged(int index);
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
    // system
    void autoRebuildCheckChanged(int state);
    void logLevelChanged(int index);
    void languageChanged(int index);

private:
    Ui::OptionDialog* m_pUi;
    FileBrowserDelegate* m_directoryItemDelegate;

    QStringList m_metaKeys;
    QStringList m_actionKeys;
    QList<Qt::KeyboardModifiers> m_iMetaKeys;
    QList<Qt::Key> m_iActionKeys;
    QList<Directory> m_memDirs;

    static QByteArray s_lastWindowGeometry;
    static int s_lastTab;
    static int s_lastPlugin;
};

} // namespace launchy

