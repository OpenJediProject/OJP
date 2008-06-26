//
/*
=======================================================================

FORCE INTERFACE 

=======================================================================
*/

// use this to get a demo build without an explicit demo build, i.e. to get the demo ui files to build
#include "ui_local.h"
#include "../qcommon/qfiles.h"
#include "ui_force.h"

int uiForceSide = FORCE_LIGHTSIDE;
int uiJediNonJedi = -1;  //racc - indicates if we're jedi or merc while in jediVsMerc mode.  -1 == unknown
int uiForceRank = FORCE_MASTERY_JEDI_KNIGHT;
int uiMaxRank = MAX_FORCE_RANK;
int uiMaxPoints = 20;
int	uiForceUsed = 0;
int uiForceAvailable=0;

extern const char *UI_TeamName(int team);

qboolean gTouchedForce = qfalse;
vmCvar_t	ui_freeSaber, ui_forcePowerDisable;

#include "../namespace_begin.h"
void Menu_ShowItemByName(menuDef_t *menu, const char *p, qboolean bShow);
#include "../namespace_end.h"

int uiForceStarShaders[NUM_FORCE_STAR_IMAGES][2];
int uiSaberColorShaders[NUM_SABER_COLORS];
void UI_InitForceShaders(void)
{
	uiForceStarShaders[0][0] = trap_R_RegisterShaderNoMip("forcestar0");
	uiForceStarShaders[0][1] = trap_R_RegisterShaderNoMip("forcestar0");
	uiForceStarShaders[1][0] = trap_R_RegisterShaderNoMip("forcecircle1");
	uiForceStarShaders[1][1] = trap_R_RegisterShaderNoMip("forcestar1");
	uiForceStarShaders[2][0] = trap_R_RegisterShaderNoMip("forcecircle2");
	uiForceStarShaders[2][1] = trap_R_RegisterShaderNoMip("forcestar2");
	uiForceStarShaders[3][0] = trap_R_RegisterShaderNoMip("forcecircle3");
	uiForceStarShaders[3][1] = trap_R_RegisterShaderNoMip("forcestar3");
	uiForceStarShaders[4][0] = trap_R_RegisterShaderNoMip("forcecircle4");
	uiForceStarShaders[4][1] = trap_R_RegisterShaderNoMip("forcestar4");
	uiForceStarShaders[5][0] = trap_R_RegisterShaderNoMip("forcecircle5");
	uiForceStarShaders[5][1] = trap_R_RegisterShaderNoMip("forcestar5");
	uiForceStarShaders[6][0] = trap_R_RegisterShaderNoMip("forcecircle6");
	uiForceStarShaders[6][1] = trap_R_RegisterShaderNoMip("forcestar6");
	uiForceStarShaders[7][0] = trap_R_RegisterShaderNoMip("forcecircle7");
	uiForceStarShaders[7][1] = trap_R_RegisterShaderNoMip("forcestar7");
	uiForceStarShaders[8][0] = trap_R_RegisterShaderNoMip("forcecircle8");
	uiForceStarShaders[8][1] = trap_R_RegisterShaderNoMip("forcestar8");
	uiForceStarShaders[9][0] = trap_R_RegisterShaderNoMip("gfx/menus/forcecircle9");
	uiForceStarShaders[9][1] = trap_R_RegisterShaderNoMip("gfx/menus/forcestar9");
	uiForceStarShaders[10][0] = trap_R_RegisterShaderNoMip("gfx/menus/forcecircle10");
	uiForceStarShaders[10][1] = trap_R_RegisterShaderNoMip("gfx/menus/forcestar10");

	uiSaberColorShaders[SABER_RED]		= trap_R_RegisterShaderNoMip("menu/art/saber_red");
	uiSaberColorShaders[SABER_ORANGE]	= trap_R_RegisterShaderNoMip("menu/art/saber_orange");
	uiSaberColorShaders[SABER_YELLOW]	= trap_R_RegisterShaderNoMip("menu/art/saber_yellow");
	uiSaberColorShaders[SABER_GREEN]	= trap_R_RegisterShaderNoMip("menu/art/saber_green");
	uiSaberColorShaders[SABER_BLUE]		= trap_R_RegisterShaderNoMip("menu/art/saber_blue");
	uiSaberColorShaders[SABER_PURPLE]	= trap_R_RegisterShaderNoMip("menu/art/saber_purple");
}


//[ExpSys]
int NumberOfSkillRanks(int skill)
{//returns the number of ranks that a given skill has.
	//[NewUI]
	int i=0;
	for(i=0;i<NUM_TOTAL_SKILLS;i++)
	{
		if(uiRank[i].skillNum == skill)
			return uiRank[i].numRanks;
	}
	//[/NewUI]
return 3;
}
//[/ExpSys]


// Draw the stars spent on the current force power
void UI_DrawForceStars(rectDef_t *rect, float scale, vec4_t color, int textStyle, int forceindex, int val, int min, int max) 
{
	int	i,pad = 4;
	int	xPos,width = 16;
	int starcolor;

	//[ExpSys]
	max = NumberOfSkillRanks(forceindex);
	//[/ExpSys]

	if (val < min || val > max) 
	{
		val = min;
	}

	if (1)	// if (val)
	{
		xPos = rect->x;

		for (i=FORCE_LEVEL_1;i<=max;i++)
		{
			starcolor = bgForcePowerCost[forceindex][i];

			if(uiRank[forceindex].disabled)
			{
				vec4_t grColor = {0.2f, 0.2f, 0.2f, 1.0f};
				trap_R_SetColor(grColor);
			}

			//[ExpSys]
			if(starcolor != 0)
			{//only render if the force skill at this level costs something.
				if (val >= i)
				{	// Draw a star.
					UI_DrawHandlePic( xPos, rect->y+6, width, width, uiForceStarShaders[starcolor][1] );
				}
				else
				{	// Draw a circle.
					UI_DrawHandlePic( xPos, rect->y+6, width, width, uiForceStarShaders[starcolor][0] );
				}
			}

			/*
			if (val >= i)
			{	// Draw a star.
				UI_DrawHandlePic( xPos, rect->y+6, width, width, uiForceStarShaders[starcolor][1] );
			}
			else
			{	// Draw a circle.
				UI_DrawHandlePic( xPos, rect->y+6, width, width, uiForceStarShaders[starcolor][0] );
			}
			*/
			//[/ExpSys]

			if(uiRank[forceindex].disabled)
			{
				trap_R_SetColor(NULL);
			}

			xPos += width + pad;
		}
	}
}

// Set the client's force power layout.
void UI_UpdateClientForcePowers(const char *teamArg)
{
	//[ExpSys]
	char newForceString[MAX_INFO_STRING];
	int i;
	//Q_strcat
	strncpy(newForceString,va("%i-%i-",uiForceRank, uiForceSide),sizeof(newForceString));

	for(i =0;i<NUM_TOTAL_SKILLS;i++)
	{
		Q_strcat(newForceString,sizeof(newForceString),va("%i",uiRank[i].uiForcePowersRank));
	}

	if (gTouchedForce)
	{
		if (teamArg && teamArg[0])
		{
			//[ExpSys]
			trap_Cmd_ExecuteText( EXEC_APPEND, va("forcechanged \"%s\" %s\n", teamArg, newForceString ) );
			//trap_Cmd_ExecuteText( EXEC_APPEND, va("forcechanged \"%s\"\n", teamArg) );
			//[/ExpSys]
		}
		else
		{
			//[ExpSys]
			trap_Cmd_ExecuteText( EXEC_APPEND, va("forcechanged x %s\n", newForceString) );
			//trap_Cmd_ExecuteText( EXEC_APPEND, "forcechanged\n" );
			//[/ExpSys]
		}

	}

	gTouchedForce = qfalse;
}

int UI_TranslateFCFIndex(int index)
{
	if (uiForceSide == FORCE_LIGHTSIDE)
	{
		return index-uiInfo.forceConfigLightIndexBegin;
	}

	return index-uiInfo.forceConfigDarkIndexBegin;
}

void UI_SaveForceTemplate()
{
	char *selectedName = UI_Cvar_VariableString("ui_SaveFCF");
	char fcfString[512];
	char forceStringValue[4];
	fileHandle_t f;
	int strPlace = 0;
	int forcePlace = 0;
	int i = 0;
	qboolean foundFeederItem = qfalse;

	if (!selectedName || !selectedName[0])
	{
		Com_Printf("You did not provide a name for the template.\n");
		return;
	}

	if (uiForceSide == FORCE_LIGHTSIDE)
	{ //write it into the light side folder
		trap_FS_FOpenFile(va("forcecfg/light/%s.fcf", selectedName), &f, FS_WRITE);
	}
	else
	{ //if it isn't light it must be dark
		trap_FS_FOpenFile(va("forcecfg/dark/%s.fcf", selectedName), &f, FS_WRITE);
	}

	if (!f)
	{
		Com_Printf("There was an error writing the template file (read-only?).\n");
		return;
	}

	Com_sprintf(fcfString, sizeof(fcfString), "%i-%i-", uiForceRank, uiForceSide);
	strPlace = strlen(fcfString);

	while (forcePlace < NUM_FORCE_POWERS)
	{
		Com_sprintf(forceStringValue, sizeof(forceStringValue), "%i", uiRank[forcePlace].uiForcePowersRank);
		//Just use the force digit even if multiple digits. Shouldn't be longer than 1.
		fcfString[strPlace] = forceStringValue[0];
		strPlace++;
		forcePlace++;
	}
	fcfString[strPlace] = '\n';
	fcfString[strPlace+1] = 0;

	trap_FS_Write(fcfString, strlen(fcfString), f);
	trap_FS_FCloseFile(f);

	Com_Printf("Template saved as \"%s\".\n", selectedName);

	//Now, update the FCF list
	UI_LoadForceConfig_List();

	//Then, scroll through and select the template for the file we just saved
	while (i < uiInfo.forceConfigCount)
	{
		if (!Q_stricmp(uiInfo.forceConfigNames[i], selectedName))
		{
			if ((uiForceSide == FORCE_LIGHTSIDE && uiInfo.forceConfigSide[i]) ||
				(uiForceSide == FORCE_DARKSIDE && !uiInfo.forceConfigSide[i]))
			{
				Menu_SetFeederSelection(NULL, FEEDER_FORCECFG, UI_TranslateFCFIndex(i), NULL);
				foundFeederItem = qtrue;
			}
		}

		i++;
	}

	//Else, go back to 0
	if (!foundFeederItem)
	{
		Menu_SetFeederSelection(NULL, FEEDER_FORCECFG, 0, NULL);
	}
}

uiRank_t prevRank[42];
// 
extern qboolean UI_TrueJediEnabled( void );
void UpdateForceUsed()
{//racc - updates the current force powers setup based on current powers selected.
	int curpower, currank,spentInForce=0,i;
	menuDef_t *menu;

	// Currently we don't make a distinction between those that wish to play Jedi of lower than maximum skill.
	uiForceRank = uiMaxRank;

	uiForceUsed = 0;
	//[ExpSys]
	uiForceAvailable = uiMaxRank;
	//uiForceAvailable = forceMasteryPoints[uiForceRank];
	//[/ExpSys]

	//[ExpSys]
	/* don't automatically give Force jump
	// Make sure that we have one freebie in jump.
	if (uiRank[FP_LEVITATION].uiForcePowersRank<1)
	{
		uiRank[FP_LEVITATION].uiForcePowersRank=1;
	}
	*/
	//[/ExpSys]

	if ( UI_TrueJediEnabled() )
	{//true jedi mode is set
		if ( uiJediNonJedi == -1 )
		{//racc - we haven't determined if we're a jedi or merc yet.
			int x = 0;
			qboolean clear = qfalse, update = qfalse;
			uiJediNonJedi = FORCE_NONJEDI;
			while ( x < NUM_FORCE_POWERS )
			{//if any force power is set, we must be a jedi
				if ( x == FP_LEVITATION || x == FP_SABER_OFFENSE )
				{
					if ( uiRank[x].uiForcePowersRank > 1 )
					{
						uiJediNonJedi = FORCE_JEDI;
						break;
					}
					else if ( uiRank[x].uiForcePowersRank > 0 )
					{
						clear = qtrue;
					}
				}
				else if ( uiRank[x].uiForcePowersRank > 0 )
				{
					uiJediNonJedi = FORCE_JEDI;
					break;
				}
				x++;
			}
			if ( uiJediNonJedi == FORCE_JEDI )
			{
				if ( uiRank[FP_SABER_OFFENSE].uiForcePowersRank < 1 )
				{
					uiRank[FP_SABER_OFFENSE].uiForcePowersRank=1;
					update = qtrue;
				}
			}
			else if ( clear )
			{
				x = 0;
				while ( x < NUM_FORCE_POWERS )
				{//clear all force
					uiRank[x].uiForcePowersRank = 0;
					x++;
				}
				update = qtrue;
			}
			if ( update )
			{
				int myTeam;
				myTeam = (int)(trap_Cvar_VariableValue("ui_myteam"));
				if ( myTeam != TEAM_SPECTATOR )
				{
					UI_UpdateClientForcePowers(UI_TeamName(myTeam));//will cause him to respawn, if it's been 5 seconds since last one
				}
				else
				{
					UI_UpdateClientForcePowers(NULL);//just update powers
				}
			}
		}
	}

	menu = Menus_FindByName("ingame_playerforce");
	// Set the cost of the saberattack according to whether its free.
	if (ui_freeSaber.integer)
	{	// Make saber free
		bgForcePowerCost[FP_SABER_OFFENSE][FORCE_LEVEL_1] = 0;
		bgForcePowerCost[FP_SABER_DEFENSE][FORCE_LEVEL_1] = 0;
		// Make sure that we have one freebie in saber if applicable.
		if (uiRank[FP_SABER_OFFENSE].uiForcePowersRank<1)
		{
			uiRank[FP_SABER_OFFENSE].uiForcePowersRank=1;
		}
		if (uiRank[FP_SABER_DEFENSE].uiForcePowersRank<1)
		{
			uiRank[FP_SABER_DEFENSE].uiForcePowersRank=1;
		}
		if (menu)
		{
			Menu_ShowItemByName(menu, "setFP_SABER_DEFENSE", qtrue);
			Menu_ShowItemByName(menu, "setfp_saberthrow", qtrue);
			Menu_ShowItemByName(menu, "effectentry", qtrue);
			Menu_ShowItemByName(menu, "effectfield", qtrue);
			Menu_ShowItemByName(menu, "nosaber", qfalse);
		}
	}
	else
	{	// Make saber normal cost
		//[ExpSys]
		//use defines since we're tweaking these values for the experience system.
		bgForcePowerCost[FP_SABER_OFFENSE][FORCE_LEVEL_1] = SABER_OFFENSE_L1;
		bgForcePowerCost[FP_SABER_DEFENSE][FORCE_LEVEL_1] = SABER_DEFENSE_L1;
		//bgForcePowerCost[FP_SABER_OFFENSE][FORCE_LEVEL_1] = 1;
		//bgForcePowerCost[FP_SABER_DEFENSE][FORCE_LEVEL_1] = 1;	

		//Made Force Seeing Level 1 a pre-req to taking any additional force powers, except in the case of free sabers.
		if(uiRank[FP_SEE].uiForcePowersRank <= FORCE_LEVEL_0)
		{//can't use a saber if we're not Force sensitive!
			uiRank[FP_SABER_OFFENSE].uiForcePowersRank=0;
			//[StanceSelection]
			uiRank[NUM_FORCE_POWERS+SK_BLUESTYLE].uiForcePowersRank=0;
			uiRank[NUM_FORCE_POWERS+SK_REDSTYLE].uiForcePowersRank=0;
			uiRank[NUM_FORCE_POWERS+SK_PURPLESTYLE].uiForcePowersRank=0;
			uiRank[NUM_FORCE_POWERS+SK_GREENSTYLE].uiForcePowersRank=0;
			uiRank[NUM_FORCE_POWERS+SK_DUALSTYLE].uiForcePowersRank=0;
			uiRank[NUM_FORCE_POWERS+SK_STAFFSTYLE].uiForcePowersRank=0;
			//[/StanceSelection]
			uiRank[FP_SABERTHROW].uiForcePowersRank=0;
			uiRank[FP_SABER_DEFENSE].uiForcePowersRank=0;
			if (menu)
			{
				Menu_ShowItemByName(menu, "setfp_saberattack", qfalse);
				//[StanceSelection]
				Menu_ShowItemByName(menu, "setfp_bluestyle", qfalse);
				Menu_ShowItemByName(menu, "setfp_redstyle", qfalse);
				Menu_ShowItemByName(menu, "setfp_greenstyle", qfalse);
				Menu_ShowItemByName(menu, "setfp_purplestyle", qfalse);
				Menu_ShowItemByName(menu, "setfp_dualstyle", qfalse);
				Menu_ShowItemByName(menu, "setfp_staffstyle", qfalse);
				//[/StanceSelection]
				Menu_ShowItemByName(menu, "setfp_saberdefend", qfalse);
				Menu_ShowItemByName(menu, "setfp_saberthrow", qfalse);
				Menu_ShowItemByName(menu, "effectentry", qfalse);
				Menu_ShowItemByName(menu, "effectfield", qfalse);
				Menu_ShowItemByName(menu, "nosaber", qtrue);
			}
		}
		// Also, check if there is no saberattack.  If there isn't, there had better not be any defense or throw!
		else if (uiRank[FP_SABER_OFFENSE].uiForcePowersRank<1)
		//if (uiRank[FP_SABER_OFFENSE].uiForcePowersRank<1)
		{
			Menu_ShowItemByName(menu, "setfp_saberattack", qtrue);
			//[/ExpSys]
			//[StanceSelection]
			uiRank[NUM_FORCE_POWERS+SK_BLUESTYLE].uiForcePowersRank=0;
			uiRank[NUM_FORCE_POWERS+SK_REDSTYLE].uiForcePowersRank=0;
			uiRank[NUM_FORCE_POWERS+SK_PURPLESTYLE].uiForcePowersRank=0;
			uiRank[NUM_FORCE_POWERS+SK_GREENSTYLE].uiForcePowersRank=0;
			uiRank[NUM_FORCE_POWERS+SK_DUALSTYLE].uiForcePowersRank=0;
			uiRank[NUM_FORCE_POWERS+SK_STAFFSTYLE].uiForcePowersRank=0;
			//[/StanceSelection]
			uiRank[FP_SABER_DEFENSE].uiForcePowersRank=0;
			uiRank[FP_SABERTHROW].uiForcePowersRank=0;
			if (menu)
			{
				//[StanceSelection]
				Menu_ShowItemByName(menu, "setfp_bluestyle", qfalse);
				Menu_ShowItemByName(menu, "setfp_redstyle", qfalse);
				Menu_ShowItemByName(menu, "setfp_greenstyle", qfalse);
				Menu_ShowItemByName(menu, "setfp_purplestyle", qfalse);
				Menu_ShowItemByName(menu, "setfp_dualstyle", qfalse);
				Menu_ShowItemByName(menu, "setfp_staffstyle", qfalse);
				//[/StanceSelection]
				Menu_ShowItemByName(menu, "setfp_saberdefend", qfalse);
				Menu_ShowItemByName(menu, "setfp_saberthrow", qfalse);
				Menu_ShowItemByName(menu, "effectentry", qfalse);
				Menu_ShowItemByName(menu, "effectfield", qfalse);
				Menu_ShowItemByName(menu, "nosaber", qtrue);
			}
		}
		else
		{
			if (menu)
			{
				Menu_ShowItemByName(menu, "setfp_saberdefend", qtrue);
				if(uiForceRank >= 10)
					Menu_ShowItemByName(menu, "setfp_bluestyle", qtrue); // soresu

				if(uiForceRank >= 30)
					Menu_ShowItemByName(menu, "setfp_redstyle", qtrue);//djem so

				if(uiForceRank >= 20)
					Menu_ShowItemByName(menu, "setfp_greenstyle", qtrue);//makashi

				if(uiForceRank >= 60)
					Menu_ShowItemByName(menu, "setfp_purplestyle", qtrue);//juyo
				
				if(uiForceRank >= 40)
					Menu_ShowItemByName(menu, "setfp_dualstyle", qtrue);//Dual

				if(uiForceRank >= 50)
					Menu_ShowItemByName(menu, "setfp_staffstyle", qtrue);//Staff


				Menu_ShowItemByName(menu, "setfp_saberthrow", qtrue);
				Menu_ShowItemByName(menu, "effectentry", qtrue);
				Menu_ShowItemByName(menu, "effectfield", qtrue);
				Menu_ShowItemByName(menu, "nosaber", qfalse);
			}
		}
	}

	for(i=0;i<NUM_FORCE_POWERS;i++)
	{
		
		if(uiRank[i].uiForcePowersRank == FORCE_LEVEL_3)
		{
			spentInForce += bgForcePowerCost[i][1];
			spentInForce += bgForcePowerCost[i][2];
			spentInForce += bgForcePowerCost[i][3];
		}
		else if(uiRank[i].uiForcePowersRank == FORCE_LEVEL_2)
		{
			spentInForce += bgForcePowerCost[i][1];
			spentInForce += bgForcePowerCost[i][2];
		}
		else
			spentInForce += bgForcePowerCost[i][uiRank[i].uiForcePowersRank];
	}

	if(spentInForce >= 25)
	{
		for(i=NUM_FORCE_POWERS;i<NUM_FORCE_POWERS+SK_DISRUPTOR+1;i++)
		{
			if(uiRank[i].uiForcePowersRank > FORCE_LEVEL_1)
				uiRank[i].uiForcePowersRank = FORCE_LEVEL_1;
		}
		if(uiRank[NUM_FORCE_POWERS+SK_FLECHETTE].uiForcePowersRank > FORCE_LEVEL_1)
			uiRank[NUM_FORCE_POWERS+SK_FLECHETTE].uiForcePowersRank = 1;
	}
	else if(spentInForce)
	{
		for(i=NUM_FORCE_POWERS;i<NUM_FORCE_POWERS+SK_DISRUPTOR+1;i++)
		{
			if(uiRank[i].uiForcePowersRank > FORCE_LEVEL_2)
				uiRank[i].uiForcePowersRank = FORCE_LEVEL_2;
		}
		if(uiRank[NUM_FORCE_POWERS+SK_FLECHETTE].uiForcePowersRank > FORCE_LEVEL_2)
			uiRank[NUM_FORCE_POWERS+SK_FLECHETTE].uiForcePowersRank = 2;
	}

	if(uiRank[FP_LEVITATION].uiForcePowersRank >= FORCE_LEVEL_1)
	{
		if(uiRank[NUM_FORCE_POWERS+SK_JETPACK].uiForcePowersRank >= FORCE_LEVEL_1)
		{//We have both jump and JP
			if(prevRank && prevRank[NUM_FORCE_POWERS+SK_JETPACK].uiForcePowersRank == 0)
			{//Just bought JP
				uiRank[NUM_FORCE_POWERS+SK_JETPACK].uiForcePowersRank = 0;
			}
			else if(prevRank && prevRank[FP_LEVITATION].uiForcePowersRank == 0)
			{//Just bought jump
				uiRank[FP_LEVITATION].uiForcePowersRank = 0;
			}
		}
		
	}

	/*
	if(uiRank[NUM_FORCE_POWERS+SK_JETPACK].uiForcePowersRank >= FORCE_LEVEL_1)
	{
		uiRank[FP_LEVITATION].uiForcePowersRank=0;
	}
	*/

	//[Repeater]
	if(uiRank[NUM_FORCE_POWERS+SK_REPEATER].uiForcePowersRank < FORCE_LEVEL_3)
	{
		uiRank[NUM_FORCE_POWERS+SK_REPEATERUPGRADE].uiForcePowersRank = 0;
		menu = Menus_FindByName("ingame_playergunnery");
		if(menu)
		Menu_ShowItemByName(menu, "repeaterupgrade", qfalse);
	}
	else
	{
		menu = Menus_FindByName("ingame_playergunnery");
		if(menu)
		Menu_ShowItemByName(menu, "repeaterupgrade", qtrue);
	}
	//[/Repeater]
	//[BlasterRateOfFireUpgrade]
	if(uiRank[NUM_FORCE_POWERS+SK_BLASTER].uiForcePowersRank < FORCE_LEVEL_3)
	{
		uiRank[NUM_FORCE_POWERS+SK_BLASTERRATEOFFIREUPGRADE].uiForcePowersRank = 0;
		menu = Menus_FindByName("ingame_playergunnery");
		if(menu)
		Menu_ShowItemByName(menu, "blasterrateoffire", qfalse);
	}
	else
	{
		menu = Menus_FindByName("ingame_playergunnery");
		if(menu)
		Menu_ShowItemByName(menu, "blasterrateoffire", qtrue);
	}
	//[/BlasterRateOfFireUpgrade]

	//[ExpSys]
	menu = Menus_FindByName("ingame_playerforce");
	//Made Force Seeing Level 1 a pre-req to taking any additional force powers, except in the case of free sabers.
	if(uiRank[FP_SEE].uiForcePowersRank <= FORCE_LEVEL_0)
	{//can't use the force if we aren't Force sensitive.
		int i;
		for(i = 0; i < FP_SEE; i++) //saber powers set above!
		{
			uiRank[i].uiForcePowersRank = 0;
		}

		if(menu)
		{
			Menu_ShowItemByName(menu, "notforcesensitive", qtrue);
			Menu_ShowItemByName(menu, "neutralpowers", qfalse);
			Menu_ShowItemByName(menu, "darkpowers", qfalse);
			Menu_ShowItemByName(menu, "lightpowers", qfalse);
		}
	}
	else
	{
		if(menu)
		{
			Menu_ShowItemByName(menu, "notforcesensitive", qfalse);
			Menu_ShowItemByName(menu, "neutralpowers", qtrue);
			Menu_ShowItemByName(menu, "darkpowers", qtrue);
			Menu_ShowItemByName(menu, "lightpowers", qtrue);
		}
	}

	/*
	if(uiMaxRank <= 100 && uiRank[FP_SEE].uiForcePowersRank && (int)(trap_Cvar_VariableValue("ojp_trueBalance")) == 1)
	{
		if(uiMaxRank <= 75)
		{
			menu = Menus_FindByName("ingame_playerforce");
			if(menu)
			{
				Menu_ShowItemByName(menu, "darkpowers", qfalse);
				Menu_ShowItemByName(menu, "setfp_mindtrick", qfalse);
			}
			menu = Menus_FindByName("ingame_playergunnery");
			if(menu)
			{
				Menu_ShowItemByName(menu, "setsk_seeker", qfalse);
				Menu_ShowItemByName(menu, "setsk_sentry", qfalse);
				Menu_ShowItemByName(menu, "setsk_flamethrower", qfalse);
				Menu_ShowItemByName(menu, "setsk_jetpack", qfalse);
				Menu_ShowItemByName(menu, "setsk_forcefield", qfalse);
			}
			uiRank[FP_GRIP].uiForcePowersRank = 0;
			uiRank[FP_LIGHTNING].uiForcePowersRank = 0;
			uiRank[FP_TELEPATHY].uiForcePowersRank = 0;
			uiRank[NUM_FORCE_POWERS+SK_SEEKER].uiForcePowersRank = 0;
			uiRank[NUM_FORCE_POWERS+SK_SENTRY].uiForcePowersRank = 0;
			uiRank[NUM_FORCE_POWERS+SK_FLAMETHROWER].uiForcePowersRank = 0;
			uiRank[NUM_FORCE_POWERS+SK_JETPACK].uiForcePowersRank = 0;
			uiRank[NUM_FORCE_POWERS+SK_FORCEFIELD].uiForcePowersRank = 0;
		}
		else
		{
			uiRank[NUM_FORCE_POWERS+SK_SEEKER].uiForcePowersRank = 0;
			uiRank[NUM_FORCE_POWERS+SK_SENTRY].uiForcePowersRank = 0;
			uiRank[NUM_FORCE_POWERS+SK_FLAMETHROWER].uiForcePowersRank = 0;

			menu = Menus_FindByName("ingame_playerforce");
			if(menu)
			{
				Menu_ShowItemByName(menu, "darkpowers", qtrue);
				Menu_ShowItemByName(menu, "setfp_mindtrick", qtrue);
			}
			menu = Menus_FindByName("ingame_playergunnery");
			{
				Menu_ShowItemByName(menu, "setsk_jetpack", qtrue);
				Menu_ShowItemByName(menu, "setsk_forcefield", qtrue);
			}
		}
	}
	else if ((int)(trap_Cvar_VariableValue("ojp_trueBalance") == 1))
	{
			menu = Menus_FindByName("ingame_playerforce");
			if(menu && uiRank[FP_SEE].uiForcePowersRank)
			{
			Menu_ShowItemByName(menu, "darkpowers", qtrue);
			Menu_ShowItemByName(menu, "setfp_mindtrick", qtrue);
			}
			menu = Menus_FindByName("ingame_playergunnery");
			if(menu)
			{
			Menu_ShowItemByName(menu, "setsk_seeker", qtrue);
			Menu_ShowItemByName(menu, "setsk_sentry", qtrue);
			Menu_ShowItemByName(menu, "setsk_flamethrower", qtrue);
			Menu_ShowItemByName(menu, "setsk_jetpack", qtrue);
			Menu_ShowItemByName(menu, "setsk_forcefield", qtrue);
			}
	}
	*/

	// Make sure that we're still legal.
	for (curpower=0;curpower<NUM_TOTAL_SKILLS;curpower++)
	//for (curpower=0;curpower<NUM_FORCE_POWERS;curpower++)
	//[/ExpSys]
	{	// Make sure that our ranks are within legal limits.
		if (uiRank[curpower].uiForcePowersRank<0)
			uiRank[curpower].uiForcePowersRank=0;
		//[ExpSys]
		else if (uiRank[curpower].uiForcePowersRank>=NumberOfSkillRanks(curpower)+1)
			uiRank[curpower].uiForcePowersRank=NumberOfSkillRanks(curpower);
		//else if (uiRank[curpower].uiForcePowersRank>=NUM_FORCE_POWER_LEVELS)
		//	uiRank[curpower].uiForcePowersRank=(NUM_FORCE_POWER_LEVELS-1);
		//[/ExpSys]

		for (currank=FORCE_LEVEL_1;currank<=uiRank[curpower].uiForcePowersRank;currank++)
		{	// Check on this force power
			if (uiRank[curpower].uiForcePowersRank>0)
			{	// Do not charge the player for the one freebie in jump, or if there is one in saber.
				//[ExpSys]
				// don't automatically give Force jump
				if  (	//(curpower == FP_LEVITATION && currank == FORCE_LEVEL_1) ||
				//[/ExpSys]
						(curpower == FP_SABER_OFFENSE && currank == FORCE_LEVEL_1 && ui_freeSaber.integer) ||
						(curpower == FP_SABER_DEFENSE && currank == FORCE_LEVEL_1 && ui_freeSaber.integer) )
				{
					// Do nothing (written this way for clarity)
				}
				else
				{	// Check if we can accrue the cost of this power.
					if (bgForcePowerCost[curpower][currank] > uiForceAvailable)
					{	// We can't afford this power.  Break to the next one.
						// Remove this power from the player's roster.
						uiRank[curpower].uiForcePowersRank = currank-1;
						break;
					}
					else
					{	// Sure we can afford it.
						uiForceUsed += bgForcePowerCost[curpower][currank];
						uiForceAvailable -= bgForcePowerCost[curpower][currank];
					}
				}
			}
		}
	}
	for(i=0;i<42;i++)
	{
		prevRank[i]=uiRank[i];
	}
}

//Mostly parts of other functions merged into one another.
//Puts the current UI stuff into a string, legalizes it, and then reads it back out.
void UI_ReadLegalForce(void)
{
	char fcfString[512];
	char forceStringValue[4];
	int strPlace = 0;
	int forcePlace = 0;
	int i = 0;
	char singleBuf[64];
	char info[MAX_INFO_VALUE];
	int c = 0;
	int iBuf = 0;
	int forcePowerRank = 0;
	int currank = 0;
	int forceTeam = 0;
	qboolean updateForceLater = qfalse;

	//First, stick them into a string.
	Com_sprintf(fcfString, sizeof(fcfString), "%i-%i-", uiForceRank, uiForceSide);
	strPlace = strlen(fcfString);

	//[ExpSys]
	//added sanity check so we don't overflow fcfString.
	while (forcePlace < NUM_TOTAL_SKILLS && strPlace < 512)
	//while (forcePlace < NUM_FORCE_POWERS)
	//[/ExpSys]
	{
		Com_sprintf(forceStringValue, sizeof(forceStringValue), "%i", uiRank[forcePlace].uiForcePowersRank);
		//Just use the force digit even if multiple digits. Shouldn't be longer than 1.
		fcfString[strPlace] = forceStringValue[0];
		strPlace++;
		forcePlace++;
	}
	fcfString[strPlace] = '\n';
	fcfString[strPlace+1] = 0;

	info[0] = '\0';
	trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));

	if (atoi( Info_ValueForKey( info, "g_forceBasedTeams" ) ))
	{
		switch((int)(trap_Cvar_VariableValue("ui_myteam")))
		{
		case TEAM_RED:
			forceTeam = FORCE_DARKSIDE;
			break;
		case TEAM_BLUE:
			forceTeam = FORCE_LIGHTSIDE;
			break;
		default:
			break;
		}
	}
	//Second, legalize them.
	if (!BG_LegalizedForcePowers(fcfString, uiMaxRank, ui_freeSaber.integer, forceTeam, atoi( Info_ValueForKey( info, "g_gametype" )), 0))
	{ //if they were illegal, we should refresh them.
		updateForceLater = qtrue;
	}

	//Lastly, put them back into the UI storage from the legalized string
	i = 0;

	while (fcfString[i] && fcfString[i] != '-')
	{
		singleBuf[c] = fcfString[i];
		c++;
		i++;
	}
	singleBuf[c] = 0;
	c = 0;
	i++;

	iBuf = atoi(singleBuf);

	if (iBuf > uiMaxRank || iBuf < 0)
	{ //this force config uses a rank level higher than our currently restricted level.. so we can't use it
	  //FIXME: Print a message indicating this to the user
	//	return;
	}

	uiForceRank = iBuf;

	while (fcfString[i] && fcfString[i] != '-')
	{
		singleBuf[c] = fcfString[i];
		c++;
		i++;
	}
	singleBuf[c] = 0;
	c = 0;
	i++;

	uiForceSide = atoi(singleBuf);

	if (uiForceSide != FORCE_LIGHTSIDE &&
		uiForceSide != FORCE_DARKSIDE)
	{
		uiForceSide = FORCE_LIGHTSIDE;
		return;
	}

	//clear out the existing powers
	while (c < NUM_FORCE_POWERS)
	{
		uiRank[c].uiForcePowersRank = 0;
		c++;
	}
	uiForceUsed = 0;
	//[ExpSys]
	uiForceAvailable = uiMaxRank;
	//uiForceAvailable = forceMasteryPoints[uiForceRank];
	//[/ExpSys]
	gTouchedForce = qtrue;

	//[ExpSys]
	for (c=0; fcfString[i] && c < NUM_TOTAL_SKILLS;c++,i++)
	//for (c=0;fcfString[i]&&c<NUM_FORCE_POWERS;c++,i++)
	//[/ExpSys]
	{
		singleBuf[0] = fcfString[i];
		singleBuf[1] = 0;
		iBuf = atoi(singleBuf);	// So, that means that Force Power "c" wants to be set to rank "iBuf".
		
		if (iBuf < 0)
		{
			iBuf = 0;
		}

		forcePowerRank = iBuf;

		if (forcePowerRank > FORCE_LEVEL_3 || forcePowerRank < 0)
		{ //err..  not correct
			continue;  // skip this power
		}

		//[ForceSys]
		/*
		if (uiForcePowerDarkLight[c] && uiForcePowerDarkLight[c] != uiForceSide)
		{ //Apparently the user has crafted a force config that has powers that don't fit with the config's side.
			continue;  // skip this power
		}
		*/
		//[/ForceSys]

		// Accrue cost for each assigned rank for this power.
		for (currank=FORCE_LEVEL_1;currank<=forcePowerRank;currank++)
		{	
			if (bgForcePowerCost[c][currank] > uiForceAvailable)
			{	// Break out, we can't afford any more power.
				break;
			}
			// Pay for this rank of this power.
			uiForceUsed += bgForcePowerCost[c][currank];
			uiForceAvailable -= bgForcePowerCost[c][currank];

			uiRank[c].uiForcePowersRank++;
		}
	}

	//[ExpSys]
	/* don't automatically give Force jump
	if (uiRank[FP_LEVITATION].uiForcePowersRank < 1)
	{
		uiRank[FP_LEVITATION].uiForcePowersRank=1;
	}
	*/
	//[/ExpSys]
	if (uiRank[FP_SABER_OFFENSE].uiForcePowersRank < 1 && ui_freeSaber.integer)
	{
		uiRank[FP_SABER_OFFENSE].uiForcePowersRank=1;
	}
	if (uiRank[FP_SABER_DEFENSE].uiForcePowersRank < 1 && ui_freeSaber.integer)
	{
		uiRank[FP_SABER_DEFENSE].uiForcePowersRank=1;
	}

	UpdateForceUsed();

	if (updateForceLater)
	{
		gTouchedForce = qtrue;
		UI_UpdateClientForcePowers(NULL);
	}
}

void UI_UpdateForcePowers()
{
	char *forcePowers = UI_Cvar_VariableString("forcepowers");
	char readBuf[256];
	int i = 0, i_f = 0, i_r = 0;

	uiForceSide = 0;

	if (forcePowers && forcePowers[0])
	{
		while (forcePowers[i])
		{
			i_r = 0;

			while (forcePowers[i] && forcePowers[i] != '-' && i_r < 255)
			{
				readBuf[i_r] = forcePowers[i];
				i_r++;
				i++;
			}
			readBuf[i_r] = '\0';
			if (i_r >= 255 || !forcePowers[i] || forcePowers[i] != '-')
			{
				uiForceSide = 0;
				goto validitycheck;
			}
			uiForceRank = atoi(readBuf);
			i_r = 0;

			if (uiForceRank > uiMaxRank)
			{
				uiForceRank = uiMaxRank;
			}

			i++;

			while (forcePowers[i] && forcePowers[i] != '-' && i_r < 255)
			{
				readBuf[i_r] = forcePowers[i];
				i_r++;
				i++;
			}
			readBuf[i_r] = '\0';
			if (i_r >= 255 || !forcePowers[i] || forcePowers[i] != '-')
			{
				uiForceSide = 0;
				goto validitycheck;
			}
			uiForceSide = atoi(readBuf);
			i_r = 0;

			i++;

			i_f = FP_HEAL;

			while (forcePowers[i] && i_f < NUM_FORCE_POWERS)
			{
				readBuf[0] = forcePowers[i];
				readBuf[1] = '\0';
				uiRank[i_f].uiForcePowersRank = atoi(readBuf);

				//[ExpSys]
				// don't automatically give Force jump
				/*
				if (i_f == FP_LEVITATION &&
					uiRank[i_f].uiForcePowersRank < 1)
				{
					uiRank[i_f].uiForcePowersRank = 1;
				}
				*/
				//[/ExpSys]

				if (i_f == FP_SABER_OFFENSE &&
					uiRank[i_f].uiForcePowersRank < 1 &&
					ui_freeSaber.integer)
				{
					uiRank[i_f].uiForcePowersRank = 1;
				}

				if (i_f == FP_SABER_DEFENSE &&
					uiRank[i_f].uiForcePowersRank < 1 &&
					ui_freeSaber.integer)
				{
					uiRank[i_f].uiForcePowersRank = 1;
				}

				i_f++;
				i++;
			}

			if (i_f < NUM_FORCE_POWERS)
			{ //info for all the powers wasn't there..
				uiForceSide = 0;
				goto validitycheck;
			}
			i++;
		}
	}

validitycheck:

	if (!uiForceSide)
	{
		uiForceSide = 1;
		uiForceRank = 1;
		i = 0;
		while (i < NUM_FORCE_POWERS)
		{
			//[ExpSys]
			/* don't automatically give levitation anymore.
			if (i == FP_LEVITATION)
			{
				uiRank[i].uiForcePowersRank = 1;
			}
			else if (i == FP_SABER_OFFENSE && ui_freeSaber.integer)
			*/
			if (i == FP_SABER_OFFENSE && ui_freeSaber.integer)
			//[/ExpSys]
			{
				uiRank[i].uiForcePowersRank = 1;
			}
			else if (i == FP_SABER_DEFENSE && ui_freeSaber.integer)
			{
				uiRank[i].uiForcePowersRank = 1;
			}
			else
			{
				uiRank[i].uiForcePowersRank = 0;
			}

			i++;
		}

		UI_UpdateClientForcePowers(NULL);
	}

	UpdateForceUsed();
}
extern int	uiSkinColor;
extern int	uiHoldSkinColor;

qboolean UI_SkinColor_HandleKey(int flags, float *special, int key, int num, int min, int max, int type) 
{
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) 
	{
		int i = num;

		if (key == A_MOUSE2) {
			i--;
		} else {
			i++;
		}

		if (i < min) {
			i = max;
		} else if (i > max) {
			i = min;
		}

		num = i;

		uiSkinColor = num;

		uiHoldSkinColor = uiSkinColor;

		UI_FeederSelection(FEEDER_Q3HEADS, uiInfo.q3SelectedHead, NULL);

		return qtrue;
	}
	return qfalse;
}

qboolean UI_ForceSide_HandleKey(int flags, float *special, int key, int num, int min, int max, int type) 
{
	char info[MAX_INFO_VALUE];

	info[0] = '\0';
	trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));

	if (atoi( Info_ValueForKey( info, "g_forceBasedTeams" ) ))
	{
		switch((int)(trap_Cvar_VariableValue("ui_myteam")))
		{
		case TEAM_RED:
			return qfalse;
		case TEAM_BLUE:
			return qfalse;
		default:
			break;
		}
	}

	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) 
	{
		int i = num;
		//[ForceSys]
		//int x = 0;
		//[/ForceSys]

		//update the feeder item selection, it might be different depending on side
		Menu_SetFeederSelection(NULL, FEEDER_FORCECFG, 0, NULL);

		if (key == A_MOUSE2)
		{
			i--;
		}
		else
		{
			i++;
		}

		if (i < min)
		{
			i = max;
		}
		else if (i > max)
		{
			i = min;
		}

		num = i;

		uiForceSide = num;

		// Resetting power ranks based on if light or dark side is chosen
		//[ForceSys]
		/*
		while (x < NUM_FORCE_POWERS)
		{
			if (uiForcePowerDarkLight[x] && uiForceSide != uiForcePowerDarkLight[x])
			{
				uiRank[x].uiForcePowersRank = 0;
			}
			x++;
		}
		*/
		//[/ForceSys]

		UpdateForceUsed();

		gTouchedForce = qtrue;
		return qtrue;
	}
	return qfalse;
}

qboolean UI_JediNonJedi_HandleKey(int flags, float *special, int key, int num, int min, int max, int type) 
{
	char info[MAX_INFO_VALUE];

	info[0] = '\0';
	trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));

	if ( !UI_TrueJediEnabled() )
	{//true jedi mode is not set
		return qfalse;
	}

	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) 
	{
		int i = num;
		int x = 0;

		if (key == A_MOUSE2)
		{
			i--;
		}
		else
		{
			i++;
		}

		if (i < min)
		{
			i = max;
		}
		else if (i > max)
		{
			i = min;
		}

		num = i;

		uiJediNonJedi = num;

		// Resetting power ranks based on if light or dark side is chosen
		if ( !num )
		{//not a jedi?
			int myTeam = (int)(trap_Cvar_VariableValue("ui_myteam"));
			while ( x < NUM_FORCE_POWERS )
			{//clear all force powers
				uiRank[x].uiForcePowersRank = 0;
				x++;
			}
			if ( myTeam != TEAM_SPECTATOR )
			{
				UI_UpdateClientForcePowers(UI_TeamName(myTeam));//will cause him to respawn, if it's been 5 seconds since last one
			}
			else
			{
				UI_UpdateClientForcePowers(NULL);//just update powers
			}
		}
		else if ( num )
		{//a jedi, set the minimums, hopefuly they know to set the rest!
			if ( uiRank[FP_LEVITATION].uiForcePowersRank < FORCE_LEVEL_1 )
			{//force jump 1 minimum
				uiRank[FP_LEVITATION].uiForcePowersRank = FORCE_LEVEL_1;
			}
			if ( uiRank[FP_SABER_OFFENSE].uiForcePowersRank < FORCE_LEVEL_1 )
			{//saber attack 1, minimum
				uiRank[FP_SABER_OFFENSE].uiForcePowersRank = FORCE_LEVEL_1;
			}
		}

		UpdateForceUsed();

		gTouchedForce = qtrue;
		return qtrue;
	}
	return qfalse;
}

qboolean UI_ForceMaxRank_HandleKey(int flags, float *special, int key, int num, int min, int max, int type) 
{//racc - handle key presses for the Force Master Rank menu item.
	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER) 
	{
		int i = num;

		if (key == A_MOUSE2) {
			i--;
		} else {
			i++;
		}

		if (i < min) {
			i = max;
		} else if (i > max){
			i = min;
		}

		num = i;

		uiMaxRank = num;

		trap_Cvar_Set( "g_maxForceRank", va("%i", num));

		// The update force used will remove overallocated powers automatically.
		UpdateForceUsed();

		gTouchedForce = qtrue;

		return qtrue;
	}
	return qfalse;
}


// This function will either raise or lower a power by one rank.
qboolean UI_ForcePowerRank_HandleKey(int flags, float *special, int key, int num, int min, int max, int type) 
{
	qboolean raising;

	if (key == A_MOUSE1 || key == A_MOUSE2 || key == A_ENTER || key == A_KP_ENTER || key == A_BACKSPACE) 
	{
		int forcepower, rank;

		//[ExpSys]
		if(type < UI_FORCE_RANK_JETPACK)
		{
			forcepower = (type - UI_FORCE_RANK)-1;
		}
		else
		{//use a different index shift for the addition skills
			forcepower = (type - UI_FORCE_RANK_JETPACK)+(UI_FORCE_RANK_SABERTHROW-UI_FORCE_RANK);
		}
		//this will give us the index as long as UI_FORCE_RANK is always one below the first force rank index
		//forcepower = (type-UI_FORCE_RANK)-1;

		//we now have a really maximum because some skills have less than 3 levels to them.
		max = NumberOfSkillRanks(forcepower);
		//[ExpSys]

		//the power is disabled on the server
		if(uiRank[forcepower].disabled)
		{
			return qtrue;
		}

		//[ExpSys]
		//Made Force Seeing Level 1 a pre-req to taking any additional force powers, except in the case of free sabers.
		//[StanceSelection]
		if (forcepower != FP_SEE 
			&& (forcepower < NUM_FORCE_POWERS 
				|| forcepower == NUM_FORCE_POWERS+SK_BLUESTYLE
				|| forcepower == NUM_FORCE_POWERS+SK_REDSTYLE
				|| forcepower == NUM_FORCE_POWERS+SK_PURPLESTYLE
				|| forcepower == NUM_FORCE_POWERS+SK_GREENSTYLE
				|| forcepower == NUM_FORCE_POWERS+SK_DUALSTYLE
				|| forcepower == NUM_FORCE_POWERS+SK_STAFFSTYLE)
			)
		//if (forcepower < NUM_FORCE_POWERS && forcepower != FP_SEE)
		//[/StanceSelection]
		{//force powers can't be bought without being force sensitive
			if (uiRank[FP_SEE].uiForcePowersRank < 1)
			{
				return qtrue;
			}
		}
		//[/ExpSys]

		// If we are not on the same side as a power, or if we are not of any rank at all.
		//[ForceSys]
		//allow players to use powers from both sides of the Force.
		/*
		if (uiForcePowerDarkLight[forcepower] && uiForceSide != uiForcePowerDarkLight[forcepower])
		{
			return qtrue;
		}
		else if (forcepower == FP_SABER_DEFENSE || forcepower == FP_SABERTHROW)
		*/
		//[StanceSelection]
		if (forcepower == FP_SABER_DEFENSE 
			|| forcepower == FP_SABERTHROW
			|| forcepower == NUM_FORCE_POWERS+SK_BLUESTYLE
			|| forcepower == NUM_FORCE_POWERS+SK_REDSTYLE
			|| forcepower == NUM_FORCE_POWERS+SK_PURPLESTYLE
			|| forcepower == NUM_FORCE_POWERS+SK_GREENSTYLE
			|| forcepower == NUM_FORCE_POWERS+SK_DUALSTYLE
			|| forcepower == NUM_FORCE_POWERS+SK_STAFFSTYLE)
		//if (forcepower == FP_SABER_DEFENSE || forcepower == FP_SABERTHROW)
		//[/StanceSelection]
		//[/ForceSys]
		{	// Saberdefend and saberthrow can't be bought if there is no saberattack
			if (uiRank[FP_SABER_OFFENSE].uiForcePowersRank < 1)
			{
				return qtrue;
			}
		}

		//[ExpSys]
		//make it possible to set zero level for force jump
		/*
		if (type == UI_FORCE_RANK_LEVITATION)
		{
			min += 1;
		}
		*/
		//[/ExpSys]
		if (type == UI_FORCE_RANK_SABERATTACK && ui_freeSaber.integer)
		{
			min += 1;
		}
		if (type == UI_FORCE_RANK_SABERDEFEND && ui_freeSaber.integer)
		{
			min += 1;
		}

		if (key == A_MOUSE2 || key == A_BACKSPACE)
		{	// Lower a point.
			if (uiRank[forcepower].uiForcePowersRank<=min)
			{
				return qtrue;
			}
			raising = qfalse;
		}
		else
		{	// Raise a point.
			if (uiRank[forcepower].uiForcePowersRank>=max)
			{
				return qtrue;
			}
			raising = qtrue;
		}

		if (raising)
		{	// Check if we can accrue the cost of this power.
			rank = uiRank[forcepower].uiForcePowersRank+1;
			if (bgForcePowerCost[forcepower][rank] > uiForceAvailable)
			{	// We can't afford this power.  Abandon ship.
				return qtrue;
			}
			else
			{	// Sure we can afford it.
				uiForceUsed += bgForcePowerCost[forcepower][rank];
				uiForceAvailable -= bgForcePowerCost[forcepower][rank];
				uiRank[forcepower].uiForcePowersRank=rank;
			}
		}
		else
		{	// Lower the point.
			rank = uiRank[forcepower].uiForcePowersRank;
			uiForceUsed -= bgForcePowerCost[forcepower][rank];
			uiForceAvailable += bgForcePowerCost[forcepower][rank];
			uiRank[forcepower].uiForcePowersRank--;
		}

		UpdateForceUsed();

		gTouchedForce = qtrue;

		return qtrue;
	}
	return qfalse;
}


int gCustRank = 0;
int gCustSide = 0;

int gCustPowersRank[NUM_FORCE_POWERS] = {
	0,//FP_HEAL = 0,//instant
	1,//FP_LEVITATION,//hold/duration, this one defaults to 1 (gives a free point)
	0,//FP_SPEED,//duration
	0,//FP_PUSH,//hold/duration
	0,//FP_PULL,//hold/duration
	0,//FP_TELEPATHY,//instant
	0,//FP_GRIP,//hold/duration
	0,//FP_LIGHTNING,//hold/duration
	0,//FP_RAGE,//duration
	0,//FP_PROTECT,
	0,//FP_ABSORB,
	0,//FP_TEAM_HEAL,
	0,//FP_TEAM_FORCE,
	0,//FP_DRAIN,
	0,//FP_SEE,
	0,//FP_SABER_OFFENSE,
	0,//FP_SABER_DEFENSE,
	0//FP_SABERTHROW,
};

/*
=================
UI_ForceConfigHandle
=================
*/
void UI_ForceConfigHandle( int oldindex, int newindex )
{
	fileHandle_t f;
	int len = 0;
	int i = 0;
	int c = 0;
	int iBuf = 0, forcePowerRank, currank;
	char fcfBuffer[8192];
	char singleBuf[64];
	char info[MAX_INFO_VALUE];
	int forceTeam = 0;

	if (oldindex == 0)
	{ //switching out from custom config, so first shove the current values into the custom storage
		i = 0;

		while (i < NUM_FORCE_POWERS)
		{
			gCustPowersRank[i] = uiRank[i].uiForcePowersRank;
			i++;
		}
		gCustRank = uiForceRank;
		gCustSide = uiForceSide;
	}

	if (newindex == 0)
	{ //switching back to custom, shove the values back in from the custom storage
		i = 0;
		uiForceUsed = 0;
		gTouchedForce = qtrue;

		while (i < NUM_FORCE_POWERS)
		{
			uiRank[i].uiForcePowersRank = gCustPowersRank[i];
			uiForceUsed += uiRank[i].uiForcePowersRank;
			i++;
		}
		uiForceRank = gCustRank;
		uiForceSide = gCustSide;

		UpdateForceUsed();
		return;
	}

	//If we made it here, we want to load in a new config
	if (uiForceSide == FORCE_LIGHTSIDE)
	{ //we should only be displaying lightside configs, so.. look in the light folder
		newindex += uiInfo.forceConfigLightIndexBegin;
		if (newindex >= uiInfo.forceConfigCount)
		{
			return;
		}
		len = trap_FS_FOpenFile(va("forcecfg/light/%s.fcf", uiInfo.forceConfigNames[newindex]), &f, FS_READ);
	}
	else
	{ //else dark
		newindex += uiInfo.forceConfigDarkIndexBegin;
		if (newindex >= uiInfo.forceConfigCount || newindex > uiInfo.forceConfigLightIndexBegin)
		{ //dark gets read in before light
			return;
		}
		len = trap_FS_FOpenFile(va("forcecfg/dark/%s.fcf", uiInfo.forceConfigNames[newindex]), &f, FS_READ);
	}

	if (len <= 0)
	{ //This should not have happened. But, before we quit out, attempt searching the other light/dark folder for the file.
		if (uiForceSide == FORCE_LIGHTSIDE)
		{
			len = trap_FS_FOpenFile(va("forcecfg/dark/%s.fcf", uiInfo.forceConfigNames[newindex]), &f, FS_READ);
		}
		else
		{
			len = trap_FS_FOpenFile(va("forcecfg/light/%s.fcf", uiInfo.forceConfigNames[newindex]), &f, FS_READ);
		}

		if (len <= 0)
		{ //still failure? Oh well.
			return;
		}
	}

	if (len >= 8192)
	{
		return;
	}

	trap_FS_Read(fcfBuffer, len, f);
	fcfBuffer[len] = 0;
	trap_FS_FCloseFile(f);

	i = 0;

	info[0] = '\0';
	trap_GetConfigString(CS_SERVERINFO, info, sizeof(info));

	if (atoi( Info_ValueForKey( info, "g_forceBasedTeams" ) ))
	{
		switch((int)(trap_Cvar_VariableValue("ui_myteam")))
		{
		case TEAM_RED:
			forceTeam = FORCE_DARKSIDE;
			break;
		case TEAM_BLUE:
			forceTeam = FORCE_LIGHTSIDE;
			break;
		default:
			break;
		}
	}

	BG_LegalizedForcePowers(fcfBuffer, uiMaxRank, ui_freeSaber.integer, forceTeam, atoi( Info_ValueForKey( info, "g_gametype" )), 0);
	//legalize the config based on the max rank

	//now that we're done with the handle, it's time to parse our force data out of the string
	//we store strings in rank-side-xxxxxxxxx format (where the x's are individual force power levels)
	while (fcfBuffer[i] && fcfBuffer[i] != '-')
	{
		singleBuf[c] = fcfBuffer[i];
		c++;
		i++;
	}
	singleBuf[c] = 0;
	c = 0;
	i++;

	iBuf = atoi(singleBuf);

	if (iBuf > uiMaxRank || iBuf < 0)
	{ //this force config uses a rank level higher than our currently restricted level.. so we can't use it
	  //FIXME: Print a message indicating this to the user
		return;
	}

	uiForceRank = iBuf;

	while (fcfBuffer[i] && fcfBuffer[i] != '-')
	{
		singleBuf[c] = fcfBuffer[i];
		c++;
		i++;
	}
	singleBuf[c] = 0;
	c = 0;
	i++;

	uiForceSide = atoi(singleBuf);

	if (uiForceSide != FORCE_LIGHTSIDE &&
		uiForceSide != FORCE_DARKSIDE)
	{
		uiForceSide = FORCE_LIGHTSIDE;
		return;
	}

	//clear out the existing powers
	while (c < NUM_FORCE_POWERS)
	{
		/*
		if (c==FP_LEVITATION)
		{
			uiRank[c].uiForcePowersRank=1;
		}
		else if (c==FP_SABER_OFFENSE && ui_freeSaber.integer)
		{
			uiRank[c].uiForcePowersRank=1;
		}
		else if (c==FP_SABER_DEFENSE && ui_freeSaber.integer)
		{
			uiRank[c].uiForcePowersRank=1;
		}
		else
		{
			uiRank[c].uiForcePowersRank = 0;
		}
		*/
		//rww - don't need to do these checks. Just trust whatever the saber config says.
		uiRank[c].uiForcePowersRank = 0;
		c++;
	}
	uiForceUsed = 0;
	//[ExpSys]
	uiForceAvailable = uiMaxRank;
	//uiForceAvailable = forceMasteryPoints[uiForceRank];
	//[/ExpSys]
	gTouchedForce = qtrue;

	for (c=0;fcfBuffer[i]&&c<NUM_FORCE_POWERS;c++,i++)
	{
		singleBuf[0] = fcfBuffer[i];
		singleBuf[1] = 0;
		iBuf = atoi(singleBuf);	// So, that means that Force Power "c" wants to be set to rank "iBuf".
		
		if (iBuf < 0)
		{
			iBuf = 0;
		}

		forcePowerRank = iBuf;

		if (forcePowerRank > FORCE_LEVEL_3 || forcePowerRank < 0)
		{ //err..  not correct
			continue;  // skip this power
		}

		//[ForceSys]
		/*
		if (uiForcePowerDarkLight[c] && uiForcePowerDarkLight[c] != uiForceSide)
		{ //Apparently the user has crafted a force config that has powers that don't fit with the config's side.
			continue;  // skip this power
		}
		*/
		//[/ForceSys]

		// Accrue cost for each assigned rank for this power.
		for (currank=FORCE_LEVEL_1;currank<=forcePowerRank;currank++)
		{	
			if (bgForcePowerCost[c][currank] > uiForceAvailable)
			{	// Break out, we can't afford any more power.
				break;
			}
			// Pay for this rank of this power.
			uiForceUsed += bgForcePowerCost[c][currank];
			uiForceAvailable -= bgForcePowerCost[c][currank];

			uiRank[c].uiForcePowersRank++;
		}
	}

	//[ExpSys]
	// don't automatically give Force jump
	/*
	if (uiRank[FP_LEVITATION].uiForcePowersRank < 1)
	{
		uiRank[FP_LEVITATION].uiForcePowersRank=1;
	}
	*/
	//[/ExpSys]
	if (uiRank[FP_SABER_OFFENSE].uiForcePowersRank < 1 && ui_freeSaber.integer)
	{
		uiRank[FP_SABER_OFFENSE].uiForcePowersRank=1;
	}
	if (uiRank[FP_SABER_DEFENSE].uiForcePowersRank < 1 && ui_freeSaber.integer)
	{
		uiRank[FP_SABER_DEFENSE].uiForcePowersRank=1;
	}

	UpdateForceUsed();
}
