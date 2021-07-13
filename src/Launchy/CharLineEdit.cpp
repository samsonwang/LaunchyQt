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

#include "CharLineEdit.h"

#include <QDebug>
#include <QKeyEvent>

#ifdef Q_OS_MAC
#include <QMacStyle>
#endif

namespace launchy {
CharLineEdit::CharLineEdit(QWidget* parent)
    : QLineEdit(parent) {
    setAttribute(Qt::WA_InputMethodEnabled);
    setContextMenuPolicy(Qt::NoContextMenu);
#ifdef Q_OS_MAC
    QMacStyle::setFocusRectPolicy(this, QMacStyle::FocusDisabled);
#endif
}

void CharLineEdit::processKey(QKeyEvent* event) {
    qDebug() << "CharLineEdit::processKey, key:" << event->key()
        << "mod:" << event->modifiers()
        << "text:" << event->text();

    bool handled = false;

    if (selectionStart() == -1) {
        switch (event->key()) {
        case Qt::Key_Backspace:
            if (isAtEndOfSeparator()) {
                // Delete separator characters in a single keypress.
                // Don't use setText This method maintains the undo history
                backspace();
                backspace();
                backspace();
                handled = true;
            }
            break;

        case Qt::Key_Delete:
            if (isAtStartOfSeparator()) {
                del();
                del();
                del();
                handled = true;
            }
            break;

        case Qt::Key_Left:
            if (isAtEndOfSeparator()) {
                cursorBackward(false, 3);
                handled = true;
            }
            break;

        case Qt::Key_Right:
            if (isAtStartOfSeparator()) {
                cursorForward(false, 3);
                handled = true;
            }
            break;
        }
    }

    if (handled) {
        event->ignore();
    }
    else {
        QLineEdit::keyPressEvent(event);
    }

    emit keyPressed(event);
}

// This is how you pick up the tab key
bool CharLineEdit::focusNextPrevChild(bool next) {
    qDebug() << "CharLineEdit::focusNextPrevChild, next =" << next;

    QKeyEvent event(QEvent::KeyPress, Qt::Key_Tab, next ? Qt::NoModifier : Qt::ShiftModifier);
    emit keyPressed(&event);

    return true;
}

void CharLineEdit::keyPressEvent(QKeyEvent* event) {
    qDebug() << "CharLineEdit::keyPressEvent, key:" << event->key()
        << "mod:" << event->modifiers()
        << "text:" << event->text();

    processKey(event);
}

void CharLineEdit::focusInEvent(QFocusEvent* event) {
    QLineEdit::focusInEvent(event);
    emit focusIn();
}

void CharLineEdit::focusOutEvent(QFocusEvent* event) {
    QLineEdit::focusOutEvent(event);
    emit focusOut();
}

void CharLineEdit::inputMethodEvent(QInputMethodEvent* event) {
    qDebug() << "CharLineEdit::inputMethodEvent, commitString:" << event->commitString()
        << "preeditString:" <<  event->preeditString();
    QLineEdit::inputMethodEvent(event);

    if (!event->commitString().isEmpty()) {
        emit inputMethod(event);
    }
}

QChar CharLineEdit::separatorChar() const {
    QFontMetrics met = fontMetrics();
    QChar arrow(0x25ba);
    if (met.inFont(arrow)) {
        return arrow;
    }
    else {
        return QChar('|');
    }
}

QString CharLineEdit::separatorText() const {
    return QString(" ") + separatorChar() + QString(" ");
}

bool CharLineEdit::isAtStartOfSeparator() const {
    return text().mid(cursorPosition(), 3) == separatorText();
}

bool CharLineEdit::isAtEndOfSeparator() const {
    return text().mid(cursorPosition() - 3, 3) == separatorText();
}
}
