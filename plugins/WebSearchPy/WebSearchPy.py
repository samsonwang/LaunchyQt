
import urllib
import urllib.parse
#import subprocess
import webbrowser
import launchy

class WebSearchPy(launchy.Plugin):
    searchEngine = {
        "gg": {
            "url": "https://www.google.com/search?q=%s&oq=hello&sourceid=chrome&ie=UTF-8",
            "name": "Google"
        },
        "gt":{
            "url": "https://translate.google.com/?hl=en&tab=wT#en/zh-CN/%s",
            "name": "Google Translate"
        },
        "ii": {
            "url": "http://www.bing.com/search?q=%s",
            "name": "Bing"
        },
        "bd": {
            "url": "https://www.baidu.com/s?wd=%s",
            "name": "Baidu"
        },
        "bt": {
            "url": "http://fanyi.baidu.com/?aldtype=85#en/zh/%s",
            "name": "Baidu Translation"
        },
        "gmap": {
            "url": "https://www.google.com/maps/search/?api=1&query=%s",
            "name": "Google Maps"
        },
        "bmap": {
            "url": "http://api.map.baidu.com/geocoder?address=%s&output=html&src=Chrome",
            "name": "Baidu Maps"
        },
        "tao": {
            "url": "https://s.taobao.com/search?q=%s",
            "name": "Taobao"
        },
        "jd": {
            "url": "https://search.jd.com/Search?keyword=%s&enc=utf-8",
            "name": "Jingdong"
        },
        "sn": {
            "url": "https://search.suning.com/%s/",
            "name": "Suning"
        },
        "pr": {
            "url": "https://pronto.inside.nsn.com/pronto/problemReportSearch.html?freeTextdropDownID=prId&searchTopText=%s",
            "name": "Pronto"
        },
        "cpp": {
            "url": "https://en.cppreference.com/mwiki/index.php?title=Special%%3ASearch&search=%s",
            "name": "Cpp Reference"
        },
        "dd": {
            "url": "http://www.dictionary.com/browse/%s",
            "name": "Dictionary"
        },
        "so": {
            "url": "http://stackoverflow.com/search?q=%s",
            "name": "StackOverflow"
        },
        "ieee": {
            "url": "http://ieeexplore.ieee.org/search/searchresult.jsp?newsearch=true&queryText=%s",
            "name": "IEEE"
        },
        "man": {
            "url": "http://www.freebsd.org/cgi/man.cgi?query=%s",
            "name": "Linux Man Page"
        },
        "cygwin": {
            "url": "https://cygwin.com/cgi-bin2/package-grep.cgi?grep=%s&arch=x86",
            "name": "cygwin package search"
        },
        "jj": {
            "url": "https://jira3.int.net.nokia.com/secure/QuickSearch.jspa?searchString=%s",
            "name": "Jira Search"
        },
        "con": {
            "url": "https://confluence.int.net.nokia.com/dosearchsite.action?cql=siteSearch+~+'%s'",
            "name": "Confluence"
        },
        "ww": {
            "url": "https://wft.int.net.nokia.com/ext/build_content/%s",
            "name": "WFT Build Content Search"
        },
        "sp": {
            "url": "https://nokia.sharepoint.com/_layouts/15/sharepoint.aspx?q=%s&v=search",
            "name": "SharePoint Search"
        },
    }

    def __init__(self):
        launchy.Plugin.__init__(self)
        self.hash = launchy.hash(self.getName())

    def init(self):
        pass

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
            resultsList.push_front(launchy.CatItem("%s: %s search" % (self.getName(), self.searchEngine.get(query).get("name")),
                                                   keyword,
                                                   self.getID(),
                                                   self.getIcon()))

    def launchItem(self, inputDataList, catItem):
        key = inputDataList[0].getText()
        query = inputDataList[-1].getText()
        webbrowser.open(self.getUrl(key, query))
        return True

    def launchyShow(self):
        pass

    def launchyHide(self):
        pass

    @classmethod
    def encodeQuery(cls, query):
        return urllib.parse.quote(query.encode("utf8"))

    @classmethod
    def getUrl(cls, key, query):
        return eval('"%s" %% "%s"' % (WebSearchPy.searchEngine.get(key).get('url'), WebSearchPy.encodeQuery(query)))


def getPlugin():
    return WebSearchPy
