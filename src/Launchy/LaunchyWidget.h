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

#include <QWidget>
#include <QSystemTrayIcon>
#include <QLabel>

#include "LaunchyLib/CatalogItem.h"
#include "LaunchyLib/InputData.h"

#include "IconExtractor.h"
#include "CommandHistory.h"

class QPushButton;
class QHotkey;

namespace launchy {
class Fader;
class AnimationLabel;
class IconDelegate;
class CharListWidget;
class CharLineEdit;
class OptionDialog;

enum CommandFlag {
    Default         = 0,
    ShowLaunchy     = 1,
    ShowOptions     = 2,
    ResetPosition   = 4,
    ResetSkin       = 8,
    Rescan          = 16,
    Exit            = 32,
    Restart         = 64
};

Q_DECLARE_FLAGS(CommandFlags, CommandFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(CommandFlags)

class LaunchyWidget : public QWidget {
    Q_OBJECT
public:
    static LaunchyWidget* instance();
    static void cleanup();

public:
    void executeStartupCommand(int command);
    void setAlternativeListMode(int mode);
    bool setHotkey(const QKeySequence& hotkey);
    bool setAlwaysShow(bool alwaysShow);
    bool setAlwaysTop(bool alwaysTop);
    void setSkin(const QString& name);
    int getHotkey() const;
    void startRebuildTimer();

    void showTrayIcon();
    void hideTrayIcon();
    void trayNotify(const QString& infoMsg);

public slots:
    void showLaunchy(bool noFade = false);
    void buildCatalog();
    void setOpaqueness(int level);

protected:
    virtual void showEvent(QShowEvent* event);
    virtual void paintEvent(QPaintEvent* event);
    virtual void closeEvent(QCloseEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void changeEvent(QEvent* event);

protected:
    // API for platform specific handler function
    virtual void focusLaunchy();

protected:
    // singleton
    LaunchyWidget(CommandFlags command);
    Q_DISABLE_COPY(LaunchyWidget)
    virtual ~LaunchyWidget();
    friend void createLaunchyWidget(CommandFlags command);

protected:
    void saveSettings();
    void createActions();
    void applySkin(const QString& name);
    void updateVersion(int oldVersion);
    void hideLaunchy(bool noFade = false);
    void showAlternativeList();
    void hideAlternativeList();
    void updateAlternativeList(bool resetSelection = true);
    void updateOutput(bool resetAlternativesSelection = true);
    void updateOutputItem(const CatItem& item);
    void updateOutputSize();
    void loadPosition(const QPoint& pt);
    void savePosition();
    void doTab();
    void doBackTab();
    void doEnter();
    void processInput();
    void searchOnInput();
    void launchItem();
    void startDropTimer();
    void retranslateUi();

protected slots:
    void showOptionDialog();
    void onHotkey();
    void dropTimeout();
    void catalogProgressUpdated(int);
    void catalogBuilt();
    void setFadeLevel(double level);
    void iconExtracted(const QString& plugin, const QString& path, const QIcon& icon);
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void reloadSkin();
    void exit();
    void onAlternativeListRowChanged(int index);
    void onAlternativeListKeyPressed(QKeyEvent* event);
    void onAlternativeListFocusOut();
    void onInputBoxKeyPressed(QKeyEvent* event);
    void onInputBoxFocusOut();
    void onInputBoxInputMethod(QInputMethodEvent* event);
    void onInputBoxTextEdited(const QString& str);
    void onSecondInstance();
    void onScreenChanged(QScreen* screen);

protected:
    QString m_currentSkin;
    bool m_skinChanged;

    CharLineEdit* m_inputBox;
    QLabel* m_outputBox;
    QLabel* m_outputIcon;
    CharListWidget* m_alternativeList;
    QPushButton* m_optionButton;
    QPushButton* m_closeButton;
    AnimationLabel* m_workingAnimation;
    QSystemTrayIcon* m_trayIcon;
    Fader* m_fader;
    QPixmap m_frameGraphic;

    QHotkey* m_pHotKey;

    QAction* m_actShow;
    QAction* m_actReloadSkin;
    QAction* m_actRebuild;
    QAction* m_actOptions;
    QAction* m_actCheckUpdate;
    QAction* m_actRestart;
    QAction* m_actExit;

    QTimer* m_rebuildTimer;
    QTimer* m_dropTimer;

    IconExtractor m_iconExtractor;

    InputDataList m_inputData;
    CommandHistory m_history;
    QList<CatItem> m_searchResult;
    CatItem m_outputItem;
    bool m_alwaysShowLaunchy;

    bool m_dragging;
    QPoint m_dragStartPoint;
    bool m_menuOpen;

    OptionDialog* m_optionDialog;
    bool m_optionsOpen;

private:
    static LaunchyWidget* s_instance;
};

void createLaunchyWidget(CommandFlags command);

#define g_mainWidget launchy::LaunchyWidget::instance()

} // namespace launchy
