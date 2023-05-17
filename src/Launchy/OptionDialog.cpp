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
#include <QNetworkProxy>
#include <QStyleFactory>
#include <QBitmap>
#include <QPainter>
#include <QMimeData>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>

#include "LaunchyLib/LaunchyLib.h"

#include "ui_OptionDialog.h"

#include "AppBase.h"
#include "LaunchyWidget.h"
#include "GlobalVar.h"
#include "PluginHandler.h"
#include "FileBrowserDelegate.h"
#include "SettingsManager.h"
#include "Logger.h"
#include "Catalog.h"
#include "CatalogBuilder.h"
#include "OptionItem.h"
#include "UpdateChecker.h"
#include "TranslationManager.h"
#include "LaunchyVersion.h"

// for QNetworkProxy::ProxyType in QVariant
Q_DECLARE_METATYPE(QNetworkProxy::ProxyType)

namespace launchy {

// for qt flags
// https://stackoverflow.com/questions/10755058/qflags-enum-type-conversion-fails-all-of-a-sudden
using ::operator|;

QByteArray OptionDialog::s_lastWindowGeometry;
int OptionDialog::s_lastTab = 0;
int OptionDialog::s_lastPlugin = -1;

OptionDialog::OptionDialog(QWidget* parent)
    : QDialog(parent),
      m_pUi(new Ui::OptionDialog),
      m_directoryItemDelegate(new FileBrowserDelegate(this, FileBrowser::Directory)) {

    setObjectName("options");
    g_needRebuildCatalog.storeRelease(0);

    m_pUi->setupUi(this);

    Qt::WindowFlags windowsFlags = windowFlags();
    windowsFlags = windowsFlags & (~Qt::WindowContextHelpButtonHint);
    windowsFlags = windowsFlags | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowStaysOnTopHint;
    setWindowFlags(windowsFlags);

    restoreGeometry(s_lastWindowGeometry);
    m_pUi->tabWidget->setCurrentIndex(s_lastTab);
    connect(m_pUi->tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    connect(m_pUi->pbOk, SIGNAL(clicked()), this, SLOT(accept()));
    connect(m_pUi->pbCancel, SIGNAL(clicked()), this, SLOT(reject()));

    initGeneralWidget();

    initSkinWidget();

    initCatalogWidget();

    initPluginsWidget();

    initUpdateWidget();

    initProxyWidget();

    initSystemWidget();

    initAboutWidget();
}

OptionDialog::~OptionDialog() {
    if (g_builder) {
        disconnect(g_builder, SIGNAL(catalogIncrement(int)), this, SLOT(catalogProgressUpdated(int)));
        disconnect(g_builder, SIGNAL(catalogFinished()), this, SLOT(catalogBuilt()));
    }

    s_lastTab = m_pUi->tabWidget->currentIndex();
    s_lastWindowGeometry = saveGeometry();

    delete m_pUi;
    m_pUi = nullptr;
}

void OptionDialog::retranslateUi() {
    m_pUi->retranslateUi(this);
}

void OptionDialog::accept() {
    if (g_settings.isNull()) {
        qWarning() << "OptionDialog::accept, fail to save setting.";
        return;
    }

    bool bSuccess = saveGeneralSettings();

    saveSkinSettings();

    saveCatalogSettings();

    savePluginsSettings();

    saveUpdateSettings();

    saveProxySettings();

    saveSystemSettings();

    g_settings->sync();

    if (!bSuccess) {
        return;
    }

    QDialog::accept();

    if (g_needRebuildCatalog.fetchAndStoreRelaxed(0) > 0) {
        g_mainWidget->buildCatalog();
    }

    g_mainWidget->showLaunchy();
}


void OptionDialog::reject() {
    if (s_lastPlugin >= 0) {
        QListWidgetItem* item = m_pUi->plugList->item(s_lastPlugin);
        PluginHandler::instance().endDialog(item->data(Qt::UserRole).toString(), false);
    }

    QDialog::reject();
    g_mainWidget->showLaunchy();
}

void OptionDialog::showEvent(QShowEvent* event) {
    // skin
    if (QListWidgetItem* pItem = m_pUi->skinList->currentItem()) {
        skinChanged(pItem->text());
    }

    connect(m_pUi->skinList, SIGNAL(currentTextChanged(const QString)),
            this, SLOT(skinChanged(const QString)));

    // plugin
    if (s_lastPlugin < 0 && m_pUi->plugList->count() > 0) {
        m_pUi->plugList->setCurrentRow(0);
    }

    if (s_lastPlugin >= 0
        && s_lastPlugin < m_pUi->plugList->count()) {
        QListWidgetItem* item = m_pUi->plugList->item(s_lastPlugin);
        loadPluginDialog(item);
        m_pUi->plugList->setCurrentRow(s_lastPlugin);
    }

    pluginChanged(m_pUi->plugList->currentRow());

    connect(m_pUi->plugList, SIGNAL(currentRowChanged(int)),
            this, SLOT(pluginChanged(int)));

    QDialog::showEvent(event);
}

void OptionDialog::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        // retranslate designer form (single inheritance approach)
        retranslateUi();
    }

    // call base class implementation
    QDialog::changeEvent(event);
}

void OptionDialog::tabChanged(int tab) {
    // Redraw the current skin (necessary because of dialog resizing issues)
    if (m_pUi->tabWidget->widget(tab)->objectName() == "Skins") {
        if (QListWidgetItem* pItem = m_pUi->skinList->currentItem()) {
            skinChanged(pItem->text());
        }
    }
//     else if (m_pUi->tabWidget->currentWidget()->objectName() == "Plugins") {
//         // We've currently no way of checking if a plugin requires a catalog rescan
//         // so assume that we need one if the user has viewed the plugins tab
//         ++g_needRebuildCatalog;
//     }
}


void OptionDialog::onAppStyleChanged(int index) {
    QString appStyle = m_pUi->cbAppStyle->itemData(index).toString();
    qApp->setStyle(QStyleFactory::create(appStyle));
}

void OptionDialog::autoRebuildCheckChanged(int state) {
    m_pUi->genRebuildMinutes->setEnabled(state > 0);
    if (m_pUi->genRebuildMinutes->value() <= 0) {
        m_pUi->genRebuildMinutes->setValue(OPTION_REBUILDTIMER_DEFAULT);
    }
}

void OptionDialog::skinChanged(const QString& newSkin) {
    if (newSkin.isEmpty()) {
        return;
    }

    // Find the skin with this name
    QString directory = SettingsManager::instance().skinPath(newSkin);

    // Load up the author file
    if (directory.isEmpty()) {
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
            pix.setMask(QBitmap(directory + "mask_nc.png"));
        else if (QFile::exists(directory + "mask.png"))
            pix.setMask(QBitmap(directory + "mask.png"));

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
    // Close any current plugin dialogs
    if (s_lastPlugin >= 0) {
        QListWidgetItem* item = m_pUi->plugList->item(s_lastPlugin);
        PluginHandler::instance().endDialog(item->data(Qt::UserRole).toString(), true);
    }

    // Open the new plugin dialog
    s_lastPlugin = row;
    if (row >= 0) {
        loadPluginDialog(m_pUi->plugList->item(row));
    }
}

void OptionDialog::loadPluginDialog(QListWidgetItem* item) {

    m_pUi->plugBox->setTitle(tr("Plugin options"));
    QLayout* pLayout = m_pUi->plugBox->layout();
    if (pLayout != nullptr) {
        while (QLayoutItem* child = pLayout->takeAt(0)) {
            delete child;
        }
    }
    QString pluginName = item->data(Qt::UserRole).toString();
    QWidget* win = PluginHandler::instance().doDialog(m_pUi->plugBox, pluginName);
    if (win != nullptr) {
        if (pLayout != nullptr) {
            pLayout->addWidget(win);
        }

        win->show();
        if (win->windowTitle() != "Form") {
            m_pUi->plugBox->setTitle(win->windowTitle());
        }
    }
}

void OptionDialog::pluginItemChanged(QListWidgetItem* item) {
    int row = m_pUi->plugList->row(item);
    if (row == -1) {
        return;
    }

    // Close current plugin dialogs
    if (s_lastPlugin == row && item->checkState() != Qt::Checked) {
        QListWidgetItem* item = m_pUi->plugList->item(s_lastPlugin);
        PluginHandler::instance().endDialog(item->data(Qt::UserRole).toString(), true);
    }

    // Write out the new config
    g_settings->beginWriteArray("Plugin");
    for (int i = 0; i < m_pUi->plugList->count(); i++) {
        QListWidgetItem* item = m_pUi->plugList->item(i);
        g_settings->setArrayIndex(i);
        g_settings->setValue("name", item->data(Qt::UserRole).toString());
        if (item->checkState() == Qt::Checked) {
            g_settings->setValue("load", true);
        }
        else {
            g_settings->setValue("load", false);
        }
    }
    g_settings->endArray();

    // Reload the plugins
    PluginHandler::instance().loadPlugins();

    if (row != m_pUi->plugList->currentRow()) {
        m_pUi->plugList->setCurrentRow(row);
    }
    else {
        // If enabled, reload the dialog
        if (item->checkState() == Qt::Checked) {
            loadPluginDialog(item);
        }
    }
}

void OptionDialog::logLevelChanged(int index) {
    Logger::setLogLevel(index);
}

void OptionDialog::languageChanged(int index) {
    QString loc = m_pUi->cbLanguage->itemData(index).toString();
    g_settings->setValue(OPTION_LANGUAGE, loc);

    qDebug() << "OptionDialog::languageChanged, loc =" << loc;
    TranslationManager::instance().setLocale(QLocale(loc));
}

void OptionDialog::onProxyTypeChanged(int index) {
    bool enable = index > 1;
    m_pUi->leProxyServerName->setEnabled(enable);
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

    m_pUi->catSize->setText(tr("Index has %n item(s)", "", g_catalog->count()));
    m_pUi->catSize->setVisible(true);
}

void OptionDialog::catRescanClicked(bool val) {
    Q_UNUSED(val)
    // Apply Directory Options
    SettingsManager::instance().writeCatalogDirectories(m_memDirs);

    g_needRebuildCatalog.storeRelease(0);
    m_pUi->catRescan->setEnabled(false);
    g_mainWidget->buildCatalog();
}


void OptionDialog::catTypesDirChanged(int state) {
    Q_UNUSED(state)
    int row = m_pUi->catDirectories->currentRow();
    if (row == -1)
        return;
    m_memDirs[row].indexDirs = m_pUi->catCheckDirs->isChecked();

    ++g_needRebuildCatalog;
}

void OptionDialog::catTypesExeChanged(int state) {
    Q_UNUSED(state)
    int row = m_pUi->catDirectories->currentRow();
    if (row == -1)
        return;
    m_memDirs[row].indexExe = m_pUi->catCheckBinaries->isChecked();

    ++g_needRebuildCatalog;
}

void OptionDialog::catDirItemChanged(QListWidgetItem* item) {
    int row = m_pUi->catDirectories->currentRow();
    if (row == -1)
        return;
    if (item != m_pUi->catDirectories->item(row))
        return;

    m_memDirs[row].name = item->text();

    ++g_needRebuildCatalog;
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
    if (row == -1) {
        return;
    }

    m_pUi->catTypes->blockSignals(true);
    m_pUi->catTypes->clear();
    foreach(QString str, m_memDirs[row].types) {
        QListWidgetItem* item = new QListWidgetItem(str, m_pUi->catTypes);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
    }
    m_pUi->catTypes->blockSignals(false);

    m_pUi->catCheckDirs->blockSignals(true);
    m_pUi->catCheckDirs->setChecked(m_memDirs[row].indexDirs);
    m_pUi->catCheckDirs->blockSignals(false);

    m_pUi->catCheckBinaries->blockSignals(true);
    m_pUi->catCheckBinaries->setChecked(m_memDirs[row].indexExe);
    m_pUi->catCheckBinaries->blockSignals(false);

    m_pUi->catDepth->blockSignals(true);
    m_pUi->catDepth->setValue(m_memDirs[row].depth);
    m_pUi->catDepth->blockSignals(false);
}

void OptionDialog::catDirMinusClicked(bool c) {
    Q_UNUSED(c)
    int dirRow = m_pUi->catDirectories->currentRow();
    if (dirRow == -1) {
        return;
    }

    delete m_pUi->catDirectories->takeItem(dirRow);
    m_pUi->catTypes->clear();

    m_memDirs.removeAt(dirRow);

    if (dirRow >= m_pUi->catDirectories->count()
        && m_pUi->catDirectories->count() > 0) {
        m_pUi->catDirectories->setCurrentRow(m_pUi->catDirectories->count() - 1);
    }
}

void OptionDialog::catDirPlusClicked(bool c) {
    Q_UNUSED(c)
    addDirectory("", true);
}

void OptionDialog::initGeneralWidget() {
    // hot key
#ifdef Q_OS_MAC
    m_metaKeys << QString("") << QString("Alt") << QString("Command") << QString("Shift") << QString("Control")
        << QString("Command+Alt") << QString("Command+Shift")
        << QString("Command+Control") << QString("Command+Alt+Shift");
#else
    m_metaKeys << QString("") << QString("Alt") << QString("Control") << QString("Shift") << QString("Win")
        << QString("Ctrl+Alt") << QString("Ctrl+Shift")
        << QString("Ctrl+Win") << QString("Ctrl+Alt+Shift");
#endif
    m_iMetaKeys << Qt::NoModifier << Qt::AltModifier << Qt::ControlModifier << Qt::ShiftModifier << Qt::MetaModifier
        << (Qt::ControlModifier | Qt::AltModifier) << (Qt::ControlModifier | Qt::ShiftModifier)
        << (Qt::ControlModifier | Qt::MetaModifier) << (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier);

    m_actionKeys << QString("Space") << QString("Tab") << QString("Caps Lock") << QString("Backspace")
        << QString("Enter") << QString("Esc") << QString("Insert") << QString("Delete") << QString("Home")
        << QString("End") << QString("Page Up") << QString("Page Down") << QString("Print") << QString("Scroll Lock")
        << QString("Pause") << QString("Num Lock")
        << tr("Up") << tr("Down") << tr("Left") << tr("Right")
        << QString("F1") << QString("F2") << QString("F3") << QString("F4") << QString("F5")
        << QString("F6") << QString("F7") << QString("F8") << QString("F9") << QString("F10")
        << QString("F11") << QString("F12") << QString("F13") << QString("F14") << QString("F15");

    m_iActionKeys << Qt::Key_Space << Qt::Key_Tab << Qt::Key_CapsLock << Qt::Key_Backspace 
        << Qt::Key_Enter << Qt::Key_Escape << Qt::Key_Insert << Qt::Key_Delete << Qt::Key_Home
        << Qt::Key_End << Qt::Key_PageUp << Qt::Key_PageDown << Qt::Key_Print << Qt::Key_ScrollLock
        << Qt::Key_Pause << Qt::Key_NumLock
        << Qt::Key_Up << Qt::Key_Down << Qt::Key_Left << Qt::Key_Right
        << Qt::Key_F1 << Qt::Key_F2 << Qt::Key_F3 << Qt::Key_F4 << Qt::Key_F5
        << Qt::Key_F6 << Qt::Key_F7 << Qt::Key_F8 << Qt::Key_F9 << Qt::Key_F10
        << Qt::Key_F11 << Qt::Key_F12 << Qt::Key_F13 << Qt::Key_F14 << Qt::Key_F15;

    for (int i = '0'; i <= '9'; ++i) {
        m_actionKeys << QString(QChar(i));
        m_iActionKeys << Qt::Key(i);
    }

    for (int i = 'A'; i <= 'Z'; ++i) {
        m_actionKeys << QString(QChar(i));
        m_iActionKeys << Qt::Key(i);
    }

    m_actionKeys << "`" << "-" << "=" << "["
        << "]" << ";" << "'" << "#"
        << "\\" << "," << "." << "/";

    m_iActionKeys << Qt::Key('`') << Qt::Key('-') << Qt::Key('=') << Qt::Key('[')
        << Qt::Key(']') << Qt::Key(';') << Qt::Key('\'') << Qt::Key('#')
        << Qt::Key('\\') << Qt::Key(',') << Qt::Key('.') << Qt::Key('/');

    // find the current hotkey
    int hotkey = g_mainWidget->getHotkey();
    int meta = hotkey & (Qt::AltModifier | Qt::MetaModifier | Qt::ShiftModifier | Qt::ControlModifier);
    hotkey &= ~(Qt::AltModifier | Qt::MetaModifier | Qt::ShiftModifier | Qt::ControlModifier);

    for (int i = 0; i < m_metaKeys.count(); ++i) {
        m_pUi->genModifierBox->addItem(m_metaKeys[i]);
        if (m_iMetaKeys[i] == meta)
            m_pUi->genModifierBox->setCurrentIndex(i);
    }

    for (int i = 0; i < m_actionKeys.count(); ++i) {
        m_pUi->genKeyBox->addItem(m_actionKeys[i]);
        if (m_iActionKeys[i] == hotkey)
            m_pUi->genKeyBox->setCurrentIndex(i);
    }

    // general options
    m_pUi->genAlwaysShow->setChecked(g_settings->value(OPTION_ALWAYSSHOW, OPTION_ALWAYSSHOW_DEFAULT).toBool());

    m_pUi->genHideFocus->setChecked(g_settings->value(OPTION_HIDEIFLOSTFOCUS, OPTION_HIDEIFLOSTFOCUS_DEFAULT).toBool());

    m_pUi->genAlwaysTop->setChecked(g_settings->value(OPTION_ALWAYSTOP, OPTION_ALWAYSTOP_DEFAULT).toBool());

    int center = g_settings->value(OPTION_ALWAYSCENTER, OPTION_ALWAYSCENTER_DEFAULT).toInt();
    m_pUi->genHCenter->setChecked((center & 1) != 0);
    m_pUi->genVCenter->setChecked((center & 2) != 0);

    m_pUi->genShiftDrag->setChecked(g_settings->value(OPTION_DRAGMODE, OPTION_DRAGMODE_DEFAULT).toBool());

    assert(qApp);
    QList<QScreen*> listScreen = qApp->screens();
    for (int i = 0; i < listScreen.size(); ++i) {
        QScreen* pScreen = listScreen[i];
        assert(pScreen);

        QString strScreen = tr("Screen %1: ").arg(i + 1);
        strScreen += pScreen->name();
        strScreen += " ";
        strScreen += pScreen->manufacturer();
        strScreen += pScreen->model();
        m_pUi->comboBoxScreenNumber->addItem(strScreen);

        qDebug() << "OptionDialog::initGeneralWidget, screen:"
            << i << pScreen->name()
            << ", geometry:" << pScreen->geometry()
            << ", virtual geometry:" << pScreen->virtualGeometry()
            << ", size:" << pScreen->size()
            << ", virtual size:" << pScreen->virtualSize();
    }
    int nScreenIndex = g_settings->value(OPTION_SCREEN_INDEX, OPTION_SCREEN_INDEX_DEFAULT).toInt();
    if (nScreenIndex < listScreen.size()) {
        m_pUi->comboBoxScreenNumber->setCurrentIndex(nScreenIndex);
    }

    m_pUi->pushButtonRescanScreen->setVisible(false);

    m_pUi->genHideTray->setChecked(g_settings->value(OPTION_HIDE_TRAY_ICON, OPTION_HIDE_TRAY_ICON_DEFAULT).toBool());

    // application style
    QString appStyle = g_settings->value(OPTION_APPSTYLE, OPTION_APPSTYLE_DEFAULT).toString();

    QStringList styles = QStyleFactory::keys();
    foreach (QString style, styles) {
        m_pUi->cbAppStyle->addItem(style, style.toLower());
    }

    int appStyleIndex = 0;
    int appStyleCount = m_pUi->cbAppStyle->count();
    for (int i = 0; i < appStyleCount; ++i) {
        if (m_pUi->cbAppStyle->itemData(i).toString() == appStyle) {
            appStyleIndex = i;
            break;
        }
    }

    m_pUi->cbAppStyle->setCurrentIndex(appStyleIndex);

    connect(m_pUi->cbAppStyle, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onAppStyleChanged(int)));

    // suggestion list
    m_pUi->genDecorateText->setChecked(g_settings->value(OPTION_DECORATETEXT, OPTION_DECORATETEXT_DEFAULT).toBool());

    m_pUi->genCondensed->setCurrentIndex(g_settings->value(OPTION_CONDENSEDVIEW, OPTION_CONDENSEDVIEW_DEFAULT).toInt());
    m_pUi->genAutoSuggestDelay->setValue(g_settings->value(OPTION_AUTOSUGGESTDELAY, OPTION_AUTOSUGGESTDELAY_DEFAULT).toInt());

    m_pUi->genMaxViewable->setValue(g_settings->value(OPTION_NUMVIEWABLE, OPTION_NUMVIEWABLE_DEFAULT).toInt());
    m_pUi->genNumResults->setValue(g_settings->value(OPTION_NUMRESULT, OPTION_NUMRESULT_DEFAULT).toInt());
    m_pUi->genNumHistory->setValue(g_settings->value(OPTION_MAXITEMSINHISTORY, OPTION_MAXITEMSINHISTORY_DEFAULT).toInt());

    m_pUi->genOpaqueness->setValue(g_settings->value(OPTION_OPAQUENESS, OPTION_OPAQUENESS_DEFAULT).toInt());
    m_pUi->genFadeIn->setValue(g_settings->value(OPTION_FADEIN, OPTION_FADEIN_DEFAULT).toInt());
    m_pUi->genFadeOut->setValue(g_settings->value(OPTION_FADEOUT, OPTION_FADEOUT_DEFAULT).toInt());

    connect(m_pUi->genOpaqueness, SIGNAL(sliderMoved(int)), g_mainWidget, SLOT(setOpaqueness(int)));
}

bool OptionDialog::saveGeneralSettings() {
    // See if the new hotkey works, if not we're not leaving the dialog.
    QKeySequence hotkey(m_iMetaKeys[m_pUi->genModifierBox->currentIndex()] | m_iActionKeys[m_pUi->genKeyBox->currentIndex()]);
    if (!g_mainWidget->setHotkey(hotkey)) {
        QMessageBox::warning(this, tr("Launchy"),
                             tr("The hotkey %1 is already in use, please select another.").arg(hotkey.toString()));
        return false;
    }

    g_settings->setValue(OPTION_HOTKEY, hotkey.isEmpty() ? OPTION_HOTKEY_DEFAULT : hotkey[0].toCombined());

    // Save General Options
    // g_settings->setValue("GenOps/showtrayicon", genShowTrayIcon->isChecked());
    g_settings->setValue(OPTION_ALWAYSSHOW, m_pUi->genAlwaysShow->isChecked());
    g_settings->setValue(OPTION_ALWAYSTOP, m_pUi->genAlwaysTop->isChecked());

    g_settings->setValue(OPTION_DECORATETEXT, m_pUi->genDecorateText->isChecked());
    g_settings->setValue(OPTION_HIDEIFLOSTFOCUS, m_pUi->genHideFocus->isChecked());
    g_settings->setValue(OPTION_ALWAYSCENTER, (m_pUi->genHCenter->isChecked() ? 1 : 0) | (m_pUi->genVCenter->isChecked() ? 2 : 0));
    g_settings->setValue(OPTION_DRAGMODE, m_pUi->genShiftDrag->isChecked());
    g_settings->setValue(OPTION_SCREEN_INDEX, m_pUi->comboBoxScreenNumber->currentIndex());
    g_settings->setValue(OPTION_HIDE_TRAY_ICON, m_pUi->genHideTray->isChecked());

    g_settings->setValue(OPTION_CONDENSEDVIEW, m_pUi->genCondensed->currentIndex());
    g_settings->setValue(OPTION_AUTOSUGGESTDELAY, m_pUi->genAutoSuggestDelay->value());

    g_settings->setValue(OPTION_NUMVIEWABLE, m_pUi->genMaxViewable->value());
    g_settings->setValue(OPTION_NUMRESULT, m_pUi->genNumResults->value());
    g_settings->setValue(OPTION_MAXITEMSINHISTORY, m_pUi->genNumHistory->value());
    g_settings->setValue(OPTION_OPAQUENESS, m_pUi->genOpaqueness->value());
    g_settings->setValue(OPTION_FADEIN, m_pUi->genFadeIn->value());
    g_settings->setValue(OPTION_FADEOUT, m_pUi->genFadeOut->value());

    // Apply General Options
    g_mainWidget->startRebuildTimer();
    g_mainWidget->setAlternativeListMode(m_pUi->genCondensed->currentIndex());

    QString appStyle = m_pUi->cbAppStyle->currentData().toString();
    g_settings->setValue(OPTION_APPSTYLE, appStyle);

    // Now save the options that require launchy to be shown or redrawed
    g_mainWidget->setAlwaysShow(m_pUi->genAlwaysShow->isChecked());
    g_mainWidget->setAlwaysTop(m_pUi->genAlwaysTop->isChecked());

    g_mainWidget->setOpaqueness(m_pUi->genOpaqueness->value());

    if (m_pUi->genHideTray->isChecked()) {
        g_mainWidget->hideTrayIcon();
    }
    else {
        g_mainWidget->showTrayIcon();
    }

    return true;
}

void OptionDialog::initSkinWidget() {
    // Load up the skins list
    QString skinName = g_settings->value(OPTION_SKIN, OPTION_SKIN_DEFAULT).toString();

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

            QListWidgetItem* item = new QListWidgetItem(d, m_pUi->skinList);
            m_pUi->skinList->addItem(item);

            if (skinName.compare(d, Qt::CaseInsensitive) == 0)
                skinRow = m_pUi->skinList->count() - 1;
        }
    }
    m_pUi->skinList->setCurrentRow(skinRow);
}

void OptionDialog::saveSkinSettings() {
    // Apply Skin Options
    QListWidgetItem* pCurrentItem = m_pUi->skinList->currentItem();
    if (!pCurrentItem) {
        return;
    }

    QString currentSkinName = pCurrentItem->text();
    QString prevSkinName = g_settings->value(OPTION_SKIN, OPTION_SKIN_DEFAULT).toString();
    if (currentSkinName != prevSkinName) {
        g_settings->setValue(OPTION_SKIN, currentSkinName);
        g_mainWidget->setSkin(currentSkinName);
    }
}

void OptionDialog::initCatalogWidget() {
    // Load the directories and types
    m_pUi->catDirectories->setItemDelegate(m_directoryItemDelegate);

    m_memDirs = SettingsManager::instance().readCatalogDirectories();
    for (int i = 0; i < m_memDirs.count(); ++i) {
        QListWidgetItem* item = new QListWidgetItem(m_memDirs[i].name, m_pUi->catDirectories);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        m_pUi->catDirectories->addItem(item);
    }

    m_pUi->catDirectories->setCurrentRow(-1);

    connect(m_pUi->catDirectories, SIGNAL(currentRowChanged(int)), this, SLOT(dirRowChanged(int)));
    connect(m_pUi->catDirectories, SIGNAL(dragEnter(QDragEnterEvent*)), this, SLOT(catDirDragEnter(QDragEnterEvent*)));
    connect(m_pUi->catDirectories, SIGNAL(drop(QDropEvent*)), this, SLOT(catDirDrop(QDropEvent*)));
    connect(m_pUi->catDirectories, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(catDirItemChanged(QListWidgetItem*)));
    //connect(m_pUi->catDirectories, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(onCatDirItemActivated(QListWidgetItem*)));

    connect(m_pUi->catDirPlus, SIGNAL(clicked(bool)), this, SLOT(catDirPlusClicked(bool)));
    connect(m_pUi->catDirMinus, SIGNAL(clicked(bool)), this, SLOT(catDirMinusClicked(bool)));
    connect(m_pUi->catTypes, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(catTypesItemChanged(QListWidgetItem*)));
    connect(m_pUi->catTypesPlus, SIGNAL(clicked(bool)), this, SLOT(catTypesPlusClicked(bool)));
    connect(m_pUi->catTypesMinus, SIGNAL(clicked(bool)), this, SLOT(catTypesMinusClicked(bool)));
    connect(m_pUi->catCheckDirs, SIGNAL(stateChanged(int)), this, SLOT(catTypesDirChanged(int)));
    connect(m_pUi->catCheckBinaries, SIGNAL(stateChanged(int)), this, SLOT(catTypesExeChanged(int)));
    connect(m_pUi->catDepth, SIGNAL(valueChanged(int)), this, SLOT(catDepthChanged(int)));
    connect(m_pUi->catRescan, SIGNAL(clicked(bool)), this, SLOT(catRescanClicked(bool)));

    m_pUi->catSize->setText(tr("Index has %n item(s)", "N/A", g_catalog->count()));

    m_pUi->catProgress->setVisible(false);
    connect(g_builder, SIGNAL(catalogIncrement(int)), this, SLOT(catalogProgressUpdated(int)));
    connect(g_builder, SIGNAL(catalogFinished()), this, SLOT(catalogBuilt()));
    if (g_builder->isRunning()) {
        catalogProgressUpdated(g_builder->getProgress());
    }
    //m_pUi->catDirectories->installEventFilter(this);
}

void OptionDialog::saveCatalogSettings() {
    // Apply Directory Options
    SettingsManager::instance().writeCatalogDirectories(m_memDirs);
}

void OptionDialog::initPluginsWidget() {
    // Load up the plugins
    // PluginHandler::instance().loadPlugins();
    foreach(const PluginInfo& info, PluginHandler::instance().getPlugins()) {
        QListWidgetItem* item = new QListWidgetItem(info.name, m_pUi->plugList);
        m_pUi->plugList->addItem(item);
        item->setData(Qt::UserRole, info.name);
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        if (info.loaded) {
            item->setCheckState(Qt::Checked);
        }
        else {
            item->setCheckState(Qt::Unchecked);
        }
    }
    m_pUi->plugList->sortItems();

    // plugin item check state change
    connect(m_pUi->plugList, SIGNAL(itemChanged(QListWidgetItem*)),
            this, SLOT(pluginItemChanged(QListWidgetItem*)));
}

void OptionDialog::savePluginsSettings() {
    if (s_lastPlugin >= 0) {
        QListWidgetItem* item = m_pUi->plugList->item(s_lastPlugin);
        PluginHandler::instance().endDialog(item->data(Qt::UserRole).toString(), true);
    }
}

void OptionDialog::initUpdateWidget() {
    // Update
    m_pUi->gbCheckUpdate->setChecked(g_settings->value(OPTION_UPDATE_CHECK_ON_STARTUP,
                                                       OPTION_UPDATE_CHECK_ON_STARTUP_DEFAULT).toBool());
    m_pUi->sbCheckUpdateDelay->setValue(g_settings->value(OPTION_UPDATE_CHECK_ON_STARTUP_DELAY,
                                                          OPTION_UPDATE_CHECK_ON_STARTUP_DELAY_DEFAULT).toInt());
    m_pUi->sbCheckUpdateInterval->setValue(g_settings->value(OPTION_UPDATE_CHECK_INTERVAL,
                                                             OPTION_UPDATE_CHECK_INTERVAL_DEFAULT).toInt());

    connect(m_pUi->pbCheckForUpdates, &QPushButton::clicked, []() {
        UpdateChecker::instance().manualCheck();
    });
}

void OptionDialog::saveUpdateSettings() {
    // Update
    g_settings->setValue(OPTION_UPDATE_CHECK_ON_STARTUP, m_pUi->gbCheckUpdate->isChecked());
    g_settings->setValue(OPTION_UPDATE_CHECK_ON_STARTUP_DELAY, m_pUi->sbCheckUpdateDelay->value());
    g_settings->setValue(OPTION_UPDATE_CHECK_INTERVAL, m_pUi->sbCheckUpdateInterval->value());

    UpdateChecker::instance().reloadConfig();
}

void OptionDialog::initProxyWidget() {

    m_pUi->cbProxyType->addItem(tr("No Proxy"), QNetworkProxy::NoProxy);
    m_pUi->cbProxyType->addItem(tr("System Proxy"), QNetworkProxy::DefaultProxy);
    m_pUi->cbProxyType->addItem(tr("HTTP"), QNetworkProxy::HttpProxy);
    m_pUi->cbProxyType->addItem(tr("SOCKS5"), QNetworkProxy::Socks5Proxy);
    QValidator* validator = new QIntValidator(0, 65535, m_pUi->leProxyServerPort);
    m_pUi->leProxyServerPort->setValidator(validator);

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

    m_pUi->leProxyServerName->setText(g_settings->value(OPTION_PROXY_SERVER_NAME,
                                                        OPTION_PROXY_SERVER_NAME_DEFAULT).toString());

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
    g_settings->setValue(OPTION_PROXY_SERVER_NAME, m_pUi->leProxyServerName->text());
    g_settings->setValue(OPTION_PROXY_SERVER_PORT, m_pUi->leProxyServerPort->text());
    g_settings->setValue(OPTION_PROXY_REQUIRE_PASSWORD, m_pUi->cbProxyRequiresPassword->isChecked());
    g_settings->setValue(OPTION_PROXY_USERNAME, m_pUi->leProxyUsername->text());
    g_settings->setValue(OPTION_PROXY_PASSWORD, m_pUi->leProxyPassword->text());


    QNetworkProxy proxy;
    proxy.setType(proxyType);
    proxy.setHostName(m_pUi->leProxyServerName->text());
    proxy.setPort(m_pUi->leProxyServerPort->text().toUInt());
    if (m_pUi->cbProxyRequiresPassword->isChecked()) {
        proxy.setUser(m_pUi->leProxyUsername->text());
        proxy.setPassword(m_pUi->leProxyPassword->text());
    }

    QNetworkProxy::setApplicationProxy(proxy);
}

void OptionDialog::initSystemWidget() {
    int rebuildInterval = g_settings->value(OPTION_REBUILDTIMER, OPTION_REBUILDTIMER_DEFAULT).toInt();
    m_pUi->genRebuildMinutes->setValue(rebuildInterval);
    m_pUi->genRebuildCatalog->setChecked(rebuildInterval > 0);
    connect(m_pUi->genRebuildCatalog, SIGNAL(stateChanged(int)), this, SLOT(autoRebuildCheckChanged(int)));

    m_pUi->genShowHidden->setChecked(g_settings->value(OPTION_SHOWHIDDENFILES, OPTION_SHOWHIDDENFILES_DEFAULT).toBool());
    m_pUi->genShowNetwork->setChecked(g_settings->value(OPTION_SHOWNETWORK, OPTION_SHOWNETWORK_DEFAULT).toBool());

    m_pUi->genPortable->setChecked(SettingsManager::instance().isPortable());

    m_pUi->cbLogLevel->setCurrentIndex(g_settings->value(OPTION_LOGLEVEL, OPTION_LOGLEVEL_DEFAULT).toInt());
    connect(m_pUi->cbLogLevel, SIGNAL(currentIndexChanged(int)), this, SLOT(logLevelChanged(int)));

    // language
    QString lang = TranslationManager::instance().getLocale().name();
    /*
    g_settings->value(OPTION_LANGUAGE, OPTION_LANGUAGE_DEFAULT).toString();
    if (lang.isEmpty()) {
        lang = TranslationManager::instance().getLocale().name();
    }
    */

    m_pUi->cbLanguage->addItem(QString("English"), QString("en")); // English is default
    int indexLang = 0;
    // load language from directory
    QList<QLocale> locales = TranslationManager::instance().getAllLocales();
    for (int i = 0; i < locales.size(); ++i) {
        const QLocale& loc = locales.at(i);
        m_pUi->cbLanguage->addItem(loc.nativeLanguageName(), loc.name());
        if (lang == loc.name()) {
            indexLang = i + 1;
        }
    }

    // set combo box language from setting file
    m_pUi->cbLanguage->setCurrentIndex(indexLang);

    connect(m_pUi->cbLanguage, SIGNAL(currentIndexChanged(int)), this, SLOT(languageChanged(int)));
}

void OptionDialog::saveSystemSettings() {
    g_settings->setValue(OPTION_REBUILDTIMER,
                         m_pUi->genRebuildCatalog->isChecked() ? m_pUi->genRebuildMinutes->value() : 0);

    g_settings->setValue(OPTION_SHOWHIDDENFILES, m_pUi->genShowHidden->isChecked());
    g_settings->setValue(OPTION_SHOWNETWORK, m_pUi->genShowNetwork->isChecked());
    SettingsManager::instance().setPortable(m_pUi->genPortable->isChecked());

    g_settings->setValue(OPTION_LOGLEVEL, m_pUi->cbLogLevel->currentIndex());

    g_settings->setValue(OPTION_LANGUAGE, m_pUi->cbLanguage->currentData().toString());
}

void OptionDialog::initAboutWidget() {
    // About
    m_pUi->aboutVer->setText(tr("Version %1").arg(LAUNCHY_VERSION_STRING));
    m_pUi->aboutBit->setText(tr("(%1-bit)").arg(LAUNCHY_BIT_STRING));
}

void OptionDialog::addDirectory(const QString& directory, bool edit) {
    QString nativeDir = QDir::toNativeSeparators(directory);
    Directory dir(nativeDir);
    m_memDirs.append(dir);

    m_pUi->catTypes->clear();
    QListWidgetItem* item = new QListWidgetItem(nativeDir, m_pUi->catDirectories);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    m_pUi->catDirectories->setCurrentItem(item);
    if (edit) {
        m_pUi->catDirectories->editItem(item);
    }

    ++g_needRebuildCatalog;
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

    ++g_needRebuildCatalog;
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

    ++g_needRebuildCatalog;
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

    ++g_needRebuildCatalog;
}

void OptionDialog::catDepthChanged(int d) {
    int row = m_pUi->catDirectories->currentRow();
    if (row == -1) {
        return;
    }
    m_memDirs[row].depth = d;
    ++g_needRebuildCatalog;
}

} // namespace launchy
