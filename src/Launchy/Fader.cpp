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

#include "Fader.h"
#include "LaunchyLib/LaunchyLib.h"
#include "OptionItem.h"

namespace launchy {

Fader::Fader(QObject* parent)
    : QThread(parent),
      m_keepRunning(true) {
    setObjectName("Fader");
}

Fader::~Fader() {

}

void Fader::fadeIn(bool quick) {
    int time = g_settings->value(OPTION_FADEIN, OPTION_FADEIN_DEFAULT).toInt();

    m_mutex.lock();
    m_targetLevel = g_settings->value(OPTION_OPAQUENESS, OPTION_OPAQUENESS_DEFAULT).toInt() / 100.0;
    m_delta = 0.05;
    m_delay = quick ? 0 : (int)(time * m_delta / m_targetLevel);
    if (m_delay > 10) {
        m_delta /= 10;
        m_delay /= 10;
    }
    m_mutex.unlock();

    if (quick || m_delay == 0) {
        // stop any current slow fades
        stop();
        wait();
        emit fadeLevel(m_targetLevel);
    }
    else if (!isRunning()) {
        m_level = 0;
        start();
    }
}

void Fader::fadeOut(bool quick) {
    int time = g_settings->value(OPTION_FADEOUT, OPTION_FADEOUT_DEFAULT).toInt();
    double opaqueness = g_settings->value(OPTION_OPAQUENESS, OPTION_OPAQUENESS_DEFAULT).toInt() / 100.0;

    m_mutex.lock();
    m_targetLevel = 0;
    m_delta = -0.05;
    m_delay = quick ? 0 : (int)(time * -m_delta / opaqueness);
    if (m_delay > 10) {
        m_delta /= 10;
        m_delay /= 10;
    }
    m_mutex.unlock();

    // qDebug() << level << " to " << targetLevel << " delay " << delay;

    if (quick || m_delay == 0) {
        // stop any current slow fades
        stop();
        wait();
        emit fadeLevel(m_targetLevel);
    }
    else if (!isRunning()) {
        m_level = opaqueness;
        start();
    }
}

void Fader::run() {
    m_keepRunning = true;
    while (m_keepRunning) {
        m_mutex.lock();
        m_level += m_delta;
        m_keepRunning &= (m_delta > 0 && m_level < m_targetLevel) || (m_delta < 0 && m_level > m_targetLevel);
        m_mutex.unlock();

        // qDebug() << delta << level << targetLevel << keepRunning;

        if (m_keepRunning) {
            emit fadeLevel(m_level);
            msleep(m_delay);
        }
    }

    // qDebug() << delta << targetLevel << targetLevel;
    // qDebug() << "";

    emit fadeLevel(m_targetLevel);
}

} // namespace launchy

