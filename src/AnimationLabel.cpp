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

#include "precompiled.h"
#include "AnimationLabel.h"

AnimationLabel::AnimationLabel(QWidget* parent) :
	QLabel(parent)
{
	animation = new QMovie(this);
	animation->setCacheMode(QMovie::CacheAll);
}


AnimationLabel::~AnimationLabel()
{
	animation->deleteLater();
}


void AnimationLabel::LoadAnimation(const QString& animationPath)
{
	// Ensure the animation continues to run after loading a new graphic
	bool running = animation->state() == QMovie::Running;
	Stop();

	animation->setFileName(animationPath);
	if (animation->isValid())
	{
		setMovie(animation);
		if (running)
			Start();
	}
}


void AnimationLabel::Start()
{
	// If the animation isn't already running, start it
	if (!animation.isNull() &&
		(animation->state() == QMovie::NotRunning || animation->state() == QMovie::Paused))
	{
		animation->start();
		setHidden(false);
	}
}


void AnimationLabel::Stop()
{
	// If the animation is running, stop it
	if (!animation.isNull() && animation->state() == QMovie::Running)
	{
		setHidden(true);
		animation->stop();
	}
}
