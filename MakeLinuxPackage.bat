@ ECHO OFF
rem This file makes the linux distro but IT DOESN'T MAKE THE jampgamei386.so!
rem Just place a compiled .so in the \Enhanced file. 

rem *********
rem Make Pk3s 
rem *********

call MakePK3.bat

ECHO.
ECHO ========================
ECHO Move files into position
ECHO ========================

mkdir temp\ojpenhanced\docs

copy ..\Basic\docs\* .\temp\ojpenhanced\docs
copy .\docs\* .\temp\ojpenhanced\docs

copy jampgamei386.so .\temp\ojpenhanced

copy ojp_enhancedstuff.pk3 .\temp\ojpenhanced

ECHO.
ECHO ==================
ECHO Zipping things up!
ECHO ==================

cd temp

..\..\Utilities\zip\7za.exe a -tzip OJPLinux.zip .\ojpenhanced -xr!.svn\ -mx9

move OJPLinux.zip ..

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
