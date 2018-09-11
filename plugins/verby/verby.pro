TEMPLATE = lib
CONFIG += plugin \
    release
VPATH += ../../src/
INCLUDEPATH += ../../src/
PRECOMPILED_HEADER = precompiled.h
UI_DIR = ../../plugins/Verby/
FORMS = dlg.ui
HEADERS = plugin_interface.h \
    gui.h \
    Verby.h \
    precompiled.h
SOURCES = plugin_interface.cpp \
    gui.cpp \
    Verby.cpp
TARGET = verby
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
    icon.files = verby.png copy.png opencontainer.png properties.png run.png
    INSTALLS += target \
        icon
}
macx {
  if(!debug_and_release|build_pass):CONFIG(debug, debug|release):DESTDIR = ../../debug/Launchy.app/Contents/MacOS/plugins
  if(!debug_and_release|build_pass):CONFIG(release, debug|release):DESTDIR = ../../release/Launchy.app/Contents/MacOS/plugins
    CONFIG(debug, debug|release):icons.path = ../../debug/Launchy.app/Contents/MacOS/plugins/icons/
    CONFIG(release, debug|release):icons.path = ../../release/Launchy.app/Contents/MacOS/plugins/icons/
    icons.files = verby.png copy.png opencontainer.png properties.png run.png
    INSTALLS += icons
  INCLUDEPATH += /opt/local/include/
}
