
from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtCore import QVariant
from PyQt5.QtWidgets import QApplication, QWidget, QDialog, QFileDialog

from .ui_websearch import *
#from .ui_pydiry_new_entry import *

import launchy

class WebSearchUi(QWidget):
    def __init__(self, parent=None, settingName=None):
        QtWidgets.QWidget.__init__(self, parent)
        self.settingName = settingName
        self.ui = Ui_WebSearchWidget()
        self.ui.setupUi(self)

        settings = launchy.settings
        table = self.ui.entriesTable

        size = settings.beginReadArray(self.settingName)
        if size <= 0:
            print("WebSearchUi, load default setting")
            from .Defaults import defaultSetting as defSet
            table.setRowCount(len(defSet.keys()))
            for i, item in enumerate(defSet):
                print("i:", i, "item:", item)
                keyItem = QtWidgets.QTableWidgetItem( item )
                nameItem = QtWidgets.QTableWidgetItem( defSet[item]['name'] )
                urlItem = QtWidgets.QTableWidgetItem( defSet[item]['url'] )
                table.setItem(i, 0, keyItem)
                table.setItem(i, 1, nameItem)
                table.setItem(i, 2, urlItem)
        else:
            table.setRowCount(size)
            for i in range(0, size):
                settings.setArrayIndex(i);
                keyItem = QtWidgets.QTableWidgetItem( settings.value("key") )
                nameItem = QtWidgets.QTableWidgetItem( settings.value("name") )
                urlItem = QtWidgets.QTableWidgetItem( settings.value("url") )
                table.setItem(i, 0, keyItem)
                table.setItem(i, 1, nameItem)
                table.setItem(i, 2, urlItem)

        settings.endArray()

    def addEntry_clicked(self):
#        newEntryDialog = NewDirectoryEntryDialog(self)
#        newEntryDialog.exec_()
#        if not newEntryDialog.isValid:
#            return
        table = self.ui.entriesTable
        lastItemCount = table.rowCount()
        table.insertRow(table.rowCount())
        table.setCurrentCell(lastItemCount, 0)
#        nameItem = QtWidgets.QTableWidgetItem( newEntryDialog.name )
#        pathItem = QtWidgets.QTableWidgetItem( newEntryDialog.directory )
#        table.setItem(lastItemCount, 0, nameItem)
#        table.setItem(lastItemCount, 1, pathItem)

    def removeEntry_clicked(self):
        currentRow = self.ui.entriesTable.currentRow()
        if currentRow != -1:
            self.ui.entriesTable.removeRow(currentRow)

    def writeSettings(self):
        print("WebSearchUi, writeSettings")
        settings = launchy.settings
        table = self.ui.entriesTable

        # Remove all empty rows
        itemsToRemove = []
        for i in range(0, table.rowCount()):
            keyItem = table.item(i,0)
            nameItem = table.item(i,1)
            urlItem = table.item(i,2)
            if keyItem == None or nameItem == None or urlItem == None:
                itemsToRemove.append(i)
            elif keyItem.text() == "" or nameItem.text() == "" or urlItem == "":
                itemsToRemove.append(i)

        for item in itemsToRemove:
            table.removeRow(i)

        # Add all rows to the dirs array
        settings.remove(self.settingName)
        print("WebSearchUi, settingName", self.settingName)
        settings.beginWriteArray(self.settingName)
        for i in range(0, table.rowCount()):
            settings.setArrayIndex(i)
            settings.setValue("key", QVariant(table.item(i,0).text().lower()))
            settings.setValue("name", QVariant(table.item(i,1).text()))
            settings.setValue("url", QVariant(table.item(i,2).text()))
        settings.endArray()
