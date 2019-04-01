/*
Launchy: Application Launcher
Copyright (C) 2007  Josh Karlin

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
#include "gui.h"
#include "ui_dlg.h"
#include "Runner.h"
#include "globals.h"


#define ROW_PADDING 6

Gui::Gui(QWidget* parent)
    : QWidget(parent),
      m_dlg(new Ui::Dlg) {

    m_dlg->setupUi(this);

    // Stretch the centre column of the table
    m_dlg->table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    // Read in the array of programs from options
    m_dlg->table->setSortingEnabled(false);
    // m_dlg->table->setItemDelegateForColumn(1, &delegate);
    int count = launchy::g_settings->beginReadArray(RUNNER_COMMANDS);
    m_dlg->table->setRowCount(count);

    for (int i = 0; i < count; ++i) {
        launchy::g_settings->setArrayIndex(i);
        m_dlg->table->setItem(i, 0, new QTableWidgetItem(launchy::g_settings->value("name").toString()));
        m_dlg->table->setItem(i, 1, new QTableWidgetItem(launchy::g_settings->value("file").toString()));
        m_dlg->table->setItem(i, 2, new QTableWidgetItem(launchy::g_settings->value("args").toString()));
        //m_dlg->table->verticalHeader()->resizeSection(i, m_dlg->table->verticalHeader()->fontMetrics().height() + ROW_PADDING);
    }
    launchy::g_settings->endArray();
    m_dlg->table->setSortingEnabled(true);

    //connect(m_dlg->table, SIGNAL(dragEnter(QDragEnterEvent*)), this, SLOT(dragEnter(QDragEnterEvent*)));
    //connect(m_dlg->table, SIGNAL(drop(QDropEvent*)), this, SLOT(drop(QDropEvent*)));
    connect(m_dlg->table, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(tableItemChanged(QTableWidgetItem*)));

    connect(m_dlg->tableNew, SIGNAL(clicked(bool)), this, SLOT(createRow(void)));
    connect(m_dlg->tableRemove, SIGNAL(clicked(bool)), this, SLOT(removeRow(void)));
}

Gui::~Gui() {
    if (m_dlg) {
        delete m_dlg;
        m_dlg = nullptr;
    }
}

void Gui::writeOptions() {
    if (launchy::g_settings == NULL) {
        return;
    }

    launchy::g_settings->beginWriteArray(RUNNER_COMMANDS);
    for (int i = 0; i < m_dlg->table->rowCount(); ++i) {
        if (m_dlg->table->item(i, 0) == NULL || m_dlg->table->item(i, 1) == NULL) continue;
        if (m_dlg->table->item(i, 0)->text() == "" || m_dlg->table->item(i, 1)->text() == "") continue;
        launchy::g_settings->setArrayIndex(i);
        launchy::g_settings->setValue("name", m_dlg->table->item(i, 0)->text());
        launchy::g_settings->setValue("file", m_dlg->table->item(i, 1)->text());
        if (m_dlg->table->item(i, 2) == NULL) {
            launchy::g_settings->setValue("args", "");
        }
        else {
            launchy::g_settings->setValue("args", m_dlg->table->item(i, 2)->text());
        }
    }
    launchy::g_settings->endArray();
}

void Gui::createRow() {
    bool sort = m_dlg->table->isSortingEnabled();
    if (sort) {
        m_dlg->table->setSortingEnabled(false);
    }

    appendRow(QString(), QString(), QString());
    m_dlg->table->setCurrentCell(m_dlg->table->rowCount()-1, 0);
    m_dlg->table->editItem(m_dlg->table->currentItem());
    m_dlg->table->setSortingEnabled(sort);

    ++launchy::g_needRebuildCatalog;
}

void Gui::removeRow() {
    int row = m_dlg->table->currentRow();
    if (row != -1) {
        m_dlg->table->removeRow(row);
        if (row >= m_dlg->table->rowCount())
            row = m_dlg->table->rowCount() - 1;
        m_dlg->table->setCurrentCell(row, m_dlg->table->currentColumn());
    }

    ++launchy::g_needRebuildCatalog;
}

void Gui::tableItemChanged(QTableWidgetItem* pItem) {
    Q_UNUSED(pItem)
    ++launchy::g_needRebuildCatalog;
}

void Gui::dragEnter(QDragEnterEvent* event) {
    const QMimeData* mimeData = event->mimeData();
    if (mimeData && mimeData->hasUrls()) {
        event->acceptProposedAction();
    }
}

void Gui::drop(QDropEvent *event) {
    const QMimeData* mimeData = event->mimeData();
    if (mimeData && mimeData->hasUrls()) {
        foreach(QUrl url, mimeData->urls()) {
            QFileInfo info(url.toLocalFile());
            if (info.exists()) {
                m_dlg->table->setSortingEnabled(false);
                if (info.isSymLink()) {
                    QFileInfo target(info.symLinkTarget());
                    appendRow(info.baseName(), QDir::toNativeSeparators(target.filePath()), "");
                }
                else
                    appendRow(info.baseName(), QDir::toNativeSeparators(info.filePath()), "");
                m_dlg->table->setCurrentCell(m_dlg->table->rowCount()-1, 0);
                m_dlg->table->setSortingEnabled(true);
            }
        }
    }
}

void Gui::appendRow(const QString& name, const QString& file, const QString& args) {
    int row = m_dlg->table->rowCount();
    m_dlg->table->insertRow(row);
    m_dlg->table->setItem(row, 0, new QTableWidgetItem(name));
    m_dlg->table->setItem(row, 1, new QTableWidgetItem(file));
    m_dlg->table->setItem(row, 2, new QTableWidgetItem(args));
    //m_dlg->table->verticalHeader()->resizeSection(row, m_dlg->table->verticalHeader()->fontMetrics().height() + ROW_PADDING);
}
