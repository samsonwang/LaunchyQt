TEMPLATE = subdirs
SUBDIRS = src/lib \
          src/pluginpy \
          src


win32 {
SUBDIRS += plugins/calcy \
           plugins/verby \
           plugins/tasky

#          plugins/controly \
#           plugins/weby \
#           plugins/runner \
#           plugins/gcalc \
}
