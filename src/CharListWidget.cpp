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
#include "CharListWidget.h"


CharListWidget::CharListWidget(QWidget* parent) : 
	QListWidget(parent)
{
#ifdef Q_WS_X11
	setWindowFlags( windowFlags() | Qt::Tool | Qt::SplashScreen);
#endif
	setAttribute(Qt::WA_AlwaysShowToolTips);

	setAlternatingRowColors(true);
}


void CharListWidget::keyPressEvent(QKeyEvent* key)
{
	emit keyPressed(key);
	QListWidget::keyPressEvent(key);
	key->ignore();
}


void CharListWidget::mouseDoubleClickEvent(QMouseEvent* /*event*/)
{
	QKeyEvent key(QEvent::KeyPress, Qt::Key_Enter, NULL);
	emit keyPressed(&key);
}


void CharListWidget::focusInEvent(QFocusEvent* event)
{
	emit focusIn(event);
}


void CharListWidget::focusOutEvent(QFocusEvent* event)
{
	emit focusOut(event);
}
