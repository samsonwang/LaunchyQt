# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'pydiry_new_entry.ui',
# licensing of 'pydiry_new_entry.ui' applies.
#
# Created: Fri Sep 25 14:09:31 2026
#      by: pyside2-uic  running on PySide2 5.12.6
#
# WARNING! All changes made in this file will be lost!

from PySide2 import QtCore, QtGui, QtWidgets

class Ui_NewDirectoryEntryDialog(object):
    def setupUi(self, NewDirectoryEntryDialog):
        NewDirectoryEntryDialog.setObjectName("NewDirectoryEntryDialog")
        NewDirectoryEntryDialog.resize(408, 118)
        NewDirectoryEntryDialog.setSizeGripEnabled(True)
        NewDirectoryEntryDialog.setModal(True)
        self.verticalLayout = QtWidgets.QVBoxLayout(NewDirectoryEntryDialog)
        self.verticalLayout.setObjectName("verticalLayout")
        self.gridLayout = QtWidgets.QGridLayout()
        self.gridLayout.setObjectName("gridLayout")
        self.label = QtWidgets.QLabel(NewDirectoryEntryDialog)
        self.label.setObjectName("label")
        self.gridLayout.addWidget(self.label, 0, 0, 1, 1)
        self.directoryLineEdit = QtWidgets.QLineEdit(NewDirectoryEntryDialog)
        self.directoryLineEdit.setObjectName("directoryLineEdit")
        self.gridLayout.addWidget(self.directoryLineEdit, 0, 1, 1, 1)
        self.selectDirectoryButton = QtWidgets.QToolButton(NewDirectoryEntryDialog)
        self.selectDirectoryButton.setPopupMode(QtWidgets.QToolButton.DelayedPopup)
        self.selectDirectoryButton.setToolButtonStyle(QtCore.Qt.ToolButtonIconOnly)
        self.selectDirectoryButton.setObjectName("selectDirectoryButton")
        self.gridLayout.addWidget(self.selectDirectoryButton, 0, 2, 1, 1)
        self.label_2 = QtWidgets.QLabel(NewDirectoryEntryDialog)
        self.label_2.setObjectName("label_2")
        self.gridLayout.addWidget(self.label_2, 1, 0, 1, 1)
        self.nameLineEdit = QtWidgets.QLineEdit(NewDirectoryEntryDialog)
        self.nameLineEdit.setObjectName("nameLineEdit")
        self.gridLayout.addWidget(self.nameLineEdit, 1, 1, 1, 1)
        self.verticalLayout.addLayout(self.gridLayout)
        self.label_3 = QtWidgets.QLabel(NewDirectoryEntryDialog)
        self.label_3.setObjectName("label_3")
        self.verticalLayout.addWidget(self.label_3)
        self.buttonBox = QtWidgets.QDialogButtonBox(NewDirectoryEntryDialog)
        self.buttonBox.setOrientation(QtCore.Qt.Horizontal)
        self.buttonBox.setStandardButtons(QtWidgets.QDialogButtonBox.Cancel|QtWidgets.QDialogButtonBox.Ok)
        self.buttonBox.setCenterButtons(False)
        self.buttonBox.setObjectName("buttonBox")
        self.verticalLayout.addWidget(self.buttonBox)

        self.retranslateUi(NewDirectoryEntryDialog)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("accepted()"), NewDirectoryEntryDialog.accept)
        QtCore.QObject.connect(self.buttonBox, QtCore.SIGNAL("rejected()"), NewDirectoryEntryDialog.reject)
        QtCore.QObject.connect(self.selectDirectoryButton, QtCore.SIGNAL("clicked()"), NewDirectoryEntryDialog.selectDirectory_clicked)
        QtCore.QMetaObject.connectSlotsByName(NewDirectoryEntryDialog)

    def retranslateUi(self, NewDirectoryEntryDialog):
        NewDirectoryEntryDialog.setWindowTitle(QtWidgets.QApplication.translate("NewDirectoryEntryDialog", "New directory entry", None, -1))
        self.label.setText(QtWidgets.QApplication.translate("NewDirectoryEntryDialog", "Directory:", None, -1))
        self.selectDirectoryButton.setText(QtWidgets.QApplication.translate("NewDirectoryEntryDialog", "...", None, -1))
        self.label_2.setText(QtWidgets.QApplication.translate("NewDirectoryEntryDialog", "Name:", None, -1))
        self.label_3.setText(QtWidgets.QApplication.translate("NewDirectoryEntryDialog", "<b>Tip</b>: You can use envrionement variables, e.g. %USERPROFILE%\\My Documents", None, -1))

