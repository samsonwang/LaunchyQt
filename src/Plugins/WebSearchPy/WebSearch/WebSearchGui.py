
import logging as log

from PySide6 import QtCore, QtGui, QtWidgets
from PySide6.QtWidgets import QApplication
from PySide6.QtWidgets import QWidget, QTableWidgetItem

from .ui_websearch import *

import launchy

class WebSearchGui(QWidget):
    def __init__(self, parent=None, settingName=None):
        QtWidgets.QWidget.__init__(self, parent)
        self.settingName = settingName
        self.ui = Ui_WebSearchWidget()
        self.ui.setupUi(self)

        settings = launchy.settings
        table = self.ui.entriesTable

        size = settings.beginReadArray(self.settingName)
        if size <= 0:
            log.debug("WebSearchGui, load default setting")
            from .Defaults import defaultSetting as defSet
            table.setRowCount(len(defSet.keys()))
            for i, item in enumerate(defSet):
                log.debug("WebSearchGui, i: %s, item: %s" % (i, item))
                keyItem = QTableWidgetItem( item )
                nameItem = QTableWidgetItem( defSet[item]['name'] )
                urlItem = QTableWidgetItem( defSet[item]['url'] )
                table.setItem(i, 0, keyItem)
                table.setItem(i, 1, nameItem)
                table.setItem(i, 2, urlItem)
        else:
            table.setRowCount(size)
            for i in range(0, size):
                settings.setArrayIndex(i);
                keyItem = QTableWidgetItem( settings.value("key") )
                nameItem = QTableWidgetItem( settings.value("name") )
                urlItem = QTableWidgetItem( settings.value("url") )
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
        log.debug("WebSearchGui, writeSettings")
        settings = launchy.settings
        table = self.ui.entriesTable

        # Remove all empty rows
        itemsToRemove = []
        for i in range(0, table.rowCount()):
            keyItem = table.item(i, 0)
            nameItem = table.item(i, 1)
            urlItem = table.item(i, 2)
            if keyItem == None or nameItem == None or urlItem == None:
                itemsToRemove.append(i)
            elif keyItem.text() == "" or nameItem.text() == "" or urlItem == "":
                itemsToRemove.append(i)

        for item in itemsToRemove:
            table.removeRow(i)

        # Add all rows to the dirs array
        settings.remove(self.settingName)
        log.debug("WebSearchUi, settingName: %s" % self.settingName)
        settings.beginWriteArray(self.settingName)
        for i in range(0, table.rowCount()):
            settings.setArrayIndex(i)
            settings.setValue("key", (table.item(i,0).text().lower()))
            settings.setValue("name", (table.item(i,1).text()))
            settings.setValue("url", (table.item(i,2).text()))
        settings.endArray()
