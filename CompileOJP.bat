@ECHO OFF
ECHO.

rem NOTE:  This file requires that your VS80COMNTOOLS env variable is set to the tools dir of your VS install.

rem ****************
rem VARIABLE DEFINES
rem ****************

set OLDDIR=%CD%
set OJPSLN=source\OJP Enhanced.sln
set COMPILER=devenv

ECHO ================
ECHO Starting Compile
ECHO ================

chdir /d "%VS80COMNTOOLS%"
cd ../IDE
IF EXIST VCExpress.exe set COMPILER=VCExpress

%COMPILER% "%OLDDIR%\%OJPSLN%" /build Final
chdir /d "%OLDDIR%"

ECHO.
ECHO ================
ECHO Compile Complete
ECHO ================