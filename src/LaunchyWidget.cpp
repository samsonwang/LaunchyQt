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

#include "precompiled.h"
#include "LaunchyWidget.h"

#include <QScrollBar>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QMenu>
#ifdef Q_OS_MAC
#include <QMacStyle>
#endif
#include "QHotkey/QHotkey.h"

#include "IconDelegate.h"
#include "GlobalVar.h"
#include "OptionDialog.h"
#include "plugin_interface.h"
#include "FileSearch.h"
#include "SettingsManager.h"
#include "AppBase.h"
#include "Fader.h"
#include "IconDelegate.h"
#include "AnimationLabel.h"
#include "CharListWidget.h"
#include "CharLineEdit.h"
#include "CatalogBuilder.h"

LaunchyWidget::LaunchyWidget(CommandFlags command)
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
    : QWidget(NULL, Qt::FramelessWindowHint | Qt::Tool),
#elif defined(Q_OS_MAC)
    : QWidget(NULL, Qt::FramelessWindowHint),
#endif
      m_skinChanged(false),
      frameGraphic(NULL),
      m_trayIcon(NULL),
      m_alternativeList(NULL),
      updateTimer(NULL),
      dropTimer(NULL),
      m_pHotKey(new QHotkey(this)) {

    g_mainWidget.reset(this);

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

    menuOpen = false;
    optionsOpen = false;
    dragging = false;
    g_searchText = "";

    alwaysShowLaunchy = false;

    connect(&iconExtractor, SIGNAL(iconExtracted(int, QString, QIcon)), 
            this, SLOT(iconExtracted(int, QString, QIcon)));

    fader = new Fader(this);
    connect(fader, SIGNAL(fadeLevel(double)), this, SLOT(setFadeLevel(double)));

    m_optionButton = new QPushButton(this);
    m_optionButton->setObjectName("opsButton");
    m_optionButton->setToolTip(tr("Launchy Options"));
    m_optionButton->setGeometry(QRect());
    connect(m_optionButton, SIGNAL(clicked()), this, SLOT(showOptionDialog()));

    m_closeButton = new QPushButton(this);
    m_closeButton->setObjectName("closeButton");
    m_closeButton->setToolTip(tr("Close Launchy"));
    m_closeButton->setGeometry(QRect());
    connect(m_closeButton, SIGNAL(clicked()), qApp, SLOT(quit()));

    m_outputBox = new QLabel(this);
    m_outputBox->setObjectName("output");
    m_outputBox->setAlignment(Qt::AlignHCenter);

    m_inputBox = new CharLineEdit(this);
#ifdef Q_OS_MAC
    QMacStyle::setFocusRectPolicy(m_inputBox, QMacStyle::FocusDisabled);
#endif
    m_inputBox->setObjectName("input");
    connect(m_inputBox, SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(onInputBoxKeyPressed(QKeyEvent*)));
    //connect(input, SIGNAL(focusIn()), this, SLOT(onInputFocusIn()));
    connect(m_inputBox, SIGNAL(focusOut()), this, SLOT(onInputBoxFocusOut()));
    //connect(m_inputBox, SIGNAL(inputMethod()), this, SLOT(onInputBoxInputMethod()));
    connect(m_inputBox, SIGNAL(textEdited(const QString&)),
            this, SLOT(onInputBoxTextEdited(const QString&)));

    m_outputIcon = new QLabel(this);
    m_outputIcon->setObjectName("outputIcon");
    m_outputIcon->setGeometry(QRect());

    m_workingAnimation = new AnimationLabel(this);
    m_workingAnimation->setObjectName("workingAnimation");
    m_workingAnimation->setGeometry(QRect());

    // If this is the first time running or a new version, call updateVersion
    if (g_settings->value("version", 0).toInt() != LAUNCHY_VERSION) {
        updateVersion(g_settings->value("version", 0).toInt());
        command |= ShowLaunchy;
    }

    m_alternativeList = new CharListWidget(this);
    m_alternativeList->setObjectName("alternatives");
    setAlternativeListMode(g_settings->value("GenOps/condensedView", 2).toInt());
    connect(m_alternativeList, SIGNAL(currentRowChanged(int)), this, SLOT(onAlternativeListRowChanged(int)));
    connect(m_alternativeList, SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(onAlternativeListKeyPressed(QKeyEvent*)));
    //connect(alternatives, SIGNAL(focusOut(QFocusEvent*)), this, SLOT(focusOutEvent(QFocusEvent*)));

    // Load the plugins
    plugins.loadPlugins();

    // Set the general options
    if (setAlwaysShow(g_settings->value("GenOps/alwaysshow", false).toBool()))
        command |= ShowLaunchy;
    setAlwaysTop(g_settings->value("GenOps/alwaystop", false).toBool());

    // Set the hotkey
    QKeySequence hotkey = getHotkey();
    connect(m_pHotKey, &QHotkey::activated, this, &LaunchyWidget::onHotkey);
    if (!setHotkey(hotkey)) {
        QMessageBox::warning(this, tr("Launchy"),
                             tr("The hotkey %1 is already in use, please select another.")
                             .arg(hotkey.toString()));
        command = ShowLaunchy | ShowOptions;
    }

    // Set the timers
    dropTimer = new QTimer(this);
    dropTimer->setSingleShot(true);
    connect(dropTimer, SIGNAL(timeout()), this, SLOT(dropTimeout()));

    updateTimer = new QTimer(this);
    updateTimer->setSingleShot(true);
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(buildCatalog()));
    startUpdateTimer();

    // Load the catalog
    g_builder.reset(new CatalogBuilder(&plugins));
    connect(g_builder.data(), SIGNAL(catalogIncrement(int)), this, SLOT(catalogProgressUpdated(int)));
    connect(g_builder.data(), SIGNAL(catalogFinished()), this, SLOT(catalogBuilt()));

    catalog = g_builder->getCatalog();
    if (!catalog->load(SettingsManager::instance().catalogFilename())) {
        command |= Rescan;
    }

    // Load the history
    history.load(SettingsManager::instance().historyFilename());

    // Load the skin
    //setStyleSheet(":/resources/basicskin.qss");
    applySkin(g_settings->value("GenOps/skin", "Default").toString());

    // Move to saved position
    loadPosition(g_settings->value("Display/pos", QPoint(0, 0)).toPoint());
    
    loadOptions();

    showTrayIcon();
    
    executeStartupCommand(command);

    connect(g_app.data(), &SingleApplication::instanceStarted,
            this, &LaunchyWidget::onSecondInstance);
}

LaunchyWidget::~LaunchyWidget() {

}

void LaunchyWidget::executeStartupCommand(int command) {
    if (command & ResetPosition) {
        QRect r = geometry();
        int primary = qApp->desktop()->primaryScreen();
        QRect scr = qApp->desktop()->availableGeometry(primary);

        QPoint pt(scr.width()/2 - r.width()/2, scr.height()/2 - r.height()/2);
        move(pt);
    }

    if (command & ResetSkin) {
        setOpaqueness(100);
        showTrayIcon();
        applySkin("Default");
    }

    if (command & ShowLaunchy)
        showLaunchy();

    if (command & ShowOptions)
        showOptionDialog();

    if (command & Rescan)
        buildCatalog();

    if (command & Exit)
        close();
}

void LaunchyWidget::showEvent(QShowEvent *event) {
    if (m_skinChanged) {
        // output icon may changed with skin
        int maxIconSize = m_outputIcon->width();
        maxIconSize = qMax(maxIconSize, m_outputIcon->height());
        qDebug() << "output icon size:" << maxIconSize;
        g_app->setPreferredIconSize(maxIconSize);
        m_skinChanged = false;
    }
    QWidget::showEvent(event);
}

void LaunchyWidget::paintEvent(QPaintEvent* event) {
    // Do the default draw first to render any background specified in the stylesheet
    QStyleOption styleOption;
    styleOption.init(this);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    style()->drawPrimitive(QStyle::PE_Widget, &styleOption, &painter, this);

    // Now draw the standard frame.png graphic if there is one
    if (frameGraphic) {
        painter.drawPixmap(0, 0, *frameGraphic);
    }

    QWidget::paintEvent(event);
}

void LaunchyWidget::setAlternativeListMode(int mode) {
    m_alternativeList->setListMode(mode);
}

bool LaunchyWidget::setHotkey(const QKeySequence& hotkey) {
    m_pHotKey->setKeySeq(hotkey);
    return m_pHotKey->registered();
}

void LaunchyWidget::showTrayIcon() {
    if (!m_trayIcon) {
        m_trayIcon = new QSystemTrayIcon(this);
    }

    QKeySequence hotkey = m_pHotKey->keySeq();
    m_trayIcon->setToolTip(tr("Launchy %1\npress %2 to activate")
                         .arg(LAUNCHY_VERSION_STRING)
                         .arg(hotkey.toString()));
    m_trayIcon->setIcon(QIcon(":/resources/launchy16.png"));
    m_trayIcon->show();
    connect(m_trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

    QMenu* trayMenu = new QMenu(this);
    trayMenu->addAction(actShow);
    trayMenu->addAction(actRebuild);
    trayMenu->addAction(actOptions);
    trayMenu->addSeparator();
    trayMenu->addAction(actExit);
    m_trayIcon->setContextMenu(trayMenu);
}


// Repopulate the alternatives list with the current search results
// and set its size and position accordingly.
void LaunchyWidget::updateAlternativeList(bool resetSelection) {
    int mode = g_settings->value("GenOps/condensedView", 2).toInt();
    int i = 0;
    for (; i < searchResults.size(); ++i) {
        qDebug() << "Alternative" << i << ":" << searchResults[i].fullPath;
        QString fullPath = QDir::toNativeSeparators(searchResults[i].fullPath);
#ifdef _DEBUG
        fullPath += QString(" (%1 launches)").arg(searchResults[i].usage);
#endif
        QListWidgetItem* item;
        if (i < m_alternativeList->count()) {
            item = m_alternativeList->item(i);
        }
        else {
            item = new QListWidgetItem(fullPath, m_alternativeList);
        }
        if (item->data(mode == 1 ? ROLE_SHORT : ROLE_FULL) != fullPath) {
            // condensedTempIcon is a blank icon or null
            item->setData(ROLE_ICON, QIcon());
        }
        item->setData(mode == 1 ? ROLE_FULL : ROLE_SHORT, searchResults[i].shortName);
        item->setData(mode == 1 ? ROLE_SHORT : ROLE_FULL, fullPath);
        if (i >= m_alternativeList->count())
            m_alternativeList->addItem(item);
    }

    while (m_alternativeList->count() > i) {
        delete m_alternativeList->takeItem(i);
    }

    if (resetSelection) {
        m_alternativeList->setCurrentRow(0);
    }
    iconExtractor.processIcons(searchResults);

    m_alternativeList->updateGeometry(pos(), m_inputBox->pos());
}


void LaunchyWidget::showAlternativeList() {
    // Ensure that any pending shows of the alternatives list are cancelled
    // so that we only update the list once.
    dropTimer->stop();

    m_alternativeList->show();
    m_alternativeList->setFocus();
}


void LaunchyWidget::hideAlternativeList() {
    // Ensure that any pending shows of the alternatives list are cancelled
    // so that the list isn't erroneously shown shortly after being dismissed.
    dropTimer->stop();

    // clear the selection before hiding to prevent flicker
    m_alternativeList->setCurrentRow(-1);
    m_alternativeList->repaint();
    m_alternativeList->hide();
    iconExtractor.stop();
}


void LaunchyWidget::launchItem(CatItem& item) {
    int ops = MSG_CONTROL_LAUNCHITEM;

    if (item.id != HASH_LAUNCHY && item.id != HASH_LAUNCHYFILE) {
        ops = plugins.execute(&inputData, &item);
        switch (ops) {
        case MSG_CONTROL_EXIT:
            close();
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

    if (ops == MSG_CONTROL_LAUNCHITEM) {
        QString args = "";
        if (inputData.count() > 1)
            for (int i = 1; i < inputData.count(); ++i)
                args += inputData[i].getText() + " ";
        runProgram(item.fullPath, args);
    }

    catalog->incrementUsage(item);
    history.addItem(inputData);
}

/*
void LaunchyWidget::focusInEvent(QFocusEvent* event) {
    if (event->gotFocus() && fader->isFading())
        fader->fadeIn(false);

    QWidget::focusInEvent(event);
}

void LaunchyWidget::focusOutEvent(QFocusEvent* event) {
    Qt::FocusReason reason = event->reason();
    if (event->reason() == Qt::ActiveWindowFocusReason) {
        if (g_settings->value("GenOps/hideiflostfocus", false).toBool()
            && !isActiveWindow()
            && !alternatives->isActiveWindow()
            && !optionsOpen
            && !fader->isFading()) {
            hideLaunchy();
        }
    }
}
*/

void LaunchyWidget::onAlternativeListRowChanged(int index)
{
    // Check that index is a valid history item index
    // If the current entry is a history item or there is no text entered
    if (index >= 0 && index < searchResults.count())
    {
        CatItem item = searchResults[index];
        if ((inputData.count() > 0 && inputData.first().hasLabel(LABEL_HISTORY)) || m_inputBox->text().length() == 0)
        {
            // Used a void* to hold an int.. ick!
            // BUT! Doing so avoids breaking existing catalogs
            long long hi = reinterpret_cast<long long>(item.data);
            int historyIndex = static_cast<int>(hi);

            if (item.id == HASH_HISTORY && historyIndex < searchResults.count())
            {
                inputData = history.getItem(historyIndex);
                m_inputBox->selectAll();
                m_inputBox->insert(inputData.toString());
                m_inputBox->selectAll();
                m_outputBox->setText(inputData[0].getTopResult().shortName);
                // No need to fetch the icon again, just grab it from the alternatives row
                m_outputIcon->setPixmap(m_alternativeList->item(index)->icon().pixmap(m_outputIcon->size()));
                outputItem = item;
                g_searchText = inputData.toString();
            }
        }
        else if (inputData.count() > 0 &&
            (inputData.last().hasLabel(LABEL_AUTOSUGGEST) || inputData.last().hasText() == 0))
        {
            qDebug() << "Autosuggest" << item.shortName;

            inputData.last().setText(item.shortName);
            inputData.last().setLabel(LABEL_AUTOSUGGEST);

            QString root = inputData.toString(true);
            m_inputBox->selectAll();
            m_inputBox->insert(root + item.shortName);
            m_inputBox->setSelection(root.length(), item.shortName.length());

            m_outputBox->setText(item.shortName);
            // No need to fetch the icon again, just grab it from the alternatives row
            m_outputIcon->setPixmap(m_alternativeList->item(index)->icon().pixmap(m_outputIcon->size()));
            outputItem = item;
            g_searchText = "";
        }
    }
}


void LaunchyWidget::onInputBoxKeyPressed(QKeyEvent* event) {
    if (event->key() == Qt::Key_Tab) {
        qDebug() << "LaunchyWidget::onInputBoxKeyPressed,"
            << "pass event to LaunchyWidget::keyPressEvent";
        keyPressEvent(event);
    }
    else {
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
        if (searchResults.count() > 0) {
            int row = m_alternativeList->currentRow();
            if (row > -1) {
                QString location = "History/" + m_inputBox->text();
                QStringList hist;
                hist << searchResults[row].lowName << searchResults[row].fullPath;
                g_settings->setValue(location, hist);

                if (row > 0)
                    searchResults.move(row, 0);

                if (event->key() == Qt::Key_Tab) {
                    doTab();
                    processKey();
                }
                else {
                    // Load up the inputData properly before running the command
                    /* commented out until I find a fix for it breaking the history selection
                    inputData.last().setTopResult(searchResults[0]);
                    doTab();
                    inputData.parse(input->text());
                    inputData.erase(inputData.end() - 1);*/

                    updateOutputBox();
                    keyPressEvent(event);
                }
            }
        }
    }
    else if (event->key() == Qt::Key_Delete
             && (event->modifiers() & Qt::ShiftModifier) != 0) {
        int row = m_alternativeList->currentRow();
        if (row > -1) {
            if (searchResults[row].id == HASH_HISTORY) {
                // Delete selected history entry from the alternatives list
                history.removeAt(row);
                m_inputBox->clear();
                searchOnInput();
                updateAlternativeList(false);
                onAlternativeListRowChanged(m_alternativeList->currentRow());
            }
            else {
                // Demote the selected item down the alternatives list
                catalog->demoteItem(searchResults[row]);
                searchOnInput();
                updateOutputBox(false);
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


void LaunchyWidget::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        if (m_alternativeList->isVisible())
            hideAlternativeList();
        else
            hideLaunchy();
    }

    else if (event->key() == Qt::Key_Return
             || event->key() == Qt::Key_Enter) {
        doEnter();
    }

    else if (event->key() == Qt::Key_Down
             || event->key() == Qt::Key_PageDown
             || event->key() == Qt::Key_Up
             || event->key() == Qt::Key_PageUp) {
        if (m_alternativeList->isVisible()) {
            if (!m_alternativeList->isActiveWindow()) {
                // Don't refactor the activateWindow outside the if, it won't work properly any other way!
                if (m_alternativeList->currentRow() < 0 && m_alternativeList->count() > 0) {
                    m_alternativeList->activateWindow();
                    m_alternativeList->setCurrentRow(0);
                }
                else {
                    m_alternativeList->activateWindow();
                    qApp->sendEvent(m_alternativeList, event);
                }
            }
        }
        else if (event->key() == Qt::Key_Down
                 || event->key() == Qt::Key_PageDown) {
            // do a search and show the results, selecting the first one
            searchOnInput();
            if (searchResults.count() > 0) {
                updateAlternativeList();
                showAlternativeList();
            }
        }
    }
    else if ((event->key() == Qt::Key_Tab
              || event->key() == Qt::Key_Backspace)
             && event->modifiers() == Qt::ShiftModifier) {
        doBackTab();
        processKey();
    }
    else if (event->key() == Qt::Key_Tab) {
        doTab();
        processKey();
    }
    else if (event->key() == Qt::Key_Slash
             || event->key() == Qt::Key_Backslash) {
        if (inputData.count() > 0 && inputData.last().hasLabel(LABEL_FILE) &&
            searchResults.count() > 0 && searchResults[0].id == HASH_LAUNCHYFILE)
            doTab();
        processKey();
    }
    else if (event->key()== Qt::Key_Insert
             && event->modifiers() == Qt::ShiftModifier) {
        // ensure pasting text with Shift+Insert also parses input
        // longer term parsing should be done using the TextChanged event
        processKey();
    }
}

// remove input text back to the previous input section
void LaunchyWidget::doBackTab()
{
    QString text = m_inputBox->text();
    int index = text.lastIndexOf(m_inputBox->separatorText());
    if (index >= 0)
    {
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
    else
    {
        m_inputBox->clear();
    }
}


void LaunchyWidget::doTab()
{
    if (inputData.count() > 0 && searchResults.count() > 0)
    {
        // If it's an incomplete file or directory, complete it
        QFileInfo info(searchResults[0].fullPath);

        if (inputData.last().hasLabel(LABEL_FILE) || info.isDir())
        {
            QString path;
            if (info.isSymLink())
                path = info.symLinkTarget();
            else
                path = searchResults[0].fullPath;

            if (info.isDir() && !path.endsWith(QDir::separator()))
                path += QDir::separator();

            m_inputBox->selectAll();
            m_inputBox->insert(inputData.toString(true) + QDir::toNativeSeparators(path));
        }
        else {
            inputData.last().setTopResult(searchResults[0]);
            inputData.last().setText(searchResults[0].shortName);
            m_inputBox->selectAll();
            m_inputBox->insert(inputData.toString() + m_inputBox->separatorText());
        }
    }
}


void LaunchyWidget::doEnter() {
    hideAlternativeList();

    if ((inputData.count() > 0 && searchResults.count() > 0)
        || inputData.count() > 1) {
        CatItem& item = inputData[0].getTopResult();
        qDebug() << "Launching" << item.shortName << ":" << item.fullPath;
        launchItem(item);
        hideLaunchy();
    }
    else {
        qDebug("Nothing to launch");
    }
}

void LaunchyWidget::inputMethodEvent(QInputMethodEvent* event) {
    Q_UNUSED(event)
    processKey();
}

void LaunchyWidget::processKey() {
    qDebug() << "LaunchyWidget::processKey";
    inputData.parse(m_inputBox->text());
    searchOnInput();
    updateOutputBox();

    // If there is no input text, ensure that the alternatives list is hidden
    // otherwise, show it after the user defined delay if it's not currently visible
    if (m_inputBox->text().isEmpty()) {
        hideAlternativeList();
    }
    else if (!m_alternativeList->isVisible()) {
        startDropTimer();
    }
}

void LaunchyWidget::searchOnInput() {
    if (catalog == NULL)
        return;

    QString searchText = inputData.count() > 0 ? inputData.last().getText() : "";
    QString searchTextLower = searchText.toLower();
    g_searchText = searchTextLower;
    searchResults.clear();

    if ((inputData.count() > 0 && inputData.first().hasLabel(LABEL_HISTORY))
        || m_inputBox->text().length() == 0) {
        // Add history items exclusively and unsorted so they remain in most recently used order
        qDebug() << "Searching history for" << searchText;
        history.search(searchTextLower, searchResults);
    }
    else {
        // Search the catalog for matching items
        if (inputData.count() == 1) {
            qDebug() << "Searching catalog for" << searchText;
            catalog->searchCatalogs(searchTextLower, searchResults);
        }

        if (searchResults.count() != 0)
            inputData.last().setTopResult(searchResults[0]);

        // Give plugins a chance to add their own dynamic matches
        plugins.getLabels(&inputData);
        plugins.getResults(&inputData, &searchResults);

        // Sort the results by match and usage, then promote any that match previously
        // executed commands
        qSort(searchResults.begin(), searchResults.end(), CatLessNoPtr);
        catalog->promoteRecentlyUsedItems(searchTextLower, searchResults);

        // Finally, if the search text looks like a file or directory name,
        // add any file or directory matches
        if (searchText.contains(QDir::separator())
            || searchText.startsWith("~")
            || (searchText.size() == 2 && searchText[0].isLetter() && searchText[1] == ':')) {
            FileSearch::search(searchText, searchResults, inputData);
        }
    }
}


// If there are current results, update the output text and icon
void LaunchyWidget::updateOutputBox(bool resetAlternativesSelection) {
    if (searchResults.count() > 0 
        && (inputData.count() > 1
            || m_inputBox->text().length() > 0)) {
        qDebug() << "Setting output text to" << searchResults[0].shortName;

        QString outputText = Catalog::decorateText(searchResults[0].shortName, g_searchText, true);
#ifdef _DEBUG
        outputText += QString(" (%1 launches)").arg(searchResults[0].usage);
#endif
        m_outputBox->setText(outputText);
        if (outputItem != searchResults[0]) {
            outputItem = searchResults[0];
            m_outputIcon->clear();
            iconExtractor.processIcon(searchResults[0], true);
        }

        if (outputItem.id != HASH_HISTORY) {
            // Did the plugin take control of the input?
            if (inputData.last().getID() != 0)
                outputItem.id = inputData.last().getID();
            inputData.last().setTopResult(searchResults[0]);
        }

        // Only update the alternatives list if it is visible
        if (m_alternativeList->isVisible()) {
            updateAlternativeList(resetAlternativesSelection);
        }
    }
    else {
        // No results to show, clear the output UI and hide the alternatives list
        m_outputBox->clear();
        m_outputIcon->clear();
        outputItem = CatItem();
        hideAlternativeList();
    }
}

void LaunchyWidget::startDropTimer() {
    int delay = g_settings->value("GenOps/autoSuggestDelay", 1000).toInt();
    if (delay > 0)
        dropTimer->start(delay);
    else
        dropTimeout();
}

void LaunchyWidget::dropTimeout() {
    // Don't do anything if Launchy has been hidden since the timer was started
    if (isVisible() && searchResults.count() > 0) {
        updateAlternativeList();
        showAlternativeList();
    }
}

void LaunchyWidget::iconExtracted(int itemIndex, QString path, QIcon icon) {
    if (itemIndex == -1) {
        // An index of -1 means update the output icon, check that it is also
        // the same item as was originally requested
        if (path == outputItem.fullPath) {
            m_outputIcon->setPixmap(icon.pixmap(m_outputIcon->size()));
        }
    }
    else if (itemIndex < m_alternativeList->count()) {
        // >=0 is an item in the alternatives list
        if (itemIndex < searchResults.count()
            && path == searchResults[itemIndex].fullPath) {
            QListWidgetItem* listItem = m_alternativeList->item(itemIndex);
            listItem->setIcon(icon);
            listItem->setData(ROLE_ICON, icon);

            QRect rect = m_alternativeList->visualItemRect(listItem);
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
    updateOutputBox();
}

void LaunchyWidget::setSkin(const QString& name) {
    hideLaunchy(true);
    applySkin(name);
    showLaunchy(false);
}

void LaunchyWidget::updateVersion(int oldVersion) {
    if (oldVersion < 199) {
        SettingsManager::instance().removeAll();
        SettingsManager::instance().load();
    }

    if (oldVersion < 249) {
        g_settings->setValue("GenOps/skin", "Default");
    }

    if (oldVersion < LAUNCHY_VERSION) {
        g_settings->setValue("donateTime", QDateTime::currentDateTime().addDays(21));
        g_settings->setValue("version", LAUNCHY_VERSION);
    }
}

void LaunchyWidget::loadPosition(QPoint pt) {
    // Get the dimensions of the screen containing the new center point
    QRect rect = geometry();
    QPoint newCenter = pt + QPoint(rect.width()/2, rect.height()/2);
    QRect screen = qApp->desktop()->availableGeometry(newCenter);

    // See if the new position is within the screen dimensions, if not pull it inside
    if (newCenter.x() < screen.left())
        pt.setX(screen.left());
    else if (newCenter.x() > screen.right())
        pt.setX(screen.right()-rect.width());
    if (newCenter.y() < screen.top())
        pt.setY(screen.top());
    else if (newCenter.y() > screen.bottom())
        pt.setY(screen.bottom()-rect.height());

    int centerOption = g_settings->value("GenOps/alwayscenter", 3).toInt();
    if (centerOption & 1)
        pt.setX(screen.center().x() - rect.width()/2);
    if (centerOption & 2)
        pt.setY(screen.center().y() - rect.height()/2);

    move(pt);
}

void LaunchyWidget::savePosition() {
    g_settings->setValue("Display/pos", pos());
}

void LaunchyWidget::saveSettings() {
    qDebug() << "Save settings";
    savePosition();
    g_settings->sync();
    catalog->save(SettingsManager::instance().catalogFilename());
    history.save(SettingsManager::instance().historyFilename());
}

void LaunchyWidget::startUpdateTimer() {
    int time = g_settings->value("GenOps/updatetimer", 10).toInt();
    if (time != 0)
        updateTimer->start(time * 60000);
    else
        updateTimer->stop();
}

void LaunchyWidget::onHotkey() {
    if (menuOpen || optionsOpen) {
        showLaunchy(true);
        return;
    }
    if (!alwaysShowLaunchy
        && isVisible()
        && !fader->isFading()
        && QApplication::activeWindow() !=0) {
        hideLaunchy();
    }
    else {
        showLaunchy();
    }
}

void LaunchyWidget::closeEvent(QCloseEvent* event) {
    event->ignore();
    hideLaunchy();
}

bool LaunchyWidget::setAlwaysShow(bool alwaysShow) {
    alwaysShowLaunchy = alwaysShow;
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
    double value = level / 100.0;
    setWindowOpacity(value);
    m_alternativeList->setWindowOpacity(value);
}

void LaunchyWidget::reloadSkin() {
    setSkin(currentSkin);
}

void LaunchyWidget::exit() {
    fader->stop();
    saveSettings();
    qApp->quit();
}

void LaunchyWidget::onInputBoxFocusOut() {
    if (g_settings->value("GenOps/hideiflostfocus", false).toBool()
        && !isActiveWindow()
        && !m_alternativeList->isActiveWindow()
        && !optionsOpen
        && !fader->isFading()) {
        hideLaunchy();
    }
}

void LaunchyWidget::onInputBoxInputMethod() {
    qDebug() << "LaunchyWidget::onInputBoxInputMethod";
    processKey();
}

void LaunchyWidget::onInputBoxTextEdited(const QString& str) {
    qDebug() << "LaunchyWidget::onInputBoxTextEdited, str:" << str;
    processKey();
}

void LaunchyWidget::onSecondInstance() {
    if (m_trayIcon) {
        m_trayIcon->showMessage(tr("Launchy"), tr("Launchy is already running!"),
                              QIcon(":/resources/launchy128.png"));
    }
}

void LaunchyWidget::applySkin(const QString& name) {
    currentSkin = name;
    m_skinChanged = true;

    qDebug() << "apply skin:" << name;

    QString skinPath = SettingsManager::instance().skinPath(name);
    // Use default skin if this one doesn't exist or isn't valid
    if (skinPath.isEmpty()) {
        QString defaultSkin = SettingsManager::instance().directory("defSkin")[0];
        skinPath = SettingsManager::instance().skinPath(defaultSkin);
        // If still no good then fail with an ugly default
        if (skinPath.isEmpty())
            return;

        g_settings->setValue("GenOps/skin", defaultSkin);
    }

    // Set a few defaults
    delete frameGraphic;
    frameGraphic = NULL;

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
    this->setStyleSheet(strStyleSheet);

    bool validFrame = false;
    QPixmap frame;
    if (g_app->supportsAlphaBorder()) {
        if (frame.load(skinPath + "frame.png")) {
            validFrame = true;
        }
        else if (frame.load(skinPath + "background.png")) {
            QPixmap border;
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
            QPixmap mask;
            if (mask.load(skinPath + "mask_nc.png")) {
                // For some reason, w/ compiz setmask won't work
                // for rectangular areas.  This is due to compiz and
                // XShapeCombineMask
                setMask(mask);
            }
        }
    }

    if (QFile::exists(skinPath + "spinner.gif")) {
        m_workingAnimation->LoadAnimation(skinPath + "spinner.gif");
    }

    if (validFrame) {
        frameGraphic = new QPixmap(frame);
        if (frameGraphic) {
            resize(frameGraphic->size());
        }
    }
}

void LaunchyWidget::mousePressEvent(QMouseEvent *e) {
    if (e->buttons() == Qt::LeftButton) {
        if (g_settings->value("GenOps/dragmode", 0).toInt() == 0
            || (e->modifiers() & Qt::ShiftModifier)) {
            dragging = true;
            dragStartPoint = e->pos();
        }
    }
    hideAlternativeList();
    activateWindow();
    m_inputBox->setFocus();
}


void LaunchyWidget::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons() == Qt::LeftButton && dragging)
    {
        QPoint p = e->globalPos() - dragStartPoint;
        move(p);
        hideAlternativeList();
        m_inputBox->setFocus();
    }
}


void LaunchyWidget::mouseReleaseEvent(QMouseEvent* event)
{
    event = event; // Warning removal
    dragging = false;
    hideAlternativeList();
    m_inputBox->setFocus();
}


void LaunchyWidget::contextMenuEvent(QContextMenuEvent* event) {
    QMenu menu(this);
    menu.addAction(actRebuild);
    menu.addAction(actReloadSkin);
    menu.addAction(actOptions);
    menu.addSeparator();
    menu.addAction(actExit);
    menuOpen = true;
    menu.exec(event->globalPos());
    menuOpen = false;
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
    }
}


void LaunchyWidget::buildCatalog() {
    updateTimer->stop();
    saveSettings();

    // Use the catalog builder to refresh the catalog in a worker thread
    QMetaObject::invokeMethod(g_builder.data(), &CatalogBuilder::buildCatalog);

    startUpdateTimer();
}


void LaunchyWidget::showOptionDialog() {
    if (!optionsOpen) {
        showLaunchy(true);
        optionsOpen = true;
        OptionDialog options(NULL);
        options.setObjectName("options");
#ifdef Q_OS_WIN
        // need to use this method in Windows to ensure that keyboard focus is set when
        // being activated via a message from another instance of Launchy
        // SetForegroundWindowEx((HWND)options.winId());
#endif
        options.exec();

        m_inputBox->activateWindow();
        m_inputBox->setFocus();
        optionsOpen = false;
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
        activateWindow();
        raise();
    }
}


void LaunchyWidget::showLaunchy(bool noFade) {
    // shouldDonate();
    hideAlternativeList();

    loadPosition(pos());

    fader->fadeIn(noFade || alwaysShowLaunchy);

#ifdef Q_OS_WIN
    // need to use this method in Windows to ensure that keyboard focus is set when
    // being activated via a hook or message from another instance of Launchy
    // SetForegroundWindowEx((HWND)winId());
#elif defined(Q_OS_LINUX)
    /* Fix for bug 2994680: Not sure why this is necessary, perhaps someone with more
       Qt experience can tell, but doing these two calls will force the window to actually
       get keyboard focus when it is activated. It seems from the bug reports that this
       only affects Linux (and I could only test it on my Linux system - running KDE), so
       it leads me to believe that it is due to an issue in the Qt implementation on Linux. */
    grabKeyboard();
    releaseKeyboard();
#endif
    m_inputBox->raise();
    m_inputBox->activateWindow();
    m_inputBox->selectAll();
    m_inputBox->setFocus();

    // Let the plugins know
    plugins.showLaunchy();
}

void LaunchyWidget::hideLaunchy(bool noFade) {
    if (!isVisible() || isHidden())
        return;

    savePosition();
    hideAlternativeList();
    if (alwaysShowLaunchy)
        return;

    if (isVisible()) {
        fader->fadeOut(noFade);
    }

    // let the plugins know
    plugins.hideLaunchy();
}


void LaunchyWidget::loadOptions() {
    // If a network proxy server is specified, apply an application wide NetworkProxy setting
    QString proxyHost = g_settings->value("WebProxy/hostAddress", "").toString();
    if (!proxyHost.isEmpty()) {
        QNetworkProxy proxy;
        proxy.setType((QNetworkProxy::ProxyType)g_settings->value("WebProxy/type", 0).toInt());
        proxy.setHostName(g_settings->value("WebProxy/hostAddress", "").toString());
        proxy.setPort((quint16)g_settings->value("WebProxy/port", "").toInt());
        QNetworkProxy::setApplicationProxy(proxy);
    }
}


int LaunchyWidget::getHotkey() const {
    int hotkey = g_settings->value("GenOps/hotkey", -1).toInt();
    if (hotkey == -1) {
#if defined(Q_OS_WIN)
        int meta = Qt::AltModifier;
#elif defined(Q_OS_LINUX)
        int meta = Qt::ControlModifier;
#elif defined(Q_OS_MAC)
        int meta = Qt::MetaModifier;
#endif
        hotkey = g_settings->value("GenOps/hotkeyModifier", meta).toInt() |
            g_settings->value("GenOps/hotkeyAction", Qt::Key_Space).toInt();
    }
    return hotkey;
}


void LaunchyWidget::createActions()
{
    actShow = new QAction(tr("Show Launchy"), this);
    connect(actShow, SIGNAL(triggered()), this, SLOT(showLaunchy()));

    actRebuild = new QAction(tr("Rebuild catalog"), this);
    actRebuild->setShortcut(QKeySequence(Qt::Key_F5));
    connect(actRebuild, SIGNAL(triggered()), this, SLOT(buildCatalog()));
    addAction(actRebuild);

    actReloadSkin = new QAction(tr("Reload skin"), this);
    actReloadSkin->setShortcut(QKeySequence(Qt::Key_F5 | Qt::SHIFT));
    connect(actReloadSkin, SIGNAL(triggered()), this, SLOT(reloadSkin()));
    addAction(actReloadSkin);

    actOptions = new QAction(tr("Options"), this);
    actOptions->setShortcut(QKeySequence(Qt::Key_Comma | Qt::CTRL));
    connect(actOptions, SIGNAL(triggered()), this, SLOT(showOptionDialog()));
    addAction(actOptions);

    actExit = new QAction(tr("Exit"), this);
    connect(actExit, SIGNAL(triggered()), this, SLOT(exit()));
}

