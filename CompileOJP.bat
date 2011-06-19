@ECHO OFF
ECHO.

rem NOTE:  This file requires that Visual Studios 2010 or 2008 to be installed.

rem ****************
rem VARIABLE DEFINES
rem ****************

set OLDDIR=%CD%

rem Determine which version of Visual Studios we have installed.
IF "%VS100COMNTOOLS%" == "" GOTO VS2008SETTINGS

:VS2010SETTINGS
set OJPSLN=source\OJP Enhanced_VS2010.sln
set TOOLDIR=%VS100COMNTOOLS%
GOTO COMPILE

:VS2008SETTINGS
set OJPSLN=source\OJP Enhanced.sln
set TOOLDIR=%VS80COMNTOOLS%
GOTO COMPILE


ECHO ================
ECHO Starting Compile
ECHO ================
:COMPILE
chdir /d "%TOOLDIR%"
cd ../IDE
IF EXIST VCExpress.exe GOTO VSEXPRESS


rem *********************
rem Normal Visual Studios
rem *********************

devenv "%OLDDIR%\%OJPSLN%" /build Final
GOTO FINISHED


rem **********************
rem Visual Studios Express
rem **********************
:VSEXPRESS
rem VS Express doesn't seem to display output to stdout, as such, we dump to a file and then read it off.
VCExpress "%OLDDIR%\%OJPSLN%" /build Final /Out "%OLDDIR%\bob.txt"
type %OLDDIR%\bob.txt
del %OLDDIR%\bob.txt
GOTO FINISHED


:FINISHED
chdir /d "%OLDDIR%"

IF ERRORLEVEL 1 GOTO ERROR

ECHO.
ECHO ================
ECHO Compile Complete
ECHO ================
GOTO END


:ERROR
ECHO ==================================================
ECHO Encountered Error While Compiling DLLS!  ABORTING!
ECHO ==================================================
GOTO END


:END

