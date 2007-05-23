; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=OJP Enhanced
AppVerName=OJP Enhanced
AppPublisher=OJP Team
AppPublisherURL=http://ojp.jediknight.net/
AppSupportURL=http://ojp.jediknight.net/
AppUpdatesURL=http://ojp.jediknight.net/
DefaultDirName={reg:HKLM\SOFTWARE\LucasArts\Star Wars Jedi Knight Jedi Academy\1.0,Install Path|{pf}\LucasArts\Star Wars Jedi Knight Jedi Academy\}
DefaultGroupName=OJP Enhanced
AllowNoIcons=yes
OutputBaseFilename=setup
Compression=lzma
SolidCompression=yes
UninstallFilesDir={app}\GameData\ojpenhanced

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "ojp_enhanceddlls.pk3"; DestDir: "{app}\GameData\ojpenhanced"; Flags: ignoreversion
Source: "ojp_enhancedstuff.pk3"; DestDir: "{app}\GameData\ojpenhanced"; Flags: ignoreversion
Source: "ojpenhanced\trueview.cfg"; DestDir: "{app}\GameData\ojpenhanced"; Flags: ignoreversion
Source: "docs\*"; DestDir: "{app}\GameData\ojpenhanced\docs"; Flags: ignoreversion
Source: "..\Basic\docs\*"; DestDir: "{app}\GameData\ojpenhanced\docs"; Flags: ignoreversion
Source: "ojpenhanced\StartOJPEnhancedServer.bat"; DestDir: "{app}\GameData\ojpenhanced"; Flags: ignoreversion
Source: "ojpenhanced\OJPEnhancedServer.cfg"; DestDir: "{app}\GameData\ojpenhanced"; Flags: onlyifdoesntexist
Source: "..\resources\OJP File Icon\OJP.ico"; DestDir: "{app}\GameData\ojpenhanced"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\OJP Enhanced"; Filename: "{app}\GameData\jamp.exe"; Parameters: "+set fs_game ojpenhanced"; IconFilename: "{app}\GameData\ojpenhanced\OJP.ico"; WorkingDir: "{app}\GameData"
Name: "{group}\{cm:ProgramOnTheWeb,OJP Enhanced}"; Filename: "http://ojp.jediknight.net/"
Name: "{group}\{cm:UninstallProgram,OJP Enhanced}"; Filename: "{uninstallexe}"
Name: "{group}\Start OJP Enhanced Server"; Filename: "{app}\GameData\ojpenhanced\StartOJPEnhancedServer.bat"; Tasks: desktopicon; IconFilename: "{app}\GameData\ojpenhanced\OJP.ico"; WorkingDir: "{app}\GameData\ojpenhanced"
Name: "{commondesktop}\OJP Enhanced"; Filename: "{app}\GameData\jamp.exe"; Tasks: desktopicon; Parameters: "+set fs_game ojpenhanced"; IconFilename: "{app}\GameData\ojpenhanced\OJP.ico"; WorkingDir: "{app}\GameData"
Name: "{commondesktop}\Start OJP Enhanced Server"; Filename: "{app}\GameData\ojpenhanced\StartOJPEnhancedServer.bat"; Tasks: desktopicon; IconFilename: "{app}\GameData\ojpenhanced\OJP.ico"; WorkingDir: "{app}\GameData\ojpenhanced"

[Run]
Filename: "{app}\GameData\jamp.exe"; Description: "{cm:LaunchProgram,OJP Enhanced}"; Flags: nowait postinstall skipifsilent; Parameters: "+set fs_game ojpenhanced"

[UninstallDelete]
Type: files; Name: "{app}\GameData\ojpenhanced\*.dll"
