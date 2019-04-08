
from PyQt5 import QtCore, QtGui, QtWidgets
from PyQt5.QtCore import QVariant
from PyQt5.QtWidgets import QApplication, QWidget, QDialog, QFileDialog

from .ui_calcy import *

import launchy

class CalcyOption(QWidget):
    def __init__(self, parent=None, setting_dir=None, settings=dict()):
        QtWidgets.QWidget.__init__(self, parent)
        self.ui = Ui_CalcyOption()
        self.ui.setupUi(self)
        self.setting_dir = setting_dir
        self.settings = settings
        print(settings)
        self.__initFromSettings()

    def __initFromSettings(self):
        decPtGrpSep = self.settings['decimalPointGroupSeparator']
        if decPtGrpSep == 1:
            self.ui.radioButtonDecSepComa.setChecked(True)
        elif decPtGrpSep == 2:
            self.ui.radioButtonDecSepDot.setChecked(True)
        else:
            self.ui.radioButtonDecSepSystem.setChecked(True)

        self.ui.spinBoxOutputPrecision.setValue(self.settings['outputPrecision'])
        self.ui.checkBoxShowGrpSep.setChecked(self.settings['showGroupSeparator'])
        self.ui.checkBoxCopyToClipboard.setChecked(self.settings['copyToClipboard'])

        self.ui.checkBoxBinOut.setChecked(self.settings['showBinOut'])
        self.ui.checkBoxOctOut.setChecked(self.settings['showOctOut'])
        self.ui.checkBoxDecOut.setChecked(self.settings['showDecOut'])
        self.ui.checkBoxHexOut.setChecked(self.settings['showHexOut'])
        self.ui.checkBoxSizeOut.setChecked(self.settings['showSizeOut'])
        self.ui.checkBoxShowBasePrefix.setChecked(self.settings['showBasePrefix'])
        self.ui.checkBoxShowLeadingZerosBin.setChecked(self.settings['showZeroBin'])
        self.ui.checkBoxShowLeadingZerosOct.setChecked(self.settings['showZeroOct'])
        self.ui.checkBoxShowLeadingZerosHex.setChecked(self.settings['showZeroHex'])

        bitwidth = self.settings['bitwidth']
        if bitwidth == 8:
            self.ui.radioButtonBW8.setChecked(True)
        elif bitwidth == 16:
            self.ui.radioButtonBW16.setChecked(True)
        elif bitwidth == 32:
            self.ui.radioButtonBW32.setChecked(True)
        elif bitwidth == 64:
            self.ui.radioButtonBW64.setChecked(True)

    def writeSettings(self):
        launchySettings = launchy.settings

        decPtGrpSep = 0
        if self.ui.radioButtonDecSepComa.isChecked():
            decPtGrpSep = 1
        elif self.ui.radioButtonDecSepDot.isChecked():
            decPtGrpSep = 2

        launchySettings.setValue(self.setting_dir + 'decimalPointGroupSeparator', decPtGrpSep)
        launchySettings.setValue(self.setting_dir + 'outputPrecision', self.ui.spinBoxOutputPrecision.value())
        launchySettings.setValue(self.setting_dir + 'showGroupSeparator', self.ui.checkBoxShowGrpSep.isChecked())
        launchySettings.setValue(self.setting_dir + 'copyToClipboard', self.ui.checkBoxCopyToClipboard.isChecked())

        launchySettings.setValue(self.setting_dir + 'showBinOut', self.ui.checkBoxBinOut.isChecked())
        launchySettings.setValue(self.setting_dir + 'showOctOut', self.ui.checkBoxOctOut.isChecked())
        launchySettings.setValue(self.setting_dir + 'showDecOut', self.ui.checkBoxDecOut.isChecked())
        launchySettings.setValue(self.setting_dir + 'showHexOut', self.ui.checkBoxHexOut.isChecked())
        launchySettings.setValue(self.setting_dir + 'showSizeOut', self.ui.checkBoxSizeOut.isChecked())

        launchySettings.setValue(self.setting_dir + 'showBasePrefix', self.ui.checkBoxShowBasePrefix.isChecked())
        launchySettings.setValue(self.setting_dir + 'showZeroBin', self.ui.checkBoxShowLeadingZerosBin.isChecked())
        launchySettings.setValue(self.setting_dir + 'showZeroOct', self.ui.checkBoxShowLeadingZerosOct.isChecked())
        launchySettings.setValue(self.setting_dir + 'showZeroHex', self.ui.checkBoxShowLeadingZerosHex.isChecked())

        bitwidth = 8
        if self.ui.radioButtonBW16.isChecked():
            bitwidth = 16
        elif self.ui.radioButtonBW32.isChecked():
            bitwidth = 32
        elif self.ui.radioButtonBW64.isChecked():
            bitwidth = 64
        launchySettings.setValue(self.setting_dir + 'bitwidth', bitwidth)
