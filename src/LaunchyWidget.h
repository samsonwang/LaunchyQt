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

#include <QSystemTrayIcon>
#include <QPushButton>

#include "plugin_handler.h"
#include "platform_util.h"
#include "catalog.h"
#include "catalog_builder.h"
#include "icon_delegate.h"
#include "icon_extractor.h"
#include "globals.h"
#include "InputDataList.h"
#include "CommandHistory.h"
#include "CharLineEdit.h"
#include "LineEditMenu.h"
#include "CharListWidget.h"
#include "AnimationLabel.h"
#include "Fader.h"

class QHotkey;

enum CommandFlag
{
	None = 0,
	ShowLaunchy = 1,
	ShowOptions = 2,
	ResetPosition = 4,
	ResetSkin = 8,
	Rescan = 16,
	Exit = 32
};

Q_DECLARE_FLAGS(CommandFlags, CommandFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(CommandFlags)

class LaunchyWidget : public QWidget {
	Q_OBJECT
public:
	LaunchyWidget(CommandFlags command);
	virtual ~LaunchyWidget();

public:
	void executeStartupCommand(int command);

	void showLaunchy(bool noFade);
	void showTrayIcon();

	void setSuggestionListMode(int mode);
	bool setHotkey(QKeySequence);
	bool setAlwaysShow(bool);
	bool setAlwaysTop(bool);
	void setSkin(const QString& name);
	void loadOptions();
	int getHotkey() const;
	void startUpdateTimer();
    void setOpaqueness(int level);


protected:
    virtual void paintEvent(QPaintEvent* event);
    virtual void closeEvent(QCloseEvent* event);
    //virtual void focusInEvent(QFocusEvent* event);
    //virtual void focusOutEvent(QFocusEvent* event);
    virtual void inputMethodEvent(QInputMethodEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void contextMenuEvent(QContextMenuEvent* event);

    void saveSettings();

private:
    void createActions();
    void applySkin(const QString& name);
    void hideLaunchy(bool noFade = false);
    void updateVersion(int oldVersion);
    void checkForUpdate();
    void shouldDonate();
    void updateAlternatives(bool resetSelection = true);
    void showAlternatives();
    void hideAlternatives();
    // void parseInput(const QString& text);
    void updateOutputWidgets(bool resetAlternativesSelection = true);
    void searchOnInput();
    void loadPosition(QPoint pt);
    void savePosition() { g_settings->setValue("Display/pos", pos()); }
    void doTab();
    void doBackTab();
    void doEnter();
    void processKey();
    void launchItem(CatItem& item);
    // void addToHistory(QList<InputData>& item);
    void startDropTimer();


public slots:
    void showLaunchy();
    void buildCatalog();

protected slots:
	void showOptionsDialog();
	void onHotkey();
	void dropTimeout();
    void inputKeyPressed(QKeyEvent* event);
	void httpGetFinished(bool result);
	void catalogProgressUpdated(int);
	void catalogBuilt();
	void alternativesRowChanged(int index);
	void alternativesKeyPressed(QKeyEvent* event);
	void setFadeLevel(double level);
	void iconExtracted(int index, QString path, QIcon icon);
	void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
	void reloadSkin();
    void exit();
    void onInputFocusOut();


public:
    Catalog* catalog;
    PluginHandler plugins;

private:
    QString currentSkin;

	Fader* fader;
	QPixmap* frameGraphic;
	QSystemTrayIcon* trayIcon;
	CharLineEdit* input;
	QLabel* output;
	QLabel* outputIcon;
	CharListWidget* alternatives;
	QRect alternativesRect;
	QPushButton* optionsButton;
	QPushButton* closeButton;
	QScrollBar* altScroll;
	QLabel* alternativesPath;
	AnimationLabel* workingAnimation;

	QAction* actShow;
	QAction* actRebuild;
    QAction* actReloadSkin;
	QAction* actOptions;
	QAction* actExit;

	QTimer* updateTimer;
	QTimer* dropTimer;
	QThread builderThread;
	IconExtractor iconExtractor;
	QIcon* condensedTempIcon;
	CatItem outputItem;
	QList<CatItem> searchResults;
	InputDataList inputData;
	CommandHistory history;
	bool alwaysShowLaunchy;
	bool dragging;
	QPoint dragStartPoint;
	bool menuOpen;
	bool optionsOpen;

	IconDelegate* listDelegate;
	QAbstractItemDelegate* defaultListDelegate;

    //QHttp* http;
	QBuffer* verBuffer;
	QBuffer* counterBuffer;

    QHotkey* m_pHotKey;
};

LaunchyWidget* createLaunchyWidget(CommandFlags command);
