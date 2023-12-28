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

# !! README : This Version needs manual configuration at line 67 : a path needs to be updated

import sys, os
import launchy
import json
import itertools
import webbrowser
import re

# Auxiliary Function to list Favorites from Edge Bookmark File.
def listFavorites(dictionary, names, urls, defs):
    for children in dictionary:
        name = children.get('name')
        url = children.get('url')
        default = children.get('default')
        nestedChild = children.get('children')
        if not (nestedChild is None):
            listFavorites(nestedChild, names, urls, defs)
        elif not( name is None or url is None):
            names.append(name)
            urls.append(url)
            if( default is None ):
                defs.append("none")
            else:
                defs.append(default)


class EdgePy(launchy.Plugin):
    def __init__(self):
        launchy.Plugin.__init__(self)
        self.hash = launchy.hash(self.getName())
        self.loaded = False
        # Favorites Names and Favorites Urls
        self.fNames  = []
        self.fUrls   = []
        self.fDefs   = []
        # Template Names and Template Urls
        self.tNames = []
        self.tUrls = []
        self.tDefs = []
        # Protocols
        self.pNames = []
        self.pUrls = []
        self.pDefs = []
    
    def loadData(self):
        if(self.loaded == True):
            return False
        
        self.loaded = True
        
        with open('C:\\Users\\chris\\AppData\\Local\\Microsoft\\Edge\\User Data\\Default\\Bookmarks', 'r')as bFile:
            bookmarksDictionary = json.load(bFile)
            listFavorites(bookmarksDictionary['roots']['synced']['children'], self.fNames, self.fUrls, self.fDefs)            
            listFavorites(bookmarksDictionary['roots']['other']['children'], self.fNames, self.fUrls, self.fDefs)
            listFavorites(bookmarksDictionary['roots']['bookmark_bar']['children'], self.fNames, self.fUrls, self.fDefs)       
            bFile.close()
            
        with open(os.path.join(self.path, "data.json"), 'r')as bFile:
            bookmarksDictionary = json.load(bFile)
            listFavorites(bookmarksDictionary['roots']['home']['children'], self.tNames, self.tUrls, self.tDefs)
            listFavorites(bookmarksDictionary['roots']['office']['children'], self.tNames, self.tUrls, self.tDefs)
            listFavorites(bookmarksDictionary['roots']['protocols']['children'], self.pNames, self.pUrls, self.pDefs)
            bFile.close()

        return True
        
    def init(self):
        pass

    def getID(self):
        return int(self.hash)

    def getName(self):
        return "Edge"

    def setPath(self, path):
        self.path = path

    def getIcon(self):
        return self.path + "/edge.png"

    def getFNames(self):
        return self.fNames

    def getFUrls(self):
        return self.fUrls

    def getFDefs(self):
        return self.fDefs

    def getTNames(self):
        return self.tNames

    def getTUrls(self):
        return self.tUrls

    def getTDefs(self):
        return self.tDefs
    
    def getPNames(self):
        return self.pNames

    def getPUrls(self):
        return self.pUrls

    def getPDefs(self):
        return self.pDefs
    
    def getLabels(self, inputDataList):
        condition = True
        if condition == True:
            inputDataList[-1].setLabel(self.hash)

    def getResults(self, inputDataList, resultsList):
        if not inputDataList[-1].hasLabel(self.hash):
            return

        # Load once data - see loadData function...
        self.loadData()

        # Look for Edge Favorites
        inputText = inputDataList[0].getText()
        for(name,url) in zip(self.getFNames(),self.getFUrls()):
            if inputText and re.search(re.escape(inputText), name, re.IGNORECASE):
                resultsList.push_back( launchy.CatItem(url, name, self.getID(), self.getIcon()))

        # Look for Url Templates
        itemIndex = 0
        finalUrl = " "
        finalName = " "
        isTemplate = False
        
        for item in inputDataList:
            inputText = item.getText()
            if itemIndex == 0:
                # the first entry is the command, thus retrieve url template and command name
                for(name,url) in zip(self.getTNames(),self.getTUrls()):
                    if re.search(re.escape(inputText), name, re.IGNORECASE):
                        finalUrl = url
                        finalName = name
                        isTemplate = True
            else:
                # other entries are parameters, replace in the url
                if inputText:
                    placeholder = "%" + str(itemIndex)
                    finalUrl = finalUrl.replace(placeholder, inputText)
                    # the parameter is replaced by the command as the user will hit tab...
                    # thus, keep name equal to the current last input parameter
                    finalName = inputText

            itemIndex = itemIndex + 1

        if isTemplate == True:
            resultsList.push_back( launchy.CatItem(finalUrl, finalName, self.getID(), self.getIcon()))
        
        # Look for Protocols
        inputText = inputDataList[0].getText()
        for(name,url) in zip(self.getPNames(),self.getPUrls()):
            if inputText.startswith(url):
                resultsList.push_back( launchy.CatItem(inputText, name, self.getID(), self.getIcon()))

    def getCatalog(self, resultsList):
        pass

    def launchItem(self, inputDataList, catItem):
        catItem = inputDataList[-1].getTopResult()

        # In case this is a Template but not filled, replace with the default url
        finalUrl = catItem.fullPath()
        for(url,default) in zip(self.getTUrls(),self.getTDefs()):
            if re.search(re.escape(catItem.fullPath()), url, re.IGNORECASE):
                finalUrl = default

        webbrowser.open(finalUrl)
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
    return EdgePy     



