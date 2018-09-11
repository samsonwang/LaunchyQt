TEMPLATE = lib
QT += network
CONFIG += plugin debug_and_release
VPATH += ../../src/
INCLUDEPATH += ../../src/
INCLUDEPATH += c:/boost/
PRECOMPILED_HEADER = precompiled.h
UI_DIR = ../../plugins/gcalc/
HEADERS = plugin_interface.h gcalc.h precompiled.h
SOURCES = plugin_interface.cpp gcalc.cpp
TARGET = gcalc
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
    
    # icon.path = $$PREFIX/lib/launchy/plugins/icons/
    # icon.files = gcalc.png
    INSTALLS += target
}
macx {
  if(!debug_and_release|build_pass):CONFIG(debug, debug|release):DESTDIR = ../../debug/Launchy.app/Contents/MacOS/plugins
  if(!debug_and_release|build_pass):CONFIG(release, debug|release):DESTDIR = ../../release/Launchy.app/Contents/MacOS/plugins


  INCLUDEPATH += /opt/local/include/
}
