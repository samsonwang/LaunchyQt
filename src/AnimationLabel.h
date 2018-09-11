/*
Launchy: Application Launcher
Copyright (C) 2010  Simon Capewell

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

#ifndef QANIMATIONLABEL_H
#define QANIMATIONLABEL_H
 

#include <QVBoxLayout>
#include <QLabel>
#include <QMovie>


class AnimationLabel : public QLabel
{
public:
	AnimationLabel(QWidget* parent);
	virtual ~AnimationLabel();
	void LoadAnimation(const QString& animationPath);

public slots:
	void Start();
	void Stop();

private:
	QPointer<QMovie> animation;
};


#endif