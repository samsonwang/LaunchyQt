TEMPLATE = lib
CONFIG += plugin \
    debug_and_release
VPATH += ../../src/
INCLUDEPATH += ../../src/
INCLUDEPATH += c:/boost/
PRECOMPILED_HEADER = precompiled.h
UI_DIR = ../../plugins/calcy/
HEADERS = plugin_interface.h \
    calcy.h \
    precompiled.h \
    gui.h
SOURCES = plugin_interface.cpp \
    calcy.cpp \
    gui.cpp
TARGET = calcy
win32 { 
    CONFIG -= embed_manifest_dll
	LIBS += user32.lib shell32.lib
	QMAKE_CXXFLAGS_RELEASE += /Zi
	QMAKE_LFLAGS_RELEASE += /DEBUG
}
if(!debug_and_release|build_pass):CONFIG(debug, debug|release):DESTDIR = ../../debug/plugins
if(!debug_and_release|build_pass):CONFIG(release, debug|release):DESTDIR = ../../release/plugins


unix:!macx {
    PREFIX = /usr
    target.path = $$PREFIX/lib/launchy/plugins/
    icon.path = $$PREFIX/lib/launchy/plugins/icons/
    icon.files = calcy.png
    INSTALLS += target \
        icon
}
FORMS += dlg.ui

macx {
  if(!debug_and_release|build_pass):CONFIG(debug, debug|release):DESTDIR = ../../debug/Launchy.app/Contents/MacOS/plugins
  if(!debug_and_release|build_pass):CONFIG(release, debug|release):DESTDIR = ../../release/Launchy.app/Contents/MacOS/plugins

    CONFIG(debug, debug|release):icons.path = ../../debug/Launchy.app/Contents/MacOS/plugins/icons/
    CONFIG(release, debug|release):icons.path = ../../release/Launchy.app/Contents/MacOS/plugins/icons/
    icons.files = calcy.png
    INSTALLS += icons

  INCLUDEPATH += /opt/local/include/
}
