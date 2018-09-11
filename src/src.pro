TEMPLATE = app
unix:!macx:TARGET = launchy
win32:TARGET = Launchy
macx:TARGET = Launchy
CONFIG += debug_and_release
PRECOMPILED_HEADER = precompiled.h

# CONFIG += qt release
INCLUDEPATH += ../common
QT += network
SOURCES = main.cpp \
    globals.cpp \
    options.cpp \
    catalog.cpp \
    catalog_builder.cpp \
    plugin_handler.cpp \
    platform_base_hotkey.cpp \
    icon_delegate.cpp \
    plugin_interface.cpp \
    catalog_types.cpp \
    icon_extractor.cpp \
    ../common/FileBrowserDelegate.cpp \
    ../common/FileBrowser.cpp \
    ../common/DropListWidget.cpp \
    Fader.cpp \
    CharListWidget.cpp \
    CharLineEdit.cpp \
    CommandHistory.cpp \
    InputDataList.cpp \
    FileSearch.cpp \
    AnimationLabel.cpp \
	SettingsManager.cpp
HEADERS = platform_base.h \
    globals.h \
    main.h \
    catalog.h \
    catalog_builder.h \
    plugin_interface.h \
    plugin_handler.h \
    options.h \
    catalog_types.h \
    icon_delegate.h \
    icon_extractor.h \
    ../common/FileBrowserDelegate.h \
    ../common/FileBrowser.h \
    ../common/DropListWidget.h \
    CharListWidget.h \
    CharLineEdit.h \
    Fader.h \
    precompiled.h \
    CommandHistory.h \
    InputDataList.h \
    FileSearch.h \
    AnimationLabel.h \
	SettingsManager.h
FORMS = options.ui
unix:!macx { 
    ICON = Launchy.ico
    SOURCES += ../platforms/unix/platform_unix.cpp \
        ../platforms/unix/platform_unix_util.cpp \
        ../platforms/unix/platform_x11_hotkey.cpp
    HEADERS += ../platforms/unix/platform_unix.h \
        ../platforms/unix/platform_unix_util.h \
        ../platforms/unix/platform_x11_hotkey.h \
        platform_base_hotkey.h \
        platform_base_hottrigger.h
    PREFIX = /usr
    DEFINES += SKINS_PATH=\\\"$$PREFIX/share/launchy/skins/\\\" \
        PLUGINS_PATH=\\\"$$PREFIX/lib/launchy/plugins/\\\" \
        PLATFORMS_PATH=\\\"$$PREFIX/lib/launchy/\\\"
    if(!debug_and_release|build_pass) { 
        CONFIG(debug, debug|release):DESTDIR = ../debug/
        CONFIG(release, debug|release):DESTDIR = ../release/
    }
    SOURCES += 
    target.path = $$PREFIX/bin/
    skins.path = $$PREFIX/share/launchy/skins/
    skins.files = ../skins/*
    icon.path = $$PREFIX/share/pixmaps
    icon.files = ../misc/Launchy_Icon/launchy_icon.png
    desktop.path = $$PREFIX/share/applications/
    desktop.files = ../linux/launchy.desktop
    INSTALLS += target \
        skins \
        icon \
        desktop
}
win32 { 
    ICON = Launchy.ico
    if(!debug_and_release|build_pass):CONFIG(debug, debug|release):CONFIG += console
    SOURCES += ../platforms/win/platform_win.cpp \
        ../platforms/win/platform_win_hotkey.cpp \
        ../platforms/win/platform_win_util.cpp \
        ../platforms/win/WinIconProvider.cpp \
        ../platforms/win/minidump.cpp
    HEADERS += ../platforms/win/WinIconProvider.h \
        platform_base_hotkey.h \
        platform_base_hottrigger.h \
        ../platforms/win/platform_win.h \
        ../platforms/win/platform_win_util.h \
        ../platforms/win/minidump.h
    CONFIG += embed_manifest_exe
    INCLUDEPATH += c:/boost/
    RC_FILE = ../win/launchy.rc
	LIBS += shell32.lib \
		user32.lib \
		gdi32.lib \
		ole32.lib \
		comctl32.lib \
		advapi32.lib \
		userenv.lib \
        netapi32.lib
    DEFINES = VC_EXTRALEAN \
        WIN32 \
        _UNICODE \
        UNICODE \
        WINVER=0x0600 \
        _WIN32_WINNT=0x0600 \
        _WIN32_WINDOWS=0x0600 \
        _WIN32_IE=0x0700
    if(!debug_and_release|build_pass) { 
        CONFIG(debug, debug|release):DESTDIR = ../debug/
        CONFIG(release, debug|release):DESTDIR = ../release/
    }
    QMAKE_CXXFLAGS_RELEASE += /Zi
    QMAKE_LFLAGS_RELEASE += /DEBUG
}
macx { 
    ICON = ../misc/Launchy_Icon/launchy_icon_mac.icns
    SOURCES += ../platforms/mac/platform_mac.cpp \
        ../platforms/mac/platform_mac_hotkey.cpp
    HEADERS += ../platforms/mac/platform_mac.h \
        ../platforms/mac/platform_mac_hotkey.h \
        platform_base_hotkey.h \
        platform_base_hottrigger.h
    if(!debug_and_release|build_pass) { 
        CONFIG(debug, debug|release):DESTDIR = ../debug/
        CONFIG(release, debug|release):DESTDIR = ../release/
    }
    INCLUDEPATH += /opt/local/include/
    LIBS += -framework \
        Carbon
    CONFIG(debug, debug|release):skins.path = ../debug/Launchy.app/Contents/Resources/skins/
    CONFIG(release, debug|release):skins.path = ../release/Launchy.app/Contents/Resources/skins/
    skins.files = 
    skins.extra = rsync -arvz ../skins/   ../release/Launchy.app/Contents/Resources/skins/   --exclude=\".svn\"
    CONFIG(debug, debug|release):translations.path = ../debug/Launchy.app/Contents/MacOS/tr/
    CONFIG(release, debug|release):translations.path = ../release/Launchy.app/Contents/MacOS/tr/
    translations.files = ../translations/*.qm
    translations.extra = lupdate \
        src.pro \
        ; \
        lrelease \
        src.pro
    dmg.path = ../release/
    dmg.files = 
    dmg.extra = cd \
        ../mac \
        ; \
        bash \
        deploy; \
        cd \
        ../src
    INSTALLS += skins \
        translations \
        dmg
}
TRANSLATIONS = ../translations/launchy_fr.ts \
    ../translations/launchy_nl.ts \
    ../translations/launchy_zh.ts \
    ../translations/launchy_es.ts \
    ../translations/launchy_de.ts \
    ../translations/launchy_ja.ts \
	../translations/launchy_zh_TW.ts \
	../translations/launchy_rus.ts
OBJECTS_DIR = build
MOC_DIR = build
RESOURCES += launchy.qrc
