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
#include "runner.h"


#define ROW_PADDING 6


Gui::Gui(QWidget* parent, QSettings* settings) 
	: QWidget(parent), settings(settings)
{
	setupUi(this);
	if (settings == NULL)
		return;

	// Stretch the centre column of the table
	table->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch); //  column 1

	// Read in the array of programs from options
	table->setSortingEnabled(false);
	table->setItemDelegateForColumn(1, &delegate);
	int count = settings->beginReadArray("runner/cmds");
	table->setRowCount(count);

	for(int i = 0; i < count; ++i) {
		settings->setArrayIndex(i);
		table->setItem(i, 0, new QTableWidgetItem(settings->value("name").toString()));
		table->setItem(i, 1, new QTableWidgetItem(settings->value("file").toString()));
		table->setItem(i, 2, new QTableWidgetItem(settings->value("args").toString()));
		table->verticalHeader()->resizeSection(i, table->verticalHeader()->fontMetrics().height() + ROW_PADDING);
	}
	settings->endArray();
	table->setSortingEnabled(true);

	connect(table, SIGNAL(dragEnter(QDragEnterEvent*)), this, SLOT(dragEnter(QDragEnterEvent*)));
	connect(table, SIGNAL(drop(QDropEvent*)), this, SLOT(drop(QDropEvent*)));
	connect(tableNew, SIGNAL(clicked(bool)), this, SLOT(newRow(void)));
	connect(tableRemove, SIGNAL(clicked(bool)), this, SLOT(remRow(void)));
}

void Gui::writeOptions()
{
	if (settings == NULL)
		return;

	settings->beginWriteArray("runner/cmds");
	for(int i = 0; i < table->rowCount(); ++i) {
		if (table->item(i,0) == NULL || table->item(i,1) == NULL) continue;
		if (table->item(i,0)->text() == "" || table->item(i,1)->text() == "") continue;
		settings->setArrayIndex(i);
		settings->setValue("name", table->item(i, 0)->text());
		settings->setValue("file", table->item(i, 1)->text());
		if (table->item(i,2) == NULL)
			settings->setValue("args", "");
		else
			settings->setValue("args", table->item(i, 2)->text());
	}
	settings->endArray();
}

void Gui::newRow() 
{
	bool sort = table->isSortingEnabled();
	if (sort)
		table->setSortingEnabled(false);
	appendRow(QString(), QString(), QString());
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
	if (mimeData && mimeData->hasUrls())
		event->acceptProposedAction();
}

void Gui::drop(QDropEvent *event)
{
	const QMimeData* mimeData = event->mimeData();
	if (mimeData && mimeData->hasUrls()) {
		foreach(QUrl url, mimeData->urls()) {
			QFileInfo info(url.toLocalFile());
			if(info.exists()) {
				table->setSortingEnabled(false);
				if (info.isSymLink()) {
					QFileInfo target(info.symLinkTarget());
					appendRow(info.baseName(), QDir::toNativeSeparators(target.filePath()), "");
				}
				else
					appendRow(info.baseName(), QDir::toNativeSeparators(info.filePath()), "");
				table->setCurrentCell(table->rowCount()-1, 0);
				table->setSortingEnabled(true);
			}
		}
	}
}

void Gui::appendRow(const QString& name, const QString& file, const QString& args)
{
	int row = table->rowCount();
	table->insertRow(row);
	table->setItem(row, 0, new QTableWidgetItem(name));
	table->setItem(row, 1, new QTableWidgetItem(file));
	table->setItem(row, 2, new QTableWidgetItem(args));
	table->verticalHeader()->resizeSection(row, table->verticalHeader()->fontMetrics().height() + ROW_PADDING);
}
