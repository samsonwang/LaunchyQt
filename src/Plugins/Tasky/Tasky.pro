TEMPLATE = lib

TARGET = Tasky

CONFIG += plugin \
          debug_and_release

SOURCES = Tasky.cpp

HEADERS = Tasky.h

#FORMS =

CONFIG += precompile_header
PRECOMPILED_HEADER = Precompiled.h

INCLUDEPATH += ../../

#VERSION = 0.2

CONFIG(debug, debug|release):DESTDIR = ../../debug/plugins/$$TARGET
CONFIG(release, debug|release):DESTDIR = ../../release/plugins/$$TARGET

# copy resources to dest dir
CONFIG += file_copies
COPIES = extra_target
extra_target.files = $$files(*.png)
extra_target.path = $$DESTDIR

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
