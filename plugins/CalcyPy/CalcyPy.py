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
from PyQt5.QtCore import QLocale
from PyQt5.QtWidgets import QWidget
from PyQt5.QtWidgets import QApplication
from sip import wrapinstance, unwrapinstance

import launchy
from launchy import CatItem

from Calculator import Calculator
from Calcy import CalcyGui

class CalcyPy(launchy.Plugin):
    setting_dir = 'CalcyPy/'
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
        text = self.__prepareExpression(text)
        if not Calculator.isValidExpression(text):
            return

        try:
            ret = Calculator.calc(text, advanced=True)
        except:
            return

        print('CalcyPy, getResults,', text, ret)

        # full divide
        if isinstance(ret, float) and ret - int(ret) < 1e-15:
            ret = int(ret)

        if isinstance(ret, int):
            retInFloat = None

            # hexadecimal
            retInHex = self.__formatHexadecimal(ret)

            # decimal
            retInDec = self.__formatDecimal(ret)

            # octal
            retInOct = self.__formatOctal(ret)

            # binary
            retInBin = self.__formatBinary(ret)

            # size
            retInSize = self.__formatSize(ret)

        else:
            retInFloat = ("%%.%df" % self.settings['outputPrecision']) % ret
            retInHex = None
            retInDec = None
            retInOct = None
            retInBin = None
            retInSize = None

        if retInFloat is not None:
            item = CatItem("float.calcypy", retInFloat,
                           self.getID(), self.getIcon())
            item.setUsage(50000)
            resultsList.append(item)

        if retInDec is not None:
            item = CatItem("dec.calcpy", retInDec,
                           self.getID(), self.getIcon())
            item.setUsage(50000)
            resultsList.append(item)

        if retInHex is not None:
            item = CatItem("hex.calcpy", retInHex,
                           self.getID(), self.getIcon())
            item.setUsage(40000)
            resultsList.append(item)

        if retInOct is not None:
            item = CatItem("oct.calcpy", retInOct,
                           self.getID(), self.getIcon())
            item.setUsage(30000)
            resultsList.append(item)

        if retInBin is not None:
            item = CatItem("bin.calcpy", retInBin,
                           self.getID(), self.getIcon())
            item.setUsage(20000)
            resultsList.append(item)

        if retInSize is not None:
            item = CatItem("size.calcpy", retInSize,
                           self.getID(), self.getIcon())
            item.setUsage(10000)
            resultsList.append(item)


    def doDialog(self, parentWidgetPtr):
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
        if self.settings['copyToClipboard']:
            QApplication.clipboard().setText(catItem.shortName())

    def __readSettings(self):
        # general
        self.settings['decimalPointGroupSeparator'] = int(launchy.settings.value(self.setting_dir + 'decimalPointGroupSeparator', 0))
        self.settings['outputPrecision'] = int(launchy.settings.value(self.setting_dir + 'outputPrecision', 3))
        self.settings['showGroupSeparator'] = launchy.settings.value(self.setting_dir + 'showGroupSeparator', False) in ['true', True]
        self.settings['copyToClipboard'] = launchy.settings.value(self.setting_dir + 'copyToClipboard', True) in ['true', True]
        self.settings['showBinOut'] = launchy.settings.value(self.setting_dir + 'showBinOut', True) in ['true', True]
        self.settings['showOctOut'] = launchy.settings.value(self.setting_dir + 'showOctOut', True) in ['true', True]
        self.settings['showHexOut'] = launchy.settings.value(self.setting_dir + 'showHexOut', True) in ['true', True]
        self.settings['showSizeOut'] = launchy.settings.value(self.setting_dir + 'showSizeOut', True) in ['true', True]
        self.settings['showBasePrefix'] = launchy.settings.value(self.setting_dir + 'showBasePrefix', True) in ['true', True]
        self.settings['showZeroBin'] = launchy.settings.value(self.setting_dir + 'showZeroBin', True) in ['true', True]
        self.settings['showZeroOct'] = launchy.settings.value(self.setting_dir + 'showZeroOct', True) in ['true', True]
        self.settings['showZeroHex'] = launchy.settings.value(self.setting_dir + 'showZeroHex', True) in ['true', True]
        self.settings['bitwidth'] = int(launchy.settings.value(self.setting_dir + 'bitwidth', 16))

        print('CalcyPy, __readSettings', self.settings)

    def __decimalPoint(self):
        if self.settings['decimalPointGroupSeparator'] == 1:
            return ','
        elif self.settings['decimalPointGroupSeparator'] == 2:
            return '.'
        else:
            return QLocale.system().decimalPoint()

    def __groupSeparator(self):
        if self.settings['decimalPointGroupSeparator'] == 1:
            return '.'
        elif self.settings['decimalPointGroupSeparator'] == 2:
            return ','
        else:
            return QLocale.system().groupSeparator()

    def __prepareExpression(self, text):
        decimalPoint = self.__decimalPoint()
        groupSeparator = self.__groupSeparator()

        if groupSeparator == None or decimalPoint == None or groupSeparator == decimalPoint:
            return text

        retText = text.replace(groupSeparator, '')
        if decimalPoint != '.':
            retText = retText.replace(decimalPoint, '.')
        return retText

    def __formatHexadecimal(self, num):
        if self.settings['showHexOut']:
            hexFmtStr = ''
            if self.settings['showBasePrefix']:
                hexFmtStr += '0x'
            if self.settings['showZeroHex']:
                width = int(self.settings['bitwidth'] / 8)
                hexFmtStr += ('%%0%dx' % width)
            else:
                hexFmtStr += '%x'
            return hexFmtStr % num
        else:
            return None

    def __formatDecimal(self, num):
        if self.settings['showGroupSeparator']:
            decStr = '{:,}'.format(num)
            if self.__groupSeparator() == QLocale.system().groupSeparator():
                return decStr
            return decStr.replace(QLocale.system().groupSeparator(),
                                  self.__groupSeparator())
        else:
            return '%d' % num

    def __formatOctal(self, num):
        if self.settings['showOctOut']:
            octFmtStr = ''
            if self.settings['showBasePrefix']:
                octFmtStr += '0'
            if self.settings['showZeroOct']:
                width = int(self.settings['bitwidth'] / 3)
                octFmtStr += ('%%0%do' % width)
            else:
                octFmtStr += '%o'
            return octFmtStr % num
        else:
            return None

    def __formatBinary(self, num):
        if self.settings['showBinOut']:
            if self.settings['showZeroBin']:
                retb = format(num, '0%db' % self.settings['bitwidth'])
            else:
                retb = format(num, 'b')

            ret = ''
            for i in range(len(retb)):
                ret += retb[i] if (i % 4 or i == 0) else (" " + retb[i])
            return ret
        else:
            return None

    def __formatSize(self, num):
        if self.settings['showSizeOut']:
            ret = ""

            size_giga_bytes = int(num / (1024 ** 3)) if int(num / (1024 ** 3)) else 0
            num -= size_giga_bytes * (1024 ** 3)
            ret += "%s GB " % size_giga_bytes if size_giga_bytes else ""

            size_mega_bytes = int(num / (1024 * 1024)) if int(num / (1024 * 1024)) else 0
            num -= size_mega_bytes * (1024 ** 2)
            ret += "%s MB " % size_mega_bytes if size_mega_bytes else ""

            size_kilo_bytes = int(num / (1024)) if int(num / 1024) else 0
            num -= size_mega_bytes * (1024 ** 1)
            ret += "%s KB " % size_kilo_bytes if size_kilo_bytes else ""

            size_bytes = int(num % (1024)) if int(num % (1024)) else 0
            ret += "%s B " % size_bytes if size_bytes else ""

            if not ret:
                return  "0 B"
            else:
                return ret
        else:
            return None


def getPlugin():
    return CalcyPy
