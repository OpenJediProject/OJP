@ECHO OFF

rem ****************
rem VARIABLE DEFINES
rem ****************

rem Setting common env vars
call SetBatchEnvVars.bat


rem ***************
rem START OF SCRIPT 
rem ***************

ECHO.
ECHO Removing Old Pk3s...
IF EXIST %PK3ASSETS%.pk3 DEL %PK3ASSETS%.pk3
IF EXIST %PK3DLL%.pk3 DEL %PK3DLL%.pk3

ECHO.
ECHO ===========
ECHO Making Pk3s
ECHO ===========

..\Utilities\zip\7za.exe a -tzip %PK3ASSETS%.pk3 .\%ASSETSFOLDER%\* -xr!.screenshots\ -xr!.svn\ -x!*.dll -x!*.so -x!.\%ASSETSFOLDER%\*.* -xr!*.nav -mx9
IF ERRORLEVEL 1 GOTO ERROR
..\Utilities\zip\7za.exe a -tzip %PK3DLL%.pk3 .\%ASSETSFOLDER%\*.dll -mx9

IF ERRORLEVEL 1 GOTO ERROR
ECHO.
ECHO ====================
ECHO Finished Making Pk3s
ECHO ====================
GOTO END


:ERROR
ECHO ================================================
ECHO Encountered Error While Zipping PK3S!  ABORTING!
ECHO ================================================
GOTO END


:END


