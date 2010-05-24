@ECHO OFF
ECHO.
ECHO This batch file compiles the OJP source code for use on linux dedicated servers.
ECHO Requirements:  
ECHO - mingw32 (with mingw make component) to be installed.  
ECHO - (mingw install folder)\bin folder in the PATH system environmental variable

rem ****************
rem VARIABLE DEFINES
rem ****************

set ERRORLEVEL=0
set OLDDIR=%CD%

ECHO.
ECHO ================
ECHO Starting Compile
ECHO ================

cd /source/game
IF EXIST jampgamei386.so DEL jampgamei386.so

mingw32-make.exe
IF NOT EXIST jampgamei386.so GOTO ERROR

ECHO.
ECHO ==================
ECHO Compile Successful
ECHO ==================
GOTO END


:ERROR
set ERRORLEVEL=1
ECHO.
ECHO ===================================================================
ECHO Encountered Error While Compiling jampgamei386.so!  Compile FAILED!
ECHO ===================================================================
GOTO END

:END
chdir /d "%OLDDIR%"

