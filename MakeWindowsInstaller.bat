@ ECHO OFF

rem ****************
rem VARIABLE DEFINES
rem ****************

rem Setting common env vars
call SetBatchEnvVars.bat

rem Installer Script Variables
set INSTALLSCRIPT=ojp_enhanced_installscript.iss
set OUTPUTFILENAME=InstallOJPEnhanced


rem ************
rem Compile DLLs
rem ************

call CompileOJP.bat
IF ERRORLEVEL 1 GOTO END


rem *********
rem Make Pk3s 
rem *********

call MakePK3.bat
IF ERRORLEVEL 1 GOTO END


ECHO.
ECHO ===========================
ECHO Building Windows Setup File
ECHO ===========================
ECHO.

..\Utilities\InnoSetup\iscc.exe %INSTALLSCRIPT% /O"." /F"%OUTPUTFILENAME%"
IF ERRORLEVEL 1 GOTO INSTALLERROR
GOTO CLEANUP


:INSTALLERROR
ECHO.
ECHO =======================================================
ECHO Encountered Error While Making Install File!  ABORTING!
ECHO =======================================================
GOTO END


:CLEANUP
ECHO.
ECHO =====================
ECHO Cleaning Up Temp Pk3s
ECHO =====================

del *.pk3


ECHO.
ECHO ============================
ECHO FINISHED BUILD SUCCESSFULLY!
ECHO ============================

:END
