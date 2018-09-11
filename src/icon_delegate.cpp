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
#include "icon_delegate.h"
#include "globals.h"
#include "catalog_types.h"

extern QLabel* alternativesPath;

IconDelegate::IconDelegate(QObject *parent) :
	QStyledItemDelegate(parent)
{
}

void IconDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
						 const QModelIndex &index) const
{
	painter->save();
	if (option.state & QStyle::State_Selected)
	{
		painter->fillRect(option.rect, option.palette.highlight());
		painter->setPen(option.palette.color(QPalette::HighlightedText));
	}

	QRect iconRect = option.rect;
	iconRect.setWidth(32);
	iconRect.setHeight(32);

	int fontHeight = painter->fontMetrics().height();
	QRect shortRect = option.rect;
	shortRect.setLeft(shortRect.left() + 38);
	shortRect.setBottom(shortRect.top() + fontHeight);

	QRect longRect = option.rect;
	longRect.setLeft(longRect.left() + 50);
	longRect.setTop(longRect.top() + fontHeight);

	QString text = Catalog::decorateText(index.data(ROLE_SHORT).toString(), gSearchText);
	painter->drawText(shortRect, Qt::AlignTop | Qt::TextShowMnemonic, text);

	if (option.state & QStyle::State_Selected)
		painter->setPen(alternativesPath->palette().color(QPalette::HighlightedText));
	else
		painter->setPen(alternativesPath->palette().color(QPalette::WindowText));

	painter->setFont(alternativesPath->font());

	QString full = index.data(ROLE_FULL).toString();
	full = painter->fontMetrics().elidedText(full, option.textElideMode, longRect.width());
	painter->drawText(longRect, Qt::AlignTop, full);

	QIcon p = index.data(ROLE_ICON).value<QIcon>();
	p.paint(painter, iconRect);
	painter->restore();
}


QSize IconDelegate::sizeHint(const QStyleOptionViewItem & /* option */,
							 const QModelIndex & /* index */) const
{
	return QSize(10, 32);
}
