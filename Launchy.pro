TEMPLATE = subdirs
SUBDIRS = src/lib \
          src/pluginpy \
          src


win32 {
SUBDIRS += plugins/verby \
           plugins/tasky

#           plugins/calcy \
#           plugins/controly \
#           plugins/weby \
#           plugins/runner \
#           plugins/gcalc \

}
