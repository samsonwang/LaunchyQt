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
#include "weby.h"

#define ROW_PADDING 6


Gui::Gui(QWidget* parent, QSettings* settings) 
	: QWidget(parent), settings(settings)
{
	setupUi(this);
	if (settings == NULL)
		return;
	booksFirefox->setChecked(settings->value("weby/firefox", true).toBool());
	booksIE->setChecked(settings->value("weby/ie", true).toBool());

	// Stretch the last column of the table
	table->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch); //  column 1

	// Read in the array of websites from options
	table->setSortingEnabled(false);
	int count = settings->beginReadArray("weby/sites");
	table->setRowCount(count);

	for(int i = 0; i < count; ++i) {
		settings->setArrayIndex(i);
		table->setItem(i, 0, new QTableWidgetItem(settings->value("name").toString()));
		table->setItem(i, 1, new QTableWidgetItem(settings->value("query").toString()));
		//table->setItem(i, 2, new QTableWidgetItem(settings->value("suggest").toString()));
		bool isDef = settings->value("default",false).toBool();
		if (isDef) {
		    defaultName = settings->value("name").toString();
		    label_default->setText(defaultName);
		}

		table->verticalHeader()->resizeSection(i, table->verticalHeader()->fontMetrics().height() + ROW_PADDING);
	}
	settings->endArray();
	table->setSortingEnabled(true);

	connect(table, SIGNAL(dragEnter(QDragEnterEvent*)), this, SLOT(dragEnter(QDragEnterEvent*)));
	connect(table, SIGNAL(drop(QDropEvent*)), this, SLOT(drop(QDropEvent*)));
	connect(tableNew, SIGNAL(clicked(bool)), this, SLOT(newRow(void)));
	connect(tableRemove, SIGNAL(clicked(bool)), this, SLOT(remRow(void)));
	connect(pushDefault, SIGNAL(clicked(bool)), this, SLOT(makeDefault(void)));
	connect(pushClearDefault, SIGNAL(clicked(bool)), this, SLOT(clearDefault(void)));
}

void Gui::writeOptions()
{
	if (settings == NULL)
		return;
	settings->setValue("weby/firefox", booksFirefox->isChecked());
	settings->setValue("weby/ie", booksIE->isChecked());

	settings->beginWriteArray("weby/sites");
	for(int i = 0; i < table->rowCount(); ++i) {
		if (table->item(i,0) == NULL || table->item(i,1) == NULL) continue;
		if (table->item(i,0)->text() == "" || table->item(i,1)->text() == "") continue;
		settings->setArrayIndex(i);
		settings->setValue("name", table->item(i, 0)->text());
		settings->setValue("query", table->item(i, 1)->text());
		//settings->setValue("suggest", table->item(i, 2)->text());	
		if (table->item(i,0)->text() == defaultName)
		    settings->setValue("default", true);
		else
		    settings->setValue("default", false);
		
	}
	settings->endArray();
}

void Gui::newRow() 
{
	bool sort = table->isSortingEnabled();
	if (sort)
		table->setSortingEnabled(false);
	int row = table->rowCount();
	table->insertRow(row);
	table->setItem(row, 0, new QTableWidgetItem());
	table->setItem(row, 1, new QTableWidgetItem());
	table->setItem(row, 2, new QTableWidgetItem());
	table->setItem(row, 3, new QTableWidgetItem());
	table->verticalHeader()->resizeSection(table->rowCount()-1, table->verticalHeader()->fontMetrics().height() + ROW_PADDING);
	table->setCurrentCell(table->rowCount()-1, 0);
	table->editItem(table->currentItem());
	table->setSortingEnabled(sort);
}

void Gui::remRow()
{
	int row = table->currentRow();
	if (row != -1)
	{
		table->removeRow(row);
		if (row >= table->rowCount())
			row = table->rowCount() - 1;
		table->setCurrentCell(row, table->currentColumn());
	}
}

void Gui::dragEnter(QDragEnterEvent *event)
{
	const QMimeData* mimeData = event->mimeData();
	if (mimeData && (mimeData->hasUrls() || mimeData->hasText()))
		event->acceptProposedAction();
}

void Gui::drop(QDropEvent *event)
{
	const QMimeData* mimeData = event->mimeData();
	if (mimeData) {
		if (mimeData->hasUrls()) {
			foreach(QUrl url, mimeData->urls()) {
				table->setSortingEnabled(false);
				QString qs = QUrl::fromPercentEncoding(url.encodedQuery());
				appendRow(url.path() ,qs);
//				appendRow(url.path(), url.toString(QUrl::RemoveQuery), qs);
				table->setCurrentCell(table->rowCount()-1, 0);
				table->setSortingEnabled(true);
			}
		}
		else if (mimeData->hasText()) {
			table->setSortingEnabled(false);
			appendRow(mimeData->text(), "");
			table->setCurrentCell(table->rowCount()-1, 0);
			table->setSortingEnabled(true);
		}
	}
}

void Gui::makeDefault()
{
    int row = table->currentRow();
    if (row > -1)
	{
		defaultName = table->item(row,0)->text();
		label_default->setText(defaultName);
	}
}

void Gui::clearDefault()
{
	defaultName = "";
    label_default->setText(defaultName);
}

void Gui::appendRow(const QString& name, const QString& path)
{
	int row = table->rowCount();
	table->insertRow(row);
	table->setItem(row, 0, new QTableWidgetItem(name));
	table->setItem(row, 1, new QTableWidgetItem(path));
	table->setItem(row, 2, new QTableWidgetItem());
	table->verticalHeader()->resizeSection(row, table->verticalHeader()->fontMetrics().height() + ROW_PADDING);
}
