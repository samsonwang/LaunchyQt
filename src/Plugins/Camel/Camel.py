# Copyright (c) 2022 Christian Russo
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

# Helps to quick and simple operations with text
# Camel <hit tab> Test Message - will output "TestMessage" (when hitting enter, the text will be copied)
# Lower <hit tab> Test Message - will output "test message"
# Upper <hit tab> Test Message - will output "TEST MESSAGE"
# Title <hit tab> test message - will output "Test Message"
# Strip <hit tab>    test    message - will output "test message" (additional spaces are removed)

import sys, os
import launchy
import re

from launchy import CatItem
from datetime import date
from datetime import datetime
from PySide6.QtWidgets import QApplication

class Camel(launchy.Plugin):
    commandName = {'camel', 'lower', 'upper', 'title', 'strip'}
    def __init__(self):
        launchy.Plugin.__init__(self)

    def init(self):
        pass

    def getName(self):
        return "Camel"

    def setPath(self, path):
        self.path = path

    def getIcon(self):
        return self.path + "/camel.png"

    def getLabels(self, inputDataList): 
        query = inputDataList[0].getText()
        if query.lower() in self.commandName:
            inputDataList[-1].setPlugin(self.getName())
            inputDataList[-1].setUsage(0xffff)

    def getResults(self, inputDataList, resultsList):
        if inputDataList[-1].getPlugin() != self.getName():
            return
        inputText = inputDataList[0].getText()

        itemIndex = 0
        finalUrl = " "
        finalName = " "
        isTemplate = False
        
        for item in inputDataList:
            inputText = item.getText()
            # the first entry is the command
            if itemIndex == 0:
                if re.search(inputText, "Camel", re.IGNORECASE):
                    isTemplate = True
                    finalName = "Camel"
                elif re.search(inputText, "Lower", re.IGNORECASE):
                    isTemplate = True
                    finalName = "Lower"
                elif re.search(inputText, "Upper", re.IGNORECASE):
                    isTemplate = True
                    finalName = "Upper"
                elif re.search(inputText, "Title", re.IGNORECASE):
                    isTemplate = True
                    finalName = "Title"
                elif re.search(inputText, "Strip", re.IGNORECASE):
                    isTemplate = True
                    finalName = "Strip"

            # other entry is the parameter
            elif inputText:
                if finalName == "Camel":
                    finalUrl = inputText.replace('_',' ').title().replace(' ', '')
                elif finalName == "Lower":
                    finalUrl = inputText.lower()
                elif finalName == "Upper":
                    finalUrl = inputText.upper()
                elif finalName == "Title":
                    finalUrl = inputText.title()
                elif finalName == "Strip":
                    finalUrl = re.sub(' +', ' ', inputText.strip())

            itemIndex = itemIndex + 1

        if isTemplate == True:
            resultsList.push_back(launchy.CatItem(finalUrl, finalName, self.getName(), self.getIcon()))

    def getCatalog(self, resultsList):
        pass

    def launchItem(self, inputDataList, catItem):
        # Copy Result in Clipboard
        catItem = inputDataList[-1].getTopResult()
        QApplication.clipboard().setText(catItem.fullPath())

    def launchyShow(self):
        pass

    def launchyHide(self):
        pass

    def doDialog(self, parent):
        pass

    def endDialog(self, accept):
        pass

def getPlugin():
    return Camel
