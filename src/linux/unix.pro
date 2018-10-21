TEMPLATE	= lib
TARGET		= platform_unix
CONFIG		+= plugin qt_warn debug_and_release
VPATH 		+= ../../src/
INCLUDEPATH     += ../../src/
SOURCES		= platform_x11_hotkey.cpp platform_base_hotkey.cpp platform_unix.cpp platform_unix_util.cpp
HEADERS		= platform_base.h platform_unix.h platform_base_hotkey.h platform_base_hottrigger.h  platform_unix_util.h \
                  platform_x11_hotkey.h
CONFIG		-= embed_manifest_dll
LIBS += -lX11 -lXext -lXrender


unix {
 PREFIX = /usr
 DEFINES += SKINS_PATH=\\\"$$PREFIX/share/launchy/skins/\\\" \
           PLUGINS_PATH=\\\"$$PREFIX/lib/launchy/plugins/\\\" \
           PLATFORMS_PATH=\\\"$$PREFIX/lib/launchy/\\\"
 target.path = $$PREFIX/lib/launchy/
 INSTALLS += target
}

if(!debug_and_release|build_pass):CONFIG(debug, debug|release) {
     DESTDIR = ../../debug/
}

if(!debug_and_release|build_pass):CONFIG(release, debug|release) {
     DESTDIR = ../../release/
}
