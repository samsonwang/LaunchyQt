
import urllib.parse
import webbrowser

import logging as log

from PySide2 import QtCore, QtGui, QtWidgets
from PySide2.QtWidgets import QWidget, QApplication
from shiboken2 import wrapInstance, getCppPointer

from launchy import Plugin, CatItem
from launchy import settings as lSettings

import WebSearch
from WebSearch.WebSearchGui import WebSearchGui
from WebSearch.FaviconCache import cache as faviconCache, toBool
from WebSearch.LaunchyProxy import proxy as launchyProxy


class WebSearchPy(Plugin):
    settingName = "WebSearchPy"
    # The engines and the "fetch site icons" switch are stored in one group, so
    # writeSettings() must not drop the whole group to rewrite the array; it
    # only removes the entries left behind by a shorter list.
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
        return self.path + "/WebSearch.png"

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
            engine = self.searchEngine.get(query)
            # The site icon when it has been downloaded already, the plugin icon
            # until then. iconPath() only looks the cache up, it never blocks.
            icon = faviconCache().iconPath(engine.get("url"))
            resultsList.push_front(
                CatItem("%s: %s search" % (self.getName(), engine.get("name")),
                        keyword, self.getName(), icon if icon else self.getIcon()))

    def launchItem(self, inputDataList, catItem):
        if len(inputDataList) == 2:
            key = inputDataList[0].getText()
            query = inputDataList[-1].getText()
            url = self.getUrl(key, query)
            log.debug('WebSerachPy::launchyItem, key: %s, query: %s url: %s'
                      % (key, query, url))
            if url:
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
            # The engines or the switch just changed, a new engine has to get
            # its icon now rather than after the throttle window.
            self.__readConfig(True)
        else:
            # The dialog may have flipped the switch while it was open, so the
            # cache is put back in the state the settings ask for.
            self.__refreshFavicons(True)
        del self.widget
        self.widget = None

    # The network may have been down while the cache was warmed at startup, so
    # the icons get another chance whenever launchy is called up. Hosts whose
    # icon is still young, or that failed in the last days, are skipped by the
    # cache itself, so this costs nothing once everything is downloaded -- it
    # still is a settings read and two stat calls per engine on the gui thread
    # for every invocation, and the icons are day-level cached. The cache
    # throttles the attempt itself (REFRESH_INTERVAL, on a stamp its state file
    # persists, so a restart inside the window skips it as well); asking
    # dueForRefresh() first is the cheap side of that throttle and keeps a
    # hotkey inside the window free of even the settings read. Only the option
    # dialog forces a refresh, a new engine has to get its icon now.
    def launchyShow(self):
        if faviconCache().dueForRefresh():
            self.__refreshFavicons()

    def __readConfig(self, forceRefresh=False):
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
        # The proxy of launchy's own option dialog may have changed while it was
        # open, the icon downloads pick the new one up from here on.
        launchyProxy().reload()
        self.__refreshFavicons(forceRefresh)

    # Top the icon cache up. Called after the engines were read, so also after
    # the option dialog was accepted; only hosts without a usable icon are
    # queued, one that was downloaded in the last week is left alone. The
    # throttle lives in the cache (prefetch skips the attempt inside its
    # window); force is for the callers whose settings just changed, a new
    # engine has to get its icon now.
    def __refreshFavicons(self, force=False):
        favicons = faviconCache()
        favicons.setEnabled(self.__fetchFaviconEnabled())
        if not favicons.isEnabled():
            log.debug("WebSearchPy::__refreshFavicons, favicon fetching is off")
            return
        # Resolve the cache dir here, where the launcher owning the temp dir is
        # the one still running.
        favicons.cacheDir()
        favicons.prefetch([engine.get("url")
                           for engine in self.searchEngine.values()], force)

    def __fetchFaviconEnabled(self):
        key = self.settingName + "/" + WebSearchGui.FETCH_FAVICON_KEY
        return toBool(lSettings.value(key, True), True)

    @classmethod
    def encodeQuery(cls, query):
        return urllib.parse.quote(query.encode("utf8"))

    @classmethod
    def getUrl(cls, key, query):
        engine = WebSearchPy.searchEngine.get(key)
        if not engine:
            log.warning("WebSearchPy::getUrl, unknown engine key: %s" % key)
            return ""
        url = engine.get("url") or ""
        # The url is a template: "%s" is the query, "%%" a literal percent and
        # everything else is kept verbatim -- in particular the "%xx" escapes
        # of a percent-encoded url. Plain "%" formatting, and the eval() this
        # used to be built on, raise on those (a "%20" is an "unsupported
        # format character" to both), which took the whole search down with
        # them; and a url holding a quote or a backslash could break out of
        # the eval()ed string altogether. The placeholder is swapped through a
        # sentinel so an escaped "%%s" stays the literal "%s" it always was.
        result = (url.replace("%%", "\x00")
                     .replace("%s", WebSearchPy.encodeQuery(query))
                     .replace("\x00", "%"))
        if "%s" not in url:
            log.debug("WebSearchPy::getUrl, %s carries no %%s, "
                      "the query is not appended" % url)
        log.debug("WebSearchPy::getUrl: %s" % result)
        return result


def getPlugin():
    return WebSearchPy
