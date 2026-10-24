# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'diry.ui',
# licensing of 'diry.ui' applies.
#
# Created: Fri Sep 25 14:09:12 2026
#      by: pyside2-uic  running on PySide2 5.12.6
#
# WARNING! All changes made in this file will be lost!

from PySide2 import QtCore, QtGui, QtWidgets

class Ui_DiryWidget(object):
    def setupUi(self, DiryWidget):
        DiryWidget.setObjectName("DiryWidget")
        DiryWidget.resize(375, 360)
        self.verticalLayout = QtWidgets.QVBoxLayout(DiryWidget)
        self.verticalLayout.setObjectName("verticalLayout")
        self.entriesTable = QtWidgets.QTableWidget(DiryWidget)
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.entriesTable.sizePolicy().hasHeightForWidth())
        self.entriesTable.setSizePolicy(sizePolicy)
        self.entriesTable.setObjectName("entriesTable")
        self.entriesTable.setColumnCount(2)
        self.entriesTable.setRowCount(0)
        item = QtWidgets.QTableWidgetItem()
        self.entriesTable.setHorizontalHeaderItem(0, item)
        item = QtWidgets.QTableWidgetItem()
        self.entriesTable.setHorizontalHeaderItem(1, item)
        self.entriesTable.horizontalHeader().setStretchLastSection(True)
        self.entriesTable.verticalHeader().setStretchLastSection(False)
        self.verticalLayout.addWidget(self.entriesTable)
        self.horizontalLayout = QtWidgets.QHBoxLayout()
        self.horizontalLayout.setObjectName("horizontalLayout")
        spacerItem = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.addEntryButton = QtWidgets.QPushButton(DiryWidget)
        self.addEntryButton.setObjectName("addEntryButton")
        self.horizontalLayout.addWidget(self.addEntryButton)
        spacerItem1 = QtWidgets.QSpacerItem(58, 17, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem1)
        self.removeEntryButton = QtWidgets.QPushButton(DiryWidget)
        self.removeEntryButton.setObjectName("removeEntryButton")
        self.horizontalLayout.addWidget(self.removeEntryButton)
        spacerItem2 = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem2)
        self.verticalLayout.addLayout(self.horizontalLayout)

        self.retranslateUi(DiryWidget)
        QtCore.QObject.connect(self.addEntryButton, QtCore.SIGNAL("clicked()"), DiryWidget.addEntry_clicked)
        QtCore.QObject.connect(self.removeEntryButton, QtCore.SIGNAL("clicked()"), DiryWidget.removeEntry_clicked)
        QtCore.QMetaObject.connectSlotsByName(DiryWidget)

    def retranslateUi(self, DiryWidget):
        DiryWidget.setWindowTitle(QtWidgets.QApplication.translate("DiryWidget", "DiryPy - Launch items from directories", None, -1))
        self.entriesTable.horizontalHeaderItem(0).setText(QtWidgets.QApplication.translate("DiryWidget", "Name", None, -1))
        self.entriesTable.horizontalHeaderItem(1).setText(QtWidgets.QApplication.translate("DiryWidget", "Path", None, -1))
        self.addEntryButton.setToolTip(QtWidgets.QApplication.translate("DiryWidget", "Add a new entry", None, -1))
        self.addEntryButton.setText(QtWidgets.QApplication.translate("DiryWidget", "+", None, -1))
        self.removeEntryButton.setToolTip(QtWidgets.QApplication.translate("DiryWidget", "Remove the selected entry", None, -1))
        self.removeEntryButton.setText(QtWidgets.QApplication.translate("DiryWidget", "-", None, -1))

