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
#
# Version 1.0:
#   * First public release
#
#import rpdb2; rpdb2.start_embedded_debugger("password")
print("diary")
import launchy
import sys, os

import glob
from CaselessDict import CaselessDict
from future_ntpath import expandvars

from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtCore import QVariant
from sip import wrapinstance, unwrapinstance

from PyDiry import PyDiryGui


class PyDiry(launchy.Plugin):
    __version__ = "1.0"
    #setting_dir = "PyDiry/dirs"
    #setting_dir = "py_directories/dirs"
    setting_dir = "py_speedcommander/dirs"

    def __init__(self):
        launchy.Plugin.__init__(self)

        self.hash = launchy.hash(self.getName())
        self.labelHash = launchy.hash("pydiry")

        self.dirs = CaselessDict()
        #self.icon = self.getIcon()

    def init(self):
        self.__readConfig()
        pass

    def getID(self):
        return self.hash

    def getName(self):
        return "PyDiry"

    def setPath(self, path):
        self.path = path

    def getIcon(self):
        return self.path + "/pydiry.ico"

    def getLabels(self, inputDataList):
        pass

    def getResults(self, inputDataList, resultsList):
        inputs = len(inputDataList)

        if inputs != 2:
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
            resultsList.append( launchy.CatItem(itemPath, self.__makeShortName(itemPath), self.getID(), itemPath ) )

    def getCatalog(self, resultsList):
        for name,path in self.dirs.items():
            resultsList.push_back( launchy.CatItem( name + ".pydiry", name, self.getID(), self.getIcon() ) )

    def launchItem(self, inputDataList, catItemOrig):
        catItem = inputDataList[-1].getTopResult()
        if catItem.fullPath().endswith(".pydiry"):
            # Launch the directory itself
            try:
                path = self.dirs[catItem.shortName]
                launchy.runProgram(path, "")
                #import os,win32api
                #win32api.ShellExecute(0, '', os.path.join(working,"programs/nsis/~~~/makensisw.exe"), path, '', 1)# 前台打开
            except:
                pass
        else:
            # Launchy a file or directory
            launchy.runProgram(catItem.fullPath(), "" )

    def hasDialog(self):
        return True

    def doDialog(self, parentWidgetPtr):
        print(parentWidgetPtr)
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

        # Test if the settings file has PyDiry configuration
#        version = settings.value("PyDiry/version", QVariant("0.0")).toString()
#        print_debug("PyDiry version: " + version)
#        if version == "0.0":
#            settings.beginWriteArray(self.setting_dir)
#            settings.setArrayIndex(0)
#            settings.setValue("name", QVariant("My Documents"))
#            settings.setValue("path", QVariant("%USERPROFILE%\\My Documents"))
#            settings.endArray()
#
#        # Set our version
#        settings.setValue("PyDiry/version", QVariant(self.__version__))

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

    def launchyShow(self):
        pass

    def launchyHide(self):
        pass

def getPlugin():
    return PyDiry
