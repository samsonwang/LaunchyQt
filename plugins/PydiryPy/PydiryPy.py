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

import sys, os
import glob
import launchy

from CaselessDict import CaselessDict
from future_ntpath import expandvars

from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtCore import QVariant
from sip import wrapinstance, unwrapinstance

from PyDiry import PyDiryGui

class PyDiry(launchy.Plugin):
    __version__ = "1.0"
    setting_dir = "PyDiryPy"

    def __init__(self):
        launchy.Plugin.__init__(self)
        self.hash = launchy.hash(self.getName())
        self.dirs = CaselessDict()

    def init(self):
        self.__readConfig()

    def getID(self):
        return int(self.hash)

    def getName(self):
        return "PyDiryPy"

    def setPath(self, path):
        self.path = path

    def getIcon(self):
        return self.path + "/pydiry.ico"

    def getLabels(self, inputDataList):
        pass

    def getResults(self, inputDataList, resultsList):
        if len(inputDataList) != 2:
            return

        firstText = inputDataList[0].getText()
        if not self.dirs.has_key(firstText):
            return

        path = self.dirs[firstText]
        query = inputDataList[1].getText()

        if query == '':
            # Default query is '*'
            query = '*'
        elif not query.startswith('*') and not query.endswith('*'):
            # Add * at both sides to ease searching
            query = '*%s*' % query

        pathContents = glob.glob(os.path.join(path, query))
        pathContents.sort()

        for itemPath in pathContents:
            resultsList.append( launchy.CatItem(itemPath,
                                                self.__makeShortName(itemPath),
                                                self.getID(),
                                                itemPath) )

    def getCatalog(self, resultsList):
        for name,path in self.dirs.items():
            resultsList.push_back( launchy.CatItem( name + ".pydiry",
                                                    name,
                                                    self.getID(),
                                                    self.getIcon() ) )

    def launchItem(self, inputDataList, catItemOrig):
        catItem = inputDataList[-1].getTopResult()
        if catItem.fullPath().endswith(".pydiry"):
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
        parentWidget = wrapinstance(parentWidgetPtr, QtWidgets.QWidget)

        self.widget = PyDiryGui.PyDiryUi(parentWidget, self.setting_dir)
        self.widget.show()

        return unwrapinstance(self.widget)

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
    return PyDiry
