# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'websearch.ui',
# licensing of 'websearch.ui' applies.
#
# Created: Sun Oct  4 11:32:04 2026
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
        self.fetchFaviconCheckBox = QtWidgets.QCheckBox(WebSearchWidget)
        self.fetchFaviconCheckBox.setChecked(True)
        self.fetchFaviconCheckBox.setObjectName("fetchFaviconCheckBox")
        self.horizontalLayout.addWidget(self.fetchFaviconCheckBox)
        self.refreshIconButton = QtWidgets.QPushButton(WebSearchWidget)
        self.refreshIconButton.setObjectName("refreshIconButton")
        self.horizontalLayout.addWidget(self.refreshIconButton)
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
        QtCore.QObject.connect(self.refreshIconButton, QtCore.SIGNAL("clicked()"), WebSearchWidget.refreshIcon_clicked)
        QtCore.QMetaObject.connectSlotsByName(WebSearchWidget)

    def retranslateUi(self, WebSearchWidget):
        WebSearchWidget.setWindowTitle(QtWidgets.QApplication.translate("WebSearchWidget", "WebSearchPy - Search items from web", None, -1))
        self.entriesTable.horizontalHeaderItem(0).setText(QtWidgets.QApplication.translate("WebSearchWidget", "Key", None, -1))
        self.entriesTable.horizontalHeaderItem(1).setText(QtWidgets.QApplication.translate("WebSearchWidget", "Name", None, -1))
        self.entriesTable.horizontalHeaderItem(2).setText(QtWidgets.QApplication.translate("WebSearchWidget", "Url", None, -1))
        self.fetchFaviconCheckBox.setToolTip(QtWidgets.QApplication.translate("WebSearchWidget", "Download the icon of every site once and show it in the results", None, -1))
        self.fetchFaviconCheckBox.setText(QtWidgets.QApplication.translate("WebSearchWidget", "Fetch site icons", None, -1))
        self.refreshIconButton.setToolTip(QtWidgets.QApplication.translate("WebSearchWidget", "Drop the downloaded icons and fetch them again", None, -1))
        self.refreshIconButton.setText(QtWidgets.QApplication.translate("WebSearchWidget", "Refresh icons", None, -1))
        self.addEntryButton.setToolTip(QtWidgets.QApplication.translate("WebSearchWidget", "Add a new entry", None, -1))
        self.addEntryButton.setText(QtWidgets.QApplication.translate("WebSearchWidget", "+", None, -1))
        self.removeEntryButton.setToolTip(QtWidgets.QApplication.translate("WebSearchWidget", "Remove the selected entry", None, -1))
        self.removeEntryButton.setText(QtWidgets.QApplication.translate("WebSearchWidget", "-", None, -1))

