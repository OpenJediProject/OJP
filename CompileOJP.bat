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

