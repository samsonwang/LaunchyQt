/*
Launchy: Application Launcher
Copyright (C) 2007-2009  Josh Karlin, Simon Capewell

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

#include <QStyledItemDelegate>
#include <QLabel>

#ifndef ICON_DELEGATE
#define ICON_DELEGATE


#define ROLE_SHORT Qt::DisplayRole
#define ROLE_FULL Qt::ToolTipRole
#define ROLE_ICON Qt::DecorationRole


class IconDelegate : public QStyledItemDelegate
{
	Q_OBJECT

public:
	IconDelegate(QObject *parent = 0);

	void paint(QPainter *painter, const QStyleOptionViewItem &option,
		const QModelIndex &index) const;

	QSize sizeHint(const QStyleOptionViewItem &option,
		const QModelIndex &index ) const;

	void setColor(QString line, bool hi = false)
	{
		if (!line.contains(","))
			color = QColor(line);

		QStringList spl = line.split(",");
		if (spl.count() != 3) return;
		if (!hi)
			color = QColor(spl.at(0).toInt(), spl.at(1).toInt(), spl.at(2).toInt());
		else
			hicolor = QColor(spl.at(0).toInt(), spl.at(1).toInt(), spl.at(2).toInt());
	}

	void setFamily(QString fam) { family = fam; }
	void setSize(int s) { size = s; }
	void setWeight(int w) { weight = w; }
	void setItalics(int i) { italics = i; }
	void setAlternativesPathWidget(QLabel* label) { alternativesPath = label; }

private:
	QColor color;
	QColor hicolor;
	QString family;
	int size;
	int weight;
	int italics;
	QLabel* alternativesPath;
};


#endif
