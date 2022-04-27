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

#include "CharListWidget.h"

#include <QScrollBar>
#include <QKeyEvent>
#include <QApplication>
#include <QScreen>
#include <QDebug>

#include "LaunchyLib/LaunchyLib.h"

#include "IconDelegate.h"
#include "OptionItem.h"

namespace launchy {

CharListWidget::CharListWidget(QWidget* parent)
    : QListWidget(parent),
      m_iconListDelegate(new IconDelegate(this)),
      m_defaultListDelegate(itemDelegate()),
      m_alternativePath(new QLabel(this)) {
#ifdef Q_OS_LINUX
    setWindowFlags(Qt::Dialog | Qt::ToolTip);
#else
    setWindowFlags(Qt::Window | Qt::Tool | Qt::FramelessWindowHint);
#endif
    setAttribute(Qt::WA_AlwaysShowToolTips);
    setAlternatingRowColors(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTextElideMode(Qt::ElideLeft);
    setUniformItemSizes(true);

    verticalScrollBar()->setObjectName("altScroll");
    m_alternativePath->setObjectName("alternativesPath");
    m_alternativePath->hide();
    m_iconListDelegate->setAlternativePathWidget(m_alternativePath);
}

void CharListWidget::updateGeometry(const QPoint& basePos, const QPoint& offset) {
    // Now resize and reposition the list
    int numViewable = g_settings->value(OPTION_NUMVIEWABLE, OPTION_NUMVIEWABLE_DEFAULT).toInt();
    int min = count() < numViewable ? count() : numViewable;

    // The stylesheet doesn't load immediately, so we cache the placement rectangle here
    if (m_baseGeometry.isNull()) {
        m_baseGeometry = geometry();
        qDebug() << "CharListWidget::updateGeometry, base geometry(x y h w):"
            << m_baseGeometry.x()
            << m_baseGeometry.y()
            << m_baseGeometry.height()
            << m_baseGeometry.width();
    }

    QRect rect = m_baseGeometry;

    //QRect rect = m_alternativeList->geometry();
    rect.setHeight(min * sizeHintForRow(0));
    rect.translate(basePos);

    qDebug() << "CharListWidget::updateGeometry, height:" << rect.height();

    // Is there room for the dropdown box?
    QScreen* screen = qApp->screenAt(basePos);
    QRect screenSize = screen->geometry();
    if (rect.y() + rect.height() > screenSize.height()) {
        // Only move it if there's more space above
        // In both cases, ensure it doesn't spill off the screen
        if (basePos.y() + offset.y() > screenSize.height() / 2) {
            rect.moveTop(basePos.y() + offset.y() - rect.height());
            if (rect.top() < 0)
                rect.setTop(0);
        }
        else {
            rect.setBottom(screenSize.height());
        }
    }

    setGeometry(rect);
}

void CharListWidget::resetGeometry() {
    m_baseGeometry = QRect();
}

void CharListWidget::setListMode(int mode) {
    if (mode) {
        setItemDelegate(m_defaultListDelegate);
    }
    else {
        setItemDelegate(m_iconListDelegate);
    }
}

void CharListWidget::setIconSize(int size) {
    m_iconListDelegate->setSize(size);
}

void CharListWidget::keyPressEvent(QKeyEvent* event) {
    /*
    qDebug() << "CharListWidget::keyPressEvent, T1, key:" << event->key()
        << "mod:" << event->modifiers() << "current row:" << currentRow();
    */

    QListWidget::keyPressEvent(event);
    emit keyPressed(event);
}

void CharListWidget::mouseDoubleClickEvent(QMouseEvent* /*event*/) {
    QKeyEvent key(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
    emit keyPressed(&key);
}

void CharListWidget::focusInEvent(QFocusEvent* event) {
    QListWidget::focusInEvent(event);
    emit focusIn();
}

void CharListWidget::focusOutEvent(QFocusEvent* event) {
    qDebug() << "CharListWidget::focusOutEvent";
    QListWidget::focusOutEvent(event);
    emit focusOut();
}
}
