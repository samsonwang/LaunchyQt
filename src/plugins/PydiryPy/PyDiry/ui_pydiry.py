# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'pydiry.ui'
#
# Created by: PyQt5 UI code generator 5.11.2
#
# WARNING! All changes made in this file will be lost!

from PyQt5 import QtCore, QtGui, QtWidgets

class Ui_PyDiryWidget(object):
    def setupUi(self, PyDiryWidget):
        PyDiryWidget.setObjectName("PyDiryWidget")
        PyDiryWidget.resize(375, 360)
        self.verticalLayout = QtWidgets.QVBoxLayout(PyDiryWidget)
        self.verticalLayout.setObjectName("verticalLayout")
        self.entriesTable = QtWidgets.QTableWidget(PyDiryWidget)
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
        self.addEntryButton = QtWidgets.QPushButton(PyDiryWidget)
        self.addEntryButton.setObjectName("addEntryButton")
        self.horizontalLayout.addWidget(self.addEntryButton)
        spacerItem1 = QtWidgets.QSpacerItem(58, 17, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem1)
        self.removeEntryButton = QtWidgets.QPushButton(PyDiryWidget)
        self.removeEntryButton.setObjectName("removeEntryButton")
        self.horizontalLayout.addWidget(self.removeEntryButton)
        spacerItem2 = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem2)
        self.verticalLayout.addLayout(self.horizontalLayout)

        self.retranslateUi(PyDiryWidget)
        self.addEntryButton.clicked.connect(PyDiryWidget.addEntry_clicked)
        self.removeEntryButton.clicked.connect(PyDiryWidget.removeEntry_clicked)
        QtCore.QMetaObject.connectSlotsByName(PyDiryWidget)

    def retranslateUi(self, PyDiryWidget):
        _translate = QtCore.QCoreApplication.translate
        PyDiryWidget.setWindowTitle(_translate("PyDiryWidget", "PyDiryPy - Launch items from directories"))
        item = self.entriesTable.horizontalHeaderItem(0)
        item.setText(_translate("PyDiryWidget", "Name"))
        item = self.entriesTable.horizontalHeaderItem(1)
        item.setText(_translate("PyDiryWidget", "Path"))
        self.addEntryButton.setToolTip(_translate("PyDiryWidget", "Add a new entry"))
        self.addEntryButton.setText(_translate("PyDiryWidget", "+"))
        self.removeEntryButton.setToolTip(_translate("PyDiryWidget", "Remove the selected entry"))
        self.removeEntryButton.setText(_translate("PyDiryWidget", "-"))

