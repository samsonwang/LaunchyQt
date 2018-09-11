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

#include "precompiled.h"
#include "CharLineEdit.h"


CharLineEdit::CharLineEdit(QWidget* parent) :
	QLineEdit(parent)
{
	setAttribute(Qt::WA_InputMethodEnabled);
}


void CharLineEdit::keyPressEvent(QKeyEvent* event)
{
	bool handled = false;

	if (selectionStart() == -1)
	{
		switch (event->key())
		{
		case Qt::Key_Backspace:
			if (isAtEndOfSeparator())
			{
				// Delete separator characters in a single keypress.
				// Don't use setText This method maintains the undo history
				backspace();
				backspace();
				backspace();
				handled = true;
			}
			break;

		case Qt::Key_Delete:
			if (isAtStartOfSeparator())
			{
				del();
				del();
				del();
				handled = true;
			}
			break;

		case Qt::Key_Left:
			if (isAtEndOfSeparator())
			{
				cursorBackward(false, 3);
				handled = true;
			}
			break;

		case Qt::Key_Right:
			if (isAtStartOfSeparator())
			{
				cursorForward(false, 3);
				handled = true;
			}
			break;
		}
	}

	if (handled)
	{
		event->ignore();
	}
	else
	{
		QLineEdit::keyPressEvent(event);
	}

	emit keyPressed(event);
}


// This is how you pick up the tab key
bool CharLineEdit::focusNextPrevChild(bool next)
{
	QKeyEvent event(QEvent::KeyPress, Qt::Key_Tab, next ? Qt::NoModifier : Qt::ShiftModifier);
	emit keyPressed(&event);
	
	return true;
}


void CharLineEdit::focusInEvent(QFocusEvent* event)
{
	QLineEdit::focusInEvent(event);

	emit focusIn(event);
}


void CharLineEdit::focusOutEvent(QFocusEvent* event)
{
	QLineEdit::focusOutEvent(event);

	emit focusOut(event);
}


void CharLineEdit::inputMethodEvent(QInputMethodEvent* event)
{
	QLineEdit::inputMethodEvent(event);

	if (event->commitString() != "")
	{
		emit inputMethod(event);
	}
}


QChar CharLineEdit::separatorChar() const
{
	QFontMetrics met = fontMetrics();
	QChar arrow(0x25ba);
	if (met.inFont(arrow))
		return arrow;
	else
		return QChar('|');
}


QString CharLineEdit::separatorText() const
{
	return QString(" ") + separatorChar() + " ";
}


bool CharLineEdit::isAtStartOfSeparator() const
{
	return text().mid(cursorPosition(), 3) == separatorText();
}


bool CharLineEdit::isAtEndOfSeparator() const
{
	return text().mid(cursorPosition() - 3, 3) == separatorText();
}
