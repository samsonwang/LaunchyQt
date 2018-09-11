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

#ifdef Q_WS_MAC
#include <QMacStyle>
#endif
#include "icon_delegate.h"
#include "main.h"
#include "globals.h"
#include "options.h"
#include "plugin_interface.h"
#include "FileSearch.h"


#ifdef Q_WS_WIN
void SetForegroundWindowEx(HWND hWnd)
{
	// Attach foreground window thread to our thread
	const DWORD foreGroundID = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
	const DWORD currentID = GetCurrentThreadId();

	AttachThreadInput(foreGroundID, currentID, TRUE);
	// Do our stuff here 
	HWND lastActivePopupWnd = GetLastActivePopup(hWnd);
	SetForegroundWindow(lastActivePopupWnd);

	// Detach the attached thread
	AttachThreadInput(foreGroundID, currentID, FALSE);
}
#endif


LaunchyWidget::LaunchyWidget(CommandFlags command) :
#ifdef Q_WS_WIN
	QWidget(NULL, Qt::FramelessWindowHint | Qt::Tool),
#endif
#ifdef Q_WS_X11
	QWidget(NULL, Qt::FramelessWindowHint | Qt::Tool),
#endif
#ifdef Q_WS_MAC
	QWidget(NULL, Qt::FramelessWindowHint),
#endif

	frameGraphic(NULL),
	trayIcon(NULL),
	alternatives(NULL),
	updateTimer(NULL),
	dropTimer(NULL),
	condensedTempIcon(NULL)
{
	setObjectName("launchy");
	setWindowTitle(tr("Launchy"));
#ifdef Q_WS_WIN
	setWindowIcon(QIcon(":/resources/launchy128.png"));
#endif
#ifdef Q_WS_MAC
	setWindowIcon(QIcon("../Resources/launchy_icon_mac.icns"));
	//setAttribute(Qt::WA_MacAlwaysShowToolWindow);
#endif

	setAttribute(Qt::WA_AlwaysShowToolTips);
	setAttribute(Qt::WA_InputMethodEnabled);
	if (platform->supportsAlphaBorder())
	{
		setAttribute(Qt::WA_TranslucentBackground);
	}
	setFocusPolicy(Qt::ClickFocus);

	createActions();

	gMainWidget = this;
	menuOpen = false;
	optionsOpen = false;
	dragging = false;
	gSearchText = "";

	alwaysShowLaunchy = false;

	connect(&iconExtractor, SIGNAL(iconExtracted(int, QString, QIcon)), this, SLOT(iconExtracted(int, QString, QIcon)));

	fader = new Fader(this);
	connect(fader, SIGNAL(fadeLevel(double)), this, SLOT(setFadeLevel(double)));

	optionsButton = new QPushButton(this);
	optionsButton->setObjectName("opsButton");
	optionsButton->setToolTip(tr("Launchy Options"));
	optionsButton->setGeometry(QRect());
	connect(optionsButton, SIGNAL(clicked()), this, SLOT(showOptionsDialog()));

	closeButton = new QPushButton(this);
	closeButton->setObjectName("closeButton");
	closeButton->setToolTip(tr("Close Launchy"));
	closeButton->setGeometry(QRect());
	connect(closeButton, SIGNAL(clicked()), qApp, SLOT(quit()));

	output = new QLabel(this);
	output->setObjectName("output");
	output->setAlignment(Qt::AlignHCenter);

	input = new CharLineEdit(this);
#ifdef Q_WS_MAC
	QMacStyle::setFocusRectPolicy(input, QMacStyle::FocusDisabled);
#endif
	input->setObjectName("input");
	connect(input, SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(inputKeyPressEvent(QKeyEvent*)));
	connect(input, SIGNAL(focusIn(QFocusEvent*)), this, SLOT(focusInEvent(QFocusEvent*)));
	connect(input, SIGNAL(focusOut(QFocusEvent*)), this, SLOT(focusOutEvent(QFocusEvent*)));
	connect(input, SIGNAL(inputMethod(QInputMethodEvent*)), this, SLOT(inputMethodEvent(QInputMethodEvent*)));

	outputIcon = new QLabel(this);
	outputIcon->setObjectName("outputIcon");

	workingAnimation = new AnimationLabel(this);
	workingAnimation->setObjectName("workingAnimation");
	workingAnimation->setGeometry(QRect());

	// Load settings
	settings.load();

	// If this is the first time running or a new version, call updateVersion
	if (gSettings->value("version", 0).toInt() != LAUNCHY_VERSION)
	{
		updateVersion(gSettings->value("version", 0).toInt());
		command |= ShowLaunchy;
	}

	alternatives = new CharListWidget(this);
	alternatives->setObjectName("alternatives");
	alternatives->setWindowFlags(Qt::Window | Qt::Tool | Qt::FramelessWindowHint);
	alternatives->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	alternatives->setTextElideMode(Qt::ElideLeft);
	alternatives->setUniformItemSizes(true);
	listDelegate = new IconDelegate(this);
	defaultListDelegate = alternatives->itemDelegate();
        setSuggestionListMode(gSettings->value("GenOps/condensedView", 2).toInt());
	altScroll = alternatives->verticalScrollBar();
	altScroll->setObjectName("altScroll");
	connect(alternatives, SIGNAL(currentRowChanged(int)), this, SLOT(alternativesRowChanged(int)));
	connect(alternatives, SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(alternativesKeyPressEvent(QKeyEvent*)));
	connect(alternatives, SIGNAL(focusOut(QFocusEvent*)), this, SLOT(focusOutEvent(QFocusEvent*)));

	alternativesPath = new QLabel(alternatives);
	alternativesPath->setObjectName("alternativesPath");
	alternativesPath->hide();
	listDelegate->setAlternativesPathWidget(alternativesPath);

	// Load the plugins
	plugins.loadPlugins();

	// Set the general options
	if (setAlwaysShow(gSettings->value("GenOps/alwaysshow", false).toBool()))
		command |= ShowLaunchy;
	setAlwaysTop(gSettings->value("GenOps/alwaystop", false).toBool());

	// Check for udpates?
	if (gSettings->value("GenOps/updatecheck", true).toBool())
	{
		checkForUpdate();
	}

	// Set the hotkey
	QKeySequence hotkey = getHotkey();
	if (!setHotkey(hotkey))
	{
		QMessageBox::warning(this, tr("Launchy"), tr("The hotkey %1 is already in use, please select another.").arg(hotkey.toString()));
		command = ShowLaunchy | ShowOptions;
	}

	// Set the timers
	dropTimer = new QTimer(this);
	dropTimer->setSingleShot(true);
	connect(dropTimer, SIGNAL(timeout()), this, SLOT(dropTimeout()));

	updateTimer = new QTimer(this);
	connect(updateTimer, SIGNAL(timeout()), this, SLOT(buildCatalog()));
	updateTimer->setSingleShot(true);
	startUpdateTimer();

	// Load the catalog
	gBuilder = new CatalogBuilder(&plugins);
	gBuilder->moveToThread(&builderThread);
	connect(gBuilder, SIGNAL(catalogIncrement(int)), this, SLOT(catalogProgressUpdated(int)));
	connect(gBuilder, SIGNAL(catalogFinished()), this, SLOT(catalogBuilt()));
	builderThread.start(QThread::IdlePriority);
	builderThread.setObjectName("CatalogBuilder");

	catalog = gBuilder->getCatalog();
	if (!catalog->load(settings.catalogFilename()))
	{
		command |= Rescan;
	}

	// Load the history
	history.load(settings.historyFilename());

	// Load the skin
	applySkin(gSettings->value("GenOps/skin", "Default").toString());

	// Move to saved position
	loadPosition(gSettings->value("Display/pos", QPoint(0,0)).toPoint());
	loadOptions();

	executeStartupCommand(command);
}


LaunchyWidget::~LaunchyWidget()
{
	delete updateTimer;
	delete dropTimer;
	delete alternatives;
}


void LaunchyWidget::executeStartupCommand(int command)
{
	if (command & ResetPosition)
	{
		QRect r = geometry();
		int primary = qApp->desktop()->primaryScreen();
		QRect scr = qApp->desktop()->availableGeometry(primary);

		QPoint pt(scr.width()/2 - r.width()/2, scr.height()/2 - r.height()/2);
		move(pt);
	}

	if (command & ResetSkin)
	{
		setOpaqueness(100);
		showTrayIcon();
		applySkin("Default");
	}

	if (command & ShowLaunchy)
		showLaunchy();

	if (command & ShowOptions)
		showOptionsDialog();

	if (command & Rescan)
		buildCatalog();

	if (command & Exit)
		close();
}


void LaunchyWidget::paintEvent(QPaintEvent* event)
{
	// Do the default draw first to render any background specified in the stylesheet
	QStyleOption styleOption;
	styleOption.init(this);
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	style()->drawPrimitive(QStyle::PE_Widget, &styleOption, &painter, this);

	// Now draw the standard frame.png graphic if there is one
	if (frameGraphic)
	{
		painter.drawPixmap(0,0, *frameGraphic);
	}

	QWidget::paintEvent(event);
}


void LaunchyWidget::setSuggestionListMode(int mode)
{
	if (mode)
	{
		// The condensed mode needs an icon placeholder or it repositions text when the icon becomes available
		if (!condensedTempIcon)
		{
			QPixmap pixmap(16, 16);
			pixmap.fill(Qt::transparent);
			condensedTempIcon = new QIcon(pixmap);
		}
		alternatives->setItemDelegate(defaultListDelegate);
	}
	else
	{
		delete condensedTempIcon;
		condensedTempIcon = NULL;
		alternatives->setItemDelegate(listDelegate);
	}
}


bool LaunchyWidget::setHotkey(QKeySequence hotkey)
{
	return platform->setHotkey(hotkey, this, SLOT(onHotkey()));
}


void LaunchyWidget::showTrayIcon()
{
	if (!QSystemTrayIcon::isSystemTrayAvailable())
		return;

	if (gSettings->value("GenOps/showtrayicon", true).toBool())
	{
		if (!trayIcon)
			trayIcon = new QSystemTrayIcon(this);
		QKeySequence hotkey = platform->getHotkey();
		trayIcon->setToolTip(tr("Launchy (press %1 to activate)").arg(hotkey.toString()));
		trayIcon->setIcon(QIcon(":/resources/launchy16.png"));
		trayIcon->show();
		connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
			this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));

		QMenu* trayMenu = new QMenu(this);
		trayMenu->addAction(actShow);
		trayMenu->addAction(actRebuild);
		trayMenu->addAction(actOptions);
		trayMenu->addSeparator();
		trayMenu->addAction(actExit);

		trayIcon->setContextMenu(trayMenu);
	}
	else if (trayIcon)
	{
		delete trayIcon;
		trayIcon = NULL;
	}
}


// Repopulate the alternatives list with the current search results
// and set its size and position accordingly.
void LaunchyWidget::updateAlternatives(bool resetSelection)
{
        int mode = gSettings->value("GenOps/condensedView", 2).toInt();
	int i = 0;
	for (; i < searchResults.size(); ++i)
	{
		qDebug() << "Alternative" << i << ":" << searchResults[i].fullPath;
		QString fullPath = QDir::toNativeSeparators(searchResults[i].fullPath);
#ifdef _DEBUG
		fullPath += QString(" (%1 launches)").arg(searchResults[i].usage);
#endif			
		QListWidgetItem* item;
		if (i < alternatives->count())
		{
			item = alternatives->item(i);
		}
		else
		{
			item = new QListWidgetItem(fullPath, alternatives);
		}
		if (item->data(mode == 1 ? ROLE_SHORT : ROLE_FULL) != fullPath)
		{
			// condensedTempIcon is a blank icon or null
			item->setData(ROLE_ICON, *condensedTempIcon);
		}
		item->setData(mode == 1 ? ROLE_FULL : ROLE_SHORT, searchResults[i].shortName);
		item->setData(mode == 1 ? ROLE_SHORT : ROLE_FULL, fullPath);
		if (i >= alternatives->count())
			alternatives->addItem(item);
	}

	while (alternatives->count() > i)
	{
		delete alternatives->takeItem(i);
	}

	if (resetSelection)
	{
		alternatives->setCurrentRow(0);
	}
	iconExtractor.processIcons(searchResults);

	// Now resize and reposition the list
	int numViewable = gSettings->value("GenOps/numviewable", "4").toInt();
	int min = alternatives->count() < numViewable ? alternatives->count() : numViewable;

	// The stylesheet doesn't load immediately, so we cache the placement rectangle here
	if (alternativesRect.isNull())
		alternativesRect = alternatives->geometry();
	QRect rect = alternativesRect;
	rect.setHeight(min * alternatives->sizeHintForRow(0));
	rect.translate(pos());

	// Is there room for the dropdown box?
	if (rect.y() + rect.height() > qApp->desktop()->height())
	{
		// Only move it if there's more space above
		// In both cases, ensure it doesn't spill off the screen
		if (pos().y() + input->pos().y() > qApp->desktop()->height() / 2)
		{
			rect.moveTop(pos().y() + input->pos().y() - rect.height());
			if (rect.top() < 0)
				rect.setTop(0);
		}
		else
		{
			rect.setBottom(qApp->desktop()->height());
		}
	}

	alternatives->setGeometry(rect);
}


void LaunchyWidget::showAlternatives()
{
	// Ensure that any pending shows of the alternatives list are cancelled
	// so that we only update the list once.
	dropTimer->stop();

	alternatives->show();
	alternatives->setFocus();
	qApp->syncX();
}


void LaunchyWidget::hideAlternatives()
{
	// Ensure that any pending shows of the alternatives list are cancelled
	// so that the list isn't erroneously shown shortly after being dismissed.
	dropTimer->stop();

	// clear the selection before hiding to prevent flicker
	alternatives->setCurrentRow(-1);
	alternatives->repaint();
	alternatives->hide();
	iconExtractor.stop();
}


void LaunchyWidget::launchItem(CatItem& item)
{
	int ops = MSG_CONTROL_LAUNCHITEM;

	if (item.id != HASH_LAUNCHY && item.id != HASH_LAUNCHYFILE)
	{
		ops = plugins.execute(&inputData, &item);
		if (ops > 1)
		{
			switch (ops)
			{
			case MSG_CONTROL_EXIT:
				close();
				break;
			case MSG_CONTROL_OPTIONS:
				showOptionsDialog();
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
	}
	
	if (ops == MSG_CONTROL_LAUNCHITEM)
	{
		QString args = "";
		if (inputData.count() > 1)
			for(int i = 1; i < inputData.count(); ++i)
				args += inputData[i].getText() + " ";

/* UPDATE
#ifdef Q_WS_X11
		if (!platform->Execute(item.fullPath, args))
			runProgram(item.fullPath, args);
#else
*/
		runProgram(item.fullPath, args);
//#endif
	}
	
	catalog->incrementUsage(item);
	history.addItem(inputData);
}


void LaunchyWidget::focusInEvent(QFocusEvent* event)
{
    if (event->gotFocus() && fader->isFading())
		fader->fadeIn(false);        


	QWidget::focusInEvent(event);
}


void LaunchyWidget::focusOutEvent(QFocusEvent* event)
{
        if (event->reason() == Qt::ActiveWindowFocusReason)
	{
		if (gSettings->value("GenOps/hideiflostfocus", false).toBool() &&
                        !isActiveWindow() && !alternatives->isActiveWindow() && !optionsOpen && !fader->isFading())
		{
			hideLaunchy();
		}
	}
}


void LaunchyWidget::alternativesRowChanged(int index)
{
	// Check that index is a valid history item index
	// If the current entry is a history item or there is no text entered
	if (index >= 0 && index < searchResults.count())
	{
		CatItem item = searchResults[index];
		if ((inputData.count() > 0 && inputData.first().hasLabel(LABEL_HISTORY)) || input->text().length() == 0)
		{
			// Used a void* to hold an int.. ick!
			// BUT! Doing so avoids breaking existing catalogs
			long long hi = reinterpret_cast<long long>(item.data);
			int historyIndex = static_cast<int>(hi);

			if (item.id == HASH_HISTORY && historyIndex < searchResults.count())
			{
				inputData = history.getItem(historyIndex);
				input->selectAll();
				input->insert(inputData.toString());
				input->selectAll();
				output->setText(inputData[0].getTopResult().shortName);
				// No need to fetch the icon again, just grab it from the alternatives row
				outputIcon->setPixmap(alternatives->item(index)->icon().pixmap(outputIcon->size()));
				outputItem = item;
				gSearchText = inputData.toString();
			}
		}
		else if (inputData.count() > 0 && 
			(inputData.last().hasLabel(LABEL_AUTOSUGGEST) || inputData.last().hasText() == 0))
		{
			qDebug() << "Autosuggest" << item.shortName;

			inputData.last().setText(item.shortName);
			inputData.last().setLabel(LABEL_AUTOSUGGEST);

			QString root = inputData.toString(true);
			input->selectAll();
			input->insert(root + item.shortName);
			input->setSelection(root.length(), item.shortName.length());

			output->setText(item.shortName);
			// No need to fetch the icon again, just grab it from the alternatives row
			outputIcon->setPixmap(alternatives->item(index)->icon().pixmap(outputIcon->size()));
			outputItem = item;
			gSearchText = "";
		}
	}
}


void LaunchyWidget::inputKeyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Tab)
	{
		keyPressEvent(event);
	}
	else
	{
		event->ignore();
	}
}


void LaunchyWidget::alternativesKeyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape)
	{
		hideAlternatives();
		input->setFocus();
		event->ignore();
	}
	else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Tab)
	{
		if (searchResults.count() > 0)
		{
			int row = alternatives->currentRow();
			if (row > -1)
			{
				QString location = "History/" + input->text();
				QStringList hist;
				hist << searchResults[row].lowName << searchResults[row].fullPath;
				gSettings->setValue(location, hist);

				if (row > 0)
					searchResults.move(row, 0);

				if (event->key() == Qt::Key_Tab)
				{
					doTab();
					processKey();
				}
				else
				{
					// Load up the inputData properly before running the command
					/* commented out until I find a fix for it breaking the history selection
					inputData.last().setTopResult(searchResults[0]);
					doTab();
					inputData.parse(input->text());
					inputData.erase(inputData.end() - 1);*/

					updateOutputWidgets();
					keyPressEvent(event);
				}
			}
		}
	}
	else if (event->key() == Qt::Key_Delete && (event->modifiers() & Qt::ShiftModifier) != 0)
	{
		int row = alternatives->currentRow();
		if (row > -1)
		{
			if (searchResults[row].id == HASH_HISTORY)
			{
				// Delete selected history entry from the alternatives list
				history.removeAt(row);
				input->clear();
				searchOnInput();
				updateAlternatives(false);
				alternativesRowChanged(alternatives->currentRow());
			}
			else
			{
				// Demote the selected item down the alternatives list
				catalog->demoteItem(searchResults[row]);
				searchOnInput();
				updateOutputWidgets(false);
			}
		}
	}
	else if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right ||
			 event->text().length() > 0)
	{
		// Send text entry to the input control
		activateWindow();
		input->setFocus();
		event->ignore();
		input->keyPressEvent(event);
		keyPressEvent(event);
	}
	alternatives->setFocus();
}


void LaunchyWidget::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Escape)
	{
		if (alternatives->isVisible())
			hideAlternatives();
		else
			hideLaunchy();
	}

	else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter)
	{
		doEnter();
	}

	else if (event->key() == Qt::Key_Down || event->key() == Qt::Key_PageDown || 
			 event->key() == Qt::Key_Up || event->key() == Qt::Key_PageUp)
	{
		if (alternatives->isVisible())
		{
			if (!alternatives->isActiveWindow())
			{
				// Don't refactor the activateWindow outside the if, it won't work properly any other way!
				if (alternatives->currentRow() < 0 && alternatives->count() > 0)
				{
					alternatives->activateWindow();
					alternatives->setCurrentRow(0);
				}
				else
				{
					alternatives->activateWindow();
					qApp->sendEvent(alternatives, event);
				}
			}
		}
		else if (event->key() == Qt::Key_Down || event->key() == Qt::Key_PageDown)
		{
			// do a search and show the results, selecting the first one
			searchOnInput();
			if (searchResults.count() > 0)
			{
				updateAlternatives();
				showAlternatives();
			}
		}
	}
	else if ((event->key() == Qt::Key_Tab || event->key() == Qt::Key_Backspace) && event->modifiers() == Qt::ShiftModifier)
	{
		doBackTab();
		processKey();
	}
	else if (event->key() == Qt::Key_Tab)
	{
		doTab();
		processKey();
	}
	else if (event->key() == Qt::Key_Slash || event->key() == Qt::Key_Backslash)
	{
		if (inputData.count() > 0 && inputData.last().hasLabel(LABEL_FILE) &&
			searchResults.count() > 0 && searchResults[0].id == HASH_LAUNCHYFILE)
			doTab();
		processKey();
	}
	else if (event->key()== Qt::Key_Insert && event->modifiers() == Qt::ShiftModifier)
	{
		// ensure pasting text with Shift+Insert also parses input
		// longer term parsing should be done using the TextChanged event
		processKey();
	}
	else if (event->text().length() > 0)
	{
		// process any other key with character output
		event->ignore();
		processKey();
	}
}


// remove input text back to the previous input section
void LaunchyWidget::doBackTab()
{
	QString text = input->text();
	int index = text.lastIndexOf(input->separatorText());
	if (index >= 0)
	{
		text.truncate(index+3);
		input->selectAll();
		input->insert(text);
    }
	
    else if (text.lastIndexOf(QDir::separator()) >= 0) {
        text.truncate(text.lastIndexOf(QDir::separator())+1);
        input->selectAll();
        input->insert(text);
    }
	else if (text.lastIndexOf(QChar(' ')) >= 0) {
		text.truncate(text.lastIndexOf(QChar(' '))+1);
		input->selectAll();
		input->insert(text);
	}
	
	else if (text.lastIndexOf(QDir::separator()) >= 0) {
		text.truncate(text.lastIndexOf(QDir::separator())+1);
		input->selectAll();
		input->insert(text);
	}
	else if (text.lastIndexOf(QChar(' ')) >= 0) {
		text.truncate(text.lastIndexOf(QChar(' '))+1);
		input->selectAll();
		input->insert(text);
	}
	else
	{
        input->clear();
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

			input->selectAll();
			input->insert(inputData.toString(true) + QDir::toNativeSeparators(path));
		}
		else
		{
			inputData.last().setTopResult(searchResults[0]);
			inputData.last().setText(searchResults[0].shortName);
			input->selectAll();
			input->insert(inputData.toString() + input->separatorText());
		}
	}
}


void LaunchyWidget::doEnter()
{
	hideAlternatives();

	if ((inputData.count() > 0 && searchResults.count() > 0) || inputData.count() > 1)
	{
		CatItem& item = inputData[0].getTopResult();
		qDebug() << "Launching" << item.shortName << ":" << item.fullPath;
		launchItem(item);
		hideLaunchy();
	}
	else
	{
		qDebug("Nothing to launch");
	}
}


void LaunchyWidget::inputMethodEvent(QInputMethodEvent* event)
{
	event = event; // Warning removal
	processKey();
}


void LaunchyWidget::processKey()
{
	inputData.parse(input->text());
	searchOnInput();
	updateOutputWidgets();

	// If there is no input text, ensure that the alternatives list is hidden
	// otherwise, show it after the user defined delay if it's not currently visible
	if (input->text().length() == 0)
	{
		hideAlternatives();
	}
	else if (!alternatives->isVisible())
	{
		startDropTimer();
	}
}


void LaunchyWidget::searchOnInput()
{
	if (catalog == NULL)
		return;

	QString searchText = inputData.count() > 0 ? inputData.last().getText() : "";
	QString searchTextLower = searchText.toLower();
	gSearchText = searchTextLower;
	searchResults.clear();

	if ((inputData.count() > 0 && inputData.first().hasLabel(LABEL_HISTORY)) || input->text().length() == 0)
	{
		// Add history items exclusively and unsorted so they remain in most recently used order
		qDebug() << "Searching history for" << searchText;
		history.search(searchTextLower, searchResults);
	}
	else
	{
		// Search the catalog for matching items
		if (inputData.count() == 1)
		{
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
		if (searchText.contains(QDir::separator()) || searchText.startsWith("~") ||
			(searchText.size() == 2 && searchText[0].isLetter() && searchText[1] == ':'))
		{
			FileSearch::search(searchText, searchResults, inputData);
		}
	}
}


// If there are current results, update the output text and icon
void LaunchyWidget::updateOutputWidgets(bool resetAlternativesSelection)
{
	if (searchResults.count() > 0 && (inputData.count() > 1 || input->text().length() > 0))
	{
		qDebug() << "Setting output text to" << searchResults[0].shortName;

		QString outputText = Catalog::decorateText(searchResults[0].shortName, gSearchText, true);
#ifdef _DEBUG
		outputText += QString(" (%1 launches)").arg(searchResults[0].usage);
#endif
		output->setText(outputText);
		if (outputItem != searchResults[0])
		{
			outputItem = searchResults[0];
			outputIcon->clear();
			iconExtractor.processIcon(searchResults[0]);
		}

		if (outputItem.id != HASH_HISTORY)
		{
			// Did the plugin take control of the input?
			if (inputData.last().getID() != 0)
				outputItem.id = inputData.last().getID();
			inputData.last().setTopResult(searchResults[0]);
		}

		// Only update the alternatives list if it is visible
		if (alternatives->isVisible())
		{
			updateAlternatives(resetAlternativesSelection);
		}
	}
	else
	{
		// No results to show, clear the output UI and hide the alternatives list
		output->clear();
		outputIcon->clear();
		outputItem = CatItem();
		hideAlternatives();
	}
}


void LaunchyWidget::startDropTimer()
{
	int delay = gSettings->value("GenOps/autoSuggestDelay", 1000).toInt();
	if (delay > 0)
		dropTimer->start(delay);
	else
		dropTimeout();
}


void LaunchyWidget::dropTimeout()
{
	// Don't do anything if Launchy has been hidden since the timer was started
	if (isVisible() && searchResults.count() > 0)
	{
		updateAlternatives();
		showAlternatives();
	}
}


void LaunchyWidget::iconExtracted(int itemIndex, QString path, QIcon icon)
{
	if (itemIndex == -1)
	{
		// An index of -1 means update the output icon, check that it is also
		// the same item as was originally requested
		if (path == outputItem.fullPath)
		{
			outputIcon->setPixmap(icon.pixmap(outputIcon->size()));
		}
	}
	else if (itemIndex < alternatives->count())
	{
		// >=0 is an item in the alternatives list
		if (itemIndex < searchResults.count() && path == searchResults[itemIndex].fullPath)
		{
			QListWidgetItem* listItem = alternatives->item(itemIndex);
			listItem->setIcon(icon);
			listItem->setData(ROLE_ICON, icon);

			QRect rect = alternatives->visualItemRect(listItem);
			repaint(rect);
		}
	}
}


void LaunchyWidget::catalogProgressUpdated(int value)
{
	if (value == 0)
	{
		workingAnimation->Start();
	}
}


void LaunchyWidget::catalogBuilt()
{
	// Save settings and updated catalog, stop the "working" animation 
	saveSettings();
	workingAnimation->Stop();

	// Now do a search using the updated catalog
	searchOnInput();
	updateOutputWidgets();
}


void LaunchyWidget::checkForUpdate()
{
	http = new QHttp(this);
	verBuffer = new QBuffer(this);
	counterBuffer = new QBuffer(this);
	verBuffer->open(QIODevice::ReadWrite);
	counterBuffer->open(QIODevice::ReadWrite);

	connect(http, SIGNAL(done( bool)), this, SLOT(httpGetFinished(bool)));
	http->setHost("www.launchy.net");
	http->get("http://www.launchy.net/version2.html", verBuffer);
}


void LaunchyWidget::httpGetFinished(bool error)
{
	if (!error)
	{
		QString str(verBuffer->data());
		int ver = str.toInt();
		if (ver > LAUNCHY_VERSION)
		{
			QMessageBox box;
			box.setIcon(QMessageBox::Information);
			box.setTextFormat(Qt::RichText);
			box.setWindowTitle(tr("A new version of Launchy is available"));
			box.setText(tr("A new version of Launchy is available.\n\nYou can download it at \
						   <qt><a href=\"http://www.launchy.net/\">http://www.launchy.net</a></qt>"));
			box.exec();
		}
		if (http != NULL)
			delete http;
		http = NULL;
	}
	verBuffer->close();
	counterBuffer->close();
	delete verBuffer;
	delete counterBuffer;
}


void LaunchyWidget::setSkin(const QString& name)
{
	hideLaunchy(true);
	applySkin(name);
	showLaunchy(false);
}


void LaunchyWidget::updateVersion(int oldVersion)
{
	if (oldVersion < 199)
	{
		settings.removeAll();
		settings.load();
	}

	if (oldVersion < 249)
	{
		gSettings->setValue("GenOps/skin", "Default");
	}

	if (oldVersion < LAUNCHY_VERSION)
	{
		gSettings->setValue("donateTime", QDateTime::currentDateTime().addDays(21));
		gSettings->setValue("version", LAUNCHY_VERSION);
	}
}


void LaunchyWidget::loadPosition(QPoint pt)
{
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

	int centerOption = gSettings->value("GenOps/alwayscenter", 3).toInt();
	if (centerOption & 1)
		pt.setX(screen.center().x() - rect.width()/2);
	if (centerOption & 2)
		pt.setY(screen.center().y() - rect.height()/2);

	move(pt);
}


void LaunchyWidget::saveSettings()
{
	qDebug() << "Save settings";
	savePosition();
	gSettings->sync();
	catalog->save(settings.catalogFilename());
	history.save(settings.historyFilename());
}


void LaunchyWidget::startUpdateTimer()
{
	int time = gSettings->value("GenOps/updatetimer", 10).toInt();
	if (time != 0)
		updateTimer->start(time * 60000);
	else
		updateTimer->stop();
}


void LaunchyWidget::onHotkey()
{
	if (menuOpen || optionsOpen)
	{
		showLaunchy(true);
		return;
	}
	if (!alwaysShowLaunchy && isVisible() && !fader->isFading() && QApplication::activeWindow() !=0)
	{
		hideLaunchy();
	}
	else
	{
		showLaunchy();
	}
}


void LaunchyWidget::closeEvent(QCloseEvent* event)
{
	builderThread.exit();
	fader->stop();
	saveSettings();

	event->accept();
	qApp->quit();
}


bool LaunchyWidget::setAlwaysShow(bool alwaysShow)
{
	alwaysShowLaunchy = alwaysShow;
	return !isVisible() && alwaysShow;
}


bool LaunchyWidget::setAlwaysTop(bool alwaysTop)
{
	if (alwaysTop && (windowFlags() & Qt::WindowStaysOnTopHint) == 0)
	{
		setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
		alternatives->setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
		return true;
	}
	else if (!alwaysTop && (windowFlags() & Qt::WindowStaysOnTopHint) != 0)
	{
		setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
		alternatives->setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint);
		return true;
	}

	return false;
}


void LaunchyWidget::setOpaqueness(int level)
{
	double value = level / 100.0;
	setWindowOpacity(value);
	alternatives->setWindowOpacity(value);
}


void LaunchyWidget::reloadSkin()
{
	setSkin(currentSkin);
}


void LaunchyWidget::applySkin(const QString& name)
{
	currentSkin = name;

	if (listDelegate == NULL)
		return;

	QString directory = settings.skinPath(name);
	// Use default skin if this one doesn't exist or isn't valid
	if (directory.length() == 0)
	{
		QString defaultSkin = settings.directory("defSkin")[0];
		directory = settings.skinPath(defaultSkin);
		// If still no good then fail with an ugly default
		if (directory.length() == 0)
			return;

		gSettings->setValue("GenOps/skin", defaultSkin);
	}

	// Set a few defaults
	delete frameGraphic;
	frameGraphic = NULL;
	closeButton->setGeometry(QRect());
	optionsButton->setGeometry(QRect());
	input->setAlignment(Qt::AlignLeft);
	output->setAlignment(Qt::AlignCenter);
	alternativesRect = QRect();

	if (!QFile::exists(directory + "misc.txt"))
	{
		// Loading use file:/// syntax allows relative paths in the stylesheet to be rooted
		// in the same directory as the stylesheet
		qApp->setStyleSheet("file:///" + directory + "style.qss");
	}
	else
	{
		// Set positions, this will signify an older launchy skin
		// Read the style sheet
		QFile file(directory + "style.qss");
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return;
		QString styleSheet = QLatin1String(file.readAll());
		file.close();

		// Remove incorrect selectors from the stylesheet
		styleSheet.replace("QLineEdit#", "#");
		styleSheet.replace("QPushButton#", "#");
		styleSheet.replace("QListWidget#", "#");

		// This is causing the ::destroyed connect errors
		qApp->setStyleSheet(styleSheet);

		file.setFileName(directory + "misc.txt");
		if (file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			QTextStream in(&file);
			while (!in.atEnd())
			{
				QString line = in.readLine();
				if (line.startsWith(";")) continue;
				QStringList spl = line.split("=");
				if (spl.size() == 2)
				{
					QStringList sizes = spl.at(1).trimmed().split(",");
					QRect rect;
					if (sizes.size() == 4)
					{
						rect.setRect(sizes[0].toInt(), sizes[1].toInt(), sizes[2].toInt(), sizes[3].toInt());
					}

					if (spl.at(0).trimmed().compare("input", Qt::CaseInsensitive) == 0)
						input->setGeometry(rect);
					else if (spl.at(0).trimmed().compare("inputAlignment", Qt::CaseInsensitive) == 0)
						input->setAlignment(
						sizes[0].trimmed().compare("left", Qt::CaseInsensitive) == 0 ? Qt::AlignLeft : 
						sizes[0].trimmed().compare("right", Qt::CaseInsensitive) == 0 ? Qt::AlignRight : Qt::AlignHCenter );
					else if (spl.at(0).trimmed().compare("output", Qt::CaseInsensitive) == 0) 
						output->setGeometry(rect);
					else if (spl.at(0).trimmed().compare("outputAlignment", Qt::CaseInsensitive) == 0)
						output->setAlignment(
						sizes[0].trimmed().compare("left", Qt::CaseInsensitive) == 0 ? Qt::AlignLeft : 
						sizes[0].trimmed().compare("right", Qt::CaseInsensitive) == 0 ? Qt::AlignRight : Qt::AlignHCenter );
					else if (spl.at(0).trimmed().compare("alternatives", Qt::CaseInsensitive) == 0)
						alternativesRect = rect;
					else if (spl.at(0).trimmed().compare("boundary", Qt::CaseInsensitive) == 0)
						resize(rect.size());
					else if (spl.at(0).trimmed().compare("icon", Qt::CaseInsensitive) == 0)
						outputIcon->setGeometry(rect);
					else if (spl.at(0).trimmed().compare("optionsbutton", Qt::CaseInsensitive) == 0)
					{
						optionsButton->setGeometry(rect);
						optionsButton->show();
					}
					else if (spl.at(0).trimmed().compare("closebutton", Qt::CaseInsensitive) == 0)
					{
						closeButton->setGeometry(rect);
						closeButton->show();
					}
					else if (spl.at(0).trimmed().compare("dropPathColor", Qt::CaseInsensitive) == 0)
					{
						listDelegate->setColor(spl.at(1));
					}
					else if (spl.at(0).trimmed().compare("dropPathSelColor", Qt::CaseInsensitive) == 0)
						listDelegate->setColor(spl.at(1),true);
					else if (spl.at(0).trimmed().compare("dropPathFamily", Qt::CaseInsensitive) == 0)
						listDelegate->setFamily(spl.at(1));
					else if (spl.at(0).trimmed().compare("dropPathSize", Qt::CaseInsensitive) == 0)
						listDelegate->setSize(spl.at(1).toInt());
					else if (spl.at(0).trimmed().compare("dropPathWeight", Qt::CaseInsensitive) == 0)
						listDelegate->setWeight(spl.at(1).toInt());
					else if (spl.at(0).trimmed().compare("dropPathItalics", Qt::CaseInsensitive) == 0)
						listDelegate->setItalics(spl.at(1).toInt());
				}
			}
			file.close();
		}
	}

	bool validFrame = false;
	QPixmap frame;

	if (platform->supportsAlphaBorder())
	{
		if (frame.load(directory + "frame.png"))
		{
			validFrame = true;
		}
		else if (frame.load(directory + "background.png"))
		{
			QPixmap border;
			if (border.load(directory + "mask.png"))
			{
				frame.setMask(border);
			}
			if (border.load(directory + "alpha.png"))
			{
				QPainter surface(&frame);
				surface.drawPixmap(0,0, border);
			}
			validFrame = true;
		}
	}
	
	if (!validFrame)
	{
		// Set the background image
		if (frame.load(directory + "background_nc.png"))
		{
			validFrame = true;

			// Set the background mask
			QPixmap mask;
			if (mask.load(directory + "mask_nc.png"))
			{
				// For some reason, w/ compiz setmask won't work
				// for rectangular areas.  This is due to compiz and
				// XShapeCombineMask
				setMask(mask);
			}
		}
	}

	if (QFile::exists(directory + "spinner.mng"))
	{
		workingAnimation->LoadAnimation(directory + "spinner.mng");
	}

	if (validFrame)
	{
		frameGraphic = new QPixmap(frame);
		if (frameGraphic)
		{
			resize(frameGraphic->size());
		}
	}

	int maxIconSize = outputIcon->width();
	maxIconSize = qMax(maxIconSize, outputIcon->height());
	platform->setPreferredIconSize(maxIconSize);
}


void LaunchyWidget::mousePressEvent(QMouseEvent *e)
{
	if (e->buttons() == Qt::LeftButton)
	{
		if (gSettings->value("GenOps/dragmode", 0).toInt() == 0 || (e->modifiers() & Qt::ShiftModifier))
		{
			dragging = true;
			dragStartPoint = e->pos();
		}
	}
	hideAlternatives();
	activateWindow();
	input->setFocus();
}


void LaunchyWidget::mouseMoveEvent(QMouseEvent *e)
{
	if (e->buttons() == Qt::LeftButton && dragging)
	{
		QPoint p = e->globalPos() - dragStartPoint;
		move(p);
		hideAlternatives();
		input->setFocus();
	}
}


void LaunchyWidget::mouseReleaseEvent(QMouseEvent* event)
{
	event = event; // Warning removal
	dragging = false;
	hideAlternatives();
	input->setFocus();
}


void LaunchyWidget::contextMenuEvent(QContextMenuEvent* event)
{
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


void LaunchyWidget::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason)
	{
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


void LaunchyWidget::buildCatalog()
{
	updateTimer->stop();
	saveSettings();

	// Use the catalog builder to refresh the catalog in a worker thread
	QMetaObject::invokeMethod(gBuilder, "buildCatalog", Qt::AutoConnection);

	startUpdateTimer();
}


void LaunchyWidget::showOptionsDialog()
{
	if (!optionsOpen)
	{
		hideAlternatives();
		optionsOpen = true;
		OptionsDialog options(this);
		options.setObjectName("options");
#ifdef Q_WS_WIN
		// need to use this method in Windows to ensure that keyboard focus is set when 
		// being activated via a message from another instance of Launchy
		SetForegroundWindowEx(options.winId());
#endif
		options.exec();

		input->activateWindow();
		input->setFocus();
		optionsOpen = false;
	}
}


void LaunchyWidget::shouldDonate()
{
	QDateTime time = QDateTime::currentDateTime();
	QDateTime donateTime = gSettings->value("donateTime",time.addDays(21)).toDateTime();
	if (donateTime.isNull()) return;
	gSettings->setValue("donateTime", donateTime);

	if (donateTime <= time)
	{
#ifdef Q_WS_WIN
		runProgram("http://www.launchy.net/donate.html", "");
#endif
		QDateTime def;
		gSettings->setValue("donateTime", def);
	}
}


void LaunchyWidget::setFadeLevel(double level)
{
	level = qMin(level, 1.0);
	level = qMax(level, 0.0);
	setWindowOpacity(level);
	alternatives->setWindowOpacity(level);
	if (level <= 0.001)
	{
		hide();
	}
	else
	{
            if (!isVisible()) {
                show();
                activateWindow();
                raise();
                }
	}

}


void LaunchyWidget::showLaunchy(bool noFade)
{
	shouldDonate();
	hideAlternatives();

	loadPosition(pos());

	fader->fadeIn(noFade || alwaysShowLaunchy);

#ifdef Q_WS_WIN
	// need to use this method in Windows to ensure that keyboard focus is set when 
	// being activated via a hook or message from another instance of Launchy
	SetForegroundWindowEx(winId());
#elif defined(Q_WS_X11)
	/* Fix for bug 2994680: Not sure why this is necessary, perhaps someone with more
	   Qt experience can tell, but doing these two calls will force the window to actually
	   get keyboard focus when it is activated. It seems from the bug reports that this
	   only affects Linux (and I could only test it on my Linux system - running KDE), so
	   it leads me to believe that it is due to an issue in the Qt implementation on Linux. */
	grabKeyboard();
	releaseKeyboard();
#endif
	input->raise();
	input->activateWindow();
	input->selectAll();
	input->setFocus();
	qApp->syncX();
	// Let the plugins know
	plugins.showLaunchy();
}


void LaunchyWidget::showLaunchy()
{
	showLaunchy(false);
}


void LaunchyWidget::hideLaunchy(bool noFade)
{
        if (!isVisible() || isHidden())
                return;

	savePosition();
	hideAlternatives();
	if (alwaysShowLaunchy)
		return;

	if (isVisible())
	{
		fader->fadeOut(noFade);
	}

	// let the plugins know
	plugins.hideLaunchy();
}


void LaunchyWidget::loadOptions()
{
	// If a network proxy server is specified, apply an application wide NetworkProxy setting
	QString proxyHost = gSettings->value("WebProxy/hostAddress", "").toString();
	if (proxyHost.length() > 0)
	{
		QNetworkProxy proxy;
		proxy.setType((QNetworkProxy::ProxyType)gSettings->value("WebProxy/type", 0).toInt());
		proxy.setHostName(gSettings->value("WebProxy/hostAddress", "").toString());
		proxy.setPort((quint16)gSettings->value("WebProxy/port", "").toInt());
		QNetworkProxy::setApplicationProxy(proxy);
	}

	showTrayIcon();
}


int LaunchyWidget::getHotkey() const
{
	int hotkey = gSettings->value("GenOps/hotkey", -1).toInt();
	if (hotkey == -1)
	{
#ifdef Q_WS_WIN
		int meta = Qt::AltModifier;
#endif
#ifdef  Q_WS_X11
		int meta = Qt::ControlModifier;
#endif
#ifdef Q_WS_MAC
		int meta = Qt::MetaModifier;
#endif
		hotkey = gSettings->value("GenOps/hotkeyModifier", meta).toInt() |
				 gSettings->value("GenOps/hotkeyAction", Qt::Key_Space).toInt();
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
	connect(actOptions, SIGNAL(triggered()), this, SLOT(showOptionsDialog()));
	addAction(actOptions);

	actExit = new QAction(tr("Exit"), this);
	connect(actExit, SIGNAL(triggered()), this, SLOT(close()));
}


void fileLogMsgHandler(QtMsgType type, const char *msg)
{
	static FILE* file;
	if (file == 0)
	{
		// Create a file for appending in the user's temp directory
		QString filename = QDir::toNativeSeparators(QDesktopServices::storageLocation(QDesktopServices::TempLocation) + "/launchylog.txt");
		file = fopen(filename.toUtf8(), "a");
	}

	if (file)
	{
		switch (type)
		{
		case QtDebugMsg:
			fprintf(file, "Debug: %s\n", msg);
			break;
		case QtWarningMsg:
			fprintf(file, "Warning: %s\n", msg);
			break;
		case QtCriticalMsg:
			fprintf(file, "Critical: %s\n", msg);
			break;
		case QtFatalMsg:
			fprintf(file, "Fatal: %s\n", msg);
			abort();
			break;
		}

		fflush(file);
	}
}


int main(int argc, char *argv[])
{
	createApplication(argc, argv);

	qApp->setQuitOnLastWindowClosed(false);

	QStringList args = qApp->arguments();
	CommandFlags command = None;
	bool allowMultipleInstances = false;

	for (int i = 0; i < args.size(); ++i)
	{
		QString arg = args[i];
		if (arg.startsWith("-") || arg.startsWith("/"))
		{
			arg = arg.mid(1);
			if (arg.compare("rescue", Qt::CaseInsensitive) == 0)
			{
				command = ResetSkin | ResetPosition | ShowLaunchy;
			}
			else if (arg.compare("show", Qt::CaseInsensitive) == 0)
			{
				command |= ShowLaunchy;
			}
			else if (arg.compare("options", Qt::CaseInsensitive) == 0)
			{
				command |= ShowOptions;
			}
			else if (arg.compare("multiple", Qt::CaseInsensitive) == 0)
			{
				allowMultipleInstances = true;
			}
			else if (arg.compare("rescan", Qt::CaseInsensitive) == 0)
			{
				command |= Rescan;
			}
			else if (arg.compare("exit", Qt::CaseInsensitive) == 0)
			{
				command |= Exit;
			}
			else if (arg.compare("log", Qt::CaseInsensitive) == 0)
			{
				qInstallMsgHandler(fileLogMsgHandler);
			}
			else if (arg.compare("profile", Qt::CaseInsensitive) == 0)
			{
				if (++i < args.length())
				{
					settings.setProfileName(args[i]);
				}
			}
		}
	}

	if (!allowMultipleInstances && platform->isAlreadyRunning())
	{
		platform->sendInstanceCommand(command);
		exit(1);
	}

	QCoreApplication::setApplicationName("Launchy");
	QCoreApplication::setOrganizationDomain("Launchy");

	QString locale = QLocale::system().name();
	QTranslator translator;
	translator.load(QString("tr/launchy_" + locale));
	qApp->installTranslator(&translator);

	qApp->setStyleSheet("file:///:/resources/basicskin.qss");

#ifdef Q_WS_WIN
	LaunchyWidget* widget = createLaunchyWidget(command);
#else
	LaunchyWidget* widget = new LaunchyWidget(command);
#endif

	qApp->exec();

	delete widget;
	widget = NULL;

	delete platform;
	platform = NULL;
}
