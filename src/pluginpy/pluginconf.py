
import sys, os

print("sys.prefix:", sys.prefix)
xlib = os.path.join(sys.prefix, 'Lib')
sys.path.insert(0, xlib)
sys.path.insert(0, sys.prefix)
sys.path.insert(0, ".")

path = os.environ.get('PATH', '')
#print ("env.path(origin):", path)

os.environ['PATH'] = path + os.pathsep + sys.prefix + os.pathsep

print ("sys.path1:", sys.path)
print ("env.path1:", os.environ.get('PATH', ''))

import site
site.main()

print ("sys.path2:", sys.path)
print ("env.path2:", os.environ.get('PATH', ''))

os.chdir(sys.prefix)


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

#ex = Example()
