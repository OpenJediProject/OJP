//[dynamicMusic]
//g_dynmusic.c
//dynamic music code file
#include "g_local.h"
#include "g_dynmusic.h"

DynamicMusicGroup_t DMSData;		//holds all our dynamic music data

void LoadDynamicMusicGroup(char *mapname, char *buffer);

void LoadDynamicMusic(void)
{//Tries to load dynamic music data for this map.
	int				len = 0;
	fileHandle_t	f;
	char			buffer[DMS_INFO_SIZE];
	vmCvar_t	mapname;

	//Open up the dynamic music file
	len = trap_FS_FOpenFile("ext_data/dms.dat", &f, FS_READ);

	if (!f)
	{//file open error
		G_Printf("LoadDynamicMusic() Error: Couldn't open ext_data/dms.dat\n");
		return;
	}

	if(len >= DMS_INFO_SIZE)
	{//file too large for buffer
		G_Printf("LoadDynamicMusic() Error: dms.dat too big.\n");
		return;
	}

	trap_FS_Read(buffer, len, f);	//read data in buffer

	trap_FS_FCloseFile(f);	//close file

	trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	
	LoadDynamicMusicGroup(mapname.string, buffer);
}


//init the DMS data for the given song/song type
extern int BG_SiegeGetPairedValue(char *buf, char *key, char *outbuf);
extern int BG_SiegeGetValueGroup(char *buf, char *group, char *outbuf);
void LoadDMSSongData(char *buffer, char *song, DynamicMusicSet_t *songData, char *mapname)
{
	char SongGroup[DMS_INFO_SIZE];
	char *transition;		//pointer used to advance the checks for transitions
	char transitionGroup[DMS_INFO_SIZE];
	char Value[MAX_QPATH];
	int numTransitions = 0;
	int numExits = 0;						

	BG_SiegeGetValueGroup(buffer, "musicfiles", SongGroup);

	//find our specific song
	if(!BG_SiegeGetValueGroup(SongGroup, song, SongGroup))
	{
		G_Printf("LoadDMSSongData Error: Couldn't find song data for DMS song %s.\n",
			song);
		return;
	}

	//convert/store the name of the music file
	strcpy(songData->fileName, va("music/%s/%s.mp3", mapname, song));	

	songData->numTransitions = 0;	//init the struct's number of transitions.

	//start loading in transition data
	transition = strstr(SongGroup, "exit");
	while(transition)
	{//still have a transition file to add

		if(numTransitions >= MAX_DMS_TRANSITIONS)
		{//too many transitions!
			G_Printf("LoadDMSSongData Error:  Too many transitions found.\n");
			return;
		}

		//setting up the new transition data slot
		songData->Transitions[numTransitions].numExitPoints = 0;

		//grab this transition group
		BG_SiegeGetValueGroup(transition, "exit", transitionGroup);

		//find transition file name
		BG_SiegeGetPairedValue(transitionGroup, "nextfile", Value);
		strcpy(songData->Transitions[numTransitions].fileName, 
			va("music/%s/%s.mp3", mapname, Value));

		//load in exit points for this transition file
		while(BG_SiegeGetPairedValue(transitionGroup, va("time%i", numExits), Value))
		{
			if(numExits >= MAX_DMS_EXITPOINTS)
			{//too many transitions!
				G_Printf("LoadDMSSongData Error:  Too many transitions found.\n");
				return;
			}

			songData->Transitions[numTransitions].exitPoints[numExits] 
			= atoi(Value) * 1000;

			numExits++;
			songData->Transitions[numTransitions].numExitPoints++;
		}
	
		//increase the number of transitions in the songData
		songData->numTransitions++;

		numTransitions++;
		numExits = 0;

		//advance the transition pointer pass the current exit data group
		transition += 4;
		transition = strstr(transition, "exit");
	}
}


void LoadLengthforSong(char *buffer, DynamicMusicSet_t *song)
{//load in the song lengths for the given DMS song
	char TempLength[MAX_QPATH];
	char token[MAX_QPATH];
	int transNum = song->numTransitions;

	//get length for the primary song

	//grab the token name
	char *tokenpointer = strrchr(song->fileName, '/');
	tokenpointer++;
	strcpy(token, tokenpointer);
	tokenpointer = strrchr(token, '.');
	*tokenpointer = '\0';
	

	BG_SiegeGetPairedValue(buffer, token, TempLength);
	song->fileLength = atoi(TempLength) * 1000;

	//find the song length for the transitions
	for(; transNum > 0; transNum--)
	{
		//grab pointer
		tokenpointer = strrchr(song->Transitions[transNum-1].fileName, '/');
		tokenpointer++;
		strcpy(token, tokenpointer);
		tokenpointer = strrchr(token, '.');
		*tokenpointer = '\0';

        if(BG_SiegeGetPairedValue(buffer, token, TempLength))
		{
			song->Transitions[transNum-1].fileLength = (int) (atof(TempLength) * (float) 1000);
		}
		else
		{//couldn't find this music file's length, use default
			G_Printf("LoadLengthforSong Warning: Couldn't find length for %s.\n", 
				token);
			song->Transitions[transNum-1].fileLength = DMS_MUSICFILE_DEFAULT;
		}
	}


}

//loads in the song lengths for the DMS music files
void LoadDMSSongLengths(void)
{
	char buffer[DMS_INFO_SIZE];
	fileHandle_t	f;
	int len;

	if(!DMSData.valid)
	{//oh boy, no DMSData.  Probably means that this map doesn't use DMS.
		return;
	}

	//Open up the dynamic music file
	len = trap_FS_FOpenFile(DMS_MUSICLENGTH_FILENAME, &f, FS_READ);

	if (!f)
	{//file open error
		G_Printf("LoadDynamicMusic() Error: Couldn't open ext_data/dms.dat\n");
		return;
	}

	if(len >= DMS_INFO_SIZE)
	{//file too large for buffer
		G_Printf("LoadDynamicMusic() Error: dms.dat too big.\n");
		return;
	}

	trap_FS_Read(buffer, len, f);	//read data in buffer

	trap_FS_FCloseFile(f);	//close file

	if(!BG_SiegeGetValueGroup(buffer, "musiclengths", buffer))
	{
		G_Printf("LoadDMSSongLengths Error:  Couldn't find musiclengths define group in musiclength.dat.\n");
	}

	if(DMSData.actionMusic.valid)
	{//load the action music lengths
		LoadLengthforSong(buffer, &DMSData.actionMusic);
	}

	if(DMSData.exploreMusic.valid)
	{//load the explore music lengths
		LoadLengthforSong(buffer, &DMSData.exploreMusic);
	}

	if(DMSData.bossMusic.valid)
	{//load the boss music lengths
		LoadLengthforSong(buffer, &DMSData.bossMusic);
	}
}


//loads in the DMS data for this map
void LoadDynamicMusicGroup(char *mapname, char *buffer)
{
	char text[MAX_QPATH];
	char MapMusicGroup[DMS_INFO_SIZE];

	//initialize DMSData
	DMSData.valid = qfalse;
	DMSData.actionMusic.valid = qfalse;
	DMSData.exploreMusic.valid = qfalse;
	DMSData.bossMusic.valid = qfalse;

	BG_SiegeGetValueGroup(buffer, "levelmusic", MapMusicGroup);

	if(!BG_SiegeGetValueGroup(MapMusicGroup, mapname, MapMusicGroup))
	{
		G_Printf("LoadDynamicMusicGroup Error:  Couldn't find DMS entry for this map.\n");
		return;
	}

	if(BG_SiegeGetPairedValue(MapMusicGroup, "uses", text))
	{//this map uses the dynamic music set of another map.  Look for that set
		LoadDynamicMusicGroup( text, buffer );
		return;
	}

	//at this point, we have the dynamic music group for this map, init the
	//DMSData data slot.
	DMSData.valid = qtrue;
	DMSData.dmDebounceTime = -1;
	DMSData.dmBeatTime = 0;
	DMSData.dmState = DM_AUTO;
	DMSData.olddmState = DM_AUTO;

	if(BG_SiegeGetPairedValue(MapMusicGroup, "explore", text))
	{//have explore music for this map
		DMSData.exploreMusic.valid = qtrue;
		LoadDMSSongData(buffer, text, &DMSData.exploreMusic, mapname);
	}

	if(BG_SiegeGetPairedValue(MapMusicGroup, "action", text))
	{//have action music for this map
		DMSData.actionMusic.valid = qtrue;
		LoadDMSSongData(buffer, text, &DMSData.actionMusic, mapname);
	}

	if(BG_SiegeGetPairedValue(MapMusicGroup, "boss", text))
	{//have boss music for this map
		DMSData.bossMusic.valid = qtrue;
		LoadDMSSongData(buffer, text, &DMSData.bossMusic, mapname);
	}

	LoadDMSSongLengths();
}


//Do the transitions between DMS action/explore DMS states
void TransitionBetweenState(void)
{
	DynamicMusicSet_t *oldSongGroup;
	DynamicMusicSet_t *newSongGroup;
	int	songTime;
	int i, x;
	int TransitionTime;

	if((DMSData.olddmState != DM_ACTION && DMSData.olddmState != DM_EXPLORE) 
		|| !DMSData.dmStartTime)
	{//not transitioning between action and explore, just start the music
		if(DMSData.dmState == DM_ACTION)
		{//want to switch to action
 			trap_SetConfigstring( CS_MUSIC, DMSData.actionMusic.fileName );
		}
		else
		{//want to switch to explore
			trap_SetConfigstring( CS_MUSIC, DMSData.exploreMusic.fileName );
		}
		DMSData.olddmState = DMSData.dmState;
		DMSData.dmStartTime = level.time;
		return;
	}

	if(DMSData.dmState == DM_ACTION)
	{//transition is from explore's group
		oldSongGroup = &DMSData.exploreMusic;
		newSongGroup = &DMSData.actionMusic;
	}
	else
	{//transition from action
		oldSongGroup = &DMSData.actionMusic;
		newSongGroup = &DMSData.exploreMusic;
	}

	//find the closest transition point for this song state
	songTime = level.time - DMSData.dmStartTime;
	while((songTime) > oldSongGroup->fileLength)
	{//convert the start time to be in relation to the file length
		songTime -= oldSongGroup->fileLength;
	}

	//have the relative songTime, check to see if we're at one of the transition points
	for(i = 0; i <oldSongGroup->numTransitions; i++)
	{
		for(x = 0; x < oldSongGroup->Transitions[i].numExitPoints; x++)
		{
			TransitionTime = songTime - oldSongGroup->Transitions[i].exitPoints[x];
			if( TransitionTime == 0 
				|| (TransitionTime > 0 && TransitionTime < DMS_TRANSITIONFUDGEFACTOR))
			{//on the money or close enough
				trap_SetConfigstring( CS_MUSIC, va("%s %s", 
					oldSongGroup->Transitions[i].fileName, 
					newSongGroup->fileName));
				DMSData.olddmState = DMSData.dmState;
				DMSData.dmStartTime = level.time + oldSongGroup->Transitions[i].fileLength;
				return;
			}
		}
	}
}


//ported from SP
void G_DynamicMusicUpdate( void )
{
	int			battle = 0;
	vec3_t		center;
	qboolean	clearLOS = qfalse;
	int			distSq, radius = 2048;
	int			i, e, x;
	gentity_t	*ent;
	int entityList[MAX_GENTITIES];
	int			entTeam;
	vec3_t		mins, maxs;
	int			numListedEntities;
	gentity_t	*player;

	if( DMSData.dmDebounceTime >= 0 && DMSData.dmDebounceTime < level.time )
	{//debounce over, reset to default music
		DMSData.dmDebounceTime = -1;
		DMSData.dmState = DM_AUTO;
		DMSData.olddmState = DM_AUTO;
	}

	if ( DMSData.dmState == DM_DEATH)
	{//Play the death music
		if(DMSData.olddmState != DM_DEATH)
		{//haven't set the state yet
			trap_SetConfigstring( CS_MUSIC, DMS_DEATH_MUSIC );
			DMSData.olddmState = DM_DEATH;
			DMSData.dmDebounceTime = level.time + DMS_DEATH_MUSIC_TIME;
		}
		return;
	}

	if ( DMSData.dmState == DM_BOSS )
	{
		if(DMSData.olddmState != DM_BOSS)
		{
			trap_SetConfigstring( CS_MUSIC, DMSData.bossMusic.fileName );
			DMSData.olddmState = DM_BOSS;
		}
		return;
	}

	if ( DMSData.dmState == DM_SILENCE )
	{//turn off the music
		if(DMSData.olddmState != DM_SILENCE)
		{
			trap_SetConfigstring( CS_MUSIC, "" );
			DMSData.olddmState = DM_SILENCE;
		}
		return;
	}

	if ( DMSData.dmBeatTime > level.time )
	{//not on a beat
		return;
	}

	DMSData.dmBeatTime = level.time + 1000;//1 second beats

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		player = &g_entities[i];

		//check to make sure this player is valid
		if(!player || !player->inuse 
			|| player->client->pers.connected == CON_DISCONNECTED
			|| player->client->sess.sessionTeam == TEAM_SPECTATOR)
		{
			continue;
		}

		//enemy-based
		VectorCopy( player->r.currentOrigin, center );
		for ( x = 0 ; x < 3 ; x++ ) 
		{
			mins[x] = center[x] - radius;
			maxs[x] = center[x] + radius;
		}
	
		numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );
		for ( e = 0 ; e < numListedEntities ; e++ ) 
		{
			ent = &g_entities[entityList[e]];
			if ( !ent || !ent->inuse )
			{
				continue;
			}

			if ( !ent->client || !ent->NPC )
			{
				if ( ent->classname && (!Q_stricmp( "PAS", ent->classname )||!Q_stricmp( "misc_turret", ent->classname )) )
				{//a turret
					entTeam = ent->teamnodmg;
					//entTeam = ent->noDamageTeam;
				}
				else
				{
					continue;
				}
			}
			else
			{//an NPC
				entTeam = ent->client->playerTeam;
			}

			if ( entTeam == player->client->playerTeam )
			{//ally
				continue;
			}

			if ( entTeam == TEAM_FREE && (!ent->enemy || !ent->enemy->client || ent->enemy->client->playerTeam != player->client->playerTeam) )
			{//a droid that is not mad at me or my allies
				continue;
			}

			if ( !trap_InPVS( player->r.currentOrigin, ent->r.currentOrigin ) )
			{//not potentially visible
				continue;
			}

			if ( ent->client && ent->s.weapon == WP_NONE )
			{//they don't have a weapon... FIXME: only do this for droids?
				continue;
			}

			clearLOS = qfalse;
			if ( (ent->enemy==player&&(!ent->NPC||ent->NPC->confusionTime<level.time)) || (ent->client&&ent->client->ps.weaponTime) || (!ent->client&&ent->attackDebounceTime>level.time))
			{//mad
				if ( ent->health > 0 )
				{//alive
					//FIXME: do I really need this check?
					if ( ent->s.weapon == WP_SABER && ent->client && ent->client->ps.saberHolstered == 2 && ent->enemy != player )
					{//a Jedi who has not yet gotten mad at me
						continue;
					}
					if ( ent->NPC && ent->NPC->behaviorState == BS_CINEMATIC )
					{//they're not actually going to do anything about being mad at me...
						continue;
					}
					//okay, they're in my PVS, but how close are they?  Are they actively attacking me?
					if ( !ent->client && ent->s.weapon == WP_TURRET && ent->fly_sound_debounce_time && ent->fly_sound_debounce_time - level.time < 10000 )
					{//a turret that shot at me less than ten seconds ago
					}
					else if( ent->NPC && level.time < ent->NPC->shotTime )
					{//npc that fired recently
					}
					/* changed from SP
					else if ( ent->client && ent->client->ps.lastShotTime && ent->client->ps.lastShotTime - level.time < 10000 )
					{//an NPC that shot at me less than ten seconds ago
					}
					*/
					else
					{//not actively attacking me lately, see how far away they are
						distSq = DistanceSquared( ent->r.currentOrigin, player->r.currentOrigin );
						if ( distSq > 4194304 )
						{//> 2048 away
							continue;
						}
						else if ( distSq > 1048576 )
						{//> 1024 away
							clearLOS = G_ClearLOS3( player, player->client->renderInfo.eyePoint, ent );
							if ( clearLOS == qfalse )
							{//No LOS
								continue;
							}
						}
					}
					battle++;
				}
			}
		}

		if ( !battle )
		{//no active enemies, but look for missiles, shot impacts, etc...
			//[CoOp]
			int alert = G_CheckAlertEvents( player, qtrue, qtrue, 1024, 1024, -1, qfalse, AEL_SUSPICIOUS, qfalse );
			//int alert = G_CheckAlertEvents( player, qtrue, qtrue, 1024, 1024, -1, qfalse, AEL_SUSPICIOUS );
			//[/CoOp]
			if ( alert != -1 )
			{//FIXME: maybe tripwires and other FIXED things need their own sound, some kind of danger/caution theme
				if ( G_CheckForDanger( player, alert ) )
				{//found danger near by
					battle = 1;
				}
			}
		}
	}

	if ( battle )
	{
		SetDMSState(DM_ACTION);
	}
	else 
	{//switch to explore
		SetDMSState(DM_EXPLORE);
	}

	if(DMSData.dmState != DMSData.olddmState)
	{//switching between action and explore modes
		TransitionBetweenState();
	}
}


//set the dynamic music system's desired state
void SetDMSState( int DMSState )
{
	if(DMSData.valid && DMSData.olddmState != DMSState)
	{
		DMSData.dmState = DMSState;
		DMSData.dmDebounceTime = -1;
	}
}
//[/dynamicMusic]

