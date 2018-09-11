/*
Launchy: Application Launcher
Copyright (C) 2007-2010  Josh Karlin

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

#include <QtGui>
#include "gui.h"
#include "myplugin.h"


Gui::Gui(QWidget* parent) 
	: QWidget(parent)
{
	setupUi(this);
        QSettings* settings = *gmypluginInstance->settings;
        if (settings == NULL)
            return;

        chkOption1->setChecked(settings->value("myplugin/option1", true).toBool());
        chkOption2->setChecked(settings->value("myplugin/option2", true).toBool());
}


Gui::~Gui()
{
    this->hide();
}


void Gui::writeOptions()
{
        QSettings* settings = *gmypluginInstance->settings;
        if (settings == NULL)
            return;

        settings->setValue("myplugin/option1", chkOption1->isChecked());
        settings->setValue("myplugin/option2", chkOption2->isChecked());
}
