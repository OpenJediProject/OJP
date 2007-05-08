ECHO OFF

rem ****************
rem VARIABLE DEFINES
rem ****************

set PK3DLL=ojp_enhanceddlls
set PK3ASSETS=ojp_enhancedstuff
set ASSETSFOLDER=ojpenhanced

rem Installer Script Variables
set INSTALLSCRIPT=ojp_enhanced_installscript.iss
set OUTPUTFILENAME=InstallOJPEnhanced



rem ***************
rem START OF SCRIPT 
rem ***************

ECHO =================
ECHO Creating New Pk3s
ECHO =================
ECHO

..\Utilities\zip\7za.exe a -tzip %PK3ASSETS%.pk3 .\%ASSETSFOLDER%\* -xr!.svn\ -x!*.dll -x!*.so -mx9
..\Utilities\zip\7za.exe a -tzip %PK3DLL%.pk3 .\%ASSETSFOLDER%\*.dll -mx9

ECHO ===========================
ECHO Building Windows Setup File
ECHO ===========================
ECHO

..\Utilities\InnoSetup\iscc.exe %INSTALLSCRIPT% /O"." /F"%OUTPUTFILENAME%"

ECHO =====================
ECHO Cleaning Up Temp Pk3s
ECHO =====================
ECHO

del %PK3ASSETS%.pk3
del %PK3DLL%.pk3

ECHO =========
ECHO FINISHED!
ECHO =========
ECHO