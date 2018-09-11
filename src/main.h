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

#ifndef MAIN_H
#define MAIN_H


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


class LaunchyWidget : public QWidget
{
	Q_OBJECT  // Enable signals and slots

public:
	LaunchyWidget(CommandFlags command);
	~LaunchyWidget();

	void executeStartupCommand(int command);

	Catalog* catalog;
	PluginHandler plugins;

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

protected:
    void paintEvent(QPaintEvent* event);

public slots:
    void focusInEvent(QFocusEvent* event);
	void focusOutEvent(QFocusEvent* event);
	void mousePressEvent(QMouseEvent* event);
	void mouseMoveEvent(QMouseEvent* event);
	void mouseReleaseEvent(QMouseEvent* event);
	void contextMenuEvent(QContextMenuEvent* event);
	void showOptionsDialog();
	void onHotkey();
	void dropTimeout();
	void setOpaqueness(int level);
	void httpGetFinished(bool result);
	void catalogProgressUpdated(int);
	void catalogBuilt();
	void buildCatalog();
	void inputMethodEvent(QInputMethodEvent* event);
	void keyPressEvent(QKeyEvent* event);
	void inputKeyPressEvent(QKeyEvent* event);
	void alternativesRowChanged(int index);
	void alternativesKeyPressEvent(QKeyEvent* event);
	void setFadeLevel(double level);
	void showLaunchy();
	void iconExtracted(int index, QString path, QIcon icon);
	void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
	void reloadSkin();

protected:
	void saveSettings();

private:
	void createActions();
	void applySkin(const QString& name);
	void closeEvent(QCloseEvent* event);
	void hideLaunchy(bool noFade = false);
	void updateVersion(int oldVersion);
	void checkForUpdate();
	void shouldDonate();
	void updateAlternatives(bool resetSelection = true);
	void showAlternatives();
	void hideAlternatives();
	void parseInput(const QString& text);
	void updateOutputWidgets(bool resetAlternativesSelection = true);
	void searchOnInput();
	void loadPosition(QPoint pt);
	void savePosition() { gSettings->setValue("Display/pos", pos()); }
	void doTab();
	void doBackTab();
	void doEnter();
	void processKey();
	void launchItem(CatItem& item);
	void addToHistory(QList<InputData>& item);
	void startDropTimer();

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

	QHttp *http;
	QBuffer *verBuffer;
	QBuffer *counterBuffer;
};


LaunchyWidget* createLaunchyWidget(CommandFlags command);


#endif
