# Copyright (c) 2019 Samson Wang
# Copyright (c) 2008 Shahar Kosti
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#import sys, os
#import math
#import re

from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtCore import QVariant
from PyQt5.QtWidgets import QWidget
from sip import wrapinstance, unwrapinstance

import launchy
from launchy import CatItem

from Calculator import Calculator
from Calcy import CalcyGui

class CalcyPy(launchy.Plugin):
    setting_dir = 'Calcy/'
    settings = dict()

    def __init__(self):
        launchy.Plugin.__init__(self)
        self.hash = launchy.hash(self.getName())

    def init(self):
        self.__readSettings()

    def getID(self):
        return int(self.hash)

    def getName(self):
        return "CalcyPy"

    def setPath(self, path):
        self.path = path

    def getIcon(self):
        return self.path + "/calcpy.ico"

    def getLabels(self, inputDataList):
        pass

    def getResults(self, inputDataList, resultsList):
        if len(inputDataList) > 1:
            return

        text = inputDataList[0].getText()
        if not Calculator.isValidExpression(text):
            return

        try:
            ret = Calculator.calc(text, advanced=True)
        except:
            ret = None

        if isinstance(ret, int):
            retInFloat = None

            # hex
            retInHex = '0x%x' % ret

            # dec
            retInDec = ret

            # octal
            retInOct = '0%o' % ret

            # bin
            retb = format(ret, '032b')
            retInBin = ""
            for i in range(len(retb)):
                retInBin += retb[i] if (i % 4 or i == 0) else ("," + retb[i])

            # size
            retInSize = ""

            size_giga_bytes = int(ret / (1024 ** 3)) if int(ret / (1024 ** 3)) else 0
            ret -= size_giga_bytes * (1024 ** 3)
            retInSize += "%s GB " % size_giga_bytes if size_giga_bytes else ""

            size_mega_bytes = int(ret / (1024 * 1024)) if int(ret / (1024 * 1024)) else 0
            ret -= size_mega_bytes * (1024 ** 2)
            retInSize += "%s MB " % size_mega_bytes if size_mega_bytes else ""

            size_kilo_bytes = int(ret / (1024)) if int(ret / (1024)) else 0
            ret -= size_mega_bytes * (1024 ** 1)
            retInSize += "%s KB " % size_kilo_bytes if size_kilo_bytes else ""

            size_bytes = int(ret % (1024)) if int(ret % (1024)) else 0
            retInSize += "%s B " % size_bytes if size_bytes else ""

            if not retInSize:
                retInSize = "0 B"

        else:
             retInFloat = ret
             retInHex = None
             retInDec = None
             retInOct = None
             retInBin = None
             retInSize = None

        if retInFloat is not None:
            resultsList.append(CatItem("float.calcypy",
                                       "%s" % (retInFloat),
                                       self.getID(), self.getIcon()))

        if retInDec is not None:
            resultsList.append(CatItem("dec.calcpy",
                                       str(retInDec),
                                       self.getID(), self.getIcon()))

        if retInOct is not None:
            resultsList.append(CatItem("oct.calcpy",
                                       str(retInOct),
                                       self.getID(), self.getIcon()))

        if retInHex is not None:
            resultsList.append(CatItem("hex.calcpy",
                                       str(retInHex),
                                       self.getID(), self.getIcon()))

        if retInBin is not None:
            resultsList.append(CatItem("bin.calcpy",
                                       str(retInBin),
                                       self.getID(), self.getIcon()))

        if retInSize is not None:
            resultsList.append(CatItem("size.calcpy",
                                       "%s" % (retInSize),
                                       self.getID(), self.getIcon()))

    def doDialog(self, parentWidgetPtr):
#        print(parentWidgetPtr)
        print('CalcyPy, doDialog')
        parentWidget = wrapinstance(parentWidgetPtr, QtWidgets.QWidget)
        self.widget = CalcyGui.CalcyOption(parentWidget, self.setting_dir, self.settings)
        self.widget.show()
        return unwrapinstance(self.widget)

    def endDialog(self, accept):
        print('CalcyPy, endDialog')
        self.widget.hide()
        if accept:
            self.widget.writeSettings()
            self.__readSettings()
        del self.widget
        self.widget = None

    def launchItem(self, inputDataList, catItem):
        pass

    def __readSettings(self):
        print('CalcyPy, __readSettings')
        launchySettings = launchy.settings
        # general
        self.settings['decimalPointGroupSeparator'] = int(launchySettings.value(self.setting_dir + 'decimalPointGroupSeparator', 0))
        self.settings['outputPrecision'] = int(launchySettings.value(self.setting_dir + 'outputPrecision', 3))
        self.settings['showGroupSeparator'] = bool(launchySettings.value(self.setting_dir + 'showGroupSeparator', False))
        self.settings['copyToClipboard'] = bool(launchySettings.value(self.setting_dir + 'copyToClipboard', True))
        self.settings['showBinOut'] = bool(launchySettings.value(self.setting_dir + 'showBinOut', True))
        self.settings['showOctOut'] = bool(launchySettings.value(self.setting_dir + 'showOctOut', True))
        self.settings['showDecOut'] = bool(launchySettings.value(self.setting_dir + 'showDecOut', True))
        self.settings['showHexOut'] = bool(launchySettings.value(self.setting_dir + 'showHexOut', True))
        self.settings['showSizeOut'] = bool(launchySettings.value(self.setting_dir + 'showSizeOut', True))
        self.settings['showBasePrefix'] = bool(launchySettings.value(self.setting_dir + 'showBasePrefix', True))
        self.settings['showZeroBin'] = bool(launchySettings.value(self.setting_dir + 'showZeroBin', True))
        self.settings['showZeroOct'] = bool(launchySettings.value(self.setting_dir + 'showZeroOct', True))
        self.settings['showZeroHex'] = bool(launchySettings.value(self.setting_dir + 'showZeroHex', True))
        self.settings['bitwidth'] = int(launchySettings.value(self.setting_dir + 'bitwidth', 16))

        print('CalcyPy, read settings', self.settings)

def getPlugin():
    return CalcyPy
