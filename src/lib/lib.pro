TEMPLATE = lib

unix:!macx:TARGET = launchy
win32:TARGET = Launchy
macx:TARGET = Launchy

CONFIG += debug_and_release

QT += core

SOURCES += CatalogItem.cpp \
           InputData.cpp \
           PluginInterface.cpp \
           PluginInfo.cpp

HEADERS += CatalogItem.h \
           InputData.h \
           LaunchyLib.h \
           PluginInterface.h \
           PluginMsg.h \
           PluginInfo.h

DEFINES += LAUNCHY_LIB

win32 {
   LIBS += user32.lib \
           shell32.lib
}
