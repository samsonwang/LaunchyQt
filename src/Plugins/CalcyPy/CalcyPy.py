# Copyright (c) 2019 Samson Wang
# Copyright (c) 2008 Shahar Kosti
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

import logging as log

try:
    from PySide2 import QtCore, QtGui, QtWidgets
    from PySide2.QtCore import QLocale
    from PySide2.QtWidgets import QWidget, QApplication
    from shiboken2 import wrapInstance, getCppPointer
except Exception as ex:
    print(f"CalcyPy, An error occurred: {ex}")

from launchy import Plugin, CatItem
from launchy import settings as lSettings

from Calculator import Calculator
from Calcy import CalcyGui

class CalcyPy(Plugin):
    setting_dir = 'CalcyPy/'

    # Every setting the plugin reads is listed here with its default. The dict is
    # used to pre-fill self.settings so a key is never missing, and to drive
    # __readSettings() one key at a time.
    default_settings = {
        # 0 = follow the system locale, 1 = "1.234,5", 2 = "1,234.5"
        'decimalPointGroupSeparator': 0,
        'outputPrecision': 3,
        'showGroupSeparator': False,
        'copyToClipboard': True,
        'showBinOut': True,
        'showOctOut': True,
        'showHexOut': True,
        'showSizeOut': True,
        'showBasePrefix': True,
        'showZeroBin': True,
        'showZeroOct': True,
        'showZeroHex': True,
        'bitwidth': 16,
    }

    # Keys whose value is fed straight into a "%0{n}x"-style format string or
    # into a branch, so only the values offered by the option dialog are usable.
    allowed_values = {
        'decimalPointGroupSeparator': (0, 1, 2),
        'bitwidth': (8, 16, 32, 64),
    }

    def __init__(self):
        Plugin.__init__(self)
        # Instance dict pre-filled with the defaults. It used to be an empty
        # class attribute filled by __readSettings(); when that read aborted
        # halfway the plugin kept running with a partial dict and raised
        # KeyError on every later query, with nothing in the log (issue #153).
        # doDialog() hands this very object to the options page, so it has to be
        # complete before anything is read from the ini file.
        self.settings = dict(CalcyPy.default_settings)

    def init(self):
        self.__readSettings()

    def getName(self):
        return "CalcyPy"

    def setPath(self, path):
        self.path = path
        log.debug("CalcyPy::setPath, path = {}".format(self.path))

    def getIcon(self):
        return self.path + "/calcpy.ico"

    def getLabels(self, inputDataList):
        pass

    def getResults(self, inputDataList, resultsList):
        # Everything below used to be unprotected: any exception raised while
        # formatting the result (a missing setting, a locale returning None, ...)
        # escaped to PluginWrapper and the plugin simply stopped answering, with
        # no trace in the log at all. Keep the whole body guarded so a failure is
        # at least visible instead of silent.
        try:
            self.__getResults(inputDataList, resultsList)
        except Exception:
            log.exception('CalcyPy::getResults, unexpected failure')

    def __getResults(self, inputDataList, resultsList):
        if len(inputDataList) > 1:
            return

        text = inputDataList[0].getText()
        text = self.__prepareExpression(text)
        if not Calculator.isValidExpression(text):
            return

        try:
            ret = Calculator.calc(text, advanced=True)
        except Exception as err:
            # Expected while typing: "3+" or "sqrt(" are not complete yet, so
            # this stays at debug level instead of spamming the log.
            log.debug('CalcyPy::getResults, cannot evaluate "{}": {}'.format(text, err))
            return

        log.debug('CalcyPy::getResults, text = {}, result = {}'.format(text, ret))

        # full divide
        if isinstance(ret, float) and abs(ret - int(ret)) < 1e-15:
            ret = int(ret)
            log.debug('CalcyPy::getResults, transform float to int: {}'.format(ret))

        if isinstance(ret, int):
            retInFloat = None
            # hexadecimal
            retInHex = self.__formatHexadecimal(ret)
            # decimal
            retInDec = self.__formatDecimal(ret)
            # octal
            retInOct = self.__formatOctal(ret)
            # binary
            retInBin = self.__formatBinary(ret)
            # size
            retInSize = self.__formatSize(ret)
        else:
            retInFloat = self.__formatFloat(ret)
            retInHex = None
            retInDec = None
            retInOct = None
            retInBin = None
            retInSize = None

        if retInFloat is not None:
            item = CatItem("float.calcypy", retInFloat,
                           self.getName(), self.getIcon())
            item.setUsage(50000)
            resultsList.append(item)

        if retInDec is not None:
            item = CatItem("dec.calcypy", retInDec,
                           self.getName(), self.getIcon())
            item.setUsage(50000)
            resultsList.append(item)

        if retInHex is not None:
            item = CatItem("hex.calcypy", retInHex,
                           self.getName(), self.getIcon())
            item.setUsage(40000)
            resultsList.append(item)

        if retInOct is not None:
            item = CatItem("oct.calcypy", retInOct,
                           self.getName(), self.getIcon())
            item.setUsage(30000)
            resultsList.append(item)

        if retInBin is not None:
            item = CatItem("bin.calcypy", retInBin,
                           self.getName(), self.getIcon())
            item.setUsage(20000)
            resultsList.append(item)

        if retInSize is not None:
            item = CatItem("size.calcypy", retInSize,
                           self.getName(), self.getIcon())
            item.setUsage(10000)
            resultsList.append(item)

    def getCatalog(self, catlogList):
        log.debug('CalcyPy::getCatalog, ...')

    def doDialog(self, parentWidgetPtr):
        log.debug('CalcyPy::doDialog, ...')
        parentWidget = wrapInstance(parentWidgetPtr, QWidget)
        self.widget = CalcyGui.CalcyOption(parentWidget, self.setting_dir, self.settings)
        # self.widget.show()
        return getCppPointer(self.widget)[0]

    def endDialog(self, accept):
        log.debug('CalcyPy::endDialog, ...')
        self.widget.hide()
        if accept:
            self.widget.writeSettings()
            self.__readSettings()
        del self.widget
        self.widget = None

    def launchItem(self, inputDataList, catItem):
        if self.settings['copyToClipboard']:
            QApplication.clipboard().setText(catItem.shortName())

    def __readSettings(self):
        log.debug('CalcyPy::__readSettings, begin, {}, {}'.format(lSettings, self.setting_dir))

        # Read one key at a time and swallow per-key failures: a single bad value
        # (a leftover entry from an older version, a hand-edited ini) used to
        # abort the read here and leave the settings dict half filled for the
        # rest of the session.
        for key, default in CalcyPy.default_settings.items():
            try:
                self.settings[key] = self.__readSetting(key, default)
            except Exception:
                log.exception('CalcyPy::__readSettings, failed to read key: {}'.format(key))
                self.settings[key] = default

        log.debug('CalcyPy::__readSettings, {}'.format(self.settings))

    def __readSetting(self, key, default):
        """Read a single key, falling back to `default` for anything unexpected.

        PySide2 5.12 does not honour the default passed to QSettings.value() and
        can hand back None, and python bools round-trip through the ini file as
        the strings "true"/"false", so the raw value is never used as is.
        """
        raw = lSettings.value(self.setting_dir + key, default)
        log.debug('CalcyPy::__readSetting, key = {}, raw = {}'.format(key, raw))

        if raw is None:
            return default

        if isinstance(default, bool):
            return CalcyPy.__asBool(raw, default)

        value = CalcyPy.__asInt(raw, default)

        allowed = CalcyPy.allowed_values.get(key)
        if allowed is not None and value not in allowed:
            log.warning('CalcyPy::__readSetting, key = {}, value {} not in {}, use default'
                        .format(key, value, allowed))
            return default

        return value

    @staticmethod
    def __asBool(raw, default):
        if isinstance(raw, bool):
            return raw
        if isinstance(raw, (int, float)):
            return bool(raw)

        text = str(raw).strip().lower()
        if text in ('true', '1', 'yes', 'on'):
            return True
        if text in ('false', '0', 'no', 'off', ''):
            return False

        log.warning('CalcyPy::__asBool, cannot interpret {}, use default'.format(raw))
        return default

    @staticmethod
    def __asInt(raw, default):
        if isinstance(raw, bool):
            # a bool here means the stored type does not match the key at all
            log.warning('CalcyPy::__asInt, bool stored for an int key, use default')
            return default

        try:
            return int(raw)
        except (TypeError, ValueError):
            pass

        try:
            return int(str(raw).strip())
        except (TypeError, ValueError):
            log.warning('CalcyPy::__asInt, cannot interpret {}, use default'.format(raw))
            return default

    def __decimalPoint(self):
        if self.settings['decimalPointGroupSeparator'] == 1:
            return ','
        elif self.settings['decimalPointGroupSeparator'] == 2:
            return '.'
        else:
            return QLocale.system().decimalPoint()

    def __groupSeparator(self):
        if self.settings['decimalPointGroupSeparator'] == 1:
            return '.'
        elif self.settings['decimalPointGroupSeparator'] == 2:
            return ','
        else:
            return QLocale.system().groupSeparator()

    def __prepareExpression(self, text):
        decimalPoint = self.__decimalPoint()
        groupSeparator = self.__groupSeparator()

        if groupSeparator == None or decimalPoint == None or groupSeparator == decimalPoint:
            return text

        retText = text.replace(groupSeparator, '')
        if decimalPoint != '.':
            retText = retText.replace(decimalPoint, '.')
        return retText

    def __formatFloat(self, num):
        integralPart = int(num);
        fractionalPart = num - integralPart;
        log.debug('integral part: {}, fractional part: {}'.format(integralPart, fractionalPart))

        decimalPoint = self.__decimalPoint()
        log.debug('CalcyPy::__formatFloat, decimal point, {}'.format(decimalPoint))

        if self.settings['showGroupSeparator']:
            groupSeparator = self.__groupSeparator()
            log.debug('CalcyPy::__formatFloat, group separator, {}'.format(groupSeparator))

            integralStr = '{:,}'.format(integralPart)
            if groupSeparator != ',':
                integralStr = integralStr.replace(',', groupSeparator)
            log.debug('CalcyPy::__formatFloat, integral str, {}'.format(integralStr))

            if self.settings['outputPrecision']:
                fractionalStr = str(round(abs(fractionalPart), self.settings['outputPrecision'])).lstrip('0')
            else:
                fractionalStr = str(abs(fractionalPart)).lstrip('0')
            if decimalPoint != '.':
                fractionalStr = fractionalStr.replace('.', decimalPoint)

            return integralStr + fractionalStr
        else:
            if self.settings['outputPrecision']:
                resultStr = str(round(num, self.settings['outputPrecision']))
            else:
                resultStr = str(num)

            if decimalPoint != '.':
                resultStr = resultStr.replace('.', decimalPoint)
                
            return resultStr

    def __formatHexadecimal(self, num):
        if self.settings['showHexOut']:
            hexFmtStr = ''
            if self.settings['showBasePrefix']:
                hexFmtStr += '0x'
            if self.settings['showZeroHex']:
                width = int(self.settings['bitwidth'] / 8)
                hexFmtStr += ('%%0%dx' % width)
            else:
                hexFmtStr += '%x'
            return hexFmtStr % num
        else:
            return None

    def __formatDecimal(self, num):
        if self.settings['showGroupSeparator']:
            decStr = '{:,}'.format(num)

            groupSeparator = self.__groupSeparator()
            if groupSeparator is None:
                return decStr

            return decStr.replace(',', groupSeparator)
        else:
            return '%d' % num

    def __formatOctal(self, num):
        if self.settings['showOctOut']:
            octFmtStr = ''
            if self.settings['showBasePrefix']:
                octFmtStr += '0'
            if self.settings['showZeroOct']:
                width = int(self.settings['bitwidth'] / 3)
                octFmtStr += ('%%0%do' % width)
            else:
                octFmtStr += '%o'
            return octFmtStr % num
        else:
            return None

    def __formatBinary(self, num):
        if self.settings['showBinOut']:
            if self.settings['showZeroBin']:
                retb = format(num, '0%db' % self.settings['bitwidth'])
            else:
                retb = format(num, 'b')

            ret = ''
            for i in range(len(retb)):
                ret += retb[i] if (i % 4 or i == 0) else (" " + retb[i])
            return ret
        else:
            return None

    def __formatSize(self, num):
        if self.settings['showSizeOut']:
            ret = ''

            size_giga_bytes = int(num / (1024 ** 3)) if int(num / (1024 ** 3)) else 0
            num -= size_giga_bytes * (1024 ** 3)
            ret += '%s GB ' % size_giga_bytes if size_giga_bytes else ''

            size_mega_bytes = int(num / (1024 * 1024)) if int(num / (1024 * 1024)) else 0
            num -= size_mega_bytes * (1024 ** 2)
            ret += '%s MB ' % size_mega_bytes if size_mega_bytes else ''

            size_kilo_bytes = int(num / (1024)) if int(num / 1024) else 0
            num -= size_kilo_bytes * 1024
            ret += '%s KB ' % size_kilo_bytes if size_kilo_bytes else ''

            size_bytes = int(num % (1024)) if int(num % (1024)) else 0
            ret += '%s B ' % size_bytes if size_bytes else ''

            if not ret:
                return  '0 B'
            else:
                return ret
        else:
            return None


def getPlugin():
    return CalcyPy
