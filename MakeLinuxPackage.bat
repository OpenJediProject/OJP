@ ECHO OFF
rem This file makes the linux distro but IT DOESN'T MAKE THE jampgamei386.so!
rem Just place a compiled .so in the \Enhanced file. 

rem ****************
rem VARIABLE DEFINES
rem ****************

set BRANCHNAME=enhanced

set ASSETSFOLDER=ojp%BRANCHNAME%
set PK3ASSETS=ojp_%BRANCHNAME%stuff
set OUTPUTFILENAME=OJP%BRANCHNAME%_Linux.zip

rem ***********************
rem Compile jampgamei386.so
rem ***********************

call CompileOJPLinux.bat
IF %ERRORLEVEL% NEQ 0 GOTO END

rem ********************
rem Copy jampgamei386.so
rem ********************

copy .\source\game\jampgamei386.so .
IF EXIST jampgamei386.so GOTO MAKEPK3

ECHO.
ECHO ===========================================================
ECHO Error! jampgamei386.so Not Found! Compile must have failed.
ECHO ===========================================================
GOTO END

:MAKEPK3
rem *********
rem Make Pk3s 
rem *********

call MakePK3.bat

ECHO.
ECHO ========================
ECHO Move files into position
ECHO ========================

mkdir temp\%ASSETSFOLDER%\docs

rem If not OJP Basic, copy the OJP Basic docs into position.
IF NOT %BRANCHNAME%==basic copy ..\Basic\docs\* .\temp\%ASSETSFOLDER%\docs

copy .\docs\* .\temp\%ASSETSFOLDER%\docs

copy jampgamei386.so .\temp\%ASSETSFOLDER%

copy ojp_%BRANCHNAME%stuff.pk3 .\temp\%ASSETSFOLDER%

ECHO.
ECHO ==================
ECHO Zipping things up!
ECHO ==================

cd temp

..\..\Utilities\zip\7za.exe a -tzip %OUTPUTFILENAME% .\%ASSETSFOLDER% -xr!.svn\ -mx9

move %OUTPUTFILENAME% ..

cd ..

ECHO.
ECHO ======================
ECHO Cleaning Up Temp Files
ECHO ======================

rmdir /s /q temp
del *.pk3

ECHO.
ECHO =========
ECHO FINISHED!
ECHO =========

:END

