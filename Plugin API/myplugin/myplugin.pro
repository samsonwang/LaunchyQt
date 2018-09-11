TEMPLATE = lib
CONFIG += plugin \
    debug_and_release
FORMS = dlg.ui
HEADERS = plugin_interface.h \
    myplugin.h \
    gui.h
SOURCES = plugin_interface.cpp \
    myplugin.cpp \
    gui.cpp
TARGET = myplugin
win32 { 
    CONFIG -= embed_manifest_dll
    LIBS += shell32.lib
}
