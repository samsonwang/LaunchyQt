
import sys, os
import launchy

print("launchy_util")
print("sys.path3:", sys.path)
print("env.path3:", os.environ.get('PATH', ''))

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
        launchy.settings.setValue("Test/testSetting", 2345)
    except ImportError as err:
        print("ImportError,", err)
    except NameError:
        print("NameError, Could not find __settings object")
    except Exception as err:
        print("Exception,", err)

setSettingsObject()
