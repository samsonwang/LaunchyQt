
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
        # change it to your own plugin name
        return "DemoPy"

    def setPath(self, path):
        self.path = path

    def getIcon(self):
        # change to your own plugin icon
        return self.path + "/demopy.png"

    def getLabels(self, inputDataList):
        # you can get inputData here,
        # if your plugin wants to deal with this inputData,
        # set last inputData label to your plugin hash
        condition = True
        if condition == True:
            inputDataList[-1].setLabel(self.hash)


    def getResults(self, inputDataList, resultsList):
        # add items that shows in suggestion list
        if not inputDataList[-1].hasLabel(self.hash):
            return
        resultsList.append(CatItem("demopy.demopy",
                                   "demopy item",
                                   self.getID(),
                                   self.getIcon()))

    def getCatalog(self, resultsList):
        # add your own catalog
        pass

    def launchItem(self, inputDataList, catItem):
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
