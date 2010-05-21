@ECHO OFF

rem ****************
rem VARIABLE DEFINES
rem ****************

if NOT %BRANCHNAME% == "" GOTO BRANCHNAMESET
set BRANCHNAME=enhanced
:BRANCHNAMESET 

if NOT %PK3ASSETS% == "" GOTO PK3ASSETSSET
set PK3ASSETS=ojp_%BRANCHNAME%stuff
:PK3ASSETSSET 

if NOT %ASSETSFOLDER% == "" GOTO ASSETSFOLDERSET
set ASSETSFOLDER=ojp%BRANCHNAME%
:ASSETSFOLDERSET 

set PK3DLL=ojp_%BRANCHNAME%dlls

rem ***************
rem START OF SCRIPT 
rem ***************

ECHO.
ECHO ===========
ECHO Making Pk3s
ECHO ===========

ECHO Y | DEL ojp_enhancedstuff.pk3
ECHO Y | DEL ojp_enhanceddlls.pk3

rem..\Utilities\zip\7za.exe a -tzip %PK3ASSETS%.pk3 .\%ASSETSFOLDER%\* -xr!.screenshots\ -xr!.svn\ -x!*.dll -x!*.so -x!.\%ASSETSFOLDER%\*.* -xr!*.nav -mx9
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


