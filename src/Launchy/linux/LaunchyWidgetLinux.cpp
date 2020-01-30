/*
  LaunchyQt
  Copyright (C) 2019 Samson Wang

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "LaunchyWidgetLinux.h"

namespace launchy {

LaunchyWidgetLinux::LaunchyWidgetLinux(CommandFlags command)
    : LaunchyWidget(command) {
}

void LaunchyWidgetLinux::focusLaunchy() {
    /* Fix for bug 2994680: Not sure why this is necessary, perhaps someone with more
       Qt experience can tell, but doing these two calls will force the window to actually
       get keyboard focus when it is activated. It seems from the bug reports that this
       only affects Linux (and I could only test it on my Linux system - running KDE), so
       it leads me to believe that it is due to an issue in the Qt implementation on Linux. */
    grabKeyboard();
    releaseKeyboard();
    LaunchyWidget::focusLaunchy();
}

// Create the main widget for the application
void createLaunchyWidget(CommandFlags command) {
    if (!LaunchyWidget::s_instance) {
        LaunchyWidget::s_instance = new LaunchyWidgetLinux(command);

        // dirty approach to make Qhotkey works, do not know why
        if (!LaunchyWidget::s_instance->isVisible()) {
            qreal opac = LaunchyWidget::s_instance->windowOpacity();
            LaunchyWidget::s_instance->setWindowOpacity(0.01);
            LaunchyWidget::s_instance->show();
            LaunchyWidget::s_instance->hide();
            LaunchyWidget::s_instance->setWindowOpacity(opac);
        }
    }
}

}
