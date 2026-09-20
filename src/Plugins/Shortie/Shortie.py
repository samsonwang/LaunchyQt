# Simulates keyboard events to launchy shortcuts which are complex to be done by hand
# Helps to access quickly to Microsoft Power Toys Functions, Maps a String to a sequence of key combinations
#

from launchy import Plugin, CatItem
import sys, os
import launchy
import json
import itertools
import webbrowser
import re
try:
    from PySide2 import QtCore, QtGui, QtWidgets
    from PySide2.QtCore import QLocale
    from PySide2.QtWidgets import QWidget, QApplication
    from shiboken2 import wrapInstance, getCppPointer
except Exception as ex:
    try:
        from PySide6.QtWidgets import QWidget, QApplication
    except Exception as ex:
        print(f"CalcyPy, An error occurred: {ex}")


class Shortie(Plugin):

    def __init__(self):
        launchy.Plugin.__init__(self)
        self.loaded = False
        self.key_combinations = {}
        self.key_aliases = {}

    # Auxiliary Function
    def list_entries(self, dictionary, list_to_populate):
        for children in dictionary:
            name = children.get('name')
            key = children.get('keys')
            nestedChild = children.get('children')
            if not (nestedChild is None):
                self.list_entries(nestedChild, list_to_populate)
            elif not(name is None):
                list_to_populate[name] = key

    def load_data(self):
        if(self.loaded == True):
            return False
        
        self.loaded = True
        self.key_combinations = {}
        self.key_aliases = {}
        
        with open(os.path.join(self.path, "data.json"), 'r', encoding="utf-8")as bFile:
            bookmarksDictionary = json.load(bFile)
            self.list_entries(bookmarksDictionary['roots']['shortcuts']['children'], self.key_combinations )
            self.list_entries(bookmarksDictionary['roots']['aliases']['children'], self.key_aliases )
            bFile.close()

        return True

    def reload_data(self):
        self.loaded = False
        self.load_data()
        return True
    
    def execute_shortkey(self,key_sequence):
        keys = key_sequence.split(",")
        return True

    def init(self):
        pass

    def getName(self):
        return "Shortie"

    def setPath(self, path):
        self.path = path

    def getIcon(self):
        return self.path + "/keyboard.png"

    def getLabels(self, inputDataList):
        inputDataList[-1].setLabel(self.getName())

    def getResults(self, inputDataList, resultsList):
        # Load once data - see loadData function...
        self.load_data()

        if not inputDataList[-1].hasLabel(self.getName()):
            return

        inputText = inputDataList[0].getText()

        # Look for Hard coded Commands
        if inputText and re.search(re.escape(inputText), "Shortie.Rescan", re.IGNORECASE):
              resultsList.push_back( launchy.CatItem("Shortie.Rescan", "Shortie.Rescan", self.getName(), self.getIcon()) )

        # Look for Shortcuts to launch
        for command in self.key_combinations:
            if re.search(inputText, command, re.IGNORECASE):
                resultsList.append(CatItem( "Key Sequence: " + self.key_combinations[command], command, self.getName(), self.getIcon()) )

        # Look for Aliases to be copied
        for alias in self.key_aliases:
            if re.search(inputText, alias, re.IGNORECASE):
                resultsList.append(CatItem( "Key Alias: " + str( self.key_aliases[alias]), alias, self.getName(), self.getIcon()) )
        

    def getCatalog(self, resultsList):
        pass

    def launchItem(self, inputDataList, catItem):
        catItem = inputDataList[-1].getTopResult()
        finalUrl = catItem.fullPath()
        finalName = catItem.shortName()

        if (finalUrl == "Shortie.Rescan"):
            return self.reload_data() 
        elif finalUrl.startswith("Key Sequence:"):
            os.system("start cmd /c .\plugins\Shortie\shor-launch.bat \"" + str(self.key_combinations[finalName]) + "\" " + str(finalName))
        elif finalUrl.startswith("Key Alias:"):
            QApplication.clipboard().setText(str(self.key_aliases[finalName]))
            
        return True

    def launchyShow(self):
        pass

    def launchyHide(self):
        pass

    def doDialog(self, parent):
        pass

    def endDialog(self, accept):
        pass

def getPlugin():
    return Shortie
