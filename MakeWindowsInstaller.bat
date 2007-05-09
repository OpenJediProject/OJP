@ ECHO OFF

rem ****************
rem VARIABLE DEFINES
rem ****************

rem Installer Script Variables
set INSTALLSCRIPT=ojp_enhanced_installscript.iss
set OUTPUTFILENAME=InstallOJPEnhanced



rem ***************
rem START OF SCRIPT 
rem ***************

rem Make pk3s
call MakePK3.bat

ECHO ===========================
ECHO Building Windows Setup File
ECHO ===========================

..\Utilities\InnoSetup\iscc.exe %INSTALLSCRIPT% /O"." /F"%OUTPUTFILENAME%"

ECHO =====================
ECHO Cleaning Up Temp Pk3s
ECHO =====================

del *.pk3

ECHO =========
ECHO FINISHED!
ECHO =========
