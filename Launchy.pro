TEMPLATE = subdirs
SUBDIRS = plugins/calcy \
          plugins/gcalc \
          plugins/runner \
          plugins/weby \
          plugins/verby \
          src
          
win32 {
	SUBDIRS += plugins/controly
}
