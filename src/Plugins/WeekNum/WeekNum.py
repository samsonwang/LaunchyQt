# Copyright (c) 2022 Christian Russo
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

import sys, os
import re

import logging as log

from datetime import date
from datetime import datetime

from PySide6.QtWidgets import QApplication

import launchy
from launchy import CatItem


class WeekNum(launchy.Plugin):
    def __init__(self):
        launchy.Plugin.__init__(self)
        self.command = "WeekNum"

    def init(self):
        pass
    
    def getName(self):
        return "WeekNum"

    def setPath(self, path):
        self.path = path

    def getIcon(self):
        return self.path + "/week-num.png"

    def getLabels(self, inputDataList):
        pass

    def getResults(self, inputDataList, resultsList):
        inputText = inputDataList[0].getText()

        # If user asked date of Today
        if re.search(inputText, "Today", re.IGNORECASE):
            result = str(date.today().strftime('%d-%m-%Y'))
            resultsList.append(CatItem(str(result), "Today", self.getName(), self.getIcon()))

        # If user asked the Week Number
        if re.search(inputText, "Week Number", re.IGNORECASE):
            weekNumber = date.today().isocalendar()[1]
            resultsList.append(CatItem( str(date.today().year) + "-CW"+ str(weekNumber), "Week Number", self.getName(), self.getIcon()))
            
        # If user asked the Week Day
        if re.search(inputText, "Week Day", re.IGNORECASE):
            result = str( date.today().strftime('%Y-CW%W.%w'))
            resultsList.append(CatItem( str(result), "Week Day", self.getName(), self.getIcon()))

        # From Date to Year, Week Number and Week Day
        test_string = inputText
        matched = re.match("([0-3])?[0-9]-([0-1])?[0-9]-[0-2][0-9][0-9][0-9]$", test_string)
        is_match = bool(matched)
        if( is_match) :
            date_time_obj = datetime.strptime(test_string, '%d-%m-%Y')
            result = date_time_obj.strftime('%Y-CW%W.%w')
            resultsList.append(CatItem(str(result), "Week Day", self.getName(), self.getIcon()))

        # From Week Number to Start and End Date of Week of Current Year
        test_string = inputText
        matched = re.match("CW[0-5][0-9]$", test_string)
        is_match = bool(matched)
        if( is_match ) :
            start_Str = str(date.today().year) + "-" + test_string + ".1"
            end___Str = str(date.today().year) + "-" + test_string + ".0"
            start_Date = datetime.strptime(start_Str, "%Y-CW%W.%w")
            end___Date = datetime.strptime(end___Str, "%Y-CW%W.%w")
            start_Result = start_Date.strftime("%d-%m-%Y")
            end___Result = end___Date.strftime("%d-%m-%Y")
            
            resultsList.append(CatItem(str(start_Result) + " to " + str(end___Result), "Week Dates", self.getName(), self.getIcon()))

        # From Week Day to date of the Current Year
        test_string = inputText
        matched = re. match("CW[0-5][0-9].[0-5]$", inputText)
        is_match = bool(matched)
        if( is_match ) :
            test_string = str(date.today().year) + "-" + test_string
            date_time_obj = datetime.strptime(test_string, "%Y-CW%W.%w")
            result = date_time_obj.strftime("%d-%m-%Y")
            resultsList.append(CatItem(str(result), "Date", self.getName(), self.getIcon()))

        # From Year and Week Number to Start and End Date
        test_string = inputText
        matched = re. match("[0-9][0-9][0-9][0-9]-CW[0-5][0-9]$", test_string)
        is_match = bool(matched)
        if( is_match ) :
            start_Str = test_string + ".1"
            end___Str = test_string + ".0"
            start_Date = datetime.strptime(start_Str, "%Y-CW%W.%w")
            end___Date = datetime.strptime(end___Str, "%Y-CW%W.%w")
            start_Result = start_Date.strftime("%d-%m-%Y")
            end___Result = end___Date.strftime("%d-%m-%Y")            
            resultsList.append(CatItem(str(start_Result) + " to " + str(end___Result), "Week Dates", self.getName(), self.getIcon()))

        # From Year, Week Number and Week Day to Date
        test_string = inputText
        matched = re. match("[0-9][0-9][0-9][0-9]-CW[0-5][0-9].[0-5]$", test_string)
        is_match = bool(matched)
        if( is_match ) :
            date_time_obj = datetime.strptime(test_string, "%Y-CW%W.%w")
            result = date_time_obj.strftime("%d-%m-%Y")
            resultsList.append(CatItem(str(result), "Date", self.getName(), self.getIcon()))

    def getCatalog(self, resultsList):
        pass

    def launchItem(self, inputDataList, catItem):
        # Copy Result in Clipboard
        catItem = inputDataList[-1].getTopResult()
        QApplication.clipboard().setText(catItem.fullPath())

    def launchyShow(self):
        pass

    def launchyHide(self):
        pass

    def doDialog(self, parent):
        pass

    def endDialog(self, accept):
        pass

def getPlugin():
    return WeekNum
