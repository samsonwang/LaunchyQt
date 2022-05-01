# -*- coding: utf-8 -*-

################################################################################
## Form generated from reading UI file 'calcy.ui'
##
## Created by: Qt User Interface Compiler version 6.2.4
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
from PySide6.QtWidgets import (QApplication, QCheckBox, QFormLayout, QGroupBox,
    QHBoxLayout, QLabel, QRadioButton, QSizePolicy,
    QSpacerItem, QSpinBox, QTabWidget, QVBoxLayout,
    QWidget)

class Ui_CalcyOption(object):
    def setupUi(self, CalcyOption):
        if not CalcyOption.objectName():
            CalcyOption.setObjectName(u"CalcyOption")
        CalcyOption.resize(516, 262)
        self.horizontalLayout_2 = QHBoxLayout(CalcyOption)
        self.horizontalLayout_2.setObjectName(u"horizontalLayout_2")
        self.tabWidget = QTabWidget(CalcyOption)
        self.tabWidget.setObjectName(u"tabWidget")
        font = QFont()
        font.setFamilies([u"Verdana"])
        font.setPointSize(10)
        self.tabWidget.setFont(font)
        self.tabOutput = QWidget()
        self.tabOutput.setObjectName(u"tabOutput")
        self.formLayout = QFormLayout(self.tabOutput)
        self.formLayout.setObjectName(u"formLayout")
        self.verticalLayout_5 = QVBoxLayout()
        self.verticalLayout_5.setObjectName(u"verticalLayout_5")
        self.groupBox_2 = QGroupBox(self.tabOutput)
        self.groupBox_2.setObjectName(u"groupBox_2")
        self.verticalLayout_6 = QVBoxLayout(self.groupBox_2)
        self.verticalLayout_6.setObjectName(u"verticalLayout_6")
        self.radioButtonDecSepSystem = QRadioButton(self.groupBox_2)
        self.radioButtonDecSepSystem.setObjectName(u"radioButtonDecSepSystem")
        self.radioButtonDecSepSystem.setChecked(True)

        self.verticalLayout_6.addWidget(self.radioButtonDecSepSystem)

        self.radioButtonDecSepComa = QRadioButton(self.groupBox_2)
        self.radioButtonDecSepComa.setObjectName(u"radioButtonDecSepComa")

        self.verticalLayout_6.addWidget(self.radioButtonDecSepComa)

        self.radioButtonDecSepDot = QRadioButton(self.groupBox_2)
        self.radioButtonDecSepDot.setObjectName(u"radioButtonDecSepDot")

        self.verticalLayout_6.addWidget(self.radioButtonDecSepDot)


        self.verticalLayout_5.addWidget(self.groupBox_2)

        self.verticalLayout_4 = QVBoxLayout()
        self.verticalLayout_4.setObjectName(u"verticalLayout_4")
        self.horizontalLayout_3 = QHBoxLayout()
        self.horizontalLayout_3.setObjectName(u"horizontalLayout_3")
        self.label = QLabel(self.tabOutput)
        self.label.setObjectName(u"label")

        self.horizontalLayout_3.addWidget(self.label)

        self.spinBoxOutputPrecision = QSpinBox(self.tabOutput)
        self.spinBoxOutputPrecision.setObjectName(u"spinBoxOutputPrecision")
        sizePolicy = QSizePolicy(QSizePolicy.Fixed, QSizePolicy.Fixed)
        sizePolicy.setHorizontalStretch(0)
        sizePolicy.setVerticalStretch(0)
        sizePolicy.setHeightForWidth(self.spinBoxOutputPrecision.sizePolicy().hasHeightForWidth())
        self.spinBoxOutputPrecision.setSizePolicy(sizePolicy)
        self.spinBoxOutputPrecision.setMinimumSize(QSize(55, 0))
        self.spinBoxOutputPrecision.setMaximumSize(QSize(60, 30))
        self.spinBoxOutputPrecision.setAlignment(Qt.AlignRight|Qt.AlignTrailing|Qt.AlignVCenter)
        self.spinBoxOutputPrecision.setMinimum(1)
        self.spinBoxOutputPrecision.setMaximum(10)
        self.spinBoxOutputPrecision.setValue(3)

        self.horizontalLayout_3.addWidget(self.spinBoxOutputPrecision)


        self.verticalLayout_4.addLayout(self.horizontalLayout_3)

        self.checkBoxShowGrpSep = QCheckBox(self.tabOutput)
        self.checkBoxShowGrpSep.setObjectName(u"checkBoxShowGrpSep")
        self.checkBoxShowGrpSep.setChecked(False)

        self.verticalLayout_4.addWidget(self.checkBoxShowGrpSep)

        self.checkBoxCopyToClipboard = QCheckBox(self.tabOutput)
        self.checkBoxCopyToClipboard.setObjectName(u"checkBoxCopyToClipboard")
        self.checkBoxCopyToClipboard.setChecked(True)

        self.verticalLayout_4.addWidget(self.checkBoxCopyToClipboard)


        self.verticalLayout_5.addLayout(self.verticalLayout_4)


        self.formLayout.setLayout(0, QFormLayout.LabelRole, self.verticalLayout_5)

        self.tabWidget.addTab(self.tabOutput, "")
        self.tabRadix = QWidget()
        self.tabRadix.setObjectName(u"tabRadix")
        self.horizontalLayout_6 = QHBoxLayout(self.tabRadix)
        self.horizontalLayout_6.setObjectName(u"horizontalLayout_6")
        self.verticalLayout_3 = QVBoxLayout()
        self.verticalLayout_3.setObjectName(u"verticalLayout_3")
        self.groupBox_3 = QGroupBox(self.tabRadix)
        self.groupBox_3.setObjectName(u"groupBox_3")
        self.horizontalLayout = QHBoxLayout(self.groupBox_3)
        self.horizontalLayout.setObjectName(u"horizontalLayout")
        self.checkBoxBinOut = QCheckBox(self.groupBox_3)
        self.checkBoxBinOut.setObjectName(u"checkBoxBinOut")
        self.checkBoxBinOut.setChecked(True)

        self.horizontalLayout.addWidget(self.checkBoxBinOut)

        self.checkBoxOctOut = QCheckBox(self.groupBox_3)
        self.checkBoxOctOut.setObjectName(u"checkBoxOctOut")
        self.checkBoxOctOut.setChecked(True)

        self.horizontalLayout.addWidget(self.checkBoxOctOut)

        self.checkBoxHexOut = QCheckBox(self.groupBox_3)
        self.checkBoxHexOut.setObjectName(u"checkBoxHexOut")
        self.checkBoxHexOut.setChecked(True)

        self.horizontalLayout.addWidget(self.checkBoxHexOut)

        self.checkBoxSizeOut = QCheckBox(self.groupBox_3)
        self.checkBoxSizeOut.setObjectName(u"checkBoxSizeOut")
        self.checkBoxSizeOut.setChecked(True)

        self.horizontalLayout.addWidget(self.checkBoxSizeOut)


        self.verticalLayout_3.addWidget(self.groupBox_3)

        self.verticalLayout = QVBoxLayout()
        self.verticalLayout.setObjectName(u"verticalLayout")
        self.checkBoxShowBasePrefix = QCheckBox(self.tabRadix)
        self.checkBoxShowBasePrefix.setObjectName(u"checkBoxShowBasePrefix")
        self.checkBoxShowBasePrefix.setChecked(True)

        self.verticalLayout.addWidget(self.checkBoxShowBasePrefix)

        self.checkBoxShowLeadingZerosBin = QCheckBox(self.tabRadix)
        self.checkBoxShowLeadingZerosBin.setObjectName(u"checkBoxShowLeadingZerosBin")
        self.checkBoxShowLeadingZerosBin.setChecked(True)

        self.verticalLayout.addWidget(self.checkBoxShowLeadingZerosBin)

        self.checkBoxShowLeadingZerosOct = QCheckBox(self.tabRadix)
        self.checkBoxShowLeadingZerosOct.setObjectName(u"checkBoxShowLeadingZerosOct")
        self.checkBoxShowLeadingZerosOct.setChecked(True)

        self.verticalLayout.addWidget(self.checkBoxShowLeadingZerosOct)

        self.checkBoxShowLeadingZerosHex = QCheckBox(self.tabRadix)
        self.checkBoxShowLeadingZerosHex.setObjectName(u"checkBoxShowLeadingZerosHex")
        self.checkBoxShowLeadingZerosHex.setChecked(True)

        self.verticalLayout.addWidget(self.checkBoxShowLeadingZerosHex)

        self.verticalSpacer_2 = QSpacerItem(20, 40, QSizePolicy.Minimum, QSizePolicy.Expanding)

        self.verticalLayout.addItem(self.verticalSpacer_2)


        self.verticalLayout_3.addLayout(self.verticalLayout)


        self.horizontalLayout_6.addLayout(self.verticalLayout_3)

        self.verticalLayout_2 = QVBoxLayout()
        self.verticalLayout_2.setObjectName(u"verticalLayout_2")
        self.groupBoxBW = QGroupBox(self.tabRadix)
        self.groupBoxBW.setObjectName(u"groupBoxBW")
        self.horizontalLayout_5 = QHBoxLayout(self.groupBoxBW)
        self.horizontalLayout_5.setObjectName(u"horizontalLayout_5")
        self.radioButtonBW64 = QRadioButton(self.groupBoxBW)
        self.radioButtonBW64.setObjectName(u"radioButtonBW64")

        self.horizontalLayout_5.addWidget(self.radioButtonBW64)

        self.radioButtonBW32 = QRadioButton(self.groupBoxBW)
        self.radioButtonBW32.setObjectName(u"radioButtonBW32")

        self.horizontalLayout_5.addWidget(self.radioButtonBW32)

        self.radioButtonBW16 = QRadioButton(self.groupBoxBW)
        self.radioButtonBW16.setObjectName(u"radioButtonBW16")
        self.radioButtonBW16.setChecked(True)

        self.horizontalLayout_5.addWidget(self.radioButtonBW16)

        self.radioButtonBW8 = QRadioButton(self.groupBoxBW)
        self.radioButtonBW8.setObjectName(u"radioButtonBW8")

        self.horizontalLayout_5.addWidget(self.radioButtonBW8)


        self.verticalLayout_2.addWidget(self.groupBoxBW)

        self.verticalSpacer = QSpacerItem(20, 40, QSizePolicy.Minimum, QSizePolicy.Expanding)

        self.verticalLayout_2.addItem(self.verticalSpacer)


        self.horizontalLayout_6.addLayout(self.verticalLayout_2)

        self.tabWidget.addTab(self.tabRadix, "")

        self.horizontalLayout_2.addWidget(self.tabWidget)


        self.retranslateUi(CalcyOption)

        self.tabWidget.setCurrentIndex(0)


        QMetaObject.connectSlotsByName(CalcyOption)
    # setupUi

    def retranslateUi(self, CalcyOption):
        CalcyOption.setWindowTitle(QCoreApplication.translate("CalcyOption", u"CalcyPy - Simple calculator", None))
        self.groupBox_2.setTitle(QCoreApplication.translate("CalcyOption", u"Decimal point and group separator", None))
#if QT_CONFIG(tooltip)
        self.radioButtonDecSepSystem.setToolTip(QCoreApplication.translate("CalcyOption", u"Use system locale settings", None))
#endif // QT_CONFIG(tooltip)
        self.radioButtonDecSepSystem.setText(QCoreApplication.translate("CalcyOption", u"System default", None))
        self.radioButtonDecSepComa.setText(QCoreApplication.translate("CalcyOption", u"Coma as decimal point and dot as group separator", None))
        self.radioButtonDecSepDot.setText(QCoreApplication.translate("CalcyOption", u"Dot as decimal point and coma as group separator", None))
        self.label.setText(QCoreApplication.translate("CalcyOption", u"Decimal output Precision", None))
#if QT_CONFIG(tooltip)
        self.spinBoxOutputPrecision.setToolTip(QCoreApplication.translate("CalcyOption", u"<html><head/><body><p>Sets the maximal number of digits</p></body></html>", None))
#endif // QT_CONFIG(tooltip)
        self.checkBoxShowGrpSep.setText(QCoreApplication.translate("CalcyOption", u"Show group separator in result", None))
#if QT_CONFIG(tooltip)
        self.checkBoxCopyToClipboard.setToolTip(QCoreApplication.translate("CalcyOption", u"If this option is enabled, pressing enter key will copy calculation result to clipboard.", None))
#endif // QT_CONFIG(tooltip)
        self.checkBoxCopyToClipboard.setText(QCoreApplication.translate("CalcyOption", u"Use Enter key to Copy result to Clipboard", None))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.tabOutput), QCoreApplication.translate("CalcyOption", u"General", None))
        self.groupBox_3.setTitle(QCoreApplication.translate("CalcyOption", u"Show result as", None))
        self.checkBoxBinOut.setText(QCoreApplication.translate("CalcyOption", u"Bin", None))
        self.checkBoxOctOut.setText(QCoreApplication.translate("CalcyOption", u"Oct", None))
        self.checkBoxHexOut.setText(QCoreApplication.translate("CalcyOption", u"Hex", None))
        self.checkBoxSizeOut.setText(QCoreApplication.translate("CalcyOption", u"Size", None))
#if QT_CONFIG(tooltip)
        self.checkBoxShowBasePrefix.setToolTip(QCoreApplication.translate("CalcyOption", u"Show prefix for given number base in results (i.e. \"0x\" for hexadecimal numbers)", None))
#endif // QT_CONFIG(tooltip)
        self.checkBoxShowBasePrefix.setText(QCoreApplication.translate("CalcyOption", u"Show base prefix in result", None))
        self.checkBoxShowLeadingZerosBin.setText(QCoreApplication.translate("CalcyOption", u"Show leading zeros in bin output", None))
        self.checkBoxShowLeadingZerosOct.setText(QCoreApplication.translate("CalcyOption", u"Show leading zeros in oct output", None))
        self.checkBoxShowLeadingZerosHex.setText(QCoreApplication.translate("CalcyOption", u"Show leading zeros in hex output", None))
        self.groupBoxBW.setTitle(QCoreApplication.translate("CalcyOption", u"Calculation bit width", None))
        self.radioButtonBW64.setText(QCoreApplication.translate("CalcyOption", u"64", None))
        self.radioButtonBW32.setText(QCoreApplication.translate("CalcyOption", u"32", None))
        self.radioButtonBW16.setText(QCoreApplication.translate("CalcyOption", u"16", None))
        self.radioButtonBW8.setText(QCoreApplication.translate("CalcyOption", u"8", None))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.tabRadix), QCoreApplication.translate("CalcyOption", u"Radix", None))
    # retranslateUi

