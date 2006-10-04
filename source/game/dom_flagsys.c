/*
¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤
Unique1's FlagSys Code.
¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤
Spawn_Flag_Base
flag_AI_Think_Axis
flag_AI_Think_Allied
flag_AI_Think
OM_Flag_Spawn_Test
OM_Flag_Spawn
¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤
*/

#ifdef __DOMINANCE__

#include "g_local.h"

extern float VectorDistance(vec3_t v1, vec3_t v2);
extern int OrgVisibleBox(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, int ignore);
extern qboolean CheckAboveOK_Player(vec3_t origin);
extern qboolean CheckBelowOK(vec3_t origin);
extern qboolean CheckEntitiesInSpot(vec3_t point);

#ifndef TEAM_NONE
#define TEAM_NONE 0
#endif

//===========================================================================
// Routine      : Spawn_Flag_Base
// Description  : Spawn a base to sit the flagpole on. We might not need this! May be useful for moveable bases on tanks/etc???
void Spawn_Flag_Base ( vec3_t origin )
{// Blue Flag...
	gentity_t* ent = G_Spawn();

	VectorCopy(origin, ent->s.origin);
	G_SetOrigin( ent, origin );

	ent->model = "models/map_objects/mp/flag_base.md3";
	ent->s.modelindex = G_ModelIndex( ent->model );
	ent->targetname = NULL;
	ent->classname = "flag_base";
	ent->s.eType = ET_GENERAL;
	ent->setTime = 0;
	
//	trap_R_ModelBounds(trap_R_RegisterModel( "models/map_objects/mp/flag_base.md3" ), ent->r.mins, ent->r.maxs);

	VectorSet( ent->r.mins, -16, -16, 0 );
	VectorSet( ent->r.maxs, 16, 16, 16 );

	//Drop to floor
	if ( 1 )
	{
		trace_t		tr;
		vec3_t		bottom, saveOrg;

		VectorCopy( ent->s.origin, saveOrg );
		VectorCopy( ent->s.origin, bottom );
		bottom[2] = MIN_WORLD_COORD;
		trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, bottom, ent->s.number, MASK_NPCSOLID );
		if ( !tr.allsolid && !tr.startsolid && tr.fraction < 1.0 )
		{
			VectorCopy(tr.endpos, ent->s.origin);
			G_SetOrigin( ent, tr.endpos );
		}
	}

	ent->r.contents = CONTENTS_SOLID|CONTENTS_OPAQUE|CONTENTS_BODY|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP;//CONTENTS_SOLID;

	trap_LinkEntity (ent);
}

//===========================================================================
// Routine      : flag_AI_Think_Axis
// Description  : Main think function for flags.
void PreCalculate_Flag_Spawnpoints( int flagnum, vec3_t angles, vec3_t origin );

void Supremacy_Flag_Think( gentity_t *ent )
{
	vec3_t mins;
	vec3_t maxs;
	int touch[MAX_GENTITIES];
	int num = 0;
	int i = 0;
	int radius = ent->s.otherEntityNum2;

	if (!ent->alt_fire)
	{// Have we initialized spawnpoints yet?
		//if (level.numConnectedClients > 0)
		{
			ent->alt_fire = qtrue;
			PreCalculate_Flag_Spawnpoints( ent->count, ent->s.angles, ent->s.origin );
		}
	}

	if (ent->nextthink > level.time)
		return;

	mins[0] = 0-radius;
	mins[1] = 0-radius;
	mins[2] = 0-(radius*0.5);

	maxs[0] = radius;
	maxs[1] = radius;
	maxs[2] = radius*0.5;

	VectorAdd( mins, ent->s.origin, mins );
	VectorAdd( maxs, ent->s.origin, maxs );

	// Run the think... Look for captures...
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for ( i=0 ; i<num ; i++ ) 
	{
		gentity_t *hit = &g_entities[touch[i]];

		if (!hit)
			continue;

		if (!hit->client)
			continue;

		if (!hit->health || hit->health <= 0)
			continue;

		if (hit->classname == "NPC_Vehicle")
			continue;

		if (hit->s.eType == ET_NPC)
			hit->s.teamowner = hit->client->playerTeam;

		if (ent->s.teamowner == TEAM_RED)
		{// Red flag...
			if ((hit->s.eType == ET_PLAYER || hit->s.eType == ET_NPC) && hit->s.teamowner == TEAM_RED)
			{// Blue player consolidating a blue flag for spawns... 
				//G_Printf("Consolidating flag!\n");

				ent->s.time2++;
				hit->client->ps.stats[STAT_CAPTURE_ENTITYNUM] = ent->s.number;

				if (ent->s.time2 >= 100)
				{// Finished consolidating.. Set new team to blue and initialize...
					ent->s.teamowner = TEAM_RED;
					ent->s.time2 = 100;
					ent->s.genericenemyindex = TEAM_NONE;
				}
			}
			else if ((hit->s.eType == ET_PLAYER || hit->s.eType == ET_NPC) && hit->s.teamowner == TEAM_BLUE)
			{// Blue player undoing red flag...
				//G_Printf("Uncapturing flag!\n");

				ent->s.time2--;
				hit->client->ps.stats[STAT_CAPTURE_ENTITYNUM] = ent->s.number;

				if (ent->s.time2 <= 0)
				{// Finished capture.. Set new team to blue...
					ent->s.teamowner = TEAM_NONE;
					ent->s.time2 = 0;
					ent->s.genericenemyindex = TEAM_NONE;
				}
			}
		}
		else if (ent->s.teamowner == TEAM_BLUE)
		{// Blue flag...
			if ((hit->s.eType == ET_PLAYER || hit->s.eType == ET_NPC) && hit->s.teamowner == TEAM_BLUE)
			{// Blue player consolidating a blue flag for spawns... 
				//G_Printf("Consolidating flag!\n");

				ent->s.time2++;
				hit->client->ps.stats[STAT_CAPTURE_ENTITYNUM] = ent->s.number;

				if (ent->s.time2 >= 100)
				{// Finished consolidating.. Set new team to blue and initialize...
					ent->s.teamowner = TEAM_BLUE;
					ent->s.time2 = 100;
					ent->s.genericenemyindex = TEAM_NONE;
				}
			}
			else if ((hit->s.eType == ET_PLAYER || hit->s.eType == ET_NPC) && hit->s.teamowner == TEAM_RED)
			{// Red player undoing blue flag...
				//G_Printf("Uncapturing flag!\n");

				ent->s.time2--;
				hit->client->ps.stats[STAT_CAPTURE_ENTITYNUM] = ent->s.number;

				if (ent->s.time2 <= 0)
				{// Finished capture.. Set new team to blue...
					ent->s.teamowner = TEAM_NONE;
					ent->s.time2 = 0;
					ent->s.genericenemyindex = TEAM_NONE;
				}
			}
		}
		else
		{// Neutral flag...
			if (!ent->s.genericenemyindex || ent->s.genericenemyindex == TEAM_NONE)
			{// Start capture... Set capturing team number...
				ent->s.genericenemyindex = hit->s.teamowner;
				ent->s.time2 = 0;
				hit->client->ps.stats[STAT_CAPTURE_ENTITYNUM] = ent->s.number;
			}
			else if (ent->s.genericenemyindex == hit->s.teamowner)
			{// Continuing partial capture...
				ent->s.time2++;
				hit->client->ps.stats[STAT_CAPTURE_ENTITYNUM] = ent->s.number;

				// Set the current capturing team number of a neutral flag..
				ent->s.genericenemyindex = hit->s.teamowner;

				//G_Printf("Capturing flag!\n");

				if ((hit->s.eType == ET_PLAYER || hit->s.eType == ET_NPC) 
					&& hit->s.teamowner == TEAM_BLUE && ent->s.time2 >= 50)
				{// Blue player capturing neutral flag... Finished capture.. Set new team to blue...
					ent->s.teamowner = TEAM_BLUE;
					ent->s.time2 = 0;
					ent->s.genericenemyindex = TEAM_NONE;
				}
				else if ((hit->s.eType == ET_PLAYER || hit->s.eType == ET_NPC) 
					&& hit->s.teamowner == TEAM_RED && ent->s.time2 >= 50)
				{// Red player capturing neutral flag... Finished capture.. Set new team to red...
					ent->s.teamowner = TEAM_RED;
					ent->s.time2 = 0;
					ent->s.genericenemyindex = TEAM_NONE;
				}
			}
			else if (ent->s.genericenemyindex != hit->s.teamowner)
			{// Undoing partial capture...
				ent->s.time2--;
				hit->client->ps.stats[STAT_CAPTURE_ENTITYNUM] = ent->s.number;

				//G_Printf("Undoing owned flag!\n");

				if (ent->s.time2 <= 0)
				{// Initialize back to untouched flag...
					ent->s.teamowner = TEAM_NONE;
					ent->s.time2 = 0;
					ent->s.genericenemyindex = TEAM_NONE;
				}
			}
		}
	}

	// Set next think...
	ent->nextthink = level.time + 500;
}

qboolean spots_filled[1024];

void AddFlag_Spawn ( int flagnum, vec3_t origin, vec3_t angles )
{// Add a new spawn to the list for this flag...
	if (flag_list[flagnum].num_spawnpoints < 32)
	{// Create a new spawn point.
		//G_Printf("A new spawn point has been extrapolated. ^1(^5#^7%i^1)^5\n", SP_Spawns);
		VectorCopy( origin, flag_list[flagnum].spawnpoints[flag_list[flagnum].num_spawnpoints] ); // Copy this npc's location to the end of the list.
		VectorCopy( angles, flag_list[flagnum].spawnangles[flag_list[flagnum].num_spawnpoints] ); // Copy this npc's location to the end of the list.
		flag_list[flagnum].num_spawnpoints++;
	}
	else
	{
		if (!spots_filled[flagnum])
		{// Finished making spawns for this flag...
			//G_Printf("Calculated 32 spawns for flag #%i.\n", flagnum);
			spots_filled[flagnum] = qtrue;
		}
	}
}

qboolean TooCloseToOtherSpawnpoint ( int flagnum, vec3_t origin )
{
	int i = 0;

	for (i = 0;i < flag_list[flagnum].num_spawnpoints;i++)
	{
		if (VectorDistance(flag_list[flagnum].spawnpoints[i], origin) < 64)
			return qtrue;
	}

	return qfalse;
}

extern int NAV_FindClosestWaypointForPoint2( vec3_t point );
extern wpobject_t *gWPArray[MAX_WPARRAY_SIZE];
extern int GetNearestVisibleWP(vec3_t org, int ignore);

void PreCalculate_Flag_Spawnpoints( int flagnum, vec3_t angles, vec3_t origin )
{
	vec3_t fwd, point;
	int tries = 0, tries2 = 0;
	qboolean visible = qfalse;

	VectorCopy(origin, point);
		
	AngleVectors( angles, fwd, NULL, NULL );

	while (1)//visible == qfalse)
	{// In case we need to try a second spawnpoint.
		int wp = -1;
		vec3_t	playerMins = {-15, -15, DEFAULT_MINS_2};
		vec3_t	playerMaxs = {15, 15, DEFAULT_MAXS_2};

		playerMins[0] = -15;
		playerMins[1] = -15;
		playerMins[2] = -1;
		playerMaxs[0] = 15;
		playerMaxs[1] = 15;
		playerMaxs[2] = 96;//1;

		while (tries < 16)
		{
			tries++;
			tries2 = 0;

			while (tries2 < 64)
			{
				int num_tries; // For secondary spawns. (Behind point).

				tries2++;

				num_tries = tries2;
				if (tries2 <= 16)
				{
				}
				else if (tries2 <= 32)
				{
					num_tries-=16;
				}
				else if (tries2 <= 48)
				{
					num_tries-=32;
				}
				else
				{
					num_tries-=48;
				}
				
				if (wp == -1)
					VectorCopy(origin, point);
				else
					VectorCopy(gWPArray[wp]->origin, point);

				if (tries2 <= 8)
				{
					point[0] += 1+(tries*64);
					point[1] += 1+(num_tries*64);
				}
				else if (tries2 <= 16)
				{
					point[0] += 1+(tries*64);
					point[1] += 1-(num_tries*64);
				}
				else if (tries2 <= 24)
				{
					point[0] += 1-(tries*64);
					point[1] += 1+(num_tries*64);
				}
				else
				{
					point[0] -= 1+(tries*64);
					point[1] -= 1+(num_tries*64);
				}

				//if (CheckAboveOK_Player(point))
				//	point[2] += 32;
				//else
				//	continue;

				point[2] += 16;

				if (wp == -1)
				{
					if (OrgVisibleBox(origin, playerMins, playerMaxs, point, flag_list[flagnum].flagentity->s.number)
						&& CheckBelowOK(point)
						&& !CheckEntitiesInSpot(point) 
						&& VectorDistance(point, origin) > 128
						&& !TooCloseToOtherSpawnpoint(flagnum, point))
					{
						//G_Printf("Adding spawn at %f %f %f.\n", point[0], point[1], point[2]);
						AddFlag_Spawn(flagnum, point, angles);
						//G_Printf("Adding spawn %f %f %f\n", point[0], point[1], point[2]);
						visible = qtrue;
					}
					//else
					//	G_Printf("Not adding spawn at %f %f %f.\n", point[0], point[1], point[2]);
				}
				else
				{
					if (OrgVisibleBox(gWPArray[wp]->origin, playerMins, playerMaxs, point, -1)
						&& CheckBelowOK(point)
						&& !CheckEntitiesInSpot(point) 
						&& VectorDistance(point, origin) > 128
						&& !TooCloseToOtherSpawnpoint(flagnum, point))
					{
						//G_Printf("Adding spawn at %f %f %f.\n", point[0], point[1], point[2]);
						AddFlag_Spawn(flagnum, point, angles);
						//G_Printf("Adding wp spawn %f %f %f\n", point[0], point[1], point[2]);
						visible = qtrue;
					}
				}
			}
		}

		if (visible != qfalse)
			break;
		else
		{
			//if (wp == -1)
				wp = GetNearestVisibleWP(flag_list[flagnum].flagentity->s.origin, flag_list[flagnum].flagentity->s.number);//NAV_FindClosestWaypointForPoint2( flag_list[flagnum].flagentity->s.origin );
			//else
			//	wp = GetNearestVisibleWP(flag_list[flagnum].flagentity->s.origin, flag_list[flagnum].flagentity->s.number);//NAV_FindClosestWaypointForPoint( flag_list[flagnum].flagentity, flag_list[flagnum].flagentity->s.origin );
		}
	}

	G_Printf("^3*** ^3DominancE^5: Added ^7%i^5 spawnpoints at flag #^7%i^5.\n", flag_list[flagnum].num_spawnpoints, flagnum);
}

int number_of_flags = 0; // Automatically number the flags if not specified by map makers...

void SP_Spawn_Supremacy_Flag ( gentity_t* ent )
{// Spawn Supremacy flag from map entity
	int radius = 0, teamowner = TEAM_NONE;
	int flagnum = -1;
	char *model, *model2, *fullname;

	G_SpawnString("model", "models/flags/r_flag.md3", &model); // Custom red model..
	G_SpawnString("model2", "models/flags/b_flag.md3", &model2); // Custom blue model..
	G_SpawnString("fullname", "models/flags/n_flag.md3", &fullname); // Custom neutral model.
	G_SpawnInt("radius", "500", &radius); // Point's capture radius..
	G_SpawnInt("teamowner", "0", &teamowner); // Point's capture radius..
	G_SpawnInt("teamuser", "-1", &flagnum); // Map's flag number.. For linking things to this flag based on team...

	if (flagnum < 0)
	{// We dont have a flag number.. Generate them for this map...
		flagnum = number_of_flags;
	}

	ent->s.otherEntityNum2 = radius; // Set capture radius...
	ent->s.otherEntityNum = flagnum; // Flag number...

	if (model != "0")
	{// Red flag model..
		ent->model = model;
		//RegisterAsModel(model);
		ent->s.modelindex = G_ModelIndex( ent->model );
	}
	else
		ent->s.modelindex = -1;

	if (model2 != "0")
	{// Blue flag model..
		ent->model2 = model2;
		//RegisterAsModel(model2);
		ent->s.modelindex2 = G_ModelIndex( ent->model2 );
	}
	else
		ent->s.modelindex2 = -1;

	if (fullname != "0")
	{// Neutral flag model..
		ent->fullName = fullname;
		//RegisterAsModel(fullname);
		ent->s.activeForcePass = G_ModelIndex( ent->fullName );
	}
	else
		ent->s.activeForcePass = -1;

	VectorSet( ent->r.mins, -8, -8, 0 );
	VectorSet( ent->r.maxs, 8, 8, 96 );

	ent->s.origin[2]+=64;
	G_SetOrigin( ent, ent->s.origin );

	if (model == "models/flags/r_flag.md3")
		Spawn_Flag_Base(ent->s.origin);

	//Drop to floor
	if ( 1 )
	{
		trace_t		tr;
		vec3_t		bottom, saveOrg;

		VectorCopy( ent->s.origin, saveOrg );
		VectorCopy( ent->s.origin, bottom );
		bottom[2] = MIN_WORLD_COORD;
		trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, bottom, ent->s.number, MASK_NPCSOLID );
		if ( !tr.allsolid && !tr.startsolid && tr.fraction < 1.0 )
		{
			VectorCopy(tr.endpos, ent->s.origin);
			G_SetOrigin( ent, tr.endpos );
		}
	}

	ent->s.teamowner = teamowner;

	if (teamowner == TEAM_RED)
	{
		ent->s.time2 = 100;
		G_Printf("^4*** ^5Flag at ^7%f %f %f^5 is ^1TEAM_RED^5! Radius is ^3%i^5.\n", ent->s.origin[0], ent->s.origin[1], ent->s.origin[2], radius);
	}
	else if (teamowner == TEAM_BLUE)
	{
		ent->s.time2 = 100;
		G_Printf("^4*** ^5Flag at ^7%f %f %f^5 is ^4TEAM_BLUE^5! Radius is ^3%i^5.\n", ent->s.origin[0], ent->s.origin[1], ent->s.origin[2], radius);
	}
	else
	{
		ent->s.time2 = 0;
		G_Printf("^4*** ^5Flag at ^7%f %f %f^5 is ^6TEAM_NEUTRAL^5! Radius is ^3%i^5.\n", ent->s.origin[0], ent->s.origin[1], ent->s.origin[2], radius);
	}

	ent->targetname = NULL;
	ent->classname = "flag";
	ent->s.eType = ET_FLAG;
	ent->think = Supremacy_Flag_Think;

	VectorSet( ent->r.mins, -8, -8, 0 );
	VectorSet( ent->r.maxs, 8, 8, 96 );
//	trap_R_ModelBounds(trap_R_RegisterModel( "models/flags/r_flag.md3" ), ent->r.mins, ent->r.maxs);

	ent->r.contents = CONTENTS_SOLID|CONTENTS_OPAQUE|CONTENTS_BODY|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP;//CONTENTS_SOLID;

	ent->nextthink = level.time + 1;

	//ent->s.eFlags |= EF_RADAROBJECT;

	trap_LinkEntity (ent);

	flag_list[flagnum].flagentity = ent;
	flag_list[flagnum].num_spawnpoints = 0;
	ent->count = flagnum;
	
	//PreCalculate_Flag_Spawnpoints( flagnum, ent->s.angles, ent->s.origin );

	number_of_flags++;
}

void SP_misc_control_point (gentity_t* ent)
{
	if (g_gametype.integer == GT_SUPREMACY)
	{
		SP_Spawn_Supremacy_Flag(ent);
	}
}

void Spawn_Scenario_Flag_Auto ( vec3_t origin, int teamowner )
{// Red Flag...
	gentity_t* ent = G_Spawn();
	int radius = 0;//, teamowner = TEAM_NONE;
	int flagnum = 0;
	char *model, *model2, *fullname;

	G_SpawnString("model", "models/flags/r_flag.md3", &model); // Custom red model..
	G_SpawnString("model2", "models/flags/b_flag.md3", &model2); // Custom blue model..
	G_SpawnString("fullname", "models/flags/n_flag.md3", &fullname); // Custom neutral model.
	G_SpawnInt("radius", "500", &radius); // Point's capture radius..
	//G_SpawnInt("teamowner", "0", &teamowner); // Point's capture radius..
	G_SpawnInt("teamuser", "-1", &flagnum); // Map's flag number.. For linking things to this flag based on team...

	if (!radius)
	{// Default radius...
		ent->s.otherEntityNum2 = 500; // Set capture radius...
	}
	else
	{
		ent->s.otherEntityNum2 = radius; // Set capture radius...
	}

	if (flagnum < 0)
	{// We dont have a flag number.. Generate them for this map...
		flagnum = number_of_flags;
		//number_of_flags++;
	}

	ent->s.otherEntityNum = flagnum; // Flag number...

//	if (model != "")
//	{// Red flag model..
		ent->model = model;
		//RegisterAsModel(model);
		ent->s.modelindex = G_ModelIndex( ent->model );
//	}
//	else
//		ent->s.modelindex = -1;

//	if (model2 != "")
//	{// Blue flag model..
		ent->model2 = model2;
		//RegisterAsModel(model2);
		ent->s.modelindex2 = G_ModelIndex( ent->model2 );
//	}
//	else
//		ent->s.modelindex2 = -1;

//	if (fullname != "")
//	{// Neutral flag model..
		ent->fullName = fullname;
		//RegisterAsModel(fullname);
		ent->s.activeForcePass = G_ModelIndex( ent->fullName );
//	}
//	else
//		ent->s.activeForcePass = -1;

	VectorSet( ent->r.mins, -8, -8, 0 );
	VectorSet( ent->r.maxs, 8, 8, 96 );

	VectorCopy(origin, ent->s.origin);
	ent->s.origin[2]+=64;
	G_SetOrigin( ent, ent->s.origin );

	if (model == "models/flags/r_flag.md3")
		Spawn_Flag_Base(ent->s.origin);

	//Drop to floor
	if ( 1 )
	{
		trace_t		tr;
		vec3_t		bottom, saveOrg;

		VectorCopy( ent->s.origin, saveOrg );
		VectorCopy( ent->s.origin, bottom );
		bottom[2] = MIN_WORLD_COORD;
		trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, bottom, ent->s.number, MASK_NPCSOLID );
		if ( !tr.allsolid && !tr.startsolid && tr.fraction < 1.0 )
		{
			VectorCopy(tr.endpos, ent->s.origin);
			G_SetOrigin( ent, tr.endpos );
		}
	}

	ent->s.teamowner = teamowner;

	if (teamowner == TEAM_RED)
	{
		ent->s.time2 = 100;
		G_Printf("^4*** ^5Flag at ^7%f %f %f^5 is ^1TEAM_RED^5! Radius is ^3%i^5.\n", ent->s.origin[0], ent->s.origin[1], ent->s.origin[2], radius);
	}
	else if (teamowner == TEAM_BLUE)
	{
		ent->s.time2 = 100;
		G_Printf("^4*** ^5Flag at ^7%f %f %f^5 is ^4TEAM_BLUE^5! Radius is ^3%i^5.\n", ent->s.origin[0], ent->s.origin[1], ent->s.origin[2], radius);
	}
	else
	{
		ent->s.time2 = 0;
		G_Printf("^4*** ^5Flag at ^7%f %f %f^5 is ^6TEAM_NEUTRAL^5! Radius is ^3%i^5.\n", ent->s.origin[0], ent->s.origin[1], ent->s.origin[2], radius);
	}

	ent->targetname = NULL;
	ent->classname = "flag";
	ent->s.eType = ET_FLAG;
	ent->think = Supremacy_Flag_Think;

//	trap_R_ModelBounds(trap_R_RegisterModel( "models/flags/r_flag.md3" ), ent->r.mins, ent->r.maxs);

	ent->r.contents = CONTENTS_SOLID|CONTENTS_OPAQUE|CONTENTS_BODY|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP;//CONTENTS_SOLID;

	ent->nextthink = level.time + 1;

	//ent->s.eFlags |= EF_RADAROBJECT;

	trap_LinkEntity (ent);

	flag_list[flagnum].flagentity = ent;
	flag_list[flagnum].num_spawnpoints = 0;

	//PreCalculate_Flag_Spawnpoints( flagnum, ent->s.angles, ent->s.origin );
	ent->count = flagnum;
	
	//PreCalculate_Flag_Spawnpoints( flagnum, ent->s.angles, ent->s.origin );

	number_of_flags++;
}

#endif //__DOMINANCE__
