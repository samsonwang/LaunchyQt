@echo off
copy ..\src\plugin_interface.h .
copy ..\src\catalog.h .
"c:\bin\doxygen" default.doxygen
rem pause