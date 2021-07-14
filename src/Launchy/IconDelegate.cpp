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

#include "IconDelegate.h"

#include <QPainter>

#include "GlobalVar.h"
#include "Catalog.h"

namespace launchy {

IconDelegate::IconDelegate(QObject* parent)
    : QStyledItemDelegate(parent),
      m_size(32) {
}

void IconDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                         const QModelIndex &index) const {
    painter->save();
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
        painter->setPen(option.palette.color(QPalette::HighlightedText));
    }

    // qDebug() << "IconDelegate::paint" << option.rect;
    QRect iconRect(option.rect.x(), option.rect.y(), m_size, m_size);
    QIcon icon = index.data(ROLE_ICON).value<QIcon>();
    icon.paint(painter, iconRect);

    int fontHeight = painter->fontMetrics().height();
    QRect shortRect = option.rect;
    shortRect.setLeft(shortRect.left() + m_size + 6);
    shortRect.setBottom(shortRect.top() + fontHeight);

    QRect longRect = option.rect;
    longRect.setLeft(longRect.left() + m_size + 18);
    longRect.setTop(longRect.top() + fontHeight);

    QString text = Catalog::decorateText(index.data(ROLE_SHORT).toString(), g_searchText);
    painter->drawText(shortRect, Qt::AlignTop | Qt::TextShowMnemonic, text);

    if (option.state & QStyle::State_Selected)
        painter->setPen(m_alternativesPath->palette().color(QPalette::HighlightedText));
    else
        painter->setPen(m_alternativesPath->palette().color(QPalette::WindowText));

    painter->setFont(m_alternativesPath->font());

    QString full = index.data(ROLE_FULL).toString();
    full = painter->fontMetrics().elidedText(full, option.textElideMode, longRect.width());
    painter->drawText(longRect, Qt::AlignTop, full);

    painter->restore();
}

QSize IconDelegate::sizeHint(const QStyleOptionViewItem & /* option */,
                             const QModelIndex & /* index */) const {
    return QSize(10, m_size);
}

void IconDelegate::setColor(QString line, bool hi) {
    if (!line.contains(","))
        m_color = QColor(line);

    QStringList spl = line.split(",");
    if (spl.count() != 3) return;
    if (!hi)
        m_color = QColor(spl.at(0).toInt(), spl.at(1).toInt(), spl.at(2).toInt());
    else
        m_hiColor = QColor(spl.at(0).toInt(), spl.at(1).toInt(), spl.at(2).toInt());
}

void IconDelegate::setFamily(QString fam) {
    m_family = fam;
}

void IconDelegate::setSize(int s) {
    m_size = s;
}

void IconDelegate::setWeight(int w) {
    m_weight = w;
}

void IconDelegate::setItalics(int i) {
    italics = i;
}

void IconDelegate::setAlternativePathWidget(QLabel* label) {
    m_alternativesPath = label;
}

} // namespace launchy
