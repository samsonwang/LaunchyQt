# -*- coding: utf-8 -*-

################################################################################
## Form generated from reading UI file 'websearch.ui'
##
## Created by: Qt User Interface Compiler version 6.5.3
##
## WARNING! All changes made in this file will be lost when recompiling UI file!
################################################################################

from PySide6.QtCore import (QCoreApplication, QDate, QDateTime, QLocale,
    QMetaObject, QObject, QPoint, QRect,
    QSize, QTime, QUrl, Qt)
from PySide6.QtGui import (QBrush, QColor, QConicalGradient, QCursor,
    QFont, QFontDatabase, QGradient, QIcon,
    QImage, QKeySequence, QLinearGradient, QPainter,
    QPalette, QPixmap, QRadialGradient, QTransform)
from PySide6.QtWidgets import (QApplication, QHBoxLayout, QHeaderView, QPushButton,
    QSizePolicy, QSpacerItem, QTableWidget, QTableWidgetItem,
    QVBoxLayout, QWidget)

class Ui_WebSearchWidget(object):
    def setupUi(self, WebSearchWidget):
        if not WebSearchWidget.objectName():
            WebSearchWidget.setObjectName(u"WebSearchWidget")
        WebSearchWidget.resize(647, 360)
        self.verticalLayout = QVBoxLayout(WebSearchWidget)
        self.verticalLayout.setObjectName(u"verticalLayout")
        self.entriesTable = QTableWidget(WebSearchWidget)
        if (self.entriesTable.columnCount() < 3):
            self.entriesTable.setColumnCount(3)
        __qtablewidgetitem = QTableWidgetItem()
        self.entriesTable.setHorizontalHeaderItem(0, __qtablewidgetitem)
        __qtablewidgetitem1 = QTableWidgetItem()
        self.entriesTable.setHorizontalHeaderItem(1, __qtablewidgetitem1)
        __qtablewidgetitem2 = QTableWidgetItem()
        self.entriesTable.setHorizontalHeaderItem(2, __qtablewidgetitem2)
        self.entriesTable.setObjectName(u"entriesTable")
        sizePolicy = QSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.entriesTable.sizePolicy().hasHeightForWidth())
        self.entriesTable.setSizePolicy(sizePolicy)
        self.entriesTable.horizontalHeader().setStretchLastSection(True)
        self.entriesTable.verticalHeader().setStretchLastSection(False)

        self.verticalLayout.addWidget(self.entriesTable)

        self.horizontalLayout = QHBoxLayout()
        self.horizontalLayout.setObjectName(u"horizontalLayout")
        self.horizontalSpacer_4 = QSpacerItem(40, 20, QSizePolicy.Expanding, QSizePolicy.Minimum)

        self.horizontalLayout.addItem(self.horizontalSpacer_4)

        self.addEntryButton = QPushButton(WebSearchWidget)
        self.addEntryButton.setObjectName(u"addEntryButton")

        self.horizontalLayout.addWidget(self.addEntryButton)

        self.horizontalSpacer = QSpacerItem(58, 17, QSizePolicy.Expanding, QSizePolicy.Minimum)

        self.horizontalLayout.addItem(self.horizontalSpacer)

        self.removeEntryButton = QPushButton(WebSearchWidget)
        self.removeEntryButton.setObjectName(u"removeEntryButton")

        self.horizontalLayout.addWidget(self.removeEntryButton)

        self.horizontalSpacer_5 = QSpacerItem(40, 20, QSizePolicy.Expanding, QSizePolicy.Minimum)

        self.horizontalLayout.addItem(self.horizontalSpacer_5)


        self.verticalLayout.addLayout(self.horizontalLayout)


        self.retranslateUi(WebSearchWidget)
        self.addEntryButton.clicked.connect(WebSearchWidget.addEntry_clicked)
        self.removeEntryButton.clicked.connect(WebSearchWidget.removeEntry_clicked)

        QMetaObject.connectSlotsByName(WebSearchWidget)
    # setupUi

    def retranslateUi(self, WebSearchWidget):
        WebSearchWidget.setWindowTitle(QCoreApplication.translate("WebSearchWidget", u"WebSearchPy - Search items from web", None))
        ___qtablewidgetitem = self.entriesTable.horizontalHeaderItem(0)
        ___qtablewidgetitem.setText(QCoreApplication.translate("WebSearchWidget", u"Key", None));
        ___qtablewidgetitem1 = self.entriesTable.horizontalHeaderItem(1)
        ___qtablewidgetitem1.setText(QCoreApplication.translate("WebSearchWidget", u"Name", None));
        ___qtablewidgetitem2 = self.entriesTable.horizontalHeaderItem(2)
        ___qtablewidgetitem2.setText(QCoreApplication.translate("WebSearchWidget", u"Url", None));
#if QT_CONFIG(tooltip)
        self.addEntryButton.setToolTip(QCoreApplication.translate("WebSearchWidget", u"Add a new entry", None))
#endif // QT_CONFIG(tooltip)
        self.addEntryButton.setText(QCoreApplication.translate("WebSearchWidget", u"+", None))
#if QT_CONFIG(tooltip)
        self.removeEntryButton.setToolTip(QCoreApplication.translate("WebSearchWidget", u"Remove the selected entry", None))
#endif // QT_CONFIG(tooltip)
        self.removeEntryButton.setText(QCoreApplication.translate("WebSearchWidget", u"-", None))
    # retranslateUi

