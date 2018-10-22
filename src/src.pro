TEMPLATE = app
unix:!macx:TARGET = launchy
win32:TARGET = Launchy
macx:TARGET = Launchy
CONFIG += debug_and_release
PRECOMPILED_HEADER = precompiled.h

# CONFIG += qt release
INCLUDEPATH += ../common
QT += network widgets
SOURCES = main.cpp \
    LaunchyWidget.cpp \
    globals.cpp \
    options.cpp \
    catalog.cpp \
    catalog_builder.cpp \
    plugin_handler.cpp \
    icon_delegate.cpp \
    plugin_interface.cpp \
    catalog_types.cpp \
    icon_extractor.cpp \
    FileBrowserDelegate.cpp \
    FileBrowser.cpp \
    DropListWidget.cpp \
    Fader.cpp \
    CharListWidget.cpp \
    CharLineEdit.cpp \
    CommandHistory.cpp \
    InputDataList.cpp \
    FileSearch.cpp \
    AnimationLabel.cpp \
    SettingsManager.cpp \
    QHotkeyP.cpp \
    QHotkey.cpp
HEADERS = platform_base.h \
    globals.h \
    LaunchyWidget.h \
    catalog.h \
    catalog_builder.h \
    plugin_interface.h \
    plugin_handler.h \
    options.h \
    catalog_types.h \
    icon_delegate.h \
    icon_extractor.h \
    FileBrowserDelegate.h \
    FileBrowser.h \
    DropListWidget.h \
    CharListWidget.h \
    CharLineEdit.h \
    Fader.h \
    precompiled.h \
    CommandHistory.h \
    InputDataList.h \
    FileSearch.h \
    AnimationLabel.h \
    SettingsManager.h \
    QHotkeyP.h \
    QHotkey.h
FORMS = options.ui
unix:!macx {
    QT += x11extras
    ICON = Launchy.ico
    SOURCES += linux/platform_unix.cpp \
               linux/platform_unix_util.cpp \
               linux/QHotkeyX11.cpp
    HEADERS += linux/platform_unix.h \
               linux/platform_unix_util.h
    PREFIX = /usr
    LIBS += -lxcb
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
    SOURCES += win/platform_win.cpp \
               win/WinIconProvider.cpp \
               win/minidump.cpp
    HEADERS += win/WinIconProvider.h \
               win/platform_win.h \
               win/platform_win_util.h \
               win/minidump.h
    CONFIG  += embed_manifest_exe
    RC_FILE  = win/launchy.rc
       LIBS += shell32.lib \
               user32.lib \
               gdi32.lib \
               ole32.lib \
               comctl32.lib \
               advapi32.lib \
               userenv.lib \
               netapi32.lib
    DEFINES  = VC_EXTRALEAN \
               WIN32 \
               _UNICODE \
               UNICODE
    if(!debug_and_release|build_pass) {
        CONFIG(debug, debug|release):DESTDIR = ../debug/
        CONFIG(release, debug|release):DESTDIR = ../release/
    }
    QMAKE_CXXFLAGS_RELEASE += /Zi
    QMAKE_LFLAGS_RELEASE += /DEBUG
}
macx {
    ICON = ../misc/Launchy_Icon/launchy_icon_mac.icns
    SOURCES += mac/platform_mac.cpp \
               mac/platform_mac_hotkey.cpp
    HEADERS += mac/platform_mac.h \
               mac/platform_mac_hotkey.h \
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
TRANSLATIONS = \
    ../translations/launchy_zh_CN.ts \
    ../translations/launchy_zh_TW.ts \
    ../translations/launchy_fr.ts \
    ../translations/launchy_nl.ts \
    ../translations/launchy_zh.ts \
    ../translations/launchy_es.ts \
    ../translations/launchy_de.ts \
    ../translations/launchy_ja.ts \
    ../translations/launchy_rus.ts
OBJECTS_DIR = build
MOC_DIR = build
RESOURCES += launchy.qrc
