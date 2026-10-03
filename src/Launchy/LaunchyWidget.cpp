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

#include "LaunchyWidget.h"

#include <QApplication>
#include <QScrollBar>
#include <QMessageBox>
#include <QMenu>
#include <QSystemTrayIcon>
#include <QPushButton>
#include <QTimer>
#include <QPainter>
#include <QDir>
#include <QPixmap>
#include <QBitmap>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QInputMethodEvent>
#include <QMouseEvent>
#include <QScreen>
#include <QUrl>
#include <QDesktopServices>

#include <QHotkey/QHotkey>

#include "LaunchyLib/PluginInterface.h"
#include "LaunchyLib/PluginMsg.h"

#include "GlobalVar.h"
#include "IconDelegate.h"
#include "OptionDialog.h"
#include "OptionItem.h"
#include "FileSearch.h"
#include "SettingsManager.h"
#include "AppBase.h"
#include "Fader.h"
#include "IconDelegate.h"
#include "AnimationLabel.h"
#include "CharListWidget.h"
#include "CharLineEdit.h"
#include "Catalog.h"
#include "CatalogBuilder.h"
#include "PluginHandler.h"
#include "UpdateChecker.h"
#include "LaunchyVersion.h"
#include "MemProfiler.h"

namespace launchy {

// for qt flags
// check this page https://stackoverflow.com/questions/10755058/qflags-enum-type-conversion-fails-all-of-a-sudden
using ::operator|;

// minimum system idle time (in seconds) required before a scheduled
// catalog rebuild is allowed to start
const int SCHEDULED_REBUILD_MIN_IDLE_SECONDS = 10 * 60;
// interval (in milliseconds) used to re-check the idle condition when a
// scheduled catalog rebuild is postponed because the system is still in use
const int SCHEDULED_REBUILD_CHECK_INTERVAL = 60 * 1000;

LaunchyWidget* LaunchyWidget::s_instance = nullptr;

LaunchyWidget::LaunchyWidget(CommandFlags command)
    : QWidget(nullptr),
      m_skinChanged(false),
      m_inputBox(new CharLineEdit(this)),
      m_outputBox(new QLabel(this)),
      m_outputIcon(new QLabel(this)),
      m_alternativeList(new CharListWidget(this)),
      m_optionButton(new QPushButton(this)),
      m_closeButton(new QPushButton(this)),
      m_workingAnimation(new AnimationLabel(this)),
      m_trayIcon(new QSystemTrayIcon(this)),
      m_fader(new Fader(this)),
      m_pHotKey(new QHotkey(this)),
      m_rebuildTimer(new QTimer(this)),
      m_dropTimer(new QTimer(this)),
      m_searchTimer(new QTimer(this)),
      m_alwaysShowLaunchy(false),
      m_dragging(false),
      m_placement(0.5, 0.5),
      m_menuOpen(false),
      m_optionDialog(nullptr),
      m_optionsOpen(false) {

    // Publish ourselves as the singleton as early as possible, the rest of the
    // constructor can then safely use g_mainWidget. The startup command runs at
    // the end of this constructor and may open the options dialog, which reads
    // g_mainWidget->getHotkey(). Without this, s_instance was still null there
    // and opening the options dialog crashed.
    s_instance = this;

#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
#elif defined(Q_OS_MAC)
    setWindowFlags(Qt::FramelessWindowHint);
#endif

    g_searchText.clear();

    setObjectName("launchy");
    setWindowTitle(tr("Launchy"));

#if defined(Q_OS_WIN)
    setWindowIcon(QIcon(":/resources/launchy128.png"));
#elif defined(Q_OS_MAC)
    setWindowIcon(QIcon("../Resources/launchy_icon_mac.icns"));
    //setAttribute(Qt::WA_MacAlwaysShowToolWindow);
#endif

    setAttribute(Qt::WA_AlwaysShowToolTips);
    setAttribute(Qt::WA_InputMethodEnabled);
    if (g_app->supportsAlphaBorder()) {
        setAttribute(Qt::WA_TranslucentBackground);
    }
    setFocusPolicy(Qt::ClickFocus);

    createActions();

    connect(&m_iconExtractor, &IconExtractor::iconExtracted,
            this, &LaunchyWidget::iconExtracted);

    m_inputBox->setObjectName("input");
    connect(m_inputBox, &CharLineEdit::keyPressed,
            this, &LaunchyWidget::onInputBoxKeyPressed);
    connect(m_inputBox, &CharLineEdit::focusOut,
            this, &LaunchyWidget::onInputBoxFocusOut);
    connect(m_inputBox, &CharLineEdit::inputMethod,
            this, &LaunchyWidget::onInputBoxInputMethod);

    m_outputBox->setObjectName("output");
    m_outputBox->setAlignment(Qt::AlignHCenter);

    m_outputIcon->setObjectName("outputIcon");
    m_outputIcon->setGeometry(QRect());

    m_alternativeList->setObjectName("alternatives");
    setAlternativeListMode(g_settings->value(OPTION_CONDENSEDVIEW, OPTION_CONDENSEDVIEW_DEFAULT).toInt());
    connect(m_alternativeList, &QListWidget::currentRowChanged,
            this, &LaunchyWidget::onAlternativeListRowChanged);
    connect(m_alternativeList, &CharListWidget::keyPressed,
            this, &LaunchyWidget::onAlternativeListKeyPressed);
    connect(m_alternativeList, &CharListWidget::focusOut,
            this, &LaunchyWidget::onAlternativeListFocusOut);

    m_optionButton->setObjectName("opsButton");
    m_optionButton->setToolTip(tr("Options"));
    m_optionButton->setGeometry(QRect());
    connect(m_optionButton, &QPushButton::clicked,
            this, &LaunchyWidget::showOptionDialog);

    m_closeButton->setObjectName("closeButton");
    m_closeButton->setToolTip(tr("Close"));
    m_closeButton->setGeometry(QRect());
    connect(m_closeButton, &QPushButton::clicked,
            qApp, &QApplication::quit);

    m_workingAnimation->setObjectName("workingAnimation");
    m_workingAnimation->setGeometry(QRect());

    // tray icon
    if (!m_trayIcon->contextMenu()) {
        QMenu* trayMenu = new QMenu(this);
        trayMenu->addAction(m_actShow);
        trayMenu->addAction(m_actReloadSkin);
        trayMenu->addAction(m_actRebuild);
        trayMenu->addSeparator();
        trayMenu->addAction(m_actOptions);
        trayMenu->addAction(m_actCheckUpdate);
        trayMenu->addSeparator();
        trayMenu->addAction(m_actRestart);
        trayMenu->addAction(m_actExit);
        m_trayIcon->setContextMenu(trayMenu);
    }

    m_trayIcon->setIcon(QIcon(":/resources/launchy16.png"));

    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &LaunchyWidget::trayIconActivated);

    connect(m_trayIcon, &QSystemTrayIcon::messageClicked,
            this, &LaunchyWidget::trayMessageClicked);

    if (g_settings->value(OPTION_HIDE_TRAY_ICON, OPTION_HIDE_TRAY_ICON_DEFAULT).toBool()) {
        hideTrayIcon();
    }
    else {
        showTrayIcon();
    }

    connect(m_fader, &Fader::fadeLevel, this, &LaunchyWidget::setFadeLevel);

    // If this is the first time running or a new version, call updateVersion
    int version = g_settings->value(OPTION_VERSION, OPTION_VERSION_DEFAULT).toInt();
    if (version != LAUNCHY_VERSION) {
        updateVersion(version);
        command |= ShowLaunchy;
    }

    // Set the general options
    if (setAlwaysShow(g_settings->value(OPTION_ALWAYSSHOW, OPTION_ALWAYSSHOW_DEFAULT).toBool())) {
        command |= ShowLaunchy;
    }
    setAlwaysTop(g_settings->value(OPTION_ALWAYSTOP, OPTION_ALWAYSTOP_DEFAULT).toBool());

    // Set the hotkey
    QKeySequence hotkey = getHotkey();
    connect(m_pHotKey, &QHotkey::activated, this, &LaunchyWidget::onHotkey);
    if (!setHotkey(hotkey)) {
        QMessageBox::warning(this, tr("Launchy"),
                             tr("The hotkey %1 is already in use, please select another.")
                             .arg(hotkey.toString()));
        command = ShowLaunchy | ShowOptions;
    }

    // Load the catalog
    connect(g_builder, &CatalogBuilder::catalogIncrement,
            this, &LaunchyWidget::catalogProgressUpdated);
    connect(g_builder, &CatalogBuilder::catalogFinished,
            this, &LaunchyWidget::catalogBuilt);

    // Rescan if the catalog is missing or if catalog directories were
    // rewritten because the user home directory changed (account rename
    // or settings migrated to another machine)
    if (!g_catalog->load(SettingsManager::instance().catalogFilename())
        || g_needRebuildCatalog.fetchAndStoreRelaxed(0) > 0) {
        command |= Rescan;
    }

    // Load the history
    m_history.load(SettingsManager::instance().historyFilename());

    // Load fail-safe basic skin
    QFile basicSkinFile(":/resources/basicskin.qss");
    basicSkinFile.open(QFile::ReadOnly);
    qApp->setStyleSheet(basicSkinFile.readAll());
    // Load skin
    applySkin(g_settings->value(OPTION_SKIN, OPTION_SKIN_DEFAULT).toString());

    // Move to saved position
    loadPosition(g_settings->value(OPTION_POS, OPTION_POS_DEFAULT).toPoint());

    connect(g_app, &SingleApplication::instanceStarted,
            this, &LaunchyWidget::onSecondInstance);

    // reload skin after new screen is added or removed
    connect(g_app, &QGuiApplication::screenAdded,
            this, &LaunchyWidget::onScreenChanged);

    connect(g_app, &QGuiApplication::screenRemoved,
            this, &LaunchyWidget::onScreenChanged);

    connect(g_app, &QGuiApplication::primaryScreenChanged,
            this, &LaunchyWidget::onScreenChanged);

    // Set the timers
    m_dropTimer->setSingleShot(true);
    connect(m_dropTimer, &QTimer::timeout, this, &LaunchyWidget::dropTimeout);

    m_rebuildTimer->setSingleShot(true);
    connect(m_rebuildTimer, &QTimer::timeout,
            this, &LaunchyWidget::scheduledBuildCatalog);
    startRebuildTimer();

    // debounce the catalog search while typing. A short single-shot
    // window means only the final query after the user pauses triggers the
    // (potentially expensive) scan, instead of one search per keystroke.
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(60);
    connect(m_searchTimer, &QTimer::timeout,
            this, &LaunchyWidget::doSearch);

    // start update checker
    UpdateChecker::instance().startup();

    // Load the plugins
    PluginHandler::instance().loadPlugins();
    launchy::memprof::record("widget:after-loadPlugins");

    executeStartupCommand(command);
}

LaunchyWidget::~LaunchyWidget() {
    s_instance = nullptr;
    m_trayIcon->hide();
    if (m_optionDialog) {
        m_optionDialog->close();
        delete m_optionDialog;
        m_optionDialog = nullptr;
    }
}

LaunchyWidget* LaunchyWidget::instance() {
    return s_instance;
}

void LaunchyWidget::cleanup() {
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

void LaunchyWidget::executeStartupCommand(int command) {
    if (command & ResetPosition) {
        // The saved position may sit outside every screen, so ask for the screen
        // covering the window center and let screenAtIndex() fall back when there is none.
        QScreen* screen = screenAtIndex(0);
        if (!screen) {
            qWarning() << "LaunchyWidget::executeStartupCommand, ResetPosition skipped, no screen available";
        }
        else {
            QRect rectScreen = screen->availableGeometry();
            QRect rectWidget = geometry();

            QPoint pos(rectScreen.width() / 2 - rectWidget.width() / 2,
                      rectScreen.height() / 2 - rectWidget.height() / 2);
            move(pos);
        }
    }

    if (command & ResetSkin) {
        setOpaqueness(100);
        showTrayIcon();
        applySkin("Default");
    }

    if (command & ShowLaunchy) {
        showLaunchy();
    }

    if (command & ShowOptions) {
        showOptionDialog();
    }

    if (command & Rescan) {
        buildCatalog();
    }

    if (command & Exit) {
        exit();
    }
}

void LaunchyWidget::showEvent(QShowEvent* event) {
    if (m_skinChanged) {
        // output icon may changed with skin
        updateOutputSize();
        m_skinChanged = false;
    }
    QWidget::showEvent(event);
}

void LaunchyWidget::paintEvent(QPaintEvent* event) {
    // Do the default draw first to render any background specified in the stylesheet
    QStyleOption styleOption;
    styleOption.initFrom(this);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    style()->drawPrimitive(QStyle::PE_Widget, &styleOption, &painter, this);

    // Now draw the standard frame.png graphic if there is one
    if (!m_frameGraphic.isNull()) {
        painter.drawPixmap(0, 0, m_frameGraphic);
    }

    QWidget::paintEvent(event);
}

void LaunchyWidget::setAlternativeListMode(int mode) {
    m_alternativeList->setListMode(mode);
}

bool LaunchyWidget::setHotkey(const QKeySequence& hotkey) {
    QKeySequence seqOld = m_pHotKey->shortcut();
    m_pHotKey->setShortcut(hotkey, true);

    if (!m_pHotKey->isRegistered()) {
        m_pHotKey->setShortcut(seqOld, true);
        return false;
    }

    m_trayIcon->setToolTip(tr("Launchy %1\npress %2 to activate")
                           .arg(LAUNCHY_VERSION_STRING)
                           .arg(hotkey.toString()));

    return true;
}

// Repopulate the alternatives list with the current search results
// and set its size and position accordingly.
void LaunchyWidget::updateAlternativeList(bool resetSelection) {
    int mode = g_settings->value(OPTION_CONDENSEDVIEW, OPTION_CONDENSEDVIEW_DEFAULT).toInt();

    int i = 0;
    for (; i < m_searchResult.size(); ++i) {
        qDebug() << "LaunchyWidget::updateAlternativeList," << i << ":"
                 << m_searchResult[i].shortName << ","
                 << m_searchResult[i].fullPath;
        QString fullPath = QDir::toNativeSeparators(m_searchResult[i].fullPath);
#ifndef NDEBUG
        fullPath += QString(" (%1 launches)").arg(m_searchResult[i].usage);
#endif
        QListWidgetItem* item = nullptr;
        if (i < m_alternativeList->count()) {
            item = m_alternativeList->item(i);
        }
        else {
            item = new QListWidgetItem(fullPath, m_alternativeList);
        }

        if (item->data(mode == 1 ? ROLE_SHORT : ROLE_FULL) != fullPath) {
            // condensedTempIcon is a blank icon or null
            item->setData(ROLE_ICON, QIcon());
            item->setSizeHint(QSize(32, 32));
        }
        item->setData(mode == 1 ? ROLE_FULL : ROLE_SHORT, m_searchResult[i].shortName);
        item->setData(mode == 1 ? ROLE_SHORT : ROLE_FULL, fullPath);
        item->setData(Qt::UserRole, m_searchResult[i].fullPath);

        if (i >= m_alternativeList->count()) {
            m_alternativeList->addItem(item);
        }
    }

    while (m_alternativeList->count() > i) {
        delete m_alternativeList->takeItem(i);
    }

    if (resetSelection) {
        m_alternativeList->setCurrentRow(0);
    }

    m_iconExtractor.processIcons(m_searchResult);

    m_alternativeList->updateGeometry(pos(), m_inputBox->pos());
}


void LaunchyWidget::showAlternativeList() {
    // Ensure that any pending shows of the alternatives list are cancelled
    // so that we only update the list once.
    m_dropTimer->stop();

    m_alternativeList->show();
    m_alternativeList->setFocus();
}


void LaunchyWidget::hideAlternativeList() {
    // Ensure that any pending shows of the alternatives list are cancelled
    // so that the list isn't erroneously shown shortly after being dismissed.
    m_dropTimer->stop();

    // clear the selection before hiding to prevent flicker
    m_alternativeList->setCurrentRow(-1);
    m_alternativeList->repaint();
    m_alternativeList->hide();
    m_iconExtractor.stop();
}

void LaunchyWidget::launchItem() {
    if (m_inputData.empty()) {
        return;
    }

    qDebug() << "LaunchyWidget::launchItem, inputdata size:" << m_inputData.size();

    CatItem& item = m_inputData[0].getTopResult();
    qDebug() << "LaunchyWidget::launchItem, item.shortName:" << item.shortName
             << "item.fullPath:" << item.fullPath
             << "item.pluginName:" << item.pluginName
             << "item.data:" << item.data;

    int ops = MSG_CONTROL_LAUNCHITEM;

    if (item.pluginName != NAME_LAUNCHY && item.pluginName != NAME_LAUNCHYFILE) {
        ops = PluginHandler::instance().launchItem(&m_inputData, &item);
        switch (ops) {
        case MSG_CONTROL_EXIT:
            exit();
            break;
        case MSG_CONTROL_OPTIONS:
            showOptionDialog();
            break;
        case MSG_CONTROL_REBUILD:
            buildCatalog();
            break;
        case MSG_CONTROL_RELOADSKIN:
            reloadSkin();
            break;
        default:
            break;
        }
    }

    qDebug() << "LaunchyWidget::launchItem, ops after plugins:" << ops;

    if (ops == MSG_CONTROL_LAUNCHITEM) {
        QString args;
        if (item.pluginName == NAME_HISTORY) {
            qDebug() << "LaunchyWidget::launchItem, get args from history";
            int historyIndex = (int)(int64_t)(item.data);
            InputDataList inputData = m_history.getItem(historyIndex);
            if (!inputData.isEmpty()) {
                for (int i = 1; i < inputData.count(); ++i) {
                    args += inputData[i].getText() + " ";
                }
            }
            else {
                // The entry aged out of the history (trimmed to the configured
                // maximum, or deleted) since the alternatives list was built,
                // fall back on the command line that is still on screen.
                qWarning() << "LaunchyWidget::launchItem, history entry gone:"
                           << historyIndex
                           << ", history size:" << m_history.getItemCount();
                for (int i = 1; i < m_inputData.count(); ++i) {
                    args += m_inputData[i].getText() + " ";
                }
            }
        }
        else if (m_inputData.count() > 1) {
            for (int i = 1; i < m_inputData.count(); ++i) {
                args += m_inputData[i].getText() + " ";
            }
        }

        qDebug() << "LaunchyWidget::launchItem, cmd:" << item.fullPath << "args:" << args;
        runProgram(item.fullPath, args);
    }

    // udpate outputbox
    updateOutputItem(item);

    g_catalog->incrementUsage(item);
    m_history.addItem(m_inputData);
}

void LaunchyWidget::onAlternativeListRowChanged(int row) {
    // Check that index is a valid history item index
    // If the current entry is a history item or there is no text entered
    if (row < 0 || row >= m_searchResult.count()) {
        qWarning() << "LaunchyWidget::onAlternativeListRowChanged, invalid row:" << row
                   << ", current row:" << m_alternativeList->currentRow();
        return;
    }

    const CatItem& item = m_searchResult[row];
    qDebug() << "LaunchyWidget::onAlternativeListRowChanged, row:" << row
             << ", item.fullpath:" << item.fullPath
             << ", item.shortName:" << item.shortName
             << ", item.pluginName:" << item.pluginName
             << ", inputBox:" << m_inputBox->text();

    if ( (!m_inputData.isEmpty() && m_inputData.first().hasLabel(LABEL_HISTORY))
         || m_inputBox->text().isEmpty() ) {
        // Used a void* to hold an int.. ick!
        // BUT! Doing so avoids breaking existing catalogs

        // The row index inside the alternatives list has nothing to do with a
        // history index: m_searchResult also carries catalog, plugin and file
        // search matches, so its count is never a valid upper bound for the
        // history. The history index is the one carried by the item itself, and
        // getItem() hands back an empty entry when it no longer resolves, which
        // is how a stale index (the history shrank after the list was built) is
        // caught here.
        int historyIndex = (int)(int64_t)(item.data);
        InputDataList historyEntry = m_history.getItem(historyIndex);
        if (item.pluginName == NAME_HISTORY && !historyEntry.isEmpty()) {
            qDebug() << "LaunchyWidget::onAlternativeListRowChanged, list history"
                     << item.shortName;

            m_inputData = historyEntry;
            m_inputBox->selectAll();
            m_inputBox->insert(m_inputData.toString());
            m_inputBox->selectAll();
            m_outputBox->setText(m_inputData[0].getTopResult().shortName);
            // No need to fetch the icon again, just grab it from the alternatives row
            m_outputIcon->setPixmap(m_alternativeList->item(row)->icon().pixmap(m_outputIcon->size()));
            m_outputItem = item;
            g_searchText = m_inputData.toString();
        }
    }
    else if (!m_inputData.isEmpty() && (m_inputData.last().hasLabel(LABEL_AUTOSUGGEST)
                                        || !m_inputData.last().hasText())) {
        qDebug() << "LaunchyWidget::onAlternativeListRowChanged"
                 << ", auto suggest:" << item.shortName
                 << ", m_inputData.size():" << m_inputData.size();

        m_inputData.last().setText(item.shortName);
        m_inputData.last().setTopResult(item);

        QString inputRoot = m_inputData.toString(true);
        m_inputBox->selectAll();
        m_inputBox->insert(inputRoot + item.shortName);
        m_inputBox->setSelection(inputRoot.length(), item.shortName.length());

        m_outputBox->setText(item.shortName);
        // No need to fetch the icon again, just grab it from the alternatives row
        m_outputIcon->setPixmap(m_alternativeList->item(row)->icon().pixmap(m_outputIcon->size()));
        m_outputItem = item;
        g_searchText = "";
    }
    else {
        qDebug() << "LaunchyWidget::onAlternativeListRowChanged, update top result";
        m_inputData.last().setTopResult(item);
    }

    qDebug() << "LaunchyWidget::onAlternativeListRowChanged, input box text:"
             << m_inputBox->text();
}

void LaunchyWidget::onInputBoxKeyPressed(QKeyEvent* event) {
    if (event == nullptr) {
        return;
    }

    // Launchy widget would not receive Key_Tab from inputbox,
    // we have to pass it manually
    if (event->key() == Qt::Key_Tab) {
        qDebug() << "LaunchyWidget::onInputBoxKeyPressed,"
                 << "pass event to LaunchyWidget::keyPressEvent";
        keyPressEvent(event);
    }
    else {
//        qDebug() << "LaunchyWidget::onInputBoxKeyPressed,"
//            << "event ignored";
        event->ignore();
    }
}

void LaunchyWidget::onAlternativeListKeyPressed(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        hideAlternativeList();
        m_inputBox->setFocus();
        event->ignore();
    }
    else if (event->key() == Qt::Key_Return
             || event->key() == Qt::Key_Enter
             || event->key() == Qt::Key_Tab) {
        if (!m_searchResult.isEmpty()) {
            int row = m_alternativeList->currentRow();
            if (row > -1) {
                QString location = "History/" + m_inputBox->text();
                QStringList hist;
                hist << m_searchResult[row].shortName << m_searchResult[row].fullPath;
                g_settings->setValue(location, hist);

                if (row > 0) {
                    m_searchResult.move(row, 0);
                }

                if (event->key() == Qt::Key_Tab) {
                    doTab();
                    processInput();
                }
                else {
                    // Load up the inputData properly before running the command
                    /* commented out until I find a fix for it breaking the history selection
                       inputData.last().setTopResult(searchResults[0]);
                       doTab();
                       inputData.parse(input->text());
                       inputData.erase(inputData.end() - 1);*/

                    //updateOutputBox();
                    keyPressEvent(event);
                }
            }
        }
    }
    else if (event->key() == Qt::Key_Delete
             && (event->modifiers() & Qt::ShiftModifier) != 0) {
        int row = m_alternativeList->currentRow();
        if (row > -1) {
            const CatItem& item = m_searchResult[row];
            if (item.pluginName == NAME_HISTORY) {
                // Delete selected history entry from the alternatives list.
                // row indexes m_searchResult, not m_history; the history index
                // is the one carried by the item itself.
                int historyIndex = (int)(int64_t)(item.data);
                qDebug() << "LaunchyWidget::onAlternativeListKeyPressed,"
                         << "delete history:" << item.shortName
                         << ", history index:" << historyIndex;
                m_history.removeAt(historyIndex);
                m_inputBox->clear();
                searchOnInput();
                updateAlternativeList(false);
                onAlternativeListRowChanged(m_alternativeList->currentRow());
            }
            else {
                // Demote the selected item down the alternatives list
                qDebug() << "LaunchyWidget::onAlternativeListKeyPressed,"
                         << "demote item:" << item.shortName;
                g_catalog->demoteItem(item);
                searchOnInput();
                updateOutput(false);
            }
        }
    }
    else if (event->key() == Qt::Key_Left
             || event->key() == Qt::Key_Right
             || event->text().length() > 0) {
        // Send text entry to the input control
        activateWindow();
        m_inputBox->setFocus();
        event->ignore();
        m_inputBox->processKey(event);
        keyPressEvent(event);
    }
    m_alternativeList->setFocus();
}

void LaunchyWidget::onAlternativeListFocusOut() {
    qDebug() << "LaunchyWidget::onAlternativeListFocusOut,"
             << "is main widget activeWindow:" << isActiveWindow()
             << ", is alternative list active window:" << m_alternativeList->isActiveWindow();
    if (g_settings->value(OPTION_HIDEIFLOSTFOCUS, OPTION_HIDEIFLOSTFOCUS_DEFAULT).toBool()
        && !isActiveWindow()
        && !m_alternativeList->isActiveWindow()
        && !m_optionsOpen
        && !m_fader->isFading()) {
        hideLaunchy();
    }
}

void LaunchyWidget::keyPressEvent(QKeyEvent* event) {
    if (!event || !m_alternativeList || !m_inputBox) {
        qWarning("LaunchyWidget::keyPressEvent, pointer is null");
        return;
    }

    int key = event->key();
    Qt::KeyboardModifiers mods = event->modifiers();

    qDebug() << "LaunchyWidget::keyPressEvent, key:" << key
             << "modifier:" << mods << "text:" << event->text();

    if (key == Qt::Key_Escape) {
        if (m_alternativeList->isVisible()) {
            hideAlternativeList();
        }
        else {
            hideLaunchy();
        }
    }

    else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        doEnter();
    }

    else if (key == Qt::Key_Down || key == Qt::Key_PageDown
             || key == Qt::Key_Up || key == Qt::Key_PageUp) {
        if (m_alternativeList->isVisible() && !m_alternativeList->isActiveWindow()) {
            // Don't refactor the activateWindow outside the if,
            // it won't work properly any other way!
            if (m_alternativeList->currentRow() < 0 && m_alternativeList->count() > 0) {
                m_alternativeList->activateWindow();
                m_alternativeList->setCurrentRow(0);
            }
            else {
                m_alternativeList->activateWindow();
                qApp->sendEvent(m_alternativeList, event);
            }
        }
        else if (key == Qt::Key_Down || key == Qt::Key_PageDown
                 || (m_inputBox->text().isEmpty()
                     && (key == Qt::Key_Up || key == Qt::Key_PageUp))) {
            // do a search and show the results, selecting the first one
            searchOnInput();
            if (!m_searchResult.isEmpty()) {
                updateAlternativeList();
                showAlternativeList();
            }
        }
    }

    else if ((key == Qt::Key_Tab || key == Qt::Key_Backspace)
             && mods == Qt::ShiftModifier) {
        doBackTab();
        processInput();
    }

    else if (key == Qt::Key_Tab) {
        doTab();
        processInput();
    }

    else if (key == Qt::Key_Slash || key == Qt::Key_Backslash) {
        if (!m_inputData.isEmpty()
            && m_inputData.last().hasLabel(LABEL_FILE)
            && !m_searchResult.isEmpty()
            && m_searchResult[0].pluginName == NAME_LAUNCHYFILE) {
            doTab();
        }
        processInput();
    }

    else if (key == Qt::Key_Insert && mods == Qt::ShiftModifier) {
        // ensure pasting text with Shift+Insert also parses input
        // longer term parsing should be done using the TextChanged event
        processInput();
    }

    else if (!event->text().isEmpty()){
        // Plain character typing: debounce so rapid input runs one search
        processInput(true);
    }

}

// remove input text back to the previous input section
void LaunchyWidget::doBackTab() {
    QString text = m_inputBox->text();
    int index = text.lastIndexOf(m_inputBox->separatorText());
    if (index >= 0) {
        text.truncate(index+3);
        m_inputBox->selectAll();
        m_inputBox->insert(text);
    }
    else if (text.lastIndexOf(QDir::separator()) >= 0) {
        text.truncate(text.lastIndexOf(QDir::separator())+1);
        m_inputBox->selectAll();
        m_inputBox->insert(text);
    }
    else if (text.lastIndexOf(QChar(' ')) >= 0) {
        text.truncate(text.lastIndexOf(QChar(' '))+1);
        m_inputBox->selectAll();
        m_inputBox->insert(text);
    }
    else {
        m_inputBox->clear();
    }
}

void LaunchyWidget::doTab() {

    if (!m_inputData.isEmpty() && !m_searchResult.isEmpty()) {
        qDebug() << "LaunchyWidget::doTab, get path";

        // If it's an incomplete file or directory, complete it
        QFileInfo info(m_searchResult.first().fullPath);

        if (m_inputData.last().hasLabel(LABEL_FILE) || info.isDir()) {
            QString path;
            if (info.isSymLink()) {
                path = info.symLinkTarget();
            }
            else {
                path = m_searchResult.first().fullPath;
            }

            if (info.isDir() && !path.endsWith(QDir::separator())) {
                path += QDir::separator();
            }

            m_inputData.last().setLabel(LABEL_FILE);
            m_inputBox->selectAll();
            m_inputBox->insert(m_inputData.toString(true) + QDir::toNativeSeparators(path));
        }
        else {
            m_inputData.last().setTopResult(m_searchResult[0]);
            m_inputData.last().setText(m_searchResult[0].shortName);
            m_inputBox->selectAll();
            m_inputBox->insert(m_inputData.toString() + m_inputBox->separatorText());
        }
    }
}

void LaunchyWidget::doEnter() {
    // if a debounced search is still pending (user typed fast then hit
    // Enter), flush it now so we launch against the final query, not a stale one.
    if (m_searchTimer->isActive()) {
        m_searchTimer->stop();
        doSearch();
    }

    hideAlternativeList();

    if ((!m_inputData.isEmpty() && !m_searchResult.isEmpty())
        || m_inputData.count() > 1) {
        launchItem();
        hideLaunchy();
    }
    else {
        qDebug("LaunchyWidget::doEnter, Nothing to launch");
    }
}

void LaunchyWidget::processInput(bool debounce) {
    qDebug() << "LaunchyWidget::processInput, inputbox text:" << m_inputBox->text();

    m_inputData.parse(m_inputBox->text());

    if (debounce) {
        // defer the search until typing pauses. Refresh the query text
        // now so highlight/decorate use the latest input; the catalog scan
        // itself only runs when the debounce timer fires (final query).
        QString searchText = m_inputData.isEmpty()
            ? QString() : m_inputData.last().getText();
        g_searchText = searchText.toLower();
        m_searchTimer->start();
        return;
    }

    doSearch();
}

// Immediate full search + UI refresh. Shared by explicit single actions and by
// the debounce timer once typing pauses.
void LaunchyWidget::doSearch() {
    searchOnInput();
    updateOutput();

    // If there is no input text, ensure that the alternatives list is hidden
    // otherwise, show it after the user defined delay if it's not currently visible
    if (m_inputBox->text().isEmpty()) {
        hideAlternativeList();
    }
    else if (!m_searchResult.isEmpty() && !m_alternativeList->isVisible()) {
        startDropTimer();
    }
}

void LaunchyWidget::searchOnInput() {
    // An explicit (immediate) search cancels any pending debounced search so a
    // deliberate action is never overwritten by a stale deferred run.
    if (m_searchTimer->isActive()) {
        m_searchTimer->stop();
    }

    QString searchText = m_inputData.isEmpty() ? "" : m_inputData.last().getText();
    QString searchTextLower = searchText.toLower();
    g_searchText = searchTextLower;
    m_searchResult.clear();

    if ((!m_inputData.isEmpty() && m_inputData.first().hasLabel(LABEL_HISTORY))
        || m_inputBox->text().isEmpty()) {
        // Add history items exclusively and unsorted so they remain in most recently used order
        qDebug() << "LaunchyWidget::searchOnInput, get all history items";
        m_history.getAllItem(m_searchResult);
    }
    else {
        // Search the catalog for matching items
        if (m_inputData.count() == 1) {
            qDebug() << "LaunchyWidget::searchOnInput, searching catalog for" << searchText;
            g_catalog->searchCatalogs(searchTextLower, m_searchResult);

            qDebug() << "LaunchyWidget::searchOnInput, searching history for" << searchText;
            m_history.search(searchTextLower, m_searchResult);
        }

        // Give plugins a chance to add their own dynamic matches
        // why getLabels first then getResults, why not getResult straightforward
        PluginHandler& pluginHandler = PluginHandler::instance();
        pluginHandler.getLabels(&m_inputData);
        pluginHandler.getResults(&m_inputData, &m_searchResult);

        // Sort the results by match and usage, then promote any that match previously
        // executed commands
        std::sort(m_searchResult.begin(), m_searchResult.end(), CatItemCompareRef);
        g_catalog->promoteRecentlyUsedItems(searchTextLower, m_searchResult);

        // Finally, if the search text looks like a file or directory name,
        // add any file or directory matches
        if (FileSearch::looksLikePath(searchText)) {
            FileSearch::search(searchText, m_searchResult, m_inputData);
        }

        if (!m_searchResult.isEmpty()) {
            m_inputData.last().setTopResult(m_searchResult[0]);
        }
    }
}

// If there are current results, update the output text and icon
void LaunchyWidget::updateOutput(bool resetAlternativesSelection) {
    if (!m_searchResult.isEmpty()
        && (m_inputData.count() > 1 || !m_inputBox->text().isEmpty())) {

        updateOutputItem(m_searchResult[0]);

        // Only update the alternatives list if it is visible
        if (m_alternativeList->isVisible()) {
            updateAlternativeList(resetAlternativesSelection);
        }
    }
    else {
        // No results to show, clear the output UI and hide the alternatives list
        m_outputBox->clear();
        m_outputIcon->clear();
        m_outputItem = CatItem();
        hideAlternativeList();
    }
}

void LaunchyWidget::updateOutputItem(const CatItem& item) {
    // qDebug() << "Setting output text to" << searchResults[0].shortName;
    QString outputText = Catalog::decorateText(item.shortName, g_searchText, true);

#ifdef _DEBUG
    outputText += QString(" (%1 launches)").arg(item.usage);
#endif

    qDebug() << "LaunchyWidget::updateOutputItem, setting output box text:"
             << outputText << ", usage: " << item.usage;
    m_outputBox->setText(outputText);

    if (m_outputItem != item) {
        m_outputIcon->clear();
        m_iconExtractor.processIcon(item, true);
    }

    m_outputItem = item;
}

void LaunchyWidget::startDropTimer() {
    int delay = g_settings->value(OPTION_AUTOSUGGESTDELAY, OPTION_AUTOSUGGESTDELAY_DEFAULT).toInt();
    if (delay > 0) {
        m_dropTimer->start(delay);
        qDebug() << "LaunchyWidget::startDropTimer, timer start, delay =" << delay;
    }
    else {
        dropTimeout();
    }
}

void LaunchyWidget::retranslateUi() {
    m_actShow->setText(tr("Show Launchy"));
    m_actReloadSkin->setText(tr("Reload skin"));
    m_actRebuild->setText(tr("Rebuild catalog"));
    m_actOptions->setText(tr("Options"));
    m_actCheckUpdate->setText(tr("Check for updates"));
    m_actRestart->setText(tr("Restart"));
    m_actExit->setText(tr("Exit"));

    m_optionButton->setToolTip(tr("Options"));
    m_closeButton->setToolTip(tr("Close"));

    m_trayIcon->setToolTip(tr("Launchy %1\npress %2 to activate")
                           .arg(LAUNCHY_VERSION_STRING)
                           .arg(m_pHotKey->shortcut().toString()));
}

void LaunchyWidget::updateOutputSize() {
    int nIconSize = qMax(m_outputIcon->width(), m_outputIcon->height());
    qDebug() << "LaunchyWidget::showEvent, output icon size:" << nIconSize;
    g_app->setPreferredIconSize(nIconSize);
    m_alternativeList->setIconSize(nIconSize);
}

void LaunchyWidget::dropTimeout() {
    qDebug("LaunchyWidget::dropTimeout, function entry");

    // Don't do anything if Launchy has been hidden since the timer was started
    if (isVisible() && m_searchResult.count() > 0) {
        updateAlternativeList();
        showAlternativeList();
    }
}

void LaunchyWidget::iconExtracted(const QString& pluginName, const QString& path, const QIcon& icon) {

    if (path == m_outputItem.fullPath) {
        m_outputIcon->setPixmap(icon.pixmap(m_outputIcon->size()));
    }

    for (int i = 0; i < m_alternativeList->count(); ++i)
    {
        QListWidgetItem* item = m_alternativeList->item(i);
        if (item && item->data(Qt::UserRole).toString() == path)
        {
            item->setIcon(icon);
            item->setData(ROLE_ICON, icon);

            QRect rect = m_alternativeList->visualItemRect(item);
            repaint(rect);
        }
    }
}

void LaunchyWidget::catalogProgressUpdated(int progress) {
    // The catalog builder reports progress 0 whenever a new scan starts, so
    // start the "working" animation here; catalogBuilt() stops it again.
    if (progress == 0) {
        m_workingAnimation->Start();
    }
}

void LaunchyWidget::catalogBuilt() {
    qDebug() << "LaunchyWidget::catalogBuilt, catalog built, updating search results";

    m_workingAnimation->Stop();

    // Now do a search using the updated catalog
    searchOnInput();
    updateOutput();
}

void LaunchyWidget::setSkin(const QString& name) {
    if (isVisible()) {
        hideLaunchy(true);
        applySkin(name);
        showLaunchy(false);
    }
    else {
        applySkin(name);
    }
}

void LaunchyWidget::updateVersion(int oldVersion) {
    if (oldVersion < 249) {
        g_settings->setValue(OPTION_SKIN, OPTION_SKIN_DEFAULT);
    }

    if (oldVersion != LAUNCHY_VERSION) {
        g_settings->setValue(OPTION_VERSION, LAUNCHY_VERSION);
    }
}

QScreen* LaunchyWidget::screenAtIndex(int index) const {
    // screenAt() returns nullptr when the point is not covered by any screen
    // (monitor unplugged, saved position outside the current layout, ...), so
    // this helper never hands out an unvalidated pointer: the last resort is
    // the primary screen, which may still be nullptr if the app has no screen.

    // index -1 means "follow the cursor". screenAt() is
    // allowed to return nullptr here, so it must be validated before use.
    if (index < 0) {
        QScreen* screen = QGuiApplication::screenAt(QCursor::pos());
        if (screen) {
            return screen;
        }
    }

    const QList<QScreen*> listScreen = QApplication::screens();
    if (index >= 0 && index < listScreen.size()) {
        return listScreen.at(index);
    }

    if (!listScreen.isEmpty()) {
        return listScreen.first();
    }

    return QGuiApplication::primaryScreen();
}

QScreen* LaunchyWidget::screenAtPoint(const QPoint& globalPos) const {
    if (QScreen* screen = QGuiApplication::screenAt(globalPos)) {
        return screen;
    }
    return QGuiApplication::primaryScreen();
}

void LaunchyWidget::updatePlacementRatio() {
    const QRect rectWidget = geometry();
    QScreen* screen = screenAtPoint(rectWidget.center());
    const QRect rectScreen = screen ? screen->availableGeometry() : QRect();
    if (rectScreen.isEmpty()) {
        // No display behind the window any more: remember the middle, the next
        // placement on a real display is then a plain centering.
        m_placement = QPointF(0.5, 0.5);
        return;
    }

    const qreal fx = (qreal)(rectWidget.center().x() - rectScreen.left()) / rectScreen.width();
    const qreal fy = (qreal)(rectWidget.center().y() - rectScreen.top()) / rectScreen.height();
    m_placement.setX(qBound(0.0, fx, 1.0));
    m_placement.setY(qBound(0.0, fy, 1.0));
}

void LaunchyWidget::relocateToCurrentScreen() {
    if (m_dragging) {
        return;
    }

    const QRect rectWidget = geometry();
    QScreen* screen = screenAtPoint(rectWidget.center());
    if (!screen) {
        return;
    }

    const QRect rectScreen = screen->availableGeometry();
    QPoint posTarget = rectScreen.topLeft();
    posTarget += QPoint(qRound(m_placement.x() * rectScreen.width()),
                        qRound(m_placement.y() * rectScreen.height()));
    posTarget -= QPoint(rectWidget.width() / 2, rectWidget.height() / 2);

    qDebug() << "LaunchyWidget::relocateToCurrentScreen, display layout changed,"
        << "pos:" << pos() << "->" << posTarget << "on screen:" << rectScreen;

    loadPosition(posTarget);
}

void LaunchyWidget::loadPosition(const QPoint& pos) {
    // move to selected screen
    int nScreenIndex = g_settings->value(OPTION_SCREEN_INDEX, OPTION_SCREEN_INDEX_DEFAULT).toInt();

    QScreen* screen = screenAtIndex(nScreenIndex);

    QRect rectWidget = geometry();
    const QPoint posCenter = pos + rectWidget.center();

    // The configured screen index can keep pointing at a monitor that is no
    // longer able to hold the window: unplugged, switched off, or simply no
    // longer the first entry of QApplication::screens() after the display
    // layout changed. Centering the window on that stale geometry is what
    // drops it into a corner - or off the edge - of the display it is really
    // shown on. Fall back to the screen the window actually sits on.
    if (nScreenIndex > 0
        && screen
        && !screen->availableGeometry().contains(posCenter)) {
        qDebug() << "LaunchyWidget::loadPosition, saved pos" << pos
            << "is not on screen" << nScreenIndex << "(" << screen->name()
            << ") any more, using the screen the window lives on";
        if (QScreen* screenHere = screenAtPoint(posCenter)) {
            screen = screenHere;
        }
    }

    if (!screen) {
        qWarning() << "LaunchyWidget::loadPosition, no screen available, keeping position:" << pos;
        return;
    }

    QRect rectScreen = screen->availableGeometry();

    qDebug() << "LaunchyWidget::loadPosition, pos:" << pos
        << "screen:" << rectScreen << "widget:" << rectWidget;

    QPoint posTarget(pos);
    int centerOption = g_settings->value(OPTION_ALWAYSCENTER, OPTION_ALWAYSCENTER_DEFAULT).toInt();
    if (centerOption & 1) {
        posTarget.setX(rectScreen.center().x() - rectWidget.width() / 2);
    }
    if (centerOption & 2) {
        posTarget.setY(rectScreen.center().y() - rectWidget.height() / 2);
    }

    // See if the new position is within the screen dimensions, if not pull it inside
    posTarget.setX(qBound(rectScreen.left(), posTarget.x(),
                         rectScreen.right() + 1 - rectWidget.width()));
    posTarget.setY(qBound(rectScreen.top(), posTarget.y(),
                         rectScreen.bottom() + 1 - rectWidget.height()));

    move(posTarget);

    // Remember where the window sits inside its display, so a later display
    // change can carry the window over instead of keeping old coordinates.
    updatePlacementRatio();
}

void LaunchyWidget::savePosition() {
    g_settings->setValue(OPTION_POS, pos());
}

void LaunchyWidget::saveSettings() {
    qDebug() << "LaunchyWidget::saveSettings";
    savePosition();
    g_settings->sync();
    g_catalog->save(SettingsManager::instance().catalogFilename());
    m_history.save(SettingsManager::instance().historyFilename());
}

void LaunchyWidget::startRebuildTimer() {
    int time = g_settings->value(OPTION_REBUILDTIMER, OPTION_REBUILDTIMER_DEFAULT).toInt();
    if (time > 0) {
        m_rebuildTimer->start(time * 60000);
    }
    else {
        m_rebuildTimer->stop();
    }
}

void LaunchyWidget::showTrayIcon() {
    m_trayIcon->show();
}

void LaunchyWidget::hideTrayIcon() {
    m_trayIcon->hide();
}

void LaunchyWidget::trayNotify(const QString& infoMsg, const QUrl& url) {
    m_notifyUrl = url;
    m_trayIcon->showMessage(tr("Launchy"), infoMsg,
                            QIcon(":/resources/launchy128.png"));
}

void LaunchyWidget::trayMessageClicked() {
    if (!m_notifyUrl.isValid() || m_notifyUrl.isEmpty()) {
        return;
    }
    qDebug() << "LaunchyWidget::trayMessageClicked, open url:" << m_notifyUrl;
    QDesktopServices::openUrl(m_notifyUrl);
    m_notifyUrl.clear();
}

void LaunchyWidget::onHotkey() {

    if (g_settings->value(OPTION_IGNORE_FULL_SCREEN,
                          OPTION_IGNORE_FULL_SCREEN_DEFAULT).toBool()
        && !g_app->allowNotification()) {
        qDebug() << "LaunchyWidget::onHotkey, not allow notification";
        return;
    }

    qDebug() << "LaunchyWidget::onHotkey,"
             << "always show launchy:" << m_alwaysShowLaunchy
             << "visible:" << isVisible()
             << "fading:" << m_fader->isFading()
             << "active window:" << QApplication::activeWindow();

    if (m_menuOpen || m_optionsOpen) {
        showLaunchy(true);
    }
    else if (!m_alwaysShowLaunchy
             && isVisible()
             && !m_fader->isFading()
             && QApplication::activeWindow() != nullptr) {
        qDebug() << "LaunchyWidget::onHotkey, hideLaunchy";
        hideLaunchy();
    }
    else {
        qDebug() << "LaunchyWidget::onHotkey, showLaunchy";
        showLaunchy();
    }
}

void LaunchyWidget::closeEvent(QCloseEvent* event) {
    event->ignore();
    hideLaunchy();
}

bool LaunchyWidget::setAlwaysShow(bool alwaysShow) {
    m_alwaysShowLaunchy = alwaysShow;
    return !isVisible() && alwaysShow;
}

bool LaunchyWidget::setAlwaysTop(bool alwaysTop) {
    if (alwaysTop && (windowFlags() & Qt::WindowStaysOnTopHint) == 0) {
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
        m_alternativeList->setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
        return true;
    }
    else if (!alwaysTop && (windowFlags() & Qt::WindowStaysOnTopHint) != 0) {
        setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
        m_alternativeList->setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
        return true;
    }

    return false;
}

void LaunchyWidget::setOpaqueness(int level) {
    qDebug() << "LaunchyWidget::setOpaqueness," << level;
    double value = level / 100.0;
    setWindowOpacity(value);
    m_alternativeList->setWindowOpacity(value);
}

void LaunchyWidget::reloadSkin() {
    setSkin(m_currentSkin);
}

void LaunchyWidget::exit() {
    qInfo() << "LaunchyWidget::exit, ...";
    m_fader->stop();
    hideTrayIcon();
    saveSettings();
    qApp->exit();
}

void LaunchyWidget::onInputBoxFocusOut() {
    if (g_settings->value(OPTION_HIDEIFLOSTFOCUS, OPTION_HIDEIFLOSTFOCUS_DEFAULT).toBool()
        && !isActiveWindow()
        && !m_alternativeList->isActiveWindow()
        && !m_optionsOpen
        && !m_fader->isFading()) {
        hideLaunchy();
    }
}

void LaunchyWidget::onInputBoxInputMethod(QInputMethodEvent* event) {
    qDebug() << "LaunchyWidget::onInputBoxInputMethod";
    QString commitStr = event->commitString();
    if (!commitStr.isEmpty()) {
        qDebug() << "LaunchyWidget::onInputBoxInputMethod, commit string:"
                 << commitStr << ", inputbox text:" << m_inputBox->text();
        processInput(true);
    }
}

void LaunchyWidget::onInputBoxTextEdited(const QString& str) {
    qDebug() << "LaunchyWidget::onInputBoxTextEdited, str:" << str;
    processInput(true);
}

void LaunchyWidget::onSecondInstance() {
    trayNotify(tr("Launchy is already running!"));
}

void LaunchyWidget::onScreenChanged(QScreen* screen) {
    qDebug() << "LaunchyWidget::onScreenChanged, screen:"
        << (screen ? screen->name() : QString());
    // reload screen after screen is changed
    reloadSkin();

    // A display was plugged or unplugged: the window keeps the absolute
    // coordinates it was placed with, which belong to the old layout. While
    // hidden that is harmless, but the next show() then feeds those stale
    // coordinates back into loadPosition() and the window reappears off screen
    // or in a corner of the remaining display - and stays there. Re-anchor it
    // as soon as the layout changes.
    relocateToCurrentScreen();
}

void LaunchyWidget::applySkin(const QString& name) {
    m_currentSkin = name;
    m_skinChanged = true;

    qDebug() << "apply skin:" << name;

    QString skinPath = SettingsManager::instance().skinPath(name);
    // Use default skin if this one doesn't exist or isn't valid
    if (skinPath.isEmpty()) {
        skinPath = SettingsManager::instance().skinPath(OPTION_SKIN_DEFAULT);
        // If still no good then fail with an ugly default
        if (skinPath.isEmpty()) {
            return;
        }

        g_settings->setValue(OPTION_SKIN, OPTION_SKIN_DEFAULT);
    }

    // Set a few defaults
    m_closeButton->setGeometry(QRect());
    m_optionButton->setGeometry(QRect());
    m_inputBox->setAlignment(Qt::AlignLeft);
    m_outputBox->setAlignment(Qt::AlignCenter);
    m_alternativeList->resetGeometry();

    QFile fileStyle(skinPath + "style.qss");
    fileStyle.open(QFile::ReadOnly);
    QString strStyleSheet(fileStyle.readAll());
    // transform stylesheet for external resources
    strStyleSheet.replace("url(", "url("+skinPath);
    qApp->setStyleSheet(strStyleSheet);

    bool validFrame = false;
    QPixmap frame;
    if (g_app->supportsAlphaBorder()) {
        if (frame.load(skinPath + "frame.png")) {
            validFrame = true;
        }
        else if (frame.load(skinPath + "background.png")) {
            QBitmap border;
            if (border.load(skinPath + "mask.png")) {
                frame.setMask(border);
            }
            if (border.load(skinPath + "alpha.png")) {
                QPainter surface(&frame);
                surface.drawPixmap(0, 0, border);
            }
            validFrame = true;
        }
    }

    if (!validFrame) {
        // Set the background image
        if (frame.load(skinPath + "background_nc.png")) {
            validFrame = true;

            // Set the background mask
            QBitmap mask;
            if (mask.load(skinPath + "mask_nc.png")) {
                // For some reason, w/ compiz setmask won't work
                // for rectangular areas. This is due to compiz and
                // XShapeCombineMask
                setMask(mask);
            }
        }
    }

    if (QFile::exists(skinPath + "spinner.gif")) {
        m_workingAnimation->LoadAnimation(skinPath + "spinner.gif");
    }

    if (validFrame) {
        m_frameGraphic.swap(frame);
        resize(m_frameGraphic.size());
    }
    else {
        m_frameGraphic.fill(Qt::transparent);
    }

    // output size may change when skin change
    updateOutputSize();

    // separator may change when skin change
    InputDataList::setSeparator(m_inputBox->separatorText());
}

void LaunchyWidget::mousePressEvent(QMouseEvent* event) {

    if (event->buttons() == Qt::LeftButton
        && (!g_settings->value(OPTION_DRAGMODE, OPTION_DRAGMODE_DEFAULT).toBool()
            || (event->modifiers() & Qt::ShiftModifier))) {

        m_dragging = true;
        m_dragStartPos = event->pos();
        m_dragStartGlobalPos = event->globalPos();

        qDebug() << "LaunchyWidget::mousePressEvent, drag begin, global pos:"
            << m_dragStartGlobalPos << "pos:" << m_dragStartPos;
    }

    hideAlternativeList();
    activateWindow();
    m_inputBox->setFocus();
}

void LaunchyWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging && event->buttons() == Qt::LeftButton) {

        hideAlternativeList();

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QPoint posTarget = event->globalPos() - m_dragStartPos;
#else
        QPoint posTarget = event->globalPosition().toPoint() - m_dragStartPos;
#endif

        move(posTarget);
        updatePlacementRatio();

        m_inputBox->setFocus();
    }
}

void LaunchyWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (m_dragging == false) {
        return;
    }

    m_dragging = false;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QPoint posTarget = event->globalPos();
#else
    QPoint posTarget = event->globalPosition().toPoint();
#endif

    qDebug() << "LaunchyWidget::mouseReleaseEvent, drag end, global pos:" << posTarget;

    hideAlternativeList();
    m_inputBox->setFocus();

    int prevScreenIndex = -1;
    int currScreenIndex = -1;

    QList<QScreen*> listScreens = qApp->screens();
    for (int i = 0; i < listScreens.size(); ++i) {
        QScreen* screen = listScreens.at(i);

        qDebug() << "LaunchyWidget::mouseReleaseEvent, screen:" << i
            << " geometry:" << screen->geometry();

        if (prevScreenIndex == -1 && screen->geometry().contains(m_dragStartGlobalPos)) {
            prevScreenIndex = i;
        }

        if (currScreenIndex == -1 && screen->geometry().contains(posTarget)) {
            currScreenIndex = i;
        }

        if (prevScreenIndex >= 0 && currScreenIndex >= 0) {
            qDebug() << "LaunchyWidget::mouseReleaseEvent, prev screen:" << prevScreenIndex
                << "curr screen:" << currScreenIndex;
            if (prevScreenIndex != currScreenIndex) {
                reloadSkin();
            }
            break;
        }
    }
}

void LaunchyWidget::contextMenuEvent(QContextMenuEvent* event) {
    qDebug() << "LaunchyWidget::contextMenuEvent, global pos:"
        << event->globalPos() << "pos:" << event->pos();

    QMenu menu(this);
    menu.addAction(m_actRebuild);
    menu.addAction(m_actReloadSkin);
    menu.addAction(m_actOptions);
    menu.addSeparator();
    menu.addAction(m_actExit);
    m_menuOpen = true;
    menu.exec(event->globalPos());
    m_menuOpen = false;
}

void LaunchyWidget::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        // retranslate ui form (single inheritance approach)
        retranslateUi();
    }

    // call base class implementation
    QWidget::changeEvent(event);
}

void LaunchyWidget::focusLaunchy() {
    activateWindow();
    raise();
}

void LaunchyWidget::trayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    switch (reason) {
    case QSystemTrayIcon::Trigger:
        showLaunchy();
        break;
    case QSystemTrayIcon::Unknown:
    case QSystemTrayIcon::Context:
    case QSystemTrayIcon::DoubleClick:
    case QSystemTrayIcon::MiddleClick:
        break;
    default:
        break;
    }
}

void LaunchyWidget::scheduledBuildCatalog() {
    // Only scheduled rebuilds are gated by the idle condition, manual
    // requests (F5, tray menu, options dialog, startup rescan) call
    // buildCatalog() directly and always rebuild immediately
    const int idleSeconds = g_app->systemIdleSeconds();
    if (idleSeconds < 0 || idleSeconds >= SCHEDULED_REBUILD_MIN_IDLE_SECONDS) {
        // system has been idle long enough, or the platform can not
        // report idle time, rebuild the catalog now
        buildCatalog();
        return;
    }

    // the system is still in use, check again in one minute
    qDebug() << "LaunchyWidget::scheduledBuildCatalog, system idle for"
             << idleSeconds << "seconds, rebuild catalog postponed";
    m_rebuildTimer->start(SCHEDULED_REBUILD_CHECK_INTERVAL);
}

void LaunchyWidget::buildCatalog() {
    m_rebuildTimer->stop();

    // Ask the worker thread to rebuild the catalog; requests received
    // while a rebuild is running are ignored by the builder
    g_builder->requestBuild();

    startRebuildTimer();
}

void LaunchyWidget::showOptionDialog() {
    if (!m_optionsOpen) {
        showLaunchy(true);
        m_optionsOpen = true;

        if (!m_optionDialog) {
            m_optionDialog = new OptionDialog(nullptr);
        }

        // move to selected screen center
        int nScreenIndex = g_settings->value(OPTION_SCREEN_INDEX, OPTION_SCREEN_INDEX_DEFAULT).toInt();
        // screenAt() returns nullptr when the cursor is not over any screen
        // (monitor unplugged, dialog anchored outside, ...), validated by screenAtIndex().
        QScreen* screen = screenAtIndex(nScreenIndex);
        // Same guard as loadPosition(): a monitor the user picked may be gone,
        // and centering the dialog on its stale geometry would leave the dialog
        // off screen or in a corner of the display that is still around.
        if (screen) {
            const QPoint posDialogCenter = m_optionDialog->geometry().center();
            if (nScreenIndex > 0
                && !screen->geometry().contains(posDialogCenter)) {
                if (QScreen* screenHere = screenAtPoint(posDialogCenter)) {
                    screen = screenHere;
                }
            }
        }
        if (!screen) {
            qWarning() << "LaunchyWidget::showOptionDialog, no screen available, keeping dialog position";

            delete m_optionDialog;
            m_optionDialog = nullptr;
            m_optionsOpen = false;
            return;
        }

        QRect rectScreenGeometry = screen->geometry();
        QRect rectDialogGeometry = m_optionDialog->geometry();

        qDebug() << "LaunchyWidget::showOptionDialog, screen:" << rectScreenGeometry
            << "dialog:" << rectDialogGeometry;

        // dialog target position
        QPoint posDialog;
        posDialog.setX(rectScreenGeometry.x() + rectScreenGeometry.width() / 2 - rectDialogGeometry.width() / 2);
        posDialog.setY(rectScreenGeometry.y() + rectScreenGeometry.height() / 2 - rectDialogGeometry.height() / 2);

        m_optionDialog->move(posDialog);

        m_optionDialog->exec();

        delete m_optionDialog;
        m_optionDialog = nullptr;

        activateWindow();
        m_inputBox->setFocus();
        m_inputBox->selectAll();
        m_optionsOpen = false;
    }
}

void LaunchyWidget::setFadeLevel(double level) {
    level = qMin(level, 1.0);
    level = qMax(level, 0.0);
    setWindowOpacity(level);
    m_alternativeList->setWindowOpacity(level);
    if (level <= 0.001) {
        hide();
    }
    else if (!isVisible()) {
        show();
        raise();
        activateWindow();
        m_inputBox->setFocus();
        m_inputBox->selectAll();
    }
}

void LaunchyWidget::showLaunchy(bool noFade) {

    hideAlternativeList();

    loadPosition(pos());

    m_fader->fadeIn(noFade || m_alwaysShowLaunchy);

    focusLaunchy();

    m_inputBox->selectAll();
    m_inputBox->setFocus();

    // Let the plugins know
    PluginHandler::instance().showLaunchy();
}

void LaunchyWidget::hideLaunchy(bool noFade) {
    if (!isVisible() || isHidden()) {
        return;
    }

    savePosition();
    hideAlternativeList();
    if (m_alwaysShowLaunchy) {
        return;
    }

    if (isVisible()) {
        m_fader->fadeOut(noFade);
    }

    // let the plugins know
    PluginHandler::instance().hideLaunchy();
}

int LaunchyWidget::getHotkey() const {
    int hotkey = g_settings->value(OPTION_HOTKEY, -1).toInt();
    if (hotkey == -1) {
        hotkey = g_settings->value(OPTION_HOTKEYMOD, OPTION_HOTKEYMOD_DEFAULT).toInt()
            | g_settings->value(OPTION_HOTKEYKEY, OPTION_HOTKEYKEY_DEFAULT).toInt();
    }
    return hotkey;
}

void LaunchyWidget::createActions() {
    m_actShow = new QAction(tr("Show Launchy"), this);
    connect(m_actShow, &QAction::triggered, this, &LaunchyWidget::showLaunchy);

    m_actReloadSkin = new QAction(tr("Reload skin"), this);
    m_actReloadSkin->setShortcut(QKeySequence(Qt::Key_F5 | Qt::SHIFT));
    connect(m_actReloadSkin, &QAction::triggered, this, &LaunchyWidget::reloadSkin);
    addAction(m_actReloadSkin);

    m_actRebuild = new QAction(tr("Rebuild catalog"), this);
    m_actRebuild->setShortcut(QKeySequence(Qt::Key_F5));
    connect(m_actRebuild, &QAction::triggered, this, &LaunchyWidget::buildCatalog);
    addAction(m_actRebuild);

    m_actOptions = new QAction(tr("Options"), this);
    m_actOptions->setShortcut(QKeySequence(Qt::Key_Comma | Qt::CTRL));
    connect(m_actOptions, &QAction::triggered, this, &LaunchyWidget::showOptionDialog);
    addAction(m_actOptions);

    m_actCheckUpdate = new QAction(tr("Check for updates"), this);
    connect(m_actCheckUpdate, &QAction::triggered, []() {
        UpdateChecker::instance().manualCheck();
    });

    m_actRestart = new QAction(tr("Relaunch"), this);
    connect(m_actRestart, &QAction::triggered, [=]() {
        qInfo() << "Performing application relaunch...";
        // restart:
        //qApp->closeAllWindows();
        hideTrayIcon();
        qApp->exit(Restart);
        qInfo() << "Finish application relaunch...";
    });

    m_actExit = new QAction(tr("Exit"), this);
    connect(m_actExit, &QAction::triggered,
            this, &LaunchyWidget::exit, Qt::QueuedConnection);
}

} // namespace launchy
