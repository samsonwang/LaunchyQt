TEMPLATE = lib
CONFIG += plugin \
    release
VPATH += ../../src/
INCLUDEPATH += ../../src/
PRECOMPILED_HEADER = precompiled.h
DEFINES += WITH_GUI

# UI_DIR = ../../plugins/controly/
FORMS = dlg.ui
HEADERS = plugin_interface.h \
    controly.h \
    gui.h \
    ControlPanelItemFinder.h \
	fhoicon.h \
	fhoenv.h \
	fhoreg.h \
	fhores.h \
    precompiled.h
SOURCES = plugin_interface.cpp \
    controly.cpp \
    gui.cpp \
    ControlPanelItemFinder.cpp \
	fhoicon.cpp \
	fhoenv.cpp \
	fhoreg.cpp \
	fhores.cpp
TARGET = controly
win32 { 
    CONFIG -= embed_manifest_dll
	LIBS += shell32.lib
	LIBS += user32.lib
	LIBS += Gdi32.lib
	LIBS += comctl32.lib
	LIBS += Advapi32.lib
	LIBS += ole32.lib
	LIBS += shlwapi.lib
	QMAKE_CXXFLAGS_RELEASE += /Zi
	QMAKE_LFLAGS_RELEASE += /DEBUG
# disable optimizations to prevent crashes with certain third party control panel
# applets when Controly is built using VC++ 2005.
	QMAKE_CXXFLAGS_RELEASE -= -O2
}

# *:debug {
# DESTDIR = ../../debug/plugins/
# }
# *:release {
# DESTDIR = ../../release/plugins/
# %QMAKE_CXXFLAGS += /Ox /Ob2 /Oi /Oy /GT /GA /WX
# }
if(!debug_and_release|build_pass):CONFIG(debug, debug|release):DESTDIR = ../../debug/plugins
if(!debug_and_release|build_pass):CONFIG(release, debug|release):DESTDIR = ../../release/plugins
