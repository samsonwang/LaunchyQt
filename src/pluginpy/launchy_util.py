
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
            except Exception:
                pass
                #print ( sys.stderr, inst)
    # Redirect stdout and stderr
    sys.stdout = FlushedFile(os.path.join(launchy.getAppPath(), 'python', 'stdout.log'))
    sys.stderr = FlushedFile(os.path.join(launchy.getAppPath(), 'python', 'stderr.log'))
    print("pluginpy is up and running")

def setSettingsObject():
    print("launchy_util, setSettingsObject called")
    # Set the launchy.settings object
    try:
        # Based on http://lists.kde.org/?l=pykde&m=108947844203156&w=2
        from PyQt5 import QtCore
        from sip import wrapinstance, unwrapinstance
        from PyQt5.QtWidgets import QApplication, QWidget
        print("QApplication:", QApplication)
        print("QWidget:", QWidget)
        print("QtCore:", QtCore)
        print("launchy:", dir(launchy))
        launchy.settings = wrapinstance(launchy.__settings, QtCore.QSettings)
        print(launchy.settings)
#        launchy.settings.setValue("Test/testSetting", 2345)
    except ImportError as err:
        print("ImportError,", err)
    except NameError:
        print("NameError, Could not find __settings object")
    except Exception as err:
        print("Exception,", err)


def loadPluginConf():
    try:
        import pluginconf
    except Exception as err:
        print("loadPluginconf,", err)

def launchy_util():
    redirectOutput()
    loadPluginConf()
#    setSettingsObject()
    print("launchy_util")
    print("sys.path:", sys.path)
    print("env.path:", os.environ.get('PATH', ''))


try:
    launchy_util()
except Exception as err:
    print("launchy_util,", err)
