@ ECHO OFF

rem ****************
rem VARIABLE DEFINES
rem ****************

rem Installer Script Variables
set INSTALLSCRIPT=ojp_enhanced_installscript.iss
set OUTPUTFILENAME=InstallOJPEnhanced

rem ************
rem Compile DLLs
rem ************

call CompileOJP.bat

rem *********
rem Make Pk3s 
rem *********

call MakePK3.bat

ECHO.
ECHO ===========================
ECHO Building Windows Setup File
ECHO ===========================

..\Utilities\InnoSetup\iscc.exe %INSTALLSCRIPT% /O"." /F"%OUTPUTFILENAME%"

ECHO.
ECHO =====================
ECHO Cleaning Up Temp Pk3s
ECHO =====================

del *.pk3

ECHO.
ECHO =========
ECHO FINISHED!
ECHO =========
