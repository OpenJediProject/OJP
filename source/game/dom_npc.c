#ifdef __DOMINANCE__

#include "g_local.h"

extern vmCvar_t g_maxNPCs;

extern float VectorDistance(vec3_t v1, vec3_t v2);
extern int OrgVisibleBox(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, int ignore);
extern qboolean CheckAboveOK_Player(vec3_t origin);
extern qboolean CheckBelowOK(vec3_t origin);
extern qboolean CheckEntitiesInSpot(vec3_t point);
extern gentity_t *NPC_Spawn_Do( gentity_t *ent );

#ifndef TEAM_NONE
#define TEAM_NONE 0
#endif

extern int number_of_flags;
int	last_flag_num = 0;
extern int redtickets;
extern int bluetickets;

extern qboolean AdvancedWouldTelefrag(vec3_t point);

int CountNPCS ( void )
{
	int numnpcs = 0;
	int loop = 0;

	for (loop = 0; loop < MAX_GENTITIES; loop++)
	{
		if (g_entities[loop].s.eType == ET_NPC 
			/*&& g_entities[loop].inuse*/)
		{// Found one!
			numnpcs++;
		}
	}

	return numnpcs;
}

int CountREDNPCS ( void )
{
	int numnpcs = 0;
	int loop = 0;

	for (loop = 0; loop < MAX_GENTITIES; loop++)
	{
		if (g_entities[loop].s.eType == ET_NPC 
			&& g_entities[loop].client->playerTeam == NPCTEAM_ENEMY)
		{// Found one!
			numnpcs++;
		}
	}

	return numnpcs;
}

int CountBLUENPCS ( void )
{
	int numnpcs = 0;
	int loop = 0;

	for (loop = 0; loop < MAX_GENTITIES; loop++)
	{
		if (g_entities[loop].s.eType == ET_NPC 
			&& g_entities[loop].client->playerTeam == NPCTEAM_PLAYER)
		{// Found one!
			numnpcs++;
		}
	}

	return numnpcs;
}

extern void ST_ClearTimers( gentity_t *ent );
extern void Jedi_ClearTimers( gentity_t *ent );
extern void NPC_ShadowTrooper_Precache( void );
extern void NPC_Gonk_Precache( void );
extern void NPC_Mouse_Precache( void );
extern void NPC_Seeker_Precache( void );
extern void NPC_Remote_Precache( void );
extern void	NPC_R2D2_Precache(void);
extern void	NPC_R5D2_Precache(void);
extern void NPC_Probe_Precache(void);
extern void NPC_Interrogator_Precache(gentity_t *self);
extern void NPC_MineMonster_Precache( void );
extern void NPC_Howler_Precache( void );
extern void NPC_ATST_Precache(void);
extern void NPC_Sentry_Precache(void);
extern void NPC_Mark1_Precache(void);
extern void NPC_Mark2_Precache(void);
extern void NPC_GalakMech_Precache( void );
extern void NPC_GalakMech_Init( gentity_t *ent );
extern void NPC_Protocol_Precache( void );
extern void Boba_Precache( void );
extern void NPC_Wampa_Precache( void );
gentity_t *NPC_SpawnType( gentity_t *ent, char *npc_type, char *targetname, qboolean isVehicle );

gentity_t *AIMod_NPC_SpawnType_Location( vec3_t origin, vec3_t angles, char *npc_type, char *targetname, qboolean isVehicle, qboolean exact ) 
{// This uses a smaller area for shops.
	gentity_t		*NPCspawner = G_Spawn();
	vec3_t			forward, end, point;
	trace_t			trace;

	if(!NPCspawner)
	{
		Com_Printf( S_COLOR_RED"NPC_Spawn Error: Out of entities!\n" );
		return NULL;
	}

	NPCspawner->think = G_FreeEntity;
	NPCspawner->nextthink = level.time + FRAMETIME;
	
	if ( !npc_type )
	{
		return NULL;
	}

	if (!npc_type[0])
	{
		Com_Printf( S_COLOR_RED"Error, expected one of:\n"S_COLOR_WHITE" NPC spawn [NPC type (from ext_data/NPCs)]\n NPC spawn vehicle [VEH type (from ext_data/vehicles)]\n" );
		return NULL;
	}

/*	if ( !ent || !ent->client )
	{//screw you, go away
		return NULL;
	}*/

	//rwwFIXMEFIXME: Care about who is issuing this command/other clients besides 0?
	//Spawn it at spot of first player
	//FIXME: will gib them!
//	AngleVectors(ent->client->ps.viewangles, forward, NULL, NULL);
	AngleVectors(angles, forward, NULL, NULL);
	VectorNormalize(forward);
	VectorMA(origin, 64, forward, end);
	trap_Trace(&trace, origin, NULL, NULL, end, 0, MASK_SOLID);
	VectorCopy(trace.endpos, end);
	end[2] -= 24;
	trap_Trace(&trace, trace.endpos, NULL, NULL, end, 0, MASK_SOLID);
	VectorCopy(trace.endpos, end);
	end[2] += 24;
	G_SetOrigin(NPCspawner, end);
	VectorCopy(NPCspawner->r.currentOrigin, NPCspawner->s.origin);
	//set the yaw so that they face away from player
	NPCspawner->s.angles[1] = angles[1];

	trap_LinkEntity(NPCspawner);

	NPCspawner->NPC_type = G_NewString( npc_type );

	if ( targetname )
	{
		NPCspawner->NPC_targetname = G_NewString(targetname);
	}

	NPCspawner->count = 1;

	NPCspawner->delay = 0;

	//NPCspawner->spawnflags |= SFB_NOTSOLID;

	//NPCspawner->playerTeam = TEAM_FREE;
	//NPCspawner->behaviorSet[BSET_SPAWN] = "common/guard";
	
	if ( isVehicle )
	{
		NPCspawner->classname = "NPC_Vehicle";
	}

	//call precache funcs for James' builds
	if ( !Q_stricmp( "gonk", NPCspawner->NPC_type))
	{
		NPC_Gonk_Precache();
	}
	else if ( !Q_stricmp( "mouse", NPCspawner->NPC_type))
	{
		NPC_Mouse_Precache();
	}
	else if ( !Q_strncmp( "r2d2", NPCspawner->NPC_type, 4))
	{
		NPC_R2D2_Precache();
	}
	else if ( !Q_stricmp( "atst", NPCspawner->NPC_type))
	{
		NPC_ATST_Precache();
	}
	else if ( !Q_strncmp( "r5d2", NPCspawner->NPC_type, 4))
	{
		NPC_R5D2_Precache();
	}
	else if ( !Q_stricmp( "mark1", NPCspawner->NPC_type))
	{
		NPC_Mark1_Precache();
	}
	else if ( !Q_stricmp( "mark2", NPCspawner->NPC_type))
	{
		NPC_Mark2_Precache();
	}
	else if ( !Q_stricmp( "interrogator", NPCspawner->NPC_type))
	{
		NPC_Interrogator_Precache(NULL);
	}
	else if ( !Q_stricmp( "probe", NPCspawner->NPC_type))
	{
		NPC_Probe_Precache();
	}
	else if ( !Q_stricmp( "seeker", NPCspawner->NPC_type))
	{
		NPC_Seeker_Precache();
	}
	else if ( !Q_stricmp( "remote", NPCspawner->NPC_type))
	{
		NPC_Remote_Precache();
	}
	else if ( !Q_strncmp( "shadowtrooper", NPCspawner->NPC_type, 13 ) )
	{
		NPC_ShadowTrooper_Precache();
	}
	else if ( !Q_stricmp( "minemonster", NPCspawner->NPC_type ))
	{
		NPC_MineMonster_Precache();
	}
	else if ( !Q_stricmp( "howler", NPCspawner->NPC_type ))
	{
		NPC_Howler_Precache();
	}
	else if ( !Q_stricmp( "sentry", NPCspawner->NPC_type ))
	{
		NPC_Sentry_Precache();
	}
	else if ( !Q_stricmp( "protocol", NPCspawner->NPC_type ))
	{
		NPC_Protocol_Precache();
	}
	else if ( !Q_stricmp( "galak_mech", NPCspawner->NPC_type ))
	{
		NPC_GalakMech_Precache();
	}
	else if ( !Q_stricmp( "wampa", NPCspawner->NPC_type ))
	{
		NPC_Wampa_Precache();
	}

	VectorCopy(origin, point);

	if (exact || g_gametype.integer == GT_SINGLE_PLAYER)
	{// We need to be exactly in this spot...
		VectorCopy(origin, NPCspawner->s.origin);
		VectorCopy(angles, NPCspawner->s.angles);
	}
/*	else if (origin && !exact && mod_precalculateSpawns.integer < 1)
	{// Push spawner away from the spawn point! - AIMod.
		if (SP_SpawnPointsOK())
		{// Single Player spawns are available. Let's try to use one.
			gentity_t *ent = SelectRandomSPSpawnPoint( NPCspawner->s.origin, NPCspawner->s.angles );
			Calculate_Advanced_Spawnpoint( ent, point );
		}
		else
		{
			team_t team = PickTeam( -1 );

			gentity_t *ent = SelectSpawnPoint( NPCspawner->s.origin, NPCspawner->s.origin, NPCspawner->s.angles, team );
			Calculate_Advanced_Spawnpoint( ent, point );
		}

		VectorCopy(point, NPCspawner->s.origin);
	}*/
	else
	{
/*		if (SP_SpawnPointsOK())
		{// Single Player spawns are available. Let's try to use one.
			qboolean good = qfalse;
			gentity_t *ent = SelectRandomSPSpawnPoint( NPCspawner->s.origin, NPCspawner->s.angles );

			while (good == qfalse)
			{// Now checks for telefrags.
				ent = SelectRandomSPSpawnPoint( NPCspawner->s.origin, NPCspawner->s.angles );

				if (SpotWouldTelefrag( ent ))
					good = qtrue;
			}
			
			VectorCopy(ent->s.origin, NPCspawner->s.origin);
		}
		else*/
		{
			team_t team = PickTeam( -1 );

			gentity_t *ent = SelectSpawnPoint( NPCspawner->s.origin, NPCspawner->s.origin, NPCspawner->s.angles, team );
			VectorCopy(ent->s.origin, NPCspawner->s.origin);
		}
	}

	return (NPC_Spawn_Do( NPCspawner ));
}

void Supremacy_CreateNPC ( void )
{// Supremacy NPC spawning system.
	int num_inserted = 0;
	vec3_t newspawn;

	if /*while*/ (CountREDNPCS() < g_maxNPCs.integer * 0.5)
	{
		int spawnpoint_num = 0;
//		int number_of_flags = 0;
		int test_flag = 0;
		int flagnum = -1;
		qboolean notgood = qtrue;
		gentity_t *npc;
		int randnum = Q_irand(0, 6);

		if (number_of_flags >= 2)
		{// We have flags on the map... Find one that belongs to us..
			float best_dist = 64000.0f;
			qboolean redfound = qfalse;
			qboolean bluefound = qfalse;

			for (test_flag = 0; test_flag < number_of_flags; test_flag++)
			{// Check we have a playable map...
				if (flag_list[test_flag].flagentity)
				{
					if (flag_list[test_flag].flagentity->s.teamowner == TEAM_RED)
						redfound = qtrue;
					if (flag_list[test_flag].flagentity->s.teamowner == TEAM_BLUE)
						bluefound = qtrue;
				}
			}

			while (redfound && bluefound)
			{
				for (test_flag = 0; test_flag < number_of_flags; test_flag++)
				{// We need to find the most forward flag point to spawn at... FIXME: Selectable spawnpoints in UI...
					if (flag_list[test_flag].flagentity)
					{
						if (flag_list[test_flag].num_spawnpoints > 0
							&& flag_list[test_flag].flagentity->s.teamowner == TEAM_RED)
						{// This is our flag...
							int test = 0;

							//G_Printf("Found our flag!\n");

							for (test = 0; test < number_of_flags; test++)
							{// Find enemy flags...
								//qboolean useAnotherFlag = qfalse;

								if (Q_irand(0, 7) < 2)
								{// Pick a different one for randomcy...
									continue;//useAnotherFlag = qtrue;
								}

								if (flag_list[test].flagentity)
								{// FIXME: Selectable spawnpoints in UI...
									if (flag_list[test].flagentity->s.teamowner != TEAM_RED
										&& flag_list[test].flagentity->s.teamowner != TEAM_NONE)
									{// This is our enemy's flag...
										if (VectorDistance(flag_list[test_flag].flagentity->s.origin, flag_list[test].flagentity->s.origin) < best_dist)
										{
											if (flagnum != -1)
												last_flag_num = flagnum;

											flagnum = test_flag;
											best_dist = VectorDistance(flag_list[test_flag].flagentity->s.origin, flag_list[test].flagentity->s.origin);
											//G_Printf("Found their flag!\n");
										}
									}
								}
							}
						}
					}
				}

				if (best_dist != 64000.0f)
					break;

			}

			spawnpoint_num = -1;

			while (notgood && spawnpoint_num < flag_list[flagnum].num_spawnpoints)
			{
				spawnpoint_num++;	

				if (!AdvancedWouldTelefrag(flag_list[flagnum].spawnpoints[spawnpoint_num])
					&& !CheckEntitiesInSpot(flag_list[flagnum].spawnpoints[spawnpoint_num]))
				{
					G_Printf("^3*** ^3DominancE^5: Spawning ^3%s^5 at flag ^7%i^5 point ^7%i^5.\n", "Red NPC", flagnum, spawnpoint_num);
					notgood = qfalse;
				}
			}

			if (notgood)
			{
				spawnpoint_num = -1;

				while (notgood && spawnpoint_num < flag_list[last_flag_num].num_spawnpoints)
				{
					spawnpoint_num++;	

					if (!AdvancedWouldTelefrag(flag_list[last_flag_num].spawnpoints[spawnpoint_num])
						&& !CheckEntitiesInSpot(flag_list[last_flag_num].spawnpoints[spawnpoint_num]))
					{
						G_Printf("^3*** ^3DominancE^5: Spawning ^3%s^5 at flag ^7%i^5 point ^7%i^5.\n", "Red NPC", last_flag_num, spawnpoint_num);
						notgood = qfalse;
					}
				}

				VectorCopy(flag_list[last_flag_num].spawnpoints[spawnpoint_num], newspawn);
			}
			else
			{
				VectorCopy(flag_list[flagnum].spawnpoints[spawnpoint_num], newspawn);
			}
		}

		redtickets--;

		trap_SendServerCommand( -1, va("tkt %i %i", redtickets, bluetickets ));

		if (randnum == 1)
		{
			npc = AIMod_NPC_SpawnType_Location( newspawn, newspawn, va("%s", va("reborn")), va("%s", va("stormtrooper")), qfalse, qtrue );
		}
		else if (randnum == 2)
		{
			npc = AIMod_NPC_SpawnType_Location( newspawn, newspawn, va("%s", va("reborn_dual")), va("%s", va("reborn_dual")), qfalse, qtrue );
		}
		else if (randnum == 3)
		{
			npc = AIMod_NPC_SpawnType_Location( newspawn, newspawn, va("%s", va("reborn_staff")), va("%s", va("reborn_staff")), qfalse, qtrue );
		}
		else if (randnum == 4)
		{
			npc = AIMod_NPC_SpawnType_Location( newspawn, newspawn, va("%s", va("sentry")), va("%s", va("sentry")), qfalse, qtrue );
		}
		else if (randnum == 5)
		{
			npc = AIMod_NPC_SpawnType_Location( newspawn, newspawn, va("%s", va("STCommander")), va("%s", va("STCommander")), qfalse, qtrue );
		}
		else
		{
			npc = AIMod_NPC_SpawnType_Location( newspawn, newspawn, va("%s", va("stormtrooper")), va("%s", va("stormtrooper")), qfalse, qtrue );
		}

		// For droideka vehicle...
/*		if (!droideka_registerred)
		{
			Register_Hybrid_Vehicle_Models ( npc );
			droideka_registerred = qtrue;
		}*/

		if (npc)
		{// Managed to spawn him...
			//G_Printf("^5Spawning Scenario NPC^5: ^7%s^5 with a ^7%s^5 (^7%i^5).\n", NPCname, weaponNameFromIndex[npc->s.weapon], npc->s.weapon);
			num_inserted++;
		}
	}
	
	if /*while*/ (CountBLUENPCS() < g_maxNPCs.integer * 0.5)
	{
		int spawnpoint_num = 0;
		//int number_of_flags = 0;
		int test_flag = 0;
		int flagnum = -1;
		qboolean notgood = qtrue;
		gentity_t *npc;
		int randnum = Q_irand(0, 6);

		if (number_of_flags >= 2)
		{// We have flags on the map... Find one that belongs to us..
			float best_dist = 64000.0f;
			qboolean redfound = qfalse;
			qboolean bluefound = qfalse;

			for (test_flag = 0; test_flag < number_of_flags; test_flag++)
			{// Check we have a playable map...
				if (flag_list[test_flag].flagentity)
				{
					if (flag_list[test_flag].flagentity->s.teamowner == TEAM_RED)
						redfound = qtrue;
					if (flag_list[test_flag].flagentity->s.teamowner == TEAM_BLUE)
						bluefound = qtrue;
				}
			}

			while (redfound && bluefound)
			{
				for (test_flag = 0; test_flag < number_of_flags; test_flag++)
				{// We need to find the most forward flag point to spawn at... FIXME: Selectable spawnpoints in UI...
					if (flag_list[test_flag].flagentity)
					{
						if (flag_list[test_flag].num_spawnpoints > 0
							&& flag_list[test_flag].flagentity->s.teamowner == TEAM_BLUE)
						{// This is our flag...
							int test = 0;

							//G_Printf("Found our flag!\n");

							for (test = 0; test < number_of_flags; test++)
							{// Find enemy flags...
								//qboolean useAnotherFlag = qfalse;

								if (Q_irand(0, 7) < 2)
								{// Pick a different one for randomcy...
									continue;//useAnotherFlag = qtrue;
								}

								if (flag_list[test].flagentity)
								{// FIXME: Selectable spawnpoints in UI...
									if (flag_list[test].flagentity->s.teamowner != TEAM_BLUE
										&& flag_list[test].flagentity->s.teamowner != TEAM_NONE)
									{// This is our enemy's flag...
										if (VectorDistance(flag_list[test_flag].flagentity->s.origin, flag_list[test].flagentity->s.origin) < best_dist)
										{
											if (flagnum != -1)
												last_flag_num = flagnum;

											flagnum = test_flag;
											best_dist = VectorDistance(flag_list[test_flag].flagentity->s.origin, flag_list[test].flagentity->s.origin);
											//G_Printf("Found their flag!\n");
										}
									}
								}
							}
						}
					}
				}

				if (best_dist != 64000.0f)
					break;

			}

			spawnpoint_num = -1;

			while (notgood && spawnpoint_num < flag_list[flagnum].num_spawnpoints)
			{
				spawnpoint_num++;	

				if (!AdvancedWouldTelefrag(flag_list[flagnum].spawnpoints[spawnpoint_num])
					&& !CheckEntitiesInSpot(flag_list[flagnum].spawnpoints[spawnpoint_num]))
				{
					G_Printf("^3*** ^3DominancE^5: Spawning ^3%s^5 at flag ^7%i^5 point ^7%i^5.\n", "Blue NPC", flagnum, spawnpoint_num);
					notgood = qfalse;
				}
			}

			if (notgood)
			{
				spawnpoint_num = -1;

				while (notgood && spawnpoint_num < flag_list[last_flag_num].num_spawnpoints)
				{
					spawnpoint_num++;	

					if (!AdvancedWouldTelefrag(flag_list[last_flag_num].spawnpoints[spawnpoint_num])
						&& !CheckEntitiesInSpot(flag_list[last_flag_num].spawnpoints[spawnpoint_num]))
					{
						G_Printf("^3*** ^3DominancE^5: Spawning ^3%s^5 at flag ^7%i^5 point ^7%i^5.\n", "Blue NPC", flagnum, spawnpoint_num);
						notgood = qfalse;
					}
				}

				VectorCopy(flag_list[last_flag_num].spawnpoints[spawnpoint_num], newspawn);
			}
			else
			{
				VectorCopy(flag_list[flagnum].spawnpoints[spawnpoint_num], newspawn);
			}
		}

		redtickets--;

		trap_SendServerCommand( -1, va("tkt %i %i", redtickets, bluetickets ));

		if (randnum == 1)
		{
			npc = AIMod_NPC_SpawnType_Location( newspawn, newspawn, va("%s", va("JediTrainer")), va("%s", va("JediTrainer")), qfalse, qtrue );
		}
		else if (randnum == 2)
		{
			npc = AIMod_NPC_SpawnType_Location( newspawn, newspawn, va("%s", va("JediMaster")), va("%s", va("JediMaster")), qfalse, qtrue );
		}
		else if (randnum == 3)
		{
			npc = AIMod_NPC_SpawnType_Location( newspawn, newspawn, va("%s", va("jedi_hf1")), va("%s", va("jedi_hf1")), qfalse, qtrue );
		}
		else if (randnum == 4)
		{
			npc = AIMod_NPC_SpawnType_Location( newspawn, newspawn, va("%s", va("Jan")), va("%s", va("Jan")), qfalse, qtrue );
		}
		else if (randnum == 5)
		{
			npc = AIMod_NPC_SpawnType_Location( newspawn, newspawn, va("%s", va("MonMothma")), va("%s", va("MonMothma")), qfalse, qtrue );
		}
		else
		{
			npc = AIMod_NPC_SpawnType_Location( newspawn, newspawn, va("%s", va("bespincop")), va("%s", va("bespincop")), qfalse, qtrue );
		}

		// For droideka vehicle...
/*		if (!droideka_registerred)
		{
			Register_Hybrid_Vehicle_Models ( npc );
			droideka_registerred = qtrue;
		}*/

		if (npc)
		{// Managed to spawn him...
//			G_Printf("^5Spawning Scenario NPC^5: ^7%s^5 with a ^7%s^5 (^7%i^5).\n", NPCname, weaponNameFromIndex[npc->s.weapon], npc->s.weapon);
			num_inserted++;
		}
	}
}

#endif //__DOMINANCE__
