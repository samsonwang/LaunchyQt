
import logging as log

from PySide2 import QtCore, QtGui, QtWidgets
from PySide2.QtWidgets import QApplication
from PySide2.QtWidgets import QWidget, QTableWidgetItem, QMessageBox

from .ui_websearch import *
from .FaviconCache import cache as faviconCache, toBool

import launchy


# Every stored value goes through here. A hand-edited ini can leave an entry
# out or unreadable, and QSettings answers that with an invalid value -- which
# QTableWidgetItem refuses with a TypeError, so one broken row used to keep
# the whole option page from opening instead of costing just that row.
def textItem(value):
    return QTableWidgetItem("" if value is None else str(value))


class WebSearchGui(QWidget):
    # The engines and the "fetch site icons" switch share one settings group.
    FETCH_FAVICON_KEY = "FetchFavicon"

    # The input separator is " \u25ba ", or " | " on a font without the arrow,
    # and the key is whatever stands in front of it -- so these characters
    # inside a key split the input somewhere the engine never looks. A plain
    # space is dead for a second reason: Tab is completion first (it replaces
    # the text with the top search result), so a multi-word key can only be
    # reached when nothing at all matches what was typed.
    SEPARATOR_CHARS = "\u25ba|"

    KEY_CHAR_PROBLEM = ("the key contains a space or the input separator, so it can "
                        "never be typed as one input segment")
    DUPLICATE_PROBLEM = "the key duplicates row %d, only the first one is used"
    URL_PROBLEM = ("the url has no %s placeholder, the query would be dropped and "
                   "the site would just open")

    # A failed cell is painted instead of only reported: while the page is open
    # this is where the row gets fixed, and the mark disappears the moment the
    # cell is corrected. An explicit foreground keeps it readable on a light and
    # a dark style alike.
    MARK_BACKGROUND = QtGui.QColor(150, 30, 30)
    MARK_FOREGROUND = QtGui.QColor(255, 255, 255)

    def __init__(self, parent=None, settingName=None):
        QtWidgets.QWidget.__init__(self, parent)
        self.settingName = settingName
        # Set while the validation rewrites cells itself, so the cellChanged
        # that every rewrite emits does not run the validation again.
        self.__updating = False
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
                table.setItem(i, 0, textItem(item))
                table.setItem(i, 1, textItem(defSet[item]['name']))
                table.setItem(i, 2, textItem(defSet[item]['url']))
        else:
            table.setRowCount(size)
            for i in range(0, size):
                settings.setArrayIndex(i);
                # The default is not decoration: a key the file lost, or one it
                # holds as something QSettings cannot read, reads back as None.
                table.setItem(i, 0, textItem(settings.value("key", "")))
                table.setItem(i, 1, textItem(settings.value("name", "")))
                table.setItem(i, 2, textItem(settings.value("url", "")))

        settings.endArray()

        # Mark what a hand-edited ini left broken right away, so the page shows
        # the problem without the row having to be touched first. Connected
        # afterwards: the marking below is not a user edit.
        self.__validate()
        self.ui.entriesTable.cellChanged[int, int].connect(self.__cellChanged)

        enabled = settings.value(self.settingName + "/" + self.FETCH_FAVICON_KEY, True)
        # Set before the connection below, so loading the stored value does not
        # count as a user toggle.
        self.ui.fetchFaviconCheckBox.setChecked(toBool(enabled, True))
        self.ui.fetchFaviconCheckBox.toggled.connect(self.__updateRefreshButton)
        self.__updateRefreshButton(self.ui.fetchFaviconCheckBox.isChecked())

    # With "Fetch site icons" off the cache refuses every request and iconPath()
    # answers None, so a refresh would download icons that are never shown -- the
    # button would be silently dead. It follows the switch instead.
    def __updateRefreshButton(self, enabled):
        self.ui.refreshIconButton.setEnabled(bool(enabled))

    # The new row starts out empty and keeps the cursor, so the key is the first
    # thing that can be typed. Its cells only exist once edited, which every
    # reader here already handles; a row that never gets a key is dropped by
    # writeSettings() without a word, so adding one and changing your mind
    # costs nothing.
    def addEntry_clicked(self):
        table = self.ui.entriesTable
        row = table.rowCount()
        table.insertRow(row)
        table.setCurrentCell(row, 0)

    def removeEntry_clicked(self):
        currentRow = self.ui.entriesTable.currentRow()
        if currentRow != -1:
            self.ui.entriesTable.removeRow(currentRow)

    # Re-fetch the icons of the listed sites. A site whose re-fetch fails
    # keeps its previous icon, so the user never sees an icon "disappear"
    # because of a transient network error or a blank placeholder response.
    def refreshIcon_clicked(self):
        # The button is disabled while the switch is off (see
        # __updateRefreshButton), so this is only the guard for a state that
        # slipped through: request() would drop every url on the floor and the
        # click would do nothing at all.
        if not self.ui.fetchFaviconCheckBox.isChecked():
            log.debug("WebSearchGui::refreshIcon_clicked, fetching is off, ignored")
            return

        table = self.ui.entriesTable
        urls = []
        for i in range(0, table.rowCount()):
            urlItem = table.item(i, 2)
            if urlItem and urlItem.text():
                urls.append(urlItem.text())

        favicons = faviconCache()
        favicons.setEnabled(True)
        favicons.refresh(urls)
        log.debug("WebSearchGui::refreshIcon_clicked, %s url(s) queued" % len(urls))

    def __cellChanged(self, row, col):
        if not self.__updating:
            self.__validate()

    # Outer whitespace is invisible in the cell and just as deadly as the rest
    # -- the key is compared against what the user types, byte for byte -- so it
    # is trimmed and written back to the cell: the table then shows exactly what
    # the array is going to receive.
    @staticmethod
    def __cellText(item):
        if item is None:
            return ""
        text = item.text()
        trimmed = text.strip()
        if trimmed != text:
            item.setText(trimmed)
        return trimmed

    # A url only carries the query where "%s" stands, and getUrl() treats "%%"
    # as a literal percent -- so a template whose only "%s" sits behind a "%%"
    # counts as one without a placeholder, exactly as getUrl() reads it.
    @classmethod
    def __hasPlaceholder(cls, url):
        return "%s" in url.replace("%%", "")

    # What keeps this row from being stored, as (column, reason) pairs. An
    # empty key means an empty row, which writeSettings drops without a word,
    # so it is not a problem yet.
    def __rowProblems(self, row):
        table = self.ui.entriesTable
        key = self.__cellText(table.item(row, 0))
        url = self.__cellText(table.item(row, 2))

        problems = []
        if key and (any(ch.isspace() for ch in key)
                    or any(ch in key for ch in self.SEPARATOR_CHARS)):
            problems.append((0, self.KEY_CHAR_PROBLEM))
        if url and not self.__hasPlaceholder(url):
            problems.append((2, self.URL_PROBLEM))
        return key, url, problems

    # Walk the table once and answer (rows to store, rows that cannot be
    # stored). Painting happens here too, because both callers want it: the
    # live cell edit (so the row is marked while it is still fixable) and
    # writeSettings (so a hand-edited ini shows up as broken before it is
    # silently left out of the array).
    def __validate(self):
        # The trimming and the painting below rewrite cells, and every rewrite
        # emits cellChanged -- this keeps that from re-entering the validation.
        self.__updating = True
        try:
            table = self.ui.entriesTable
            rows = []
            blocked = []
            marks = {}
            seen = {}
            for row in range(0, table.rowCount()):
                key, url, problems = self.__rowProblems(row)
                if not key:
                    continue

                # Only a row that is otherwise usable claims its key, otherwise
                # a broken row would shadow a good one further down.
                if not problems:
                    if key.lower() in seen:
                        problems.append((0, self.DUPLICATE_PROBLEM % (seen[key.lower()] + 1)))
                    else:
                        seen[key.lower()] = row

                if problems:
                    blocked.append((row, key, [reason for _, reason in problems]))
                    for col, reason in problems:
                        marks[(row, col)] = reason
                    continue

                nameItem = table.item(row, 1)
                name = "" if nameItem is None else nameItem.text()
                rows.append((key.lower(), name, url))

            self.__markCells(marks)
            return rows, blocked
        finally:
            self.__updating = False

    def __markCells(self, marks):
        table = self.ui.entriesTable
        for row in range(0, table.rowCount()):
            for col in (0, 2):
                item = table.item(row, col)
                if item is None:
                    continue
                reason = marks.get((row, col), "")
                if reason:
                    item.setBackground(QtGui.QBrush(self.MARK_BACKGROUND))
                    item.setForeground(QtGui.QBrush(self.MARK_FOREGROUND))
                    item.setToolTip(reason)
                else:
                    # An empty brush hands the cell back its default look.
                    item.setBackground(QtGui.QBrush())
                    item.setForeground(QtGui.QBrush())
                    item.setToolTip("")

    # The dropped rows have to be named out loud: the page is thrown away the
    # moment writeSettings() returns (the option dialog owns it) and rebuilt
    # from the settings, so a marked cell would vanish unseen and the entry
    # would be gone without a trace.
    def __report(self, blocked):
        if not blocked:
            return
        lines = ["Row %d (%s): %s" % (row + 1, key, "; ".join(reasons))
                 for row, key, reasons in blocked]
        log.warning("WebSearchGui::writeSettings, not stored: %s" % " | ".join(lines))
        parent = self.window()
        QMessageBox.warning(parent if parent is not None else self,
                            "WebSearchPy - entries not saved",
                            "These entries cannot work and were not saved:\n\n%s"
                            % "\n".join(lines))

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

        # A row that cannot work is left out instead of being written: a key
        # with a space or the separator in it is dead on arrival, and a url
        # without "%s" throws the query away and opens the bare site. The user
        # is told which rows those are, see __report().
        rows, blocked = self.__validate()

        # Store the rows that survived validation
        log.debug("WebSearchUi, settingName: %s" % self.settingName)
        settings.beginWriteArray(self.settingName)
        for i, (key, name, url) in enumerate(rows):
            settings.setArrayIndex(i)
            settings.setValue("key", key)
            settings.setValue("name", name)
            settings.setValue("url", url)
        settings.endArray()

        # The engines of an array are numbered from 1 on, so the ones behind the
        # new count are what is left of the previous, longer list. Only they are
        # dropped, the switch below lives in the same group and has to stay.
        for i in range(len(rows) + 1, size + 1):
            settings.remove("%s/%d" % (self.settingName, i))

        # Stored as text on purpose: PySide2 hands a boolean "false" back as
        # None, which would silently turn the switch on again.
        settings.setValue(self.settingName + "/" + self.FETCH_FAVICON_KEY,
                          "true" if self.ui.fetchFaviconCheckBox.isChecked() else "false")

        self.__report(blocked)
