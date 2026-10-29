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


import ast
import math
import operator
import re

# Based on http://www.peterbe.com/plog/calculator-in-python-for-dummies
class Calculator:
    # Names the expression may reference: the public functions and constants of
    # the math module (ex. __xxx__ and 'e' which clashes with scientific
    # notation). Only these identifiers are legal, which closes the door on any
    # unknown name reaching the evaluator.
    _math_names = [f for f in dir(math) if f[:2] != '__' and f != 'e']

    # A single legal token, used only as a first line of defence before the AST
    # walker runs. Tokens: a number (digits with an optional decimal point), a
    # known math function/constant name, an operator, or a parenthesis.
    _token = (r'\d+(?:\.\d+)?|\.\d+|'
              r'(?:%s)|' % '|'.join(sorted(_math_names, key=len, reverse=True)) +
              r'[+\-*/%^()]')
    # The whole expression must be a sequence of such tokens, anchored at both
    # ends. `re.fullmatch` (not the old `re.match`) rejects trailing garbage such
    # as `1+2)x`, which the prefix-only check used to let through.
    whitelist = re.compile(r'(?:\s|%s)+$' % _token)

    # Binary operators we are willing to evaluate. Mapping is explicit so that
    # nothing outside this table can ever be reached.
    _binops = {
        ast.Add: operator.add,
        ast.Sub: operator.sub,
        ast.Mult: operator.mul,
        ast.Div: operator.truediv,
        ast.FloorDiv: operator.floordiv,
        ast.Mod: operator.mod,
        ast.Pow: operator.pow,
    }

    _unops = {
        ast.UAdd: operator.pos,
        ast.USub: operator.neg,
    }

    @staticmethod
    def isValidExpression(exp):
        return Calculator.whitelist.fullmatch(exp) is not None

    @staticmethod
    def calc(expr, advanced=False):
        expr = expr.replace('^', '**')
        symbols = vars(math) if advanced else {}
        return Calculator._evaluate(expr, symbols)

    @staticmethod
    def _evaluate(expr, symbols):
        """Evaluate `expr` using a whitelist-only AST walker.

        `eval` is never used: the expression is parsed with `ast.parse` and every
        node is inspected against a fixed allow-list before being executed. Any
        construct outside that list (attribute access, subscripts, calls to
        unknown names, comprehensions, lambdas, ...) raises, which closes the
        classic `().__class__.__mro__...` escape chain that a plain
        `__builtins__=None` eval cannot block.
        """
        tree = ast.parse(expr, mode='eval')
        return Calculator._eval_node(tree.body, symbols)

    @staticmethod
    def _eval_node(node, symbols):
        if isinstance(node, ast.Expression):
            return Calculator._eval_node(node.body, symbols)

        if isinstance(node, ast.Num):  # int, float, complex
            return node.n

        if isinstance(node, ast.UnaryOp):
            op = Calculator._unops.get(type(node.op))
            if op is None:
                raise ValueError('unsupported unary operator: {!r}'.format(type(node.op).__name__))
            return op(Calculator._eval_node(node.operand, symbols))

        if isinstance(node, ast.BinOp):
            op = Calculator._binops.get(type(node.op))
            if op is None:
                raise ValueError('unsupported binary operator: {!r}'.format(type(node.op).__name__))
            return op(Calculator._eval_node(node.left, symbols),
                      Calculator._eval_node(node.right, symbols))

        if isinstance(node, ast.Name):
            if node.id not in symbols:
                raise NameError('name {!r} is not defined'.format(node.id))
            return symbols[node.id]

        if isinstance(node, ast.Call):
            func = Calculator._eval_node(node.func, symbols)
            if not callable(func):
                raise TypeError('{!r} is not callable'.format(node.func))
            args = [Calculator._eval_node(a, symbols) for a in node.args]
            if node.keywords:
                raise ValueError('keyword arguments are not allowed')
            return func(*args)

        raise ValueError(
            'unsupported expression element: {!r}'.format(type(node).__name__))
