# Copyright (c) 2008 Shahar Kosti
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
# Version 1.0:
#   * First public release
#
#import rpdb2; rpdb2.start_embedded_debugger("password")

import sys, os
import math
import re
import launchy

# Based on http://www.peterbe.com/plog/calculator-in-python-for-dummies
class Calculator:
    whitelist = '|'.join(
        # oprators, digits
        ['-', '\+', '/', '\\', '\*', '\^', '\*\*', '\(', '\)', '\d+', 'e']
        # functions of math module (ex. __xxx__)
        + [f for f in dir(math) if f[:2] != '__' and f != 'e']
    )

    integers_regex = re.compile(r'\b[\d\.]+\b')

    @staticmethod
    def isValidExpression(exp):
        return re.match(Calculator.whitelist, exp)

    @staticmethod
    def calc(expr, advanced=False):
        def safe_eval(expr, symbols={}):
            return eval(expr, dict(__builtins__=None), symbols)
        def whole_number_to_float(match):
            group = match.group()
            if group.find('.') == -1:
                return group + '.0'
            return group
        expr = expr.replace('^','**')
        expr = Calculator.integers_regex.sub(whole_number_to_float, expr)
        if advanced:
            return safe_eval(expr, vars(math))
        else:
            return safe_eval(expr)

class CalcyPy(launchy.Plugin):
    def __init__(self):
        launchy.Plugin.__init__(self)
        self.hash = launchy.hash(self.getName())

    def init(self):
        pass

    def getID(self):
        return int(self.hash)

    def getName(self):
        return "CalcyPy"

    def setPath(self, path):
        self.path = path

    def getIcon(self):
        return self.path + "/calcpy.ico"

    def getLabels(self, inputDataList):
        pass

    def getResults(self, inputDataList, resultsList):
        if len(inputDataList) > 1:
            return

        text = inputDataList[0].getText()
        if not Calculator.isValidExpression(text):
            return

        try:
            result = Calculator.calc(text, advanced=True)
        except:
            pass
        else:
            resultsList.append(launchy.CatItem(str(result)+".calcypy", str(result), self.getID(), self.getIcon() ) )

    def getCatalog(self, resultsList):
        pass

    def launchItem(self, inputDataList, catItemOrig):
        pass

    def launchyShow(self):
        pass

    def launchyHide(self):
        pass

def getPlugin():
    return CalcyPy
