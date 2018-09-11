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
#include "Fader.h"
#include "globals.h"


Fader::Fader(QObject* parent) :
	QThread(parent),
	keepRunning(true)
{
	setObjectName("Fader");
}


Fader::~Fader()
{
}


void Fader::fadeIn(bool quick)
{
        int time = gSettings->value("GenOps/fadein", 0).toInt();

	mutex.lock();
	targetLevel = gSettings->value("GenOps/opaqueness", 100).toInt() / 100.0;
	delta = 0.05;
	delay = quick ? 0 : (int)(time * delta / targetLevel);
	if (delay > 10)
	{
		delta /= 10;
		delay /= 10;
	}
	mutex.unlock();

	if (quick || delay == 0)
	{
		// stop any current slow fades
		stop();
		wait();
		emit fadeLevel(targetLevel);
	}
	else if (!isRunning())
	{
		level = 0;
		start();
	}
}


void Fader::fadeOut(bool quick)
{
	int time = gSettings->value("GenOps/fadeout", 0).toInt();
	double opaqueness = gSettings->value("GenOps/opaqueness", 100).toInt() / 100.0;

	mutex.lock();
	targetLevel = 0;
	delta = -0.05;
	delay = quick ? 0 : (int)(time * -delta / opaqueness);
	if (delay > 10)
	{
		delta /= 10;
		delay /= 10;
	}
	mutex.unlock();

	// qDebug() << level << " to " << targetLevel << " delay " << delay;

	if (quick || delay == 0)
	{
		// stop any current slow fades
		stop();
		wait();
		emit fadeLevel(targetLevel);
	}
	else if (!isRunning())
	{
		level = opaqueness;
		start();
	}
}


void Fader::run()
{
	keepRunning = true;

	while (keepRunning)
	{
		mutex.lock();
		level += delta;
		keepRunning &= (delta > 0 && level < targetLevel) || (delta < 0 && level > targetLevel);
		mutex.unlock();

		// qDebug() << delta << level << targetLevel << keepRunning;

		if (keepRunning)
		{
			emit fadeLevel(level);
			msleep(delay);
		}
	}

	// qDebug() << delta << targetLevel << targetLevel;
	// qDebug() << "";

	emit fadeLevel(targetLevel);
}
