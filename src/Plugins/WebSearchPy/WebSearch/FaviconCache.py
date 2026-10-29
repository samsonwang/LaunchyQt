# Favicon cache of the WebSearchPy plugin.
#
# Every search engine entry shows the icon of its own site. The favicon is
# downloaded once, normalised to a 32x32 png and stored under the temp dir of
# the running instance, so the gui is never blocked by a network round trip.
#
# Threading model:
#   - iconPath() runs on the launchy gui thread, from Plugin.getResults(), and
#     has to stay cheap: a dict lookup, at most one file existence check.
#   - the downloads run on one daemon thread, a single host at a time, every
#     request bounded by a short timeout. Only QImage is taken from Qt there,
#     no widget is ever touched from that thread.
#
# The sites are not hit on every launchy start: the time an icon was fetched
# and the time a fetch failed are kept per host in one state file, and a host
# is only asked again when both are older than a week. The same file also
# records when the cache was last topped up as a whole, which is what lets the
# plugin throttle that topping-up across a restart.

import atexit
import html
import json
import os
import queue
import re
import struct
import threading
import time
import urllib.parse
import urllib.request

import logging as log

from .LaunchyProxy import proxy as launchyProxy

# Imported here, while the plugin is loaded on the gui thread, so the download
# thread never has to initialise PySide2 itself. Without PySide2 the image
# conversion is skipped and only an icon that already is a png gets stored.
try:
    from PySide2 import QtCore, QtGui
except Exception as err:
    log.debug("FaviconCache, PySide2 unavailable, %s" % err)
    QtCore = None
    QtGui = None

# Size the stored icon is normalised to, matches the size launchy asks for.
ICON_SIZE = 32

# Seconds allowed for a single http request.
REQUEST_TIMEOUT = 8

# Seconds one host may consume in total. It is fetched over up to two schemes
# and each scheme can cost two requests, so a site that only hangs would
# otherwise hold the queue for minutes.
HOST_BUDGET = 18

# Hosts are fetched by this many workers. One dead site (a vpn only intranet
# host for instance) has to fail on its own, not behind every other site.
MAX_WORKERS = 4

# A host that could not serve an icon is not retried again before this expires.
FAILURE_TTL = 3 * 24 * 3600

# An icon that was downloaded is left alone for this long. Launchy is started
# several times a day and the sites behind the engines are not ours to poll, so
# one refresh a week is enough.
SUCCESS_TTL = 7 * 24 * 3600

# Topping the cache up as a whole (prefetch) is throttled to this: the ttls
# above are day-level, so an attempt more often buys nothing, and one attempt
# is not free either -- a stat pair per host plus, through the plugin, a
# settings read and possibly a parse of the whole ini, all on the gui thread.
# The stamp of the last attempt lives in the state file, so a restart inside
# the window does not run it a second time.
REFRESH_INTERVAL = 12 * 3600

# <app temp dir>/WebSearchPy, the plugin keeps its downloaded icons there
CACHE_DIR_NAME = "WebSearchPy"

# Per host, when its icon was last downloaded and when a download of it last
# failed. Both in one file, they say together whether a host is worth a request:
# {"ok": {host: epoch}, "failed": {host: epoch}, "refreshed": epoch}
# "refreshed" is when the cache was last topped up as a whole (prefetch or
# refresh); the plugin throttles its next prefetch on it, and it is persisted
# so the throttle window survives a restart.
STATE_FILE = "favicons.json"

# Max bytes read from a response, a favicon is small and a homepage only needs
# its <head> to carry the icon link. Sites exist whose head is longer than
# expected (mediawiki skins push their <link> tags past 80k), hence the headroom.
ICON_READ_LIMIT = 256 * 1024
PAGE_READ_LIMIT = 256 * 1024

USER_AGENT = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) LaunchyQt/WebSearchPy"

IMAGE_MAGIC = (b"\x89PNG", b"GIF8", b"\xff\xd8\xff", b"\x00\x00\x01\x00")

# Layout of the ico directory: a 6 byte header followed by 16 byte entries.
ICO_HEADER_SIZE = 6
ICO_ENTRY_SIZE = 16


# QSettings of the ini file hands values back as strings, so a stored bool
# arrives as "true"/"false" rather than as a bool. Shared by the plugin and by
# its option dialog, which both read the "fetch site icons" switch.
def toBool(value, defaultValue):
    if isinstance(value, bool):
        return value
    text = str(value).strip().lower()
    if text in ("true", "1", "yes", "on"):
        return True
    if text in ("false", "0", "no", "off"):
        return False
    return defaultValue


# Keep the host -> epoch records that still say something, an entry older than
# its ttl only means the host is fetched again on the next start.
def pruneStamps(mapping, now, ttl):
    kept = {}
    if not isinstance(mapping, dict):
        return kept
    for host, stamp in mapping.items():
        try:
            stamp = int(stamp)
        except Exception:
            continue
        if now - stamp < ttl:
            kept[str(host)] = stamp
    return kept


# The stamp of the last refresh, as read back from the state file. Only a
# number in the past is trusted: one from the future (a clock that jumped, a
# hand-edited file) would hold every refresh back until time reached it, so it
# counts as "never refreshed" instead.
def saneStamp(value, now):
    try:
        stamp = float(value)
    except Exception:
        return 0.0
    if stamp <= 0 or stamp > now + 60:
        return 0.0
    return stamp


# Extract the host of a search engine url. The url still holds the "%s" of the
# query, urlsplit() does not care about it.
def hostOf(url):
    if not url:
        return ""
    try:
        host = urllib.parse.urlsplit(url).netloc
    except Exception as err:
        log.warning("FaviconCache::hostOf, fail to split url: %s, %s" % (url, err))
        return ""
    if "@" in host:
        host = host.rsplit("@", 1)[1]
    return host.strip().lower()


# The host becomes a file name, so anything a file system may dislike (the colon
# of "host:8080" for instance) is replaced.
def safeName(host):
    name = re.sub(r"[^A-Za-z0-9._-]", "_", host)
    return name if name else "_"


# True when the bytes look like an image we can store or convert. Sites happily
# answer a favicon request with a html error page, so the content is checked
# instead of the http status.
def isImage(data):
    if not data:
        return False
    return data.startswith(IMAGE_MAGIC)


class FaviconCache:
    def __init__(self):
        self.m_dir = ""
        self.m_icons = {}     # host -> png path, only entries known to exist
        self.m_missing = set()  # hosts already checked and not on disk
        self.m_done = {}      # host -> epoch of the last successful download
        self.m_failed = {}    # host -> epoch of the last failed download
        self.m_lastRefresh = 0.0  # epoch of the last prefetch()/refresh()
        self.m_pending = set()  # hosts queued for the download thread
        self.m_refreshHosts = set()  # hosts forced past __isFresh by refresh()
        self.m_enabled = True
        self.m_proxyKey = None  # proxy the failures below were recorded under
        self.m_lock = threading.Lock()
        self.m_dirLock = threading.Lock()
        self.m_queue = queue.Queue()
        self.m_threads = []

    # ---------------------------------------------------------------- public

    def setEnabled(self, enabled):
        self.m_enabled = bool(enabled)

    def isEnabled(self):
        return self.m_enabled

    # Path of the cached icon of the site behind url, or None when it is not
    # downloaded yet. Called from the gui thread, must not do any network or
    # heavy work.
    def iconPath(self, url):
        # Switched off means the plain plugin icon, even for sites already in
        # the cache.
        if not self.m_enabled:
            return None
        host = hostOf(url)
        if not host:
            return None

        with self.m_lock:
            path = self.m_icons.get(host)
            knownMissing = host in self.m_missing
        if path:
            return path
        if knownMissing:
            return None

        path = self.__pathForHost(host)
        if os.path.isfile(path) and os.path.getsize(path) > 0:
            with self.m_lock:
                self.m_icons[host] = path
            return path

        with self.m_lock:
            self.m_missing.add(host)
        return None

    # Queue the site behind url for a download, unless it is cached, young
    # enough, already queued or failed recently.
    def request(self, url):
        if not self.m_enabled:
            return
        host = hostOf(url)
        if not host:
            return

        with self.m_lock:
            if host in self.m_icons or host in self.m_pending:
                return
            if self.__recentlyFailedLocked(host):
                return
            forceRefresh = host in self.m_refreshHosts

        # The icon of the last week is still good, the site does not hear from
        # us again before SUCCESS_TTL has passed. A host flagged by refresh()
        # bypasses this: the user asked to re-fetch it even when the on-disk
        # icon is still young, so the mtime fallback in __isFresh does not
        # short-circuit a refresh either.
        if not forceRefresh and self.__isFresh(host):
            return

        with self.m_lock:
            self.m_pending.add(host)

        self.__startWorkers()
        self.m_queue.put(url)

    def prefetch(self, urls, force=False):
        self.cacheDir()
        # Throttled on the stamp of the last attempt, which the state file
        # persists: a launchy that starts inside the window does not run it
        # again. force is for the callers whose settings just changed, a new
        # engine has to get its icon now rather than after the window.
        if not force:
            elapsed = time.time() - self.lastRefresh()
            if elapsed < REFRESH_INTERVAL:
                log.debug("FaviconCache::prefetch, last refresh %.0fs ago, "
                          "skipped" % elapsed)
                return
        # The attempt counts even when every host turns out to be young: it is
        # the attempt the throttle is on, and the stamp is what a restart reads
        # back.
        self.__stampRefresh()
        # A new proxy can reach hosts the old one could not, so a change makes
        # the sites worth another try instead of waiting out the failure ttl.
        self.__forgetFailuresOnProxyChange()
        for url in urls or []:
            self.request(url)

    # Hard wipe: drop every downloaded icon and the state file. No longer
    # used by the "refresh" button (which calls refresh() for a soft reset
    # that keeps the previous icons), kept as a utility for a future "clear
    # all" action.
    def clear(self):
        cacheDir = self.cacheDir()
        if cacheDir:
            for name in os.listdir(cacheDir):
                if name.endswith(".png") or name.endswith(".tmp"):
                    try:
                        os.remove(os.path.join(cacheDir, name))
                    except Exception as err:
                        log.warning("FaviconCache::clear, fail to remove %s, %s"
                                    % (name, err))
            try:
                os.remove(os.path.join(cacheDir, STATE_FILE))
            except Exception:
                pass

        with self.m_lock:
            self.m_icons.clear()
            self.m_missing.clear()
            self.m_done.clear()
            self.m_failed.clear()
            # A wiped cache is re-warmed at once, not held back by the stamp
            # that was in the state file it just deleted.
            self.m_lastRefresh = 0.0

    # Soft refresh: re-fetch the listed sites but keep their previous icons
    # on disk. A host whose re-fetch fails keeps its old .png, unlike clear()
    # which wipes everything before the re-fetch even starts. Used by the
    # "refresh icons" button of the option dialog.
    def refresh(self, urls):
        self.__forgetFailuresOnProxyChange()
        hosts = set()
        for url in urls or []:
            host = hostOf(url)
            if host:
                hosts.add(host)
        if not hosts:
            log.debug("FaviconCache::refresh, no host to refresh")
            return

        log.debug("FaviconCache::refresh, re-fetching %d host(s)" % len(hosts))
        cacheDir = self.cacheDir()
        with self.m_lock:
            # Drop the in-memory bookkeeping so iconPath() re-checks disk and
            # request() does not short-circuit on a stale "known" or "failed
            # recently" entry. The .png files and m_done stay: m_done is
            # overwritten on success and popped on failure, and the png files
            # are the very thing this method preserves.
            self.m_icons.clear()
            self.m_missing.clear()
            self.m_failed.clear()
            self.m_refreshHosts.clear()
            self.m_refreshHosts |= hosts

        # Persist the cleared failure records and the refresh stamp, so a
        # crash mid-refresh does not bring back the old "recently failed" gate
        # on the next start.
        self.__stampRefresh()

        # A .tmp leftover is from a write that was interrupted, it is never
        # useful now and would shadow nothing, so it is cleaned up here. The
        # .png files are deliberately left alone.
        if cacheDir:
            for name in os.listdir(cacheDir):
                if name.endswith(".tmp"):
                    try:
                        os.remove(os.path.join(cacheDir, name))
                    except Exception:
                        pass

        for url in urls or []:
            self.request(url)

    # Resolved once and under a lock, the cache dir cannot move while the
    # download workers write into it.
    def cacheDir(self):
        if not self.m_dir:
            with self.m_dirLock:
                if not self.m_dir:
                    self.m_dir = self.__resolveCacheDir()
        return self.m_dir

    # Epoch of the last refresh of the cache as a whole (prefetch or refresh),
    # 0.0 when it was never refreshed. It is kept in the state file, so a
    # launchy started minutes after the last refresh still sees the young
    # stamp and leaves the sites alone. Resolves the cache dir, which is what
    # loads the stamp from the file on the first call; the in-memory value
    # keeps the throttle working even when the file cannot be written.
    def lastRefresh(self):
        self.cacheDir()
        with self.m_lock:
            return self.m_lastRefresh

    # --------------------------------------------------------------- private

    # The icons are throwaway data, a host whose record expired is fetched
    # again anyway, so they belong in the temp dir rather than next to the ini
    # file: in a portable install that dir sits under "Program Files" and is
    # not writable for a normal user at all. launchy.getAppTempPath() already
    # points at <temp>/Launchy, building that path from %TEMP% by hand only
    # risks a second, different directory.
    def __resolveCacheDir(self):
        try:
            import launchy
            base = str(launchy.getAppTempPath())
        except Exception as err:
            log.warning("FaviconCache::__resolveCacheDir, fail to ask launchy, %s" % err)
            base = ""

        if base:
            path = os.path.join(base, CACHE_DIR_NAME)
            if self.__ensureDir(path):
                self.m_done, self.m_failed, self.m_lastRefresh = \
                    self.__loadState(path)
                return path

        log.warning("FaviconCache::__resolveCacheDir, no writable cache dir found")
        return ""

    def __ensureDir(self, path):
        try:
            if not os.path.isdir(path):
                os.makedirs(path)
            probe = os.path.join(path, ".writetest")
            with open(probe, "wb") as probeFile:
                probeFile.write(b"1")
            os.remove(probe)
            return True
        except Exception as err:
            log.debug("FaviconCache::__ensureDir, %s is not usable, %s" % (path, err))
            return False

    def __pathForHost(self, host):
        cacheDir = self.cacheDir()
        if not cacheDir:
            return ""
        return os.path.join(cacheDir, safeName(host) + ".png")

    # The downloads and the failures of every host, plus the stamp of the last
    # refresh of the cache as a whole, read together.
    def __loadState(self, cacheDir):
        state = self.__readJson(os.path.join(cacheDir, STATE_FILE))
        if not isinstance(state, dict):
            return {}, {}, 0.0

        now = int(time.time())
        return (pruneStamps(state.get("ok"), now, SUCCESS_TTL),
                pruneStamps(state.get("failed"), now, FAILURE_TTL),
                saneStamp(state.get("refreshed"), now))

    def __readJson(self, path):
        if not os.path.isfile(path):
            return None
        try:
            with open(path, "r") as stateFile:
                return json.load(stateFile)
        except Exception as err:
            log.warning("FaviconCache::__readJson, fail to read %s, %s" % (path, err))
            return None

    def __saveState(self):
        cacheDir = self.cacheDir()
        if not cacheDir:
            return
        now = int(time.time())
        with self.m_lock:
            state = {"ok": pruneStamps(self.m_done, now, SUCCESS_TTL),
                     "failed": pruneStamps(self.m_failed, now, FAILURE_TTL),
                     "refreshed": int(self.m_lastRefresh)}
        # Written through a temp file: launchy can exit while a worker is in
        # the middle of it, and a truncated file loses every record.
        if not self.__writeAtomic(os.path.join(cacheDir, STATE_FILE),
                                  json.dumps(state, sort_keys=True, indent=2).encode("utf-8")):
            return

    # Record a refresh of the cache as a whole and persist it right away: the
    # stamp is the whole point, and a refresh that queued nothing (everything
    # still young) would otherwise never reach the state file -- nothing else
    # saves it. The in-memory value is set even when the write fails, so the
    # throttle keeps working for the rest of this process either way.
    def __stampRefresh(self):
        now = int(time.time())
        with self.m_lock:
            self.m_lastRefresh = now
        self.__saveState()

    # True when the icon of host is on disk and young enough to spare the site
    # another request. Called from request(), on the gui thread.
    def __isFresh(self, host):
        # Also resolves the cache dir, and with it the state file, so the
        # record below is loaded before it is asked for.
        path = self.__pathForHost(host)
        if not path or not os.path.isfile(path) or os.path.getsize(path) <= 0:
            return False

        with self.m_lock:
            stamp = self.m_done.get(host)
        if not stamp:
            # An icon downloaded by a build that did not record anything yet,
            # its own mtime is the best guess at when that happened.
            try:
                stamp = int(os.path.getmtime(path))
            except Exception as err:
                log.debug("FaviconCache::__isFresh, no age for %s, %s" % (path, err))
                return False
            with self.m_lock:
                self.m_done[host] = stamp

        if int(time.time()) - stamp >= SUCCESS_TTL:
            return False

        with self.m_lock:
            self.m_icons[host] = path
        return True

    # A failure says as much about the proxy it happened under as about the
    # host, so a different proxy gets another try at the hosts that failed. The
    # icons already on disk stay, they are favicons whoever served them.
    def __forgetFailuresOnProxyChange(self):
        try:
            key = launchyProxy().config().key()
        except Exception as err:
            log.debug("FaviconCache::__forgetFailuresOnProxyChange, %s" % err)
            return

        with self.m_lock:
            if self.m_proxyKey is None:
                self.m_proxyKey = key
                return
            if key == self.m_proxyKey:
                return
            self.m_proxyKey = key
            self.m_failed.clear()
            self.m_missing.clear()

        self.__saveState()
        log.debug("FaviconCache::__forgetFailuresOnProxyChange, proxy changed to %r" % (key,))

    # Called with m_lock held.
    def __recentlyFailedLocked(self, host):
        stamp = self.m_failed.get(host)
        if not stamp:
            return False
        return int(time.time()) - int(stamp) < FAILURE_TTL

    # A dead host is fetched on its own, so several workers pull from the same
    # queue: one slow site must not push the other twenty behind it.
    def __startWorkers(self):
        with self.m_lock:
            self.m_threads = [thread for thread in self.m_threads if thread.is_alive()]
            for _ in range(MAX_WORKERS - len(self.m_threads)):
                thread = threading.Thread(target=self.__run,
                                          name="WebSearchPyFavicons")
                thread.daemon = True
                thread.start()
                self.m_threads.append(thread)

    def __run(self):
        while True:
            url = self.m_queue.get()
            if url is None:
                # The sentinel shutdown() drops into the queue: stop the worker
                # instead of treating None as a url.
                return
            try:
                self.__download(url)
            except Exception as err:
                log.warning("FaviconCache::__run, fail to download %s, %s" % (url, err))
            finally:
                host = hostOf(url)
                with self.m_lock:
                    self.m_pending.discard(host)
                    self.m_refreshHosts.discard(host)

    # Drop one sentinel per worker so every __run loop exits instead of
    # blocking on the queue for ever. The workers are daemon threads and the
    # process exit reclaims them anyway, but the sentinel makes the None check
    # in __run real and lets a graceful close actually stop the downloads.
    def shutdown(self):
        with self.m_lock:
            workers = [thread for thread in self.m_threads if thread.is_alive()]
        for _ in workers:
            self.m_queue.put(None)
        for thread in workers:
            thread.join(timeout=REQUEST_TIMEOUT)

    def __download(self, url):
        host = hostOf(url)
        with self.m_lock:
            if host in self.m_icons:
                return

        data = self.__fetchIcon(url, time.time() + HOST_BUDGET)
        if not data:
            self.__markFailed(host)
            return

        path = self.__pathForHost(host)
        if not path or not self.__writePng(data, path):
            self.__markFailed(host)
            return

        now = int(time.time())
        with self.m_lock:
            self.m_icons[host] = path
            self.m_missing.discard(host)
            self.m_failed.pop(host, None)
            # From here on the host is left alone for SUCCESS_TTL, so the state
            # has to be written even though nothing failed.
            self.m_done[host] = now
        self.__saveState()

        log.debug("FaviconCache::__download, icon of %s stored as %s" % (host, path))

    # The favicon is looked for at the classic location first, and in the
    # homepage's <link rel="icon"> when that answers with something else.
    # deadline bounds the whole host, however many requests that takes.
    def __fetchIcon(self, url, deadline):
        for base in self.__baseUrls(url):
            data = self.__get(base + "/favicon.ico", ICON_READ_LIMIT, deadline)
            if isImage(data):
                return data

            page = self.__get(base + "/", PAGE_READ_LIMIT, deadline)
            if page:
                href = self.__parseIconLink(page, base + "/")
                if href:
                    icon = self.__get(href, ICON_READ_LIMIT, deadline)
                    if isImage(icon):
                        return icon

            # The host answered, trying the other scheme would only repeat it.
            if page or data:
                return None

        return None


    # The scheme the user configured first, then the other one: some sites only
    # serve a favicon over one of them.
    def __baseUrls(self, url):
        scheme = "https"
        try:
            scheme = urllib.parse.urlsplit(url).scheme or "https"
        except Exception:
            pass

        host = hostOf(url)
        bases = ["%s://%s" % (scheme, host)]
        other = "http" if scheme == "https" else "https"
        bases.append("%s://%s" % (other, host))
        return bases

    def __get(self, url, limit, deadline):
        remaining = deadline - time.time()
        if remaining <= 0:
            log.debug("FaviconCache::__get, out of time before %s" % url)
            return None
        try:
            request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
            # urlopen() would walk past the proxy of launchy's own option
            # dialog, so the opener is the one built from those settings.
            with launchyProxy().open(request, min(REQUEST_TIMEOUT, remaining)) as response:
                return response.read(limit)
        except Exception as err:
            log.debug("FaviconCache::__get, fail to get %s, %s" % (url, err))
            return None

    def __parseIconLink(self, page, baseUrl):
        try:
            text = page.decode("utf-8", "ignore")
        except Exception:
            return ""

        tag = re.search(r"<link[^>]+rel\s*=\s*[\"'][^\"']*icon[^\"']*[\"'][^>]*>",
                        text, re.IGNORECASE)
        if not tag:
            return ""

        href = re.search(r"href\s*=\s*[\"']([^\"']+)[\"']", tag.group(0), re.IGNORECASE)
        if not href:
            return ""

        return urllib.parse.urljoin(baseUrl, html.unescape(href.group(1)))

    # An ico is a directory of images and most sites store them as png blobs.
    # Taking one out needs no image library, so it still works when Qt cannot
    # find its imageformats plugins (ico, gif and jpeg are plugins, png is not).
    def __pngFromIco(self, data):
        if not data.startswith(b"\x00\x00\x01\x00") or len(data) < ICO_HEADER_SIZE:
            return None

        try:
            count = struct.unpack("<H", data[4:6])[0]
        except Exception as err:
            log.debug("FaviconCache::__pngFromIco, broken ico header, %s" % err)
            return None

        best = None
        bestScore = 0
        for index in range(count):
            entry = ICO_HEADER_SIZE + index * ICO_ENTRY_SIZE
            if entry + ICO_ENTRY_SIZE > len(data):
                break
            width = data[entry] or 256
            try:
                size, offset = struct.unpack("<II", data[entry + 8:entry + 16])
            except Exception:
                continue
            payload = data[offset:offset + size]
            if not payload.startswith(b"\x89PNG"):
                continue
            score = abs(width - ICON_SIZE)
            if best is None or score < bestScore:
                best = payload
                bestScore = score

        return best

    # True when image carries no opaque pixel. Mirrors hasVisiblePixel() in
    # UWPApp.cpp: an image without an alpha channel is always visible, and an
    # image whose every pixel has alpha 0 is blank. Called only after a
    # successful decode on the download thread, so the 32*32 pixel scan is
    # fine. Fails open: any pixel-read error returns False so __writePng still
    # proceeds with the write.
    def __isBlank(self, image):
        if QtGui is None:
            return False
        try:
            if image is None or image.isNull():
                return True
            if not image.hasAlphaChannel():
                return False
            for y in range(image.height()):
                for x in range(image.width()):
                    # QImage.pixel() returns a QRgb int (0xAARRGGBB); the
                    # high byte is the alpha channel.
                    if (image.pixel(x, y) >> 24) & 0xFF != 0:
                        return False
            return True
        except Exception as err:
            log.debug("FaviconCache::__isBlank, pixel scan failed, %s" % err)
            return False

    # The cache only holds png so launchy needs no ico reader, but whatever the
    # site served (ico, gif, jpeg) is converted through QImage.
    def __writePng(self, data, path):
        cacheDir = os.path.dirname(path)
        if not self.__ensureDir(cacheDir):
            return False

        # Prefer a png stored inside an ico, it needs no image plugin to be read.
        payload = data
        if not payload.startswith(b"\x89PNG"):
            payload = self.__pngFromIco(data) or data

        try:
            if QtGui is None:
                raise ImportError("PySide2 is unavailable")
            image = QtGui.QImage.fromData(QtCore.QByteArray(payload))
            if not image.isNull():
                image = image.scaled(ICON_SIZE, ICON_SIZE,
                                     QtCore.Qt.KeepAspectRatio,
                                     QtCore.Qt.SmoothTransformation)
                # A site that removed its favicon may answer with a 1x1
                # transparent gif/png: it passes isImage() and decodes fine,
                # but the scaled result is fully transparent and would
                # overwrite a good cached icon with an invisible one. When a
                # valid old icon is on disk, refuse the blank so __download
                # routes to __markFailed and the previous icon survives.
                if (os.path.isfile(path) and os.path.getsize(path) > 0
                        and self.__isBlank(image)):
                    log.debug("FaviconCache::__writePng, blank image for %s, "
                              "keeping the previous icon" % path)
                    return False
                tmpPath = path + ".tmp"
                if image.save(tmpPath, "PNG"):
                    # The rename is the atomic publish; when it fails the .tmp
                    # is a half-written leftover and must not linger to shadow
                    # the next attempt (a stale .tmp is never a valid icon).
                    try:
                        os.replace(tmpPath, path)
                    except Exception as err:
                        log.warning("FaviconCache::__writePng, fail to publish %s, %s"
                                    % (path, err))
                        try:
                            os.remove(tmpPath)
                        except Exception:
                            pass
                        return False
                    return True
                log.debug("FaviconCache::__writePng, fail to save %s" % tmpPath)
            else:
                log.debug("FaviconCache::__writePng, unreadable image for %s" % path)
        except Exception as err:
            log.warning("FaviconCache::__writePng, fail to convert icon %s, %s"
                        % (path, err))

        # Qt could not read it, but a plain png can still be stored as it is.
        if payload.startswith(b"\x89PNG"):
            return self.__writeAtomic(path, payload)

        return False

    # Launchy reads the icon from its own thread, so it must never see a half
    # written file: write to a temp name first, then rename.
    def __writeAtomic(self, path, data):
        tmpPath = path + ".tmp"
        try:
            with open(tmpPath, "wb") as iconFile:
                iconFile.write(data)
            os.replace(tmpPath, path)
            return True
        except Exception as err:
            log.warning("FaviconCache::__writeAtomic, fail to write %s, %s" % (path, err))
            try:
                os.remove(tmpPath)
            except Exception:
                pass
            return False

    def __markFailed(self, host):
        if not host:
            return
        now = int(time.time())
        with self.m_lock:
            self.m_failed[host] = now
            self.m_done.pop(host, None)
        self.__saveState()
        log.debug("FaviconCache::__markFailed, %s" % host)


_g_cache = None
_g_cacheLock = threading.Lock()


# One cache is shared by the plugin and by its option dialog, they run in the
# same process.
def cache():
    global _g_cache
    with _g_cacheLock:
        if _g_cache is None:
            _g_cache = FaviconCache()
    return _g_cache


# The workers are daemon threads, so the interpreter can exit without them, but
# asking them to stop first lets an in-flight download finish its atomic write
# instead of being cut off mid-file. Registered once, when this module loads.
def _shutdownCache():
    global _g_cache
    with _g_cacheLock:
        cache_ = _g_cache
    if cache_ is not None:
        cache_.shutdown()


atexit.register(_shutdownCache)
