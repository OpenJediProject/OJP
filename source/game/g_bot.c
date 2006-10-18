// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_bot.c

#include "g_local.h"


static int		g_numBots;
static char		*g_botInfos[MAX_BOTS];


int				g_numArenas;
static char		*g_arenaInfos[MAX_ARENAS];


#define BOT_BEGIN_DELAY_BASE		2000
#define BOT_BEGIN_DELAY_INCREMENT	1500

#define BOT_SPAWN_QUEUE_DEPTH	16

typedef struct {
	int		clientNum;
	int		spawnTime;
} botSpawnQueue_t;

//static int			botBeginDelay = 0;  // bk001206 - unused, init
static botSpawnQueue_t	botSpawnQueue[BOT_SPAWN_QUEUE_DEPTH];

vmCvar_t bot_minplayers;

extern gentity_t	*podium1;
extern gentity_t	*podium2;
extern gentity_t	*podium3;

#include "../namespace_begin.h"
float trap_Cvar_VariableValue( const char *var_name ) {
	char buf[128];

	trap_Cvar_VariableStringBuffer(var_name, buf, sizeof(buf));
	return atof(buf);
}
#include "../namespace_end.h"


/*
===============
G_ParseInfos
===============
*/
int G_ParseInfos( char *buf, int max, char *infos[] ) {
	char	*token;
	int		count;
	char	key[MAX_TOKEN_CHARS];
	char	info[MAX_INFO_STRING];

	count = 0;

	while ( 1 ) {
		token = COM_Parse( (const char **)(&buf) );
		if ( !token[0] ) {
			break;
		}
		if ( strcmp( token, "{" ) ) {
			Com_Printf( "Missing { in info file\n" );
			break;
		}

		if ( count == max ) {
			Com_Printf( "Max infos exceeded\n" );
			break;
		}

		info[0] = '\0';
		while ( 1 ) {
			token = COM_ParseExt( (const char **)(&buf), qtrue );
			if ( !token[0] ) {
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}
			if ( !strcmp( token, "}" ) ) {
				break;
			}
			Q_strncpyz( key, token, sizeof( key ) );

			token = COM_ParseExt( (const char **)(&buf), qfalse );
			if ( !token[0] ) {
				strcpy( token, "<NULL>" );
			}
			Info_SetValueForKey( info, key, token );
		}
		//NOTE: extra space for arena number
		infos[count] = (char *) G_Alloc(strlen(info) + strlen("\\num\\") + strlen(va("%d", MAX_ARENAS)) + 1);
		if (infos[count]) {
			strcpy(infos[count], info);
			count++;
		}
	}
	return count;
}

/*
===============
G_LoadArenasFromFile
===============
*/
static void G_LoadArenasFromFile( char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[MAX_ARENAS_TEXT];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Printf( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if ( len >= MAX_ARENAS_TEXT ) {
		trap_Printf( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_ARENAS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	g_numArenas += G_ParseInfos( buf, MAX_ARENAS - g_numArenas, &g_arenaInfos[g_numArenas] );
}

int G_GetMapTypeBits(char *type)
{
	int typeBits = 0;

	if( *type ) {
		if( strstr( type, "ffa" ) ) {
			typeBits |= (1 << GT_FFA);

			//[OLDGAMETYPES]
			//all ffa maps support JediMaster mode with the new code that
			//adds a info_jedimaster_start to maps that don't have them.
			typeBits |= (1 << GT_JEDIMASTER);
			//[/OLDGAMETYPES]

		//[Asteroids]
			//typeBits |= (1 << GT_TEAM);
		//[/Asteroids]
		}

		//[CoOp]
		if( strstr( type, "coop" ) ) {
			typeBits |= (1 << GT_SINGLE_PLAYER);
		}
		//[CoOp]

		if( strstr( type, "team" ) ) {
			typeBits |= (1 << GT_TEAM);
		}	
		//[/Asteroids]
		if( strstr( type, "holocron" ) ) {
			typeBits |= (1 << GT_HOLOCRON);
		}
		if( strstr( type, "jedimaster" ) ) {
			typeBits |= (1 << GT_JEDIMASTER);
		}
		if( strstr( type, "duel" ) ) {
			typeBits |= (1 << GT_DUEL);
			typeBits |= (1 << GT_POWERDUEL);
		}
		if( strstr( type, "powerduel" ) ) {
			typeBits |= (1 << GT_DUEL);
			typeBits |= (1 << GT_POWERDUEL);
		}
		if( strstr( type, "siege" ) ) {
			typeBits |= (1 << GT_SIEGE);
		}
		if( strstr( type, "ctf" ) ) {
			typeBits |= (1 << GT_CTF);
//[OLDGAMETYPES]
			typeBits |= (1 << GT_CTY);
//[/OLDGAMETYPES]
		}
		if( strstr( type, "cty" ) ) {
			typeBits |= (1 << GT_CTY);
		//[NewGametypes][EnhancedImpliment]
		/*
		} // MJN - RPG
		if( strstr( type, "rpg" ) ) {
			typeBits |= (1 << GT_RPG);
		}
		if( strstr( type, "single" ) ) {
			typeBits |= (1 << GT_COOP);
		}
		// MJN - ITG
		if( strstr( type, "instagib" ) ) {
			typeBits |= (1 << GT_ITG);
		}
		if( strstr( type, "attack" ) ) {
			typeBits |= (1 << GT_ATTACK);
		}
		if( strstr( type, "defence" ) ) {
			typeBits |= (1 << GT_DEFENCE);
		}
		if( strstr( type, "scenario" ) ) {
			typeBits |= (1 << GT_SCENARIO);
		*/
		//[/NewGametypes][EnhancedImpliment]
		}
	} else {
		typeBits |= (1 << GT_FFA);
		//[OLDGAMETYPES]
		//all ffa maps support JediMaster mode with the new code that
		//adds a info_jedimaster_start to maps that don't have them.
		typeBits |= (1 << GT_JEDIMASTER);
		//[/OLDGAMETYPES]
	}

	return typeBits;
}

qboolean G_DoesMapSupportGametype(const char *mapname, int gametype)
{
	int			typeBits = 0;
	int			thisLevel = -1;
	int			n = 0;
	char		*type = NULL;

	if (!g_arenaInfos[0])
	{
		return qfalse;
	}

	if (!mapname || !mapname[0])
	{
		return qfalse;
	}

	for( n = 0; n < g_numArenas; n++ )
	{
		type = Info_ValueForKey( g_arenaInfos[n], "map" );

		if (Q_stricmp(mapname, type) == 0)
		{
			thisLevel = n;
			break;
		}
	}

	if (thisLevel == -1)
	{
		return qfalse;
	}

	type = Info_ValueForKey(g_arenaInfos[thisLevel], "type");

	typeBits = G_GetMapTypeBits(type);
	if (typeBits & (1 << gametype))
	{ //the map in question supports the gametype in question, so..
		return qtrue;
	}

	return qfalse;
}

//rww - auto-obtain nextmap. I could've sworn Q3 had something like this, but I guess not.
const char *G_RefreshNextMap(int gametype, qboolean forced)
{
	int			typeBits = 0;
	int			thisLevel = 0;
	int			desiredMap = 0;
	int			n = 0;
	char		*type = NULL;
	qboolean	loopingUp = qfalse;
	//[RawMapName]
	//vmCvar_t	mapname;
	//[/RawMapName]

	if (!g_autoMapCycle.integer && !forced)
	{
		return NULL;
	}

	if (!g_arenaInfos[0])
	{
		return NULL;
	}

	//[RawMapName]
	//trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	//[/RawMapName]
	for( n = 0; n < g_numArenas; n++ )
	{
		type = Info_ValueForKey( g_arenaInfos[n], "map" );

		//[RawMapName]
		if (Q_stricmp(level.rawmapname, type) == 0)
		//if (Q_stricmp(mapname.string, type) == 0)
		//[/RawMapName]
		{
			thisLevel = n;
			break;
		}
	}

	desiredMap = thisLevel;

	n = thisLevel+1;
	while (n != thisLevel)
	{ //now cycle through the arena list and find the next map that matches the gametype we're in
		if (!g_arenaInfos[n] || n >= g_numArenas)
		{
			if (loopingUp)
			{ //this shouldn't happen, but if it does we have a null entry break in the arena file
			  //if this is the case just break out of the loop instead of sticking in an infinite loop
				break;
			}
			n = 0;
			loopingUp = qtrue;
		}

		type = Info_ValueForKey(g_arenaInfos[n], "type");
		
		typeBits = G_GetMapTypeBits(type);
		if (typeBits & (1 << gametype))
		{
			desiredMap = n;
			break;
		}

		n++;
	}

	if (desiredMap == thisLevel)
	{ //If this is the only level for this game mode or we just can't find a map for this game mode, then nextmap
	  //will always restart.
		trap_Cvar_Set( "nextmap", "map_restart 0");
	}
	else
	{ //otherwise we have a valid nextmap to cycle to, so use it.
		type = Info_ValueForKey( g_arenaInfos[desiredMap], "map" );
		trap_Cvar_Set( "nextmap", va("map %s", type));
	}

	return Info_ValueForKey( g_arenaInfos[desiredMap], "map" );
}

/*
===============
G_LoadArenas
===============
*/
static void G_LoadArenas( void ) {
	int			numdirs;
	char		filename[128];
	char		dirlist[1024];
	char*		dirptr;
	int			i, n;
	int			dirlen;

	g_numArenas = 0;

	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList("scripts", ".arena", dirlist, 1024 );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		G_LoadArenasFromFile(filename);
	}
//	trap_Printf( va( "%i arenas parsed\n", g_numArenas ) );
	
	for( n = 0; n < g_numArenas; n++ ) {
		Info_SetValueForKey( g_arenaInfos[n], "num", va( "%i", n ) );
	}

	G_RefreshNextMap(g_gametype.integer, qfalse);
}


/*
===============
G_GetArenaInfoByNumber
===============
*/
const char *G_GetArenaInfoByMap( const char *map ) {
	int			n;

	for( n = 0; n < g_numArenas; n++ ) {
		if( Q_stricmp( Info_ValueForKey( g_arenaInfos[n], "map" ), map ) == 0 ) {
			return g_arenaInfos[n];
		}
	}

	return NULL;
}

#if 0
/*
=================
PlayerIntroSound
=================
*/
static void PlayerIntroSound( const char *modelAndSkin ) {
	char	model[MAX_QPATH];
	char	*skin;

	Q_strncpyz( model, modelAndSkin, sizeof(model) );
	skin = Q_strrchr( model, '/' );
	if ( skin ) {
		*skin++ = '\0';
	}
	else {
		skin = model;
	}

	if( Q_stricmp( skin, "default" ) == 0 ) {
		skin = model;
	}

	trap_SendConsoleCommand( EXEC_APPEND, va( "play sound/player/announce/%s.wav\n", skin ) );
}
#endif
//[RandomBotNames]
static const char *firstNames[] = {
	"Evil",
	"Dark",
	"Redneck",
	"Braindead",
	"Killer",
	"Last",
	"First",
	"John",
	"Light",
	"Fast",
	"Slow",
	"Nooby",
	"Professor",
	"Master",
	"Looser",
	"Owned",
	"Red",
	"Shadow",
	"Sneaky",
	"Broken",
	"Lost",
	"Hidden",
	"Silent",
	"The",
	"Da",
	"Your",
	"Laggy",
	"JKA",
	"Angry",
	"Confused",
	"Lucky",
	"Bad",
	"Charlie",
	"Hungry",
	"Living",
	"Dead",
	"Johnny",
	"Defiant",
	"Green",
	"Lazy",
	"Mr",
	"Miss",
	"Mrs",
	"Young",
	"Old",
	"New",
	"Ancient",
	"Falling",
	"Fallen",
	"Darkened",
	"Idiot",
	"Moron",
	"Stupid",
	"Running",
	"Hiding",
	"Rising",
	"Jumping",
	"Retreating",
	"Commander",
	"Funny",
	"Faster",
	"Slower",
	"Grand",
	"Shallow",
	"Hollow",
	"Under",
	"Lesser",
	"Micro",
	"Macro",
	"Jack",
	"Dickie",
	"Dancing",
	"Lasting",
	"Fastest",
	"Slowest",
	"Needy",
	"Sniping",
	"Medic",
	"Hero",
	"Our",
	"My",
	"Holy",
	"The",
	"The",
	"The",
	"The",
	"Da",
	"Da",
	"Da",
	"Da",
	"Big",
	"Small",
	"Mad",
	"Crazy",
	"Jedi",
	"Jedi",
	"Jedi",
	"Jedi",
	"Sith",
	"Sith",
	"Sith",
	"Sith",
	"Sith",
	"Smiling",
	"Black"
}; // 105

static const char *lastNames[] = {
	"One",
	"Avenger",
	"Destroyer",
	"Apprentice",
	"Sniper",
	"Killer",
	"Protector",
	"Assassin",
	"Defender",
	"Attacker",
	"Master",
	"Knight",
	"1",
	"Lagalot",
	"Spacefiller",
	"Player",
	"69",
	"101",
	"Hunter",
	"Jack",
	"Lamer",
	"Death",
	"Hidden",
	"Runner",
	"Skywalker",
	"Soul",
	"Hacker",
	"Hack",
	"Noob",
	"Missfire",
	"Gunfire",
	"Gunslinger",
	"Stabber",
	"Backstab",
	"Swiftkick",
	"Swift",
	"Slasher",
	"007",
	"Leadridden",
	"Leadrush",
	"Killer",
	"Jr",
	"Virus",
	"Trojan",
	"Guardmaster",
	"Attacker",
	"Battlemaster",
	"Kitty",
	"Wolf",
	"Fox",
	"Idiot",
	"Jack",
	"John",
	"Joe",
	"Mac",
	"Frontrunner",
	"Man",
	"Woman",
	"Dog",
	"Hog",
	"Fist",
	"Core",
	"Jackson",
	"Voss",
	"Vlad",
	"Voodoo",
	"Lost",
	"Doom",
	"Death",
	"Demise",
	"Maul",
	"Vader",
	"Skywalker",
	"Solo",
	"Lando",
	"Falcon",
	"Raven",
	"Hawk",
	"Skull",
	"Sith",
	"Jedi",
	"Carnage",
	"Rumble",
	"Botman"
}; // 84

char *PickName ( void )
{// Choose a random name by combining!
	int choice1 = rand()%105;
	int choice2 = rand()%84;
	int color1 = rand()%7;
	int color2 = rand()%7;

	return va("^%i%s^%i%s", color1, firstNames[choice1], color2, lastNames[choice2]);
}
//[/RandomBotNames]

/*
===============
G_AddRandomBot
===============
*/
//RACC - Randomly add a bot to team.  -1 = any team.
void G_AddRandomBot( int team ) {
	int		i, n, num;
	float	skill;
	//[RandomBotNames]
	char	*value, netname[36], *teamstr, fullname[36];
	//char	*value, netname[36], *teamstr;
	//[/RandomBotNames]
	gclient_t	*cl;

	num = 0;
	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "name" );
		//
		for ( i=0 ; i< g_maxclients.integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			//[ClientNumFix]
			if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
			//if ( !(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) ) {
			//[/ClientNumFix]
				continue;
			}
			if (g_gametype.integer == GT_SIEGE)
			{
				if ( team >= 0 && cl->sess.siegeDesiredTeam != team ) {
					continue;
				}
			}
			else
			{
				if ( team >= 0 && cl->sess.sessionTeam != team ) {
					continue;
				}
			}
			if ( !Q_stricmp( value, cl->pers.netname ) ) {
				break;
			}
		}
		if (i >= g_maxclients.integer) {
			num++;
		}
	}
	num = random() * num;
	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "name" );
		//
		for ( i=0 ; i< g_maxclients.integer ; i++ ) {
			cl = level.clients + i;
			if ( cl->pers.connected != CON_CONNECTED ) {
				continue;
			}
			//[ClientNumFix]
			if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
			//if ( !(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) ) {
			//[/ClientNumFix]
				continue;
			}
			if (g_gametype.integer == GT_SIEGE)
			{
				if ( team >= 0 && cl->sess.siegeDesiredTeam != team ) {
					continue;
				}
			}
			else
			{
				if ( team >= 0 && cl->sess.sessionTeam != team ) {
					continue;
				}
			}
			if ( !Q_stricmp( value, cl->pers.netname ) ) {
				break;
			}
		}
		if (i >= g_maxclients.integer) {
			num--;
			if (num <= 0) {
				skill = trap_Cvar_VariableValue( "g_spSkill" );
				if (team == TEAM_RED) teamstr = "red";
				else if (team == TEAM_BLUE) teamstr = "blue";
				else teamstr = "";
				strncpy(netname, value, sizeof(netname)-1);
				//[RandomBotNames]
				strncpy(fullname, PickName(), sizeof(fullname)-1);
				//[/RandomBotNames]
				netname[sizeof(netname)-1] = '\0';
				Q_CleanStr(netname);
				//[RandomBotNames]
				//trap_SendConsoleCommand( EXEC_INSERT, va("addbot \"%s\" %f %s %i\n", netname, skill, teamstr, 0) );
				//[TABBots]
				//make random bots be TABBots.
				trap_SendConsoleCommand( EXEC_INSERT, va("addbot \"%s\" %f \"%s\" %i \"%s\" %i\n", netname, skill, teamstr, 0, fullname, BOT_TAB) );
				//[/TABBots]
				//[RandomBotNames]
				return;
			}
		}
	}
} 

/*
===============
G_RemoveRandomBot
===============
*/
//RACC - Randomly remove a bot from team.  -1 = any team.
int G_RemoveRandomBot( int team ) {
	int i;
	char netname[36];
	gclient_t	*cl;

	for ( i=0 ; i< g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		//[ClientNumFix]
		if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
		//if ( !(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) ) {
		//[/ClientNumFix]
			continue;
		}
		//[BugFix9]
		if ( cl->sess.sessionTeam == TEAM_SPECTATOR 
			&& cl->sess.spectatorState == SPECTATOR_FOLLOW )
		{//this entity is actually following another entity so the ps data is for a
			//different entity.  Bots never spectate like this so, skip this player.
			continue;
		}
		//[/BugFix9]

		if (g_gametype.integer == GT_SIEGE)
		{
			if ( team >= 0 && cl->sess.siegeDesiredTeam != team ) {
				continue;
			}
		}
		else
		{
			if ( team >= 0 && cl->sess.sessionTeam != team ) {
				continue;
			}
		}
		strcpy(netname, cl->pers.netname);
		Q_CleanStr(netname);
		//[test]
		//hmmm, this method seems to be breaking stuff.  doublechecking.
		//bots that left disconnect instead of being kicked.  I think it's scaring
		//players
		//trap_DropClient( cl->ps.clientNum, va(S_COLOR_WHITE "%s\n", G_GetStringEdString("MP_SVGAME", "DISCONNECTED")) );
		//[/test]
		trap_SendConsoleCommand( EXEC_INSERT, va("kick \"%s\"\n", netname) );
		return qtrue;
	}
	return qfalse;
}

/*
===============
G_CountHumanPlayers
===============
*/
//RACC - checks for the number of human players on a given team. team = -1 for all teams.
//[AdminSys]
int G_CountHumanPlayers( int ignoreClientNum, int team ) {
//int G_CountHumanPlayers( int team ) {
//[/AdminSys]
	int i, num;
	gclient_t	*cl;

	num = 0;
	for ( i=0 ; i< g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		//[AdminSys]
		if ( i == ignoreClientNum ) {
			continue;
		}
		//[/AdminSys]

		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		//[BugFix17]
		//can't use cl->ps.clientNum since the ps.clientNum might be for the clientNum of the player that this client is specing.
		if ( g_entities[i].r.svFlags & SVF_BOT ) {
		//if ( g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT ) {
		//[/BugFix17]
			continue;
		}
		if ( team >= 0 && cl->sess.sessionTeam != team ) {
			continue;
		}

		//[BotTweaks]
		//don't count as a human player (for the bot_minplayers stuff) until
		//the player isn't a specator.
		if(g_gametype.integer != GT_DUEL && g_gametype.integer != GT_POWERDUEL 
			//human players in the game are specators while not in the duel.  Don't 
			//use this rule for those gametypes.
			&& cl->sess.sessionTeam == TEAM_SPECTATOR)
		{
			continue;
		}
		//[/BotTweaks]

		num++;
	}
	return num;
}

/*
===============
G_CountBotPlayers
===============
*/
//RACC - checks for the number of human players on a given team. team = -1 for all teams.
int G_CountBotPlayers( int team ) {
	int i, n, num;
	gclient_t	*cl;

	num = 0;
	for ( i=0 ; i< g_maxclients.integer ; i++ ) {
		cl = level.clients + i;
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}

		//[BugFix17]
		//can't use cl->ps.clientNum since the ps.clientNum might be for the clientNum of the player that this client is specing.
		if ( !(g_entities[i].r.svFlags & SVF_BOT) ) {
		//if ( !(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) ) {
			continue;
		}
		//[/BugFix17]

		if (g_gametype.integer == GT_SIEGE)
		{
			if ( team >= 0 && cl->sess.siegeDesiredTeam != team ) {
				continue;
			}
		}
		else
		{
			if ( team >= 0 && cl->sess.sessionTeam != team ) {
				continue;
			}
		}
		num++;
	}
	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( !botSpawnQueue[n].spawnTime ) {
			continue;
		}
		if ( botSpawnQueue[n].spawnTime > level.time ) {
			continue;
		}
		num++;
	}
	return num;
}

/*
===============
G_CheckMinimumPlayers
===============
*/
//[BotTweaks]
extern	vmCvar_t	g_allowBotLimit;
extern	vmCvar_t	g_minHumans;
extern	vmCvar_t	g_maxBots;
extern int OJP_PointSpread(void);
//[/BotTweaks]
//RACC - Adds/removes bots to maintain the minimum player limit.
void G_CheckMinimumPlayers( void ) {
	int minplayers;
	int humanplayers, botplayers;
	//[BotTweaks]
	int humanplayers2, botplayers2;
	//[/BotTweaks]
	static int checkminimumplayers_time;

	//[TABBot]
	//We want the minimum players system to work in siege.
	/*
	if (g_gametype.integer == GT_SIEGE)
	{
		return;
	}
	*/
	//[/TABBot]

	//[NewGameTypes][EnhancedImpliment]
	/*
	if (g_gametype.integer == GT_RPG)// MJN - Not implementing this for RPG
	{
		return;
	}
	*/
	//[/NewGameTypes]

	//[TABBots]
	if(level.time - level.startTime < 10000)
	{//don't spawn in new bots for 10 seconds.  Otherwise we're going to be adding/removing
		//bots before the original ones spawn in.
		return;
	}
	//[/TABBots]

	if (level.intermissiontime) return;
	//only check once each 10 seconds
	if (checkminimumplayers_time > level.time - 10000) {
		return;
	}
	checkminimumplayers_time = level.time;
	trap_Cvar_Update(&bot_minplayers);
	minplayers = bot_minplayers.integer;

	//[BotTweaks]	
	//MJN - All that new fancy bot auto limiting code. :)
	if(g_allowBotLimit.integer == 0)
	{
		if (minplayers <= 0) 
			return;
	}

	if(g_allowBotLimit.integer == 1)
	{
		if ( g_minHumans.integer < 0 )
		{//clamp g_minHumans to positive values.
			g_minHumans.integer = 0;
		}
			
		if(g_maxBots.integer <= 0) 
		{//just don't do anything if g_maxBots is zero or negative.
			return;
		}
	}
	
	if(g_allowBotLimit.integer == 1)
	{//use the new bot code that Chosen One did for us.
		// Teams each get Max Bots specified
		if (g_gametype.integer >= GT_TEAM) 
		{//team gametypes
			// MJN - Make sure numbers don't exceed maxclients
			if (minplayers >= g_maxclients.integer / 2) 
			{
				minplayers = (g_maxclients.integer / 2) -1;
			}
			if (g_maxBots.integer >= g_maxclients.integer / 2) 
			{
				g_maxBots.integer = (g_maxclients.integer / 2) -1;
			}
			if (g_minHumans.integer >= g_maxclients.integer / 2) 
			{
				g_minHumans.integer = (g_maxclients.integer / 2) -1;
			}

			//Handle Red Team Count

			//[AdminSys]
			humanplayers = G_CountHumanPlayers( -1, TEAM_RED );
			//humanplayers = G_CountHumanPlayers( TEAM_RED );
			//[/AdminSys]
			botplayers = G_CountBotPlayers(	TEAM_RED );

			if(botplayers < g_maxBots.integer && humanplayers < g_minHumans.integer )
			{
				G_AddRandomBot( TEAM_RED );
			}
			else if (humanplayers + botplayers > g_maxBots.integer 
						&& humanplayers >= g_minHumans.integer ) 
			{
				G_RemoveRandomBot( TEAM_RED );
			} 

			//Handle Blue Team Count
			//[AdminSys]
			humanplayers2 = G_CountHumanPlayers( -1, TEAM_BLUE );
			//humanplayers2 = G_CountHumanPlayers( TEAM_BLUE );
			//[/AdminSys]
			botplayers2 = G_CountBotPlayers( TEAM_BLUE );

			// MJN - Max Bots/Min Humans added here	
			if(botplayers2 < g_maxBots.integer && humanplayers2 < g_minHumans.integer)
			{
				G_AddRandomBot( TEAM_BLUE );
			}
			else if (humanplayers2 + botplayers2 > g_maxBots.integer 
						&& humanplayers2 >= g_minHumans.integer) 
			{
				G_RemoveRandomBot( TEAM_BLUE );
			} 
		}
		else if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL) 
		{//duel gametypes
			// MJN - Make sure numbers don't exceed maxclients
			if (minplayers >= g_maxclients.integer) 
			{
				minplayers = g_maxclients.integer-1;
			}
			if (g_maxBots.integer >= g_maxclients.integer) 
			{
				g_maxBots.integer = g_maxclients.integer-1;
			}
			if (g_minHumans.integer >= g_maxclients.integer) 
			{
				g_minHumans.integer = g_maxclients.integer-1;
			}

			//[AdminSys]
			humanplayers = G_CountHumanPlayers( -1, -1 );
			//humanplayers = G_CountHumanPlayers( -1 );
			//[/AdminSys]
			botplayers = G_CountBotPlayers( -1 );

			if(botplayers < g_maxBots.integer && humanplayers < g_minHumans.integer)
			{
				G_AddRandomBot( TEAM_FREE );
			}
			else if (humanplayers + botplayers > g_maxBots.integer 
						&& humanplayers >= g_minHumans.integer) 
			{
				// try to remove spectators first
				if (!G_RemoveRandomBot( TEAM_SPECTATOR )) 
				{
					// just remove the bot that is playing
					G_RemoveRandomBot( -1 );
				}
			} 
		}
		else if (g_gametype.integer == GT_FFA) 
		{//ffa gametype
			// MJN - Make sure numbers don't exceed maxclients
			if (minplayers >= g_maxclients.integer) 
			{
				minplayers = g_maxclients.integer-1;
			}
			if (g_maxBots.integer >= g_maxclients.integer) 
			{
				g_maxBots.integer = g_maxclients.integer-1;
			}
			if (g_minHumans.integer >= g_maxclients.integer) 
			{
				g_minHumans.integer = g_maxclients.integer-1;
			}

			//[AdminSys]
			humanplayers = G_CountHumanPlayers( -1, TEAM_FREE );
			//humanplayers = G_CountHumanPlayers( TEAM_FREE );
			//[/AdminSys]
			botplayers = G_CountBotPlayers( TEAM_FREE );

			if(botplayers < g_maxBots.integer && humanplayers < g_minHumans.integer)
			{
				G_AddRandomBot( TEAM_FREE );
			}
			else if (humanplayers + botplayers > g_maxBots.integer 
						&& humanplayers >= g_minHumans.integer) 
			{
				G_RemoveRandomBot( TEAM_FREE );
			} 
		}
		else if (g_gametype.integer == GT_HOLOCRON || g_gametype.integer == GT_JEDIMASTER) 
		{//special ffa gametypes
			// MJN - Make sure numbers don't exceed maxclients
			if (minplayers >= g_maxclients.integer) 
			{
				minplayers = g_maxclients.integer-1;
			}

			if (g_maxBots.integer >= g_maxclients.integer) 
			{
				g_maxBots.integer = g_maxclients.integer-1;
			}
			if (g_minHumans.integer >= g_maxclients.integer) 
			{
				g_minHumans.integer = g_maxclients.integer-1;
			}

			//[AdminSys]
			humanplayers = G_CountHumanPlayers( -1, TEAM_FREE );
			//humanplayers = G_CountHumanPlayers( TEAM_FREE );
			//[/AdminSys]
			botplayers = G_CountBotPlayers( TEAM_FREE );
			
			if(botplayers < g_maxBots.integer && humanplayers < g_minHumans.integer)
			{
				G_AddRandomBot( TEAM_FREE );
			}else if (humanplayers + botplayers > g_maxBots.integer 
						&& humanplayers >= g_minHumans.integer) 
			{
				G_RemoveRandomBot( TEAM_FREE );
			} 
		}
	}
	else
	{//use the basejka system for redundency's sake.
		if (minplayers <= 0) return;

		if (minplayers > g_maxclients.integer)
		{
			minplayers = g_maxclients.integer;
		}

		//[AdminSys]
		humanplayers = G_CountHumanPlayers( -1, -1 );
		//humanplayers = G_CountHumanPlayers( -1 );
		//[/AdminSys]
		botplayers = G_CountBotPlayers(	-1 );

		if ((humanplayers+botplayers) < minplayers)
		{
			G_AddRandomBot(-1);
		}
		else if ((humanplayers+botplayers) > minplayers && botplayers)
		{
			// try to remove spectators first
			if (!G_RemoveRandomBot(TEAM_SPECTATOR))
			{
				//[AdminSys]
				if(g_gametype.integer < GT_TEAM)
				{//no teams, just remove a bot.
					G_RemoveRandomBot(-1);
				}
				else if( g_teamForceBalance.integer > 1 )
				{//team game, determine which team to pull from.
					int botRemoveTeam = -1;
					int	counts[TEAM_NUM_TEAMS];

					counts[TEAM_BLUE] = TeamCount( -1, TEAM_BLUE );
					counts[TEAM_RED] = TeamCount( -1, TEAM_RED );

					//always remove bot from the team with the most players on it.
					//that should always balance the teams properly, except for human/bot
					//balancing.
					if(counts[TEAM_RED] > counts[TEAM_BLUE])
					{//red team has too many players
						botRemoveTeam = TEAM_RED;
					}
					else if(counts[TEAM_BLUE] > counts[TEAM_RED])
					{//blue team has too many players
						botRemoveTeam = TEAM_BLUE;
					}

					if(botRemoveTeam == -1 || !G_RemoveRandomBot(botRemoveTeam))
					{//didn't have a specific team to remove from or we couldn't remove from the team
						//we wanted.
						G_RemoveRandomBot(-1);
					}
				}

				// just remove the bot that is playing
				//G_RemoveRandomBot(-1);
				//[/AdminSys]
			}
		}
	}

	/* basejka code
	if (minplayers <= 0) return;

	if (minplayers > g_maxclients.integer)
	{
		minplayers = g_maxclients.integer;
	}

	humanplayers = G_CountHumanPlayers( -1 );
	botplayers = G_CountBotPlayers(	-1 );

	if ((humanplayers+botplayers) < minplayers)
	{
		G_AddRandomBot(-1);
	}
	else if ((humanplayers+botplayers) > minplayers && botplayers)
	{
		// try to remove spectators first
		if (!G_RemoveRandomBot(TEAM_SPECTATOR))
		{
			// just remove the bot that is playing
			G_RemoveRandomBot(-1);
		}
	}
	*/
	//[/BotTweaks]

	/*
	if (g_gametype.integer >= GT_TEAM) {
		int humanplayers2, botplayers2;
		if (minplayers >= g_maxclients.integer / 2) {
			minplayers = (g_maxclients.integer / 2) -1;
		}

		humanplayers = G_CountHumanPlayers( TEAM_RED );
		botplayers = G_CountBotPlayers(	TEAM_RED );
		humanplayers2 = G_CountHumanPlayers( TEAM_BLUE );
		botplayers2 = G_CountBotPlayers( TEAM_BLUE );
		//
		if ((humanplayers+botplayers+humanplayers2+botplayers) < minplayers)
		{
			if ((humanplayers+botplayers) < (humanplayers2+botplayers2))
			{
				G_AddRandomBot( TEAM_RED );
			}
			else
			{
				G_AddRandomBot( TEAM_BLUE );
			}
		}
		else if ((humanplayers+botplayers+humanplayers2+botplayers) > minplayers && botplayers)
		{
			if ((humanplayers+botplayers) < (humanplayers2+botplayers2))
			{
				G_RemoveRandomBot( TEAM_BLUE );
			}
			else
			{
				G_RemoveRandomBot( TEAM_RED );
			}
		}
	}
	else if (g_gametype.integer == GT_DUEL || g_gametype.integer == GT_POWERDUEL) {
		if (minplayers >= g_maxclients.integer) {
			minplayers = g_maxclients.integer-1;
		}
		humanplayers = G_CountHumanPlayers( -1 );
		botplayers = G_CountBotPlayers( -1 );
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot( TEAM_FREE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			// try to remove spectators first
			if (!G_RemoveRandomBot( TEAM_SPECTATOR )) {
				// just remove the bot that is playing
				G_RemoveRandomBot( -1 );
			}
		}
	}
	else if (g_gametype.integer == GT_FFA) {
		if (minplayers >= g_maxclients.integer) {
			minplayers = g_maxclients.integer-1;
		}
		humanplayers = G_CountHumanPlayers( TEAM_FREE );
		botplayers = G_CountBotPlayers( TEAM_FREE );
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot( TEAM_FREE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot( TEAM_FREE );
		}
	}
	else if (g_gametype.integer == GT_HOLOCRON || g_gametype.integer == GT_JEDIMASTER) {
		if (minplayers >= g_maxclients.integer) {
			minplayers = g_maxclients.integer-1;
		}
		humanplayers = G_CountHumanPlayers( TEAM_FREE );
		botplayers = G_CountBotPlayers( TEAM_FREE );
		//
		if (humanplayers + botplayers < minplayers) {
			G_AddRandomBot( TEAM_FREE );
		} else if (humanplayers + botplayers > minplayers && botplayers) {
			G_RemoveRandomBot( TEAM_FREE );
		}
	}
	*/
}

/*
===============
G_CheckBotSpawn
===============
*/
//RACC - Checks and does all the bot spawning stuff needed for this frame.
void G_CheckBotSpawn( void ) {
	int		n;

	G_CheckMinimumPlayers();

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( !botSpawnQueue[n].spawnTime ) {
			continue;
		}
		if ( botSpawnQueue[n].spawnTime > level.time ) {
			continue;
		}
		ClientBegin( botSpawnQueue[n].clientNum, qfalse );
		botSpawnQueue[n].spawnTime = 0;

		/*
		if( g_gametype.integer == GT_SINGLE_PLAYER ) {
			trap_GetUserinfo( botSpawnQueue[n].clientNum, userinfo, sizeof(userinfo) );
			PlayerIntroSound( Info_ValueForKey (userinfo, "model") );
		}
		*/
	}
}


/*
===============
AddBotToSpawnQueue
===============
*/
static void AddBotToSpawnQueue( int clientNum, int delay ) {
	int		n;

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( !botSpawnQueue[n].spawnTime ) {
			botSpawnQueue[n].spawnTime = level.time + delay;
			botSpawnQueue[n].clientNum = clientNum;
			return;
		}
	}

	G_Printf( S_COLOR_YELLOW "Unable to delay spawn\n" );
	ClientBegin( clientNum, qfalse );
}


/*
===============
G_RemoveQueuedBotBegin

Called on client disconnect to make sure the delayed spawn
doesn't happen on a freed index
===============
*/
void G_RemoveQueuedBotBegin( int clientNum ) {
	int		n;

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ ) {
		if( botSpawnQueue[n].clientNum == clientNum ) {
			botSpawnQueue[n].spawnTime = 0;
			return;
		}
	}
}


/*
===============
G_BotConnect
===============
*/
qboolean G_BotConnect( int clientNum, qboolean restart ) {
	bot_settings_t	settings;
	char			userinfo[MAX_INFO_STRING];

	trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );

	Q_strncpyz( settings.personalityfile, Info_ValueForKey( userinfo, "personality" ), sizeof(settings.personalityfile) );
	settings.skill = atof( Info_ValueForKey( userinfo, "skill" ) );
	Q_strncpyz( settings.team, Info_ValueForKey( userinfo, "team" ), sizeof(settings.team) );
	//[TABBot]
	settings.botType = atoi( Info_ValueForKey( userinfo, "bottype" ) );
	//[/TABBot]

	if (!BotAISetupClient( clientNum, &settings, restart )) {
		trap_DropClient( clientNum, "BotAISetupClient failed" );
		return qfalse;
	}

	return qtrue;
}


/*
===============
G_AddBot
===============
*/
//[TABBot]
//added bot type varible
static void G_AddBot( const char *name, float skill, const char *team, int delay, char *altname, int bottype) {
//static void G_AddBot( const char *name, float skill, const char *team, int delay, char *altname) {
//[/TABBot]
	int				clientNum;
	char			*botinfo;
	gentity_t		*bot;
	char			*key;
	char			*s;
	char			*botname;
	char			*model;
//	char			*headmodel;
	char			userinfo[MAX_INFO_STRING];
	int				preTeam = 0;
	//[DuelGuns][EnhancedImpliment]
	/*
	char			*firearm; //** change gun model	
	qboolean		bot_dualguns = qfalse;
	int				gunoption=0;
	*/
	//[/DuelGuns][EnhancedImpliment]

	// get the botinfo from bots.txt
	botinfo = G_GetBotInfoByName( name );
	if ( !botinfo ) {
		G_Printf( S_COLOR_RED "Error: Bot '%s' not defined\n", name );
		return;
	}

	// create the bot's userinfo
	userinfo[0] = '\0';

	botname = Info_ValueForKey( botinfo, "funname" );
	if( !botname[0] ) {
		botname = Info_ValueForKey( botinfo, "name" );
	}
	// check for an alternative name
	if (altname && altname[0]) {
		botname = altname;
	}
	Info_SetValueForKey( userinfo, "name", botname );
	Info_SetValueForKey( userinfo, "rate", "25000" );
	Info_SetValueForKey( userinfo, "snaps", "20" );
	Info_SetValueForKey( userinfo, "skill", va("%1.2f", skill) );

	if ( skill >= 1 && skill < 2 ) {
		Info_SetValueForKey( userinfo, "handicap", "50" );
	}
	else if ( skill >= 2 && skill < 3 ) {
		Info_SetValueForKey( userinfo, "handicap", "70" );
	}
	else if ( skill >= 3 && skill < 4 ) {
		Info_SetValueForKey( userinfo, "handicap", "90" );
	}

	key = "model";
	model = Info_ValueForKey( botinfo, key );
	if ( !*model ) {
		model = "kyle/default";
	}
	Info_SetValueForKey( userinfo, key, model );

/*	key = "headmodel";
	headmodel = Info_ValueForKey( botinfo, key );
	if ( !*headmodel ) {
		headmodel = model;
	}
	Info_SetValueForKey( userinfo, key, headmodel );
	key = "team_headmodel";
	Info_SetValueForKey( userinfo, key, headmodel );
*/
	key = "gender";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "male";
	}
	Info_SetValueForKey( userinfo, "sex", s );

	key = "color1";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "4";
	}
	Info_SetValueForKey( userinfo, key, s );

	key = "color2";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "4";
	}
	Info_SetValueForKey( userinfo, key, s );

	key = "saber1";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "single_1";
	}
	Info_SetValueForKey( userinfo, key, s );

	key = "saber2";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "none";
	}
	Info_SetValueForKey( userinfo, key, s );

	s = Info_ValueForKey(botinfo, "personality");
	if (!*s )
	{
		Info_SetValueForKey( userinfo, "personality", "botfiles/default.jkb" );
	}
	else
	{
		Info_SetValueForKey( userinfo, "personality", s );
	}

	//[DuelGuns][EnhancedImpliment]
	/*
//	if(1)//f_dualguns.integer)
//	{
		key = "dualgun";
		s = Info_ValueForKey(botinfo, key);
		if (*s)
		{
			gunoption = atoi(s);
		}
		if(gunoption>0)
			bot_dualguns = qtrue;
//	}

	firearm = Info_ValueForKey( botinfo, "firearm");
	if (!*firearm)
	{
		Info_SetValueForKey( userinfo, "firearm", botname );
	}
	else
	{
		Info_SetValueForKey( userinfo, "firearm", firearm);
	}
	*/
	//[/DuelGuns][EnhancedImpliment]

	//[RGBSabers]
	key = "rgb_saber1";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "255,0,0";
	}
	Info_SetValueForKey( userinfo, key, s );

	key = "rgb_saber2";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "0,255,255";
	}
	Info_SetValueForKey( userinfo, key, s );

	key = "rgb_script1";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "none";
	}
	Info_SetValueForKey( userinfo, key, s );

	key = "rgb_script2";
	s = Info_ValueForKey( botinfo, key );
	if ( !*s ) {
		s = "none";
	}
	Info_SetValueForKey( userinfo, key, s );
	//[/RGBSabers]

	//[ClientPlugInDetect]
	//set it so that the bots are assumed to have the OJP client plugin
	//this should be CURRENT_OJPENHANCED_CLIENTVERSION
	Info_SetValueForKey( userinfo, "ojp_clientplugin", CURRENT_OJPENHANCED_CLIENTVERSION );
	//[/ClientPlugInDetect]

	// have the server allocate a client slot
	clientNum = trap_BotAllocateClient();
	if ( clientNum == -1 ) {
//		G_Printf( S_COLOR_RED "Unable to add bot.  All player slots are in use.\n" );
//		G_Printf( S_COLOR_RED "Start server with more 'open' slots.\n" );
		trap_SendServerCommand( -1, va("print \"%s\n\"", G_GetStringEdString("MP_SVGAME", "UNABLE_TO_ADD_BOT")));
		return;
	}

	//[DuelGuns][EnhancedImpliment]
	/*
	if(bot_dualguns)
	{
		g_entities[clientNum].client->ps.dualguns = 1;		
	}
	*/
	//[/DuelGuns][EnhancedImpliment]

	// initialize the bot settings
	if( !team || !*team ) {
		if( g_gametype.integer >= GT_TEAM ) {
			//[AdminSys]
			if( PickTeam(clientNum, qtrue) == TEAM_RED) {
			//if( PickTeam(clientNum) == TEAM_RED) {
			//[/AdminSys]
				team = "red";
			}
			else {
				team = "blue";
			}
		}
		else {
			team = "red";
		}
	}
//	Info_SetValueForKey( userinfo, "characterfile", Info_ValueForKey( botinfo, "aifile" ) );
	Info_SetValueForKey( userinfo, "skill", va( "%5.2f", skill ) );
	Info_SetValueForKey( userinfo, "team", team );
	//[TABBot]
	Info_SetValueForKey( userinfo, "bottype", va( "%i", bottype) );
	//[/TABBot]

	bot = &g_entities[ clientNum ];
	bot->r.svFlags |= SVF_BOT;
	bot->inuse = qtrue;

	// register the userinfo
	trap_SetUserinfo( clientNum, userinfo );

	//[NewGameTypes][EnhancedImpliment]
	//if (g_gametype.integer >= GT_TEAM && g_gametype.integer != GT_RPG)
	if (g_gametype.integer >= GT_TEAM)
	//[/NewGameTypes][EnhancedImpliment]
	{
		if (team && Q_stricmp(team, "red") == 0)
		{
			bot->client->sess.sessionTeam = TEAM_RED;
		}
		else if (team && Q_stricmp(team, "blue") == 0)
		{
			bot->client->sess.sessionTeam = TEAM_BLUE;
		}
		else
		{
			//[AdminSys]
			bot->client->sess.sessionTeam = PickTeam( -1, qtrue );
			//bot->client->sess.sessionTeam = PickTeam( -1 );
			//[/AdminSys]
		}
	}

	if (g_gametype.integer == GT_SIEGE)
	{
		bot->client->sess.siegeDesiredTeam = bot->client->sess.sessionTeam;
		bot->client->sess.sessionTeam = TEAM_SPECTATOR;
	}

	preTeam = bot->client->sess.sessionTeam;

	// have it connect to the game as a normal client
	if ( ClientConnect( clientNum, qtrue, qtrue ) ) {
		return;
	}

	if (bot->client->sess.sessionTeam != preTeam)
	{
		trap_GetUserinfo(clientNum, userinfo, MAX_INFO_STRING);

		if (bot->client->sess.sessionTeam == TEAM_SPECTATOR)
		{
			bot->client->sess.sessionTeam = preTeam;
		}

		if (bot->client->sess.sessionTeam == TEAM_RED)
		{
			team = "Red";
		}
		else
		{
			if (g_gametype.integer == GT_SIEGE)
			{
				if (bot->client->sess.sessionTeam == TEAM_BLUE)
				{
					team = "Blue";
				}
				else
				{
					team = "s";
				}
			}
			else
			{
				team = "Blue";
			}
		}

		Info_SetValueForKey( userinfo, "team", team );

		trap_SetUserinfo( clientNum, userinfo );

		bot->client->ps.persistant[ PERS_TEAM ] = bot->client->sess.sessionTeam;

		G_ReadSessionData( bot->client );
		ClientUserinfoChanged( clientNum );
	}

	if (g_gametype.integer == GT_DUEL ||
		g_gametype.integer == GT_POWERDUEL)
	{
		int loners = 0;
		int doubles = 0;

		bot->client->sess.duelTeam = 0;
		G_PowerDuelCount(&loners, &doubles, qtrue);

		if (!doubles || loners > (doubles/2))
		{
            bot->client->sess.duelTeam = DUELTEAM_DOUBLE;
		}
		else
		{
            bot->client->sess.duelTeam = DUELTEAM_LONE;
		}

		bot->client->sess.sessionTeam = TEAM_SPECTATOR;
		SetTeam(bot, "s");
	}
	else
	{
		if( delay == 0 ) {
			ClientBegin( clientNum, qfalse );
			//UNIQUEFIX - what's the purpose of this?
			//ClientUserinfoChanged( clientNum );
			return;
		}

		AddBotToSpawnQueue( clientNum, delay );
		//UNIQUEFIX - what's the purpose of this?
		//ClientUserinfoChanged( clientNum );
	}
}


/*
===============
Svcmd_AddBot_f
===============
*/
void Svcmd_AddBot_f( void ) {
	float			skill;
	int				delay;
	char			name[MAX_TOKEN_CHARS];
	char			altname[MAX_TOKEN_CHARS];
	char			string[MAX_TOKEN_CHARS];
	char			team[MAX_TOKEN_CHARS];
	//[TABBot]
	int				bottype;
	//[/TABBot]

	// are bots enabled?
	if ( !trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		return;
	}

	// name
	trap_Argv( 1, name, sizeof( name ) );
	if ( !name[0] ) {
		//[TABBots]
		trap_Printf( "Usage: Addbot <botname> [skill 1-5] [team] [msec delay] [altname] [bottype]\n" );
		//trap_Printf( "Usage: Addbot <botname> [skill 1-5] [team] [msec delay] [altname]\n" );
		//[/TABBots]
		return;
	}

	// skill
	trap_Argv( 2, string, sizeof( string ) );
	if ( !string[0] ) {
		skill = 4;
	}
	else {
		skill = atof( string );
	}

	// team
	trap_Argv( 3, team, sizeof( team ) );

	// delay
	trap_Argv( 4, string, sizeof( string ) );
	if ( !string[0] ) {
		delay = 0;
	}
	else {
		delay = atoi( string );
	}

	// alternative name
	trap_Argv( 5, altname, sizeof( altname ) );

	//[TABBot]
	trap_Argv( 6, string, sizeof( string ) );
	if ( !string[0] ) 
	{
		bottype = BOT_TAB;
	}
	else 
	{
		bottype = atoi( string );
	}
	G_AddBot( name, skill, team, delay, altname, bottype );
	//G_AddBot( name, skill, team, delay, altname );
	//[/TABBot]

	// if this was issued during gameplay and we are playing locally,
	// go ahead and load the bot's media immediately
	if ( level.time - level.startTime > 1000 &&
		trap_Cvar_VariableIntegerValue( "cl_running" ) ) {
		trap_SendServerCommand( -1, "loaddefered\n" );	// FIXME: spelled wrong, but not changing for demo
	}
}

/*
===============
Svcmd_BotList_f
===============
*/
void Svcmd_BotList_f( void ) {
	int i;
	char name[MAX_TOKEN_CHARS];
	char funname[MAX_TOKEN_CHARS];
	char model[MAX_TOKEN_CHARS];
	char personality[MAX_TOKEN_CHARS];

	trap_Printf("^1name             model            personality              funname\n");
	for (i = 0; i < g_numBots; i++) {
		strcpy(name, Info_ValueForKey( g_botInfos[i], "name" ));
		if ( !*name ) {
			strcpy(name, "Padawan");
		}
		strcpy(funname, Info_ValueForKey( g_botInfos[i], "funname" ));
		if ( !*funname ) {
			strcpy(funname, "");
		}
		strcpy(model, Info_ValueForKey( g_botInfos[i], "model" ));
		if ( !*model ) {
			strcpy(model, "kyle/default");
		}
		strcpy(personality, Info_ValueForKey( g_botInfos[i], "personality"));
		if (!*personality ) {
			strcpy(personality, "botfiles/kyle.jkb");
		}
		trap_Printf(va("%-16s %-16s %-20s %-20s\n", name, model, personality, funname));
	}
}

#if 0
/*
===============
G_SpawnBots
===============
*/
static void G_SpawnBots( char *botList, int baseDelay ) {
	char		*bot;
	char		*p;
	float		skill;
	int			delay;
	char		bots[MAX_INFO_VALUE];

	podium1 = NULL;
	podium2 = NULL;
	podium3 = NULL;

	skill = trap_Cvar_VariableValue( "g_spSkill" );
	if( skill < 1 ) {
		trap_Cvar_Set( "g_spSkill", "1" );
		skill = 1;
	}
	else if ( skill > 5 ) {
		trap_Cvar_Set( "g_spSkill", "5" );
		skill = 5;
	}

	Q_strncpyz( bots, botList, sizeof(bots) );
	p = &bots[0];
	delay = baseDelay;
	while( *p ) {
		//skip spaces
		while( *p && *p == ' ' ) {
			p++;
		}
		if( !p ) {
			break;
		}

		// mark start of bot name
		bot = p;

		// skip until space of null
		while( *p && *p != ' ' ) {
			p++;
		}
		if( *p ) {
			*p++ = 0;
		}

		// we must add the bot this way, calling G_AddBot directly at this stage
		// does "Bad Things"
		trap_SendConsoleCommand( EXEC_INSERT, va("addbot \"%s\" %f free %i\n", bot, skill, delay) );

		delay += BOT_BEGIN_DELAY_INCREMENT;
	}
}
#endif


/*
===============
G_LoadBotsFromFile
===============
*/
static void G_LoadBotsFromFile( char *filename ) {
	int				len;
	fileHandle_t	f;
	char			buf[MAX_BOTS_TEXT];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Printf( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}
	if ( len >= MAX_BOTS_TEXT ) {
		trap_Printf( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_BOTS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	g_numBots += G_ParseInfos( buf, MAX_BOTS - g_numBots, &g_botInfos[g_numBots] );
}

/*
===============
G_LoadBots
===============
*/
static void G_LoadBots( void ) {
	vmCvar_t	botsFile;
	int			numdirs;
	char		filename[128];
	char		dirlist[1024];
	char*		dirptr;
	int			i;
	int			dirlen;

	if ( !trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		return;
	}

	g_numBots = 0;

	trap_Cvar_Register( &botsFile, "g_botsFile", "", CVAR_INIT|CVAR_ROM );
	if( *botsFile.string ) {
		G_LoadBotsFromFile(botsFile.string);
	}
	else {
		//G_LoadBotsFromFile("scripts/bots.txt");
		G_LoadBotsFromFile("botfiles/bots.txt");
	}

	// get all bots from .bot files
	numdirs = trap_FS_GetFileList("scripts", ".bot", dirlist, 1024 );
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen+1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		G_LoadBotsFromFile(filename);
	}
//	trap_Printf( va( "%i bots parsed\n", g_numBots ) );
}



/*
===============
G_GetBotInfoByNumber
===============
*/
char *G_GetBotInfoByNumber( int num ) {
	if( num < 0 || num >= g_numBots ) {
		trap_Printf( va( S_COLOR_RED "Invalid bot number: %i\n", num ) );
		return NULL;
	}
	return g_botInfos[num];
}


/*
===============
G_GetBotInfoByName
===============
*/
char *G_GetBotInfoByName( const char *name ) {
	int		n;
	char	*value;

	for ( n = 0; n < g_numBots ; n++ ) {
		value = Info_ValueForKey( g_botInfos[n], "name" );
		if ( !Q_stricmp( value, name ) ) {
			return g_botInfos[n];
		}
	}

	return NULL;
}

//rww - pd
void LoadPath_ThisLevel(void);
//end rww

/*
===============
G_InitBots
===============
*/
void G_InitBots( qboolean restart ) {
	G_LoadBots();
	G_LoadArenas();

	trap_Cvar_Register( &bot_minplayers, "bot_minplayers", "0", CVAR_SERVERINFO );

	//rww - new bot route stuff
	LoadPath_ThisLevel();
	//end rww
}
