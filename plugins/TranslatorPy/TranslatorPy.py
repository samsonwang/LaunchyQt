# Copyright (c) 2019 Samson Wang
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

from translator import *

import launchy
from launchy import CatItem

#from Calculator import Calculator
#from Calcy import CalcyGui

class TranslatorPy(launchy.Plugin):
    setting_dir = 'TranslatorPy/'
    settings = dict()

    def __init__(self):
        launchy.Plugin.__init__(self)
        self.hash = launchy.hash(self.getName())

    def init(self):
        self.__readSettings()

    def getID(self):
        return int(self.hash)

    def getName(self):
        return "TranslatorPy"

    def setPath(self, path):
        self.path = path

    def getIcon(self):
        return self.path + "/translatorpy.png"

    def getCatalog(self, resultsList):
        resultsList.push_back( launchy.CatItem("translatorpy",
                                               "tr",
                                               self.getID(),
                                               self.getIcon() ) )

    def getLabels(self, inputDataList):
        pass

    def getResults(self, inputDataList, resultsList):
        if len(inputDataList) <= 1 or inputDataList[0].getText() != 'tr':
            return

        text = inputDataList[1].getText()
        print('TranslatorPy, getResults, input text:', text)
        if len(text) == 0:
            print('TranslatorPy, getResults, input text is empty')
            return

        # press tab to query
        if len(inputDataList) < 3:
            item = CatItem('press tab to translate' , text,
                           self.getID(), self.getIcon())
            item.setUsage(50000)
            resultsList.append(item)
            return

        gt = YoudaoTranslator()
        print('TranslatorPy, getResults, translating ...')
        ret = gt.translate('auto', 'auto', text)
        if not ret:
            print('TranslatorPy, getResults, fail to translate')
            return

        print('TranslatorPy, getResults, ret:', ret)
        item = CatItem(text+".translatorpy", ret['result'],
                       self.getID(), self.getIcon())
        item.setUsage(50000)
        resultsList.append(item)

    def doDialog(self, parentWidgetPtr):
        print('Translatorpy, doDialog')

    def endDialog(self, accept):
        print('Translatorpy, endDialog')

    def launchItem(self, inputDataList, catItem):
        pass
#        if self.settings['copyToClipboard']:
#        QApplication.clipboard().setText(catItem.fullPath())

    def __readSettings(self):
        print('TranslatorPy, __readSettings', self.settings)
        pass
        # general
#        self.settings['bitwidth'] = int(launchy.settings.value(self.setting_dir + 'bitwidth', 16))

def getPlugin():
    return TranslatorPy
