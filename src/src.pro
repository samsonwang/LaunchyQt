TEMPLATE = subdirs

SUBDIRS = Launchy \
          LaunchyLib \
          PluginPy

win32 {
SUBDIRS += Plugins/Verby \
           Plugins/Tasky \
           Plugins/Runner \
           Plugins/UWPApp

#           plugins/calcy \
#           plugins/controly \
#           plugins/weby \
#           plugins/gcalc \

}
