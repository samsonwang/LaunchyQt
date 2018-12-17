
QT += core gui widgets
CONFIG += c++11

HEADERS += $$PWD/QHotkey.h \
           $$PWD/QHotkeyP.h
SOURCES += $$PWD/QHotkey.cpp \
           $$PWD/QHotkeyP.cpp

#INCLUDEPATH += $$PWD

win32 {
    SOURCES    += $$PWD/QHotkeyWin.cpp
    QT         += winextras
    #msvc:LIBS += Advapi32.lib
    #gcc:LIBS  += -ladvapi32
}

unix:!macx {
    SOURCES   += $$PWD/QHotkeyX11.cpp
    QT        += x11extras
    CONFIG    += link_pkgconfig
    PKGCONFIG += x11
    LIBS      += -lX11 -lxcb
}
