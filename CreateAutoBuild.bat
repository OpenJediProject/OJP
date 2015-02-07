@ECHO OFF
rem ****************
rem Overview
rem ****************
rem Purpose:  This batch file generates a .zip build of OJP using the assets in the local workspace.
rem           The filename is based on the subversion revision the local workspace is synced to.
rem
rem Requirements:
rem - Visual Studios (Tested with VS2010, might work with VS2008 and Express versions)
rem - svnversion (Comes with standard svn or TortoiseSVN w/ command line svn installed)


rem ****************
rem VARIABLE DEFINES
rem ****************
set ERRORLEVEL=0
set OLDDIR=%CD%
set PK3DLL=ojp_enhanceddlls
set PK3ASSETS=ojp_enhancedstuff
set ASSETSFOLDER=ojpenhanced


rem ***************
rem START OF SCRIPT 
rem ***************


ECHO.
ECHO ==================================================
ECHO Starting Autobuild Process...
ECHO ================================================== 


rem Grabbing current svn repository changelist
FOR /F "tokens=*" %%i IN ('call svnversion') DO set CHANGELIST=%%i
ECHO.
ECHO ==================================================
ECHO Build version is %CHANGELIST%.
ECHO ================================================== 


rem Compile dlls
call CompileOJP.bat
IF %ERRORLEVEL% NEQ 0 GOTO COMPILEERROR


rem Make pk3s
ECHO.
ECHO ==================================================
ECHO Creating Build Pk3s...
ECHO ================================================== 
call MakePK3.bat
IF %ERRORLEVEL% NEQ 0 GOTO PK3ERROR


rem Creating build zip
ECHO.
ECHO ==================================================
ECHO Arraigning Assets...
ECHO ================================================== 
rem Remove old temp folder.
mkdir temp\%ASSETSFOLDER%\docs
rem Copy new assets into temp folder
copy .\docs\* .\temp\%ASSETSFOLDER%\docs
copy %PK3DLL%.pk3 .\temp\%ASSETSFOLDER%
copy %PK3ASSETS%.pk3 .\temp\%ASSETSFOLDER%

rem ZIP IT!
ECHO.
ECHO ==================================================
ECHO Creating Zip File...
ECHO ================================================== 
cd temp
..\..\Utilities\zip\7za.exe a -tzip ..\ojpenhanced_build_r%CHANGELIST%.zip .\%ASSETSFOLDER% -xr!.svn\ -mx9
rem move %OUTPUTFILENAME% ..
chdir /d "%OLDDIR%"

rem TODO - Post the build zip as a download to the to OJPA Google code repository.

ECHO.
ECHO ==================================================
ECHO Build Successful.
ECHO ==================================================
GOTO END


:COMPILEERROR
ECHO.
ECHO ==================================================
ECHO Build Failed: Encountered Error While Compiling DLLS!
ECHO ==================================================
GOTO END

:PK3ERROR
ECHO.
ECHO ==================================================
ECHO Build Failed: Encountered Error While making pk3!!
ECHO ==================================================
GOTO END


:END

