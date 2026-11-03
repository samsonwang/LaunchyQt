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
#include <QUrl>

#include "Core/CommandHistory.h"
#include "Core/StartupOption.h"

#include "PluginInterface/CatalogItem.h"
#include "PluginInterface/InputData.h"

#include "Platform/IconExtractor.h"

#include "CatalogBuilder.h"

class QPushButton;
class QHotkey;

namespace launchy {
class Catalog;
class Fader;
class AnimationLabel;
class IconDelegate;
class CharListWidget;
class CharLineEdit;
class OptionDialog;

// Passed to QCoreApplication::exit() to mean "start over with the same command
// line" rather than "quit". main() compares the event loop's exit code against
// it after exec() and re-runs the arguments it parsed. Any value works as long
// as it never collides with a code the event loop returns on its own.
constexpr int EXIT_CODE_RESTART = 64;

class LaunchyWidget : public QWidget {
    Q_OBJECT
public:
    static LaunchyWidget* instance();
    // Public on purpose: the singleton is destroyed by the free function
    // cleanupLaunchyWidget(), and `delete` through a LaunchyWidget* needs access
    // to the destructor. Construction stays protected, so the singleton rule is
    // still enforced.
    virtual ~LaunchyWidget();

public:
    // Act on the parsed command line. Called once, at the end of the
    // constructor, with what that constructor decided to add to it.
    void executeStartupCommand(const StartupOption& option);
    void setAlternativeListMode(int mode);
    bool setHotkey(const QKeySequence& hotkey);
    bool setAlwaysTop(bool alwaysTop);
    void setSkin(const QString& name);
    int getHotkey() const;
    void startRebuildTimer();

    void showTrayIcon();
    void hideTrayIcon();
    // show a tray notification, clicking it opens url in browser when url is valid
    void trayNotify(const QString& infoMsg, const QUrl& url = QUrl());

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
    // The parsed command line, read only: the constructor copies it and adds to
    // that copy (first run, always-show, a catalog that needs rebuilding, an
    // unusable hotkey) instead of writing into the caller's object.
    LaunchyWidget(const StartupOption& option);
    Q_DISABLE_COPY(LaunchyWidget)

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
    // Resolve the screen a window should be placed on. A negative index means
    // "follow the cursor", any out of range index falls back to the first screen.
    QScreen* screenAtIndex(int index) const;
    // Screen covering the given global point. Falls back to the primary screen
    // when no screen covers the point (monitor unplugged, point outside the
    // virtual desktop), so this helper never returns an unvalidated pointer.
    QScreen* screenAtPoint(const QPoint& globalPos) const;
    void loadPosition(const QPoint& pt);
    void savePosition();
    void doTab();
    void doBackTab();
    void doEnter();
    void processInput(bool debounce = false);
    // Text the result decoration (Catalog::decorateText) matches against. Single
    // writer: this class, from the places that know the current query; it also
    // pushes the value to the alternatives list, whose delegate needs it while
    // painting.
    void setSearchText(const QString& text);
    // Immediate full search + UI refresh. Used by explicit actions and by the
    // debounce timer once typing pauses.
    void doSearch();
    void searchOnInput();
    void launchItem();
    void startDropTimer();
    // Re-applies the translated texts to the actions, the tool tips and the
    // tray icon. Called from changeEvent() on QEvent::LanguageChange.
    void retranslateUi();

protected slots:
    // Handlers of the signals emitted by the objects owned by this class.
    void showOptionDialog();                   // m_optionButton, m_actOptions
    void onHotkey();                           // m_pHotKey
    void dropTimeout();                        // m_dropTimer
    void catalogProgressUpdated(int progress); // m_catalogBuilder
    void catalogBuilt();
    void setFadeLevel(double level);
    void iconExtracted(const QString& plugin, const QString& path, const QIcon& icon);
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void trayMessageClicked();
    void reloadSkin();
    void exit();
    // m_rebuildTimer, gated by the system idle time
    void scheduledBuildCatalog();
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
    QUrl m_notifyUrl;
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
    // debounce timer that defers the catalog search while the user is
    // typing rapidly; only the final query after a short idle window triggers it.
    QTimer* m_searchTimer;

    InputDataList m_inputData;
    QList<CatItem> m_searchResult;
    CatItem m_outputItem;
    // Decoration search text, see setSearchText(). Empty means "highlight nothing".
    QString m_searchText;

    IconExtractor m_iconExtractor;

    CommandHistory m_history;
    // The catalog and the builder that fills it are both owned here. Declared
    // last so the builder (destroyed first) has already stopped its worker
    // thread; the catalog itself is deleted in the destructor, right after
    // m_catalogBuilder.shutdown(), because that worker thread scans it.
    Catalog* m_catalog;
    CatalogBuilder m_catalogBuilder;

    bool m_dragging;
    QPoint m_dragStartPos;
    QPoint m_dragStartGlobalPos;
    bool m_menuOpen;

    OptionDialog* m_optionDialog;
    bool m_optionsOpen;
};

// Create the one widget, handing it the parsed command line. Implemented per
// platform (Windows/LaunchyWidgetWin.cpp, Linux/LaunchyWidgetLinux.cpp), which
// is also why the class constructors stay protected: this function and the
// subclass are the only ones allowed to build it. The option is only read here;
// the copy the constructor is allowed to add to is made there.
void initLaunchyWidget(const StartupOption& option);
void cleanupLaunchyWidget();

#define g_mainWidget launchy::LaunchyWidget::instance()

} // namespace launchy
