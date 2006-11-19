//[CoOp]
//[CoOpEditor]
//g_autosave.c


#include "g_local.h"

//the max autosave file size define
#define MAX_AUTOSAVE_FILESIZE 1024

extern void Use_Autosave( gentity_t *ent, gentity_t *other, gentity_t *activator );
void Touch_Autosave(gentity_t *self, gentity_t *other, trace_t *trace)
{//touch function used by SP_trigger_autosave
	if(!other || !other->inuse  || !other->client || !(other->s.number < MAX_CLIENTS) )
	{//not a valid player
		return;
	}

	//activate the autosave.
	Use_Autosave(self, NULL, other);
}


extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);
void SpawnPointRender()
{//renders all the spawnpoints used for CoOp so that the CoOp editor can add more of them as needed.
	//warning: It's assumed that the debounce for this function is handled elsewhere (in this case BotWaypointRender)
	//without a debounce, this can overload and crash the game with events!
	gentity_t *spawnpoint = NULL;
	vec3_t end;
	
	while ( (spawnpoint = G_Find( spawnpoint, FOFS( classname ), "info_player_deathmatch" )) != NULL )
	{//Scan thru all the coop spawnpoints
		if(spawnpoint->spawnflags & 1)
		{//this is a spawnpoint created by our coopeditor, display it.
			VectorCopy(spawnpoint->r.currentOrigin, end);
			end[2] += 20;
			G_TestLine(spawnpoint->r.currentOrigin, end, SABER_BLUE, FRAMETIME);
		}
	}
}


void AutosaveRender()
{//renders all the autosaves used for CoOp so that the CoOp editor can add more of them as needed.
	//warning: It's assumed that the debounce for this function is handled elsewhere (in this case BotWaypointRender)
	//without a debounce, this can overload and crash the game with events!
	gentity_t *autosave = NULL;
	vec3_t end, start;
	
	while ( (autosave = G_Find( autosave, FOFS( classname ), "trigger_autosave" )) != NULL )
	{//Scan thru all the coop autosaves
		if(autosave->spawnflags & AUTOSAVE_EDITOR)
		{//this is an autosave created by our coopeditor, display it.
			VectorAdd(autosave->r.currentOrigin, autosave->r.maxs, start);
			VectorAdd(autosave->r.currentOrigin, autosave->r.mins, end);
			G_TestLine(start, end, 0x000000, FRAMETIME);
		}
	}
}


/*QUAKED trigger_autosave (1 0 0) (-4 -4 -4) (4 4 4)
This is map entity is used by the Autosave Editor to place additional autosaves in premade
maps.  This should not be used by map creators as it must be called from the autosave 
editor code.
*/
extern vmCvar_t bot_wp_edit;
extern void InitTrigger( gentity_t *self );
void SP_trigger_autosave(gentity_t *self)
{
	InitTrigger(self);

	self->touch = Touch_Autosave;

	self->classname = "trigger_autosave";

	trap_LinkEntity(self);
}


extern void SP_info_player_start(gentity_t *ent);
void Create_Autosave( vec3_t origin, int size, qboolean teleportPlayers )
{//create a new SP_trigger_autosave.
	gentity_t* newAutosave = G_Spawn();

	if(newAutosave)
	{
		G_SetOrigin(newAutosave, origin);

		if(size == -1)
		{//create a spawnpoint at this location
			SP_info_player_start(newAutosave);
			newAutosave->spawnflags |= 1;	//set manually created flag

			//manually created spawnpoints have the lowest priority.
			newAutosave->genericValue1 = 50; 
			return;
		}

		if(teleportPlayers)
		{//we want to force all players to teleport to this autosave.
			//use this to get around tricky scripting in the maps.
			newAutosave->spawnflags |= AUTOSAVE_TELE;
		}

		if(!size)
		{//default size
			size = 30;
		}

		//create bbox cube.
		newAutosave->r.maxs[0] = size;
		newAutosave->r.maxs[1] = size;
		newAutosave->r.maxs[2] = size;
		newAutosave->r.mins[0] = -size;
		newAutosave->r.mins[1] = -size;
		newAutosave->r.mins[2] = -size;

		SP_trigger_autosave(newAutosave);

		newAutosave->spawnflags |= AUTOSAVE_EDITOR; //indicate that this was manually created and should be rendered if we're in the coop editor.
	}
}


void Load_Autosaves(void)
{//load in our autosave from the .autosp
	char			*s;
	int				len;
	fileHandle_t	f;
	char			buf[MAX_AUTOSAVE_FILESIZE];
	char			loadPath[MAX_QPATH];
	vec3_t			positionData;
	int				sizeData;
	//[RawMapName]
	//vmCvar_t		mapname;
	//[/RawMapName]
	qboolean		teleportPlayers = qfalse;

	G_Printf("^5Loading Autosave File Data...");

	//[RawMapName]
	//trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	//Com_sprintf(loadPath, MAX_QPATH, "maps/%s.autosp", mapname.string);
	Com_sprintf(loadPath, sizeof(loadPath), "maps/%s.autosp", level.rawmapname);
	//[/RawMapName]

	len = trap_FS_FOpenFile( loadPath, &f, FS_READ );
	if ( !f )
	{
		G_Printf("^5No autosave file found.\n");
		return;
	}
	if ( !len )
	{ //empty file
		G_Printf("^5Empty autosave file!\n");
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	trap_FS_FCloseFile( f );

	s = buf;

	while(*s != '\0' && s-buf < len)
	{
		if(*s == '\n')
		{//hop over newlines
			s++;
			continue;
		}

		sscanf(s, "%f %f %f %i %i", &positionData[0], &positionData[1], &positionData[2], &sizeData, &teleportPlayers);

		Create_Autosave( positionData, sizeData, teleportPlayers );

		//advance to the end of the line
		while(*s != '\n' && *s != '\0' && s-buf < len)
		{
			s++;
		}
	}

	G_Printf("^5Done.\n");
}


void Save_Autosaves(void)
{//save the autosaves
	fileHandle_t	f;
	char			lineBuf[MAX_QPATH];
	char			fileBuf[MAX_AUTOSAVE_FILESIZE];
	char			loadPath[MAX_QPATH];
	int				len;
	//[RawMapName]
	//vmCvar_t		mapname;
	//[/RawMapName]
	gentity_t*		autosavePoint;

	fileBuf[0] = '\0';

	G_Printf("^5Saving Autosave File Data...");

	//[RawMapName]
	//trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	//Com_sprintf(loadPath, MAX_QPATH, "maps/%s.autosp", mapname.string);
	Com_sprintf(loadPath, sizeof(loadPath), "maps/%s.autosp", level.rawmapname);
	//[/RawMapName]

	len = trap_FS_FOpenFile( loadPath, &f, FS_WRITE );
	if ( !f )
	{
		G_Printf("^5Couldn't create autosave file.\n");
		return;
	}

	//find all the manually created autosave points.
	autosavePoint = NULL;
	while ( (autosavePoint = G_Find (autosavePoint, FOFS(classname), "trigger_autosave")) != NULL )
	{
		Com_sprintf(lineBuf, MAX_QPATH, "%f %f %f %i %i\n", 
			autosavePoint->r.currentOrigin[0], autosavePoint->r.currentOrigin[1],
			autosavePoint->r.currentOrigin[2], (int) autosavePoint->r.maxs[0],
			(autosavePoint->spawnflags & AUTOSAVE_TELE) );
		strcat(fileBuf, lineBuf);
	}

	//find all the manually added spawnpoints
	autosavePoint = NULL;
	while ( (autosavePoint = G_Find (autosavePoint, FOFS(classname), "info_player_deathmatch")) != NULL )
	{
		if(!(autosavePoint->spawnflags & 1))
		{//not a manually placed spawnpoint
			continue;
		}
		Com_sprintf(lineBuf, MAX_QPATH, "%f %f %f %i %i\n", 
			autosavePoint->r.currentOrigin[0], autosavePoint->r.currentOrigin[1],
			autosavePoint->r.currentOrigin[2], -1, 0 );
		strcat(fileBuf, lineBuf);
	}

	if(fileBuf[0] != '\0')
	{//actually written something
		trap_FS_Write( fileBuf, strlen(fileBuf), f );
	}
	trap_FS_FCloseFile( f );
	G_Printf("^5Done.\n");
}


extern qboolean G_PointInBounds( vec3_t point, vec3_t mins, vec3_t maxs );
void Delete_Autosaves(gentity_t* ent)
{
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd( ent->r.currentOrigin, ent->r.mins, mins );
	VectorAdd( ent->r.currentOrigin, ent->r.maxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) 
	{
		hit = &g_entities[touch[i]];

		if(Q_stricmp (hit->classname, "trigger_autosave") == 0)
		{//found a manually set autosave entity
			G_FreeEntity(hit);
		}
	}

	hit = NULL;
	while ( (hit = G_Find (hit, FOFS(classname), "info_player_deathmatch")) != NULL )
	{
		if(hit->spawnflags & 1 
			&& G_PointInBounds(hit->r.currentOrigin, mins, maxs))
		{//found a manually set spawn point
			G_FreeEntity(hit);
		}
	}
}
//[/CoOpEditor]
//[/CoOp]

