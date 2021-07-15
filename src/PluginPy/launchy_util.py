
import sys, os

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
    try:
        import launchy
        sys.stdout = FlushedFile(os.path.join(launchy.getAppTempPath(), 'py_stdout.log'))
        sys.stderr = FlushedFile(os.path.join(launchy.getAppTempPath(), 'py_stderr.log'))
    except:
        sys.stdout = FlushedFile('python/py_stdout.log')
        sys.stderr = FlushedFile('python/py_stderr.log')
    print("launchy_util, redirect output finished")


def setSettingsObject():
    print("launchy_util, setSettingsObject called")
    # Set the launchy.settings object
    try:
        # Based on http://lists.kde.org/?l=pykde&m=108947844203156&w=2
        from PyQt5 import QtCore
        from sip import wrapinstance
        import launchy
        print("launchy:", dir(launchy))
        launchy.settings = wrapinstance(launchy.__settings, QtCore.QSettings)
        print("launchy.settings:", launchy.settings)
    except ImportError as err:
        print("launchy_util, setSettingsObject, ImportError,", err)
    except NameError:
        print("launchy_util, setSettingsObject, NameError, Could not find __settings object")
    except Exception as err:
        print("launchy_util, setSettingsObject, Exception,", err)


def initPipPackage():
    import sys, os
    print("launchy_util, initPipPackage, sys.prefix:", sys.prefix)

    xlib = os.path.join(sys.prefix, 'Lib')

    if os.path.exists(xlib):
        print("launchy_util, initPipPackage, Lib path found, init site")
        sys.path.insert(0, xlib)
        sys.path.insert(0, sys.prefix)
        sys.path.insert(0, ".")

        path = os.environ.get('PATH', '')
        #print ("env.path(origin):", path)
        os.environ['PATH'] = path + os.pathsep + sys.prefix + os.pathsep

        #print("launchy_util, initPipPackage, sys.path:", sys.path)
        #print("launchy_util, initPipPackage, env.path:", os.environ.get('PATH', ''))

        import site
        site.main()
        os.chdir(sys.prefix)
    else:
        print("launchy_util, initPipPackage, Lib path not found, skip init site")

def loadPyConf():
    try:
        print("launchy_util, loadPyConf, begin")
        import launchy_pyconf
        print("launchy_util, loadPyConf, end")
    except Exception as err:
        print("launchy_util, loadPyConf, catched exception:", err)

try:
    redirectOutput()
    initPipPackage()
    #loadPyConf()
    print("launchy_util, sys.path:", sys.path)
    print("launchy_util, env.path:", os.environ.get('PATH', ''))
except Exception as err:
    print("launchy_util, catched exception", err)
