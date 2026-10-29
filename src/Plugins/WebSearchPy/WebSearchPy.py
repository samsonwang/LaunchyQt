
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
    def __init__(self):
        Plugin.__init__(self)
        # The engines and the "fetch site icons" switch are stored in one
        # group, so writeSettings() must not drop the whole group to rewrite
        # the array; it only removes the entries left behind by a shorter list.
        # Kept as an instance dict so one plugin instance can never leak its
        # engines into another (it used to be a mutable class attribute).
        self.searchEngine = {}

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
            inputDataList[-1].setLabel(self.getName())

    def getResults(self, inputDataList, resultsList):
        if not inputDataList[-1].hasLabel(self.getName()):
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

    def getCatalog(self, resultsList):
        # One of the two refresh entry points (the other is the "refresh icons"
        # button of the option dialog). Runs on the catalog builder thread, so
        # it must not touch the gui: prefetch() only queues hosts on the cache's
        # own download thread and never blocks on the network here.
        self.__refreshFavicons()

    def launchItem(self, inputDataList, catItem):
        if len(inputDataList) == 2:
            key = inputDataList[0].getText()
            query = inputDataList[-1].getText()
            url = self.__getUrl(key, query)
            log.debug('WebSerachPy::launchyItem, key: %s, query: %s url: %s'
                      % (key, query, url))
            if url:
                webbrowser.open(url)
        return True

    def doDialog(self, parentWidgetPtr):
        log.debug('WebSearchPy::doDialog ...')
        parentWidget = wrapInstance(parentWidgetPtr, QWidget)
        self.widget = WebSearchGui(parentWidget, self.getName())
        self.widget.show()
        return getCppPointer(self.widget)[0]

    def endDialog(self, accept):
        log.debug('WebSearchPy::endDialog ...')
        self.widget.hide()
        if accept:
            self.widget.writeSettings()
        # Re-read the engines and the "fetch site icons" switch whether the
        # dialog was accepted or cancelled: the switch may have been toggled
        # while it was open. No refresh is forced here -- the catalog builder
        # refreshes the icons when it next runs getCatalog.
        self.__readConfig()
        del self.widget
        self.widget = None

    def __readConfig(self):
        settings = lSettings
        self.searchEngine.clear()

        # Read directories from the settings file
        size = settings.beginReadArray(self.getName())
        if size <= 0:
            log.debug("WebSearchPy::__readConfig, load defaults")
            from WebSearch import Defaults
            self.searchEngine.update(Defaults.defaultSetting)
        else:
            log.debug("WebSearchPy::__readConfig, read from config, count: %s" % size)
            for i in range(0, size):
                settings.setArrayIndex(i)
                # A hand-edited ini can lose an entry, and value() answers None
                # for it: as a dict key that is an engine no input can ever
                # match, as a name it would print as "None" in the result.
                # Outer whitespace is stripped, a key typed with a stray space
                # in the ini would otherwise never match the input.
                key = str(settings.value("key", "") or "").strip()
                name = str(settings.value("name", "") or "").strip()
                url = str(settings.value("url", "") or "").strip()
                if not key:
                    log.warning("WebSearchPy::__readConfig, entry %d has no key, skipped" % i)
                    continue
                if key in self.searchEngine:
                    # The dict would silently overwrite the first entry, which
                    # is the one the option dialog keeps.
                    log.warning("WebSearchPy::__readConfig, duplicate key %s, "
                                "the first entry is kept" % key)
                    continue
                self.searchEngine[key] = {"name":name, "url": url}
        settings.endArray()
        log.debug("WebSearchPy::__readConfig: %s" % self.searchEngine)
        # The proxy of launchy's own option dialog may have changed while it was
        # open, the icon downloads pick the new one up from here on.
        launchyProxy().reload()

    # Top the icon cache up. The only automatic entry point is getCatalog(),
    # which runs on the catalog builder thread; the other, manual one is the
    # "refresh icons" button of the option dialog. prefetch() is forced so a
    # freshly added engine gets its icon right away instead of waiting out the
    # REFRESH_INTERVAL window: the force only skips the whole-cache throttle,
    # each host still carries its own ttl (a success is left alone for a week,
    # a failure is not retried for days), so once everything is downloaded a
    # run costs nothing but a stat per engine.
    def __refreshFavicons(self):
        favicons = faviconCache()
        favicons.setEnabled(self.__fetchFaviconEnabled())
        if not favicons.isEnabled():
            log.debug("WebSearchPy::__refreshFavicons, favicon fetching is off")
            return
        urls = [engine.get("url") for engine in self.searchEngine.values()]
        log.debug("WebSearchPy::__refreshFavicons, refreshing %d engine(s)" % len(urls))
        # Resolve the cache dir here, where the launcher owning the temp dir is
        # the one still running.
        favicons.prefetch(urls, force=True)

    def __fetchFaviconEnabled(self):
        key = self.getName() + "/" + WebSearchGui.FETCH_FAVICON_KEY
        return toBool(lSettings.value(key, True), True)

    def __encodeQuery(self, query):
        return urllib.parse.quote(query.encode("utf8"))

    def __getUrl(self, key, query):
        engine = self.searchEngine.get(key)
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
                     .replace("%s", self.__encodeQuery(query))
                     .replace("\x00", "%"))
        if "%s" not in url:
            log.debug("WebSearchPy::getUrl, %s carries no %%s, "
                      "the query is not appended" % url)
        log.debug("WebSearchPy::getUrl: %s" % result)
        return result


def getPlugin():
    return WebSearchPy
