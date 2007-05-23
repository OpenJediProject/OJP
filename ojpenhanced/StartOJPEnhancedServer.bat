@ ECHO OFF
ECHO This batch file starts an OJP Enhanced dedicated server.  
ECHO It will autorestart the server whenever it crashes.
ECHO To stop the restart cycle, just close this window.

ECHO Starting Server...
ECHO.
cd ..
call jamp.exe +set fs_game ojpenhanced +set dedicated 2 +exec OJPEnhancedServer.cfg

ECHO.
ECHO Restarting Server...
ECHO.
ECHO.
cd ojpenhanced
call .\StartOJPEnhancedServer.bat