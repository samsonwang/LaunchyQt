
import sys, os
import launchy

def redirectOutput():
    # If I write code such as sys.stdout = open('stdout.txt', 'w'),
    # I have to flush the file after writing to it (using "print" for example).
    # The following class saves the need to flush the file each time.
    class FlushedFile:
        """ Provides an output file that is immediately flushed after write """
        def __init__(self, filepath):
            try:
                self.terminal = sys.stdout
                self.file = open(filepath, 'w')
            except Exception:
                self.file = None

        def write(self, str):
            if self.file == None:
                return
            self.file.write(str)
            self.file.flush()

        def __getattr__(self, attr):
            try:
                self.file.flush()
                return getattr(self.terminal, attr)
            except Exception as err:
                print("launchy_util, FlashedFile.__getattr__,", err)

    # Redirect stdout and stderr
    sys.stdout = FlushedFile(os.path.join(launchy.getAppPath(), 'python', 'stdout.log'))
    sys.stderr = FlushedFile(os.path.join(launchy.getAppPath(), 'python', 'stderr.log'))
    print("launchy_util, redirect output finished")


def setSettingsObject():
    print("launchy_util, setSettingsObject called")
    # Set the launchy.settings object
    try:
        # Based on http://lists.kde.org/?l=pykde&m=108947844203156&w=2
        from PyQt5 import QtCore
        from sip import wrapinstance
        print("launchy:", dir(launchy))
        launchy.settings = wrapinstance(launchy.__settings, QtCore.QSettings)
        print("launchy.settings:", launchy.settings)
    except ImportError as err:
        print("launchy_util, setSettingsObject, ImportError,", err)
    except NameError:
        print("launchy_util, setSettingsObject, NameError, Could not find __settings object")
    except Exception as err:
        print("launchy_util, setSettingsObject, Exception,", err)


def loadPluginConf():
    try:
        from pluginconf import loadConf
        loadConf()
    except Exception as err:
        print("launchy_util, loadPluginconf,", err)


def launchy_util():
    redirectOutput()
    loadPluginConf()
    print("launchy_util, sys.path:", sys.path)
    print("launchy_util, env.path:", os.environ.get('PATH', ''))

try:
    launchy_util()
except Exception as err:
    print("launchy_util,", err)
