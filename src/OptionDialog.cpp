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

#include "OptionDialog.h"
#include <QMessageBox>
#include "AppBase.h"
#include "ui_OptionDialog.h"
#include "LaunchyWidget.h"
#include "GlobalVar.h"
#include "PluginHandler.h"
#include "FileBrowserDelegate.h"
#include "SettingsManager.h"
#include "Logger.h"
#include "CatalogBuilder.h"
#include "OptionItem.h"

// to store QNetworkProxy::ProxyType in QVariant
Q_DECLARE_METATYPE(QNetworkProxy::ProxyType)

namespace launchy {

// for qt flags
// check this page https://stackoverflow.com/questions/10755058/qflags-enum-type-conversion-fails-all-of-a-sudden
using ::operator|;

QByteArray OptionDialog::s_windowGeometry;
int OptionDialog::s_currentTab;
int OptionDialog::s_currentPlugin;

OptionDialog::OptionDialog(QWidget * parent)
    : QDialog(parent),
      m_pUi(new Ui::OptionDialog),
      m_directoryItemDelegate(this, FileBrowser::Directory) {

    m_pUi->setupUi(this);

    Qt::WindowFlags windowsFlags = windowFlags();
    windowsFlags = windowsFlags & (~Qt::WindowContextHelpButtonHint);
    windowsFlags = windowsFlags | Qt::MSWindowsFixedSizeDialogHint;
    setWindowFlags(windowsFlags);

    m_curPlugin = -1;

    restoreGeometry(s_windowGeometry);
    m_pUi->tabWidget->setCurrentIndex(s_currentTab);

    // Load General Options		
    m_pUi->genAlwaysShow->setChecked(g_settings->value(OPSTION_ALWAYSSHOW, OPSTION_ALWAYSSHOW_DEFAULT).toBool());
    m_pUi->genAlwaysTop->setChecked(g_settings->value(OPSTION_ALWAYSTOP, OPSTION_ALWAYSTOP_DEFAULT).toBool());
    m_pUi->genPortable->setChecked(SettingsManager::instance().isPortable());
    m_pUi->genHideFocus->setChecked(g_settings->value(OPSTION_HIDEIFLOSTFOCUS, OPSTION_HIDEIFLOSTFOCUS_DEFAULT).toBool());
    m_pUi->genDecorateText->setChecked(g_settings->value(OPSTION_DECORATETEXT, OPSTION_DECORATETEXT_DEFAULT).toBool());

    int center = g_settings->value(OPSTION_ALWAYSCENTER, OPSTION_ALWAYSCENTER_DEFAULT).toInt();
    m_pUi->genHCenter->setChecked((center & 1) != 0);
    m_pUi->genVCenter->setChecked((center & 2) != 0);

    m_pUi->genShiftDrag->setChecked(g_settings->value(OPSTION_DRAGMODE, OPSTION_DRAGMODE_DEFAULT).toBool());
    m_pUi->genLog->setCurrentIndex(g_settings->value(OPSTION_LOGLEVEL, OPSTION_LOGLEVEL_DEFAULT).toInt());
    connect(m_pUi->genLog, SIGNAL(currentIndexChanged(int)), this, SLOT(logLevelChanged(int)));

    m_pUi->genShowHidden->setChecked(g_settings->value(OPSTION_SHOWHIDDENFILES, OPSTION_SHOWHIDDENFILES_DEFAULT).toBool());
    m_pUi->genShowNetwork->setChecked(g_settings->value(OPSTION_SHOWNETWORK, OPSTION_SHOWNETWORK_DEFAULT).toBool());
    m_pUi->genCondensed->setCurrentIndex(g_settings->value(OPSTION_CONDENSEDVIEW, OPSTION_CONDENSEDVIEW_DEFAULT).toInt());
    m_pUi->genAutoSuggestDelay->setValue(g_settings->value(OPSTION_AUTOSUGGESTDELAY, OPSTION_AUTOSUGGESTDELAY_DEFAULT).toInt());

    int updateInterval = g_settings->value(OPSTION_UPDATETIMER, OPSTION_UPDATETIMER_DEFAULT).toInt();
    connect(m_pUi->genUpdateCatalog, SIGNAL(stateChanged(int)), this, SLOT(autoUpdateCheckChanged(int)));
    m_pUi->genUpdateMinutes->setValue(updateInterval);
    m_pUi->genUpdateCatalog->setChecked(updateInterval > 0);
    m_pUi->genMaxViewable->setValue(g_settings->value(OPSTION_NUMVIEWABLE, OPSTION_NUMVIEWABLE_DEFAULT).toInt());
    m_pUi->genNumResults->setValue(g_settings->value(OPSTION_NUMRESULT, OPSTION_NUMRESULT_DEFAULT).toInt());
    m_pUi->genNumHistory->setValue(g_settings->value(OPSTION_MAXITEMSINHISTORY, OPSTION_MAXITEMSINHISTORY_DEFAULT).toInt());
    m_pUi->genOpaqueness->setValue(g_settings->value(OPSTION_OPAQUENESS, OPSTION_OPAQUENESS_DEFAULT).toInt());
    m_pUi->genFadeIn->setValue(g_settings->value(OPSTION_FADEIN, OPSTION_FADEIN_DEFAULT).toInt());
    m_pUi->genFadeOut->setValue(g_settings->value(OPSTION_FADEOUT, OPSTION_FADEOUT_DEFAULT).toInt());
    connect(m_pUi->genOpaqueness, SIGNAL(sliderMoved(int)), g_mainWidget.data(), SLOT(setOpaqueness(int)));

#ifdef Q_OS_MAC
    metaKeys << QString("") << QString("Alt") << QString("Command") << QString("Shift") << QString("Control")
        << QString("Command+Alt") << QString("Command+Shift") << QString("Command+Control");
#else
    metaKeys << QString("") << QString("Alt") << QString("Control") << QString("Shift") << QString("Win")
        << QString("Ctrl+Alt") << QString("Ctrl+Shift") << QString("Ctrl+Win");
#endif
    iMetaKeys << Qt::NoModifier << Qt::AltModifier << Qt::ControlModifier << Qt::ShiftModifier << Qt::MetaModifier
        << (Qt::ControlModifier | Qt::AltModifier) << (Qt::ControlModifier | Qt::ShiftModifier)
        << (Qt::ControlModifier | Qt::MetaModifier);

    actionKeys << QString("Space") << QString("Tab") << QString("Caps Lock") << QString("Backspace")
        << QString("Enter") << QString("Esc") << QString("Insert") << QString("Delete") << QString("Home")
        << QString("End") << QString("Page Up") << QString("Page Down") << QString("Print") << QString("Scroll Lock")
        << QString("Pause") << QString("Num Lock")
        << tr("Up") << tr("Down") << tr("Left") << tr("Right")
        << QString("F1") << QString("F2") << QString("F3") << QString("F4") << QString("F5")
        << QString("F6") << QString("F7") << QString("F8") << QString("F9") << QString("F10")
        << QString("F11") << QString("F12") << QString("F13") << QString("F14") << QString("F15");

    iActionKeys << Qt::Key_Space << Qt::Key_Tab << Qt::Key_CapsLock << Qt::Key_Backspace << Qt::Key_Enter << Qt::Key_Escape <<
        Qt::Key_Insert << Qt::Key_Delete << Qt::Key_Home << Qt::Key_End << Qt::Key_PageUp << Qt::Key_PageDown <<
        Qt::Key_Print << Qt::Key_ScrollLock << Qt::Key_Pause << Qt::Key_NumLock <<
        Qt::Key_Up << Qt::Key_Down << Qt::Key_Left << Qt::Key_Right <<
        Qt::Key_F1 << Qt::Key_F2 << Qt::Key_F3 << Qt::Key_F4 << Qt::Key_F5 << Qt::Key_F6 << Qt::Key_F7 << Qt::Key_F8 <<
        Qt::Key_F9 << Qt::Key_F10 << Qt::Key_F11 << Qt::Key_F12 << Qt::Key_F13 << Qt::Key_F14 << Qt::Key_F15;

    for (int i = '0'; i <= '9'; ++i) {
        actionKeys << QString(QChar(i));
        iActionKeys << i;
    }

    for (int i = 'A'; i <= 'Z'; ++i) {
        actionKeys << QString(QChar(i));
        iActionKeys << i;
    }

    actionKeys << "`" << "-" << "=" << "[" << "]" <<
        ";" << "'" << "#" << "\\" << "," << "." << "/";

    iActionKeys << '`' << '-' << '=' << '[' << ']' <<
        ';' << '\'' << '#' << '\\' << ',' << '.' << '/';

    // Find the current hotkey
    int hotkey = g_mainWidget->getHotkey();
    int meta = hotkey & (Qt::AltModifier | Qt::MetaModifier | Qt::ShiftModifier | Qt::ControlModifier);
    hotkey &= ~(Qt::AltModifier | Qt::MetaModifier | Qt::ShiftModifier | Qt::ControlModifier);

    for (int i = 0; i < metaKeys.count(); ++i) {
        m_pUi->genModifierBox->addItem(metaKeys[i]);
        if (iMetaKeys[i] == meta)
            m_pUi->genModifierBox->setCurrentIndex(i);
    }

    for (int i = 0; i < actionKeys.count(); ++i) {
        m_pUi->genKeyBox->addItem(actionKeys[i]);
        if (iActionKeys[i] == hotkey)
            m_pUi->genKeyBox->setCurrentIndex(i);
    }

    // Load up web proxy settings
    m_pUi->genProxyHostname->setText(g_settings->value(OPSTION_HOSTADDRESS).toString());
    m_pUi->genProxyPort->setText(g_settings->value(OPSTION_HOSTPORT).toString());

    // Load up the skins list
    QString skinName = g_settings->value(OPSTION_SKIN, OPSTION_SKIN_DEFAULT).toString();

    int skinRow = 0;
    QHash<QString, bool> knownSkins;
    foreach(QString szDir, SettingsManager::instance().directory("skins")) {
        QDir dir(szDir);
        QStringList dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

        foreach(QString d, dirs) {
            if (knownSkins.contains(d))
                continue;
            knownSkins[d] = true;

            QFile f(dir.absolutePath() + "/" + d + "/style.qss");
            // Only look for 2.0+ skins
            if (!f.exists())
                continue;

            QListWidgetItem* item = new QListWidgetItem(d);
            m_pUi->skinList->addItem(item);

            if (skinName.compare(d, Qt::CaseInsensitive) == 0)
                skinRow = m_pUi->skinList->count() - 1;
        }
    }
    m_pUi->skinList->setCurrentRow(skinRow);

    connect(m_pUi->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));

    initCatalogWidget();

    initPluginsWidget();

    initUpdateWidget();

    initProxyWidget();

    // About
    m_pUi->aboutVer->setText(tr("This is Launchy %1").arg(LAUNCHY_VERSION_STRING));

    m_needRescan = false;
}


OptionDialog::~OptionDialog() {
    if (g_builder != NULL) {
        disconnect(g_builder.data(), SIGNAL(catalogIncrement(int)), this, SLOT(catalogProgressUpdated(int)));
        disconnect(g_builder.data(), SIGNAL(catalogFinished()), this, SLOT(catalogBuilt()));
    }

    s_currentTab = m_pUi->tabWidget->currentIndex();
    s_windowGeometry = saveGeometry();

    delete m_pUi;
    m_pUi = nullptr;
}


void OptionDialog::setVisible(bool visible) {
    QDialog::setVisible(visible);

    if (visible) {
        connect(m_pUi->skinList, SIGNAL(currentTextChanged(const QString)),
                this, SLOT(skinChanged(const QString)));
        skinChanged(m_pUi->skinList->currentItem()->text());
    }
}


void OptionDialog::accept() {
    if (g_settings.isNull()) {
        qWarning() << "OptionDialog::accept, fail to save setting.";
        return;
    }

    // See if the new hotkey works, if not we're not leaving the dialog.
    QKeySequence hotkey(iMetaKeys[m_pUi->genModifierBox->currentIndex()] + iActionKeys[m_pUi->genKeyBox->currentIndex()]);
    if (!g_mainWidget->setHotkey(hotkey)) {
        QMessageBox::warning(this, tr("Launchy"), 
                             tr("The hotkey %1 is already in use, please select another.").arg(hotkey.toString()));
        return;
    }

    g_settings->setValue(OPSTION_HOTKEY, hotkey.count() > 0 ? hotkey[0] : OPSTION_HOTKEY_DEFAULT);

    // Save General Options
//	g_settings->setValue("GenOps/showtrayicon", genShowTrayIcon->isChecked());
    g_settings->setValue(OPSTION_ALWAYSSHOW, m_pUi->genAlwaysShow->isChecked());
    g_settings->setValue(OPSTION_ALWAYSTOP, m_pUi->genAlwaysTop->isChecked());
    g_settings->setValue(OPSTION_LOGLEVEL, m_pUi->genLog->currentIndex());
    g_settings->setValue(OPSTION_DECORATETEXT, m_pUi->genDecorateText->isChecked());
    g_settings->setValue(OPSTION_HIDEIFLOSTFOCUS, m_pUi->genHideFocus->isChecked());
    g_settings->setValue(OPSTION_ALWAYSCENTER, (m_pUi->genHCenter->isChecked() ? 1 : 0) | (m_pUi->genVCenter->isChecked() ? 2 : 0));
    g_settings->setValue(OPSTION_DRAGMODE, m_pUi->genShiftDrag->isChecked());
    g_settings->setValue(OPSTION_SHOWHIDDENFILES, m_pUi->genShowHidden->isChecked());
    g_settings->setValue(OPSTION_SHOWNETWORK, m_pUi->genShowNetwork->isChecked());
    g_settings->setValue(OPSTION_CONDENSEDVIEW, m_pUi->genCondensed->currentIndex());
    g_settings->setValue(OPSTION_AUTOSUGGESTDELAY, m_pUi->genAutoSuggestDelay->value());
    g_settings->setValue(OPSTION_UPDATETIMER, m_pUi->genUpdateCatalog->isChecked() ? m_pUi->genUpdateMinutes->value() : 0);
    g_settings->setValue(OPSTION_NUMVIEWABLE, m_pUi->genMaxViewable->value());
    g_settings->setValue(OPSTION_NUMRESULT, m_pUi->genNumResults->value());
    g_settings->setValue(OPSTION_MAXITEMSINHISTORY, m_pUi->genNumHistory->value());
    g_settings->setValue(OPSTION_OPAQUENESS, m_pUi->genOpaqueness->value());
    g_settings->setValue(OPSTION_FADEIN, m_pUi->genFadeIn->value());
    g_settings->setValue(OPSTION_FADEOUT, m_pUi->genFadeOut->value());

    g_settings->setValue(OPSTION_HOSTADDRESS, m_pUi->genProxyHostname->text());
    g_settings->setValue(OPSTION_HOSTPORT, m_pUi->genProxyPort->text());

    // Apply General Options
    SettingsManager::instance().setPortable(m_pUi->genPortable->isChecked());
    g_mainWidget->startUpdateTimer();
    g_mainWidget->setAlternativeListMode(m_pUi->genCondensed->currentIndex());
    g_mainWidget->loadOptions();

    saveCatalogSettings();

    savePluginsSettings();

    saveUpdateSettings();

    saveProxySettings();

    g_settings->sync();

    QDialog::accept();

    // Now save the options that require launchy to be shown or redrawed
    bool show = g_mainWidget->setAlwaysShow(m_pUi->genAlwaysShow->isChecked());
    show |= g_mainWidget->setAlwaysTop(m_pUi->genAlwaysTop->isChecked());

    g_mainWidget->setOpaqueness(m_pUi->genOpaqueness->value());

    // Apply Skin Options
    QString prevSkinName = g_settings->value(OPSTION_SKIN, OPSTION_SKIN_DEFAULT).toString();
    QString currentSkinName = m_pUi->skinList->currentItem()->text();
    if (m_pUi->skinList->currentRow() >= 0 && currentSkinName != prevSkinName) {
        g_settings->setValue(OPSTION_SKIN, currentSkinName);
        g_mainWidget->setSkin(currentSkinName);
        show |= true;
    }

    if (m_needRescan) {
        g_mainWidget->buildCatalog();
    }

    if (show) {
        g_mainWidget->showLaunchy();
    }
}


void OptionDialog::reject() {
    if (m_curPlugin >= 0) {
        QListWidgetItem* item = m_pUi->plugList->item(m_curPlugin);
        g_pluginHandler->endDialog(item->data(Qt::UserRole).toUInt(), false);
    }

    QDialog::reject();
}


void OptionDialog::tabChanged(int tab) {
    Q_UNUSED(tab)
    // Redraw the current skin (necessary because of dialog resizing issues)
    if (m_pUi->tabWidget->currentWidget()->objectName() == "Skins") {
        skinChanged(m_pUi->skinList->currentItem()->text());
    }
    else if (m_pUi->tabWidget->currentWidget()->objectName() == "Plugins") {
        // We've currently no way of checking if a plugin requires a catalog rescan
        // so assume that we need one if the user has viewed the plugins tab
        m_needRescan = true;
    }
}

void OptionDialog::autoUpdateCheckChanged(int state) {
    m_pUi->genUpdateMinutes->setEnabled(state > 0);
    if (m_pUi->genUpdateMinutes->value() <= 0)
        m_pUi->genUpdateMinutes->setValue(10);
}


void OptionDialog::skinChanged(const QString& newSkin)
{
    if (newSkin.count() == 0)
        return;

    // Find the skin with this name
    QString directory = SettingsManager::instance().skinPath(newSkin);

    // Load up the author file
    if (directory.length() == 0) {
        m_pUi->authorInfo->setText("");
        return;
    }
    QFile fileAuthor(directory + "author.txt");
    if (!fileAuthor.open(QIODevice::ReadOnly)) {
        m_pUi->authorInfo->setText("");
    }

    QString line, total;
    QTextStream in(&fileAuthor);
    line = in.readLine();
    while (!line.isNull()) {
        total += line + "\n";
        line = in.readLine();
    }
    m_pUi->authorInfo->setText(total);
    fileAuthor.close();

    // Set the image preview
    QPixmap pix;
    if (pix.load(directory + "background.png")) {
        if (!g_app->supportsAlphaBorder() && QFile::exists(directory + "background_nc.png"))
            pix.load(directory + "background_nc.png");
        if (pix.hasAlpha())
            pix.setMask(pix.mask());
        if (!g_app->supportsAlphaBorder() && QFile::exists(directory + "mask_nc.png"))
            pix.setMask(QPixmap(directory + "mask_nc.png"));
        else if (QFile::exists(directory + "mask.png"))
            pix.setMask(QPixmap(directory + "mask.png"));

        if (g_app->supportsAlphaBorder()) {
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
            QPixmap scaled = pix.scaled(m_pUi->skinPreview->size(),
                                        Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_pUi->skinPreview->setPixmap(scaled);
        }
    }
    else if (pix.load(directory + "frame.png")) {
        QPixmap scaled = pix.scaled(m_pUi->skinPreview->size(),
                                    Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_pUi->skinPreview->setPixmap(scaled);
    }
    else {
        m_pUi->skinPreview->clear();
    }
}


void OptionDialog::pluginChanged(int row) {
    m_pUi->plugBox->setTitle(tr("Plugin options"));

    if (m_pUi->plugBox->layout() != NULL) {
        for (int i = 1; i < m_pUi->plugBox->layout()->count(); ++i) {
            m_pUi->plugBox->layout()->removeItem(m_pUi->plugBox->layout()->itemAt(i));
        }
    }

    // Close any current plugin dialogs
    if (m_curPlugin >= 0) {
        QListWidgetItem* item = m_pUi->plugList->item(m_curPlugin);
        g_pluginHandler->endDialog(item->data(Qt::UserRole).toUInt(), true);
    }

    // Open the new plugin dialog
    m_curPlugin = row;
    s_currentPlugin = row;
    if (row >= 0) {
        loadPluginDialog(m_pUi->plugList->item(row));
    }
}


void OptionDialog::loadPluginDialog(QListWidgetItem* item) {
    QWidget* win = g_pluginHandler->doDialog(m_pUi->plugBox, item->data(Qt::UserRole).toUInt());
    if (win != NULL) {
        if (m_pUi->plugBox->layout() != NULL)
            m_pUi->plugBox->layout()->addWidget(win);
        win->show();
        if (win->windowTitle() != "Form")
            m_pUi->plugBox->setTitle(win->windowTitle());
    }
}


void OptionDialog::pluginItemChanged(QListWidgetItem* iz) {
    int row = m_pUi->plugList->currentRow();
    if (row == -1)
        return;

    // Close any current plugin dialogs
    if (m_curPlugin >= 0) {
        QListWidgetItem* item = m_pUi->plugList->item(m_curPlugin);
        g_pluginHandler->endDialog(item->data(Qt::UserRole).toUInt(), true);
    }

    // Write out the new config
    g_settings->beginWriteArray("plugins");
    for (int i = 0; i < m_pUi->plugList->count(); i++) {
        QListWidgetItem* item = m_pUi->plugList->item(i);
        g_settings->setArrayIndex(i);
        g_settings->setValue("id", item->data(Qt::UserRole).toUInt());
        if (item->checkState() == Qt::Checked) {
            g_settings->setValue("load", true);
        }
        else {
            g_settings->setValue("load", false);
        }
    }
    g_settings->endArray();

    // Reload the plugins
    g_pluginHandler->loadPlugins();

    // If enabled, reload the dialog
    if (iz->checkState() == Qt::Checked) {
        loadPluginDialog(iz);
    }
}


void OptionDialog::logLevelChanged(int index) {
    Logger::setLogLevel(index);
}

void OptionDialog::onCheckUpdateToggled(bool checked) {
    if (checked) {
        m_pUi->sbCheckUpdateInterval->setEnabled(m_pUi->cbCheckUpdateRepeat->isChecked());
    }
    else {
        m_pUi->sbCheckUpdateInterval->setEnabled(false);
    }
}

void OptionDialog::onCheckUpdateRepeatToggled(bool checked) {
    m_pUi->sbCheckUpdateInterval->setEnabled(checked);
}

void OptionDialog::onProxyTypeChanged(int index) {
    bool enable = index > 1;
    m_pUi->leProxyServerIp->setEnabled(enable);
    m_pUi->leProxyServerPort->setEnabled(enable);
    m_pUi->cbProxyRequiresPassword->setEnabled(enable);
    bool requirePassword = m_pUi->cbProxyRequiresPassword->isChecked();
    m_pUi->leProxyUsername->setEnabled(enable && requirePassword);
    m_pUi->leProxyPassword->setEnabled(enable && requirePassword);
}

void OptionDialog::onProxyRequiresPasswordToggled(bool checked) {
    m_pUi->leProxyUsername->setEnabled(checked);
    m_pUi->leProxyPassword->setEnabled(checked);
}

void OptionDialog::catalogProgressUpdated(int value) {
    m_pUi->catSize->setVisible(false);
    m_pUi->catProgress->setValue(value);
    m_pUi->catProgress->setVisible(true);
    m_pUi->catRescan->setEnabled(false);
}


void OptionDialog::catalogBuilt() {
    m_pUi->catProgress->setVisible(false);
    m_pUi->catRescan->setEnabled(true);

    m_pUi->catSize->setText(tr("Index has %n items", "", g_catalog->count()));
    m_pUi->catSize->setVisible(true);
}


void OptionDialog::catRescanClicked(bool val) {
    val = val; // Compiler warning

    // Apply Directory Options
    SettingsManager::instance().writeCatalogDirectories(m_memDirs);

    m_needRescan = false;
    m_pUi->catRescan->setEnabled(false);
    g_mainWidget->buildCatalog();
}


void OptionDialog::catTypesDirChanged(int state)
{
    state = state; // Compiler warning
    int row = m_pUi->catDirectories->currentRow();
    if (row == -1)
        return;
    m_memDirs[row].indexDirs = m_pUi->catCheckDirs->isChecked();

    m_needRescan = true;
}


void OptionDialog::catTypesExeChanged(int state) {
    Q_UNUSED(state)
    int row = m_pUi->catDirectories->currentRow();
    if (row == -1)
        return;
    m_memDirs[row].indexExe = m_pUi->catCheckBinaries->isChecked();

    m_needRescan = true;
}


void OptionDialog::catDirItemChanged(QListWidgetItem* item) {
    int row = m_pUi->catDirectories->currentRow();
    if (row == -1)
        return;
    if (item != m_pUi->catDirectories->item(row))
        return;

    m_memDirs[row].name = item->text();

    m_needRescan = true;
}


void OptionDialog::catDirDragEnter(QDragEnterEvent *event) {
    const QMimeData* mimeData = event->mimeData();
    if (mimeData && mimeData->hasUrls())
        event->acceptProposedAction();
}


void OptionDialog::catDirDrop(QDropEvent *event) {
    const QMimeData* mimeData = event->mimeData();
    if (mimeData && mimeData->hasUrls()) {
        foreach(QUrl url, mimeData->urls()) {
            QFileInfo info(url.toLocalFile());
            if (info.exists() && info.isDir()) {
                addDirectory(info.filePath());
            }
        }
    }
}


void OptionDialog::dirRowChanged(int row) {
    if (row == -1)
        return;

    m_pUi->catTypes->clear();
    foreach(QString str, m_memDirs[row].types) {
        QListWidgetItem* item = new QListWidgetItem(str, m_pUi->catTypes);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
    m_pUi->catCheckDirs->setChecked(m_memDirs[row].indexDirs);
    m_pUi->catCheckBinaries->setChecked(m_memDirs[row].indexExe);
    m_pUi->catDepth->setValue(m_memDirs[row].depth);

    m_needRescan = true;
}


void OptionDialog::catDirMinusClicked(bool c) {
    Q_UNUSED(c)
    int dirRow = m_pUi->catDirectories->currentRow();

    delete m_pUi->catDirectories->takeItem(dirRow);
    m_pUi->catTypes->clear();

    m_memDirs.removeAt(dirRow);

    if (dirRow >= m_pUi->catDirectories->count()
        && m_pUi->catDirectories->count() > 0) {
        m_pUi->catDirectories->setCurrentRow(m_pUi->catDirectories->count() - 1);
        dirRowChanged(m_pUi->catDirectories->count() - 1);
    }
}

void OptionDialog::catDirPlusClicked(bool c) {
    Q_UNUSED(c)
    addDirectory("", true);
}

void OptionDialog::initCatalogWidget() {
    // Load the directories and types
    m_pUi->catDirectories->setItemDelegate(&m_directoryItemDelegate);

    connect(m_pUi->catDirectories, SIGNAL(currentRowChanged(int)), this, SLOT(dirRowChanged(int)));
    connect(m_pUi->catDirectories, SIGNAL(dragEnter(QDragEnterEvent*)), this, SLOT(catDirDragEnter(QDragEnterEvent*)));
    connect(m_pUi->catDirectories, SIGNAL(drop(QDropEvent*)), this, SLOT(catDirDrop(QDropEvent*)));
    connect(m_pUi->catDirectories, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(catDirItemChanged(QListWidgetItem*)));
    connect(m_pUi->catDirPlus, SIGNAL(clicked(bool)), this, SLOT(catDirPlusClicked(bool)));
    connect(m_pUi->catDirMinus, SIGNAL(clicked(bool)), this, SLOT(catDirMinusClicked(bool)));
    connect(m_pUi->catTypes, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(catTypesItemChanged(QListWidgetItem*)));
    connect(m_pUi->catTypesPlus, SIGNAL(clicked(bool)), this, SLOT(catTypesPlusClicked(bool)));
    connect(m_pUi->catTypesMinus, SIGNAL(clicked(bool)), this, SLOT(catTypesMinusClicked(bool)));
    connect(m_pUi->catCheckDirs, SIGNAL(stateChanged(int)), this, SLOT(catTypesDirChanged(int)));
    connect(m_pUi->catCheckBinaries, SIGNAL(stateChanged(int)), this, SLOT(catTypesExeChanged(int)));
    connect(m_pUi->catDepth, SIGNAL(valueChanged(int)), this, SLOT(catDepthChanged(int)));
    connect(m_pUi->catRescan, SIGNAL(clicked(bool)), this, SLOT(catRescanClicked(bool)));
    m_pUi->catProgress->setVisible(false);

    m_memDirs = SettingsManager::instance().readCatalogDirectories();
    for (int i = 0; i < m_memDirs.count(); ++i) {
        m_pUi->catDirectories->addItem(m_memDirs[i].name);
        QListWidgetItem* it = m_pUi->catDirectories->item(i);
        it->setFlags(it->flags() | Qt::ItemIsEditable);
    }

    if (m_pUi->catDirectories->count() > 0) {
        m_pUi->catDirectories->setCurrentRow(0);
    }

    m_pUi->catSize->setText(tr("Index has %n item(s)", "N/A", g_catalog->count()));

    connect(g_builder.data(), SIGNAL(catalogIncrement(int)), this, SLOT(catalogProgressUpdated(int)));
    connect(g_builder.data(), SIGNAL(catalogFinished()), this, SLOT(catalogBuilt()));
    if (g_builder->isRunning()) {
        catalogProgressUpdated(g_builder->getProgress());
    }
    m_pUi->catDirectories->installEventFilter(this);
}

void OptionDialog::saveCatalogSettings() {
    // Apply Directory Options
    SettingsManager::instance().writeCatalogDirectories(m_memDirs);
}

void OptionDialog::initPluginsWidget() {
    // Load up the plugins		
    connect(m_pUi->plugList, SIGNAL(currentRowChanged(int)), this, SLOT(pluginChanged(int)));
    connect(m_pUi->plugList, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(pluginItemChanged(QListWidgetItem*)));
    g_pluginHandler->loadPlugins();
    foreach(const PluginInfo& info, g_pluginHandler->getPlugins()) {
        QListWidgetItem* item = new QListWidgetItem(info.name, m_pUi->plugList);
        m_pUi->plugList->addItem(item);
        item->setData(Qt::UserRole, info.id);
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        if (info.loaded) {
            item->setCheckState(Qt::Checked);
        }
        else {
            item->setCheckState(Qt::Unchecked);
        }
    }
    m_pUi->plugList->sortItems();
    if (m_pUi->plugList->count() > s_currentPlugin) {
        m_pUi->plugList->setCurrentRow(s_currentPlugin);
    }
}

void OptionDialog::savePluginsSettings() {
    if (m_curPlugin >= 0) {
        QListWidgetItem* item = m_pUi->plugList->item(m_curPlugin);
        g_pluginHandler->endDialog(item->data(Qt::UserRole).toUInt(), true);
    }
}

void OptionDialog::initUpdateWidget() {
    connect(m_pUi->gbCheckUpdate, &QGroupBox::toggled, this, &OptionDialog::onCheckUpdateToggled);
    connect(m_pUi->cbCheckUpdateRepeat, &QCheckBox::toggled, this, &OptionDialog::onCheckUpdateRepeatToggled);

    // Update
    m_pUi->gbCheckUpdate->setChecked(g_settings->value(OPTION_UPDATE_CHECK_ON_STARTUP,
                                                       OPTION_UPDATE_CHECK_ON_STARTUP_DEFAULT).toBool());
    m_pUi->sbCheckUpdateDelay->setValue(g_settings->value(OPTION_UPDATE_CHECK_ON_STARTUP_DELAY,
                                                          OPTION_UPDATE_CHECK_ON_STARTUP_DELAY_DEFAULT).toInt());
    m_pUi->cbCheckUpdateRepeat->setChecked(g_settings->value(OPTION_UPDATE_CHECK_REPEAT,
                                                             OPTION_UPDATE_CHECK_REPEAT_DEFAULT).toBool());
    m_pUi->sbCheckUpdateInterval->setValue(g_settings->value(OPTION_UPDATE_CHECK_REPEAT_INTERVAL,
                                                             OPTION_UPDATE_CHECK_REPEAT_INTERVAL_DEFAULT).toInt());

    bool enable = m_pUi->gbCheckUpdate->isChecked() && m_pUi->cbCheckUpdateRepeat->isChecked();
    m_pUi->sbCheckUpdateInterval->setEnabled(enable);

}

void OptionDialog::saveUpdateSettings() {
    // Update
    g_settings->setValue(OPTION_UPDATE_CHECK_ON_STARTUP, m_pUi->gbCheckUpdate->isChecked());
    g_settings->setValue(OPTION_UPDATE_CHECK_ON_STARTUP_DELAY, m_pUi->sbCheckUpdateDelay->value());
    g_settings->setValue(OPTION_UPDATE_CHECK_REPEAT, m_pUi->cbCheckUpdateRepeat->isChecked());
    g_settings->setValue(OPTION_UPDATE_CHECK_REPEAT_INTERVAL, m_pUi->sbCheckUpdateInterval->value());
}

void OptionDialog::initProxyWidget() {

    m_pUi->cbProxyType->addItem(tr("No Proxy"), QNetworkProxy::NoProxy);
    m_pUi->cbProxyType->addItem(tr("System Proxy"), QNetworkProxy::DefaultProxy);
    m_pUi->cbProxyType->addItem(tr("HTTP"), QNetworkProxy::HttpProxy);
    m_pUi->cbProxyType->addItem(tr("SOCKS5"), QNetworkProxy::Socks5Proxy);

    connect(m_pUi->cbProxyType, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onProxyTypeChanged(int)));
    connect(m_pUi->cbProxyRequiresPassword, SIGNAL(toggled(bool)),
            this, SLOT(onProxyRequiresPasswordToggled(bool)));

    // Proxy
    QNetworkProxy::ProxyType proxyType
        = g_settings->value(OPTION_PROXY_TYPE,
                            OPTION_PROXY_TYPE_DEFAULT).value<QNetworkProxy::ProxyType>();

    // convert proxy type to index in combobox
    int proxyIndex = 0;
    switch (proxyType) {
    case QNetworkProxy::NoProxy:
        proxyIndex = 0;
        break;
    case QNetworkProxy::DefaultProxy:
        proxyIndex = 1;
        break;
    case QNetworkProxy::HttpProxy:
        proxyIndex = 2;
        break;
    case QNetworkProxy::Socks5Proxy:
        proxyIndex = 3;
        break;
    default:
        break;
    }
    m_pUi->cbProxyType->setCurrentIndex(proxyIndex);

    if (proxyType == QNetworkProxy::NoProxy || proxyType == QNetworkProxy::DefaultProxy) {
        return;
    }

    m_pUi->leProxyServerIp->setText(g_settings->value(OPTION_PROXY_SERVER_IP,
                                                      OPTION_PROXY_SERVER_IP_DEFAULT).toString());
    
    m_pUi->leProxyServerPort->setText(g_settings->value(OPTION_PROXY_SERVER_PORT,
                                                        OPTION_PROXY_SERVER_PORT_DEFAULT).toString());

    m_pUi->cbProxyRequiresPassword->setChecked(g_settings->value(OPTION_PROXY_REQUIRE_PASSWORD,
                                                                 OPTION_PROXY_REQUIRE_PASSWORD_DEFAULT).toBool());

    if (m_pUi->cbProxyRequiresPassword->isChecked()) {
        m_pUi->leProxyUsername->setText(g_settings->value(OPTION_PROXY_USERNAME,
                                                          OPTION_PROXY_USERNAME_DEFAULT).toString());
        m_pUi->leProxyPassword->setText(g_settings->value(OPTION_PROXY_PASSWORD,
                                                          OPTION_PROXY_PASSWORD_DEFAULT).toString());
    }
}


void OptionDialog::saveProxySettings() {
    // Proxy
    QNetworkProxy::ProxyType proxyType = m_pUi->cbProxyType->currentData().value<QNetworkProxy::ProxyType>();
    g_settings->setValue(OPTION_PROXY_TYPE, proxyType);
    g_settings->setValue(OPTION_PROXY_SERVER_IP, m_pUi->leProxyServerIp->text());
    g_settings->setValue(OPTION_PROXY_SERVER_PORT, m_pUi->leProxyServerPort->text());
    g_settings->setValue(OPTION_PROXY_REQUIRE_PASSWORD, m_pUi->cbProxyRequiresPassword->isChecked());
    g_settings->setValue(OPTION_PROXY_USERNAME, m_pUi->leProxyUsername->text());
    g_settings->setValue(OPTION_PROXY_PASSWORD, m_pUi->leProxyPassword->text());
}

void OptionDialog::addDirectory(const QString& directory, bool edit) {
    QString nativeDir = QDir::toNativeSeparators(directory);
    Directory dir(nativeDir);
    m_memDirs.append(dir);

    m_pUi->catTypes->clear();
    QListWidgetItem* item = new QListWidgetItem(nativeDir, m_pUi->catDirectories);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    m_pUi->catDirectories->setCurrentItem(item);
    if (edit)
    {
        m_pUi->catDirectories->editItem(item);
    }

    m_needRescan = true;
}


void OptionDialog::catTypesItemChanged(QListWidgetItem* item) {
    Q_UNUSED(item);

    int row = m_pUi->catDirectories->currentRow();
    if (row == -1)
        return;
    int typesRow = m_pUi->catTypes->currentRow();
    if (typesRow == -1)
        return;

    m_memDirs[row].types[typesRow] = m_pUi->catTypes->item(typesRow)->text();

    m_needRescan = true;
}


void OptionDialog::catTypesPlusClicked(bool c) {
    Q_UNUSED(c)
    int row = m_pUi->catDirectories->currentRow();
    if (row == -1)
        return;

    m_memDirs[row].types << "";
    QListWidgetItem* item = new QListWidgetItem(m_pUi->catTypes);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    m_pUi->catTypes->setCurrentItem(item);
    m_pUi->catTypes->editItem(item);

    m_needRescan = true;
}

void OptionDialog::catTypesMinusClicked(bool c) {
    Q_UNUSED(c)
    int dirRow = m_pUi->catDirectories->currentRow();
    if (dirRow == -1)
        return;

    int typesRow = m_pUi->catTypes->currentRow();
    if (typesRow == -1)
        return;

    m_memDirs[dirRow].types.removeAt(typesRow);
    delete m_pUi->catTypes->takeItem(typesRow);

    if (typesRow >= m_pUi->catTypes->count()
        && m_pUi->catTypes->count() > 0)
        m_pUi->catTypes->setCurrentRow(m_pUi->catTypes->count() - 1);

    m_needRescan = true;
}

void OptionDialog::catDepthChanged(int d) {
    int row = m_pUi->catDirectories->currentRow();
    if (row != -1)
        m_memDirs[row].depth = d;

    m_needRescan = true;
}

}