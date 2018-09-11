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

#ifndef FADER_H
#define FADER_H

#include "precompiled.h"

class Fader : public QThread
{
	Q_OBJECT

public:
	Fader(QObject* parent = NULL);
	~Fader();
	
	void fadeIn(bool quick);
	void fadeOut(bool quick);
	void run();

	inline void stop() { keepRunning = false; }
	inline bool isFading() const { return delta < 0 && isRunning(); }

signals:
	void fadeLevel(double level);

private:
	QMutex mutex;
	bool keepRunning;

	double delta;
	int delay;

	double level;
	double targetLevel;
};


#endif
