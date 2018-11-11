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

#pragma once

#include <QListWidget>
class IconDelegate;

class CharListWidget : public QListWidget {
    Q_OBJECT
public:
    CharListWidget(QWidget* parent = 0);
    
    void updateGeometry(const QPoint& basePos, const QPoint& offset);
    void resetGeometry();
    void setListMode(int mode);
    void setIconSize(int size);

protected:
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void focusInEvent(QFocusEvent* event);
    virtual void focusOutEvent(QFocusEvent* event);

signals:
    void keyPressed(QKeyEvent* event);
    void focusIn(QFocusEvent* event);
    void focusOut(QFocusEvent* event);

private:
    QRect m_baseGeometry;
    IconDelegate* m_iconListDelegate;
    QAbstractItemDelegate* m_defaultListDelegate;
    QLabel* m_alternativePath;
};
