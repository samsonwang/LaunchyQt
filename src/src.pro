TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS = LaunchyLib \
          PluginPy \
          Launchy

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
