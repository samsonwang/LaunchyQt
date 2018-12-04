
import sys, os
import launchy

print("launchy_test")
print("sys.path4:", sys.path)
print("env.path4:", os.environ.get('PATH', ''))

def setTestWidgetObject():
    print("launchy_test, setTestWidgetObject called")
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
        launchy.testWidget = wrapinstance(launchy.__testWidget, QWidget)
        print(launchy.testWidget)
    except ImportError as err:
        print("ImportError,", err)
    except NameError:
        print("launchy_test, NameError")
    except Exception as err:
        print("Exception,", err)


def initTestWidget():
    from PyQt5 import QtWidgets
    print("launchy_test, initTestwidget called")
    testWidget = launchy.testWidget
    testWidget.label = QtWidgets.QLabel(testWidget)
    testWidget.label.setText("hello world")
    testWidget.show()

try:
    setTestWidgetObject()
    initTestWidget()
except Exception as err:
    print("launchy_test,", err)
