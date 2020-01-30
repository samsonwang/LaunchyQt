
from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtCore import QVariant
from PyQt5.QtWidgets import QApplication, QWidget, QDialog, QFileDialog

import launchy

from .ui_pydiry import *
from .ui_pydiry_new_entry import *

class NewDirectoryEntryDialog(QDialog):
    def __init__(self, parent=None):
        QtWidgets.QDialog.__init__(self, parent)
        self.ui = Ui_NewDirectoryEntryDialog()
        self.ui.setupUi(self)
        self.isValid = False

    def accept(self):
        try:
            self.directory = self.ui.directoryLineEdit.text()
            self.name = self.ui.nameLineEdit.text()

            if not self.directory:
                self.reject()
            elif not self.name:
                QtWidgets.QMessageBox.information(self,
                                                  "Name is empty",
                                                  "Please set a name for the directory")
                return
            else:
                self.isValid = True
                QtWidgets.QDialog.accept(self)
        except Exception as err:
            print("PyDiryPy, exception catched on new directory dialog accept,", err)

    def reject(self):
        QtWidgets.QDialog.reject(self)

    def selectDirectory_clicked(self):
        try:
            dir = QtWidgets.QFileDialog.getExistingDirectory(self, "open directory",
                                                             ".")
                                                             #self.ui.directoryLineEdit.text())
        except Exception as err:
            print("PyDiryPy, exception catched on selectDirectory_clicked,", err)

        nativeDir = QtCore.QDir.toNativeSeparators(dir)
        self.ui.directoryLineEdit.setText(nativeDir)


class PyDiryUi(QWidget):
    def __init__(self, parent=None,setting_dir=None):
        QtWidgets.QWidget.__init__(self, parent)
        self.setting_dir = setting_dir
        self.cell_changed = False
        self.ui = Ui_PyDiryWidget()
        self.ui.setupUi(self)
        self.__loadSettings()
        self.ui.entriesTable.cellChanged['int','int'].connect(self.entriesTable_cellChanged)

    def addEntry_clicked(self):
        newEntryDialog = NewDirectoryEntryDialog(self)
        newEntryDialog.exec_()
        if not newEntryDialog.isValid:
            return

        table = self.ui.entriesTable
        lastItemCount = table.rowCount()
        table.insertRow(table.rowCount())
        table.setCurrentCell(lastItemCount, 0)
        nameItem = QtWidgets.QTableWidgetItem( newEntryDialog.name )
        pathItem = QtWidgets.QTableWidgetItem( newEntryDialog.directory )
        table.setItem(lastItemCount, 0, nameItem)
        table.setItem(lastItemCount, 1, pathItem)

    def removeEntry_clicked(self):
        currentRow = self.ui.entriesTable.currentRow()
        if currentRow != -1:
            self.ui.entriesTable.removeRow(currentRow)

    def entriesTable_cellChanged(self, row, col):
        print("PyDiryUi, entriesTable_cellChanged", row, col)
        self.cell_changed = True

    def writeSettings(self):
        settings = launchy.settings
        table = self.ui.entriesTable

        # Remove all empty rows
        itemsToRemove = []
        for i in range(0, table.rowCount()):
            nameItem = table.item(i,0)
            pathItem = table.item(i,1)
            if nameItem == None or pathItem == None:
                itemsToRemove.append(i)
            elif nameItem.text() == "" or pathItem == "":
                itemsToRemove.append(i)

        for item in itemsToRemove:
            table.removeRow(i)

        # Add all rows to the dirs array
        settings.remove(self.setting_dir)
        settings.beginWriteArray(self.setting_dir)
        for i in range(0, table.rowCount()):
            settings.setArrayIndex(i)
            settings.setValue("name", QVariant(table.item(i,0).text()))
            settings.setValue("path", QVariant(table.item(i,1).text()))
        settings.endArray()

        if self.cell_changed == True:
            launchy.setNeedRebuildCatalog()


    def __loadSettings(self):
        settings = launchy.settings
        table = self.ui.entriesTable
        size = settings.beginReadArray(self.setting_dir)
        table.setRowCount(size)
        for i in range(0, size):
            settings.setArrayIndex(i)
            nameItem = QtWidgets.QTableWidgetItem( settings.value("name") )
            pathItem = QtWidgets.QTableWidgetItem( settings.value("path") )
            table.setItem(i, 0, nameItem)
            table.setItem(i, 1, pathItem)
        settings.endArray()
