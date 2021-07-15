TEMPLATE = lib

TARGET = UWPApp
QT += gui widgets
CONFIG += plugin debug_and_release

INCLUDEPATH += ../../

#CONFIG += precompile_header
#PRECOMPILED_HEADER = Precompiled.h

FORMS = 

HEADERS = UWPApp.h
          #Package.h \
          #Application.h

SOURCES = UWPApp.cpp
          #Package.cpp \
          #Application.cpp

DISTFILES += uwpapp.json

CONFIG(debug, debug|release):DESTDIR = ../../debug/plugins/$$TARGET
CONFIG(release, debug|release):DESTDIR = ../../release/plugins/$$TARGET

win32 {
 	CONFIG -= embed_manifest_dll

        LIBS += user32.lib \
                shell32.lib \
                Propsys.lib \
                $$DESTDIR/../../Launchy.lib
#	LIBS += shell32.lib
#	LIBS += user32.lib
#	LIBS += Gdi32.lib
#	LIBS += comctl32.lib
#	LIBS += shlwapi.lib
#	LIBS += dwmapi.lib
#	LIBS += xmllite.lib
        #QMAKE_CXXFLAGS += /ZW
        #/AI"$(VCInstallDir)vcpackages;$(UniversalCRTSdkDir)UnionMetadata"
}


