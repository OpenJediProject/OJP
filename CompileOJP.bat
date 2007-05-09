@ECHO OFF
ECHO.

rem NOTE:  This file requires that your VS80COMNTOOLS env variable is set to the tools dir of your VS install.

rem ****************
rem VARIABLE DEFINES
rem ****************

set OLDDIR=%CD%
set OJPSLN=source\OJP Enhanced.sln

ECHO ================
ECHO Starting Compile
ECHO ================

chdir /d "%VS80COMNTOOLS%"
cd ../IDE
devenv "%OLDDIR%\%OJPSLN%" /build Final
chdir /d "%OLDDIR%"

ECHO.
ECHO ================
ECHO Compile Complete
ECHO ================