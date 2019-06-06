TEMPLATE = lib

unix:!macx:TARGET = launchy
win32:TARGET = Launchy
macx:TARGET = Launchy

CONFIG += debug_and_release

QT += core

SOURCES += CatalogItem.cpp \
           InputData.cpp \
           LaunchyLib.cpp \
           PluginInterface.cpp \
           PluginInfo.cpp \
           UnicodeTable.cpp

HEADERS += CatalogItem.h \
           InputData.h \
           LaunchyLib.h \
           PluginInterface.h \
           PluginMsg.h \
           PluginInfo.h \
           UnicodeTable.h

DEFINES += LAUNCHY_LIB

if(!debug_and_release|build_pass) {
    CONFIG(debug, debug|release):DESTDIR = ../../debug/
    CONFIG(release, debug|release):DESTDIR = ../../release/
}

unix:!macx {
    PREFIX   = /usr
    target.path   = $$PREFIX/lib/
    INSTALLS += target
}

win32 {
   LIBS += user32.lib \
           shell32.lib
}
