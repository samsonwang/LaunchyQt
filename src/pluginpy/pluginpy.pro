TEMPLATE = lib

TARGET = PluginPy

QT += core

CONFIG += debug_and_release

SOURCES += PluginLoader.cpp \
           PluginMgr.cpp \
           PluginWrapper.cpp \
           ExportPy.cpp \
           ExportPyPlugin.cpp \
           ExportPyCatItem.cpp \
           ExportPyInputData.cpp

HEADERS += PluginPyLib.h \
           PluginLoader.h \
           PluginMgr.h \
           PluginWrapper.h \
           ExportPy.h \
           ExportPyPlugin.h \
           ExportPyCatItem.h \
           ExportPyInputData.h

INCLUDEPATH += ../LaunchyLib \
               ../../deps

PRECOMPILED_HEADER = Precompiled.h
CONFIG += precompile_header

DEFINES += PLUGINPY_LIB

CONFIG(debug, debug|release):DESTDIR = ../debug
CONFIG(release, debug|release):DESTDIR = ../release

LIBS += $$DESTDIR/Launchy.lib

# extra target copy to python
CONFIG += file_copies
COPIES = extra_target
extra_target.files = launchy_util.py
extra_target.path = $$DESTDIR/python

win32 {
    INCLUDEPATH += $$(PYTHONPATH)/include
    LIBS += user32.lib \
            shell32.lib \
            $$(PYTHONPATH)/libs/python3.lib \
            $$(PYTHONPATH)/libs/python36.lib


    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$DESTDIR/PluginPy.dll) \
                       $$shell_path($$DESTDIR/python/launchy.pyd)

    #dir_commands = $$QMAKE_MKDIR $$shell_path($$DESTDIR/python)

    #pyd_commands = -$$QMAKE_COPY $$shell_path($$DESTDIR/PluginPy.dll) \
                   $$shell_path($$DESTDIR/python/launchy.pyd)

    #QMAKE_EXTRA_TARGETS += extra_target dir pyd util
    #POST_TARGETDEPS += extra_target
                       #$$QMAKE_MKDIR $$shell_path($$DESTDIR/python) $$escape_expand(\\n\\t) \
                       # $$escape_expand(\\n\\t) \
                       #$$QMAKE_COPY $$shell_path($$PWD/launchy_util.py) \
                       #$$shell_path($$DESTDIR/python)
}

unix:!macx {
    #python3-config
    INCLUDEPATH += /usr/include/python3.6m
    LIBS += -lpython3.6m -lpthread -ldl  -lutil -lm

    PREFIX = /usr
    target.path = $$PREFIX/lib/
    INSTALLS += target
}


