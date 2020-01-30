
import urllib
import urllib.parse
#import subprocess
import webbrowser

from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtCore import QVariant
from sip import wrapinstance, unwrapinstance

import launchy
from WebSearch import WebSearchGui

class WebSearchPy(launchy.Plugin):
    settingName = "WebSearchPy"
    searchEngine = {}
    def __init__(self):
        launchy.Plugin.__init__(self)
        self.hash = launchy.hash(self.getName())

    def init(self):
        self.__readConfig()

    def getID(self):
        return int(self.hash)

    def getName(self):
        return "WebSearchPy"

    def setPath(self, path):
        self.path = path

    def getIcon(self):
        return self.path + "/WebSearch.ico"

    def getLabels(self, inputDataList):
        query = inputDataList[0].getText()
        if query in self.searchEngine.keys():
            inputDataList[-1].setLabel(self.hash);

    def getResults(self, inputDataList, resultsList):
        if not inputDataList[-1].hasLabel(self.hash):
            return
        query = inputDataList[0].getText()
        keyword = inputDataList[-1].getText()
        if query in self.searchEngine.keys():
            resultsList.push_front(
                launchy.CatItem("%s: %s search"
                                % (self.getName(),
                                   self.searchEngine.get(query).get("name")),
                                keyword,
                                self.getID(),
                                self.getIcon()))

    def launchItem(self, inputDataList, catItem):
        if len(inputDataList) == 2:
            key = inputDataList[0].getText()
            query = inputDataList[-1].getText()
            url = self.getUrl(key, query)
            print('WebSerach, launchyItem:', key, query, url)
            webbrowser.open(url)
        return True

    def doDialog(self, parentWidgetPtr):
        parentWidget = wrapinstance(parentWidgetPtr, QtWidgets.QWidget)
        self.widget = WebSearchGui.WebSearchUi(parentWidget, self.settingName)
        self.widget.show()
        return unwrapinstance(self.widget);

    def endDialog(self, accept):
        self.widget.hide()
        if accept:
            self.widget.writeSettings()
            self.__readConfig()
        del self.widget
        self.widget = None

    def __readConfig(self):
        settings = launchy.settings
        self.searchEngine.clear()

        # Read directories from the settings file
        size = settings.beginReadArray(self.settingName)
        if size <= 0:
            print("WebSearchPy, __readConfig, load defaults")
            from WebSearch import Defaults
            self.searchEngine.update(Defaults.defaultSetting)
        else:
            print("WebSearchPy, __readConfig, read from config, count:", size)
            for i in range(0, size):
                settings.setArrayIndex(i)
                key = settings.value("key")
                name = settings.value("name")
                url = settings.value("url")
                self.searchEngine[key] = {"name":name, "url": url}
        settings.endArray()
        print("WebSearchPy, __readConfig:", self.searchEngine)

    @classmethod
    def encodeQuery(cls, query):
        return urllib.parse.quote(query.encode("utf8"))

    @classmethod
    def getUrl(cls, key, query):
        url =  eval('"%s" %% "%s"'
                    % (WebSearchPy.searchEngine.get(key).get('url'),
                       WebSearchPy.encodeQuery(query)))
        print("WebSearchPy, getUrl:", url)
        return url


def getPlugin():
    return WebSearchPy
