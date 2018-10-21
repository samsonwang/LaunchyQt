/*
Launchy: Application Launcher
Copyright (C) 2009  Simon Capewell

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


#include "FileBrowserDelegate.h"

FileBrowserDelegate::FileBrowserDelegate(QObject *parent, FileBrowser::BrowseType browseType)
	: QItemDelegate(parent),
	browseType(browseType)
{
}

QWidget *FileBrowserDelegate::createEditor(QWidget *parent,
										const QStyleOptionViewItem & /*option*/ ,
										const QModelIndex & /*index*/ ) const
{
	FileBrowser *editor = new FileBrowser(parent);
	editor->setBrowseType(browseType);

	return editor;
}

void FileBrowserDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	QString value = index.model()->data(index, Qt::EditRole).toString();

	FileBrowser *edit = static_cast<FileBrowser*>(editor);
	edit->setFilename(value);
}

void FileBrowserDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	FileBrowser *edit = static_cast<FileBrowser*>(editor);
	QString value = edit->getFilename();

	model->setData(index, value, Qt::EditRole);
}

void FileBrowserDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const
{
	editor->setGeometry(option.rect);
}
