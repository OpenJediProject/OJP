/*
¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤
Unique1's Experimental OM SpawnSys Code.
¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤
OrgVisibleBox
CheckAboveOK_Player
CheckBelowOK
CheckEntitiesInSpot
Extrapolate_Advanced_Spawnpoint
¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤¤
*/

#ifdef __DOMINANCE__

#include "g_local.h"

extern float VectorDistance(vec3_t v1, vec3_t v2);
extern int OrgVisibleBox(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, int ignore);

qboolean CheckAboveOK_Player(vec3_t origin) // For player/npc/bot spawns!
{// Check directly above a point for clearance.
	trace_t tr;
	vec3_t up, mins, maxs;

	mins[0] = -16;
	mins[1] = -16;
	mins[2] = -16;
	maxs[0] = 16;
	maxs[1] = 16;
	maxs[2] = 16;

	VectorCopy(origin, up);

	up[2] += 4096;

	trap_Trace(&tr, origin, mins, maxs, up, ENTITYNUM_NONE, MASK_SOLID); // Look for ground.

	VectorSubtract(origin, tr.endpos, up);

	if (up[2] <= 96
		&& tr.fraction == 1)
		return qfalse; // No room above!
	else
		return qtrue; // All is ok!
}

qboolean CheckBelowOK(vec3_t origin)
{// Check directly below us.
	trace_t tr;
	vec3_t down, mins, maxs;

	mins[0] = -16;
	mins[1] = -16;
	mins[2] = -16;
	maxs[0] = 16;
	maxs[1] = 16;
	maxs[2] = 16;

	VectorCopy(origin, down);

	down[2] -= 1024;

	trap_Trace(&tr, origin, mins, maxs, down, ENTITYNUM_NONE, MASK_SOLID); // Look for ground.

	VectorSubtract(origin, tr.endpos, down);

	//if (down[2] >= 500)
	if (down[2] >= 128 || !tr.fraction)
		return qfalse; // Long way down!
	else
		return qtrue; // All is ok!
}

qboolean AdvancedWouldTelefrag(vec3_t point)
{
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	vec3_t		mins, maxs;
	vec3_t	playerMins = {-15, -15, DEFAULT_MINS_2};
	vec3_t	playerMaxs = {15, 15, DEFAULT_MAXS_2};


	if (!point)
		return qtrue;

	VectorAdd( point, playerMins, mins );
	VectorAdd( point, playerMaxs, maxs );

	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) 
	{
		hit = &g_entities[touch[i]];

		if ( hit->client ) 
		{
			return qtrue;
		}

		if (hit->r.contents)
		{
			if (hit->r.contents&MASK_SOLID)
				return qtrue;
		}
	}

	return qfalse;
}

qboolean CheckEntitiesInSpot(vec3_t point)
{// Any entities too close to us?
	int entitynum = 0;
	qboolean foundbad = qfalse;

	while (entitynum < MAX_CLIENTS)
	{// Find a clear point.
		gentity_t *ent = &g_entities[entitynum];

		if (!ent)
			continue;

		//if (ent->s.solid != SOLID_BMODEL && !(ent->r.contents&CONTENTS_MONSTERCLIP) && !(ent->r.contents&CONTENTS_BOTCLIP) && !(ent->r.contents&CONTENTS_SOLID))
		//	continue;

		if (VectorDistance(point, ent->r.currentOrigin) < 128 || VectorDistance(point, ent->s.origin) < 128)
		{// Bad point.
			return qtrue;
		}
		
		if (entitynum < MAX_CLIENTS 
			&& ent->client 
			&& ent->client->ps.origin
			&& VectorDistance(point, ent->client->ps.origin) < 128)
		{// Bad point.
			return qtrue;
		}

		// Bad point?
		entitynum++;
	}

	return foundbad;
}

qboolean Extrapolate_Advanced_Spawnpoint( vec3_t original_origin, vec3_t newspawn )
{// Will output newspawn for spawnpoint extrapolation from a single point... (Eg: Up to 64 players(guess) or so per origin).
	vec3_t point;
	qboolean visible = qfalse;
	int tries = 0, tries2 = 0;

	VectorCopy(original_origin, point);
		
	while (visible == qfalse && tries < 8)
	{
		vec3_t	playerMins = {-18, -18, -24};
		vec3_t	playerMaxs = {18, 18, 24};

		tries++;
		tries2 = 0;

		while (visible == qfalse && tries2 < 16)
		{
			int num_tries; // For secondary spawns. (Behind point).

			tries2++;

			num_tries = tries2;
				
			if (num_tries > 8)// Secondary Set.
				num_tries -= 8;

			VectorCopy(original_origin, point);

			if (num_tries <= 4)
			{
				point[0] += (64*8)-(tries*64);
				point[1] += (64*8)-(num_tries*64);
			}
			else if (num_tries <= 8)
			{
				point[0] += (64*8)-(tries*64);
				point[1] -= (64*8)-(num_tries*64);
			}
			else if (num_tries <= 12)
			{
				point[0] -= (64*8)-(tries*64);
				point[1] += (64*8)-(num_tries*64);
			}
			else
			{
				point[0] -= (64*8)-(tries*64);
				point[1] -= (64*8)-(num_tries*64);
			}

			if (!CheckAboveOK_Player(point))
				continue;

			if (OrgVisibleBox(original_origin, playerMins, playerMaxs, point, -1)
				&& CheckBelowOK(point)
				&& !CheckEntitiesInSpot(point) )
			{
				visible = qtrue;
				break;
			}
		}
	}

	if (!visible)
	{// Try another spawnpoint.
		return qfalse;
	}

	VectorCopy(point, newspawn);

	return qtrue;
}

#endif //__DOMINANCE__

