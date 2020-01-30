# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'websearch.ui'
#
# Created by: PyQt5 UI code generator 5.11.2
#
# WARNING! All changes made in this file will be lost!

from PyQt5 import QtCore, QtGui, QtWidgets

class Ui_WebSearchWidget(object):
    def setupUi(self, WebSearchWidget):
        WebSearchWidget.setObjectName("WebSearchWidget")
        WebSearchWidget.resize(647, 360)
        self.verticalLayout = QtWidgets.QVBoxLayout(WebSearchWidget)
        self.verticalLayout.setObjectName("verticalLayout")
        self.entriesTable = QtWidgets.QTableWidget(WebSearchWidget)
        sizePolicy = QtWidgets.QSizePolicy(QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.entriesTable.sizePolicy().hasHeightForWidth())
        self.entriesTable.setSizePolicy(sizePolicy)
        self.entriesTable.setObjectName("entriesTable")
        self.entriesTable.setColumnCount(3)
        self.entriesTable.setRowCount(0)
        item = QtWidgets.QTableWidgetItem()
        self.entriesTable.setHorizontalHeaderItem(0, item)
        item = QtWidgets.QTableWidgetItem()
        self.entriesTable.setHorizontalHeaderItem(1, item)
        item = QtWidgets.QTableWidgetItem()
        self.entriesTable.setHorizontalHeaderItem(2, item)
        self.entriesTable.horizontalHeader().setStretchLastSection(True)
        self.entriesTable.verticalHeader().setStretchLastSection(False)
        self.verticalLayout.addWidget(self.entriesTable)
        self.horizontalLayout = QtWidgets.QHBoxLayout()
        self.horizontalLayout.setObjectName("horizontalLayout")
        spacerItem = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem)
        self.addEntryButton = QtWidgets.QPushButton(WebSearchWidget)
        self.addEntryButton.setObjectName("addEntryButton")
        self.horizontalLayout.addWidget(self.addEntryButton)
        spacerItem1 = QtWidgets.QSpacerItem(58, 17, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem1)
        self.removeEntryButton = QtWidgets.QPushButton(WebSearchWidget)
        self.removeEntryButton.setObjectName("removeEntryButton")
        self.horizontalLayout.addWidget(self.removeEntryButton)
        spacerItem2 = QtWidgets.QSpacerItem(40, 20, QtWidgets.QSizePolicy.Expanding, QtWidgets.QSizePolicy.Minimum)
        self.horizontalLayout.addItem(spacerItem2)
        self.verticalLayout.addLayout(self.horizontalLayout)

        self.retranslateUi(WebSearchWidget)
        self.addEntryButton.clicked.connect(WebSearchWidget.addEntry_clicked)
        self.removeEntryButton.clicked.connect(WebSearchWidget.removeEntry_clicked)
        QtCore.QMetaObject.connectSlotsByName(WebSearchWidget)

    def retranslateUi(self, WebSearchWidget):
        _translate = QtCore.QCoreApplication.translate
        WebSearchWidget.setWindowTitle(_translate("WebSearchWidget", "WebSearchPy - Search items from web"))
        item = self.entriesTable.horizontalHeaderItem(0)
        item.setText(_translate("WebSearchWidget", "Key"))
        item = self.entriesTable.horizontalHeaderItem(1)
        item.setText(_translate("WebSearchWidget", "Name"))
        item = self.entriesTable.horizontalHeaderItem(2)
        item.setText(_translate("WebSearchWidget", "Url"))
        self.addEntryButton.setToolTip(_translate("WebSearchWidget", "Add a new entry"))
        self.addEntryButton.setText(_translate("WebSearchWidget", "+"))
        self.removeEntryButton.setToolTip(_translate("WebSearchWidget", "Remove the selected entry"))
        self.removeEntryButton.setText(_translate("WebSearchWidget", "-"))

