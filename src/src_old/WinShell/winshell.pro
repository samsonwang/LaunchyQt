 TEMPLATE      = lib
 CONFIG       += plugin release
 VPATH 		  += ../../src/
 INCLUDEPATH += ../../src/
 UI_DIR		   = ../../plugins/winshell/
 FORMS		   = dlg.ui
 HEADERS       = plugin_interface.h winshell.h gui.h globals.h
 SOURCES       = plugin_interface.cpp winshell.cpp gui.cpp
 TARGET		   = winshell
 
 win32 {
 	CONFIG -= embed_manifest_dll
	LIBS += shell32.lib
%	LIBS += user32.lib
%	LIBS += Gdi32.lib
%	LIBS += comctl32.lib
}
 
 *:debug {
	DESTDIR = ../../debug/plugins/
 }
 *:release {
	DESTDIR = ../../release/plugins/
	%QMAKE_CXXFLAGS += /Ox /Ob2 /Oi /Oy /GT /GA  /WX
 }

