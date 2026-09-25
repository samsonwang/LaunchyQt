# Helps to quick and simple operations with text
# Camel  <hit tab> Test Message - will output "TestMessage" (when hitting enter, the text will be copied)
# Lower  <hit tab> Test Message - will output "test message"
# Upper  <hit tab> Test Message - will output "TEST MESSAGE"
# Title  <hit tab> test message - will output "Test Message"
# Strip  <hit tab>   test  mess - will output "test mess" (additional spaces are removed)
# Join   <hit tab> Test\nMessage - will output "Test,Message"
# Split  <hit tab> Test,Message - will output "Test\nMessage"
# Compat <hit tab> Test\tMessage - will output "TestMessage"

import sys, os
import re
import launchy
from datetime import date
from datetime import datetime
from launchy import Plugin, CatItem
try:
    from PySide2 import QtCore, QtGui, QtWidgets
    from PySide2.QtCore import QLocale
    from PySide2.QtWidgets import QWidget, QApplication
except Exception as ex:
    try:
        from PySide6.QtWidgets import QWidget, QApplication
    except Exception as ex:
        print(f"CalcyPy, An error occurred: {ex}")



class Camel(Plugin):

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
        inputDataList[-1].setLabel(self.getName())

    def getResults(self, inputDataList, resultsList):

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
                elif re.search(inputText, "Join", re.IGNORECASE):
                    isTemplate = True
                    finalName = "Join"
                elif re.search(inputText, "Split", re.IGNORECASE):
                    isTemplate= True
                    finalName = "Split"
                elif re.search(inputText, "Compat", re.IGNORECASE):
                    isTemplate= True
                    finalName = "Compat"
                elif re.search(inputText, "Flip", re.IGNORECASE):
                    isTemplate= True
                    finalName = "Flip"

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
                elif finalName == "Join":
                    finalUrl = inputText.replace("\n",",")
                elif finalName == "Split":
                    finalUrl = inputText.replace(",","\n")
                elif finalName == "Compat":
                    finalUrl = inputText.replace("\t","")
                elif finalName == "Flip":
                    finalUrl = inputText.replace("/","\\")
                    if finalUrl == inputText:
                        finalUrl = inputText.replace("\\","/")

            itemIndex = itemIndex + 1

        if isTemplate == True:
            resultsList.push_back( launchy.CatItem(finalUrl, finalName, self.getName(), self.getIcon()))

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
