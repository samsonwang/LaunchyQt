# Proxy support of the python plugins.
#
# launchy keeps the proxy of its own option dialog in launchy.ini (group
# "Proxy") and hands it to QNetworkProxy::setApplicationProxy(), which only
# requests going through Qt use. urllib looks at the environment and at the
# windows registry instead and would walk past it, so the opener is rebuilt
# here from the very same settings.
#
# The ini is read with configparser rather than through launchy.settings: the
# read happens on the download thread and a QSettings object is not meant to be
# shared with another one.
#
# Threading model:
#   - config()/open() may be called from any thread, the settings are read
#     again after RELOAD_INTERVAL at most.
#   - a socks5 connection is opened by hand because urllib has no handler for
#     it, see socks5Connect().

import configparser
import http.client
import os
import socket
import struct
import threading
import time
import urllib.parse
import urllib.request

import logging as log

# Mirror of QNetworkProxy::ProxyType. The order is the one Qt 5.12 declares in
# <QtNetwork/qnetworkproxy.h> and it is NOT alphabetical nor the order of the
# option dialog:
#
#   DefaultProxy = 0, Socks5Proxy = 1, NoProxy = 2, HttpProxy = 3,
#   HttpCachingProxy = 4, FtpCachingProxy = 5
#
# so a launchy.ini with "proxyType=1" is a socks5 proxy, not the system one.
PROXY_SYSTEM = 0
PROXY_SOCKS5 = 1
PROXY_NONE = 2
PROXY_HTTP = 3

# Seconds the settings are kept before they are read again, the option dialog
# can change the proxy while launchy is running.
RELOAD_INTERVAL = 60

# Group of launchy.ini holding the proxy.
SETTINGS_SECTION = "Proxy"


# QSettings writes every value as text, so a number arrives as a string.
def toInt(value, defaultValue):
    if isinstance(value, bool):
        return int(value)
    if isinstance(value, int):
        return value
    try:
        return int(str(value).strip())
    except Exception:
        return defaultValue


# Same rules as FaviconCache.toBool, duplicated so this module stands alone.
def toBool(value, defaultValue):
    if isinstance(value, bool):
        return value
    text = str(value).strip().lower()
    if text in ("true", "1", "yes", "on"):
        return True
    if text in ("false", "0", "no", "off"):
        return False
    return defaultValue


class ProxyConfig:
    """What launchy's option dialog asks for."""

    def __init__(self, proxyType=PROXY_SYSTEM, host="", port=0, user="", password=""):
        self.type = proxyType
        self.host = host
        self.port = port
        self.user = user
        self.password = password

    def key(self):
        return (self.type, self.host, self.port, self.user, self.password)

    def __repr__(self):
        return ("ProxyConfig(type=%d, host=%s, port=%d, user=%s)"
                % (self.type, self.host, self.port, self.user))

    # A host and a port are what makes a proxy usable, an empty one cannot be
    # dialled at all.
    def isUsable(self):
        return bool(self.host) and self.port > 0

    # "http://[user:password@]host[:port]", the form ProxyHandler expects.
    def url(self):
        netloc = self.host or ""
        if self.port > 0:
            netloc = "%s:%d" % (netloc, self.port)
        if self.user:
            auth = urllib.parse.quote(self.user, safe="")
            if self.password:
                auth += ":" + urllib.parse.quote(self.password, safe="")
            netloc = "%s@%s" % (auth, netloc)
        return "http://" + netloc


# --------------------------------------------------------------- socks5

def _recv(sock, count):
    data = b""
    while len(data) < count:
        chunk = sock.recv(count - len(data))
        if not chunk:
            break
        data += chunk
    return data


# Username/password sub negotiation of rfc 1929.
def _socks5Authenticate(sock, config):
    user = config.user.encode("utf-8")
    password = (config.password or "").encode("utf-8")
    if len(user) > 255 or len(password) > 255:
        raise IOError("socks5: credentials too long")
    sock.sendall(b"\x01" + bytes([len(user)]) + user
                 + bytes([len(password)]) + password)
    reply = _recv(sock, 2)
    if len(reply) != 2 or reply[1] != 0:
        raise IOError("socks5: authentication rejected")


# Open a socket to address through the socks5 proxy config, rfc 1928. The
# greeting, the optional authentication and the CONNECT request are all tiny,
# so no library is needed for them.
def socks5Connect(address, timeout, config):
    sock = socket.create_connection((config.host, config.port), timeout)
    try:
        sock.settimeout(timeout)

        methods = bytearray(b"\x05\x02\x00\x02") if config.user \
            else bytearray(b"\x05\x01\x00")
        sock.sendall(bytes(methods))

        reply = _recv(sock, 2)
        if len(reply) != 2 or reply[0] != 5:
            raise IOError("socks5: unexpected greeting reply %r" % (reply,))
        if reply[1] == 2:
            _socks5Authenticate(sock, config)
        elif reply[1] != 0:
            raise IOError("socks5: no acceptable method (%d)" % reply[1])

        request = bytearray(b"\x05\x01\x00")
        host = str(address[0])
        try:
            request += b"\x01" + socket.inet_aton(host)
        except Exception:
            # Not an ipv4 literal, so it goes over as a name.
            encoded = host.encode("idna")
            if len(encoded) > 255:
                raise IOError("socks5: host name too long")
            request += b"\x03" + bytes([len(encoded)]) + encoded
        request += struct.pack(">H", int(address[1]))
        sock.sendall(bytes(request))

        reply = _recv(sock, 4)
        if len(reply) != 4 or reply[0] != 5:
            raise IOError("socks5: unexpected connect reply %r" % (reply,))
        if reply[1] != 0:
            raise IOError("socks5: connect rejected (%d)" % reply[1])

        # The bound address behind the reply is of no use, it only has to be
        # read so the socket stays in sync.
        if reply[3] == 1:
            _recv(sock, 6)
        elif reply[3] == 3:
            length = _recv(sock, 1)
            if length:
                _recv(sock, length[0] + 2)
        elif reply[3] == 4:
            _recv(sock, 18)

        return sock
    except Exception:
        try:
            sock.close()
        except Exception:
            pass
        raise


# http.client keeps the factory it connects with in an instance attribute, so
# replacing it is enough to push every connection of that object through the
# proxy.
class _Socks5Connection:
    def __init__(self, *args, **kwargs):
        self.m_socksConfig = kwargs.pop("socksConfig", None)
        super().__init__(*args, **kwargs)
        self._create_connection = self.__connect

    def __connect(self, address, timeout, sourceAddress):
        return socks5Connect(address, timeout, self.m_socksConfig)


class _Socks5HTTPConnection(_Socks5Connection, http.client.HTTPConnection):
    pass


class _Socks5HTTPSConnection(_Socks5Connection, http.client.HTTPSConnection):
    pass


class _Socks5HTTPHandler(urllib.request.HTTPHandler):
    def __init__(self, config):
        urllib.request.HTTPHandler.__init__(self)
        self.m_config = config

    def http_open(self, req):
        return self.do_open(_Socks5HTTPConnection, req, socksConfig=self.m_config)


class _Socks5HTTPSHandler(urllib.request.HTTPSHandler):
    def __init__(self, config):
        urllib.request.HTTPSHandler.__init__(self)
        self.m_config = config

    def https_open(self, req):
        return self.do_open(_Socks5HTTPSConnection, req,
                            context=self._context,
                            check_hostname=self._check_hostname,
                            socksConfig=self.m_config)


class LaunchyProxy:
    """The urllib opener that matches launchy's own proxy settings."""

    def __init__(self):
        self.m_lock = threading.Lock()
        self.m_stamp = 0.0
        self.m_config = None
        self.m_opener = None

    # Forget the settings read so far, the next open() reads them again. Called
    # after the option dialog of launchy may have changed the proxy.
    def reload(self):
        with self.m_lock:
            self.m_stamp = 0.0

    def config(self):
        with self.m_lock:
            return self.__currentLocked()

    # Open request through the proxy launchy is configured for. The response is
    # handed back to the caller, which closes it.
    def open(self, request, timeout=None):
        opener = self.__opener()
        if opener is None:
            return urllib.request.urlopen(request, timeout=timeout)
        if timeout is None:
            return opener.open(request)
        return opener.open(request, timeout=timeout)

    # --------------------------------------------------------------- private

    def __currentLocked(self):
        if self.m_config is None or time.time() - self.m_stamp >= RELOAD_INTERVAL:
            config = self.__read()
            if self.m_config is None or config.key() != self.m_config.key():
                # The opener is bound to the proxy it was built for.
                self.m_opener = None
            self.m_config = config
            self.m_stamp = time.time()
        return self.m_config

    def __opener(self):
        with self.m_lock:
            config = self.__currentLocked()
            if self.m_opener is None:
                self.m_opener = self.__build(config)
            return self.m_opener

    # The proxy of launchy's own option dialog. Anything missing or unreadable
    # falls back to the system proxy, which is what urllib does on its own.
    def __read(self):
        path = self.__iniPath()
        if not path:
            log.debug("LaunchyProxy::__read, no ini file, using the system proxy")
            return ProxyConfig(PROXY_SYSTEM)

        try:
            with open(path, "rb") as iniFile:
                # QSettings writes utf-8; whatever is not, only mangles the
                # characters of a value, not the keys looked for here.
                text = iniFile.read().decode("utf-8", "replace")

            parser = configparser.RawConfigParser()
            parser.optionxform = str  # the keys are camel case
            parser.read_string(text)
        except Exception as err:
            log.warning("LaunchyProxy::__read, fail to read %s, %s" % (path, err))
            return ProxyConfig(PROXY_SYSTEM)

        if not parser.has_section(SETTINGS_SECTION):
            return ProxyConfig(PROXY_SYSTEM)

        def value(name, default=""):
            return parser.get(SETTINGS_SECTION, name, fallback=default)

        proxyType = toInt(value("proxyType"), PROXY_SYSTEM)
        host = value("serverName").strip()
        port = toInt(value("serverPort"), 0)

        user = ""
        password = ""
        if toBool(value("requirePassword"), False):
            user = value("username").strip()
            password = value("password")

        return ProxyConfig(proxyType, host, port, user, password)

    def __iniPath(self):
        try:
            import launchy
            path = launchy.settings.fileName()
            if path:
                path = str(path)
                if os.path.isfile(path):
                    return path
                log.debug("LaunchyProxy::__iniPath, %s does not exist" % path)
        except Exception as err:
            log.debug("LaunchyProxy::__iniPath, fail to ask launchy, %s" % err)
        return ""

    def __build(self, config):
        log.debug("LaunchyProxy::__build, %r" % config)

        # "No proxy" has to be spelled out: urllib would otherwise fall back to
        # the proxy windows keeps in its registry.
        if config.type == PROXY_NONE:
            return urllib.request.build_opener(urllib.request.ProxyHandler({}))

        # A proxy launchy named but that has no address cannot be dialled, so
        # nothing is used rather than a proxy from somewhere else.
        if config.type == PROXY_HTTP:
            if not config.isUsable():
                return urllib.request.build_opener(urllib.request.ProxyHandler({}))
            url = config.url()
            return urllib.request.build_opener(
                urllib.request.ProxyHandler({"http": url, "https": url}))

        # urllib has no handler for socks5, so it brings its own connection.
        if config.type == PROXY_SOCKS5:
            if not config.isUsable():
                return urllib.request.build_opener(urllib.request.ProxyHandler({}))
            return urllib.request.build_opener(
                urllib.request.ProxyHandler({}),
                _Socks5HTTPHandler(config),
                _Socks5HTTPSHandler(config))

        # PROXY_SYSTEM and anything unknown: let urllib look at the environment
        # and at the registry, which is what the system proxy means.
        return urllib.request.build_opener(urllib.request.ProxyHandler())


_g_proxy = None
_g_proxyLock = threading.Lock()


# One proxy is shared by the plugins of the process.
def proxy():
    global _g_proxy
    with _g_proxyLock:
        if _g_proxy is None:
            _g_proxy = LaunchyProxy()
    return _g_proxy
