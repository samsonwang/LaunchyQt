/*
Launchy: Application Launcher
Copyright (C) 2009  Simon Capewell

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


#ifndef FILESEARCH_H
#define FILESEARCH_H

#include "InputDataList.h"
#include "LaunchyLib/CatalogItem.h"

namespace launchy {
class FileSearch {
public:
	static void search(const QString& searchText,
                       QList<CatItem>& searchResults,
                       InputDataList& inputData);

	// True when the text looks like a file system path, so that the file
	// system should be searched as well as the catalog. Kept next to search()
	// so that this test never drifts away from the way search() splits paths.
	static bool looksLikePath(const QString& searchText);
};
} // namespace launchy

#endif
