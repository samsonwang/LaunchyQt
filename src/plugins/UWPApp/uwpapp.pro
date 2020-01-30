TEMPLATE = lib
CONFIG += plugin debug_and_release
VPATH += ../../src/

# Check for requried environment variables
!exists($$(BOOST_DIR)) {
 error("The BOOST_DIR environment variable is not defined.")
}

INCLUDEPATH += ../../src/
INCLUDEPATH += $$(BOOST_DIR)

TR_EXCLUDE += $$(BOOST_DIR)/*

PRECOMPILED_HEADER = precompiled.h

greaterThan(QT_MAJOR_VERSION, 4): QT += gui widgets

FORMS = 
HEADERS = plugin_interface.h uwpapp.h precompiled.h Package.h Application.h
SOURCES = plugin_interface.cpp uwpapp.cpp Package.cpp Application.cpp
TARGET = uwpapp
 
win32 {
 	CONFIG -= embed_manifest_dll
	LIBS += shell32.lib
	LIBS += user32.lib
	LIBS += Gdi32.lib
	LIBS += comctl32.lib
	LIBS += shlwapi.lib
	LIBS += dwmapi.lib
	LIBS += xmllite.lib
	QMAKE_CXXFLAGS += /ZW /AI"$(VCInstallDir)vcpackages;$(UniversalCRTSdkDir)UnionMetadata"
}

if(!debug_and_release|build_pass):CONFIG(debug, debug|release):DESTDIR = ../../debug/plugins
if(!debug_and_release|build_pass):CONFIG(release, debug|release):DESTDIR = ../../release/plugins

DISTFILES += uwpapp.json
