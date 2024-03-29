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

#pragma once

#include "IconProviderBase.h"

namespace launchy {

class IconProviderWin : public IconProviderBase {
public:
    IconProviderWin();
    virtual ~IconProviderWin();

    virtual QIcon icon(const QFileInfo& info) const;

private:
    QString linkTargetPathTo64(const QFileInfo& info) const;
    bool addIconFromImageList(int imageListIndex, int iconIndex, QIcon& icon) const;
    bool addIconFromShellFactory(const QString& filePath, QIcon& icon) const;
};

} // namespace launchy

