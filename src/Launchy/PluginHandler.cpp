/*
Launchy: Application Launcher
Copyright (C) 2007-2009  Josh Karlin

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

#include "PluginHandler.h"

#include <QWidget>

#include "PluginFactory.h"

namespace launchy {

PluginHandler& PluginHandler::instance() {
    static PluginHandler s_obj;
    return s_obj;
}

PluginHandler::PluginHandler()
    : m_factory(new PluginFactory) {
}

void PluginHandler::loadPlugins() {
    PluginRegistry::instance().loadPlugins(*m_factory);
}

QWidget* PluginHandler::doDialog(QWidget* parent, const QString& name) {
    return static_cast<QWidget*>(PluginRegistry::instance().doDialog(parent, name));
}

} // namespace launchy
