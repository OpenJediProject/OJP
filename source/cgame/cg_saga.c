// Copyright (C) 2000-2002 Raven Software, Inc.
//
/*****************************************************************************
 * name:		cg_siege.c
 *
 * desc:		Clientgame-side module for Siege gametype.
 *
 *
 *****************************************************************************/
#include "cg_local.h"
#include "bg_saga.h"

int cgSiegeRoundState = 0;
int cgSiegeRoundTime = 0;

static char		team1[512];
static char		team2[512];

int			team1Timed = 0;
int			team2Timed = 0;

int			cgSiegeTeam1PlShader = 0;
int			cgSiegeTeam2PlShader = 0;

//[TrueView]
#define		MAX_TRUEVIEW_INFO_SIZE					8192
char		true_view_info[MAX_TRUEVIEW_INFO_SIZE];
int			true_view_valid;
//[/TrueView]

static char cgParseObjectives[MAX_SIEGE_INFO_SIZE];

extern void CG_LoadCISounds(clientInfo_t *ci, qboolean modelloaded); //cg_players.c

void CG_DrawSiegeMessage( const char *str, int objectiveScreen );
void CG_DrawSiegeMessageNonMenu( const char *str );
void CG_SiegeBriefingDisplay(int team, int dontshow);

//[SIEGECVARFIX]
char *siege_Str(void) {
	static int cur;
	static char strings[1024][256];
	return strings[cur++];
}

char **ui_siegeStruct;

/*
===============
siege_Cvar_Set
replacement for the trap to reduce cvar usage
===============
*/
void siege_Cvar_Set( char *cvarName, char *value )
{
	char **tmp;
	char ui_siegeInfo[MAX_STRING_CHARS];
	int i;
	if(!ui_siegeStruct)
	{
		trap_Cvar_VariableStringBuffer( "ui_siegeInfo", ui_siegeInfo, MAX_STRING_CHARS );
		sscanf(ui_siegeInfo,"%p",&ui_siegeStruct);
		if(!ui_siegeStruct) return;
	}
	for(tmp=ui_siegeStruct;*tmp;tmp+=2)
	{
		if(*tmp && !strcmp(*tmp,cvarName)) { //cvar already exists, overwrite existing value
			Q_strncpyz(*(tmp+1),value,256);  //:nervou
			return;
		}
	}
	i = tmp - ui_siegeStruct;
	ui_siegeStruct[i] = siege_Str();
	Q_strncpyz( ui_siegeStruct[i], cvarName, 256 );
	ui_siegeStruct[i+1] = siege_Str();
	Q_strncpyz( ui_siegeStruct[i+1], value, 256 );
	ui_siegeStruct[i+2] = 0;
	return;
}

void siege_Cvar_VariableStringBuffer( char *var_name, char *buffer, int bufsize )
{
	char **tmp;
	char ui_siegeInfo[MAX_STRING_CHARS];
	if(!ui_siegeStruct)
	{
		trap_Cvar_VariableStringBuffer( "ui_siegeInfo", ui_siegeInfo, MAX_STRING_CHARS );
		sscanf(ui_siegeInfo,"%p",&ui_siegeStruct);
		if(!ui_siegeStruct) return;
	}
	for(tmp=ui_siegeStruct;*tmp;tmp+=2)
	{
		if(*tmp && !strcmp(*tmp,var_name)) {
			Q_strncpyz(buffer,*(tmp+1),bufsize);
			return;
		}
	}
	trap_Cvar_VariableStringBuffer( var_name, buffer, bufsize );
	return;
}

void siegecvarlist(void)
{
	char **tmp;
	char ui_siegeInfo[MAX_STRING_CHARS];
	if(!ui_siegeStruct)
	{
		trap_Cvar_VariableStringBuffer( "ui_siegeInfo", ui_siegeInfo, MAX_STRING_CHARS );
		sscanf(ui_siegeInfo,"%p",&ui_siegeStruct);
		if(!ui_siegeStruct) return;
	}
	for(tmp=ui_siegeStruct;*tmp;tmp+=2)
	{
		CG_Printf("\t%s \"%s\"\n",*tmp,*(tmp+1));
	}
	CG_Printf("%d total cvars\n",(tmp-ui_siegeStruct)/2);
}
//[/SIEGECVARFIX]

void CG_PrecacheSiegeObjectiveAssetsForTeam(int myTeam)
{
	char			teamstr[64];
	char			objstr[256];
	char			foundobjective[MAX_SIEGE_INFO_SIZE];

	if (!siege_valid)
	{
		CG_Error("Siege data does not exist on client!\n");
		return;
	}

	if (myTeam == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{
		int i = 1;
		while (i < 32)
		{ //eh, just try 32 I guess
			Com_sprintf(objstr, sizeof(objstr), "Objective%i", i);

			if (BG_SiegeGetValueGroup(cgParseObjectives, objstr, foundobjective))
			{
				char str[MAX_QPATH];

				if (BG_SiegeGetPairedValue(foundobjective, "sound_team1", str))
				{
					trap_S_RegisterSound(str);
				}
				if (BG_SiegeGetPairedValue(foundobjective, "sound_team2", str))
				{
					trap_S_RegisterSound(str);
				}
				if (BG_SiegeGetPairedValue(foundobjective, "objgfx", str))
				{
					trap_R_RegisterShaderNoMip(str);
				}
				if (BG_SiegeGetPairedValue(foundobjective, "mapicon", str))
				{
					trap_R_RegisterShaderNoMip(str);
				}
				if (BG_SiegeGetPairedValue(foundobjective, "litmapicon", str))
				{
					trap_R_RegisterShaderNoMip(str);
				}
				if (BG_SiegeGetPairedValue(foundobjective, "donemapicon", str))
				{
					trap_R_RegisterShaderNoMip(str);
				}
			}
			else
			{ //no more
				break;
			}
			i++;
		}
	}
}

void CG_PrecachePlayersForSiegeTeam(int team)
{
	siegeTeam_t *stm;
	int i = 0;

	stm = BG_SiegeFindThemeForTeam(team);

	if (!stm)
	{ //invalid team/no theme for team?
		return;
	}

	while (i < stm->numClasses)
	{
		siegeClass_t *scl = stm->classes[i];

		if (scl->forcedModel[0])
		{
			clientInfo_t fake;

			memset(&fake, 0, sizeof(fake));
			strcpy(fake.modelName, scl->forcedModel);

			trap_R_RegisterModel(va("models/players/%s/model.glm", scl->forcedModel));
			if (scl->forcedSkin[0])
			{
				trap_R_RegisterSkin(va("models/players/%s/model_%s.skin", scl->forcedModel, scl->forcedSkin));
				strcpy(fake.skinName, scl->forcedSkin);
			}
			else
			{
				strcpy(fake.skinName, "default");
			}

			//precache the sounds for the model...
			CG_LoadCISounds(&fake, qtrue);
		}

		i++;
	}
}

void CG_InitSiegeMode(void)
{
	char			levelname[MAX_QPATH];
	char			btime[1024];
	char			teams[2048];
	char			teamInfo[MAX_SIEGE_INFO_SIZE];
	int				len = 0;
	int				i = 0;
	int				j = 0;
	siegeClass_t		*cl;
	siegeTeam_t		*sTeam;
	fileHandle_t	f;
	char			teamIcon[128];

	if (cgs.gametype != GT_SIEGE)
	{
		goto failure;
	}

	Com_sprintf(levelname, sizeof(levelname), "%s\0", cgs.mapname);

	i = strlen(levelname)-1;

	while (i > 0 && levelname[i] && levelname[i] != '.')
	{
		i--;
	}

	if (!i)
	{
		goto failure;
	}

	levelname[i] = '\0'; //kill the ".bsp"

	Com_sprintf(levelname, sizeof(levelname), "%s.siege\0", levelname); 

	if (!levelname || !levelname[0])
	{
		goto failure;
	}

	len = trap_FS_FOpenFile(levelname, &f, FS_READ);

	if (!f || len >= MAX_SIEGE_INFO_SIZE)
	{
		goto failure;
	}

	trap_FS_Read(siege_info, len, f);

	trap_FS_FCloseFile(f);

	siege_valid = 1;

	if (BG_SiegeGetValueGroup(siege_info, "Teams", teams))
	{
		char buf[1024];

		//[SIEGECVARFIX]
		siege_Cvar_VariableStringBuffer("cg_siegeTeam1", buf, 1024);
		//trap_Cvar_VariableStringBuffer("cg_siegeTeam1", buf, 1024);
		//[SIEGECVARFIX]
		if (buf[0] && Q_stricmp(buf, "none"))
		{
			strcpy(team1, buf);
		}
		else
		{
			BG_SiegeGetPairedValue(teams, "team1", team1);
		}

		if (team1[0] == '@')
		{ //it's a damn stringed reference.
			char b[256];
			trap_SP_GetStringTextString(team1+1, b, 256);
			trap_Cvar_Set("cg_siegeTeam1Name", b);
		}
		else
		{
			trap_Cvar_Set("cg_siegeTeam1Name", team1);
		}

		//[SIEGECVARFIX]
		siege_Cvar_VariableStringBuffer("cg_siegeTeam2", buf, 1024);
		//trap_Cvar_VariableStringBuffer("cg_siegeTeam2", buf, 1024);
		//[/SIEGECVARFIX]
		if (buf[0] && Q_stricmp(buf, "none"))
		{
			strcpy(team2, buf);
		}
		else
		{
			BG_SiegeGetPairedValue(teams, "team2", team2);
		}

		if (team2[0] == '@')
		{ //it's a damn stringed reference.
			char b[256];
			trap_SP_GetStringTextString(team2+1, b, 256);
			trap_Cvar_Set("cg_siegeTeam2Name", b);
		}
		else
		{
			trap_Cvar_Set("cg_siegeTeam2Name", team2);
		}
	}
	else
	{
		CG_Error("Siege teams not defined");
	}

	if (BG_SiegeGetValueGroup(siege_info, team1, teamInfo))
	{
		if (BG_SiegeGetPairedValue(teamInfo, "TeamIcon", teamIcon))
		{
			trap_Cvar_Set( "team1_icon", teamIcon);
		}

		if (BG_SiegeGetPairedValue(teamInfo, "Timed", btime))
		{
			team1Timed = atoi(btime)*1000;
			CG_SetSiegeTimerCvar ( team1Timed );
		}
		else
		{
			team1Timed = 0;
		}
	}
	else
	{
		CG_Error("No team entry for '%s'\n", team1);
	}

	if (BG_SiegeGetPairedValue(siege_info, "mapgraphic", teamInfo))
	{
		trap_Cvar_Set("siege_mapgraphic", teamInfo);
	}
	else
	{
		trap_Cvar_Set("siege_mapgraphic", "gfx/mplevels/siege1_hoth");
	}

	if (BG_SiegeGetPairedValue(siege_info, "missionname", teamInfo))
	{
		trap_Cvar_Set("siege_missionname", teamInfo);
	}
	else
	{
		trap_Cvar_Set("siege_missionname", " ");
	}

	if (BG_SiegeGetValueGroup(siege_info, team2, teamInfo))
	{
		if (BG_SiegeGetPairedValue(teamInfo, "TeamIcon", teamIcon))
		{
			trap_Cvar_Set( "team2_icon", teamIcon);
		}

		if (BG_SiegeGetPairedValue(teamInfo, "Timed", btime))
		{
			team2Timed = atoi(btime)*1000;
			CG_SetSiegeTimerCvar ( team2Timed );
		}
		else
		{
			team2Timed = 0;
		}
	}
	else
	{
		CG_Error("No team entry for '%s'\n", team2);
	}

	//Load the player class types
	BG_SiegeLoadClasses(NULL);

	if (!bgNumSiegeClasses)
	{ //We didn't find any?!
		CG_Error("Couldn't find any player classes for Siege");
	}

	//Now load the teams since we have class data.
	BG_SiegeLoadTeams();

	if (!bgNumSiegeTeams)
	{ //React same as with classes.
		CG_Error("Couldn't find any player teams for Siege");
	}

	//Get and set the team themes for each team. This will control which classes can be
	//used on each team.
	if (BG_SiegeGetValueGroup(siege_info, team1, teamInfo))
	{
		if (BG_SiegeGetPairedValue(teamInfo, "UseTeam", btime))
		{
			BG_SiegeSetTeamTheme(SIEGETEAM_TEAM1, btime);
		}
		if (BG_SiegeGetPairedValue(teamInfo, "FriendlyShader", btime))
		{
			cgSiegeTeam1PlShader = trap_R_RegisterShaderNoMip(btime);
		}
		else
		{
			cgSiegeTeam1PlShader = 0;
		}
	}
	if (BG_SiegeGetValueGroup(siege_info, team2, teamInfo))
	{
		if (BG_SiegeGetPairedValue(teamInfo, "UseTeam", btime))
		{
			BG_SiegeSetTeamTheme(SIEGETEAM_TEAM2, btime);
		}
		if (BG_SiegeGetPairedValue(teamInfo, "FriendlyShader", btime))
		{
			cgSiegeTeam2PlShader = trap_R_RegisterShaderNoMip(btime);
		}
		else
		{
			cgSiegeTeam2PlShader = 0;
		}
	}

	//Now go through the classes used by the loaded teams and try to precache
	//any forced models or forced skins.
	i = SIEGETEAM_TEAM1;

	while (i <= SIEGETEAM_TEAM2)
	{
		j = 0;
		sTeam = BG_SiegeFindThemeForTeam(i);

		if (!sTeam)
		{
			i++;
			continue;
		}

		//Get custom team shaders while we're at it.
		if (i == SIEGETEAM_TEAM1)
		{
			cgSiegeTeam1PlShader = sTeam->friendlyShader;
		}
		else if (i == SIEGETEAM_TEAM2)
		{
			cgSiegeTeam2PlShader = sTeam->friendlyShader;
		}

		while (j < sTeam->numClasses)
		{
			cl = sTeam->classes[j];

			if (cl->forcedModel[0])
			{ //This class has a forced model, so precache it.
				trap_R_RegisterModel(va("models/players/%s/model.glm", cl->forcedModel));

				if (cl->forcedSkin[0])
				{ //also has a forced skin, precache it.
					char *useSkinName;

					if (strchr(cl->forcedSkin, '|'))
					{//three part skin
						useSkinName = va("models/players/%s/|%s", cl->forcedModel, cl->forcedSkin);
					}
					else
					{
						useSkinName = va("models/players/%s/model_%s.skin", cl->forcedModel, cl->forcedSkin);
					}

					trap_R_RegisterSkin(useSkinName);
				}
			}
			
			j++;
		}
		i++;
	}

	//precache saber data for classes that use sabers on both teams
	BG_PrecacheSabersForSiegeTeam(SIEGETEAM_TEAM1);
	BG_PrecacheSabersForSiegeTeam(SIEGETEAM_TEAM2);

	CG_PrecachePlayersForSiegeTeam(SIEGETEAM_TEAM1);
	CG_PrecachePlayersForSiegeTeam(SIEGETEAM_TEAM2);

	CG_PrecachePlayersForSiegeTeam(SIEGETEAM_TEAM1);
	CG_PrecachePlayersForSiegeTeam(SIEGETEAM_TEAM2);

	CG_PrecacheSiegeObjectiveAssetsForTeam(SIEGETEAM_TEAM1);
	CG_PrecacheSiegeObjectiveAssetsForTeam(SIEGETEAM_TEAM2);

	return;
failure:
	siege_valid = 0;
}

static char CGAME_INLINE *CG_SiegeObjectiveBuffer(int team, int objective)
{
	static char buf[8192];
	char teamstr[1024];

	if (team == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{ //found the team group
		if (BG_SiegeGetValueGroup(cgParseObjectives, va("Objective%i", objective), buf))
		{ //found the objective group
			return buf;
		}
	}

	return NULL;
}

void CG_ParseSiegeObjectiveStatus(const char *str)
{
	int i = 0;
	int	team = SIEGETEAM_TEAM1;
	char *cvarName;
	char *s;
	int objectiveNum = 0;

	if (!str || !str[0])
	{
		return;
	}

	while (str[i])
	{
		if (str[i] == '|')
		{ //switch over to team2, this is the next section
            team = SIEGETEAM_TEAM2;
			objectiveNum = 0;
		}
		else if (str[i] == '-')
		{
			objectiveNum++;
			i++;

			cvarName = va("team%i_objective%i", team, objectiveNum);
			if (str[i] == '1')
			{ //it's completed
				trap_Cvar_Set(cvarName, "1");
			}
			else
			{ //otherwise assume it is not
				trap_Cvar_Set(cvarName, "0");
			}

			s = CG_SiegeObjectiveBuffer(team, objectiveNum);
			if (s && s[0])
			{ //now set the description and graphic cvars to by read by the menu
				char buffer[8192];

				cvarName = va("team%i_objective%i_longdesc", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "objdesc", buffer))
				{
					trap_Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap_Cvar_Set(cvarName, "UNSPECIFIED");
				}

				cvarName = va("team%i_objective%i_gfx", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "objgfx", buffer))
				{
					trap_Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap_Cvar_Set(cvarName, "UNSPECIFIED");
				}

				cvarName = va("team%i_objective%i_mapicon", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "mapicon", buffer))
				{
					trap_Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap_Cvar_Set(cvarName, "UNSPECIFIED");
				}

				cvarName = va("team%i_objective%i_litmapicon", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "litmapicon", buffer))
				{
					trap_Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap_Cvar_Set(cvarName, "UNSPECIFIED");
				}

				cvarName = va("team%i_objective%i_donemapicon", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "donemapicon", buffer))
				{
					trap_Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap_Cvar_Set(cvarName, "UNSPECIFIED");
				}

				cvarName = va("team%i_objective%i_mappos", team, objectiveNum);
				if (BG_SiegeGetPairedValue(s, "mappos", buffer))
				{
					trap_Cvar_Set(cvarName, buffer);
				}
				else
				{
					trap_Cvar_Set(cvarName, "0 0 32 32");
				}
			}
		}
		i++;
	}

	if (cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_SPECTATOR)
	{ //update menu cvars
		CG_SiegeBriefingDisplay(cg.predictedPlayerState.persistant[PERS_TEAM], 1);
	}
}

void CG_SiegeRoundOver(centity_t *ent, int won)
{
	int				myTeam;
	char			teamstr[64];
	char			appstring[1024];
	char			soundstr[1024];
	int				success = 0;
	playerState_t	*ps = NULL;

	if (!siege_valid)
	{
		CG_Error("ERROR: Siege data does not exist on client!\n");
		return;
	}

	if (cg.snap)
	{ //this should always be true, if it isn't though use the predicted ps as a fallback
		ps = &cg.snap->ps;
	}
	else
	{
		ps = &cg.predictedPlayerState;
	}

	if (!ps)
	{
		assert(0);
		return;
	}

	myTeam = ps->persistant[PERS_TEAM];

	if (myTeam == TEAM_SPECTATOR)
	{
		return;
	}

	if (myTeam == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{
		if (won == myTeam)
		{
			success = BG_SiegeGetPairedValue(cgParseObjectives, "wonround", appstring);
		}
		else
		{
			success = BG_SiegeGetPairedValue(cgParseObjectives, "lostround", appstring);
		}

		if (success)
		{
			CG_DrawSiegeMessage(appstring, 0);
		}

		appstring[0] = 0;
		soundstr[0] = 0;

		if (myTeam == won)
		{
			Com_sprintf(teamstr, sizeof(teamstr), "roundover_sound_wewon");
		}
		else
		{
			Com_sprintf(teamstr, sizeof(teamstr), "roundover_sound_welost");
		}

		if (BG_SiegeGetPairedValue(cgParseObjectives, teamstr, appstring))
		{
			Com_sprintf(soundstr, sizeof(soundstr), appstring);
		}
		/*
		else
		{
			if (myTeam != won)
			{
				Com_sprintf(soundstr, sizeof(soundstr), DEFAULT_LOSE_ROUND);
			}
			else
			{
				Com_sprintf(soundstr, sizeof(soundstr), DEFAULT_WIN_ROUND);
			}
		}
		*/

		if (soundstr[0])
		{
			trap_S_StartLocalSound(trap_S_RegisterSound(soundstr), CHAN_ANNOUNCER);
		}
	}
}

void CG_SiegeGetObjectiveDescription(int team, int objective, char *buffer)
{
	char teamstr[1024];
	char objectiveStr[8192];

	buffer[0] = 0; //set to 0 ahead of time in case we fail to find the objective group/name

	if (team == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{ //found the team group
		if (BG_SiegeGetValueGroup(cgParseObjectives, va("Objective%i", objective), objectiveStr))
		{ //found the objective group
			//Parse the name right into the buffer.
			BG_SiegeGetPairedValue(objectiveStr, "goalname", buffer);
		}
	}
}

int CG_SiegeGetObjectiveFinal(int team, int objective )
{
	char finalStr[64];
	char teamstr[1024];
	char objectiveStr[8192];

	if (team == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{ //found the team group
		if (BG_SiegeGetValueGroup(cgParseObjectives, va("Objective%i", objective), objectiveStr))
		{ //found the objective group
			//Parse the name right into the buffer.
			BG_SiegeGetPairedValue(objectiveStr, "final", finalStr);
			return (atoi( finalStr ));
		}
	}
	return 0;
}

void CG_SiegeBriefingDisplay(int team, int dontshow)
{
	char			teamstr[64];
	char			briefing[8192];
	char			properValue[1024];
	char			objectiveDesc[1024];
	int				i = 1;
	int				useTeam = team;
	qboolean		primary = qfalse;

	if (!siege_valid)
	{
		return;
	}

	if (team == TEAM_SPECTATOR)
	{
		return;
	}

	if (team == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	if (useTeam != SIEGETEAM_TEAM1 && useTeam != SIEGETEAM_TEAM2)
	{ //This shouldn't be happening. But just fall back to team 2 anyway.
		useTeam = SIEGETEAM_TEAM2;
	}

	//[SIEGECVARFIX]
	siege_Cvar_Set(va("siege_primobj_inuse"), "0");
	//trap_Cvar_Set(va("siege_primobj_inuse"), "0");
	//[/SIEGECVARFIX]

	while (i < 16)
	{ //do up to 16 objectives I suppose
		//Get the value for this objective on this team
		//Now set the cvar for the menu to display.
		
		//primary = (CG_SiegeGetObjectiveFinal(useTeam, i)>-1)?qtrue:qfalse;
		primary = (CG_SiegeGetObjectiveFinal(useTeam, i)>0)?qtrue:qfalse;

		properValue[0] = 0;
		trap_Cvar_VariableStringBuffer(va("team%i_objective%i", useTeam, i), properValue, 1024);
		if (primary)
		{
			//[SIEGECVARFIX]
			siege_Cvar_Set(va("siege_primobj"), properValue);
			//trap_Cvar_Set(va("siege_primobj"), properValue);
			//[/SIEGECVARFIX]
		}
		else
		{
			//[SIEGECVARFIX]
			siege_Cvar_Set(va("siege_objective%i", i), properValue);
			//trap_Cvar_Set(va("siege_objective%i", i), properValue);
			//[/SIEGECVARFIX]
		}

		//Now set the long desc cvar for the menu to display.
		properValue[0] = 0;
		trap_Cvar_VariableStringBuffer(va("team%i_objective%i_longdesc", useTeam, i), properValue, 1024);
		if (primary)
		{
			//[SIEGECVARFIX]
			siege_Cvar_Set(va("siege_primobj_longdesc"), properValue);
			//trap_Cvar_Set(va("siege_primobj_longdesc"), properValue);
			//[/SIEGECVARFIX]
		}
		else
		{
			//[SIEGECVARFIX]
			siege_Cvar_Set(va("siege_objective%i_longdesc", i), properValue);
			//trap_Cvar_Set(va("siege_objective%i_longdesc", i), properValue);
			//[/SIEGECVARFIX]
		}

		//Now set the gfx cvar for the menu to display.
		properValue[0] = 0;
		trap_Cvar_VariableStringBuffer(va("team%i_objective%i_gfx", useTeam, i), properValue, 1024);
		if (primary)
		{
			//[SIEGECVARFIX]
			siege_Cvar_Set(va("siege_primobj_gfx"), properValue);
			//trap_Cvar_Set(va("siege_primobj_gfx"), properValue);
			//[/SIEGECVARFIX]
		}
		else
		{
			//[SIEGECVARFIX]
			siege_Cvar_Set(va("siege_objective%i_gfx", i), properValue);
			//trap_Cvar_Set(va("siege_objective%i_gfx", i), properValue);
			//[/SIEGECVARFIX]
		}

		//Now set the mapicon cvar for the menu to display.
		properValue[0] = 0;
		trap_Cvar_VariableStringBuffer(va("team%i_objective%i_mapicon", useTeam, i), properValue, 1024);
		if (primary)
		{
			//[SIEGECVARFIX]
			siege_Cvar_Set(va("siege_primobj_mapicon"), properValue);
			//trap_Cvar_Set(va("siege_primobj_mapicon"), properValue);
			//[/SIEGECVARFIX]
		}
		else
		{
			//[SIEGECVARFIX]
			siege_Cvar_Set(va("siege_objective%i_mapicon", i), properValue);
			//trap_Cvar_Set(va("siege_objective%i_mapicon", i), properValue);
			//[/SIEGECVARFIX]
		}

		//Now set the mappos cvar for the menu to display.
		properValue[0] = 0;
		trap_Cvar_VariableStringBuffer(va("team%i_objective%i_mappos", useTeam, i), properValue, 1024);
		if (primary)
		{
			//[SIEGECVARFIX]
			siege_Cvar_Set(va("siege_primobj_mappos"), properValue);
			//trap_Cvar_Set(va("siege_primobj_mappos"), properValue);
			//[/SIEGECVARFIX]
		}
		else
		{
			//[SIEGECVARFIX]
			siege_Cvar_Set(va("siege_objective%i_mappos", i), properValue);
			//trap_Cvar_Set(va("siege_objective%i_mappos", i), properValue);
			//[/SIEGECVARFIX]
		}

		//Now set the description cvar for the objective
		CG_SiegeGetObjectiveDescription(useTeam, i, objectiveDesc);

		if (objectiveDesc[0])
		{ //found a valid objective description
			if ( primary )
			{
				//[SIEGECVARFIX]
				siege_Cvar_Set(va("siege_primobj_desc"), objectiveDesc);
				//this one is marked not in use because it gets primobj
				siege_Cvar_Set(va("siege_objective%i_inuse", i), "0");
				siege_Cvar_Set(va("siege_primobj_inuse"), "1");
				//trap_Cvar_Set(va("siege_primobj_desc"), objectiveDesc);
				////this one is marked not in use because it gets primobj
				//trap_Cvar_Set(va("siege_objective%i_inuse", i), "0");
				//trap_Cvar_Set(va("siege_primobj_inuse"), "1");
				//[/SIEGECVARFIX]

				trap_Cvar_Set(va("team%i_objective%i_inuse", useTeam, i), "1");

			}
			else
			{
				//[SIEGECVARFIX]
				siege_Cvar_Set(va("siege_objective%i_desc", i), objectiveDesc);
				siege_Cvar_Set(va("siege_objective%i_inuse", i), "2");
				//trap_Cvar_Set(va("siege_objective%i_desc", i), objectiveDesc);
				//trap_Cvar_Set(va("siege_objective%i_inuse", i), "2");
				//[/SIEGECVARFIX]
				trap_Cvar_Set(va("team%i_objective%i_inuse", useTeam, i), "2");

			}
		}
		//[SIEGECVARFIX]
		else
		{ //didn't find one, so set the "inuse" cvar to 0 for the objective and mark it non-complete.
		  	properValue[0] = 0;
			trap_Cvar_VariableStringBuffer(va("team%i_objective%i", useTeam, i), properValue, 1024);
			if(properValue[0]) {
				//siege_Cvar_Set(va("siege_objective%i_inuse", i), "0");
				//siege_Cvar_Set(va("siege_objective%i", i), "0");
				trap_Cvar_Set(va("team%i_objective%i_inuse", useTeam, i), "0");
				trap_Cvar_Set(va("team%i_objective%i", useTeam, i), "0");
	
				//siege_Cvar_Set(va("siege_objective%i_mappos", i), "");
				trap_Cvar_Set(va("team%i_objective%i_mappos", useTeam, i), "");
				//siege_Cvar_Set(va("siege_objective%i_gfx", i), "");
				trap_Cvar_Set(va("team%i_objective%i_gfx", useTeam, i), "");
				//siege_Cvar_Set(va("siege_objective%i_mapicon", i), "");
				trap_Cvar_Set(va("team%i_objective%i_mapicon", useTeam, i), "");
			} else break;
		}
		/*else
		{ //didn't find one, so set the "inuse" cvar to 0 for the objective and mark it non-complete.
			trap_Cvar_Set(va("siege_objective%i_inuse", i), "0");
			trap_Cvar_Set(va("siege_objective%i", i), "0");
			trap_Cvar_Set(va("team%i_objective%i_inuse", useTeam, i), "0");
			trap_Cvar_Set(va("team%i_objective%i", useTeam, i), "0");

			trap_Cvar_Set(va("siege_objective%i_mappos", i), "");
			trap_Cvar_Set(va("team%i_objective%i_mappos", useTeam, i), "");
			trap_Cvar_Set(va("siege_objective%i_gfx", i), "");
			trap_Cvar_Set(va("team%i_objective%i_gfx", useTeam, i), "");
			trap_Cvar_Set(va("siege_objective%i_mapicon", i), "");
			trap_Cvar_Set(va("team%i_objective%i_mapicon", useTeam, i), "");
		}*/
		//[/SIEGECVARFIX]

		i++;
	}

	if (dontshow)
	{
		return;
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{
		if (BG_SiegeGetPairedValue(cgParseObjectives, "briefing", briefing))
		{
			CG_DrawSiegeMessage(briefing, 1);
		}
	}
}

void CG_SiegeObjectiveCompleted(centity_t *ent, int won, int objectivenum)
{
	int				myTeam;
	char			teamstr[64];
	char			objstr[256];
	char			foundobjective[MAX_SIEGE_INFO_SIZE];
	char			appstring[1024];
	char			soundstr[1024];
	int				success = 0;
	playerState_t	*ps = NULL;

	if (!siege_valid)
	{
		CG_Error("Siege data does not exist on client!\n");
		return;
	}

	if (cg.snap)
	{ //this should always be true, if it isn't though use the predicted ps as a fallback
		ps = &cg.snap->ps;
	}
	else
	{
		ps = &cg.predictedPlayerState;
	}

	if (!ps)
	{
		assert(0);
		return;
	}

	myTeam = ps->persistant[PERS_TEAM];

	if (myTeam == TEAM_SPECTATOR)
	{
		return;
	}

	if (won == SIEGETEAM_TEAM1)
	{
		Com_sprintf(teamstr, sizeof(teamstr), team1);
	}
	else
	{
		Com_sprintf(teamstr, sizeof(teamstr), team2);
	}

	if (BG_SiegeGetValueGroup(siege_info, teamstr, cgParseObjectives))
	{
		Com_sprintf(objstr, sizeof(objstr), "Objective%i", objectivenum);

		if (BG_SiegeGetValueGroup(cgParseObjectives, objstr, foundobjective))
		{
			if (myTeam == SIEGETEAM_TEAM1)
			{
				success = BG_SiegeGetPairedValue(foundobjective, "message_team1", appstring);
			}
			else
			{
				success = BG_SiegeGetPairedValue(foundobjective, "message_team2", appstring);
			}

			if (success)
			{
				CG_DrawSiegeMessageNonMenu(appstring);
			}

			appstring[0] = 0;
			soundstr[0] = 0;

			if (myTeam == SIEGETEAM_TEAM1)
			{
				Com_sprintf(teamstr, sizeof(teamstr), "sound_team1");
			}
			else
			{
				Com_sprintf(teamstr, sizeof(teamstr), "sound_team2");
			}

			if (BG_SiegeGetPairedValue(foundobjective, teamstr, appstring))
			{
				Com_sprintf(soundstr, sizeof(soundstr), appstring);
			}
			/*
			else
			{
				if (myTeam != won)
				{
					Com_sprintf(soundstr, sizeof(soundstr), DEFAULT_LOSE_OBJECTIVE);
				}
				else
				{
					Com_sprintf(soundstr, sizeof(soundstr), DEFAULT_WIN_OBJECTIVE);
				}
			}
			*/

			if (soundstr[0])
			{
				trap_S_StartLocalSound(trap_S_RegisterSound(soundstr), CHAN_ANNOUNCER);
			}
		}
	}
}

siegeExtended_t cg_siegeExtendedData[MAX_CLIENTS];

//parse a single extended siege data entry
void CG_ParseSiegeExtendedDataEntry(const char *conStr)
{
	char s[MAX_STRING_CHARS];
	char *str = (char *)conStr;
	int argParses = 0;
	int i;
	int maxAmmo = 0, clNum = -1, health = 1, maxhealth = 1, ammo = 1;
	centity_t *cent;

	if (!conStr || !conStr[0])
	{
		return;
	}

	while (*str && argParses < 4)
	{
		i = 0;
        while (*str && *str != '|')
		{
			s[i] = *str;
			i++;
			*str++;
		}
		s[i] = 0;
        switch (argParses)
		{
		case 0:
			clNum = atoi(s);
			break;
		case 1:
			health = atoi(s);
			break;
		case 2:
			maxhealth = atoi(s);
			break;
		case 3:
			ammo = atoi(s);
			break;
		default:
			break;
		}
		argParses++;
		str++;
	}

	if (clNum < 0 || clNum >= MAX_CLIENTS)
	{
		return;
	}

	cg_siegeExtendedData[clNum].health = health;
	cg_siegeExtendedData[clNum].maxhealth = maxhealth;
	cg_siegeExtendedData[clNum].ammo = ammo;

	cent = &cg_entities[clNum];

	maxAmmo = ammoData[weaponData[cent->currentState.weapon].ammoIndex].max;
	if ( (cent->currentState.eFlags & EF_DOUBLE_AMMO) )
	{
		maxAmmo *= 2.0f;
	}
	if (ammo >= 0 && ammo <= maxAmmo )
	{ //assure the weapon number is valid and not over max
		//keep the weapon so if it changes before our next ext data update we'll know
		//that the ammo is not applicable.
		cg_siegeExtendedData[clNum].weapon = cent->currentState.weapon;
	}
	else
	{ //not valid? Oh well, just invalidate the weapon too then so we don't display ammo
		cg_siegeExtendedData[clNum].weapon = -1;
	}

	cg_siegeExtendedData[clNum].lastUpdated = cg.time;
}

//parse incoming siege data, see counterpart in g_saga.c
void CG_ParseSiegeExtendedData(void)
{
	int numEntries = trap_Argc();
	int i = 0;

	if (numEntries < 1)
	{
		assert(!"Bad numEntries for sxd");
		return;
	}

	while (i < numEntries)
	{
		CG_ParseSiegeExtendedDataEntry(CG_Argv(i+1));
		i++;
	}
}

void CG_SetSiegeTimerCvar ( int msec )
{
	int seconds;
	int mins;
	int tens;

	seconds = msec / 1000;
	mins = seconds / 60;
	seconds -= mins * 60;
	tens = seconds / 10;
	seconds -= tens * 10;

	//[SIEGECVARFIX]
	siege_Cvar_Set("ui_siegeTimer", va( "%i:%i%i", mins, tens, seconds ) );
	//trap_Cvar_Set("ui_siegeTimer", va( "%i:%i%i", mins, tens, seconds ) );
	//[SIEGECVARFIX]
}


//[TrueView]
//Loads in the True View auto eye positioning data so you don't have to worry about disk access later in the 
//game
//Based on CG_InitSagaMode and tck's tck_InitBuffer
void CG_TrueViewInit( void )
{
	int				len = 0;
	fileHandle_t	f;


	len = trap_FS_FOpenFile("trueview.cfg", &f, FS_READ);

	if (!f)
	{
		CG_Printf("Error: File Not Found: trueview.cfg\n");
		true_view_valid = 0;
		return;
	}

	if( len >= MAX_TRUEVIEW_INFO_SIZE )
	{
		CG_Printf("Error: trueview.cfg is over the trueview.cfg filesize limit.\n");
		trap_FS_FCloseFile( f );
		true_view_valid = 0;
		return;
	}

	
	trap_FS_Read(true_view_info, len, f);

	true_view_valid = 1;

	trap_FS_FCloseFile( f );

	return;

}


//Tries to adjust the eye position from the data in cfg file if possible.
void CG_AdjustEyePos (const char *modelName)
{
	//eye position
	char	eyepos[MAX_QPATH];

	if ( true_view_valid )
	{
		
		if( BG_SiegeGetPairedValue(true_view_info, (char*) modelName, eyepos) )
		{
			CG_Printf("True View Eye Adjust Loaded for %s.\n", modelName);
			trap_Cvar_Set( "cg_trueeyeposition", eyepos );
		}
		else
		{//Couldn't find an entry for the desired model.  Not nessicarily a bad thing.
			trap_Cvar_Set( "cg_trueeyeposition", "0" );
		}
	}
	else
	{//The model eye position list is messed up.  Default to 0.0 for the eye position
		trap_Cvar_Set( "cg_trueeyeposition", "0" );
	}

}
//[/TrueView]
