
def initPipPackage():
    import sys, os
    print("pluginconf, initPipPackage, sys.prefix:", sys.prefix)
    xlib = os.path.join(sys.prefix, 'Lib')
    sys.path.insert(0, xlib)
    sys.path.insert(0, sys.prefix)
    sys.path.insert(0, ".")

    path = os.environ.get('PATH', '')
    #print ("env.path(origin):", path)
    os.environ['PATH'] = path + os.pathsep + sys.prefix + os.pathsep

    print ("pluginconf, initPipPackage, sys.path:", sys.path)
    print ("pluginconf, initPipPackage, env.path:", os.environ.get('PATH', ''))

    import site
    site.main()

    os.chdir(sys.prefix)


def testWidget():
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

# the entrance of pluginconf
def loadConf():
    initPipPackage()
