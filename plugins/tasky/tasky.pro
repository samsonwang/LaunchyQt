TEMPLATE = lib

CONFIG += plugin \
          debug_and_release
INCLUDEPATH += ../../src \
               ../../src/lib
FORMS =
HEADERS = Tasky.h
SOURCES = Tasky.cpp
PRECOMPILED_HEADER = Precompiled.h
TARGET = Tasky
#VERSION = 0.2

CONFIG(debug, debug|release):DESTDIR = ../../debug/plugins/$$TARGET
CONFIG(release, debug|release):DESTDIR = ../../release/plugins/$$TARGET

win32 {
    QT += winextras
#    CONFIG -= embed_manifest_dll
    LIBS += psapi.lib \
            shell32.lib \
            user32.lib \
            Gdi32.lib \
            comctl32.lib \
            $$DESTDIR/../../Launchy.lib
}
