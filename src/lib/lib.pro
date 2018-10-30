TEMPLATE = lib
unix:!macx:TARGET = launchy
win32:TARGET = Launchy
macx:TARGET = Launchy

SOURCES += CatalogItem.cpp \
           InputData.cpp

HEADERS += CatalogItem.h \
           InputData.h

DEFINES += LAUNCHY_LIB
