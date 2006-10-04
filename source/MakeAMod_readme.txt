Jedi Academy MP Game Source 1.01  (you must install the 1.01 game patch 
Copyright (C) 2001-2003 Raven Softare, Activision, Lucas Arts

The Game Source is broken out into 3 areas.

game - governs the game, runs on the server side.
cgame - governs the client side of the game, runs on the client side.
ui - handles the ui on the client side.

Making a quick mod is very straightforward. This document assumes Microsoft Visual C++ v7.00. It covers making a slight mod to the game source, recompiling for debugging and rebuilding the VMs for distribution.

Slow Rockets - TestMod
----------------------
1. Open up the jka_mp(SDK).sln in Microsoft Visual C++.
2. Set the "game" project as the active project.
3. Open the "g_local.h" file and change the GAMEVERSION define from "basejka_mod" to "TestMod" (or whatever you like)
4. Save "g_local.h"
5. Open the "g_weapon.c" file.
6. Go to line 90 and change the 900 to 300. The old line reads:
	
	#define	ROCKET_VELOCITY				900

The new line should read

	#define	ROCKET_VELOCITY				100

7. Save "g_weapon.c"
8. Set your configuration to Final and perform a "Build Solution" command to build a DLL for the game. 

At this point you have two options. You can run the debugger, choosing 'jaMP.exe' as the executable host which will load your DLL for debugging (you'll probably want to use Deubug or Release for debugging). When you release mods, you must build FINAL Dllss and put them in a pk3 file. (NOTE: the FINAL build configuration is like Release in that is is fully optimized, but it removes various developer prints and warnings.)

To build the sample MOD for the slow rocket test, do the following:

1. compile the dll as noted above
This produces a 'jampgamex86.dll' in the codemp\Final\ path. 

2. Make a "TestMod" path under your JA directory. This will be a sibling to 'base'
3. Zip 'jampgamex86.dll' to TestMod.pk3
4. move this pk3 to "\YourJAPath\GameData\TestMod"
5. Run JAMP with the following command line "jamp +set fs_game TestMod"
6. "TestMod" should be the referenced game and you should be able to catch up with and outrun your rockets.
(use devmap mp\ffa1 to start a map with cheats enabled, then type "give weapons" to give yourself the weapons.)


-----------------------------------------------
Using Visual Studio to Build and Debug your Mod

1. In VC 7.0, Open JKA_mp(SDK).sln
2. In VC 7.0, Select the JK2Game item underJKA_mp(SDK) and click Project->Properties
3. Select Debugging under Configuration Properties: 
   -set the "Command" to your jaMP.exe (e.g. C:\Program Files\LucasArts\Star Wars Jedi Knight Jedi Academy\GameData\jamp.exe)
   -set the "Command arguments" to:  +set fs_cdpath yourJAfolder +set fs_dirbeforepak 1 +set sv_pure 0 +set r_fullscreen 0 +set viewlog 1
   -set the "Working Directory" to your code path.  e.g. "C:\projects\JEDI_Academy_SDK\codemp\debug"
4. Right click on JK2Game and select 'Set as startup project'
5. hit f5 to launch the game.

Do this for each of projects you are making modifications to (cgame, game, ui)
NOTE: use quotes if you have spaces in your folder names.
e.g. +set fs_cdpath "C:\Program Files\LucasArts\Star Wars Jedi Knight Jedi Academy\GameData"

-------------------------------------------------
Making my Mod show up on the Mod list in the game

You need to have a TestMod.pk3 file in your mod directory
before it will show up on the in-game menu. Create a "description.txt"
file with some information about your game mod. Use WinZIP to create 
the TestMod.pk3 file.  You can now put the .dll files in this directory.



	
