# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'websearch.ui',
# licensing of 'websearch.ui' applies.
#
# Created: Fri Jun 20 14:39:15 2025
#      by: pyside2-uic  running on PySide2 5.12.6
#
# WARNING! All changes made in this file will be lost!

from PySide2 import QtCore, QtGui, QtWidgets

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
        QtCore.QObject.connect(self.addEntryButton, QtCore.SIGNAL("clicked()"), WebSearchWidget.addEntry_clicked)
        QtCore.QObject.connect(self.removeEntryButton, QtCore.SIGNAL("clicked()"), WebSearchWidget.removeEntry_clicked)
        QtCore.QMetaObject.connectSlotsByName(WebSearchWidget)

    def retranslateUi(self, WebSearchWidget):
        WebSearchWidget.setWindowTitle(QtWidgets.QApplication.translate("WebSearchWidget", "WebSearchPy - Search items from web", None, -1))
        self.entriesTable.horizontalHeaderItem(0).setText(QtWidgets.QApplication.translate("WebSearchWidget", "Key", None, -1))
        self.entriesTable.horizontalHeaderItem(1).setText(QtWidgets.QApplication.translate("WebSearchWidget", "Name", None, -1))
        self.entriesTable.horizontalHeaderItem(2).setText(QtWidgets.QApplication.translate("WebSearchWidget", "Url", None, -1))
        self.addEntryButton.setToolTip(QtWidgets.QApplication.translate("WebSearchWidget", "Add a new entry", None, -1))
        self.addEntryButton.setText(QtWidgets.QApplication.translate("WebSearchWidget", "+", None, -1))
        self.removeEntryButton.setToolTip(QtWidgets.QApplication.translate("WebSearchWidget", "Remove the selected entry", None, -1))
        self.removeEntryButton.setText(QtWidgets.QApplication.translate("WebSearchWidget", "-", None, -1))

