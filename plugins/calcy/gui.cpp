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
#include "calcy.h"
#include "LaunchyLib.h"

Gui::Gui(QWidget* parent)
    : QWidget(parent) {
    setupUi(this);

    if (launchy::g_settings.isNull()) {
        return;
    }

    txtRounding->setValue(launchy::g_settings->value("calcy/outputRounding", 10).toInt());
    chkDigitGrouping->setChecked(launchy::g_settings->value("calcy/outputGroupSeparator", true).toBool());
    chkCopyToClipboard->setChecked(launchy::g_settings->value("calcy/copyToClipboard", true).toBool());
    chkComma->setChecked(launchy::g_settings->value("calcy/useCommaForDecimal", false).toBool());
}


Gui::~Gui() {
	this->hide();
}

void Gui::writeOptions() {
    if (launchy::g_settings.isNull()) {
        return;
    }

    launchy::g_settings->setValue("calcy/outputRounding", txtRounding->value());
    launchy::g_settings->setValue("calcy/outputGroupSeparator", chkDigitGrouping->isChecked());
    launchy::g_settings->setValue("calcy/copyToClipboard", chkCopyToClipboard->isChecked());
    launchy::g_settings->setValue("calcy/useCommaForDecimal", chkComma->isChecked());
}
