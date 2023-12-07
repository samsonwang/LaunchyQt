/*
Launchy: Application Launcher
Copyright (C) 2007  Josh Karlin

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

#include <QString>
#include <QStringList>

namespace launchy {

class Directory {
public:
    Directory();
    Directory(const QString& n);
    Directory(const QString& n, const QStringList& t,
              bool d, bool e, int dep);

public:
    QString name;
    QStringList types;
    bool indexDirs;
    bool indexExe;
    int depth;
};

} // namespace launchy
