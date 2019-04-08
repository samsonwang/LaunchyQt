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


import math
import re

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

        if advanced:
            return safe_eval(expr, vars(math))
        else:
            return safe_eval(expr)
