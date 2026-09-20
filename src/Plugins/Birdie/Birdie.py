from launchy import Plugin, CatItem
import sys, os
import launchy
import json
import itertools
import webbrowser
import re

# Auxiliary Function to list Favorites from Edge Bookmark File.
def listFolders(dictionary, folders, scripts, paths):
    for children in dictionary:
        url = children.get('url')
        nestedChild = children.get('children')
        if not (nestedChild is None):
            listFolders(nestedChild, folders, scripts, paths)
        elif not(url is None):
            # Get the Folder and append it
            folders.append(url)

            for entry in os.listdir(str(url)):
                file = os.path.join(str(url), entry)
                if os.path.isfile(file) and str(entry).endswith(".py"):
                    scripts.append(str(entry))
                    paths.append(str(url))

class Birdie(Plugin):

    def __init__(self):
        launchy.Plugin.__init__(self)
        self.loaded = False
        self.Folders  = []
        self.Scripts = []
        self.Paths = []

    def loadData(self):
        if(self.loaded == True):
            return False
        
        self.loaded = True

        with open(os.path.join(self.path, "data.json"), 'r')as bFile:
            bookmarksDictionary = json.load(bFile)
            listFolders(bookmarksDictionary['roots']['folders']['children'], self.Folders, self.Scripts, self.Paths)
            bFile.close()

        return True

    def reloadData(self):
        self.loaded = False
        self.Folders  = []
        self.Scripts = []
        self.Paths = []
        self.loadData()
        return True

    def init(self):
        pass

    def getName(self):
        return "Birdie"

    def setPath(self, path):
        self.path = path

    def getIcon(self):
        return self.path + "/birdie.png"

    def getScripts(self):
        return self.Scripts

    def getPaths(self):
        return self.Paths

    def getLabels(self, inputDataList):
        inputDataList[-1].setLabel(self.getName())

    def getResults(self, inputDataList, resultsList):
        # Load once data - see loadData function...
        self.loadData()

        if not inputDataList[-1].hasLabel(self.getName()):
            return

        inputText = inputDataList[0].getText()

        # Look for Hard coded Commands
        if inputText and re.search(re.escape(inputText), "Birdie.Rescan", re.IGNORECASE):
              resultsList.push_back( launchy.CatItem("Birdie.Rescan", "Birdie.Rescan", self.getName(), self.getIcon()) )

        # Look for Scripts to launch
        for(name,url) in zip(self.getScripts(),self.getPaths()):
            if(re.search(re.escape(inputText), name, re.IGNORECASE)):
                resultsList.push_back( launchy.CatItem(url, name, self.getName(), self.getIcon()))

    def getCatalog(self, resultsList):
        pass

    def launchItem(self, inputDataList, catItem):
        catItem = inputDataList[-1].getTopResult()
        finalUrl = catItem.fullPath()
        finalName = catItem.shortName()

        if (finalUrl == "Birdie.Rescan"):
            return self.reloadData()

        os.system("start cmd /k .\plugins\Birdie\cip-launch.bat \"" + str(finalUrl) + "\" " + str(finalName))

    def launchyShow(self):
        pass

    def launchyHide(self):
        pass

    def doDialog(self, parent):
        pass

    def endDialog(self, accept):
        pass

def getPlugin():
    return Birdie
