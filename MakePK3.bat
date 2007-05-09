@ECHO OFF

rem ****************
rem VARIABLE DEFINES
rem ****************

set PK3DLL=ojp_enhanceddlls
set PK3ASSETS=ojp_enhancedstuff
set ASSETSFOLDER=ojpenhanced


rem ***************
rem START OF SCRIPT 
rem ***************

ECHO ===========
ECHO Making Pk3s
ECHO ===========

..\Utilities\zip\7za.exe a -tzip %PK3ASSETS%.pk3 .\%ASSETSFOLDER%\* -xr!.svn\ -x!*.dll -x!*.so -x!.\%ASSETSFOLDER%\*.* -x!*.nav -mx9
..\Utilities\zip\7za.exe a -tzip %PK3DLL%.pk3 .\%ASSETSFOLDER%\*.dll -mx9

ECHO ====================
ECHO Finished Making Pk3s
ECHO ====================
