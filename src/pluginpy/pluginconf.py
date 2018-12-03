
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

redirectOutput()

#import sys
print("sys.prefix:", sys.prefix)
xlib = os.path.join(sys.prefix, 'Lib')
sys.path.insert(0, xlib)
sys.path.insert(0, sys.prefix)
sys.path.insert(0, ".")

#path = os.environ.get('PATH', '')
#print ("env.path(origin):", path)

#os.environ['PATH'] = path + os.pathsep + sys.prefix #+ os.pathsep

print ("sys.path1:", sys.path)
print ("env.path1:", os.environ.get('PATH', ''))

try:
    import site
    site.main()
except Exception:
    print("fail to import site")
    pass

print ("sys.path2:", sys.path)
print ("env.path2:", os.environ.get('PATH', ''))

os.chdir(sys.prefix)

"""
def setSettingsObject():
    # Set the launchy.settings object
    try:
        # Based on http://lists.kde.org/?l=pykde&m=108947844203156&w=2
        from PyQt5 import QtCore
        from sip import wrapinstance, unwrapinstance
        from PyQt5.QtWidgets import QApplication, QWidget
        print ("QApplication:", QApplication)
        print ("QWidget:", QWidget)
        print ("QtCore:", QtCore)
        print ("launchy:", dir(launchy))
        launchy.settings =  wrapinstance(launchy.__settings, QtCore.QSettings)
        print ( launchy.settings )
    except (ImportError, inst):
        print ( sys.stderr, inst)
    except NameError:
        print (sys.stderr, "Could not find __settings object")
    except (Exception, inst):
        print ( sys.stderr, inst)

setSettingsObject()


from PyQt5.QtWidgets import (QWidget, QToolTip,
    QPushButton, QApplication)
from PyQt5.QtGui import QFont

class Example(QWidget):
    def __init__(self):
        super().__init__()
        self.initUI()

    def initUI(self):
        QToolTip.setFont(QFont('SansSerif', 10))

        self.setToolTip('This is a <b>QWidget</b> widget')

        btn = QPushButton('Button', self)
        btn.setToolTip('This is a <b>QPushButton</b> widget')
        btn.resize(btn.sizeHint())
        btn.move(50, 50)

        self.setGeometry(300, 300, 300, 200)
        self.setWindowTitle('Tooltips')
        self.show()
ex = Example()
"""
