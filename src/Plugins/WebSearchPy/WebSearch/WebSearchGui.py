
import logging as log

from PySide2 import QtCore, QtGui, QtWidgets
from PySide2.QtWidgets import QApplication
from PySide2.QtWidgets import QWidget, QTableWidgetItem

from .ui_websearch import *
from .FaviconCache import cache as faviconCache, toBool

import launchy

class WebSearchGui(QWidget):
    # The engines and the "fetch site icons" switch share one settings group.
    FETCH_FAVICON_KEY = "fetchFavicon"

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

        enabled = settings.value(self.settingName + "/" + self.FETCH_FAVICON_KEY, True)
        self.ui.fetchFaviconCheckBox.setChecked(toBool(enabled, True))

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

    # Drop every downloaded icon and fetch the ones of the listed sites again.
    def refreshIcon_clicked(self):
        table = self.ui.entriesTable
        urls = []
        for i in range(0, table.rowCount()):
            urlItem = table.item(i, 2)
            if urlItem and urlItem.text():
                urls.append(urlItem.text())

        favicons = faviconCache()
        favicons.setEnabled(self.ui.fetchFaviconCheckBox.isChecked())
        favicons.clear()
        favicons.prefetch(urls)
        log.debug("WebSearchGui::refreshIcon_clicked, %s url(s) queued" % len(urls))

    def writeSettings(self):
        log.debug("WebSearchGui, writeSettings")
        settings = launchy.settings
        table = self.ui.entriesTable

        # How many engines are stored now, endArray() leaves the entries behind
        # the new count in the file.
        size = settings.beginReadArray(self.settingName)
        settings.endArray()

        # Remove all empty rows
        itemsToRemove = []
        for i in range(0, table.rowCount()):
            keyItem = table.item(i, 0)
            nameItem = table.item(i, 1)
            urlItem = table.item(i, 2)
            if keyItem is None or nameItem is None or urlItem is None:
                itemsToRemove.append(i)
            elif keyItem.text() == "" or nameItem.text() == "" or urlItem.text() == "":
                itemsToRemove.append(i)

        # Remove from the bottom up, every removal shifts the rows above it
        for i in reversed(itemsToRemove):
            table.removeRow(i)

        # Add all rows to the dirs array
        log.debug("WebSearchUi, settingName: %s" % self.settingName)
        settings.beginWriteArray(self.settingName)
        for i in range(0, table.rowCount()):
            settings.setArrayIndex(i)
            settings.setValue("key", (table.item(i,0).text().lower()))
            settings.setValue("name", (table.item(i,1).text()))
            settings.setValue("url", (table.item(i,2).text()))
        settings.endArray()

        # The engines of an array are numbered from 1 on, so the ones behind the
        # new count are what is left of the previous, longer list. Only they are
        # dropped, the switch below lives in the same group and has to stay.
        for i in range(table.rowCount() + 1, size + 1):
            settings.remove("%s/%d" % (self.settingName, i))

        # Stored as text on purpose: PySide2 hands a boolean "false" back as
        # None, which would silently turn the switch on again.
        settings.setValue(self.settingName + "/" + self.FETCH_FAVICON_KEY,
                          "true" if self.ui.fetchFaviconCheckBox.isChecked() else "false")
