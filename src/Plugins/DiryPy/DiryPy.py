# Copyright (c) 2008 Shahar Kosti
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

import os
import launchy

from fnmatch import fnmatchcase

from CaselessDict import CaselessDict
from future_ntpath import expandvars

from PySide2 import QtCore, QtGui, QtWidgets
from shiboken2 import wrapInstance, getCppPointer

from PyDiry import PyDiryGui

class DiryPy(launchy.Plugin):
    __version__ = "1.1"
    setting_dir = "DiryPy"

    # Dynamic results must outrank catalog matches that happen to share a few
    # letters with the query. Catalog::CatItemComparePtr weighs usage before
    # word count, which is the only branch these items can reach. Folders get
    # a slightly higher usage so they sort before files.
    file_usage = 50000
    folder_usage = 50001

    # The whole folder is re-listed on every keystroke, so bound the tail.
    # Folders are collected first and therefore survive the truncation.
    max_results = 200

    def __init__(self):
        launchy.Plugin.__init__(self)
        self.dirs = CaselessDict()

    def init(self):
        self.__readConfig()

    def getName(self):
        return "DiryPy"

    def setPath(self, path):
        self.path = path

    def getIcon(self):
        return self.path + "/DiryPy.ico"

    def getLabels(self, inputDataList):
        pass

    def getResults(self, inputDataList, resultsList):
        # The alias and the filter are two input segments: "alias <Tab> filter".
        # A bare alias is left to the catalog entry built in getCatalog().
        if len(inputDataList) != 2:
            return

        firstText = inputDataList[0].getText()
        if not self.dirs.has_key(firstText):
            return

        for itemPath, isFolder in self.__findEntries(self.dirs[firstText],
                                                     inputDataList[1].getText()):
            item = launchy.CatItem(itemPath,
                                   self.__makeShortName(itemPath),
                                   self.getName(),
                                   itemPath)
            item.setUsage(self.folder_usage if isFolder else self.file_usage)
            resultsList.append(item)

    def __findEntries(self, directory, query):
        """Case-insensitive wildcard match inside directory, folders first."""
        patterns = self.__makePatterns(query)
        # Normalise so a configured "C:/dir/" reports the same paths a
        # configured "C:\dir" would.
        directory = os.path.normpath(directory)
        folders = []
        files = []
        try:
            with os.scandir(directory) as entries:
                for entry in entries:
                    name = entry.name.lower()
                    if not all(fnmatchcase(name, pattern) for pattern in patterns):
                        continue
                    try:
                        isFolder = entry.is_dir()
                    except OSError:
                        isFolder = False
                    if isFolder:
                        folders.append(entry.path)
                    else:
                        files.append(entry.path)
        except OSError:
            # Missing or unreadable folder: stay silent, the option dialog is
            # where the path is expected to be fixed.
            return []

        folders.sort(key=str.lower)
        files.sort(key=str.lower)
        found = [(path, True) for path in folders]
        found += [(path, False) for path in files]
        return found[:self.max_results]

    def __makePatterns(self, query):
        """One glob pattern per word, all of which have to match.

        A word that already carries wildcards is used verbatim, anything else is
        wrapped so that "report" also finds "annual-report.txt". An empty query
        yields no pattern, which matches every entry.
        """
        patterns = []
        for word in query.split():
            word = word.lower()
            if any(wildcard in word for wildcard in "*?["):
                patterns.append(word)
            else:
                patterns.append("*%s*" % word)
        return patterns

    def getCatalog(self, resultsList):
        for name,path in self.dirs.items():
            resultsList.push_back( launchy.CatItem( name + ".dirypy",
                                                    name,
                                                    self.getName(),
                                                    self.getIcon() ) )

    def launchItem(self, inputDataList, catItemOrig):
        catItem = inputDataList[-1].getTopResult()
        if catItem.fullPath().endswith(".dirypy"):
            # Launch the directory itself
            try:
                path = self.dirs[catItem.shortName]
                launchy.runProgram(path, "")
            except:
                pass
        else:
            # Launchy a file or directory
            launchy.runProgram(catItem.fullPath(), "" )

    def doDialog(self, parentWidgetPtr):
        parentWidget = wrapInstance(parentWidgetPtr, QtWidgets.QWidget)

        self.widget = PyDiryGui.PyDiryUi(parentWidget, self.setting_dir)
        self.widget.show()

        return getCppPointer(self.widget)[0]

    def endDialog(self, accept):
        self.widget.hide()
        if accept:
            self.widget.writeSettings()
            self.__readConfig()

        del self.widget
        self.widget = None

    def __readConfig(self):
        settings = launchy.settings
        self.dirs.clear()

        # Read directories from the settings file
        size = settings.beginReadArray(self.setting_dir)
        for i in range(0, size):
            settings.setArrayIndex(i);
            name = settings.value("name")
            path = settings.value("path")
            self.dirs[name] = expandvars(path)
        settings.endArray()

    def __makeShortName(self, itemPath):
        try:
            return os.path.split(itemPath)[1]
        except:
            return itemPath

def getPlugin():
    return DiryPy
