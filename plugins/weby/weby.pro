TEMPLATE = lib
QT += network
CONFIG += plugin \
    debug_and_release
VPATH += ../../src/
PRECOMPILED_HEADER = precompiled.h
INCLUDEPATH += ../../src/
INCLUDEPATH += c:/boost/
INCLUDEPATH += ../../common
UI_DIR = ../../plugins/weby/
FORMS = dlg.ui
HEADERS = plugin_interface.h \
    weby.h \
    gui.h \
    globals.h \
    ../../common/DropTableWidget.h \
    precompiled.h \
    IconCache.h
SOURCES = plugin_interface.cpp \
    weby.cpp \
    gui.cpp \
    ../../common/DropTableWidget.cpp \
    IconCache.cpp
TARGET = weby
win32 { 
    CONFIG -= embed_manifest_dll
	LIBS += user32.lib shell32.lib
}
if(!debug_and_release|build_pass):CONFIG(debug, debug|release):DESTDIR = ../../debug/plugins
if(!debug_and_release|build_pass):CONFIG(release, debug|release):DESTDIR = ../../release/plugins
unix:!macx {
    PREFIX = /usr
    target.path = $$PREFIX/lib/launchy/plugins/
    icon.path = $$PREFIX/lib/launchy/plugins/icons/
    icon.files = weby.png
    INSTALLS += target \
        icon
}

macx {
  if(!debug_and_release|build_pass):CONFIG(debug, debug|release):DESTDIR = ../../debug/Launchy.app/Contents/MacOS/plugins
  if(!debug_and_release|build_pass):CONFIG(release, debug|release):DESTDIR = ../../release/Launchy.app/Contents/MacOS/plugins
    CONFIG(debug, debug|release):icons.path = ../../debug/Launchy.app/Contents/MacOS/plugins/icons/
    CONFIG(release, debug|release):icons.path = ../../release/Launchy.app/Contents/MacOS/plugins/icons/
    icons.files = weby.png
    INSTALLS += icons
  INCLUDEPATH += /opt/local/include/
}
