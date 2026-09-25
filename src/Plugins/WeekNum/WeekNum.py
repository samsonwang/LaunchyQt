# 0.5 Fix, change Import for QApplication to be compatible with Launchy 3.18.0
# 0.4 Change, Better definition of the week formats, auxiliary class to ease code reading, code review of the getResults routine to reduce duplication, Week and Week Number
# 0.2 Change, First Week of the Year is always 01 even if it starts on the previous year
# 0.1 Change, Date Formats centralized in WeekNum Class

import sys, os
import launchy
import re

from launchy import CatItem
from datetime import date
from datetime import datetime
from datetime import timedelta

try:
    from PySide2 import QtCore, QtGui, QtWidgets
    from PySide2.QtCore import QLocale
    from PySide2.QtWidgets import QWidget, QApplication
except Exception as ex:
    try:
        from PySide6.QtWidgets import QWidget, QApplication
    except Exception as ex:
        print(f"CalcyPy, An error occurred: {ex}")

# Permette di ottenere facilmente date a partire dalla data attuale
class Calendario:

    @staticmethod
    def delta_al_giorno(oggi_numero, giorno_settimana):
        time_delta = 0
        if giorno_settimana <= oggi_numero:
            time_delta = 7

        time_delta = time_delta + (giorno_settimana - oggi_numero)
        return time_delta

    def __init__(self):
        self.oggi = date.today()
        self.ieri = (self.oggi - timedelta(days=1))
        self.domani = (self.oggi + timedelta(days=1))

        # calcola gli estremi della settimana corrente
        oggi_numero = int( self.oggi.strftime("%w"))
        self.inizio_settimana = self.oggi - timedelta(days=(oggi_numero-1))
        self.fine_settimana = self.oggi +timedelta(days=(7-oggi_numero))
        self.settimana_corrente = [self.inizio_settimana, self.fine_settimana]
        self.settimana_precedente = [self.inizio_settimana - timedelta(7), self.fine_settimana - timedelta(7)]
        self.settimana_successiva = [self.inizio_settimana + timedelta(7), self.fine_settimana + timedelta(7)]

        self.lun = self.oggi + timedelta(days=self.delta_al_giorno(oggi_numero, 1))
        self.mar = self.oggi + timedelta(days=self.delta_al_giorno(oggi_numero, 2))
        self.mer = self.oggi + timedelta(days=self.delta_al_giorno(oggi_numero, 3))
        self.gio = self.oggi + timedelta(days=self.delta_al_giorno(oggi_numero, 4))
        self.ven = self.oggi + timedelta(days=self.delta_al_giorno(oggi_numero, 5))
        self.sab = self.oggi + timedelta(days=self.delta_al_giorno(oggi_numero, 6))
        self.dom = self.oggi + timedelta(days=self.delta_al_giorno(oggi_numero, 7))

class WeekNum(launchy.Plugin):

    def __init__(self):
        launchy.Plugin.__init__(self)
        self.command = "Week Number"
        self.dateInputFormat = '%Y-%m-%d'
        self.weekInputFormat = "%Y-CW%W.%w"
        self.calendario = Calendario()

    def init(self):
        pass

    def getName(self):
        return "WeekNum"

    def setPath(self, path):
        self.path = path

    def getIcon(self):
        return self.path + "/week-num.png"

    def getLabels(self, inputDataList):
        inputDataList[-1].setLabel(self.getName())
        
    def is_year_starting_week_0(self, year_str):
        # tells is the first week of the year is returned by datetime as week 0 (true) or week 1 (false)
        week_0 = datetime.strptime(year_str + "-CW00.1" , "%Y-CW%W.%w")
        week_1 = datetime.strptime(year_str + "-CW01.1" , "%Y-CW%W.%w")
        return ( week_0 != week_1)

    # restituisce una data in una stringa codificata con uno specifico formato
    def data_in_stringa(self, data):
        if data <= self.calendario.oggi:
            formato = "%Y-%m-%d"
        else:
            formato = "%#d/%#m"

        return str(data.strftime(formato))
    
    # restituisce una settimana (una lista con due date, inizio e fine) in una stringa codificata con uno specifico formato
    def settimana_in_stringa(self,settimana):
        formato = "%#d/%#m/%Y"
        
        if settimana[0].year != settimana[1].year:
            return str(settimana[0].strftime(formato) + " - " + settimana[1].strftime(formato))
        if settimana[0].month != settimana[1].month:
            return str(settimana[0].strftime("%#d/%#m") + "-" + settimana[1].strftime(formato))
        if settimana[0].day != settimana[1].day:
            return str(settimana[0].strftime("%#d") + "-" + settimana[1].strftime(formato))
        return str(settimana[1].strftime(formato))

    # restituisce una settimana in un formato numerico
    def settimana_in_stringa_numerica(self,settimana):
        year = settimana[0].strftime("%Y")
        week = settimana[0].strftime("%W")
        
        if self.is_year_starting_week_0( str(settimana[0].year) ):
            week = "{:02}".format( int(week) +1 )
        return str( ""+ year + "-CW"+ week)

    # restituisce una data in un formato codificato per permettere un ordinamento temporale crescente e decrescente
    def data_in_stringa_numerica(self, data):
        year = data.strftime("%Y")
        week = data.strftime("%W")
        day = data.strftime("%w")
        
        if self.is_year_starting_week_0(str(data.year)):
            week = "{:02}".format( int(week) +1 )
        
        return str( ""+ year + "-CW"+ week+ "."+day)

    def settimana_numerica_in_stringa(self, settimana):
        if self.is_year_starting_week_0( str(settimana[0].year) ):
            settimana[0] = settimana[0] - timedelta(7)
            settimana[1] = settimana[1] - timedelta(7)
        return self.settimana_in_stringa(settimana)

    def data_numerica_in_stringa(self, data):
        if self.is_year_starting_week_0( str(data.year) ):
            data = data - timedelta(7)
        return str(self.data_in_stringa(data))
        
    def getResults(self, inputDataList, resultsList):

        inputText = inputDataList[0].getText()
        
        # when the pc is hibernated, time passes by and the calendar is not updated...
        if self.calendario.oggi != date.today():
            self.calendario = Calendario()
        
        fixed_command_results = { 
            "Week": self.settimana_in_stringa(self.calendario.settimana_corrente),
            "Date": self.data_in_stringa(self.calendario.oggi),
            "Today": self.data_in_stringa(self.calendario.oggi),
            "Tomorrow": self.data_in_stringa(self.calendario.domani),
            "Yesterday": self.data_in_stringa(self.calendario.ieri),
            "Monday": self.data_in_stringa(self.calendario.lun), 
            "Tuesday": self.data_in_stringa(self.calendario.mar), 
            "Wednesday": self.data_in_stringa(self.calendario.mer),
            "Thursday": self.data_in_stringa(self.calendario.gio),
            "Friday": self.data_in_stringa(self.calendario.ven),
            "Saturday": self.data_in_stringa(self.calendario.sab),
            "Sunday": self.data_in_stringa(self.calendario.dom),
            "Week Number": self.settimana_in_stringa_numerica(self.calendario.settimana_corrente),
            "Week": self.settimana_in_stringa(self.calendario.settimana_corrente),
            "Next Week": self.settimana_in_stringa(self.calendario.settimana_successiva),
            "Prev Week": self.settimana_in_stringa(self.calendario.settimana_precedente),
            "Week Day": self.data_in_stringa_numerica(self.calendario.oggi)}
        
        for command in fixed_command_results:
            if re.search(inputText, command, re.IGNORECASE):
                resultsList.append(CatItem( fixed_command_results[command], command, self.getName(), self.getIcon()) )

        # From a date defined as Year-Month-Day to its numeric representation as Year-CWMonth.Day 
        matched = re.match("[0-2][0-9][0-9][0-9]-([0-1])?[0-9]-([0-3])?[0-9]$", inputText)
        is_match = bool(matched)
        if( is_match) :
            date_time_obj = datetime.strptime(inputText, self.dateInputFormat)
            resultsList.append(CatItem( self.data_in_stringa_numerica(date_time_obj), "Week Day", self.getName(), self.getIcon()) )

        # From Week Number to Start and End Date of Week of Current Year
        matched = re.match("CW[0-5][0-9]$", inputText)
        matched_optional = re.match("CW[0-9]$", inputText)
        is_match = bool(matched) or bool(matched_optional)
        if( is_match ) :
            beg_week = str(date.today().year) + "-" + inputText + ".1"
            end_week = str(date.today().year) + "-" + inputText + ".0"
            settimana = [ datetime.strptime(beg_week, self.weekInputFormat), datetime.strptime(end_week, self.weekInputFormat)]
            resultsList.append(CatItem(self.settimana_numerica_in_stringa(settimana), "Week Dates", self.getName(), self.getIcon()))

        # From Year and Week Number to Start and End Date
        matched = re.match("[0-9][0-9][0-9][0-9]-CW[0-5][0-9]$", inputText)
        matched_optional = re.match("[0-9][0-9][0-9][0-9]-CW[0-9]$", inputText)
        is_match = bool(matched) or bool(matched_optional)
        if( is_match ) :
            beg_week = inputText + ".1"
            end_week = inputText + ".0"
            settimana = [ datetime.strptime(beg_week, self.weekInputFormat), datetime.strptime(end_week, self.weekInputFormat)]
            resultsList.append(CatItem(self.settimana_numerica_in_stringa(settimana), "Week Dates", self.getName(), self.getIcon()))

        # From Week Day to date of the Current Year
        matched = re.match("CW[0-5][0-9].[0-6]$", inputText)
        matched_optional = re.match("CW[0-9].[0-6]$", inputText)
        is_match = bool(matched) or bool(matched_optional)
        if( is_match ) :
            date_time_obj = datetime.strptime( str( self.calendario.oggi.year ) + "-" + inputText , self.weekInputFormat).date()
            resultsList.append(CatItem(self.data_numerica_in_stringa(date_time_obj), "Week Date", self.getName(), self.getIcon()))

        # From Year, Week Number and Week Day to Date
        matched = re.match("[0-9][0-9][0-9][0-9]-CW[0-5][0-9].[0-6]$", inputText)
        matched_optional = re.match("[0-9][0-9][0-9][0-9]-CW[0-9].[0-6]$", inputText)
        is_match = bool(matched) or bool(matched_optional)
        if( is_match ) :
            date_time_obj = datetime.strptime(inputText, self.weekInputFormat).date()
            resultsList.append(CatItem(self.data_numerica_in_stringa(date_time_obj), "Year Week Date", self.getName(), self.getIcon()))

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
