# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'pydiry_new_entry.ui'
#
# Created by: PyQt5 UI code generator 5.11.2
#
# WARNING! All changes made in this file will be lost!

from PyQt5 import QtCore, QtGui, QtWidgets

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
        self.buttonBox.accepted.connect(NewDirectoryEntryDialog.accept)
        self.buttonBox.rejected.connect(NewDirectoryEntryDialog.reject)
        self.selectDirectoryButton.clicked.connect(NewDirectoryEntryDialog.selectDirectory_clicked)
        QtCore.QMetaObject.connectSlotsByName(NewDirectoryEntryDialog)

    def retranslateUi(self, NewDirectoryEntryDialog):
        _translate = QtCore.QCoreApplication.translate
        NewDirectoryEntryDialog.setWindowTitle(_translate("NewDirectoryEntryDialog", "New directory entry"))
        self.label.setText(_translate("NewDirectoryEntryDialog", "Directory:"))
        self.selectDirectoryButton.setText(_translate("NewDirectoryEntryDialog", "..."))
        self.label_2.setText(_translate("NewDirectoryEntryDialog", "Name:"))
        self.label_3.setText(_translate("NewDirectoryEntryDialog", "<b>Tip</b>: You can use envrionement variables, e.g. %USERPROFILE%\\My Documents"))

