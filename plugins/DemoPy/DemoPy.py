
import sys, os
import launchy
from launchy import CatItem

class DemoPy(launchy.Plugin):
    def __init__(self):
        launchy.Plugin.__init__(self)
        self.hash = launchy.hash(self.getName())

    def init(self):
        pass

    def getID(self):
        return int(self.hash)

    def getName(self):
        return "DemoPy"

    def setPath(self, path):
        self.path = path

    def getIcon(self):
        return self.path + "/demopy.png"

    def getLabels(self, inputDataList):
        inputDataList[-1].setLabel(self.hash)
        pass

    def getResults(self, inputDataList, resultsList):
        if not inputDataList[-1].hasLabel(self.hash):
            return
        resultsList.append(CatItem("demopy.demopy",
                                   "demopy item",
                                   self.getID(),
                                   self.getIcon()))

    def getCatalog(self, resultsList):
        pass

    def launchItem(self, inputDataList, catItemOrig):
        pass

    def launchyShow(self):
        pass

    def launchyHide(self):
        pass

    def doDialog(self, parent):
        pass

    def endDialog(self, accept):
        pass

def getPlugin():
    return DemoPy
