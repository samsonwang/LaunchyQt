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
#include "options.h"
#include "main.h"
#include "globals.h"
#include "plugin_handler.h"
#include "FileBrowserDelegate.h"


QByteArray OptionsDialog::windowGeometry;
int OptionsDialog::currentTab;
int OptionsDialog::currentPlugin;


OptionsDialog::OptionsDialog(QWidget * parent) :
	QDialog(parent),
	directoryItemDelegate(this, FileBrowser::Directory)
{
	setupUi(this);

	curPlugin = -1;

	restoreGeometry(windowGeometry);
	tabWidget->setCurrentIndex(currentTab);

#ifdef Q_WS_WIN
	about_homepage->setText(about_homepage->text() + \
		"<p><br>If you would like to uninstall Launchy, please close Launchy and run \"Uninstall Launchy\" from the start menu.</br></p>");
#endif
	// Load General Options
	if (QSystemTrayIcon::isSystemTrayAvailable())
		genShowTrayIcon->setChecked(gSettings->value("GenOps/showtrayicon", true).toBool());
	else
		genShowTrayIcon->hide();
	genAlwaysShow->setChecked(gSettings->value("GenOps/alwaysshow", false).toBool());
	genAlwaysTop->setChecked(gSettings->value("GenOps/alwaystop", false).toBool());
	genPortable->setChecked(settings.isPortable());
	genHideFocus->setChecked(gSettings->value("GenOps/hideiflostfocus", false).toBool());
	genDecorateText->setChecked(gSettings->value("GenOps/decoratetext", false).toBool());
	int center = gSettings->value("GenOps/alwayscenter", 3).toInt();
	genHCenter->setChecked((center & 1) != 0);
	genVCenter->setChecked((center & 2) != 0);
	genShiftDrag->setChecked(gSettings->value("GenOps/dragmode", 0) == 1);
	genUpdateCheck->setChecked(gSettings->value("GenOps/updatecheck", true).toBool());
	genShowHidden->setChecked(gSettings->value("GenOps/showHiddenFiles", false).toBool());
	genShowNetwork->setChecked(gSettings->value("GenOps/showNetwork", true).toBool());
        genCondensed->setCurrentIndex(gSettings->value("GenOps/condensedView", 2).toInt());
	genAutoSuggestDelay->setValue(gSettings->value("GenOps/autoSuggestDelay", 1000).toInt());
	int updateInterval = gSettings->value("GenOps/updatetimer", 10).toInt();
	connect(genUpdateCatalog, SIGNAL(stateChanged(int)), this, SLOT(autoUpdateCheckChanged(int)));
	genUpdateMinutes->setValue(updateInterval);
	genUpdateCatalog->setChecked(updateInterval > 0);
	genMaxViewable->setValue(gSettings->value("GenOps/numviewable", 4).toInt());
	genNumResults->setValue(gSettings->value("GenOps/numresults", 10).toInt());
	genNumHistory->setValue(gSettings->value("GenOps/maxitemsinhistory", 20).toInt());
	genOpaqueness->setValue(gSettings->value("GenOps/opaqueness", 100).toInt());
	genFadeIn->setValue(gSettings->value("GenOps/fadein", 0).toInt());
	genFadeOut->setValue(gSettings->value("GenOps/fadeout", 20).toInt());
	connect(genOpaqueness, SIGNAL(sliderMoved(int)), gMainWidget, SLOT(setOpaqueness(int)));

#ifdef Q_WS_MAC
	metaKeys << tr("") << tr("Alt") << tr("Command") << tr("Shift") << tr("Control") <<
				tr("Command+Alt") << tr("Command+Shift") << tr("Command+Control");
#else
	metaKeys << tr("") << tr("Alt") << tr("Control") << tr("Shift") << tr("Win") <<
		tr("Ctrl+Alt") << tr("Ctrl+Shift") << tr("Ctrl+Win");
#endif
	iMetaKeys << Qt::NoModifier << Qt::AltModifier << Qt::ControlModifier << Qt::ShiftModifier << Qt::MetaModifier <<
		(Qt::ControlModifier | Qt::AltModifier) << (Qt::ControlModifier | Qt::ShiftModifier) << (Qt::ControlModifier | Qt::MetaModifier);

	actionKeys << tr("Space") << tr("Tab") << tr("Caps Lock") << tr("Backspace") << tr("Enter") << tr("Esc") <<
		tr("Insert") << tr("Delete") << tr("Home") << tr("End") << tr("Page Up") << tr("Page Down") <<
		tr("Print") << tr("Scroll Lock") << tr("Pause") << tr("Num Lock") <<
		tr("Up") << tr("Down") << tr("Left") << tr("Right") <<
		tr("F1") << tr("F2") << tr("F3") << tr("F4") << tr("F5") << tr("F6") << tr("F7") << tr("F8") <<
		tr("F9") << tr("F10") << tr("F11") << tr("F12") << tr("F13") << tr("F14") << tr("F15");

	iActionKeys << Qt::Key_Space << Qt::Key_Tab << Qt::Key_CapsLock << Qt::Key_Backspace << Qt::Key_Enter << Qt::Key_Escape <<
		Qt::Key_Insert << Qt::Key_Delete << Qt::Key_Home << Qt::Key_End << Qt::Key_PageUp << Qt::Key_PageDown <<
		Qt::Key_Print << Qt::Key_ScrollLock << Qt::Key_Pause << Qt::Key_NumLock <<
		Qt::Key_Up << Qt::Key_Down << Qt::Key_Left << Qt::Key_Right <<
		Qt::Key_F1 << Qt::Key_F2 << Qt::Key_F3 << Qt::Key_F4 << Qt::Key_F5 << Qt::Key_F6 << Qt::Key_F7 << Qt::Key_F8 <<
		Qt::Key_F9 << Qt::Key_F10 << Qt::Key_F11 << Qt::Key_F12 << Qt::Key_F13 << Qt::Key_F14 << Qt::Key_F15;

	for (int i = '0'; i <= '9'; i++) 
	{
		actionKeys << QString(QChar(i));
		iActionKeys << i;
	}
	for (int i = 'A'; i <= 'Z'; i++) 
	{
		actionKeys << QString(QChar(i));
		iActionKeys << i;
	}

	actionKeys << "`" << "-" << "=" << "[" << "]" <<
		";" << "'" << "#" << "\\" << "," << "." << "/";

	iActionKeys << '`' << '-' << '=' << '[' << ']' <<  
		';' << '\'' << '#' << '\\' << ',' << '.' << '/';

	// Find the current hotkey
	int hotkey = gMainWidget->getHotkey();
	int meta = hotkey & (Qt::AltModifier | Qt::MetaModifier | Qt::ShiftModifier | Qt::ControlModifier);
	hotkey &= ~(Qt::AltModifier | Qt::MetaModifier | Qt::ShiftModifier | Qt::ControlModifier);

	for (int i = 0; i < metaKeys.count(); ++i)
	{
		genModifierBox->addItem(metaKeys[i]);
		if (iMetaKeys[i] == meta) 
			genModifierBox->setCurrentIndex(i);
	}

	for (int i = 0; i < actionKeys.count(); ++i)
	{
		genKeyBox->addItem(actionKeys[i]);
		if (iActionKeys[i] == hotkey) 
			genKeyBox->setCurrentIndex(i);
	}

	// Load up web proxy settings
	genProxyHostname->setText(gSettings->value("WebProxy/hostAddress").toString());
	genProxyPort->setText(gSettings->value("WebProxy/port").toString());

	// Load up the skins list
	QString skinName = gSettings->value("GenOps/skin", "Default").toString();

	int skinRow = 0;
	QHash<QString, bool> knownSkins;
	foreach(QString szDir, settings.directory("skins"))
	{
		QDir dir(szDir);
		QStringList dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

		foreach(QString d, dirs)
		{
			if (knownSkins.contains(d))
				continue;
			knownSkins[d] = true;

			QFile f(dir.absolutePath() + "/" + d + "/style.qss");
			// Only look for 2.0+ skins
			if (!f.exists())
				continue;

			QListWidgetItem* item = new QListWidgetItem(d);
			skinList->addItem(item);

			if (skinName.compare(d, Qt::CaseInsensitive) == 0)
				skinRow = skinList->count() - 1;
		}
	}
	skinList->setCurrentRow(skinRow);

	connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

	// Load the directories and types
	catDirectories->setItemDelegate(&directoryItemDelegate);

	connect(catDirectories, SIGNAL(currentRowChanged(int)), this, SLOT(dirRowChanged(int)));
	connect(catDirectories, SIGNAL(dragEnter(QDragEnterEvent*)), this, SLOT(catDirDragEnter(QDragEnterEvent*)));
	connect(catDirectories, SIGNAL(drop(QDropEvent*)), this, SLOT(catDirDrop(QDropEvent*)));
	connect(catDirectories, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(catDirItemChanged(QListWidgetItem*)));
	connect(catDirPlus, SIGNAL(clicked(bool)), this, SLOT(catDirPlusClicked(bool)));
	connect(catDirMinus, SIGNAL(clicked(bool)), this, SLOT(catDirMinusClicked(bool)));
	connect(catTypes, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(catTypesItemChanged(QListWidgetItem*)));
	connect(catTypesPlus, SIGNAL(clicked(bool)), this, SLOT(catTypesPlusClicked(bool)));
	connect(catTypesMinus, SIGNAL(clicked(bool)), this, SLOT(catTypesMinusClicked(bool)));
	connect(catCheckDirs, SIGNAL(stateChanged(int)), this, SLOT(catTypesDirChanged(int)));
	connect(catCheckBinaries, SIGNAL(stateChanged(int)), this, SLOT(catTypesExeChanged(int)));
	connect(catDepth, SIGNAL(valueChanged(int)),this, SLOT(catDepthChanged(int)));
	connect(catRescan, SIGNAL(clicked(bool)), this, SLOT(catRescanClicked(bool)));
	catProgress->setVisible(false);

	memDirs = SettingsManager::readCatalogDirectories();
	for (int i = 0; i < memDirs.count(); ++i)
	{
		catDirectories->addItem(memDirs[i].name);
		QListWidgetItem* it = catDirectories->item(i);
		it->setFlags(it->flags() | Qt::ItemIsEditable);
	}
	if (catDirectories->count() > 0)
		catDirectories->setCurrentRow(0);

	genOpaqueness->setRange(15, 100);

	if (gMainWidget->catalog != NULL)
	{
		catSize->setText(tr("Index has %n item(s)", "", gMainWidget->catalog->count()));
	}

	connect(gBuilder, SIGNAL(catalogIncrement(int)), this, SLOT(catalogProgressUpdated(int)));
	connect(gBuilder, SIGNAL(catalogFinished()), this, SLOT(catalogBuilt()));
	if (gBuilder->isRunning())
	{
		catalogProgressUpdated(gBuilder->getProgress());
	}

	// Load up the plugins		
	connect(plugList, SIGNAL(currentRowChanged(int)), this, SLOT(pluginChanged(int)));
	connect(plugList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(pluginItemChanged(QListWidgetItem*)));
	gMainWidget->plugins.loadPlugins();
	foreach(PluginInfo info, gMainWidget->plugins.getPlugins())
	{
		plugList->addItem(info.name);
		QListWidgetItem* item = plugList->item(plugList->count()-1);
		item->setData(Qt::UserRole, info.id);
		item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
		if (info.loaded)
			item->setCheckState(Qt::Checked);
		else
			item->setCheckState(Qt::Unchecked);
	}
	plugList->sortItems();
	if (plugList->count() > 0)
	{
		plugList->setCurrentRow(currentPlugin);
	}
	aboutVer->setText(tr("This is Launchy version %1").arg(LAUNCHY_VERSION_STRING));
	catDirectories->installEventFilter(this);

	needRescan = false;
}


OptionsDialog::~OptionsDialog()
{
	if (gBuilder != NULL)
	{
		disconnect(gBuilder, SIGNAL(catalogIncrement(int)), this, SLOT(catalogProgressUpdated(int)));
		disconnect(gBuilder, SIGNAL(catalogFinished()), this, SLOT(catalogBuilt()));
	}

	currentTab = tabWidget->currentIndex();
	windowGeometry = saveGeometry();
}


void OptionsDialog::setVisible(bool visible)
{
	QDialog::setVisible(visible);

	if (visible)
	{
		connect(skinList, SIGNAL(currentTextChanged(const QString)), this, SLOT(skinChanged(const QString)));
		skinChanged(skinList->currentItem()->text());
	}
}


void OptionsDialog::accept()
{
	if (gSettings == NULL)
		return;

	// See if the new hotkey works, if not we're not leaving the dialog.
	QKeySequence hotkey(iMetaKeys[genModifierBox->currentIndex()] + iActionKeys[genKeyBox->currentIndex()]);
	if (!gMainWidget->setHotkey(hotkey))
	{
		QMessageBox::warning(this, tr("Launchy"), tr("The hotkey %1 is already in use, please select another.").arg(hotkey.toString()));
		return;
	}

	gSettings->setValue("GenOps/hotkey", hotkey.count() > 0 ? hotkey[0] : 0);

	// Save General Options
	gSettings->setValue("GenOps/showtrayicon", genShowTrayIcon->isChecked());
	gSettings->setValue("GenOps/alwaysshow", genAlwaysShow->isChecked());
	gSettings->setValue("GenOps/alwaystop", genAlwaysTop->isChecked());
	gSettings->setValue("GenOps/updatecheck", genUpdateCheck->isChecked());
	gSettings->setValue("GenOps/decoratetext", genDecorateText->isChecked());
	gSettings->setValue("GenOps/hideiflostfocus", genHideFocus->isChecked());
	gSettings->setValue("GenOps/alwayscenter", (genHCenter->isChecked() ? 1 : 0) | (genVCenter->isChecked() ? 2 : 0));
	gSettings->setValue("GenOps/dragmode", genShiftDrag->isChecked() ? 1 : 0);
	gSettings->setValue("GenOps/showHiddenFiles", genShowHidden->isChecked());
	gSettings->setValue("GenOps/showNetwork", genShowNetwork->isChecked());
	gSettings->setValue("GenOps/condensedView", genCondensed->currentIndex());
	gSettings->setValue("GenOps/autoSuggestDelay", genAutoSuggestDelay->value());
	gSettings->setValue("GenOps/updatetimer", genUpdateCatalog->isChecked() ? genUpdateMinutes->value() : 0);
	gSettings->setValue("GenOps/numviewable", genMaxViewable->value());
	gSettings->setValue("GenOps/numresults", genNumResults->value());
	gSettings->setValue("GenOps/maxitemsinhistory", genNumHistory->value());
	gSettings->setValue("GenOps/opaqueness", genOpaqueness->value());
	gSettings->setValue("GenOps/fadein", genFadeIn->value());
	gSettings->setValue("GenOps/fadeout", genFadeOut->value());

	gSettings->setValue("WebProxy/hostAddress", genProxyHostname->text());
	gSettings->setValue("WebProxy/port", genProxyPort->text());

	// Apply General Options
	settings.setPortable(genPortable->isChecked());
	gMainWidget->startUpdateTimer();
	gMainWidget->setSuggestionListMode(genCondensed->currentIndex());
	gMainWidget->loadOptions();

	// Apply Directory Options
	SettingsManager::writeCatalogDirectories(memDirs);

	if (curPlugin >= 0)
	{
		QListWidgetItem* item = plugList->item(curPlugin);
		gMainWidget->plugins.endDialog(item->data(Qt::UserRole).toUInt(), true);
	}

	gSettings->sync();

	QDialog::accept();

	// Now save the options that require launchy to be shown or redrawed
	bool show = gMainWidget->setAlwaysShow(genAlwaysShow->isChecked());
	show |= gMainWidget->setAlwaysTop(genAlwaysTop->isChecked());

	gMainWidget->setOpaqueness(genOpaqueness->value());

	// Apply Skin Options
	QString prevSkinName = gSettings->value("GenOps/skin", "Default").toString();
	QString skinName = skinList->currentItem()->text();
	if (skinList->currentRow() >= 0 && skinName != prevSkinName)
	{
		gSettings->setValue("GenOps/skin", skinName);
		gMainWidget->setSkin(skinName);
		show = false;
	}

	if (needRescan)
		gMainWidget->buildCatalog();

	if (show)
		gMainWidget->showLaunchy();
}


void OptionsDialog::reject()
{
	if (curPlugin >= 0)
	{
		QListWidgetItem* item = plugList->item(curPlugin);
		gMainWidget->plugins.endDialog(item->data(Qt::UserRole).toUInt(), false);
	}

	QDialog::reject();
}


void OptionsDialog::tabChanged(int tab)
{
	tab = tab; // Compiler warning
	// Redraw the current skin (necessary because of dialog resizing issues)
	if (tabWidget->currentWidget()->objectName() == "Skins")
	{
		skinChanged(skinList->currentItem()->text());
	}
	else if (tabWidget->currentWidget()->objectName() == "Plugins")
	{
		// We've currently no way of checking if a plugin requires a catalog rescan
		// so assume that we need one if the user has viewed the plugins tab
		needRescan = true;
	}
}


void OptionsDialog::autoUpdateCheckChanged(int state)
{
	genUpdateMinutes->setEnabled(state > 0);
	if (genUpdateMinutes->value() <= 0)
		genUpdateMinutes->setValue(10);
}


void OptionsDialog::skinChanged(const QString& newSkin)
{
	if (newSkin.count() == 0)
		return;

	// Find the skin with this name
	QString directory = settings.skinPath(newSkin);

	// Load up the author file
	if (directory.length() == 0)
	{
		authorInfo->setText("");
		return;
	}
	QFile author(directory + "author.txt"); 
	if (!author.open(QIODevice::ReadOnly))
	{
		authorInfo->setText("");
	}

	QTextStream in(&author);

	QString line, total;
	line = in.readLine();
	while (!line.isNull())
	{
		total += line + "\n";
		line = in.readLine();
	}
	authorInfo->setText(total);
	author.close();

	// Set the image preview
	QPixmap pix;
	if (pix.load(directory + "background.png"))
	{
		if (!platform->supportsAlphaBorder() && QFile::exists(directory + "background_nc.png"))
			pix.load(directory + "background_nc.png");
		if (pix.hasAlpha())
			pix.setMask(pix.mask());
		if (!platform->supportsAlphaBorder() && QFile::exists(directory + "mask_nc.png"))
			pix.setMask(QPixmap(directory + "mask_nc.png"));
		else if (QFile::exists(directory + "mask.png"))
			pix.setMask(QPixmap(directory + "mask.png"));

		if (platform->supportsAlphaBorder())
		{
			// Compose the alpha image with the background
			QImage sourceImage(pix.toImage());
			QImage destinationImage(directory + "alpha.png");
			QImage resultImage(destinationImage.size(), QImage::Format_ARGB32_Premultiplied);

			QPainter painter(&resultImage);
			painter.setCompositionMode(QPainter::CompositionMode_Source);
			painter.fillRect(resultImage.rect(), Qt::transparent);
			painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
			painter.drawImage(0, 0, sourceImage);
			painter.drawImage(0, 0, destinationImage);
			painter.end();

			pix = QPixmap::fromImage(resultImage);
			QPixmap scaled = pix.scaled(skinPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
			skinPreview->setPixmap(scaled);
		}
	}
	else if (pix.load(directory + "frame.png"))
	{
		QPixmap scaled = pix.scaled(skinPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
		skinPreview->setPixmap(scaled);
	}
	else
	{
		skinPreview->clear();
	}
}


void OptionsDialog::pluginChanged(int row)
{
	plugBox->setTitle(tr("Plugin options"));

	if (plugBox->layout() != NULL)
		for (int i = 1; i < plugBox->layout()->count(); i++) 
			plugBox->layout()->removeItem(plugBox->layout()->itemAt(i));

	// Close any current plugin dialogs
	if (curPlugin >= 0)
	{
		QListWidgetItem* item = plugList->item(curPlugin);
		gMainWidget->plugins.endDialog(item->data(Qt::UserRole).toUInt(), true);
	}

	// Open the new plugin dialog
	curPlugin = row;
	currentPlugin = row;
	if (row >= 0)
	{
		loadPluginDialog(plugList->item(row));
	}
}


void OptionsDialog::loadPluginDialog(QListWidgetItem* item)
{
	QWidget* win = gMainWidget->plugins.doDialog(plugBox, item->data(Qt::UserRole).toUInt());
	if (win != NULL)
	{
		if (plugBox->layout() != NULL)
			plugBox->layout()->addWidget(win);
		win->show();
		if (win->windowTitle() != "Form")
			plugBox->setTitle(win->windowTitle());
	}
}


void OptionsDialog::pluginItemChanged(QListWidgetItem* iz)
{
	int row = plugList->currentRow();
	if (row == -1)
		return;

	// Close any current plugin dialogs
	if (curPlugin >= 0)
	{
		QListWidgetItem* item = plugList->item(curPlugin);
		gMainWidget->plugins.endDialog(item->data(Qt::UserRole).toUInt(), true);
	}

	// Write out the new config
	gSettings->beginWriteArray("plugins");
	for (int i = 0; i < plugList->count(); i++)
	{
		QListWidgetItem* item = plugList->item(i);
		gSettings->setArrayIndex(i);
		gSettings->setValue("id", item->data(Qt::UserRole).toUInt());
		if (item->checkState() == Qt::Checked)
		{
			gSettings->setValue("load", true);
		}
		else
		{
			gSettings->setValue("load", false);
		}
	}
	gSettings->endArray();

	// Reload the plugins
	gMainWidget->plugins.loadPlugins();

	// If enabled, reload the dialog
	if (iz->checkState() == Qt::Checked)
	{
		loadPluginDialog(iz);
	}
}


void OptionsDialog::catalogProgressUpdated(int value)
{
	catSize->setVisible(false);
	catProgress->setValue(value);
	catProgress->setVisible(true);
	catRescan->setEnabled(false);
}


void OptionsDialog::catalogBuilt()
{
	catProgress->setVisible(false);
	catRescan->setEnabled(true);
	if (gMainWidget->catalog != NULL)
	{
		catSize->setText(tr("Index has %n items", "", gMainWidget->catalog->count()));
		catSize->setVisible(true);
	}
}


void OptionsDialog::catRescanClicked(bool val)
{
	val = val; // Compiler warning

	// Apply Directory Options
	SettingsManager::writeCatalogDirectories(memDirs);

	needRescan = false;
	catRescan->setEnabled(false);
	gMainWidget->buildCatalog();
}


void OptionsDialog::catTypesDirChanged(int state)
{
	state = state; // Compiler warning
	int row = catDirectories->currentRow();
	if (row == -1)
		return;
	memDirs[row].indexDirs = catCheckDirs->isChecked();

	needRescan = true;
}


void OptionsDialog::catTypesExeChanged(int state)
{
	state = state; // Compiler warning
	int row = catDirectories->currentRow();
	if (row == -1)
		return;
	memDirs[row].indexExe = catCheckBinaries->isChecked();

	needRescan = true;
}


void OptionsDialog::catDirItemChanged(QListWidgetItem* item)
{
	int row = catDirectories->currentRow();
	if (row == -1)
		return;
	if (item != catDirectories->item(row))
		return;	

	memDirs[row].name = item->text();

	needRescan = true;
}


void OptionsDialog::catDirDragEnter(QDragEnterEvent *event)
{
	const QMimeData* mimeData = event->mimeData();
	if (mimeData && mimeData->hasUrls())
		event->acceptProposedAction();
}


void OptionsDialog::catDirDrop(QDropEvent *event)
{
	const QMimeData* mimeData = event->mimeData();
	if (mimeData && mimeData->hasUrls())
	{
		foreach(QUrl url, mimeData->urls())
		{
			QFileInfo info(url.toLocalFile());
			if(info.exists() && info.isDir())
			{
				addDirectory(info.filePath());
			}
		}
	}
}


void OptionsDialog::dirRowChanged(int row)
{
	if (row == -1)
		return;

	catTypes->clear();
	foreach(QString str, memDirs[row].types)
	{
		QListWidgetItem* item = new QListWidgetItem(str, catTypes);
		item->setFlags(item->flags() | Qt::ItemIsEditable);
	}
	catCheckDirs->setChecked(memDirs[row].indexDirs);
	catCheckBinaries->setChecked(memDirs[row].indexExe);
	catDepth->setValue(memDirs[row].depth);

	needRescan = true;
}


void OptionsDialog::catDirMinusClicked(bool c)
{
	c = c; // Compiler warning
	int dirRow = catDirectories->currentRow();

	delete catDirectories->takeItem(dirRow);
	catTypes->clear();

	memDirs.removeAt(dirRow);

	if (dirRow >= catDirectories->count() && catDirectories->count() > 0)
	{
		catDirectories->setCurrentRow(catDirectories->count() - 1);
		dirRowChanged(catDirectories->count() - 1);
	}
}


void OptionsDialog::catDirPlusClicked(bool c)
{
	c = c; // Compiler warning
	addDirectory("", true);
}


void OptionsDialog::addDirectory(const QString& directory, bool edit)
{
	QString nativeDir = QDir::toNativeSeparators(directory);
	Directory dir(nativeDir);
	memDirs.append(dir);

	catTypes->clear();
	QListWidgetItem* item = new QListWidgetItem(nativeDir, catDirectories);
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	catDirectories->setCurrentItem(item);
	if (edit)
	{
		catDirectories->editItem(item);
	}

	needRescan = true;
}


void OptionsDialog::catTypesItemChanged(QListWidgetItem* item)
{
	Q_UNUSED(item);

	int row = catDirectories->currentRow();
	if (row == -1)
		return;
	int typesRow = catTypes->currentRow();
	if (typesRow == -1)
		return;

	memDirs[row].types[typesRow] = catTypes->item(typesRow)->text();
	
	needRescan = true;
}


void OptionsDialog::catTypesPlusClicked(bool c)
{
	c = c; // Compiler warning
	int row = catDirectories->currentRow();
	if (row == -1)
		return;

	memDirs[row].types << "";
	QListWidgetItem* item = new QListWidgetItem(catTypes);
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	catTypes->setCurrentItem(item);
	catTypes->editItem(item);

	needRescan = true;
}


void OptionsDialog::catTypesMinusClicked(bool c)
{
	c = c; // Compiler warning
	int dirRow = catDirectories->currentRow();
	if (dirRow == -1)
		return;

	int typesRow = catTypes->currentRow();
	if (typesRow == -1)
		return;

	memDirs[dirRow].types.removeAt(typesRow);
	delete catTypes->takeItem(typesRow);

	if (typesRow >= catTypes->count() && catTypes->count() > 0)
		catTypes->setCurrentRow(catTypes->count() - 1);

	needRescan = true;
}


void OptionsDialog::catDepthChanged(int d)
{
	int row = catDirectories->currentRow();
	if (row != -1)
		memDirs[row].depth = d;

	needRescan = true;
}
