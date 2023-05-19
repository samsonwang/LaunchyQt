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

namespace launchy {

// for qt flags
// check this page https://stackoverflow.com/questions/10755058/qflags-enum-type-conversion-fails-all-of-a-sudden
using ::operator|;

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
      m_alwaysShowLaunchy(false),
      m_dragging(false),
      m_menuOpen(false),
      m_optionDialog(nullptr),
      m_optionsOpen(false) {

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
    connect(m_inputBox, SIGNAL(keyPressed(QKeyEvent*)),
            this, SLOT(onInputBoxKeyPressed(QKeyEvent*)));
    connect(m_inputBox, SIGNAL(focusOut()),
            this, SLOT(onInputBoxFocusOut()));
    connect(m_inputBox, SIGNAL(inputMethod(QInputMethodEvent*)),
            this, SLOT(onInputBoxInputMethod(QInputMethodEvent*)));

    m_outputBox->setObjectName("output");
    m_outputBox->setAlignment(Qt::AlignHCenter);

    m_outputIcon->setObjectName("outputIcon");
    m_outputIcon->setGeometry(QRect());

    m_alternativeList->setObjectName("alternatives");
    setAlternativeListMode(g_settings->value(OPTION_CONDENSEDVIEW, OPTION_CONDENSEDVIEW_DEFAULT).toInt());
    connect(m_alternativeList, SIGNAL(currentRowChanged(int)),
            this, SLOT(onAlternativeListRowChanged(int)));
    connect(m_alternativeList, SIGNAL(keyPressed(QKeyEvent*)),
            this, SLOT(onAlternativeListKeyPressed(QKeyEvent*)));
    connect(m_alternativeList, SIGNAL(focusOut()),
            this, SLOT(onAlternativeListFocusOut()));

    m_optionButton->setObjectName("opsButton");
    m_optionButton->setToolTip(tr("Options"));
    m_optionButton->setGeometry(QRect());
    connect(m_optionButton, SIGNAL(clicked()),
            this, SLOT(showOptionDialog()));

    m_closeButton->setObjectName("closeButton");
    m_closeButton->setToolTip(tr("Close"));
    m_closeButton->setGeometry(QRect());
    connect(m_closeButton, SIGNAL(clicked()),
            qApp, SLOT(quit()));

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

    connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

    if (g_settings->value(OPTION_HIDE_TRAY_ICON, OPTION_HIDE_TRAY_ICON_DEFAULT).toBool()) {
        hideTrayIcon();
    }
    else {
        showTrayIcon();
    }

    connect(m_fader, SIGNAL(fadeLevel(double)), this, SLOT(setFadeLevel(double)));

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
    connect(g_builder, SIGNAL(catalogIncrement(int)), this, SLOT(catalogProgressUpdated(int)));
    connect(g_builder, SIGNAL(catalogFinished()), this, SLOT(catalogBuilt()));

    if (!g_catalog->load(SettingsManager::instance().catalogFilename())) {
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

    // Set the timers
    m_dropTimer->setSingleShot(true);
    connect(m_dropTimer, SIGNAL(timeout()), this, SLOT(dropTimeout()));

    m_rebuildTimer->setSingleShot(true);
    connect(m_rebuildTimer, SIGNAL(timeout()), this, SLOT(buildCatalog()));
    startRebuildTimer();

    // start update checker
    UpdateChecker::instance().startup();

    // Load the plugins
    PluginHandler::instance().loadPlugins();

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
        QRect r = geometry();
        auto screen = qApp->screenAt(r.topLeft());
        QRect scr = screen->availableGeometry();
        QPoint pt(scr.width()/2 - r.width()/2, scr.height()/2 - r.height()/2);
        move(pt);
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
            for (int i = 1; i < inputData.count(); ++i) {
                args += inputData[i].getText() + " ";
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
    int historyIndex = (int)(int64_t)(item.data);
    qDebug() << "LaunchyWidget::onAlternativeListRowChanged, row:" << row
             << ", item.fullpath:" << item.fullPath
             << ", item.shortName:" << item.shortName
             << ", item.pluginName:" << item.pluginName
             << ", historyIndex:" << historyIndex
             << ", inputBox:" << m_inputBox->text();

    if ( (!m_inputData.isEmpty() && m_inputData.first().hasLabel(LABEL_HISTORY))
         || m_inputBox->text().isEmpty() ) {
        // Used a void* to hold an int.. ick!
        // BUT! Doing so avoids breaking existing catalogs

        if (item.pluginName == NAME_HISTORY && historyIndex < m_searchResult.count()) {
            qDebug() << "LaunchyWidget::onAlternativeListRowChanged, list history"
                     << item.shortName;

            m_inputData = m_history.getItem(historyIndex);
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
                // Delete selected history entry from the alternatives list
                qDebug() << "LaunchyWidget::onAlternativeListKeyPressed,"
                         << "delete history:" << item.shortName;
                m_history.removeAt(row);
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
        processInput();
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

void LaunchyWidget::processInput() {
    qDebug() << "LaunchyWidget::processInput, inputbox text:" << m_inputBox->text();

    m_inputData.parse(m_inputBox->text());
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
        std::sort(m_searchResult.begin(), m_searchResult.end(), CatLessRef);
        g_catalog->promoteRecentlyUsedItems(searchTextLower, m_searchResult);

        // Finally, if the search text looks like a file or directory name,
        // add any file or directory matches
        if (searchText.contains(QDir::separator())
            || searchText.startsWith("~")
            || (searchText.size() == 2 && searchText[0].isLetter() && searchText[1] == ':')) {
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

void LaunchyWidget::catalogProgressUpdated(int value) {
    if (value == 0) {
        m_workingAnimation->Start();
    }
}

void LaunchyWidget::catalogBuilt() {
    // Save settings and updated catalog, stop the "working" animation
    saveSettings();
    m_workingAnimation->Stop();

    // Now do a search using the updated catalog
    searchOnInput();
    updateOutput();
}

void LaunchyWidget::setSkin(const QString& name) {
    hideLaunchy(true);
    applySkin(name);
    showLaunchy(false);
}

void LaunchyWidget::updateVersion(int oldVersion) {
    if (oldVersion < 249) {
        g_settings->setValue(OPTION_SKIN, OPTION_SKIN_DEFAULT);
    }

    if (oldVersion < LAUNCHY_VERSION) {
        g_settings->setValue(OPTION_VERSION, LAUNCHY_VERSION);
    }
}

void LaunchyWidget::loadPosition(const QPoint& pt) {
    // move to selected screen
    QList<QScreen*> listScreen = QApplication::screens();
    QScreen* screen = nullptr;
    int nScreenIndex = g_settings->value(OPTION_SCREEN_INDEX, OPTION_SCREEN_INDEX_DEFAULT).toInt();
    if (nScreenIndex < listScreen.size()) {
        screen = listScreen.at(nScreenIndex);
    }
    else {
        screen = listScreen.front();
    }

    QRect rtScreen = screen->availableGeometry();
    QRect rtWidget = geometry();

    qDebug() << "LaunchyWidget::loadPosition, pos:" << pt
        << "screen:" << rtScreen << "widget:" << rtWidget;

    QPoint ptCenter = pt + QPoint(rtWidget.width() / 2, rtWidget.height() / 2);

    QPoint ptTarget(pt);
    // See if the new position is within the screen dimensions, if not pull it inside
    if (ptCenter.x() < rtScreen.left()) {
        ptTarget.setX(rtScreen.left());
    }
    else if (ptCenter.x() > rtScreen.right()) {
        ptTarget.setX(rtScreen.right() - rtWidget.width());
    }
    if (ptCenter.y() < rtScreen.top()) {
        ptTarget.setY(rtScreen.top());
    }
    else if (ptCenter.y() > rtScreen.bottom()) {
        ptTarget.setY(rtScreen.bottom() - rtWidget.height());
    }

    int centerOption = g_settings->value(OPTION_ALWAYSCENTER, OPTION_ALWAYSCENTER_DEFAULT).toInt();
    if (centerOption & 1) {
        ptTarget.setX(rtScreen.center().x() - rtWidget.width() / 2);
    }
    if (centerOption & 2) {
        ptTarget.setY(rtScreen.center().y() - rtWidget.height() / 2);
    }

    move(ptTarget);
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

void LaunchyWidget::trayNotify(const QString& infoMsg) {
    m_trayIcon->showMessage(tr("Launchy"), infoMsg,
                            QIcon(":/resources/launchy128.png"));
}

void LaunchyWidget::onHotkey() {
    qDebug() << "LaunchyWidget::onHotkey,"
             << "m_alwaysShowLaunchy:" << m_alwaysShowLaunchy
             << "isVisible()" << isVisible()
             << "isFading():" << m_fader->isFading()
             << "QApplication::activeWindow():" << QApplication::activeWindow();

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
        processInput();
    }
}

void LaunchyWidget::onInputBoxTextEdited(const QString& str) {
    qDebug() << "LaunchyWidget::onInputBoxTextEdited, str:" << str;
    processInput();
}

void LaunchyWidget::onSecondInstance() {
    trayNotify(tr("Launchy is already running!"));
}

void LaunchyWidget::onScreenChanged(QScreen* screen) {
    qDebug() << "LaunchyWidget::onScreenChanged, screen:" << screen->name();
    // reload screen after screen is changed
    if (isVisible()) {
        reloadSkin();
    }
    else {
        applySkin(m_currentSkin);
    }
}

void LaunchyWidget::applySkin(const QString& name) {
    m_currentSkin = name;
    m_skinChanged = true;

    qDebug() << "apply skin:" << name;

    QString skinPath = SettingsManager::instance().skinPath(name);
    // Use default skin if this one doesn't exist or isn't valid
    if (skinPath.isEmpty()) {
        QString defaultSkin = SettingsManager::instance().directory("defSkin")[0];
        skinPath = SettingsManager::instance().skinPath(defaultSkin);
        // If still no good then fail with an ugly default
        if (skinPath.isEmpty()) {
            return;
        }

        g_settings->setValue(OPTION_SKIN, defaultSkin);
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

void LaunchyWidget::mousePressEvent(QMouseEvent *event) {
    if (event->buttons() == Qt::LeftButton) {
        if (!g_settings->value(OPTION_DRAGMODE, OPTION_DRAGMODE_DEFAULT).toBool()
            || (event->modifiers() & Qt::ShiftModifier)) {
            m_dragging = true;
            m_dragStartPoint = event->pos();
        }
    }
    hideAlternativeList();
    activateWindow();
    m_inputBox->setFocus();
}

void LaunchyWidget::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons() == Qt::LeftButton
        && m_dragging) {

        hideAlternativeList();

        QPoint pt = event->globalPosition().toPoint() - m_dragStartPoint;
        move(pt);

        // qDebug() << "LaunchyWidget::mouseMoveEvent, pos:" << pt;

        m_inputBox->setFocus();
    }
}

void LaunchyWidget::mouseReleaseEvent(QMouseEvent* event) {

    m_dragging = false;

    hideAlternativeList();
    m_inputBox->setFocus();

    int screenIndex = g_settings->value(OPTION_SCREEN_INDEX, OPTION_SCREEN_INDEX_DEFAULT).toInt();

    QPoint pt = event->globalPosition().toPoint();

    QList<QScreen*> listScreens = qApp->screens();
    for (int i = 0; i < listScreens.size(); ++i) {
        QScreen* pScreen = listScreens.at(i);

        if (pScreen->geometry().contains(pt)) {
            g_settings->setValue(OPTION_SCREEN_INDEX, i);

            if (screenIndex != i) {
                reloadSkin();
            }
            break;
        }
    }
}

void LaunchyWidget::contextMenuEvent(QContextMenuEvent* event) {
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
    qApp->setActiveWindow(this);
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

void LaunchyWidget::buildCatalog() {
    m_rebuildTimer->stop();
    saveSettings();

    // Use the catalog builder to refresh the catalog in a worker thread
    // QMetaObject::invokeMethod(g_builder, &CatalogBuilder::buildCatalog);
    QMetaObject::invokeMethod(g_builder, "buildCatalog");

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
        QList<QScreen*> listScreen = QApplication::screens();
        QScreen* pScreen = nullptr;
        int nScreenIndex = g_settings->value(OPTION_SCREEN_INDEX, OPTION_SCREEN_INDEX_DEFAULT).toInt();
        if (nScreenIndex < listScreen.size()) {
            pScreen = listScreen.at(nScreenIndex);
        }
        else {
            pScreen = listScreen.front();
        }

        QRect rectScreenGeometry = pScreen->geometry();
        QRect rectDialogGeometry = m_optionDialog->geometry();

        qDebug() << "LaunchyWidget::showOptionDialog, screen:" << rectScreenGeometry
            << "dialog:" << rectDialogGeometry;

        // dialog target position
        QPoint pointDialog;
        pointDialog.setX(rectScreenGeometry.x() + rectScreenGeometry.width() / 2 - rectDialogGeometry.width() / 2);
        pointDialog.setY(rectScreenGeometry.y() + rectScreenGeometry.height() / 2 - rectDialogGeometry.height() / 2);

        m_optionDialog->move(pointDialog);

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
    connect(m_actShow, SIGNAL(triggered()), this, SLOT(showLaunchy()));

    m_actReloadSkin = new QAction(tr("Reload skin"), this);
    m_actReloadSkin->setShortcut(QKeySequence(Qt::Key_F5 | Qt::SHIFT));
    connect(m_actReloadSkin, SIGNAL(triggered()), this, SLOT(reloadSkin()));
    addAction(m_actReloadSkin);

    m_actRebuild = new QAction(tr("Rebuild catalog"), this);
    m_actRebuild->setShortcut(QKeySequence(Qt::Key_F5));
    connect(m_actRebuild, SIGNAL(triggered()), this, SLOT(buildCatalog()));
    addAction(m_actRebuild);

    m_actOptions = new QAction(tr("Options"), this);
    m_actOptions->setShortcut(QKeySequence(Qt::Key_Comma | Qt::CTRL));
    connect(m_actOptions, SIGNAL(triggered()), this, SLOT(showOptionDialog()));
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
    connect(m_actExit, SIGNAL(triggered()),
            this, SLOT(exit()), Qt::QueuedConnection);
}

} // namespace launchy
