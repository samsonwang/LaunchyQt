# 0.2 Porting to Launchy 3.1.8
# 0.1 Allows to count from one time to another and provide result formatted for JIRA ;)

import sys, os
import launchy
import re
from datetime import datetime

from launchy import CatItem
try:
    from PySide2 import QtCore, QtGui, QtWidgets
    from PySide2.QtCore import QLocale
    from PySide2.QtWidgets import QWidget, QApplication
except Exception as ex:
    try:
        from PySide6.QtWidgets import QWidget, QApplication
    except Exception as ex:
        print(f"CalcyPy, An error occurred: {ex}")



class Chrono(launchy.Plugin):

    def __init__(self):
        launchy.Plugin.__init__(self)
        self.startTime = 0

    def init(self):
        pass

    def getName(self):
        return "Chrono"

    def setPath(self, path):
        self.path = path

    def getIcon(self):
        return self.path + "/chrono.png"

    def getLabels(self, inputDataList):
        inputDataList[-1].setLabel(self.getName())

    def getResults(self, inputDataList, resultsList):
        finalUrl = " "
        finalName = " "
        isTemplate = False
        
        for item in inputDataList:
            inputText = item.getText()
            if re.search(inputText, "Start", re.IGNORECASE):
                isTemplate = True
                finalName = "Start"
                finalUrl = "Start"
            elif re.search(inputText, "Stop", re.IGNORECASE):
                isTemplate = True
                finalName = "Stop"
                finalUrl = self.getElapsedTime()

        if isTemplate == True:
            resultsList.push_back( launchy.CatItem(finalUrl, finalName, self.getName(), self.getIcon()))

    def getCatalog(self, resultsList):
        pass

    def getStartTime(self):
        return(self.startTime)
    
    def setStartTime(self):
        self.startTime = datetime.now()
    
    def stopStartTime(self):
        self.startTime = 0

    def getElapsedTime(self):
        if self.getStartTime() != 0: 
            diff = datetime.now() - self.getStartTime()
            days, seconds = diff.days, diff.seconds
            hours = days * 24 + seconds // 3600
            minutes = (seconds % 3600) // 60
            seconds = seconds % 60
            result = []
            if days != 0:
                result.append("{0}d".format(days))
            if hours != 0:
                result.append("{0}h".format(hours))
            if minutes != 0:
                result.append("{0}m".format(minutes))
            if seconds != 0:
                result.append("{0}s".format(seconds))
                
            return " ".join(result)
        else:
            return "..."

    def launchItem(self, inputDataList, catItem):
        # Check command and count elapsed time
        catItem = inputDataList[-1].getTopResult()
        command = catItem.fullPath()
        elapsedTime = "0"
        
        if command == "Start":
            self.setStartTime()
        else:
            elapsedTime = self.getElapsedTime()
            self.stopStartTime()
        
        QApplication.clipboard().setText(elapsedTime)

    def launchyShow(self):
        pass

    def launchyHide(self):
        pass

    def doDialog(self, parent):
        pass

    def endDialog(self, accept):
        pass

def getPlugin():
    return Chrono
