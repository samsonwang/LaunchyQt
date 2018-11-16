 TEMPLATE      = lib
 CONFIG       += plugin debug_and_release
 FORMS		   =
 HEADERS       = tasky.h
 SOURCES       = tasky.cpp
 TARGET		   = Tasky
 TARGET_EXT	   = .dll
 VERSION	   = 0.2

 win32 {
 	CONFIG -= embed_manifest_dll
 	LIBS += psapi.lib
	LIBS += shell32.lib
	LIBS += user32.lib
	LIBS += Gdi32.lib
%	LIBS += comctl32.lib
}
