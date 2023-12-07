
import urllib
import webbrowser

import logging as log

from PySide6 import QtCore, QtGui, QtWidgets
from PySide6.QtWidgets import QWidget, QApplication
from shiboken6 import wrapInstance, getCppPointer

from launchy import Plugin, CatItem
from launchy import settings as lSettings

import WebSearch
from WebSearch.WebSearchGui import WebSearchGui

class WebSearchPy(Plugin):
    settingName = "WebSearchPy"
    searchEngine = {}
    def __init__(self):
        Plugin.__init__(self)

    def init(self):
        self.__readConfig()

    def getName(self):
        return "WebSearchPy"

    def setPath(self, path):
        self.path = path

    def getIcon(self):
        return self.path + "/WebSearch.ico"

    def getLabels(self, inputDataList):
        query = inputDataList[0].getText()
        if query in self.searchEngine.keys():
            inputDataList[-1].setPlugin(self.getName())

    def getResults(self, inputDataList, resultsList):
        if inputDataList[-1].getPlugin() != self.getName():
            return
        query = inputDataList[0].getText()
        keyword = inputDataList[-1].getText()
        if query in self.searchEngine.keys():
            resultsList.push_front(
                CatItem("%s: %s search" % (self.getName(), self.searchEngine.get(query).get("name")),
                        keyword, self.getName(), self.getIcon()))

    def launchItem(self, inputDataList, catItem):
        if len(inputDataList) == 2:
            key = inputDataList[0].getText()
            query = inputDataList[-1].getText()
            url = self.getUrl(key, query)
            log.debug('WebSerachPy::launchyItem, key: %s, query: %s url: %s'
                      % (key, query, url))
            webbrowser.open(url)
        return True

    def doDialog(self, parentWidgetPtr):
        log.debug('WebSearchPy::doDialog ...')
        parentWidget = wrapInstance(parentWidgetPtr, QWidget)
        self.widget = WebSearchGui(parentWidget, self.settingName)
        self.widget.show()
        return getCppPointer(self.widget)[0]

    def endDialog(self, accept):
        log.debug('WebSearchPy::endDialog ...')
        self.widget.hide()
        if accept:
            self.widget.writeSettings()
            self.__readConfig()
        del self.widget
        self.widget = None

    def __readConfig(self):
        settings = lSettings
        self.searchEngine.clear()

        # Read directories from the settings file
        size = settings.beginReadArray(self.settingName)
        if size <= 0:
            log.debug("WebSearchPy::__readConfig, load defaults")
            from WebSearch import Defaults
            self.searchEngine.update(Defaults.defaultSetting)
        else:
            log.debug("WebSearchPy::__readConfig, read from config, count: %s" % size)
            for i in range(0, size):
                settings.setArrayIndex(i)
                key = settings.value("key")
                name = settings.value("name")
                url = settings.value("url")
                self.searchEngine[key] = {"name":name, "url": url}
        settings.endArray()
        log.debug("WebSearchPy::__readConfig: %s" % self.searchEngine)

    @classmethod
    def encodeQuery(cls, query):
        return urllib.parse.quote(query.encode("utf8"))

    @classmethod
    def getUrl(cls, key, query):
        url =  eval('"%s" %% "%s"'
                    % (WebSearchPy.searchEngine.get(key).get('url'),
                       WebSearchPy.encodeQuery(query)))
        log.debug("WebSearchPy::getUrl: %s" % url)
        return url


def getPlugin():
    return WebSearchPy
