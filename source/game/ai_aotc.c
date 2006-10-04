//[AotCAI]

//includes
#include "g_local.h"
#include "ai_main.h"
#include "bg_local.h"
#include "w_saber.h"

int AOTC_BotFallbackNavigation(bot_state_t *bs);

//externs
extern vmCvar_t bot_getinthecarrr;
extern bot_state_t	*botstates[MAX_CLIENTS];
extern int AltFiring(bot_state_t *bs);
extern void BotAimLeading(bot_state_t *bs, vec3_t headlevel, float leadAmount);
extern void BotAimOffsetGoalAngles(bot_state_t *bs);
extern void BotCheckDetPacks(bot_state_t *bs);
extern void BotDeathNotify(bot_state_t *bs);
extern int BotGetWeaponRange(bot_state_t *bs);
extern void BotScanForLeader(bot_state_t *bs);
extern int BotSelectChoiceWeapon(bot_state_t *bs, int weapon, int doselection);
extern int BotSurfaceNear(bot_state_t *bs);
extern int BotTrace_Duck(bot_state_t *bs, vec3_t traceto);
extern int BotTrace_Jump(bot_state_t *bs, vec3_t traceto);
extern int BotWeaponBlockable(int weapon);
extern float BotWeaponCanLead(bot_state_t *bs);
extern float VectorDistance(vec3_t v1, vec3_t v2);
extern gentity_t *CheckForFriendInLOF(bot_state_t *bs);
extern int CheckForFunc(vec3_t org, int ignore);
extern int CombatBotAI(bot_state_t *bs, float thinktime);
extern void CTFFlagMovement(bot_state_t *bs);
extern int EntityVisibleBox(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, int ignore, int ignore2);
extern void GetIdealDestination(bot_state_t *bs);
extern int KeepAltFromFiring(bot_state_t *bs);
extern int KeepPrimFromFiring(bot_state_t *bs);
extern void MeleeCombatHandling(bot_state_t *bs);
extern void MoveTowardIdealAngles(bot_state_t *bs);
extern void NPC_SetAnim(gentity_t *ent, int setAnimParts, int anim, int setAnimFlags);
extern int PassLovedOneCheck(bot_state_t *bs, gentity_t *ent);
extern int PassStandardEnemyChecks(bot_state_t *bs, gentity_t *en);
extern int PassWayCheck(bot_state_t *bs, int windex);
extern int PrimFiring(bot_state_t *bs);
extern void SaberCombatHandling(bot_state_t *bs);
extern int ScanForEnemies(bot_state_t *bs);
extern void StrafeTracing(bot_state_t *bs);
extern float TotalTrailDistance(int start, int end, bot_state_t *bs);
extern int WaitingForNow(bot_state_t *bs, vec3_t goalpos);
extern void WPConstantRoutine(bot_state_t *bs);
extern int WPOrgVisible(gentity_t *bot, vec3_t org1, vec3_t org2, int ignore);
extern void WPTouchRoutine(bot_state_t *bs);
extern int BotCanHear(bot_state_t *bs, gentity_t *en, float endist);
extern int BotMindTricked(int botClient, int enemyClient);
extern qboolean BotPVSCheck( const vec3_t p1, const vec3_t p2 );
extern void BotChangeViewAngles(bot_state_t *bs, float thinktime);
extern void BotDoTeamplayAI(bot_state_t *bs);
extern void BotReplyGreetings(bot_state_t *bs);
extern int BotSelectIdealWeapon(bot_state_t *bs);
extern int BotTryAnotherWeapon(bot_state_t *bs);
extern int BotUseInventoryItem(bot_state_t *bs);
extern void CommanderBotAI(bot_state_t *bs);

extern vmCvar_t mod_classes;
extern vmCvar_t bot_cpu_usage;

//defines
#define	APEX_HEIGHT		200.0f

//AotC data stuff
qboolean bot_will_fall[MAX_CLIENTS];
vec3_t jumpPos[MAX_CLIENTS];
int	next_bot_fallcheck[MAX_CLIENTS];
int next_bot_vischeck[MAX_CLIENTS];
int last_bot_wp_vischeck_result[MAX_CLIENTS];
vec3_t safePos[MAX_CLIENTS]; // Safe return from jump positions.
int last_checkfall[MAX_GENTITIES];


qboolean CheckFall_By_Vectors(vec3_t origin, vec3_t angles, gentity_t *ent)
{// Check a little in front of us.
	trace_t tr;
	vec3_t down, flatAng, fwd, use;

	if (last_checkfall[ent->s.number] > level.time - 250) // Only check every 1/4 sec.
		return qtrue;//lastresult[ent->s.number]; // To speed things up.

	last_checkfall[ent->s.number] = level.time;
	
	VectorCopy(angles, flatAng);
	flatAng[PITCH] = 0;

	AngleVectors(flatAng, fwd, 0, 0);

	use[0] = origin[0] + fwd[0]*96;
	use[1] = origin[1] + fwd[1]*96;
	use[2] = origin[2] + fwd[2]*96;

	VectorCopy(use, down);

	down[2] -= 4096;

	trap_Trace(&tr, use, ent->r.mins, ent->r.maxs, down, ent->s.clientNum, MASK_SOLID); // Look for ground.

	VectorSubtract(use, tr.endpos, down);

	//if (VectorLength(down) >= 1500)
	//if (down[2] >= 500)
	if (down[2] >= 1000)
		return qtrue; // Long way down!

	use[0] = origin[0] + fwd[0]*64;
	use[1] = origin[1] + fwd[1]*64;
	use[2] = origin[2] + fwd[2]*64;

	VectorCopy(use, down);

	down[2] -= 4096;

	trap_Trace(&tr, use, ent->r.mins, ent->r.maxs, down, ent->s.clientNum, MASK_SOLID); // Look for ground.

	VectorSubtract(use, tr.endpos, down);

	//if (VectorLength(down) >= 1500)
	//if (down[2] >= 500)
	if (down[2] >= 1000)
		return qtrue; // Long way down!


	use[0] = origin[0] + fwd[0]*32;
	use[1] = origin[1] + fwd[1]*32;
	use[2] = origin[2] + fwd[2]*32;

	VectorCopy(use, down);

	down[2] -= 4096;

	trap_Trace(&tr, use, ent->r.mins, ent->r.maxs, down, ent->s.clientNum, MASK_SOLID); // Look for ground.

	VectorSubtract(use, tr.endpos, down);

	//if (VectorLength(down) >= 1500)
	//if (down[2] >= 500)
	if (down[2] >= 1000)
		return qtrue; // Long way down!

	return qfalse; // All is ok!
}


void Set_Enemy_Path ( bot_state_t *bs )
{// For aborted jumps.
	vec3_t fwd, b_angle;

	bs->goalAngles[PITCH] = 0;
	bs->goalAngles[ROLL] = 0;

	VectorCopy(bs->goalAngles, b_angle);
	
	AngleVectors(b_angle, fwd, NULL, NULL);

	if (!bs->currentEnemy // No enemy!
		|| CheckFall_By_Vectors(bs->origin, fwd, &g_entities[bs->entitynum]) == qtrue) // We're gonna fall!!!
	{
		if (bs->wpCurrent)
		{// Fall back to the old waypoint info... Should be already set.		
		
		}
		else
		{
			bs->beStill = level.time + 1000;
		}
	}
	else
	{// Set enemy as our goal.
		VectorCopy(bs->currentEnemy->r.currentOrigin, bs->goalPosition);
	}
}


void AIMod_Jump ( bot_state_t *bs )
{
	vec3_t		dir, p1, p2, apex;
	float		time, height, forward, z, xy, dist, apexHeight;
//	float heightmod;
	gentity_t *bot = &g_entities[bs->entitynum];
	gentity_t *target;

	if (!bs->currentEnemy)
	{
		bs->BOTjumpState = JS_WAITING;
		return;
	}

	target = &g_entities[bs->currentEnemy->client->ps.clientNum]; // We NEED a valid enemy!

	if( !target )
	{//Should have task completed the navgoal
//		G_Printf("No target!\n");
		bs->BOTjumpState = JS_WAITING;
		//Set_Enemy_Path(bs);
		return;
	}

	//We don't really care about pitch here
	switch ( bs->BOTjumpState )
	{
	case JS_FACING:
/*			NPC_SetAnim(bot, SETANIM_LEGS, BOTH_CROUCH1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			//G_SetAnim(NPC, NULL, SETANIM_BOTH, BOTH_STAND1TO2, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD|SETANIM_FLAG_HOLDLESS, 0);
			BOTjumpState[bs->cur_ps.clientNum] = JS_CROUCHING;
		break;*/
	case JS_CROUCHING:
/*		if ( bot->client->ps.legsTimer > 0 )
		{//Still playing crouching anim
			return;
		}*/

		/*
		if (mod_classes.integer == 2 
			&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] == GCLASS_SOLDIER )
		{// Jetpacker.. Jetpack ON!
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING; //going up
		}*/

		//Create a parabola

		if ( bot->r.currentOrigin[2] > jumpPos[bot->s.number][2] )
		{
			VectorCopy( bot->r.currentOrigin, p1 );
			VectorCopy( jumpPos[bot->s.number], p2 );
		}
		else if ( bot->r.currentOrigin[2] < jumpPos[bot->s.number][2] )
		{
			VectorCopy( jumpPos[bot->s.number], p1 );
			VectorCopy( bot->r.currentOrigin, p2 );
		}
		else
		{
			VectorCopy( bot->r.currentOrigin, p1 );
			VectorCopy( jumpPos[bot->s.number], p2 );
		}

		//z = xy*xy
		VectorSubtract( p2, p1, dir );
		dir[2] = 0;

		//Get xy and z diffs
		xy = VectorNormalize( dir );// + (VectorNormalize( dir )*0.0002);
/*
		// Check Jump
		G_Printf("XY is %f", xy);
		heightmod = 1.0;
		if (xy >= 1200)
		{
			xy += xy*0.7;
			heightmod = 1.7;
			G_Printf(" - XY changed to %f - Heightmod set to %f.\n", xy, heightmod);
		}
		else if (xy >= 800)
		{
			xy += xy*0.4;
			heightmod = 1.4;
			G_Printf(" - XY changed to %f - Heightmod set to %f.\n", xy, heightmod);
		}
		else if (xy >= 400)
		{
			xy += xy*0.1;
			heightmod = 1.1;
			G_Printf(" - XY changed to %f - Heightmod set to %f.\n", xy, heightmod);
		}
		else
			G_Printf("\n");
		*/
		z = p1[2] - p2[2];

/*		G_Printf("XY is %f", xy);
		if (xy >= 1000)
		{
			xy += xy*0.7;
			apexHeight = APEX_HEIGHT;
			G_Printf(" - XY changed to %f - apexHeight set to %f.\n", xy, apexHeight);
		}
		else if (xy >= 700)
		{
			xy += xy*0.4;
			apexHeight = APEX_HEIGHT/1.3;
			G_Printf(" - XY changed to %f - apexHeight set to %f.\n", xy, apexHeight);
		}
		else if (xy >= 500)
		{
			xy += xy*0.1;
			apexHeight = APEX_HEIGHT/1.6;
			G_Printf(" - XY changed to %f - apexHeight set to %f.\n", xy, apexHeight);
		}
		else
		{
			G_Printf("\n");
			apexHeight = APEX_HEIGHT/2;
		}*/
		
		//G_Printf("XY is %f\n", xy);

		if (xy >= 600)
		{
			bs->BOTjumpState = JS_WAITING;
			//Set_Enemy_Path(bs);
			return;
		}

		apexHeight = APEX_HEIGHT/2;

		//FIXME: length of xy will change curve of parabola, need to account for this
		//somewhere... PARA_WIDTH
		z = (sqrt(apexHeight + z) - sqrt(apexHeight));

		assert(z >= 0);

//		Com_Printf("apex is %4.2f percent from p1: ", (xy-z)*0.5/xy*100.0f);

		xy -= z;
		xy *= 0.5;
		
//		assert(xy > 0);
		if (xy <= 0)
			xy = 1.0f;

		VectorMA( p1, xy, dir, apex );

		apex[2] += apexHeight;
/*		if (xy >= 1200)
		{
			apex[2] += apexHeight*0.1;
		}
		else if (xy >= 800)
		{
			apex[2] += apexHeight*0.05;
		}
		else if (xy >= 400)
		{
			apex[2] += apexHeight*0.02;
		}*/
	
		VectorCopy(apex, bot->pos1);
		
		//Now we have the apex, aim for it
		
		height = apex[2] - bot->r.currentOrigin[2];

		time = sqrt( height / ( .5 * bot->client->ps.gravity ) );
		if ( !time ) 
		{
//			Com_Printf("ERROR no time in jump\n");
			//return;
			dist = VectorDistance(jumpPos[bot->s.number], bot->r.currentOrigin);
			if (dist > 1200)
			{
				G_Printf("Too far! %i is > 1200!", dist);
				bs->BOTjumpState = JS_WAITING;
				Set_Enemy_Path(bs);
				return;
			}

			height = dist;

			time = sqrt( height / ( .5 * bot->client->ps.gravity ) );
		}

		// set s.origin2 to the push velocity
		VectorSubtract ( apex, bot->r.currentOrigin, bot->client->ps.velocity );
		bot->client->ps.velocity[2] = 0;
		dist = VectorNormalize( bot->client->ps.velocity );
		
//		G_Printf("Dist: %i\n", dist);
/*		if (dist <= 200)
		{
			dist*=1.10;
		}
		else if (dist <= 500)
		{
			dist*=1.06;
		}
		else if (dist <= 650)
		{
			dist*=1.04;
		}
		else if (dist <= 800)
		{
			dist*=1.02;
		}
		else if (dist <= 1000)
		{
			dist*=1.01;
		}
		else
		{
			dist*=1.00;
		}*/
		//dist = VectorDistance(target->r.currentOrigin, bot->r.currentOrigin);

		forward = dist / time;

		VectorScale( bot->client->ps.velocity, forward, bot->client->ps.velocity );

		bot->client->ps.velocity[2] = time * bot->client->ps.gravity;

//		Com_Printf( "%s jumping %s, gravity at %4.0f percent\n", NPC->targetname, vtos(NPC->client->ps.velocity), NPC->client->ps.gravity/8.0f );

		bot->flags |= FL_NO_KNOCKBACK;
		bs->BOTjumpState = JS_JUMPING;
		//FIXME: jumpsound?
		break;
	case JS_JUMPING:
		/*
		if (mod_classes.integer == 2 
			&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] == GCLASS_SOLDIER )
		{// Jetpacker.. Jetpack ON!
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING; //going up
		}*/

		if ( bot->r.currentOrigin[0] == jumpPos[bot->s.number][0]
			&& bot->r.currentOrigin[1] == jumpPos[bot->s.number][1])
			//&& EntityHeight(bot) <= 256 )
		{//Let's land!
			//FIXME: if the 
			bot->client->ps.velocity[0] *= 0.5;
			bot->client->ps.velocity[1] *= 0.5;
			if (bot->client->ps.velocity[2] < 0)
				bot->client->ps.velocity[2] -= -16;
			else
				bot->client->ps.velocity[2] = -16;
		}
		else if ( bot->s.groundEntityNum != ENTITYNUM_NONE)
		{//Landed, start landing anim
			//FIXME: if the 
			VectorClear(bot->client->ps.velocity);
			NPC_SetAnim(bot, SETANIM_BOTH, BOTH_LAND1, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
			bs->BOTjumpState = JS_LANDING;
			//FIXME: landsound?
		}
		else if ( bot->client->ps.legsTimer > 0 )
		{//Still playing jumping anim
			//FIXME: apply jump velocity here, a couple frames after start, not right away
			return;
		}
		else
		{//still in air, but done with jump anim, play inair anim
			NPC_SetAnim(bot, SETANIM_BOTH, BOTH_INAIR1, SETANIM_FLAG_OVERRIDE);
		}
		break;
	case JS_LANDING:
		/*
		if (mod_classes.integer == 2 
			&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] == GCLASS_SOLDIER 
			&& Q_irand(0,5) < 2 )
		{// Jetpacker Slowing.. Jetpack ON and OFF!
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING; //going up
		}*/

		if ( bot->client->ps.legsTimer > 0 )
		{//Still playing landing anim
			return;
		}
		else
		{
			bs->BOTjumpState = JS_WAITING;
			bot->client->pers.cmd.forwardmove = 0;
			bot->flags &= ~FL_NO_KNOCKBACK;
		}
		break;
	case JS_WAITING:
		/*
		if (mod_classes.integer == 2 
			&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] == GCLASS_SOLDIER )
		{// Jetpacker.. Jetpack ON!
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING; //going up
		}*/

		//Set_Enemy_Path(bs);
		break;
	default:
		/*
		if (mod_classes.integer == 2 
			&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] == GCLASS_SOLDIER )
		{// Jetpacker.. Jetpack ON!
			bs->cur_ps.eFlags |= EF_JETPACK_ACTIVE;
			bs->cur_ps.eFlags |= EF_JETPACK_FLAMING; //going up
		}*/

		bs->BOTjumpState = JS_FACING;
		break;
	}
}

int next_point[MAX_CLIENTS];

// AIMod Fallback Navigation.
int AOTC_BotFallbackNavigation(bot_state_t *bs)
{
	vec3_t b_angle, fwd, trto, mins, maxs;
	trace_t tr;
	vec3_t a, ang;

	if (gWPArray && gWPArray[0])
		return 2;

	if (bs->currentEnemy && bs->frame_Enemy_Vis)
	{
		return 2; //we're busy
	}

	if (bs->cur_ps.weapon != WP_SABER 
		&& bs->currentEnemy && bs->frame_Enemy_Vis)
	{// If we are looking at enemy and have line of sight, don't move. Stand and shoot.
		return 2; //we're busy
	}

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = 0;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	bs->goalAngles[PITCH] = 0;
	bs->goalAngles[ROLL] = 0;

	VectorCopy(bs->goalAngles, b_angle);

	AngleVectors(b_angle, fwd, NULL, NULL);

	if (bs->currentEnemy && bs->currentEnemy->health > 0 
		&& bs->currentEnemy->client->ps.fallingToDeath == 0
		&& bs->frame_Enemy_Vis
		&& bs->currentEnemy->health > 0
		&& !(bs->currentEnemy->client && bs->currentEnemy->client->ps.weapon == WP_SABER && bs->currentEnemy->client->ps.saberHolstered >= 1))
	{// Like AIMod, head toward any enimies. 
		VectorCopy(bs->currentEnemy->r.currentOrigin, trto);
	}
	else
	{// No current enemy. Randomize. AIMod Fallback code.
		if (next_point[bs->entitynum] < level.time
			|| VectorDistance(bs->goalPosition, bs->origin) < 64
			|| !( bs->velocity[0] < 32 && bs->velocity[1] < 32 && bs->velocity[2] < 32 && bs->velocity[0] > -32 && bs->velocity[1] > -32 && bs->velocity[2] > -32 ))
		{// Ready for a new point.
			int choice = rand()%4;
			qboolean found = qfalse;
			
			while (found == qfalse)
			{
				if (choice == 2)
				{
					trto[0] = bs->origin[0] - ((rand()%1000));
					trto[1] = bs->origin[1] + ((rand()%1000));
				}
				else if (choice == 3)
				{
					trto[0] = bs->origin[0] + ((rand()%1000));
					trto[1] = bs->origin[1] - ((rand()%1000));
				}
				else if (choice == 4)
				{
					trto[0] = bs->origin[0] - ((rand()%1000));
					trto[1] = bs->origin[1] - ((rand()%1000));
				}
				else
				{
					trto[0] = bs->origin[0] + ((rand()%1000));
					trto[1] = bs->origin[1] + ((rand()%1000));
				}
			
				trto[2] = bs->origin[2];

				if (OrgVisible(bs->origin, trto, -1))
					found = qtrue;
			}

			next_point[bs->entitynum] = level.time + 2000 + (rand()%5)*1000;
		}
		else
		{// Still moving to the last point.
			return 1; // All is ok.. Keep moving forward.
		}
	}

	// Set the angles forward for checks.
	//VectorSubtract(bs->goalPosition, bs->origin, a);
	VectorSubtract(trto, bs->origin, a);
	vectoangles(a, ang);
	VectorCopy(ang, bs->goalAngles);

	trap_Trace(&tr, bs->origin, mins, maxs, trto, -1, MASK_SOLID);

	if (tr.fraction == 1)
	{// Visible point.
		if (CheckFall_By_Vectors(bs->origin, ang, &g_entities[bs->entitynum]) == qtrue)
		{// We would fall.
			VectorCopy(bs->origin, bs->goalPosition); // Stay still.
		}
		else
		{// Try to walk to "trto" if we won't fall.
			VectorCopy(trto, bs->goalPosition); // Original.

			VectorSubtract(bs->goalPosition, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);

			BotChangeViewAngles(bs, bs->goalAngles[YAW]);
		}
	}
	else
	{
		int tries = 0;

		while (CheckFall_By_Vectors(bs->origin, ang, &g_entities[bs->entitynum]) == qtrue && tries <= bot_thinklevel.integer)
		{// Keep trying until we get something valid? Too much CPU maybe?
			bs->goalAngles[YAW] = rand()%360; // Original.
			BotChangeViewAngles(bs, bs->goalAngles[YAW]);
			tries++;

			bs->goalAngles[PITCH] = 0;
			bs->goalAngles[ROLL] = 0;

			VectorCopy(bs->goalAngles, b_angle);

			AngleVectors(b_angle, fwd, NULL, NULL);

			VectorCopy(b_angle, ang);

			trto[0] = bs->origin[0] + ((rand()%4)*64);
			trto[1] = bs->origin[1] + ((rand()%4)*64);
			trto[2] = bs->origin[2];
		
			VectorCopy(trto, bs->goalPosition); // Move to new goal.
		}

		if (tries > bot_thinklevel.integer)
		{// Ran out of random attempts. We would fall. Stay still instead for now.
			VectorCopy(bs->origin, bs->goalPosition); // Stay still.
		}
	}

	return 1; // Success!
}
// End of AIMod fallback nav.


void FastBotAI(bot_state_t *bs, float thinktime)
{//fast version of Bot AI.  Used for situations when we don't want to do all Bot AI stuff
	int /*wp,*/ enemy;
	int desiredIndex;
	int goalWPIndex;
	int doingFallback = 0;
	int fjHalt;
	vec3_t a, ang, headlevel, eorg, noz_x, noz_y, dif, a_fo;
	float reaction;
	float bLeadAmount;
	int meleestrafe = 0;
	int useTheForce = 0;
	int forceHostile = 0;
	int cBAI = 0;
	gentity_t *friendInLOF = 0;
	float mLen;
	int visResult = 0;
//	int selResult = 0;
	int mineSelect = 0;
	int detSelect = 0;
	vec3_t preFrameGAngles;
	vec3_t	b_angle, fwd, trto; // AIMod
	qboolean isCapturing = qfalse;
/*	int thinklevel;

	if (next_think[bs->cur_ps.clientNum] > level.time)
	{
		if (bs->currentEnemy)
			trap_EA_Move(bs->client, bs->goalMovedir, 2500);
		else
			trap_EA_Move(bs->client, bs->goalMovedir, 5000);
		return;
	}

	thinklevel = bot_thinklevel.integer;
	if (thinklevel <= 0)
		thinklevel = 1;

	next_think[bs->cur_ps.clientNum] = level.time + (1400/thinklevel);
*/
	if (gDeactivated)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;
		return;
	}

	if (g_entities[bs->client].s.eType != ET_NPC &&
		g_entities[bs->client].inuse &&
		g_entities[bs->client].client &&
		g_entities[bs->client].client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;
		return;
	}


#ifndef FINAL_BUILD
	if (bot_getinthecarrr.integer)
	{ //stupid vehicle debug, I tire of having to connect another client to test passengers.
		gentity_t *botEnt = &g_entities[bs->client];

		if (botEnt->inuse && botEnt->client && botEnt->client->ps.m_iVehicleNum)
		{ //in a vehicle, so...
			bs->noUseTime = level.time + 5000;

			if (bot_getinthecarrr.integer != 2)
			{
				trap_EA_MoveForward(bs->client);

				if (bot_getinthecarrr.integer == 3)
				{ //use alt fire
					trap_EA_Alt_Attack(bs->client);
				}
			}
		}
		else
		{ //find one, get in
			int i = 0;
			gentity_t *vehicle = NULL;
			//find the nearest, manned vehicle
			while (i < MAX_GENTITIES)
			{
				vehicle = &g_entities[i];

				if (vehicle->inuse && vehicle->client && vehicle->s.eType == ET_NPC &&
					vehicle->s.NPC_class == CLASS_VEHICLE && vehicle->m_pVehicle &&
					(vehicle->client->ps.m_iVehicleNum || bot_getinthecarrr.integer == 2))
				{ //ok, this is a vehicle, and it has a pilot/passengers
					break;
				}
				i++;
			}
			if (i != MAX_GENTITIES && vehicle)
			{ //broke before end so we must've found something
				vec3_t v;

				VectorSubtract(vehicle->client->ps.origin, bs->origin, v);
				VectorNormalize(v);
				vectoangles(v, bs->goalAngles);
				MoveTowardIdealAngles(bs);
				trap_EA_Move(bs->client, v, 5000.0f);

				if (bs->noUseTime < (level.time-400))
				{
					bs->noUseTime = level.time + 500;
				}
			}
		}

		return;
	}
#endif

	if (bot_forgimmick.integer)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;

		if (bot_forgimmick.integer == 2)
		{ //for debugging saber stuff, this is handy
			trap_EA_Attack(bs->client);
		}

		if (bot_forgimmick.integer == 3)
		{ //for testing cpu usage moving around rmg terrain without AI
			vec3_t mdir;

			VectorSubtract(bs->origin, vec3_origin, mdir);
			VectorNormalize(mdir);
			trap_EA_Attack(bs->client);
			trap_EA_Move(bs->client, mdir, 5000);
		}

		if (bot_forgimmick.integer == 4)
		{ //constantly move toward client 0
			if (g_entities[0].client && g_entities[0].inuse)
			{
				vec3_t mdir;

				VectorSubtract(g_entities[0].client->ps.origin, bs->origin, mdir);
				VectorNormalize(mdir);
				trap_EA_Move(bs->client, mdir, 5000);
			}
		}

		if (bs->forceMove_Forward)
		{
			if (bs->forceMove_Forward > 0)
			{
				trap_EA_MoveForward(bs->client);
			}
			else
			{
				trap_EA_MoveBack(bs->client);
			}
		}
		if (bs->forceMove_Right)
		{
			if (bs->forceMove_Right > 0)
			{
				trap_EA_MoveRight(bs->client);
			}
			else
			{
				trap_EA_MoveLeft(bs->client);
			}
		}
		if (bs->forceMove_Up)
		{
			trap_EA_Jump(bs->client);
		}
		return;
	}

	if (!bs->lastDeadTime)
	{ //just spawned in?
		bs->lastDeadTime = level.time;
	}

	if (g_entities[bs->client].health < 1)
	{
		bs->lastDeadTime = level.time;

		if (!bs->deathActivitiesDone && bs->lastHurt && bs->lastHurt->client && bs->lastHurt->s.number != bs->client)
		{
			BotDeathNotify(bs);
			if (PassLovedOneCheck(bs, bs->lastHurt))
			{
				//CHAT: Died
				bs->chatObject = bs->lastHurt;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "Died", 0);
			}
			else if (!PassLovedOneCheck(bs, bs->lastHurt) &&
				botstates[bs->lastHurt->s.number] &&
				PassLovedOneCheck(botstates[bs->lastHurt->s.number], &g_entities[bs->client]))
			{ //killed by a bot that I love, but that does not love me
				bs->chatObject = bs->lastHurt;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "KilledOnPurposeByLove", 0);
			}

			bs->deathActivitiesDone = 1;
		}
		
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpCamping = NULL;
		bs->wpCampingTo = NULL;
		bs->wpStoreDest = NULL;
		bs->wpDestIgnoreTime = 0;
		bs->wpDestSwitchTime = 0;
		bs->wpSeenTime = 0;
		bs->wpDirection = 0;

		if (rand()%10 < 5 &&
			(!bs->doChat || bs->chatTime < level.time))
		{
			trap_EA_Attack(bs->client);
		}

		return;
	}

	VectorCopy(bs->goalAngles, preFrameGAngles);

	bs->doAttack = 0;
	bs->doAltAttack = 0;
	//reset the attack states

/*	if (bs->isSquadLeader)
	{
		CommanderBotAI(bs);
	}
	else
	{
		BotDoTeamplayAI(bs);
	}*/

	if (!bs->currentEnemy)
	{
		bs->frame_Enemy_Vis = 0;
	}

	if (bs->revengeEnemy && bs->revengeEnemy->client &&
		bs->revengeEnemy->client->pers.connected != CA_ACTIVE && bs->revengeEnemy->client->pers.connected != CA_AUTHORIZING)
	{
		bs->revengeEnemy = NULL;
		bs->revengeHateLevel = 0;
	}

	if (bs->currentEnemy && bs->currentEnemy->client &&
		bs->currentEnemy->client->pers.connected != CA_ACTIVE && bs->currentEnemy->client->pers.connected != CA_AUTHORIZING)
	{
		bs->currentEnemy = NULL;
	}

	fjHalt = 0;

/*#ifndef FORCEJUMP_INSTANTMETHOD
	if (bs->forceJumpChargeTime > level.time)
	{
		useTheForce = 1;
		forceHostile = 0;
	}


	if (bs->currentEnemy && bs->currentEnemy->client && bs->frame_Enemy_Vis && bs->forceJumpChargeTime < level.time)
#else
	if (bs->currentEnemy && bs->currentEnemy->client && bs->frame_Enemy_Vis)
#endif
	{
		VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, a_fo);
		vectoangles(a_fo, a_fo);

		if (g_gametype.integer == GT_RPG || g_gametype.integer == GT_COOP)
		{// Coop and RPG have new powers...
			//do this above all things
			if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH)) && (bs->doForcePush > level.time || bs->cur_ps.fd.forceGripBeingGripped > level.time) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
				useTheForce = 1;
				forceHostile = 1;
			}
			else if (bs->cur_ps.fd.forceSide == FORCE_DARKSIDE)
			{ //try dark side powers
			  //in order of priority top to bottom
				if (g_entities[bs->entitynum].health <= 75 
					&& (bs->cur_ps.fd.forcePowersKnown & (1 << FP_DRAIN)) && bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo) && bs->currentEnemy->client->ps.fd.forcePower > 10 )
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_LIGHTNING)) && bs->frame_Enemy_Len < FORCE_LIGHTNING_RADIUS && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_LIGHTNING;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) && (bs->cur_ps.fd.forcePowersActive & (1 << FP_GRIP)) && InFieldOfVision(bs->viewangles, 50, a_fo))
				{ //already gripping someone, so hold it
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) && bs->frame_Enemy_Len < MAX_GRIP_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_GRIP]][FP_GRIP] && InFieldOfVision(bs->viewangles, 50, a_fo))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_RAGE)) && g_entities[bs->client].health < 25 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_RAGE]][FP_RAGE])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_RAGE;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_DRAIN)) && bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo) && bs->currentEnemy->client->ps.fd.forcePower > 10 && bs->currentEnemy->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
					useTheForce = 1;
					forceHostile = 1;
				}
			}
			else if (bs->cur_ps.fd.forceSide == FORCE_LIGHTSIDE)
			{ //try light side powers
				if (g_entities[bs->entitynum].health <= 70
					&& bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL) 
					&& level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] )
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && bs->cur_ps.fd.forceGripCripple &&
					 level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{ //absorb to get out
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && bs->cur_ps.electrifyTime >= level.time &&
					 level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{ //absorb lightning
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_TELEPATHY)) && bs->frame_Enemy_Len < MAX_TRICK_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TELEPATHY]][FP_TELEPATHY] && InFieldOfVision(bs->viewangles, 50, a_fo) && !(bs->currentEnemy->client->ps.fd.forcePowersActive & (1 << FP_SEE)))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TELEPATHY;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && g_entities[bs->client].health < 75 && bs->currentEnemy->client->ps.fd.forceSide == FORCE_DARKSIDE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PROTECT)) && g_entities[bs->client].health < 35 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PROTECT]][FP_PROTECT])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_PROTECT;
					useTheForce = 1;
					forceHostile = 0;
				}
			}
		}
		else
		{
			//do this above all things
			if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH)) && (bs->doForcePush > level.time || bs->cur_ps.fd.forceGripBeingGripped > level.time) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH] )
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
				useTheForce = 1;
				forceHostile = 1;
			}
			else if (bs->cur_ps.fd.forceSide == FORCE_DARKSIDE)
			{ //try dark side powers
			  //in order of priority top to bottom
				if (g_entities[bs->entitynum].health <= 75 
					&& (bs->cur_ps.fd.forcePowersKnown & (1 << FP_DRAIN)) && bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo) && bs->currentEnemy->client->ps.fd.forcePower > 10 )
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) && (bs->cur_ps.fd.forcePowersActive & (1 << FP_GRIP)) && InFieldOfVision(bs->viewangles, 50, a_fo))
				{ //already gripping someone, so hold it
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) && bs->frame_Enemy_Len < MAX_GRIP_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_GRIP]][FP_GRIP] && InFieldOfVision(bs->viewangles, 50, a_fo))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_RAGE)) && g_entities[bs->client].health < 25 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_RAGE]][FP_RAGE])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_RAGE;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_LIGHTNING)) && bs->frame_Enemy_Len < FORCE_LIGHTNING_RADIUS && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_LIGHTNING;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_DRAIN)) && bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo) && bs->currentEnemy->client->ps.fd.forcePower > 10 && bs->currentEnemy->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
					useTheForce = 1;
					forceHostile = 1;
				}
			}
			else if (bs->cur_ps.fd.forceSide == FORCE_LIGHTSIDE)
			{ //try light side powers
				if (g_entities[bs->entitynum].health <= 70
					&& bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL) 
					&& level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] )
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && bs->cur_ps.fd.forceGripCripple &&
					 level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{ //absorb to get out
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && bs->cur_ps.electrifyTime >= level.time &&
					 level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{ //absorb lightning
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_TELEPATHY)) && bs->frame_Enemy_Len < MAX_TRICK_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TELEPATHY]][FP_TELEPATHY] && InFieldOfVision(bs->viewangles, 50, a_fo) && !(bs->currentEnemy->client->ps.fd.forcePowersActive & (1 << FP_SEE)))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TELEPATHY;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && g_entities[bs->client].health < 75 && bs->currentEnemy->client->ps.fd.forceSide == FORCE_DARKSIDE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PROTECT)) && g_entities[bs->client].health < 35 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PROTECT]][FP_PROTECT])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_PROTECT;
					useTheForce = 1;
					forceHostile = 0;
				}
			}	
		}

		if (!useTheForce)
		{ //try neutral powers
			if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH)) && bs->cur_ps.fd.forceGripBeingGripped > level.time && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH] && InFieldOfVision(bs->viewangles, 50, a_fo))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
				useTheForce = 1;
				forceHostile = 1;
			}
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_SPEED)) && g_entities[bs->client].health < 25 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_SPEED]][FP_SPEED])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SPEED;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_SEE)) && BotMindTricked(bs->client, bs->currentEnemy->s.number) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_SEE]][FP_SEE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SEE;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PULL)) && bs->frame_Enemy_Len < 256 && level.clients[bs->client].ps.fd.forcePower > 75 && InFieldOfVision(bs->viewangles, 50, a_fo))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PULL;
				useTheForce = 1;
				forceHostile = 1;
			}
		}
	}

	if (!useTheForce)
	{ //try powers that we don't care if we have an enemy for
		if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL)) && g_entities[bs->client].health < 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] && bs->cur_ps.fd.forcePowerLevel[FP_HEAL] > FORCE_LEVEL_1)
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
			useTheForce = 1;
			forceHostile = 0;
		}
		else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL)) && g_entities[bs->client].health < 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] && !bs->currentEnemy && bs->isCamping > level.time)
		{ //only meditate and heal if we're camping
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
			useTheForce = 1;
			forceHostile = 0;
		}
	}*/

/*	if (useTheForce && forceHostile)
	{
		if (bs->currentEnemy && bs->currentEnemy->client &&
			!ForcePowerUsableOn(&g_entities[bs->client], bs->currentEnemy, level.clients[bs->client].ps.fd.forcePowerSelected))
		{
			useTheForce = 0;
			forceHostile = 0;
		}
	}*/

	doingFallback = 0;

	bs->deathActivitiesDone = 0;

	/*if (BotUseInventoryItem(bs))
	{
		if (rand()%10 < 5)
		{
			trap_EA_Use(bs->client);
		}
	}*/

	/*if (bs->cur_ps.ammo[weaponData[bs->cur_ps.weapon].ammoIndex] < weaponData[bs->cur_ps.weapon].energyPerShot)
	{
		if (BotTryAnotherWeapon(bs))
		{
			return;
		}
	}
	else
	{
		if (bs->currentEnemy && bs->lastVisibleEnemyIndex == bs->currentEnemy->s.number &&
			bs->frame_Enemy_Vis && bs->forceWeaponSelect)
		{
			bs->forceWeaponSelect = 0;
		}

		if (bs->plantContinue > level.time)
		{
			bs->doAttack = 1;
			bs->destinationGrabTime = 0;
		}

		if (!bs->forceWeaponSelect && bs->cur_ps.hasDetPackPlanted && bs->plantKillEmAll > level.time)
		{
			bs->forceWeaponSelect = WP_DET_PACK;
		}

		if (bs->forceWeaponSelect)
		{
			selResult = BotSelectChoiceWeapon(bs, bs->forceWeaponSelect, 1);
		}

		if (selResult)
		{
			if (selResult == 2)
			{ //newly selected
				return;
			}
		}
		else if (BotSelectIdealWeapon(bs))
		{
			return;
		}
	}*/
	/*if (BotSelectMelee(bs))
	{
		return;
	}*/

	reaction = bs->skills.reflex/bs->settings.skill;

	if (reaction < 0)
	{
		reaction = 0;
	}
	if (reaction > 2000)
	{
		reaction = 2000;
	}

	if (!bs->currentEnemy)
	{
		bs->timeToReact = level.time + reaction;
	}

	if (bs->cur_ps.weapon == WP_DET_PACK && bs->cur_ps.hasDetPackPlanted && bs->plantKillEmAll > level.time)
	{
		bs->doAltAttack = 1;
	}

	if (bs->wpCamping)
	{
		if (bs->isCamping < level.time)
		{
			bs->wpCamping = NULL;
			bs->isCamping = 0;
		}

		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->wpCamping = NULL;
			bs->isCamping = 0;
		}
	}

	if (bs->wpCurrent &&
		(bs->wpSeenTime < level.time || bs->wpTravelTime < level.time))
	{
		bs->wpCurrent = NULL;
	}
	/*
	if (g_gametype.integer == GT_SCENARIO)
	{
		if (!bs->isCamping)
		{
			int captureNum = g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CAPTURE_ENTITYNUM];
			
			if (captureNum > 1)
			{
				gentity_t *flag = &g_entities[captureNum];

				if (!flag)
				{

				}
				else if (!bs->wpCurrent)
				{

				}
				else if (flag->classname != "flag" || flag->s.teamowner != &g_entities[bs->cur_ps.clientNum].s.teamowner && flag->s.time2 < 100)
				{// Check for capturing in scenario game...
					if (bs->wpCurrent->index+1 < gWPNum)
					{
						if (VectorDistance(bs->wpCurrent->origin, flag->s.origin) < VectorDistance(gWPArray[bs->wpCurrent->index+1]->origin, flag->s.origin))
							isCapturing = qtrue;//bs->isCamping = level.time + 5000; // Capturing a flag...
					}
				}
			}
		}
	}*/

	if (bs->currentEnemy)
	{
		if (bs->enemySeenTime < level.time ||
			!PassStandardEnemyChecks(bs, bs->currentEnemy))
		{
			if (bs->revengeEnemy == bs->currentEnemy &&
				bs->currentEnemy->health < 1 &&
				bs->lastAttacked && bs->lastAttacked == bs->currentEnemy)
			{
				//CHAT: Destroyed hated one [KilledHatedOne section]
				bs->chatObject = bs->revengeEnemy;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "KilledHatedOne", 1);
				bs->revengeEnemy = NULL;
				bs->revengeHateLevel = 0;
			}
			else if (bs->currentEnemy->health < 1 && PassLovedOneCheck(bs, bs->currentEnemy) &&
				bs->lastAttacked && bs->lastAttacked == bs->currentEnemy)
			{
				//CHAT: Killed
				bs->chatObject = bs->currentEnemy;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "Killed", 0);
			}

			bs->currentEnemy = NULL;
		}
	}

	if (bot_honorableduelacceptance.integer)
	{
		if (bs->currentEnemy && bs->currentEnemy->client &&
			bs->cur_ps.weapon == WP_SABER &&
			g_privateDuel.integer &&
			bs->frame_Enemy_Vis &&
			bs->frame_Enemy_Len < 400 &&
			bs->currentEnemy->client->ps.weapon == WP_SABER &&
			bs->currentEnemy->client->ps.saberHolstered)
		{
			vec3_t e_ang_vec;

			VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, e_ang_vec);

			if (InFieldOfVision(bs->viewangles, 100, e_ang_vec))
			{ //Our enemy has his saber holstered and has challenged us to a duel, so challenge him back
				if (!bs->cur_ps.saberHolstered)
				{
					Cmd_ToggleSaber_f(&g_entities[bs->client]);
				}
				else
				{
					if (bs->currentEnemy->client->ps.duelIndex == bs->client &&
						bs->currentEnemy->client->ps.duelTime > level.time &&
						!bs->cur_ps.duelInProgress)
					{
						Cmd_EngageDuel_f(&g_entities[bs->client]);
					}
				}

				bs->doAttack = 0;
				bs->doAltAttack = 0;
				bs->botChallengingTime = level.time + 100;
				bs->beStill = level.time + 100;
			}
		}
	}
	//Apparently this "allows you to cheese" when fighting against bots. I'm not sure why you'd want to con bots
	//into an easy kill, since they're bots and all. But whatever.

	if (!bs->wpCurrent)
	{
		/*wp = GetNearestVisibleWP(bs->origin, bs->client);

		if (wp != -1)
		{
			bs->wpCurrent = gWPArray[wp];
			bs->wpSeenTime = level.time + 1500;
			bs->wpTravelTime = level.time + 10000; //never take more than 10 seconds to travel to a waypoint
		}*/
		return;
	}

	if (bs->enemySeenTime < level.time || !bs->frame_Enemy_Vis || !bs->currentEnemy ||
		(bs->currentEnemy /*&& bs->cur_ps.weapon == WP_SABER && bs->frame_Enemy_Len > 300*/))
	{
		enemy = ScanForEnemies(bs);

		if (enemy != -1)
		{
			bs->currentEnemy = &g_entities[enemy];
			bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
		}
	}

	if (!bs->squadLeader && !bs->isSquadLeader)
	{
		BotScanForLeader(bs);
	}

	if (!bs->squadLeader && bs->squadCannotLead < level.time)
	{ //if still no leader after scanning, then become a squad leader
		bs->isSquadLeader = 1;
	}

	if (bs->isSquadLeader && bs->squadLeader)
	{ //we don't follow anyone if we are a leader
		bs->squadLeader = NULL;
	}

	//ESTABLISH VISIBILITIES AND DISTANCES FOR THE WHOLE FRAME HERE
	if (bs->wpCurrent)
	{
		if (g_RMG.integer)
		{ //this is somewhat hacky, but in RMG we don't really care about vertical placement because points are scattered across only the terrain.
			vec3_t vecB, vecC;

			vecB[0] = bs->origin[0];
			vecB[1] = bs->origin[1];
			vecB[2] = bs->origin[2];

			vecC[0] = bs->wpCurrent->origin[0];
			vecC[1] = bs->wpCurrent->origin[1];
			vecC[2] = vecB[2];


			VectorSubtract(vecC, vecB, a);
		}
		else
		{
			VectorSubtract(bs->wpCurrent->origin, bs->origin, a);
		}
		bs->frame_Waypoint_Len = VectorLength(a);

		if ( next_bot_vischeck[bs->entitynum] < level.time 
			|| VectorDistance(bs->origin, bs->wpCurrent->origin) > 128 
			|| !last_bot_wp_vischeck_result[bs->entitynum] )
		{
			visResult = WPOrgVisible(&g_entities[bs->client], bs->origin, bs->wpCurrent->origin, bs->client);
			//next_bot_vischeck[bs->entitynum] = level.time + 500;
			next_bot_vischeck[bs->entitynum] = level.time + 100;
			last_bot_wp_vischeck_result[bs->entitynum] = visResult;
		}
		else
		{
			visResult = last_bot_wp_vischeck_result[bs->entitynum];
			//visResult = 1;
		}

		if (visResult == 2)
		{
			bs->frame_Waypoint_Vis = 0;
			bs->wpSeenTime = 0;
			bs->wpDestination = NULL;
			bs->wpDestIgnoreTime = level.time + 5000;

			if (bs->wpDirection)
			{
				bs->wpDirection = 0;
			}
			else
			{
				bs->wpDirection = 1;
			}
		}
		else if (visResult)
		{
			bs->frame_Waypoint_Vis = 1;
		}
		else
		{
			bs->frame_Waypoint_Vis = 0;
		}
	}

	if (bs->currentEnemy)
	{
		if (bs->currentEnemy->client)
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, eorg);
			eorg[2] += bs->currentEnemy->client->ps.viewheight;
		}
		else
		{
			VectorCopy(bs->currentEnemy->s.origin, eorg);
		}

		VectorSubtract(eorg, bs->eye, a);
		bs->frame_Enemy_Len = VectorLength(a);

		if (OrgVisible(bs->eye, eorg, bs->client))
		{
			bs->frame_Enemy_Vis = 1;
			VectorCopy(eorg, bs->lastEnemySpotted);
			VectorCopy(bs->origin, bs->hereWhenSpotted);
			bs->lastVisibleEnemyIndex = bs->currentEnemy->s.number;
			//VectorCopy(bs->eye, bs->lastEnemySpotted);
			bs->hitSpotted = 0;
		}
		else
		{
			bs->frame_Enemy_Vis = 0;
		}
	}
	else
	{
		bs->lastVisibleEnemyIndex = ENTITYNUM_NONE;
	}
	//END

	if (bs->frame_Enemy_Vis)
	{
		bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
	}

	if (isCapturing)
	{// Dont move while capturing a flag...

	}
	else if (bs->wpCurrent && gWPArray /*&& !(bs->currentEnemy && bs->frame_Enemy_Vis)*/)
	{
		int wpTouchDist = BOT_WPTOUCH_DISTANCE;
		WPConstantRoutine(bs);

		if (!bs->wpCurrent)
		{ //WPConstantRoutine has the ability to nullify the waypoint if it fails certain checks, so..
			return;
		}

		if (bs->wpCurrent->flags & WPFLAG_WAITFORFUNC)
		{
			if (!CheckForFunc(bs->wpCurrent->origin, -1))
			{
				bs->beStill = level.time + 500; //no func brush under.. wait
			}
		}
		if (bs->wpCurrent->flags & WPFLAG_NOMOVEFUNC)
		{
			if (CheckForFunc(bs->wpCurrent->origin, -1))
			{
				bs->beStill = level.time + 500; //func brush under.. wait
			}
		}

		if (bs->frame_Waypoint_Vis || (bs->wpCurrent->flags & WPFLAG_NOVIS))
		{
			if (g_RMG.integer)
			{
				bs->wpSeenTime = level.time + 5000; //if we lose sight of the point, we have 1.5 seconds to regain it before we drop it
			}
			else
			{
				bs->wpSeenTime = level.time + 1500; //if we lose sight of the point, we have 1.5 seconds to regain it before we drop it
			}
		}
		VectorCopy(bs->wpCurrent->origin, bs->goalPosition);
		if (bs->wpDirection)
		{
			goalWPIndex = bs->wpCurrent->index-1;
		}
		else
		{
			goalWPIndex = bs->wpCurrent->index+1;
		}

		if (bs->wpCamping)
		{
			VectorSubtract(bs->wpCampingTo->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);

			VectorSubtract(bs->origin, bs->wpCamping->origin, a);
			if (VectorLength(a) < 64)
			{
				VectorCopy(bs->wpCamping->origin, bs->goalPosition);
				bs->beStill = level.time + 1000;

				if (!bs->campStanding)
				{
					bs->duckTime = level.time + 1000;
				}
			}
		}
		else if (gWPArray[goalWPIndex] && gWPArray[goalWPIndex]->inuse &&
			!(gLevelFlags & LEVELFLAG_NOPOINTPREDICTION))
		{
			VectorSubtract(gWPArray[goalWPIndex]->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);
		}
		else
		{
			VectorSubtract(bs->wpCurrent->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);
		}

		if (bs->destinationGrabTime < level.time /*&& (!bs->wpDestination || (bs->currentEnemy && bs->frame_Enemy_Vis))*/)
		{
			GetIdealDestination(bs);
		}
		
		if (bs->wpCurrent && bs->wpDestination)
		{
			if (TotalTrailDistance(bs->wpCurrent->index, bs->wpDestination->index, bs) == -1)
			{
				bs->wpDestination = NULL;
				bs->destinationGrabTime = level.time + 10000;
			}
		}

		if (g_RMG.integer)
		{
			if (bs->frame_Waypoint_Vis)
			{
				if (bs->wpCurrent && !bs->wpCurrent->flags)
				{
					wpTouchDist *= 3;
				}
			}
		}

		if (bs->frame_Waypoint_Len < wpTouchDist || (g_RMG.integer && bs->frame_Waypoint_Len < wpTouchDist*2))
		{
			WPTouchRoutine(bs);

			if (!bs->wpDirection)
			{
				desiredIndex = bs->wpCurrent->index+1;
			}
			else
			{
				desiredIndex = bs->wpCurrent->index-1;
			}

			if (gWPArray[desiredIndex] &&
				gWPArray[desiredIndex]->inuse &&
				desiredIndex < gWPNum &&
				desiredIndex >= 0 &&
				PassWayCheck(bs, desiredIndex))
			{
				bs->wpCurrent = gWPArray[desiredIndex];
			}
			else
			{
				if (bs->wpDestination)
				{
					bs->wpDestination = NULL;
					bs->destinationGrabTime = level.time + 10000;
				}

				if (bs->wpDirection)
				{
					bs->wpDirection = 0;
				}
				else
				{
					bs->wpDirection = 1;
				}
			}
		}
	}
	else //We can't find a waypoint, going to need a fallback routine.
	{
		/*if (g_gametype.integer == GT_DUEL)*/
		{ //helps them get out of messy situations
			/*if ((level.time - bs->forceJumpChargeTime) > 3500)
			{
				bs->forceJumpChargeTime = level.time + 2000;
				trap_EA_MoveForward(bs->client);
			}
			*/
			/*bs->jumpTime = level.time + 1500;
			bs->jumpHoldTime = level.time + 1500;
			bs->jDelay = 0;*/
		}
		doingFallback = AOTC_BotFallbackNavigation(bs);
	}

	if (bs->currentEnemy && bs->entitynum < MAX_CLIENTS)
	{
		if (g_entities[bs->entitynum].health <= 0 || bs->currentEnemy->health <= 0 )
		{

		}
		/*
		else if ( mod_classes.integer == 2 
		&& bs->settings.team == TEAM_RED
		&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_JEDI
		&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_FORCEMASTER )
		{// No jumping for other red team players...

		}*/
		else
		{
			vec3_t p1, p2, dir;
			float xy;
			qboolean willFall = qfalse;

			bs->goalAngles[PITCH] = 0;
			bs->goalAngles[ROLL] = 0;

			VectorCopy(bs->goalAngles, b_angle);
	
			AngleVectors(b_angle, fwd, NULL, NULL);

			if (!gWPArray && bs->BOTjumpState <= JS_WAITING) // Not in a jump.
			{
				VectorCopy(bs->currentEnemy->r.currentOrigin, trto);
			}

			bs->goalAngles[PITCH] = 0;
			bs->goalAngles[ROLL] = 0;

			VectorCopy(bs->goalAngles, b_angle);
	
			AngleVectors(b_angle, fwd, NULL, NULL);

			if (next_bot_fallcheck[bs->entitynum] < level.time)
			{
				if (CheckFall_By_Vectors(bs->origin, fwd, &g_entities[bs->entitynum]) == qtrue)
					willFall = qtrue;

				if (bot_will_fall[bs->entitynum])

				next_bot_fallcheck[bs->entitynum] = level.time + 50;
				bot_will_fall[bs->entitynum] = willFall;
			}
			else
				willFall = bot_will_fall[bs->entitynum];

			if ( bs->origin[2] > bs->currentEnemy->r.currentOrigin[2] )
			{
				VectorCopy( bs->origin, p1 );
				VectorCopy( bs->currentEnemy->r.currentOrigin, p2 );
			}
			else if ( bs->origin[2] < bs->currentEnemy->r.currentOrigin[2] )
			{
				VectorCopy( bs->currentEnemy->r.currentOrigin, p1 );
				VectorCopy( bs->origin, p2 );
			}
			else
			{
				VectorCopy( bs->origin, p1 );
				VectorCopy( bs->currentEnemy->r.currentOrigin, p2 );
			}

			VectorSubtract( p2, p1, dir );
			dir[2] = 0;

			//Get xy and z diffs
			xy = VectorNormalize( dir );

			// Jumping Stuff.
			if (bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& xy < 300 
				&& bs->currentEnemy->r.currentOrigin[2] > bs->origin[2] + 32
				&& bs->currentEnemy->health > 0
				&& bs->currentEnemy->client
				&& bs->currentEnemy->client->ps.groundEntityNum	!= ENTITYNUM_NONE /*!Falling(bs->currentEnemy)*/)
			{// Jump to enemy. NPC style! They are above us. Do a Jump.
				bs->BOTjumpState = JS_FACING;
				AIMod_Jump ( bs );
				
				VectorCopy(bs->currentEnemy->r.currentOrigin, jumpPos[bs->cur_ps.clientNum]);
			}
			else if (bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& xy > 700 
				&& bs->currentEnemy->r.currentOrigin[2] < bs->origin[2] - 32
				&& bs->currentEnemy->health > 0
				&& bs->currentEnemy->client
				&& bs->currentEnemy->client->ps.groundEntityNum	!= ENTITYNUM_NONE /*&& !Falling(bs->currentEnemy)*/)
			{// Jump to enemy. NPC style! They are below us.
				bs->BOTjumpState = JS_FACING;
				AIMod_Jump ( bs );
				
				VectorCopy(bs->currentEnemy->r.currentOrigin, jumpPos[bs->cur_ps.clientNum]);
			}
			else if (bs->BOTjumpState >= JS_CROUCHING)
			{// Continue any jumps.
				AIMod_Jump ( bs );
			}
		
			if (trto)
			{// We have a possibility.. Check it and maybe use it.
				VectorSubtract(trto, bs->origin, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);

				if (bs->jumpTime > level.time // In a jump.
					/*|| CheckFall_By_Vectors(bs->origin, ang, &g_entities[bs->entitynum]) == qfalse*/|| willFall) // or we wouldn't fall!
				{
					VectorCopy(trto, bs->goalPosition);			

					VectorSubtract(bs->goalPosition, bs->origin, a);
					vectoangles(a, ang);
					VectorCopy(ang, bs->goalAngles);
				}
				/*else // We would fall. Stay still.
				{
					if (bs->wpCurrent)
					{
						//int wp = GetSharedVisibleWP(&g_entities[bs->cur_ps.clientNum], bs->currentEnemy);
						//bs->wpCurrent = gWPArray[wp];
					}
					else
					{
						VectorCopy(bs->origin, bs->goalPosition);
						bs->beStill = level.time + 1000; //nowhere to go.
					}
				}*/
			}
			else
			{// No location found.
				if (bs->wpCurrent)
				{
					//int wp = GetSharedVisibleWP(&g_entities[bs->cur_ps.clientNum], bs->currentEnemy);
					//bs->wpCurrent = gWPArray[wp];
				}
				/*
				else if ( mod_classes.integer == 2 
					&& bs->settings.team == TEAM_RED
					&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_JEDI
					&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_FORCEMASTER )
				{// No jumping for other red team players...

				}*/
				else if (bs->jumpTime > level.time) // In a jump.
				{// We could be in jump.. Head to safepos if we have one.
					if (safePos[bs->cur_ps.clientNum])
					{
						VectorCopy(safePos[bs->cur_ps.clientNum], bs->goalPosition);
						VectorSubtract(bs->goalPosition, bs->origin, a);
						vectoangles(a, ang);
						VectorCopy(ang, bs->goalAngles);
					}
					else
					{
						VectorCopy(bs->origin, bs->goalPosition);
						bs->beStill = level.time + 1000; //nowhere to go.
					}
				}
				else
				{
					VectorCopy(bs->origin, bs->goalPosition);
					bs->beStill = level.time + 1000; //nowhere to go.
				}
			}
		}
	}
	//end of new aimod ai

	if (bs->currentEnemy && bs->BOTjumpState < JS_CROUCHING) // Force bots to walk while they have an enemy visible... Unique1
		g_entities[bs->cur_ps.clientNum].client->pers.cmd.buttons |= BUTTON_WALKING;

	if (g_RMG.integer)
	{ //for RMG if the bot sticks around an area too long, jump around randomly some to spread to a new area (horrible hacky method)
		vec3_t vSubDif;

		VectorSubtract(bs->origin, bs->lastSignificantAreaChange, vSubDif);
		if (VectorLength(vSubDif) > 1500)
		{
			VectorCopy(bs->origin, bs->lastSignificantAreaChange);
			bs->lastSignificantChangeTime = level.time + 20000;
		}

		if (bs->lastSignificantChangeTime < level.time)
		{
			bs->iHaveNoIdeaWhereIAmGoing = level.time + 17000;
		}
	}

	if (bs->iHaveNoIdeaWhereIAmGoing > level.time && !bs->currentEnemy)
	{
		VectorCopy(preFrameGAngles, bs->goalAngles);
		bs->wpCurrent = NULL;
		bs->wpSwitchTime = level.time + 150;
		doingFallback = AOTC_BotFallbackNavigation(bs);
		bs->jumpTime = level.time + 150;
		bs->jumpHoldTime = level.time + 150;
		bs->jDelay = 0;
		bs->lastSignificantChangeTime = level.time + 25000;
	}

	if (isCapturing)
	{

	}
	else if (bs->wpCurrent && g_RMG.integer)
	{
		qboolean doJ = qfalse;

		if (bs->wpCurrent->origin[2]-192 > bs->origin[2])
		{
			doJ = qtrue;
		}
		else if ((bs->wpTravelTime - level.time) < 5000 && bs->wpCurrent->origin[2]-64 > bs->origin[2])
		{
			doJ = qtrue;
		}
		else if ((bs->wpTravelTime - level.time) < 7000 && (bs->wpCurrent->flags & WPFLAG_RED_FLAG))
		{
			if ((level.time - bs->jumpTime) > 200)
			{
				bs->jumpTime = level.time + 100;
				bs->jumpHoldTime = level.time + 100;
				bs->jDelay = 0;
			}
		}
		else if ((bs->wpTravelTime - level.time) < 7000 && (bs->wpCurrent->flags & WPFLAG_BLUE_FLAG))
		{
			if ((level.time - bs->jumpTime) > 200)
			{
				bs->jumpTime = level.time + 100;
				bs->jumpHoldTime = level.time + 100;
				bs->jDelay = 0;
			}
		}
		else if (bs->wpCurrent->index > 0)
		{
			if ((bs->wpTravelTime - level.time) < 7000)
			{
				if ((gWPArray[bs->wpCurrent->index-1]->flags & WPFLAG_RED_FLAG) ||
					(gWPArray[bs->wpCurrent->index-1]->flags & WPFLAG_BLUE_FLAG))
				{
					if ((level.time - bs->jumpTime) > 200)
					{
						bs->jumpTime = level.time + 100;
						bs->jumpHoldTime = level.time + 100;
						bs->jDelay = 0;
					}
				}
			}
		}

		if (doJ)
		{
			bs->jumpTime = level.time + 1500;
			bs->jumpHoldTime = level.time + 1500;
			bs->jDelay = 0;
		}
	}

	if (doingFallback)
	{
		bs->doingFallback = qtrue;
	}
	else
	{
		bs->doingFallback = qfalse;
	}

	if (bs->timeToReact < level.time && bs->currentEnemy && bs->enemySeenTime > level.time + (ENEMY_FORGET_MS - (ENEMY_FORGET_MS*0.2)))
	{
		if (bs->frame_Enemy_Vis)
		{
			cBAI = CombatBotAI(bs, thinktime);
		}
		else if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT)
		{ //keep charging in case we see him again before we lose track of him
			bs->doAltAttack = 1;
		}
		else if (bs->cur_ps.weaponstate == WEAPON_CHARGING)
		{ //keep charging in case we see him again before we lose track of him
			bs->doAttack = 1;
		}

		if (bs->destinationGrabTime > level.time + 100)
		{
			bs->destinationGrabTime = level.time + 100; //assures that we will continue staying within a general area of where we want to be in a combat situation
		}

		if (bs->currentEnemy->client)
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, headlevel);
			headlevel[2] += bs->currentEnemy->client->ps.viewheight;
		}
		else
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, headlevel);
		}

		if (!bs->frame_Enemy_Vis)
		{
			//if (!bs->hitSpotted && VectorLength(a) > 256)
			if (OrgVisible(bs->eye, bs->lastEnemySpotted, -1))
			{
				VectorCopy(bs->lastEnemySpotted, headlevel);
				VectorSubtract(headlevel, bs->eye, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);

				if (bs->cur_ps.weapon == WP_FLECHETTE &&
					bs->cur_ps.weaponstate == WEAPON_READY &&
					bs->currentEnemy && bs->currentEnemy->client)
				{
					mLen = VectorLength(a) > 128;
					if (mLen > 128 && mLen < 1024)
					{
						VectorSubtract(bs->currentEnemy->client->ps.origin, bs->lastEnemySpotted, a);

						bs->frame_Enemy_Vis = qtrue;

						if (VectorLength(a) < 300)
						{
							bs->doAltAttack = 1;
						}
					}
				}
			}
		}
		else
		{
			bLeadAmount = BotWeaponCanLead(bs);
			if ((bs->skills.accuracy/bs->settings.skill) <= 8 &&
				bLeadAmount)
			{
				BotAimLeading(bs, headlevel, bLeadAmount);
			}
			else
			{
				VectorSubtract(headlevel, bs->eye, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);
			}

			BotAimOffsetGoalAngles(bs);
		}
	}

	if (bs->cur_ps.saberInFlight)
	{
		bs->saberThrowTime = level.time + Q_irand(4000, 10000);
	}

	if (bs->currentEnemy)
	{
		if (BotGetWeaponRange(bs) == BWEAPONRANGE_SABER)
		{
			int saberRange = SABER_ATTACK_RANGE;

			VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, a_fo);
			vectoangles(a_fo, a_fo);

			if (bs->saberPowerTime < level.time)
			{ //Don't just use strong attacks constantly, switch around a bit
				if (Q_irand(1, 10) <= 5)
				{
					bs->saberPower = qtrue;
				}
				else
				{
					bs->saberPower = qfalse;
				}

				bs->saberPowerTime = level.time + Q_irand(3000, 15000);
			}

/*			if ( g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_STAFF
				&& g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_DUAL )
			{
				if (bs->currentEnemy->health > 75 
					&& g_entities[bs->client].client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] > 2)
				{
					if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_STRONG 
						&& bs->saberPower)
					{ //if we are up against someone with a lot of health and we have a strong attack available, then h4q them
						Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
					}
				}
				else if (bs->currentEnemy->health > 40 
					&& g_entities[bs->client].client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] > 1)
				{
					if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_MEDIUM)
					{ //they're down on health a little, use level 2 if we can
						Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
					}
				}
				else
				{
					if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_FAST)
					{ //they've gone below 40 health, go at them with quick attacks
						Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
					}
				}
			}*/

			if ( (bs->cur_ps.weapon == WP_SABER /*|| BG_Is_Staff_Weapon(bs->cur_ps.weapon)*/)
				&& Q_irand(0,30000) < 2 ) 
			{// Switch stance every so often...
				Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
			}

			/*if (g_gametype.integer == GT_COOP)
			{
				saberRange *= 3;
			}*/

			if (bs->frame_Enemy_Len <= saberRange)
			{
				SaberCombatHandling(bs);

				if (bs->frame_Enemy_Len < 80)
				{
					meleestrafe = 1;
				}
			}
			else if (bs->saberThrowTime < level.time && !bs->cur_ps.saberInFlight &&
				(bs->cur_ps.fd.forcePowersKnown & (1 << FP_SABERTHROW)) &&
				InFieldOfVision(bs->viewangles, 30, a_fo) &&
				bs->frame_Enemy_Len < BOT_SABER_THROW_RANGE &&
				bs->cur_ps.fd.saberAnimLevel != SS_STAFF)
			{
				bs->doAltAttack = 1;
				bs->doAttack = 0;
			}
			else if (bs->cur_ps.saberInFlight && bs->frame_Enemy_Len > 300 && bs->frame_Enemy_Len < BOT_SABER_THROW_RANGE)
			{
				bs->doAltAttack = 1;
				bs->doAttack = 0;
			}
		}
		else if (BotGetWeaponRange(bs) == BWEAPONRANGE_MELEE)
		{
			if (bs->frame_Enemy_Len <= MELEE_ATTACK_RANGE)
			{
				MeleeCombatHandling(bs);
				meleestrafe = 1;
			}
		}
	}

	if (doingFallback && bs->currentEnemy
		&& g_entities[bs->entitynum].client->ps.weapon != WP_SABER) //just stand and fire if we have no idea where we are
	{
		VectorCopy(bs->origin, bs->goalPosition);
	}
	/*
	if ( mod_classes.integer == 2 
		&& bs->settings.team == TEAM_RED
		&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_JEDI
		&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_FORCEMASTER )
	{// No jumping for other red team players...

	}
	else*/ if (bs->forceJumping > level.time)
	{
		VectorCopy(bs->origin, noz_x);
		VectorCopy(bs->goalPosition, noz_y);

		noz_x[2] = noz_y[2];

		VectorSubtract(noz_x, noz_y, noz_x);

		if (VectorLength(noz_x) < 32)
		{
			fjHalt = 1;
		}
	}

	if (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTY)
		CTFFlagMovement(bs);

	if (/*bs->wpDestination &&*/ bs->shootGoal &&
		/*bs->wpDestination->associated_entity == bs->shootGoal->s.number &&*/
		bs->shootGoal->health > 0 && bs->shootGoal->takedamage)
	{
		dif[0] = (bs->shootGoal->r.absmax[0]+bs->shootGoal->r.absmin[0])/2;
		dif[1] = (bs->shootGoal->r.absmax[1]+bs->shootGoal->r.absmin[1])/2;
		dif[2] = (bs->shootGoal->r.absmax[2]+bs->shootGoal->r.absmin[2])/2;

		if (!bs->currentEnemy || bs->frame_Enemy_Len > 256)
		{ //if someone is close then don't stop shooting them for this
			VectorSubtract(dif, bs->eye, a);
			vectoangles(a, a);
			VectorCopy(a, bs->goalAngles);

			if (InFieldOfVision(bs->viewangles, 30, a) /*&&
				EntityVisibleBox(bs->origin, NULL, NULL, dif, bs->client, bs->shootGoal->s.number)*/)
			{
				bs->doAttack = 1;
			}
		}
	}

	if (bs->cur_ps.hasDetPackPlanted)
	{ //check if our enemy gets near it and detonate if he does
		BotCheckDetPacks(bs);
	}
	else if (bs->currentEnemy && bs->lastVisibleEnemyIndex == bs->currentEnemy->s.number && !bs->frame_Enemy_Vis && bs->plantTime < level.time &&
		!bs->doAttack && !bs->doAltAttack)
	{
		VectorSubtract(bs->origin, bs->hereWhenSpotted, a);

		if (bs->plantDecided > level.time || (bs->frame_Enemy_Len < BOT_PLANT_DISTANCE*2 && VectorLength(a) < BOT_PLANT_DISTANCE))
		{
			mineSelect = BotSelectChoiceWeapon(bs, WP_TRIP_MINE, 0);
			detSelect = BotSelectChoiceWeapon(bs, WP_DET_PACK, 0);
			if (bs->cur_ps.hasDetPackPlanted)
			{
				detSelect = 0;
			}

			if (bs->plantDecided > level.time && bs->forceWeaponSelect &&
				bs->cur_ps.weapon == bs->forceWeaponSelect)
			{
				bs->doAttack = 1;
				bs->plantDecided = 0;
				bs->plantTime = level.time + BOT_PLANT_INTERVAL;
				bs->plantContinue = level.time + 500;
				bs->beStill = level.time + 500;
			}
			else if (mineSelect || detSelect)
			{
				if (BotSurfaceNear(bs))
				{
					if (!mineSelect)
					{ //if no mines use detpacks, otherwise use mines
						mineSelect = WP_DET_PACK;
					}
					else
					{
						mineSelect = WP_TRIP_MINE;
					}

					detSelect = BotSelectChoiceWeapon(bs, mineSelect, 1);

					if (detSelect && detSelect != 2)
					{ //We have it and it is now our weapon
						bs->plantDecided = level.time + 1000;
						bs->forceWeaponSelect = mineSelect;
						return;
					}
					else if (detSelect == 2)
					{
						bs->forceWeaponSelect = mineSelect;
						return;
					}
				}
			}
		}
	}
	else if (bs->plantContinue < level.time)
	{
		bs->forceWeaponSelect = 0;
	}

	if (g_gametype.integer == GT_JEDIMASTER && !bs->cur_ps.isJediMaster && bs->jmState == -1 && gJMSaberEnt && gJMSaberEnt->inuse)
	{
		vec3_t saberLen;
		float fSaberLen = 0;

		VectorSubtract(bs->origin, gJMSaberEnt->r.currentOrigin, saberLen);
		fSaberLen = VectorLength(saberLen);

		if (fSaberLen < 256)
		{
			if (OrgVisible(bs->origin, gJMSaberEnt->r.currentOrigin, bs->client))
			{
				VectorCopy(gJMSaberEnt->r.currentOrigin, bs->goalPosition);
			}
		}
	}

	if (isCapturing)
	{

	}
	else if (bs->beStill < level.time && !WaitingForNow(bs, bs->goalPosition) && !fjHalt)
	{
		VectorSubtract(bs->goalPosition, bs->origin, bs->goalMovedir);
		VectorNormalize(bs->goalMovedir);

		if (bs->jumpTime > level.time && bs->jDelay < level.time &&
			level.clients[bs->client].pers.cmd.upmove > 0)
		{
		//	trap_EA_Move(bs->client, bs->origin, 5000);
			bs->beStill = level.time + 200;
		}
		else
		{
			if (bs->currentEnemy)
				trap_EA_Move(bs->client, bs->goalMovedir, 2500);
			else
				trap_EA_Move(bs->client, bs->goalMovedir, 5000);
		}

		if (meleestrafe)
		{
			StrafeTracing(bs);
		}

		if (bs->meleeStrafeDir && meleestrafe && bs->meleeStrafeDisable < level.time)
		{
			trap_EA_MoveRight(bs->client);
		}
		else if (meleestrafe && bs->meleeStrafeDisable < level.time)
		{
			trap_EA_MoveLeft(bs->client);
		}

		if (BotTrace_Jump(bs, bs->goalPosition))
		{
			bs->jumpTime = level.time + 100;
		}
		else if (BotTrace_Duck(bs, bs->goalPosition))
		{
			bs->duckTime = level.time + 100;
		}
#ifdef BOT_STRAFE_AVOIDANCE
		else
		{
			int strafeAround = BotTrace_Strafe(bs, bs->goalPosition);

			if (strafeAround == STRAFEAROUND_RIGHT)
			{
				trap_EA_MoveRight(bs->client);
			}
			else if (strafeAround == STRAFEAROUND_LEFT)
			{
				trap_EA_MoveLeft(bs->client);
			}
		}
#endif
	}

#ifndef FORCEJUMP_INSTANTMETHOD
	if (bs->forceJumpChargeTime > level.time)
	{
		bs->jumpTime = 0;
	}
#endif

	if (bs->jumpPrep > level.time)
	{
		bs->forceJumpChargeTime = 0;
	}

	if (bs->forceJumpChargeTime > level.time)
	{
		bs->jumpHoldTime = ((bs->forceJumpChargeTime - level.time)/2) + level.time;
		bs->forceJumpChargeTime = 0;
	}

	if (bs->jumpHoldTime > level.time)
	{
		bs->jumpTime = bs->jumpHoldTime;
	}

    if (isCapturing)
	{

	}
	else if (bs->jumpTime > level.time && bs->jDelay < level.time)
	{
//		gclient_t *client = g_entities[bs->cur_ps.clientNum].client;
		/*
		if ( g_gametype.integer == GT_COOP && classnumber[client->ps.clientNum] == CLASS_SOLDIER )
		{// Jumping for coop soldiers...
			if (client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
			{
				client->ps.eFlags |= EF_JETPACK;
			}
			else
			{
				client->ps.eFlags &= ~EF_JETPACK;
			}

			client->jetPackOn = qtrue;

			client->ps.pm_type = PM_JETPACK;
			client->ps.eFlags |= EF_JETPACK_ACTIVE;

			if (bs->jumpHoldTime > level.time)
			{
				if (bs->client > MAX_CLIENTS)
					g_entities[bs->client].client->ps.velocity[2] = 128;
				else
					trap_EA_Jump(bs->client);

				if (bs->wpCurrent)
				{
					if ((bs->wpCurrent->origin[2] - bs->origin[2]) < 64)
					{
						trap_EA_MoveForward(bs->client);
					}
				}
				else
				{
					trap_EA_MoveForward(bs->client);
				}
				if (g_entities[bs->client].client->ps.groundEntityNum == ENTITYNUM_NONE)
				{
					g_entities[bs->client].client->ps.pm_flags |= PMF_JUMP_HELD;
				}
			}
			else if (!(bs->cur_ps.pm_flags & PMF_JUMP_HELD))
			{
				if (bs->client > MAX_CLIENTS)
					g_entities[bs->client].client->ps.velocity[2] = 128;
				else
					trap_EA_Jump(bs->client);
			}
		}
		else if ( mod_classes.integer == 2 && bs->settings.team == TEAM_BLUE && g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] == GCLASS_SOLDIER )
		{
			if (client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
			{
				client->ps.eFlags |= EF_JETPACK;
			}
			else
			{
				client->ps.eFlags &= ~EF_JETPACK;
			}

			client->jetPackOn = qtrue;

			client->ps.pm_type = PM_JETPACK;
			client->ps.eFlags |= EF_JETPACK_ACTIVE;

			if (bs->jumpHoldTime > level.time)
			{
				if (bs->client > MAX_CLIENTS)
					g_entities[bs->client].client->ps.velocity[2] = 128;
				else
					trap_EA_Jump(bs->client);

				if (bs->wpCurrent)
				{
					if ((bs->wpCurrent->origin[2] - bs->origin[2]) < 64)
					{
						trap_EA_MoveForward(bs->client);
					}
				}
				else
				{
					trap_EA_MoveForward(bs->client);
				}
				if (g_entities[bs->client].client->ps.groundEntityNum == ENTITYNUM_NONE)
				{
					g_entities[bs->client].client->ps.pm_flags |= PMF_JUMP_HELD;
				}
			}
			else if (!(bs->cur_ps.pm_flags & PMF_JUMP_HELD))
			{
				if (bs->client > MAX_CLIENTS)
					g_entities[bs->client].client->ps.velocity[2] = 128;
				else
					trap_EA_Jump(bs->client);
			}
		}
		else*/
		{
			if (bs->jumpHoldTime > level.time)
			{
				trap_EA_Jump(bs->client);
				if (bs->wpCurrent)
				{
					if ((bs->wpCurrent->origin[2] - bs->origin[2]) < 64)
					{
						trap_EA_MoveForward(bs->client);
					}
				}
				else
				{
					trap_EA_MoveForward(bs->client);
				}
				if (g_entities[bs->client].client->ps.groundEntityNum == ENTITYNUM_NONE)
				{
					g_entities[bs->client].client->ps.pm_flags |= PMF_JUMP_HELD;
				}
			}
			else if (!(bs->cur_ps.pm_flags & PMF_JUMP_HELD))
			{
				trap_EA_Jump(bs->client);
			}
		}
	}

	if (bs->duckTime > level.time)
	{
		trap_EA_Crouch(bs->client);
	}

	if ( bs->dangerousObject && bs->dangerousObject->inuse && bs->dangerousObject->health > 0 &&
		bs->dangerousObject->takedamage && (!bs->frame_Enemy_Vis || !bs->currentEnemy) &&
		(BotGetWeaponRange(bs) == BWEAPONRANGE_MID || BotGetWeaponRange(bs) == BWEAPONRANGE_LONG) &&
		bs->cur_ps.weapon != WP_DET_PACK && bs->cur_ps.weapon != WP_TRIP_MINE && bs->cur_ps.weapon /*!= WP_TRIP_MINE_2*/ &&
		!bs->shootGoal )
	{
		float danLen;

		VectorSubtract(bs->dangerousObject->r.currentOrigin, bs->eye, a);

		danLen = VectorLength(a);

		if (danLen > 256)
		{
			vectoangles(a, a);
			VectorCopy(a, bs->goalAngles);

			if (Q_irand(1, 10) < 5)
			{
				bs->goalAngles[YAW] += Q_irand(0, 3);
				bs->goalAngles[PITCH] += Q_irand(0, 3);
			}
			else
			{
				bs->goalAngles[YAW] -= Q_irand(0, 3);
				bs->goalAngles[PITCH] -= Q_irand(0, 3);
			}

			if (InFieldOfVision(bs->viewangles, 30, a) &&
				EntityVisibleBox(bs->origin, NULL, NULL, bs->dangerousObject->r.currentOrigin, bs->client, bs->dangerousObject->s.number))
			{
				bs->doAttack = 1;
			}			
		}
	}

	if (PrimFiring(bs) ||
		AltFiring(bs))
	{
		friendInLOF = CheckForFriendInLOF(bs);

		if (friendInLOF)
		{
			if (PrimFiring(bs))
			{
				KeepPrimFromFiring(bs);
			}
			if (AltFiring(bs))
			{
				KeepAltFromFiring(bs);
			}
			if (useTheForce && forceHostile)
			{
				useTheForce = 0;
			}

			if (!useTheForce && friendInLOF->client)
			{ //we have a friend here and are not currently using force powers, see if we can help them out
				if (friendInLOF->health <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_HEAL;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if (friendInLOF->client->ps.fd.forcePower <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_FORCE;
					useTheForce = 1;
					forceHostile = 0;
				}
			}
		}
	}
	else if (g_gametype.integer >= GT_TEAM /*&& g_gametype.integer != GT_RPG*/)
	{ //still check for anyone to help..
		friendInLOF = CheckForFriendInLOF(bs);

		if (!useTheForce && friendInLOF)
		{
			if (friendInLOF->health <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_HEAL;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if (friendInLOF->client->ps.fd.forcePower <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_FORCE;
				useTheForce = 1;
				forceHostile = 0;
			}
		}
	}

	if (bs->doAttack && bs->cur_ps.weapon == WP_DET_PACK &&
		bs->cur_ps.hasDetPackPlanted)
	{ //maybe a bit hackish, but bots only want to plant one of these at any given time to avoid complications
		bs->doAttack = 0;
	}

	if (bs->doAttack && bs->cur_ps.weapon == WP_SABER &&
		bs->saberDefending && bs->currentEnemy && bs->currentEnemy->client &&
		BotWeaponBlockable(bs->currentEnemy->client->ps.weapon) )
	{
		bs->doAttack = 0;
	}

	if (bs->cur_ps.saberLockTime > level.time)
	{
		if (rand()%10 < 5)
		{
			bs->doAttack = 1;
		}
		else
		{
			bs->doAttack = 0;
		}
	}

	if (bs->botChallengingTime > level.time)
	{
		bs->doAttack = 0;
		bs->doAltAttack = 0;
	}

	if (bs->cur_ps.weapon == WP_SABER &&
		bs->cur_ps.saberInFlight &&
		!bs->cur_ps.saberEntityNum)
	{ //saber knocked away, keep trying to get it back
		bs->doAttack = 1;
		bs->doAltAttack = 0;
	}

	if (bs->doAttack)
	{
		trap_EA_Attack(bs->client);
	}
	else if (bs->doAltAttack)
	{
		trap_EA_Alt_Attack(bs->client);
	}
	MoveTowardIdealAngles(bs);
}


int GetSharedVisibleWP(gentity_t *bot, gentity_t *enemy)
{
	int i;
	float bestdist;
	float flLen;
	int bestindex;
	vec3_t a, mins, maxs;

	i = 0;
	bestdist = 800;//99999;
			   //don't trace over 800 units away to avoid GIANT HORRIBLE SPEED HITS ^_^
	bestindex = -1;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -1;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 1;

	while (i < gWPNum)
	{
		if (gWPArray[i] && gWPArray[i]->inuse)
		{
			VectorSubtract(bot->r.currentOrigin, gWPArray[i]->origin, a);
			flLen = VectorLength(a);

//			VectorSubtract(org, gWPArray[i]->origin, a);
//			flLenEnemy = VectorLength(a);

			if (flLen < bestdist 
				//&& trap_InPVS(bot->r.currentOrigin, gWPArray[i]->origin) 
				&& OrgVisibleBox(bot->r.currentOrigin, mins, maxs, gWPArray[i]->origin, 0)
				//&& trap_InPVS(enemy->r.currentOrigin, gWPArray[i]->origin) 
				&& OrgVisibleBox(enemy->r.currentOrigin, mins, maxs, gWPArray[i]->origin, 0))
			{// Visible to both the bot, and the enemy.
				bestdist = flLen;
				bestindex = i;
			}
		}

		i++;
	}

	return bestindex;
}


int Scenario_ScanForEnemies(bot_state_t *bs)
{
	vec3_t a;
	float distcheck;
	float closest;
	int bestindex;
	int i;
	float hasEnemyDist = 0;
//	qboolean noAttackNonJM = qfalse;

	closest = 999999;
	//closest = 4000; // Much closer in AIMod.
	i = 0;
	bestindex = -1;

	if (bs->currentEnemy)
	{ //only switch to a new enemy if he's significantly closer
		hasEnemyDist = bs->frame_Enemy_Len;
	}

	while (i <= MAX_GENTITIES)
	{
		if (g_entities[i].s.eType != ET_NPC && g_entities[i].s.eType != ET_PLAYER)
		{
			i++;
			continue;
		}

		if ( VectorDistance( g_entities[i].r.currentOrigin, bs->origin ) > 4000 )
		{
			i++;
			continue;
		}

		if (i == bs->client || !g_entities[i].client)
		{
			i++;
			continue;
		}

		if ( !OnSameTeam(&g_entities[bs->client], &g_entities[i]) && PassStandardEnemyChecks(bs, &g_entities[i]) && BotPVSCheck(g_entities[i].r.currentOrigin, bs->eye) )
		{
//			qboolean holstered = qfalse;
			VectorSubtract(g_entities[i].client->ps.origin, bs->eye, a);
			distcheck = VectorLength(a);
			vectoangles(a, a);

			if (distcheck < closest)
			{
				if ( ( InFieldOfVision(bs->viewangles, 90, a) && !BotMindTricked(bs->client, i) ) || BotCanHear(bs, &g_entities[i], distcheck) ) 
				{
					//if ( OrgVisible(bs->eye, g_entities[i].client->ps.origin, -1) ) //<-- AIMod.
					{
						if (BotMindTricked(bs->client, i))
						{
							if (distcheck < 256 || (level.time - g_entities[i].client->dangerTime) < 100)
							{
								if (!hasEnemyDist || distcheck < (hasEnemyDist - 128))
								{ //if we have an enemy, only switch to closer if he is 128+ closer to avoid flipping out
									closest = distcheck;
									bestindex = i;
								}
							}
						}
						else
						{
							if (!hasEnemyDist || distcheck < (hasEnemyDist - 128))
							{ //if we have an enemy, only switch to closer if he is 128+ closer to avoid flipping out
								closest = distcheck;
								bestindex = i;
							}
						}
					}
				}
			}
		}
		i++;
	}
	
	return bestindex;
}


//[NewGameTypes][EnhancedImpliment]
/*
//standard check to find a new enemy.
int Scenario_ScanForEnemies(bot_state_t *bs)
{
	vec3_t a;
	float distcheck;
	float closest;
	int bestindex;
	int i;
	float hasEnemyDist = 0;
//	qboolean noAttackNonJM = qfalse;

	closest = 999999;
	//closest = 4000; // Much closer in AIMod.
	i = 0;
	bestindex = -1;

	if (bs->currentEnemy)
	{ //only switch to a new enemy if he's significantly closer
		hasEnemyDist = bs->frame_Enemy_Len;
	}

	while (i <= MAX_GENTITIES)
	{
		if (g_entities[i].s.eType != ET_NPC && g_entities[i].s.eType != ET_PLAYER)
		{
			i++;
			continue;
		}

		if ( VectorDistance( g_entities[i].r.currentOrigin, bs->origin ) > 4000 )
		{
			i++;
			continue;
		}

		if (i == bs->client || !g_entities[i].client)
		{
			i++;
			continue;
		}

		if ( !OnSameTeam(&g_entities[bs->client], &g_entities[i]) && PassStandardEnemyChecks(bs, &g_entities[i]) && BotPVSCheck(g_entities[i].r.currentOrigin, bs->eye) )
		{
//			qboolean holstered = qfalse;
			VectorSubtract(g_entities[i].client->ps.origin, bs->eye, a);
			distcheck = VectorLength(a);
			vectoangles(a, a);

			if (distcheck < closest)
			{
				if ( ( InFieldOfVision(bs->viewangles, 90, a) && !BotMindTricked(bs->client, i) ) || BotCanHear(bs, &g_entities[i], distcheck) ) 
				{
					//if ( OrgVisible(bs->eye, g_entities[i].client->ps.origin, -1) ) //<-- AIMod.
					{
						if (BotMindTricked(bs->client, i))
						{
							if (distcheck < 256 || (level.time - g_entities[i].client->dangerTime) < 100)
							{
								if (!hasEnemyDist || distcheck < (hasEnemyDist - 128))
								{ //if we have an enemy, only switch to closer if he is 128+ closer to avoid flipping out
									closest = distcheck;
									bestindex = i;
								}
							}
						}
						else
						{
							if (!hasEnemyDist || distcheck < (hasEnemyDist - 128))
							{ //if we have an enemy, only switch to closer if he is 128+ closer to avoid flipping out
								closest = distcheck;
								bestindex = i;
							}
						}
					}
				}
			}
		}
		i++;
	}
	
	return bestindex;
}
*/
//[/NewGameTypes]

//[VoiceEvents][EnhancedImpliment]
/*
extern vmCvar_t	mod_noForcedModDownload;

int next_voice_event[MAX_CLIENTS];
void G_BotVoiceEvent( gentity_t *self )
{
	int event = 0;

	if (next_voice_event[self->client->ps.clientNum] > level.time)
		return;
	
	next_voice_event[self->client->ps.clientNum] = level.time + Q_irand(25000, 60000);

	if (mod_noForcedModDownload.integer > 1)
		return; // No voicechat when no forced download.

	if ( !self->client || self->client->ps.pm_type >= PM_DEAD )
	{
		return;
	}

	if (self->s.weapon && self->s.weapon == WP_SABER)
	{
		int choice = rand()%27;

		switch (choice)
		{
			// Generic Sounds.
			case 1:
				event = EV_ANGER1;
				break;
			case 2:
				event = EV_ANGER2;
				break;
			case 3:
				event = EV_ANGER3;
				break;
			case 4:
				event = EV_CONFUSE1;
				break;
			case 5:
				event = EV_CONFUSE2;
				break;
			case 6:
				event = EV_CONFUSE3;
				break;
			case 7:
				event = EV_FFWARN;
				break;
			case 8:
				event = EV_FFTURN;
				break;
			case 9:
				event = EV_ANGER1;
				break;
			// Jedi Sounds.
			case 10:
				event = EV_COMBAT1;
				break;
			case 11:
				event = EV_COMBAT2;
				break;
			case 12:
				event = EV_COMBAT3;
				break;
			case 13:
				event = EV_JDETECTED1;
				break;
			case 14:
				event = EV_JDETECTED2;
				break;
			case 15:
				event = EV_JDETECTED3;
				break;
			case 16:
				event = EV_TAUNT1;
				break;
			case 17:
				event = EV_TAUNT2;
				break;
			case 18:
				event = EV_TAUNT3;
				break;
			case 19:
				event = EV_JCHASE1;
				break;
			case 20:
				event = EV_JCHASE2;
				break;
			case 21:
				event = EV_JCHASE3;
				break;
			case 22:
				event = EV_JLOST1;
				break;
			case 23:
				event = EV_JLOST2;
				break;
			case 24:
				event = EV_JLOST3;
				break;
			case 25:
				event = EV_GLOAT1;
				break;
			case 26:
				event = EV_GLOAT2;
				break;
			case 27:
				event = EV_GLOAT3;
				break;
			default:
				event = EV_TAUNT1;
				break;
		}
	}
	else
	{
		int choice = rand()%45;

		switch (choice)
		{
			// Generic Sounds.
			case 1:
				event = EV_ANGER1;
				break;
			case 2:
				event = EV_ANGER2;
				break;
			case 3:
				event = EV_ANGER3;
				break;
			case 4:
				event = EV_CONFUSE1;
				break;
			case 5:
				event = EV_CONFUSE2;
				break;
			case 6:
				event = EV_CONFUSE3;
				break;
			case 7:
				event = EV_FFWARN;
				break;
			case 8:
				event = EV_FFTURN;
				break;
			case 9:
				event = EV_ANGER1;
				break;
			// Jedi Sounds.
			case 10:
				event = EV_CHASE1;
				break;
			case 11:
				event = EV_CHASE2;
				break;
			case 12:
				event = EV_CHASE3;
				break;
			case 13:
				event = EV_COVER1;
				break;
			case 14:
				event = EV_COVER2;
				break;
			case 15:
				event = EV_COVER3;
				break;
			case 16:
				event = EV_COVER4;
				break;
			case 17:
				event = EV_COVER5;
				break;
			case 18:
				event = EV_DETECTED1;
				break;
			case 19:
				event = EV_DETECTED2;
				break;
			case 20:
				event = EV_DETECTED3;
				break;
			case 21:
				event = EV_DETECTED4;
				break;
			case 22:
				event = EV_DETECTED5;
				break;
			case 23:
				event = EV_LOST1;
				break;
			case 24:
				event = EV_OUTFLANK1;
				break;
			case 25:
				event = EV_OUTFLANK2;
				break;
			case 26:
				event = EV_ESCAPING1;
				break;
			case 27:
				event = EV_ESCAPING2;
				break;
			case 28:
				event = EV_ESCAPING3;
				break;
			case 29:
				event = EV_GIVEUP1;
				break;
			case 30:
				event = EV_GIVEUP2;
				break;
			case 31:
				event = EV_GIVEUP3;
				break;
			case 32:
				event = EV_GIVEUP4;
				break;
			case 33:
				event = EV_LOOK1;
				break;
			case 34:
				event = EV_LOOK2;
				break;
			case 35:
				event = EV_SIGHT1;
				break;
			case 36:
				event = EV_SIGHT2;
				break;
			case 37:
				event = EV_SIGHT3;
				break;
			case 38:
				event = EV_SOUND1;
				break;
			case 39:
				event = EV_SOUND2;
				break;
			case 40:
				event = EV_SOUND3;
				break;
			case 41:
				event = EV_SUSPICIOUS1;
				break;
			case 42:
				event = EV_SUSPICIOUS2;
				break;
			case 43:
				event = EV_SUSPICIOUS3;
				break;
			case 44:
				event = EV_SUSPICIOUS4;
				break;
			case 45:
				event = EV_SUSPICIOUS5;
				break;
			default:
				event = EV_TAUNT1;
				break;
		}
	}

	//FIXME: Also needs to check for teammates. Don't want
	//		everyone babbling at once

	//NOTE: was losing too many speech events, so we do it directly now, screw networking!
	G_AddEvent( self, event, 0 );
	//G_SpeechEvent( self, event );

	//won't speak again for 5 seconds (unless otherwise specified)
	//self->NPC->blockedSpeechDebounceTime = level.time + ((speakDebounceTime==0) ? 5000 : speakDebounceTime);
}
*/
//[/VoiceEvents][EnhancedImpliment]


/* looks like other AotC code uses this.  I'm dumping this here for now.
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
*/


qboolean visible (gentity_t *self, gentity_t *other)
{// From my ET mod... Unique1.
	vec3_t		spot1;
	vec3_t		spot2;
	trace_t		tr;
	gentity_t	*traceEnt;

	if (!self->r.currentOrigin || !other->r.currentOrigin)
		return qfalse;

	if (!other->client || !self->client)
		return qfalse;

	VectorCopy(self->r.currentOrigin, spot1);
	spot1[2]+=48;

	VectorCopy(other->r.currentOrigin, spot2);
	spot2[2]+=48;

	//trap_Trace (&tr, self->r.currentOrigin, NULL, NULL, other->r.currentOrigin, self->s.number, MASK_SHOT);

	// And a standard pass..
	trap_Trace (&tr, spot1, NULL, NULL, other->r.currentOrigin, self->s.number, MASK_SHOT);
	
	traceEnt = &g_entities[ tr.entityNum ];

	if (traceEnt == other)
		return qtrue;

	return qfalse;	
}


//[NewGameTypes][EnhancedImpliment]
/*
qboolean CheckAboveOK(vec3_t origin) // For rancor!
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

	if (up[2] <= 128
		&& tr.fraction == 1)
		return qfalse; // No room above!
	else
		return qtrue; // All is ok!
}

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
*/
//[/NewGameTypes][EnhancedImpliment]

int next_taunt[MAX_CLIENTS];	
void AOTC_StandardBotAI(bot_state_t *bs, float thinktime)
{
	int wp, enemy;
	int desiredIndex;
	int goalWPIndex;
	int doingFallback = 0;
	int fjHalt;
	vec3_t a, ang, headlevel, eorg, noz_x, noz_y, dif, a_fo;
	float reaction;
	float bLeadAmount;
	int meleestrafe = 0;
	int useTheForce = 0;
	int forceHostile = 0;
	int cBAI = 0;
	gentity_t *friendInLOF = 0;
	float mLen;
	int visResult = 0;
	int selResult = 0;
	int mineSelect = 0;
	int detSelect = 0;
	vec3_t preFrameGAngles;
	vec3_t	b_angle, fwd, trto; // AIMod
	qboolean isCapturing = qfalse;
/*	int thinklevel;

	if (next_think[bs->cur_ps.clientNum] > level.time)
	{
		if (bs->currentEnemy)
			trap_EA_Move(bs->client, bs->goalMovedir, 2500);
		else
			trap_EA_Move(bs->client, bs->goalMovedir, 5000);
		return;
	}

	thinklevel = bot_thinklevel.integer;
	if (thinklevel <= 0)
		thinklevel = 1;

	next_think[bs->cur_ps.clientNum] = level.time + (1400/thinklevel);
*/
	if (gDeactivated)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;
		return;
	}

	if (g_entities[bs->client].s.eType != ET_NPC &&
		g_entities[bs->client].inuse &&
		g_entities[bs->client].client &&
		g_entities[bs->client].client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;
		return;
	}


#ifndef FINAL_BUILD
	if (bot_getinthecarrr.integer)
	{ //stupid vehicle debug, I tire of having to connect another client to test passengers.
		gentity_t *botEnt = &g_entities[bs->client];

		if (botEnt->inuse && botEnt->client && botEnt->client->ps.m_iVehicleNum)
		{ //in a vehicle, so...
			bs->noUseTime = level.time + 5000;

			if (bot_getinthecarrr.integer != 2)
			{
				trap_EA_MoveForward(bs->client);

				if (bot_getinthecarrr.integer == 3)
				{ //use alt fire
					trap_EA_Alt_Attack(bs->client);
				}
			}
		}
		else
		{ //find one, get in
			int i = 0;
			gentity_t *vehicle = NULL;
			//find the nearest, manned vehicle
			while (i < MAX_GENTITIES)
			{
				vehicle = &g_entities[i];

				if (vehicle->inuse && vehicle->client && vehicle->s.eType == ET_NPC &&
					vehicle->s.NPC_class == CLASS_VEHICLE && vehicle->m_pVehicle &&
					(vehicle->client->ps.m_iVehicleNum || bot_getinthecarrr.integer == 2))
				{ //ok, this is a vehicle, and it has a pilot/passengers
					break;
				}
				i++;
			}
			if (i != MAX_GENTITIES && vehicle)
			{ //broke before end so we must've found something
				vec3_t v;

				VectorSubtract(vehicle->client->ps.origin, bs->origin, v);
				VectorNormalize(v);
				vectoangles(v, bs->goalAngles);
				MoveTowardIdealAngles(bs);
				trap_EA_Move(bs->client, v, 5000.0f);

				if (bs->noUseTime < (level.time-400))
				{
					bs->noUseTime = level.time + 500;
				}
			}
		}

		return;
	}
#endif

	if (bot_forgimmick.integer)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;

		if (bot_forgimmick.integer == 2)
		{ //for debugging saber stuff, this is handy
			trap_EA_Attack(bs->client);
		}

		if (bot_forgimmick.integer == 3)
		{ //for testing cpu usage moving around rmg terrain without AI
			vec3_t mdir;

			VectorSubtract(bs->origin, vec3_origin, mdir);
			VectorNormalize(mdir);
			trap_EA_Attack(bs->client);
			trap_EA_Move(bs->client, mdir, 5000);
		}

		if (bot_forgimmick.integer == 4)
		{ //constantly move toward client 0
			if (g_entities[0].client && g_entities[0].inuse)
			{
				vec3_t mdir;

				VectorSubtract(g_entities[0].client->ps.origin, bs->origin, mdir);
				VectorNormalize(mdir);
				trap_EA_Move(bs->client, mdir, 5000);
			}
		}

		if (bs->forceMove_Forward)
		{
			if (bs->forceMove_Forward > 0)
			{
				trap_EA_MoveForward(bs->client);
			}
			else
			{
				trap_EA_MoveBack(bs->client);
			}
		}
		if (bs->forceMove_Right)
		{
			if (bs->forceMove_Right > 0)
			{
				trap_EA_MoveRight(bs->client);
			}
			else
			{
				trap_EA_MoveLeft(bs->client);
			}
		}
		if (bs->forceMove_Up)
		{
			trap_EA_Jump(bs->client);
		}
		return;
	}

	if (!bs->lastDeadTime)
	{ //just spawned in?
		bs->lastDeadTime = level.time;
	}

	if (g_entities[bs->client].health < 1)
	{
		bs->lastDeadTime = level.time;

		if (!bs->deathActivitiesDone && bs->lastHurt && bs->lastHurt->client && bs->lastHurt->s.number != bs->client)
		{
			BotDeathNotify(bs);
			if (PassLovedOneCheck(bs, bs->lastHurt))
			{
				//CHAT: Died
				bs->chatObject = bs->lastHurt;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "Died", 0);
			}
			else if (!PassLovedOneCheck(bs, bs->lastHurt) &&
				botstates[bs->lastHurt->s.number] &&
				PassLovedOneCheck(botstates[bs->lastHurt->s.number], &g_entities[bs->client]))
			{ //killed by a bot that I love, but that does not love me
				bs->chatObject = bs->lastHurt;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "KilledOnPurposeByLove", 0);
			}

			bs->deathActivitiesDone = 1;
		}
		
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpCamping = NULL;
		bs->wpCampingTo = NULL;
		bs->wpStoreDest = NULL;
		bs->wpDestIgnoreTime = 0;
		bs->wpDestSwitchTime = 0;
		bs->wpSeenTime = 0;
		bs->wpDirection = 0;

		if (rand()%10 < 5 &&
			(!bs->doChat || bs->chatTime < level.time))
		{
			trap_EA_Attack(bs->client);
		}

		return;
	}

	VectorCopy(bs->goalAngles, preFrameGAngles);

	bs->doAttack = 0;
	bs->doAltAttack = 0;
	//reset the attack states

	if (bs->isSquadLeader)
	{
		CommanderBotAI(bs);
	}
	else
	{
		BotDoTeamplayAI(bs);
	}

	if (!bs->currentEnemy)
	{
		bs->frame_Enemy_Vis = 0;
	}

	if (bs->revengeEnemy && bs->revengeEnemy->client &&
		bs->revengeEnemy->client->pers.connected != CA_ACTIVE && bs->revengeEnemy->client->pers.connected != CA_AUTHORIZING)
	{
		bs->revengeEnemy = NULL;
		bs->revengeHateLevel = 0;
	}

	if (bs->currentEnemy && bs->currentEnemy->client &&
		bs->currentEnemy->client->pers.connected != CA_ACTIVE && bs->currentEnemy->client->pers.connected != CA_AUTHORIZING)
	{
		bs->currentEnemy = NULL;
	}

	fjHalt = 0;

#ifndef FORCEJUMP_INSTANTMETHOD
	if (bs->forceJumpChargeTime > level.time)
	{
		useTheForce = 1;
		forceHostile = 0;
	}

	if (bs->currentEnemy && bs->currentEnemy->client && bs->frame_Enemy_Vis && bs->forceJumpChargeTime < level.time)
#else
	if (bs->currentEnemy && bs->currentEnemy->client && bs->frame_Enemy_Vis)
#endif
	{
		VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, a_fo);
		vectoangles(a_fo, a_fo);

		//[NewGameTypes][EnhancedImpliment]
		/*
		if (g_gametype.integer == GT_RPG || g_gametype.integer == GT_COOP)
		{// Coop and RPG have new powers...
			//do this above all things
			if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH)) && (bs->doForcePush > level.time || bs->cur_ps.fd.forceGripBeingGripped > level.time) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
				useTheForce = 1;
				forceHostile = 1;
			}
			else if (bs->cur_ps.fd.forceSide == FORCE_DARKSIDE)
			{ //try dark side powers
			  //in order of priority top to bottom
				if (g_entities[bs->entitynum].health <= 75 
					&& (bs->cur_ps.fd.forcePowersKnown & (1 << FP_DRAIN)) && bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo) && bs->currentEnemy->client->ps.fd.forcePower > 10 )
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_LIGHTNING)) && bs->frame_Enemy_Len < FORCE_LIGHTNING_RADIUS && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_LIGHTNING;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) && (bs->cur_ps.fd.forcePowersActive & (1 << FP_GRIP)) && InFieldOfVision(bs->viewangles, 50, a_fo))
				{ //already gripping someone, so hold it
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) && bs->frame_Enemy_Len < MAX_GRIP_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_GRIP]][FP_GRIP] && InFieldOfVision(bs->viewangles, 50, a_fo))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_RAGE)) && g_entities[bs->client].health < 25 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_RAGE]][FP_RAGE])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_RAGE;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_DRAIN)) && bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo) && bs->currentEnemy->client->ps.fd.forcePower > 10 && bs->currentEnemy->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
					useTheForce = 1;
					forceHostile = 1;
				}
			}
			else if (bs->cur_ps.fd.forceSide == FORCE_LIGHTSIDE)
			{ //try light side powers
				if (g_entities[bs->entitynum].health <= 70
					&& bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL) 
					&& level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] )
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && bs->cur_ps.fd.forceGripCripple &&
					 level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{ //absorb to get out
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && bs->cur_ps.electrifyTime >= level.time &&
					 level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{ //absorb lightning
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_TELEPATHY)) && bs->frame_Enemy_Len < MAX_TRICK_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TELEPATHY]][FP_TELEPATHY] && InFieldOfVision(bs->viewangles, 50, a_fo) && !(bs->currentEnemy->client->ps.fd.forcePowersActive & (1 << FP_SEE)))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TELEPATHY;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && g_entities[bs->client].health < 75 && bs->currentEnemy->client->ps.fd.forceSide == FORCE_DARKSIDE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PROTECT)) && g_entities[bs->client].health < 35 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PROTECT]][FP_PROTECT])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_PROTECT;
					useTheForce = 1;
					forceHostile = 0;
				}
			}
		}
		else
		*/
		//[/NewGameTypes][EnhancedImpliment]
		{
			//do this above all things
			if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH)) && (bs->doForcePush > level.time || bs->cur_ps.fd.forceGripBeingGripped > level.time) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH] /*&& InFieldOfVision(bs->viewangles, 50, a_fo)*/)
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
				useTheForce = 1;
				forceHostile = 1;
			}
			else if (bs->cur_ps.fd.forceSide == FORCE_DARKSIDE)
			{ //try dark side powers
			  //in order of priority top to bottom
				if (g_entities[bs->entitynum].health <= 75 
					&& (bs->cur_ps.fd.forcePowersKnown & (1 << FP_DRAIN)) && bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo) && bs->currentEnemy->client->ps.fd.forcePower > 10 )
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) && (bs->cur_ps.fd.forcePowersActive & (1 << FP_GRIP)) && InFieldOfVision(bs->viewangles, 50, a_fo))
				{ //already gripping someone, so hold it
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) && bs->frame_Enemy_Len < MAX_GRIP_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_GRIP]][FP_GRIP] && InFieldOfVision(bs->viewangles, 50, a_fo))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_RAGE)) && g_entities[bs->client].health < 25 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_RAGE]][FP_RAGE])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_RAGE;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_LIGHTNING)) && bs->frame_Enemy_Len < FORCE_LIGHTNING_RADIUS && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_LIGHTNING;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_DRAIN)) && bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo) && bs->currentEnemy->client->ps.fd.forcePower > 10 && bs->currentEnemy->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
					useTheForce = 1;
					forceHostile = 1;
				}
			}
			else if (bs->cur_ps.fd.forceSide == FORCE_LIGHTSIDE)
			{ //try light side powers
				if (g_entities[bs->entitynum].health <= 70
					&& bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL) 
					&& level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] )
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && bs->cur_ps.fd.forceGripCripple &&
					 level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{ //absorb to get out
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && bs->cur_ps.electrifyTime >= level.time &&
					 level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{ //absorb lightning
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_TELEPATHY)) && bs->frame_Enemy_Len < MAX_TRICK_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TELEPATHY]][FP_TELEPATHY] && InFieldOfVision(bs->viewangles, 50, a_fo) && !(bs->currentEnemy->client->ps.fd.forcePowersActive & (1 << FP_SEE)))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TELEPATHY;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && g_entities[bs->client].health < 75 && bs->currentEnemy->client->ps.fd.forceSide == FORCE_DARKSIDE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PROTECT)) && g_entities[bs->client].health < 35 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PROTECT]][FP_PROTECT])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_PROTECT;
					useTheForce = 1;
					forceHostile = 0;
				}
			}	
		}

		if (!useTheForce)
		{ //try neutral powers
			if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH)) && bs->cur_ps.fd.forceGripBeingGripped > level.time && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH] && InFieldOfVision(bs->viewangles, 50, a_fo))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
				useTheForce = 1;
				forceHostile = 1;
			}
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_SPEED)) && g_entities[bs->client].health < 25 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_SPEED]][FP_SPEED])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SPEED;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_SEE)) && BotMindTricked(bs->client, bs->currentEnemy->s.number) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_SEE]][FP_SEE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SEE;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PULL)) && bs->frame_Enemy_Len < 256 && level.clients[bs->client].ps.fd.forcePower > 75 && InFieldOfVision(bs->viewangles, 50, a_fo))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PULL;
				useTheForce = 1;
				forceHostile = 1;
			}
		}
	}

	if (!useTheForce)
	{ //try powers that we don't care if we have an enemy for
		if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL)) && g_entities[bs->client].health < 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] && bs->cur_ps.fd.forcePowerLevel[FP_HEAL] > FORCE_LEVEL_1)
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
			useTheForce = 1;
			forceHostile = 0;
		}
		else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL)) && g_entities[bs->client].health < 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] && !bs->currentEnemy && bs->isCamping > level.time)
		{ //only meditate and heal if we're camping
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
			useTheForce = 1;
			forceHostile = 0;
		}
	}

	if (useTheForce && forceHostile)
	{
		if (bs->currentEnemy && bs->currentEnemy->client &&
			!ForcePowerUsableOn(&g_entities[bs->client], bs->currentEnemy, level.clients[bs->client].ps.fd.forcePowerSelected))
		{
			useTheForce = 0;
			forceHostile = 0;
		}
	}

	doingFallback = 0;

	bs->deathActivitiesDone = 0;

	if (BotUseInventoryItem(bs))
	{
		if (rand()%10 < 5)
		{
			trap_EA_Use(bs->client);
		}
	}

	if (bs->cur_ps.ammo[weaponData[bs->cur_ps.weapon].ammoIndex] < weaponData[bs->cur_ps.weapon].energyPerShot)
	{
		if (BotTryAnotherWeapon(bs))
		{
			return;
		}
	}
	else
	{
		if (bs->currentEnemy && bs->lastVisibleEnemyIndex == bs->currentEnemy->s.number &&
			bs->frame_Enemy_Vis && bs->forceWeaponSelect /*&& bs->plantContinue < level.time*/)
		{
			bs->forceWeaponSelect = 0;
		}

		if (bs->plantContinue > level.time)
		{
			bs->doAttack = 1;
			bs->destinationGrabTime = 0;
		}

		if (!bs->forceWeaponSelect && bs->cur_ps.hasDetPackPlanted && bs->plantKillEmAll > level.time)
		{
			bs->forceWeaponSelect = WP_DET_PACK;
		}

		if (bs->forceWeaponSelect)
		{
			selResult = BotSelectChoiceWeapon(bs, bs->forceWeaponSelect, 1);
		}

		if (selResult)
		{
			if (selResult == 2)
			{ //newly selected
				return;
			}
		}
		else if (BotSelectIdealWeapon(bs))
		{
			return;
		}
	}
	/*if (BotSelectMelee(bs))
	{
		return;
	}*/

	reaction = bs->skills.reflex/bs->settings.skill;

	if (reaction < 0)
	{
		reaction = 0;
	}
	if (reaction > 2000)
	{
		reaction = 2000;
	}

	if (!bs->currentEnemy)
	{
		bs->timeToReact = level.time + reaction;
	}

	if (bs->cur_ps.weapon == WP_DET_PACK && bs->cur_ps.hasDetPackPlanted && bs->plantKillEmAll > level.time)
	{
		bs->doAltAttack = 1;
	}

	if (bs->wpCamping)
	{
		if (bs->isCamping < level.time)
		{
			bs->wpCamping = NULL;
			bs->isCamping = 0;
		}

		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->wpCamping = NULL;
			bs->isCamping = 0;
		}
	}

	if (bs->wpCurrent &&
		(bs->wpSeenTime < level.time || bs->wpTravelTime < level.time))
	{
		bs->wpCurrent = NULL;
	}

	//[NewGameTypes][EnhancedImpliment]
	/*
	if (g_gametype.integer == GT_SCENARIO)
	{
		if (!bs->isCamping)
		{
			int captureNum = g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CAPTURE_ENTITYNUM];
			
			if (captureNum > 1)
			{
				gentity_t *flag = &g_entities[captureNum];

				if (!flag)
				{

				}
				else if (!bs->wpCurrent)
				{

				}
				else if (flag->classname != "flag" || flag->s.teamowner != &g_entities[bs->cur_ps.clientNum].s.teamowner && flag->s.time2 < 100)
				{// Check for capturing in scenario game...
					if (bs->wpCurrent->index+1 < gWPNum)
					{
						if (VectorDistance(bs->wpCurrent->origin, flag->s.origin) < VectorDistance(gWPArray[bs->wpCurrent->index+1]->origin, flag->s.origin))
							isCapturing = qtrue;//bs->isCamping = level.time + 5000; // Capturing a flag...
					}
				}
			}
		}
	}
	*/
	//[/NewGameTypes][EnhancedImpliment]

	if (bs->currentEnemy)
	{
		if (bs->enemySeenTime < level.time ||
			!PassStandardEnemyChecks(bs, bs->currentEnemy))
		{
			if (bs->revengeEnemy == bs->currentEnemy &&
				bs->currentEnemy->health < 1 &&
				bs->lastAttacked && bs->lastAttacked == bs->currentEnemy)
			{
				//CHAT: Destroyed hated one [KilledHatedOne section]
				bs->chatObject = bs->revengeEnemy;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "KilledHatedOne", 1);
				bs->revengeEnemy = NULL;
				bs->revengeHateLevel = 0;
			}
			else if (bs->currentEnemy->health < 1 && PassLovedOneCheck(bs, bs->currentEnemy) &&
				bs->lastAttacked && bs->lastAttacked == bs->currentEnemy)
			{
				//CHAT: Killed
				bs->chatObject = bs->currentEnemy;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "Killed", 0);
			}

			bs->currentEnemy = NULL;
		}
	}

	if (bot_honorableduelacceptance.integer)
	{
		if (bs->currentEnemy && bs->currentEnemy->client &&
			bs->cur_ps.weapon == WP_SABER &&
			g_privateDuel.integer &&
			bs->frame_Enemy_Vis &&
			bs->frame_Enemy_Len < 400 &&
			bs->currentEnemy->client->ps.weapon == WP_SABER &&
			bs->currentEnemy->client->ps.saberHolstered)
		{
			vec3_t e_ang_vec;

			VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, e_ang_vec);

			if (InFieldOfVision(bs->viewangles, 100, e_ang_vec))
			{ //Our enemy has his saber holstered and has challenged us to a duel, so challenge him back
				if (!bs->cur_ps.saberHolstered)
				{
					Cmd_ToggleSaber_f(&g_entities[bs->client]);
				}
				else
				{
					if (bs->currentEnemy->client->ps.duelIndex == bs->client &&
						bs->currentEnemy->client->ps.duelTime > level.time &&
						!bs->cur_ps.duelInProgress)
					{
						Cmd_EngageDuel_f(&g_entities[bs->client]);
					}
				}

				bs->doAttack = 0;
				bs->doAltAttack = 0;
				bs->botChallengingTime = level.time + 100;
				bs->beStill = level.time + 100;
			}
		}
	}
	//Apparently this "allows you to cheese" when fighting against bots. I'm not sure why you'd want to con bots
	//into an easy kill, since they're bots and all. But whatever.

	if (!bs->wpCurrent)
	{
		wp = GetNearestVisibleWP(bs->origin, bs->client);

		if (wp != -1)
		{
			bs->wpCurrent = gWPArray[wp];
			bs->wpSeenTime = level.time + 1500;
			bs->wpTravelTime = level.time + 10000; //never take more than 10 seconds to travel to a waypoint
		}
	}

	if (bs->enemySeenTime < level.time || !bs->frame_Enemy_Vis || !bs->currentEnemy ||
		(bs->currentEnemy /*&& bs->cur_ps.weapon == WP_SABER && bs->frame_Enemy_Len > 300*/))
	{
		enemy = ScanForEnemies(bs);

		if (enemy != -1)
		{
			bs->currentEnemy = &g_entities[enemy];
			bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
		}
	}

	if (!bs->squadLeader && !bs->isSquadLeader)
	{
		BotScanForLeader(bs);
	}

	if (!bs->squadLeader && bs->squadCannotLead < level.time)
	{ //if still no leader after scanning, then become a squad leader
		bs->isSquadLeader = 1;
	}

	if (bs->isSquadLeader && bs->squadLeader)
	{ //we don't follow anyone if we are a leader
		bs->squadLeader = NULL;
	}

	//ESTABLISH VISIBILITIES AND DISTANCES FOR THE WHOLE FRAME HERE
	if (bs->wpCurrent)
	{
		if (g_RMG.integer)
		{ //this is somewhat hacky, but in RMG we don't really care about vertical placement because points are scattered across only the terrain.
			vec3_t vecB, vecC;

			vecB[0] = bs->origin[0];
			vecB[1] = bs->origin[1];
			vecB[2] = bs->origin[2];

			vecC[0] = bs->wpCurrent->origin[0];
			vecC[1] = bs->wpCurrent->origin[1];
			vecC[2] = vecB[2];


			VectorSubtract(vecC, vecB, a);
		}
		else
		{
			VectorSubtract(bs->wpCurrent->origin, bs->origin, a);
		}
		bs->frame_Waypoint_Len = VectorLength(a);

		visResult = WPOrgVisible(&g_entities[bs->client], bs->origin, bs->wpCurrent->origin, bs->client);

		if (visResult == 2)
		{
			bs->frame_Waypoint_Vis = 0;
			bs->wpSeenTime = 0;
			bs->wpDestination = NULL;
			bs->wpDestIgnoreTime = level.time + 5000;

			if (bs->wpDirection)
			{
				bs->wpDirection = 0;
			}
			else
			{
				bs->wpDirection = 1;
			}
		}
		else if (visResult)
		{
			bs->frame_Waypoint_Vis = 1;
		}
		else
		{
			bs->frame_Waypoint_Vis = 0;
		}
	}

	if (bs->currentEnemy)
	{
		if (bs->currentEnemy->client)
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, eorg);
			eorg[2] += bs->currentEnemy->client->ps.viewheight;
		}
		else
		{
			VectorCopy(bs->currentEnemy->s.origin, eorg);
		}

		VectorSubtract(eorg, bs->eye, a);
		bs->frame_Enemy_Len = VectorLength(a);

		if (OrgVisible(bs->eye, eorg, bs->client))
		{
			bs->frame_Enemy_Vis = 1;
			VectorCopy(eorg, bs->lastEnemySpotted);
			VectorCopy(bs->origin, bs->hereWhenSpotted);
			bs->lastVisibleEnemyIndex = bs->currentEnemy->s.number;
			//VectorCopy(bs->eye, bs->lastEnemySpotted);
			bs->hitSpotted = 0;
		}
		else
		{
			bs->frame_Enemy_Vis = 0;
		}
	}
	else
	{
		bs->lastVisibleEnemyIndex = ENTITYNUM_NONE;
	}
	//END

	if (bs->frame_Enemy_Vis)
	{
		bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
	}
	
	//[PlayerClasses][EnhancedImpliment]
	/*
	if (mod_classes.integer <= 0 && next_taunt[bs->entitynum] < level.time && bs->currentEnemy && bs->entitynum < MAX_CLIENTS)
	{// AIMod - BOT Taunting.
		G_BotVoiceEvent( &g_entities[bs->entitynum] );
		next_taunt[bs->entitynum] = level.time + 10000 + Q_irand(0, 95000); // Every 10->45 secs per bot.
	}
	*/
	//[/PlayerClasses][EnhancedImpliment]

	if (isCapturing)
	{// Dont move while capturing a flag...

	}
	else if (bs->wpCurrent && gWPArray /*&& !(bs->currentEnemy && bs->frame_Enemy_Vis)*/)
	{
		int wpTouchDist = BOT_WPTOUCH_DISTANCE;
		WPConstantRoutine(bs);

		if (!bs->wpCurrent)
		{ //WPConstantRoutine has the ability to nullify the waypoint if it fails certain checks, so..
			return;
		}

		if (bs->wpCurrent->flags & WPFLAG_WAITFORFUNC)
		{
			if (!CheckForFunc(bs->wpCurrent->origin, -1))
			{
				bs->beStill = level.time + 500; //no func brush under.. wait
			}
		}
		if (bs->wpCurrent->flags & WPFLAG_NOMOVEFUNC)
		{
			if (CheckForFunc(bs->wpCurrent->origin, -1))
			{
				bs->beStill = level.time + 500; //func brush under.. wait
			}
		}

		if (bs->frame_Waypoint_Vis || (bs->wpCurrent->flags & WPFLAG_NOVIS))
		{
			if (g_RMG.integer)
			{
				bs->wpSeenTime = level.time + 5000; //if we lose sight of the point, we have 1.5 seconds to regain it before we drop it
			}
			else
			{
				bs->wpSeenTime = level.time + 1500; //if we lose sight of the point, we have 1.5 seconds to regain it before we drop it
			}
		}
		VectorCopy(bs->wpCurrent->origin, bs->goalPosition);
		if (bs->wpDirection)
		{
			goalWPIndex = bs->wpCurrent->index-1;
		}
		else
		{
			goalWPIndex = bs->wpCurrent->index+1;
		}

		if (bs->wpCamping)
		{
			VectorSubtract(bs->wpCampingTo->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);

			VectorSubtract(bs->origin, bs->wpCamping->origin, a);
			if (VectorLength(a) < 64)
			{
				VectorCopy(bs->wpCamping->origin, bs->goalPosition);
				bs->beStill = level.time + 1000;

				if (!bs->campStanding)
				{
					bs->duckTime = level.time + 1000;
				}
			}
		}
		else if (gWPArray[goalWPIndex] && gWPArray[goalWPIndex]->inuse &&
			!(gLevelFlags & LEVELFLAG_NOPOINTPREDICTION))
		{
			VectorSubtract(gWPArray[goalWPIndex]->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);
		}
		else
		{
			VectorSubtract(bs->wpCurrent->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);
		}

		if (bs->destinationGrabTime < level.time /*&& (!bs->wpDestination || (bs->currentEnemy && bs->frame_Enemy_Vis))*/)
		{
			GetIdealDestination(bs);
		}
		
		if (bs->wpCurrent && bs->wpDestination)
		{
			if (TotalTrailDistance(bs->wpCurrent->index, bs->wpDestination->index, bs) == -1)
			{
				bs->wpDestination = NULL;
				bs->destinationGrabTime = level.time + 10000;
			}
		}

		if (g_RMG.integer)
		{
			if (bs->frame_Waypoint_Vis)
			{
				if (bs->wpCurrent && !bs->wpCurrent->flags)
				{
					wpTouchDist *= 3;
				}
			}
		}

		if (bs->frame_Waypoint_Len < wpTouchDist || (g_RMG.integer && bs->frame_Waypoint_Len < wpTouchDist*2))
		{
			WPTouchRoutine(bs);

			if (!bs->wpDirection)
			{
				desiredIndex = bs->wpCurrent->index+1;
			}
			else
			{
				desiredIndex = bs->wpCurrent->index-1;
			}

			if (gWPArray[desiredIndex] &&
				gWPArray[desiredIndex]->inuse &&
				desiredIndex < gWPNum &&
				desiredIndex >= 0 &&
				PassWayCheck(bs, desiredIndex))
			{
				bs->wpCurrent = gWPArray[desiredIndex];
			}
			else
			{
				if (bs->wpDestination)
				{
					bs->wpDestination = NULL;
					bs->destinationGrabTime = level.time + 10000;
				}

				if (bs->wpDirection)
				{
					bs->wpDirection = 0;
				}
				else
				{
					bs->wpDirection = 1;
				}
			}
		}
	}
	else //We can't find a waypoint, going to need a fallback routine.
	{
		/*if (g_gametype.integer == GT_DUEL)*/
		{ //helps them get out of messy situations
			/*if ((level.time - bs->forceJumpChargeTime) > 3500)
			{
				bs->forceJumpChargeTime = level.time + 2000;
				trap_EA_MoveForward(bs->client);
			}
			*/
			/*bs->jumpTime = level.time + 1500;
			bs->jumpHoldTime = level.time + 1500;
			bs->jDelay = 0;*/
		}
		doingFallback = AOTC_BotFallbackNavigation(bs);
	}

	if (bs->currentEnemy && bs->entitynum < MAX_CLIENTS)
	{
		if (g_entities[bs->entitynum].health <= 0 || bs->currentEnemy->health <= 0 )
		{

		}
		//[PlayerClasses][EnhancedImpliment]
		/*
		else if ( mod_classes.integer == 2 
		&& bs->settings.team == TEAM_RED
		&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_JEDI
		&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_FORCEMASTER )
		{// No jumping for other red team players...

		}
		*/
		//[/PlayerClasses][EnhancedImpliment]
		else
		{
			vec3_t p1, p2, dir;
			float xy;
			qboolean willFall = qfalse;

			bs->goalAngles[PITCH] = 0;
			bs->goalAngles[ROLL] = 0;

			VectorCopy(bs->goalAngles, b_angle);
	
			AngleVectors(b_angle, fwd, NULL, NULL);

			/*if (bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& CheckFall_By_Vectors(bs->cur_ps.origin, fwd, &g_entities[bs->entitynum]) == qtrue) // We're gonna fall!!!
			{
				if (bs->wpCurrent)
				{// Fall back to the old waypoint info... Already set above.		

				}
				else
				{
					bs->beStill = level.time + 1000;
				}
			}
			else */if (!gWPArray && bs->BOTjumpState <= JS_WAITING) // Not in a jump.
			{
				VectorCopy(bs->currentEnemy->r.currentOrigin, trto);
			}

			bs->goalAngles[PITCH] = 0;
			bs->goalAngles[ROLL] = 0;

			VectorCopy(bs->goalAngles, b_angle);
	
			AngleVectors(b_angle, fwd, NULL, NULL);

			if (CheckFall_By_Vectors(bs->origin, fwd, &g_entities[bs->entitynum]) == qtrue)
				willFall = qtrue;

			if ( bs->origin[2] > bs->currentEnemy->r.currentOrigin[2] )
			{
				VectorCopy( bs->origin, p1 );
				VectorCopy( bs->currentEnemy->r.currentOrigin, p2 );
			}
			else if ( bs->origin[2] < bs->currentEnemy->r.currentOrigin[2] )
			{
				VectorCopy( bs->currentEnemy->r.currentOrigin, p1 );
				VectorCopy( bs->origin, p2 );
			}
			else
			{
				VectorCopy( bs->origin, p1 );
				VectorCopy( bs->currentEnemy->r.currentOrigin, p2 );
			}

			VectorSubtract( p2, p1, dir );
			dir[2] = 0;

			//Get xy and z diffs
			xy = VectorNormalize( dir );

			// Jumping Stuff.
			if (bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& xy < 300 
				&& bs->currentEnemy->r.currentOrigin[2] > bs->origin[2] + 32
				&& bs->currentEnemy->health > 0
				&& bs->currentEnemy->client
				&& bs->currentEnemy->client->ps.groundEntityNum	!= ENTITYNUM_NONE /*!Falling(bs->currentEnemy)*/)
			{// Jump to enemy. NPC style! They are above us. Do a Jump.
				bs->BOTjumpState = JS_FACING;
				AIMod_Jump ( bs );
				
				VectorCopy(bs->currentEnemy->r.currentOrigin, jumpPos[bs->cur_ps.clientNum]);
			}
			else if (bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& xy > 700 
				&& bs->currentEnemy->r.currentOrigin[2] < bs->origin[2] - 32
				&& bs->currentEnemy->health > 0
				&& bs->currentEnemy->client
				&& bs->currentEnemy->client->ps.groundEntityNum	!= ENTITYNUM_NONE /*&& !Falling(bs->currentEnemy)*/)
			{// Jump to enemy. NPC style! They are below us.
				bs->BOTjumpState = JS_FACING;
				AIMod_Jump ( bs );
				
				VectorCopy(bs->currentEnemy->r.currentOrigin, jumpPos[bs->cur_ps.clientNum]);
			}
			else if (bs->BOTjumpState >= JS_CROUCHING)
			{// Continue any jumps.
				AIMod_Jump ( bs );
			}
		
			if (trto)
			{// We have a possibility.. Check it and maybe use it.
				VectorSubtract(trto, bs->origin, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);

				if (bs->jumpTime > level.time // In a jump.
					/*|| CheckFall_By_Vectors(bs->origin, ang, &g_entities[bs->entitynum]) == qfalse*/|| willFall) // or we wouldn't fall!
				{
					VectorCopy(trto, bs->goalPosition);			

					VectorSubtract(bs->goalPosition, bs->origin, a);
					vectoangles(a, ang);
					VectorCopy(ang, bs->goalAngles);
				}
				/*else // We would fall. Stay still.
				{
					if (bs->wpCurrent)
					{
						//int wp = GetSharedVisibleWP(&g_entities[bs->cur_ps.clientNum], bs->currentEnemy);
						//bs->wpCurrent = gWPArray[wp];
					}
					else
					{
						VectorCopy(bs->origin, bs->goalPosition);
						bs->beStill = level.time + 1000; //nowhere to go.
					}
				}*/
			}
			else
			{// No location found.
				if (bs->wpCurrent)
				{
					//int wp = GetSharedVisibleWP(&g_entities[bs->cur_ps.clientNum], bs->currentEnemy);
					//bs->wpCurrent = gWPArray[wp];
				}
				//[PlayerClasses][EnhancedImpliment]
				/*
				else if ( mod_classes.integer == 2 
					&& bs->settings.team == TEAM_RED
					&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_JEDI
					&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_FORCEMASTER )
				{// No jumping for other red team players...

				}
				*/
				//[/PlayerClasses][EnhancedImpliment]
				else if (bs->jumpTime > level.time) // In a jump.
				{// We could be in jump.. Head to safepos if we have one.
					if (safePos[bs->cur_ps.clientNum])
					{
						VectorCopy(safePos[bs->cur_ps.clientNum], bs->goalPosition);
						VectorSubtract(bs->goalPosition, bs->origin, a);
						vectoangles(a, ang);
						VectorCopy(ang, bs->goalAngles);
					}
					else
					{
						VectorCopy(bs->origin, bs->goalPosition);
						bs->beStill = level.time + 1000; //nowhere to go.
					}
				}
				else
				{
					VectorCopy(bs->origin, bs->goalPosition);
					bs->beStill = level.time + 1000; //nowhere to go.
				}
			}
		}
	}
	//end of new aimod ai

	if (bs->currentEnemy && bs->BOTjumpState < JS_CROUCHING) // Force bots to walk while they have an enemy visible... Unique1
		g_entities[bs->cur_ps.clientNum].client->pers.cmd.buttons |= BUTTON_WALKING;

	if (g_RMG.integer)
	{ //for RMG if the bot sticks around an area too long, jump around randomly some to spread to a new area (horrible hacky method)
		vec3_t vSubDif;

		VectorSubtract(bs->origin, bs->lastSignificantAreaChange, vSubDif);
		if (VectorLength(vSubDif) > 1500)
		{
			VectorCopy(bs->origin, bs->lastSignificantAreaChange);
			bs->lastSignificantChangeTime = level.time + 20000;
		}

		if (bs->lastSignificantChangeTime < level.time)
		{
			bs->iHaveNoIdeaWhereIAmGoing = level.time + 17000;
		}
	}

	if (bs->iHaveNoIdeaWhereIAmGoing > level.time && !bs->currentEnemy)
	{
		VectorCopy(preFrameGAngles, bs->goalAngles);
		bs->wpCurrent = NULL;
		bs->wpSwitchTime = level.time + 150;
		doingFallback = AOTC_BotFallbackNavigation(bs);
		bs->jumpTime = level.time + 150;
		bs->jumpHoldTime = level.time + 150;
		bs->jDelay = 0;
		bs->lastSignificantChangeTime = level.time + 25000;
	}

	if (isCapturing)
	{

	}
	else if (bs->wpCurrent && g_RMG.integer)
	{
		qboolean doJ = qfalse;

		if (bs->wpCurrent->origin[2]-192 > bs->origin[2])
		{
			doJ = qtrue;
		}
		else if ((bs->wpTravelTime - level.time) < 5000 && bs->wpCurrent->origin[2]-64 > bs->origin[2])
		{
			doJ = qtrue;
		}
		else if ((bs->wpTravelTime - level.time) < 7000 && (bs->wpCurrent->flags & WPFLAG_RED_FLAG))
		{
			if ((level.time - bs->jumpTime) > 200)
			{
				bs->jumpTime = level.time + 100;
				bs->jumpHoldTime = level.time + 100;
				bs->jDelay = 0;
			}
		}
		else if ((bs->wpTravelTime - level.time) < 7000 && (bs->wpCurrent->flags & WPFLAG_BLUE_FLAG))
		{
			if ((level.time - bs->jumpTime) > 200)
			{
				bs->jumpTime = level.time + 100;
				bs->jumpHoldTime = level.time + 100;
				bs->jDelay = 0;
			}
		}
		else if (bs->wpCurrent->index > 0)
		{
			if ((bs->wpTravelTime - level.time) < 7000)
			{
				if ((gWPArray[bs->wpCurrent->index-1]->flags & WPFLAG_RED_FLAG) ||
					(gWPArray[bs->wpCurrent->index-1]->flags & WPFLAG_BLUE_FLAG))
				{
					if ((level.time - bs->jumpTime) > 200)
					{
						bs->jumpTime = level.time + 100;
						bs->jumpHoldTime = level.time + 100;
						bs->jDelay = 0;
					}
				}
			}
		}

		if (doJ)
		{
			bs->jumpTime = level.time + 1500;
			bs->jumpHoldTime = level.time + 1500;
			bs->jDelay = 0;
		}
	}

	if (doingFallback)
	{
		bs->doingFallback = qtrue;
	}
	else
	{
		bs->doingFallback = qfalse;
	}

	if (bs->timeToReact < level.time && bs->currentEnemy && bs->enemySeenTime > level.time + (ENEMY_FORGET_MS - (ENEMY_FORGET_MS*0.2)))
	{
		if (bs->frame_Enemy_Vis)
		{
			cBAI = CombatBotAI(bs, thinktime);
		}
		else if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT)
		{ //keep charging in case we see him again before we lose track of him
			bs->doAltAttack = 1;
		}
		else if (bs->cur_ps.weaponstate == WEAPON_CHARGING)
		{ //keep charging in case we see him again before we lose track of him
			bs->doAttack = 1;
		}

		if (bs->destinationGrabTime > level.time + 100)
		{
			bs->destinationGrabTime = level.time + 100; //assures that we will continue staying within a general area of where we want to be in a combat situation
		}

		if (bs->currentEnemy->client)
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, headlevel);
			headlevel[2] += bs->currentEnemy->client->ps.viewheight;
		}
		else
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, headlevel);
		}

		if (!bs->frame_Enemy_Vis)
		{
			//if (!bs->hitSpotted && VectorLength(a) > 256)
			if (OrgVisible(bs->eye, bs->lastEnemySpotted, -1))
			{
				VectorCopy(bs->lastEnemySpotted, headlevel);
				VectorSubtract(headlevel, bs->eye, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);

				if (bs->cur_ps.weapon == WP_FLECHETTE &&
					bs->cur_ps.weaponstate == WEAPON_READY &&
					bs->currentEnemy && bs->currentEnemy->client)
				{
					mLen = VectorLength(a) > 128;
					if (mLen > 128 && mLen < 1024)
					{
						VectorSubtract(bs->currentEnemy->client->ps.origin, bs->lastEnemySpotted, a);

						bs->frame_Enemy_Vis = qtrue;

						if (VectorLength(a) < 300)
						{
							bs->doAltAttack = 1;
						}
					}
				}
			}
		}
		else
		{
			bLeadAmount = BotWeaponCanLead(bs);
			if ((bs->skills.accuracy/bs->settings.skill) <= 8 &&
				bLeadAmount)
			{
				BotAimLeading(bs, headlevel, bLeadAmount);
			}
			else
			{
				VectorSubtract(headlevel, bs->eye, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);
			}

			BotAimOffsetGoalAngles(bs);
		}
	}

	if (bs->cur_ps.saberInFlight)
	{
		bs->saberThrowTime = level.time + Q_irand(4000, 10000);
	}

	if (bs->currentEnemy)
	{
		if (BotGetWeaponRange(bs) == BWEAPONRANGE_SABER)
		{
			int saberRange = SABER_ATTACK_RANGE;

			VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, a_fo);
			vectoangles(a_fo, a_fo);

			if (bs->saberPowerTime < level.time)
			{ //Don't just use strong attacks constantly, switch around a bit
				if (Q_irand(1, 10) <= 5)
				{
					bs->saberPower = qtrue;
				}
				else
				{
					bs->saberPower = qfalse;
				}

				bs->saberPowerTime = level.time + Q_irand(3000, 15000);
			}

/*			if ( g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_STAFF
				&& g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_DUAL )
			{
				if (bs->currentEnemy->health > 75 
					&& g_entities[bs->client].client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] > 2)
				{
					if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_STRONG 
						&& bs->saberPower)
					{ //if we are up against someone with a lot of health and we have a strong attack available, then h4q them
						Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
					}
				}
				else if (bs->currentEnemy->health > 40 
					&& g_entities[bs->client].client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] > 1)
				{
					if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_MEDIUM)
					{ //they're down on health a little, use level 2 if we can
						Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
					}
				}
				else
				{
					if (g_entities[bs->client].client->ps.fd.saberAnimLevel != SS_FAST)
					{ //they've gone below 40 health, go at them with quick attacks
						Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
					}
				}
			}*/
			//[NewWeapons][EnhancedImpliment]
			//if ( (bs->cur_ps.weapon == WP_SABER || BG_Is_Staff_Weapon(bs->cur_ps.weapon))
			if ( bs->cur_ps.weapon == WP_SABER
			//[/NewWeapons][EnhancedImpliment]
				&& Q_irand(0,30000) < 2 ) 
			{// Switch stance every so often...
				Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
			}
			//[NewGameTypes][EnhancedImpliment]
			/*
			if (g_gametype.integer == GT_COOP)
			{
				saberRange *= 3;
			}
			*/
			//[/NewGameTypes][EnhancedImpliment]

			if (bs->frame_Enemy_Len <= saberRange)
			{
				SaberCombatHandling(bs);

				if (bs->frame_Enemy_Len < 80)
				{
					meleestrafe = 1;
				}
			}
			else if (bs->saberThrowTime < level.time && !bs->cur_ps.saberInFlight &&
				(bs->cur_ps.fd.forcePowersKnown & (1 << FP_SABERTHROW)) &&
				InFieldOfVision(bs->viewangles, 30, a_fo) &&
				bs->frame_Enemy_Len < BOT_SABER_THROW_RANGE &&
				bs->cur_ps.fd.saberAnimLevel != SS_STAFF)
			{
				bs->doAltAttack = 1;
				bs->doAttack = 0;
			}
			else if (bs->cur_ps.saberInFlight && bs->frame_Enemy_Len > 300 && bs->frame_Enemy_Len < BOT_SABER_THROW_RANGE)
			{
				bs->doAltAttack = 1;
				bs->doAttack = 0;
			}
		}
		else if (BotGetWeaponRange(bs) == BWEAPONRANGE_MELEE)
		{
			if (bs->frame_Enemy_Len <= MELEE_ATTACK_RANGE)
			{
				MeleeCombatHandling(bs);
				meleestrafe = 1;
			}
		}
	}

	if (doingFallback && bs->currentEnemy
		&& g_entities[bs->entitynum].client->ps.weapon != WP_SABER) //just stand and fire if we have no idea where we are
	{
		VectorCopy(bs->origin, bs->goalPosition);
	}

	//[PlayerClasses][EnhancedImpliment]
	/*
	if ( mod_classes.integer == 2 
		&& bs->settings.team == TEAM_RED
		&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_JEDI
		&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_FORCEMASTER )
	{// No jumping for other red team players...

	}
	else if (bs->forceJumping > level.time)
	*/
	if (bs->forceJumping > level.time)
	//[/PlayerClasses][EnhancedImpliment]
	{
		VectorCopy(bs->origin, noz_x);
		VectorCopy(bs->goalPosition, noz_y);

		noz_x[2] = noz_y[2];

		VectorSubtract(noz_x, noz_y, noz_x);

		if (VectorLength(noz_x) < 32)
		{
			fjHalt = 1;
		}
	}

	if (bs->doChat && bs->chatTime > level.time && (!bs->currentEnemy || !bs->frame_Enemy_Vis))
	{
		return;
	}
	else if (bs->doChat && bs->currentEnemy && bs->frame_Enemy_Vis)
	{
		//bs->chatTime = level.time + bs->chatTime_stored;
		bs->doChat = 0; //do we want to keep the bot waiting to chat until after the enemy is gone?
		bs->chatTeam = 0;
	}
	else if (bs->doChat && bs->chatTime <= level.time)
	{
		if (bs->chatTeam)
		{
			trap_EA_SayTeam(bs->client, bs->currentChat);
			bs->chatTeam = 0;
		}
		else
		{
			trap_EA_Say(bs->client, bs->currentChat);
		}
		if (bs->doChat == 2)
		{
			BotReplyGreetings(bs);
		}
		bs->doChat = 0;
	}

	if (g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTY)
		CTFFlagMovement(bs);

	if (/*bs->wpDestination &&*/ bs->shootGoal &&
		/*bs->wpDestination->associated_entity == bs->shootGoal->s.number &&*/
		bs->shootGoal->health > 0 && bs->shootGoal->takedamage)
	{
		dif[0] = (bs->shootGoal->r.absmax[0]+bs->shootGoal->r.absmin[0])/2;
		dif[1] = (bs->shootGoal->r.absmax[1]+bs->shootGoal->r.absmin[1])/2;
		dif[2] = (bs->shootGoal->r.absmax[2]+bs->shootGoal->r.absmin[2])/2;

		if (!bs->currentEnemy || bs->frame_Enemy_Len > 256)
		{ //if someone is close then don't stop shooting them for this
			VectorSubtract(dif, bs->eye, a);
			vectoangles(a, a);
			VectorCopy(a, bs->goalAngles);

			if (InFieldOfVision(bs->viewangles, 30, a) &&
				EntityVisibleBox(bs->origin, NULL, NULL, dif, bs->client, bs->shootGoal->s.number))
			{
				bs->doAttack = 1;
			}
		}
	}

	if (bs->cur_ps.hasDetPackPlanted)
	{ //check if our enemy gets near it and detonate if he does
		BotCheckDetPacks(bs);
	}
	else if (bs->currentEnemy && bs->lastVisibleEnemyIndex == bs->currentEnemy->s.number && !bs->frame_Enemy_Vis && bs->plantTime < level.time &&
		!bs->doAttack && !bs->doAltAttack)
	{
		VectorSubtract(bs->origin, bs->hereWhenSpotted, a);

		if (bs->plantDecided > level.time || (bs->frame_Enemy_Len < BOT_PLANT_DISTANCE*2 && VectorLength(a) < BOT_PLANT_DISTANCE))
		{
			mineSelect = BotSelectChoiceWeapon(bs, WP_TRIP_MINE, 0);
			detSelect = BotSelectChoiceWeapon(bs, WP_DET_PACK, 0);
			if (bs->cur_ps.hasDetPackPlanted)
			{
				detSelect = 0;
			}

			if (bs->plantDecided > level.time && bs->forceWeaponSelect &&
				bs->cur_ps.weapon == bs->forceWeaponSelect)
			{
				bs->doAttack = 1;
				bs->plantDecided = 0;
				bs->plantTime = level.time + BOT_PLANT_INTERVAL;
				bs->plantContinue = level.time + 500;
				bs->beStill = level.time + 500;
			}
			else if (mineSelect || detSelect)
			{
				if (BotSurfaceNear(bs))
				{
					if (!mineSelect)
					{ //if no mines use detpacks, otherwise use mines
						mineSelect = WP_DET_PACK;
					}
					else
					{
						mineSelect = WP_TRIP_MINE;
					}

					detSelect = BotSelectChoiceWeapon(bs, mineSelect, 1);

					if (detSelect && detSelect != 2)
					{ //We have it and it is now our weapon
						bs->plantDecided = level.time + 1000;
						bs->forceWeaponSelect = mineSelect;
						return;
					}
					else if (detSelect == 2)
					{
						bs->forceWeaponSelect = mineSelect;
						return;
					}
				}
			}
		}
	}
	else if (bs->plantContinue < level.time)
	{
		bs->forceWeaponSelect = 0;
	}

	if (g_gametype.integer == GT_JEDIMASTER && !bs->cur_ps.isJediMaster && bs->jmState == -1 && gJMSaberEnt && gJMSaberEnt->inuse)
	{
		vec3_t saberLen;
		float fSaberLen = 0;

		VectorSubtract(bs->origin, gJMSaberEnt->r.currentOrigin, saberLen);
		fSaberLen = VectorLength(saberLen);

		if (fSaberLen < 256)
		{
			if (OrgVisible(bs->origin, gJMSaberEnt->r.currentOrigin, bs->client))
			{
				VectorCopy(gJMSaberEnt->r.currentOrigin, bs->goalPosition);
			}
		}
	}

	if (isCapturing)
	{

	}
	else if (bs->beStill < level.time && !WaitingForNow(bs, bs->goalPosition) && !fjHalt)
	{
		VectorSubtract(bs->goalPosition, bs->origin, bs->goalMovedir);
		VectorNormalize(bs->goalMovedir);

		if (bs->jumpTime > level.time && bs->jDelay < level.time &&
			level.clients[bs->client].pers.cmd.upmove > 0)
		{
		//	trap_EA_Move(bs->client, bs->origin, 5000);
			bs->beStill = level.time + 200;
		}
		else
		{
			if (bs->currentEnemy)
				trap_EA_Move(bs->client, bs->goalMovedir, 2500);
			else
				trap_EA_Move(bs->client, bs->goalMovedir, 5000);
		}

		if (meleestrafe)
		{
			StrafeTracing(bs);
		}

		if (bs->meleeStrafeDir && meleestrafe && bs->meleeStrafeDisable < level.time)
		{
			trap_EA_MoveRight(bs->client);
		}
		else if (meleestrafe && bs->meleeStrafeDisable < level.time)
		{
			trap_EA_MoveLeft(bs->client);
		}

		if (BotTrace_Jump(bs, bs->goalPosition))
		{
			bs->jumpTime = level.time + 100;
		}
		else if (BotTrace_Duck(bs, bs->goalPosition))
		{
			bs->duckTime = level.time + 100;
		}
#ifdef BOT_STRAFE_AVOIDANCE
		else
		{
			int strafeAround = BotTrace_Strafe(bs, bs->goalPosition);

			if (strafeAround == STRAFEAROUND_RIGHT)
			{
				trap_EA_MoveRight(bs->client);
			}
			else if (strafeAround == STRAFEAROUND_LEFT)
			{
				trap_EA_MoveLeft(bs->client);
			}
		}
#endif
	}

#ifndef FORCEJUMP_INSTANTMETHOD
	if (bs->forceJumpChargeTime > level.time)
	{
		bs->jumpTime = 0;
	}
#endif

	if (bs->jumpPrep > level.time)
	{
		bs->forceJumpChargeTime = 0;
	}

	if (bs->forceJumpChargeTime > level.time)
	{
		bs->jumpHoldTime = ((bs->forceJumpChargeTime - level.time)/2) + level.time;
		bs->forceJumpChargeTime = 0;
	}

	if (bs->jumpHoldTime > level.time)
	{
		bs->jumpTime = bs->jumpHoldTime;
	}

    if (isCapturing)
	{

	}
	else if (bs->jumpTime > level.time && bs->jDelay < level.time)
	{
		//[PlayerClasses][EnhancedImpliment]
		/*
		gclient_t *client = g_entities[bs->cur_ps.clientNum].client;

		if ( g_gametype.integer == GT_COOP && classnumber[client->ps.clientNum] == CLASS_SOLDIER )
		{// Jumping for coop soldiers...
			if (client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
			{
				client->ps.eFlags |= EF_JETPACK;
			}
			else
			{
				client->ps.eFlags &= ~EF_JETPACK;
			}

			client->jetPackOn = qtrue;

			client->ps.pm_type = PM_JETPACK;
			client->ps.eFlags |= EF_JETPACK_ACTIVE;

			if (bs->jumpHoldTime > level.time)
			{
				if (bs->client > MAX_CLIENTS)
					g_entities[bs->client].client->ps.velocity[2] = 128;
				else
					trap_EA_Jump(bs->client);

				if (bs->wpCurrent)
				{
					if ((bs->wpCurrent->origin[2] - bs->origin[2]) < 64)
					{
						trap_EA_MoveForward(bs->client);
					}
				}
				else
				{
					trap_EA_MoveForward(bs->client);
				}
				if (g_entities[bs->client].client->ps.groundEntityNum == ENTITYNUM_NONE)
				{
					g_entities[bs->client].client->ps.pm_flags |= PMF_JUMP_HELD;
				}
			}
			else if (!(bs->cur_ps.pm_flags & PMF_JUMP_HELD))
			{
				if (bs->client > MAX_CLIENTS)
					g_entities[bs->client].client->ps.velocity[2] = 128;
				else
					trap_EA_Jump(bs->client);
			}
		}
		else if ( mod_classes.integer == 2 && bs->settings.team == TEAM_BLUE && g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] == GCLASS_SOLDIER )
		{
			if (client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
			{
				client->ps.eFlags |= EF_JETPACK;
			}
			else
			{
				client->ps.eFlags &= ~EF_JETPACK;
			}

			client->jetPackOn = qtrue;

			client->ps.pm_type = PM_JETPACK;
			client->ps.eFlags |= EF_JETPACK_ACTIVE;

			if (bs->jumpHoldTime > level.time)
			{
				if (bs->client > MAX_CLIENTS)
					g_entities[bs->client].client->ps.velocity[2] = 128;
				else
					trap_EA_Jump(bs->client);

				if (bs->wpCurrent)
				{
					if ((bs->wpCurrent->origin[2] - bs->origin[2]) < 64)
					{
						trap_EA_MoveForward(bs->client);
					}
				}
				else
				{
					trap_EA_MoveForward(bs->client);
				}
				if (g_entities[bs->client].client->ps.groundEntityNum == ENTITYNUM_NONE)
				{
					g_entities[bs->client].client->ps.pm_flags |= PMF_JUMP_HELD;
				}
			}
			else if (!(bs->cur_ps.pm_flags & PMF_JUMP_HELD))
			{
				if (bs->client > MAX_CLIENTS)
					g_entities[bs->client].client->ps.velocity[2] = 128;
				else
					trap_EA_Jump(bs->client);
			}
		}
		else
		*/
		//[/PlayerClasses][EnhancedImpliment]
		{
			if (bs->jumpHoldTime > level.time)
			{
				trap_EA_Jump(bs->client);
				if (bs->wpCurrent)
				{
					if ((bs->wpCurrent->origin[2] - bs->origin[2]) < 64)
					{
						trap_EA_MoveForward(bs->client);
					}
				}
				else
				{
					trap_EA_MoveForward(bs->client);
				}
				if (g_entities[bs->client].client->ps.groundEntityNum == ENTITYNUM_NONE)
				{
					g_entities[bs->client].client->ps.pm_flags |= PMF_JUMP_HELD;
				}
			}
			else if (!(bs->cur_ps.pm_flags & PMF_JUMP_HELD))
			{
				trap_EA_Jump(bs->client);
			}
		}
	}

	if (bs->duckTime > level.time)
	{
		trap_EA_Crouch(bs->client);
	}

	if ( bs->dangerousObject && bs->dangerousObject->inuse && bs->dangerousObject->health > 0 &&
		bs->dangerousObject->takedamage && (!bs->frame_Enemy_Vis || !bs->currentEnemy) &&
		(BotGetWeaponRange(bs) == BWEAPONRANGE_MID || BotGetWeaponRange(bs) == BWEAPONRANGE_LONG) &&
		//[NewWeapons][EnhancedImpliment]
		//bs->cur_ps.weapon != WP_DET_PACK && bs->cur_ps.weapon != WP_TRIP_MINE && bs->cur_ps.weapon != WP_TRIP_MINE_2 &&
		bs->cur_ps.weapon != WP_DET_PACK && bs->cur_ps.weapon != WP_TRIP_MINE &&
		//[/NewWeapons][EnhancedImpliment]
		!bs->shootGoal )
	{
		float danLen;

		VectorSubtract(bs->dangerousObject->r.currentOrigin, bs->eye, a);

		danLen = VectorLength(a);

		if (danLen > 256)
		{
			vectoangles(a, a);
			VectorCopy(a, bs->goalAngles);

			if (Q_irand(1, 10) < 5)
			{
				bs->goalAngles[YAW] += Q_irand(0, 3);
				bs->goalAngles[PITCH] += Q_irand(0, 3);
			}
			else
			{
				bs->goalAngles[YAW] -= Q_irand(0, 3);
				bs->goalAngles[PITCH] -= Q_irand(0, 3);
			}

			if (InFieldOfVision(bs->viewangles, 30, a) &&
				EntityVisibleBox(bs->origin, NULL, NULL, bs->dangerousObject->r.currentOrigin, bs->client, bs->dangerousObject->s.number))
			{
				bs->doAttack = 1;
			}			
		}
	}

	if (PrimFiring(bs) ||
		AltFiring(bs))
	{
		friendInLOF = CheckForFriendInLOF(bs);

		if (friendInLOF)
		{
			if (PrimFiring(bs))
			{
				KeepPrimFromFiring(bs);
			}
			if (AltFiring(bs))
			{
				KeepAltFromFiring(bs);
			}
			if (useTheForce && forceHostile)
			{
				useTheForce = 0;
			}

			if (!useTheForce && friendInLOF->client)
			{ //we have a friend here and are not currently using force powers, see if we can help them out
				if (friendInLOF->health <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_HEAL;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if (friendInLOF->client->ps.fd.forcePower <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_FORCE;
					useTheForce = 1;
					forceHostile = 0;
				}
			}
		}
	}
	//[NewGameTypes][EnhancedImpliment]
	//else if (g_gametype.integer >= GT_TEAM && g_gametype.integer != GT_RPG)
	else if (g_gametype.integer >= GT_TEAM)
	//[/NewGameTypes][EnhancedImpliment]
	{ //still check for anyone to help..
		friendInLOF = CheckForFriendInLOF(bs);

		if (!useTheForce && friendInLOF)
		{
			if (friendInLOF->health <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_HEAL;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if (friendInLOF->client->ps.fd.forcePower <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_FORCE;
				useTheForce = 1;
				forceHostile = 0;
			}
		}
	}

	if (bs->doAttack && bs->cur_ps.weapon == WP_DET_PACK &&
		bs->cur_ps.hasDetPackPlanted)
	{ //maybe a bit hackish, but bots only want to plant one of these at any given time to avoid complications
		bs->doAttack = 0;
	}

	if (bs->doAttack && bs->cur_ps.weapon == WP_SABER &&
		bs->saberDefending && bs->currentEnemy && bs->currentEnemy->client &&
		BotWeaponBlockable(bs->currentEnemy->client->ps.weapon) )
	{
		bs->doAttack = 0;
	}

	if (bs->cur_ps.saberLockTime > level.time)
	{
		if (rand()%10 < 5)
		{
			bs->doAttack = 1;
		}
		else
		{
			bs->doAttack = 0;
		}
	}

	if (bs->botChallengingTime > level.time)
	{
		bs->doAttack = 0;
		bs->doAltAttack = 0;
	}

	if (bs->cur_ps.weapon == WP_SABER &&
		bs->cur_ps.saberInFlight &&
		!bs->cur_ps.saberEntityNum)
	{ //saber knocked away, keep trying to get it back
		bs->doAttack = 1;
		bs->doAltAttack = 0;
	}

	if (bs->doAttack)
	{
		trap_EA_Attack(bs->client);
	}
	else if (bs->doAltAttack)
	{
		trap_EA_Alt_Attack(bs->client);
	}

	if (useTheForce && forceHostile && bs->botChallengingTime > level.time)
	{
		useTheForce = qfalse;
	}

	if (useTheForce)
	{
#ifndef FORCEJUMP_INSTANTMETHOD
		if (bs->forceJumpChargeTime > level.time)
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_LEVITATION;
			trap_EA_ForcePower(bs->client);
		}
		else
		{
#endif
			if (bot_forcepowers.integer && !g_forcePowerDisable.integer)
			{
				trap_EA_ForcePower(bs->client);
			}
#ifndef FORCEJUMP_INSTANTMETHOD
		}
#endif
	}

	MoveTowardIdealAngles(bs);
}


//[NewGameTypes][EnhancedImpliment]
/*
int scenario_wp_visible[MAX_CLIENTS]; // Caches waypoint visibility...
int scenario_wp_visible_checktime[MAX_CLIENTS]; // Next time for above.
qboolean scenario_enemy_visible[MAX_CLIENTS]; // Caches enemy visibility...
int scenario_enemy_visible_checktime[MAX_CLIENTS]; // Next time for above.

void ScenarioBotAI(bot_state_t *bs, float thinktime)
{
	int wp, enemy;
	int desiredIndex;
	int goalWPIndex;
	int doingFallback = 0;
	int fjHalt;
	vec3_t a, ang, headlevel, eorg, noz_x, noz_y, dif, a_fo;
	float reaction;
	float bLeadAmount;
	int meleestrafe = 0;
	int useTheForce = 0;
	int forceHostile = 0;
	int cBAI = 0;
	gentity_t *friendInLOF = 0;
	float mLen;
	int visResult = 0;
	int selResult = 0;
	int mineSelect = 0;
	int detSelect = 0;
	vec3_t preFrameGAngles;
	vec3_t	b_angle, fwd, trto; // AIMod
	qboolean isCapturing = qfalse;

	if (gDeactivated)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;
		return;
	}

	if (g_entities[bs->client].s.eType != ET_NPC &&
		g_entities[bs->client].inuse &&
		g_entities[bs->client].client &&
		g_entities[bs->client].client->sess.sessionTeam == TEAM_SPECTATOR)
	{
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpDirection = 0;
		return;
	}


#ifndef FINAL_BUILD
	if (bot_getinthecarrr.integer)
	{ //stupid vehicle debug, I tire of having to connect another client to test passengers.
		gentity_t *botEnt = &g_entities[bs->client];

		if (botEnt->inuse && botEnt->client && botEnt->client->ps.m_iVehicleNum)
		{ //in a vehicle, so...
			bs->noUseTime = level.time + 5000;

			if (bot_getinthecarrr.integer != 2)
			{
				trap_EA_MoveForward(bs->client);

				if (bot_getinthecarrr.integer == 3)
				{ //use alt fire
					trap_EA_Alt_Attack(bs->client);
				}
			}
		}
		else
		{ //find one, get in
			int i = 0;
			gentity_t *vehicle = NULL;
			//find the nearest, manned vehicle
			while (i < MAX_GENTITIES)
			{
				vehicle = &g_entities[i];

				if (vehicle->inuse && vehicle->client && vehicle->s.eType == ET_NPC &&
					vehicle->s.NPC_class == CLASS_VEHICLE && vehicle->m_pVehicle &&
					(vehicle->client->ps.m_iVehicleNum || bot_getinthecarrr.integer == 2))
				{ //ok, this is a vehicle, and it has a pilot/passengers
					break;
				}
				i++;
			}
			if (i != MAX_GENTITIES && vehicle)
			{ //broke before end so we must've found something
				vec3_t v;

				VectorSubtract(vehicle->client->ps.origin, bs->origin, v);
				VectorNormalize(v);
				vectoangles(v, bs->goalAngles);
				MoveTowardIdealAngles(bs);
				trap_EA_Move(bs->client, v, 5000.0f);

				if (bs->noUseTime < (level.time-400))
				{
					bs->noUseTime = level.time + 500;
				}
			}
		}

		return;
	}
#endif

*/
/*	{// Bots should use vehicles in scenario... So...
		int i = 0;
		gentity_t *vehicle = NULL;
		//find the nearest, manned vehicle
		while (i < MAX_GENTITIES)
		{
			vehicle = &g_entities[i];

			if (vehicle->inuse 
				&& vehicle->client 
				&& vehicle->s.eType == ET_NPC 
				&& vehicle->s.NPC_class == CLASS_VEHICLE 
				&& vehicle->m_pVehicle 
				&& !vehicle->client->ps.m_iVehicleNum
				&& VectorDistance(vehicle->r.currentOrigin, bs->cur_ps.origin) < 512 )
			{ //ok, this is a vehicle, and it has no pilot/passengers
				if (OrgVisible(vehicle->r.currentOrigin, bs->cur_ps.origin, bs->cur_ps.origin))
					break;
			}
			i++;
		}

		if (i != MAX_GENTITIES && vehicle)
		{ //broke before end so we must've found something
			vec3_t v;

			VectorSubtract(vehicle->client->ps.origin, bs->origin, v);
			VectorNormalize(v);
			vectoangles(v, bs->goalAngles);
			MoveTowardIdealAngles(bs);
			trap_EA_Move(bs->client, v, 5000.0f);

			if (bs->noUseTime < (level.time-400))
			{
				bs->noUseTime = level.time + 500;
			}
			return;
		}
	}
*/
/*

	if (!bs->lastDeadTime)
	{ //just spawned in?
		bs->lastDeadTime = level.time;
	}

	if (g_entities[bs->client].health < 1)
	{
		bs->lastDeadTime = level.time;

		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		bs->wpCamping = NULL;
		bs->wpCampingTo = NULL;
		bs->wpStoreDest = NULL;
		bs->wpDestIgnoreTime = 0;
		bs->wpDestSwitchTime = 0;
		bs->wpSeenTime = 0;
		bs->wpDirection = 0;

		if (rand()%10 < 5 &&
			(!bs->doChat || bs->chatTime < level.time))
		{
			trap_EA_Attack(bs->client);
		}

		return;
	}

	VectorCopy(bs->goalAngles, preFrameGAngles);

	bs->doAttack = 0;
	bs->doAltAttack = 0;
	//reset the attack states

	if (bs->isSquadLeader)
	{
		CommanderBotAI(bs);
	}
	else
	{
		BotDoTeamplayAI(bs);
	}

	if (!bs->currentEnemy)
	{
		bs->frame_Enemy_Vis = 0;
	}

	if (bs->currentEnemy && bs->currentEnemy->client &&
		bs->currentEnemy->client->pers.connected != CA_ACTIVE && bs->currentEnemy->client->pers.connected != CA_AUTHORIZING)
	{
		bs->currentEnemy = NULL;
	}

	fjHalt = 0;

	if (bs->currentEnemy && bs->currentEnemy->client && bs->frame_Enemy_Vis)
	{
		VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, a_fo);
		vectoangles(a_fo, a_fo);

		{
			//do this above all things
			if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH)) && (bs->doForcePush > level.time || bs->cur_ps.fd.forceGripBeingGripped > level.time) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
				useTheForce = 1;
				forceHostile = 1;
			}
			else if (bs->cur_ps.fd.forceSide == FORCE_DARKSIDE)
			{ //try dark side powers
			  //in order of priority top to bottom
				if (g_entities[bs->entitynum].health <= 75 
					&& (bs->cur_ps.fd.forcePowersKnown & (1 << FP_DRAIN)) && bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo) && bs->currentEnemy->client->ps.fd.forcePower > 10 )
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) && (bs->cur_ps.fd.forcePowersActive & (1 << FP_GRIP)) && InFieldOfVision(bs->viewangles, 50, a_fo))
				{ //already gripping someone, so hold it
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) && bs->frame_Enemy_Len < MAX_GRIP_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_GRIP]][FP_GRIP] && InFieldOfVision(bs->viewangles, 50, a_fo))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_RAGE)) && g_entities[bs->client].health < 25 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_RAGE]][FP_RAGE])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_RAGE;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_LIGHTNING)) && bs->frame_Enemy_Len < FORCE_LIGHTNING_RADIUS && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_LIGHTNING;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_DRAIN)) && bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo) && bs->currentEnemy->client->ps.fd.forcePower > 10 && bs->currentEnemy->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
					useTheForce = 1;
					forceHostile = 1;
				}
			}
			else if (bs->cur_ps.fd.forceSide == FORCE_LIGHTSIDE)
			{ //try light side powers
				if (g_entities[bs->entitynum].health <= 70
					&& bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL) 
					&& level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] )
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && bs->cur_ps.fd.forceGripCripple &&
					 level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{ //absorb to get out
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && bs->cur_ps.electrifyTime >= level.time &&
					 level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{ //absorb lightning
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_TELEPATHY)) && bs->frame_Enemy_Len < MAX_TRICK_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TELEPATHY]][FP_TELEPATHY] && InFieldOfVision(bs->viewangles, 50, a_fo) && !(bs->currentEnemy->client->ps.fd.forcePowersActive & (1 << FP_SEE)))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TELEPATHY;
					useTheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && g_entities[bs->client].health < 75 && bs->currentEnemy->client->ps.fd.forceSide == FORCE_DARKSIDE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PROTECT)) && g_entities[bs->client].health < 35 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PROTECT]][FP_PROTECT])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_PROTECT;
					useTheForce = 1;
					forceHostile = 0;
				}
			}	
		}

		if (!useTheForce)
		{ //try neutral powers
			if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH)) && bs->cur_ps.fd.forceGripBeingGripped > level.time && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH] && InFieldOfVision(bs->viewangles, 50, a_fo))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
				useTheForce = 1;
				forceHostile = 1;
			}
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_SPEED)) && g_entities[bs->client].health < 25 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_SPEED]][FP_SPEED])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SPEED;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_SEE)) && BotMindTricked(bs->client, bs->currentEnemy->s.number) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_SEE]][FP_SEE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SEE;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PULL)) && bs->frame_Enemy_Len < 256 && level.clients[bs->client].ps.fd.forcePower > 75 && InFieldOfVision(bs->viewangles, 50, a_fo))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PULL;
				useTheForce = 1;
				forceHostile = 1;
			}
		}
	}

	if (!useTheForce)
	{ //try powers that we don't care if we have an enemy for
		if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL)) && g_entities[bs->client].health < 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] && bs->cur_ps.fd.forcePowerLevel[FP_HEAL] > FORCE_LEVEL_1)
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
			useTheForce = 1;
			forceHostile = 0;
		}
		else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL)) && g_entities[bs->client].health < 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] && !bs->currentEnemy && bs->isCamping > level.time)
		{ //only meditate and heal if we're camping
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
			useTheForce = 1;
			forceHostile = 0;
		}
	}

	if (useTheForce && forceHostile)
	{
		if (bs->currentEnemy && bs->currentEnemy->client &&
			!ForcePowerUsableOn(&g_entities[bs->client], bs->currentEnemy, level.clients[bs->client].ps.fd.forcePowerSelected))
		{
			useTheForce = 0;
			forceHostile = 0;
		}
	}

	doingFallback = 0;

	bs->deathActivitiesDone = 0;

	if (Q_irand(0,400) < 2)
	{
		if (BotUseInventoryItem(bs))
		{
			trap_EA_Use(bs->client);
		}
	}

	if (bs->cur_ps.ammo[weaponData[bs->cur_ps.weapon].ammoIndex] < weaponData[bs->cur_ps.weapon].energyPerShot || bs->cur_ps.weapon == WP_NONE)
	{
		if (BotTryAnotherWeapon(bs))
		{
			return;
		}
	}
	else
	{
		if (bs->currentEnemy && bs->lastVisibleEnemyIndex == bs->currentEnemy->s.number &&
			bs->frame_Enemy_Vis && bs->forceWeaponSelect)
		{
			bs->forceWeaponSelect = 0;
		}

		if (bs->plantContinue > level.time)
		{
			bs->doAttack = 1;
			bs->destinationGrabTime = 0;
		}

		if (!bs->forceWeaponSelect && bs->cur_ps.hasDetPackPlanted && bs->plantKillEmAll > level.time)
		{
			bs->forceWeaponSelect = WP_DET_PACK;
		}

		if (bs->forceWeaponSelect)
		{
			selResult = BotSelectChoiceWeapon(bs, bs->forceWeaponSelect, 1);
		}

		if (selResult)
		{
			if (selResult == 2)
			{ //newly selected
				return;
			}
		}
		else if (BotSelectIdealWeapon(bs))
		{
			return;
		}
	}
	*//*if (BotSelectMelee(bs))
	{
		return;
	}*/
	/*
	reaction = bs->skills.reflex/bs->settings.skill;

	if (reaction < 0)
	{
		reaction = 0;
	}
	if (reaction > 1000)
	{
		reaction = 1000;
	}

	if (!bs->currentEnemy)
	{
		bs->timeToReact = level.time + reaction;
	}

	if (bs->cur_ps.weapon == WP_DET_PACK && bs->cur_ps.hasDetPackPlanted && bs->plantKillEmAll > level.time)
	{
		bs->doAltAttack = 1;
	}

	if (bs->wpCamping)
	{
		if (bs->isCamping < level.time)
		{
			bs->wpCamping = NULL;
			bs->isCamping = 0;
		}

		if (bs->currentEnemy && bs->frame_Enemy_Vis)
		{
			bs->wpCamping = NULL;
			bs->isCamping = 0;
		}
	}

	if (bs->wpCurrent &&
		(bs->wpSeenTime < level.time || bs->wpTravelTime < level.time))
	{
		bs->wpCurrent = NULL;
	}

	if (g_gametype.integer == GT_SCENARIO)
	{
		if (!bs->isCamping)
		{
			int captureNum = g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CAPTURE_ENTITYNUM];
			
			if (captureNum > 1)
			{
				gentity_t *flag = &g_entities[captureNum];

				if (!flag)
				{

				}
				else if (!bs->wpCurrent)
				{

				}
				else if (flag->classname != "flag" || flag->s.teamowner != &g_entities[bs->cur_ps.clientNum].s.teamowner && flag->s.time2 < 100)
				{// Check for capturing in scenario game...
					if (bs->wpCurrent->index+1 < gWPNum)
					{
						if (VectorDistance(bs->wpCurrent->origin, flag->s.origin) < VectorDistance(gWPArray[bs->wpCurrent->index+1]->origin, flag->s.origin))
							isCapturing = qtrue;//bs->isCamping = level.time + 5000; // Capturing a flag...
					}
				}
			}
		}
	}

	if (bs->currentEnemy)
	{
		if (bs->enemySeenTime < level.time ||
			!PassStandardEnemyChecks(bs, bs->currentEnemy))
		{
			bs->currentEnemy = NULL;
		}
	}

	//Apparently this "allows you to cheese" when fighting against bots. I'm not sure why you'd want to con bots
	//into an easy kill, since they're bots and all. But whatever.

	if (!bs->wpCurrent)
	{
		wp = GetNearestVisibleWP(bs->origin, bs->client);

		if (wp != -1)
		{
			bs->wpCurrent = gWPArray[wp];
			bs->wpSeenTime = level.time + 1500;
			bs->wpTravelTime = level.time + 10000; //never take more than 10 seconds to travel to a waypoint
		}
	}

	if (bs->enemySeenTime < level.time || !bs->frame_Enemy_Vis || !bs->currentEnemy ||
		(bs->currentEnemy *//*&& bs->cur_ps.weapon == WP_SABER))
	{
		enemy = ScanForEnemies(bs);

		if (enemy != -1)
		{
			bs->currentEnemy = &g_entities[enemy];
			bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
		}
	}

	if (!bs->squadLeader && !bs->isSquadLeader)
	{
		BotScanForLeader(bs);
	}

	if (!bs->squadLeader && bs->squadCannotLead < level.time)
	{ //if still no leader after scanning, then become a squad leader
		bs->isSquadLeader = 1;
	}

	if (bs->isSquadLeader && bs->squadLeader)
	{ //we don't follow anyone if we are a leader
		bs->squadLeader = NULL;
	}

	//ESTABLISH VISIBILITIES AND DISTANCES FOR THE WHOLE FRAME HERE
	if (bs->wpCurrent)
	{
		if (g_RMG.integer)
		{ //this is somewhat hacky, but in RMG we don't really care about vertical placement because points are scattered across only the terrain.
			vec3_t vecB, vecC;

			vecB[0] = bs->origin[0];
			vecB[1] = bs->origin[1];
			vecB[2] = bs->origin[2];

			vecC[0] = bs->wpCurrent->origin[0];
			vecC[1] = bs->wpCurrent->origin[1];
			vecC[2] = vecB[2];


			VectorSubtract(vecC, vecB, a);
		}
		else
		{
			VectorSubtract(bs->wpCurrent->origin, bs->origin, a);
		}
		bs->frame_Waypoint_Len = VectorLength(a);

		if (scenario_wp_visible_checktime[bs->entitynum] < level.time
			|| VectorDistance(bs->origin, bs->wpCurrent->origin) > 128 
			|| !last_bot_wp_vischeck_result[bs->entitynum] )
		{// Let's limit wp tracing a bit for speed in scenario...
			visResult = WPOrgVisible(&g_entities[bs->client], bs->origin, bs->wpCurrent->origin, bs->client);
			scenario_wp_visible_checktime[bs->entitynum] = level.time + bot_cpu_usage.integer;
			scenario_wp_visible[bs->entitynum] = visResult;
		}
		else
		{
			visResult = scenario_wp_visible[bs->entitynum];
		}

		if (visResult == 2)
		{
			bs->frame_Waypoint_Vis = 0;
			bs->wpSeenTime = 0;
			bs->wpDestination = NULL;
			bs->wpDestIgnoreTime = level.time + 5000;

			if (bs->wpDirection)
			{
				bs->wpDirection = 0;
			}
			else
			{
				bs->wpDirection = 1;
			}
		}
		else if (visResult)
		{
			bs->frame_Waypoint_Vis = 1;
		}
		else
		{
			bs->frame_Waypoint_Vis = 0;
		}
	}

	if (bs->wpCurrent && !bs->wpDestination)
	{// Find a good scenario flag pos to head for...
		int selectWP = GetBestIdleGoal(bs);

		if (selectWP >= 0)
			bs->wpDestination = gWPArray[selectWP];
	}

	if (bs->currentEnemy)
	{
		if (bs->currentEnemy->client)
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, eorg);
			eorg[2] += bs->currentEnemy->client->ps.viewheight;
		}
		else
		{
			VectorCopy(bs->currentEnemy->s.origin, eorg);
		}

		VectorSubtract(eorg, bs->eye, a);
		bs->frame_Enemy_Len = VectorLength(a);

		if (scenario_enemy_visible_checktime[bs->entitynum] < level.time)
		{
			if (OrgVisible(bs->eye, eorg, bs->client))
			{
				scenario_enemy_visible[bs->entitynum] = qtrue;
			}
			else
			{
				scenario_enemy_visible[bs->entitynum] = qfalse;
			}

			scenario_enemy_visible_checktime[bs->entitynum] = level.time + 100;
		}

		if (scenario_enemy_visible[bs->entitynum])
		{
			bs->frame_Enemy_Vis = 1;
			VectorCopy(eorg, bs->lastEnemySpotted);
			VectorCopy(bs->origin, bs->hereWhenSpotted);
			bs->lastVisibleEnemyIndex = bs->currentEnemy->s.number;
			//VectorCopy(bs->eye, bs->lastEnemySpotted);
			bs->hitSpotted = 0;
		}
		else
		{
			bs->frame_Enemy_Vis = 0;
		}
	}
	else
	{
		bs->lastVisibleEnemyIndex = ENTITYNUM_NONE;
	}
	//END

	if (bs->frame_Enemy_Vis)
	{
		bs->enemySeenTime = level.time + ENEMY_FORGET_MS;
	}

	if (isCapturing)
	{// Dont move while capturing a flag...

	}
	else if (bs->wpCurrent && gWPArray)
	{
		int wpTouchDist = BOT_WPTOUCH_DISTANCE;
		vec3_t botpos, wppos;
		float wpDist;

		WPConstantRoutine(bs);

		if (!bs->wpCurrent)
		{ //WPConstantRoutine has the ability to nullify the waypoint if it fails certain checks, so..
			return;
		}

		if (bs->wpCurrent->flags & WPFLAG_WAITFORFUNC)
		{
			if (!CheckForFunc(bs->wpCurrent->origin, -1))
			{
				bs->beStill = level.time + 500; //no func brush under.. wait
			}
		}
		if (bs->wpCurrent->flags & WPFLAG_NOMOVEFUNC)
		{
			if (CheckForFunc(bs->wpCurrent->origin, -1))
			{
				bs->beStill = level.time + 500; //func brush under.. wait
			}
		}

		if (bs->frame_Waypoint_Vis || (bs->wpCurrent->flags & WPFLAG_NOVIS))
		{
			if (g_RMG.integer)
			{
				bs->wpSeenTime = level.time + 5000; //if we lose sight of the point, we have 1.5 seconds to regain it before we drop it
			}
			else
			{
				bs->wpSeenTime = level.time + 1500; //if we lose sight of the point, we have 1.5 seconds to regain it before we drop it
			}
		}
		VectorCopy(bs->wpCurrent->origin, bs->goalPosition);
		if (bs->wpDirection)
		{
			goalWPIndex = bs->wpCurrent->index-1;
		}
		else
		{
			goalWPIndex = bs->wpCurrent->index+1;
		}

		if (bs->wpCamping)
		{
			VectorSubtract(bs->wpCampingTo->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);

			VectorSubtract(bs->origin, bs->wpCamping->origin, a);
			if (VectorLength(a) < 64)
			{
				VectorCopy(bs->wpCamping->origin, bs->goalPosition);
				bs->beStill = level.time + 1000;

				if (!bs->campStanding)
				{
					bs->duckTime = level.time + 1000;
				}
			}
		}
		else if (gWPArray[goalWPIndex] && gWPArray[goalWPIndex]->inuse &&
			!(gLevelFlags & LEVELFLAG_NOPOINTPREDICTION))
		{
			VectorSubtract(gWPArray[goalWPIndex]->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);
		}
		else
		{
			VectorSubtract(bs->wpCurrent->origin, bs->origin, a);
			vectoangles(a, ang);
			VectorCopy(ang, bs->goalAngles);
		}

		if (bs->destinationGrabTime < level.time)
		{
			GetIdealDestination(bs);
		}
		
		if (bs->wpCurrent && bs->wpDestination)
		{
			if (TotalTrailDistance(bs->wpCurrent->index, bs->wpDestination->index, bs) == -1)
			{
				bs->wpDestination = NULL;
				bs->destinationGrabTime = level.time + 1000;//30000;
			}
		}

		if (g_RMG.integer)
		{
			if (bs->frame_Waypoint_Vis)
			{
				if (bs->wpCurrent && !bs->wpCurrent->flags)
				{
					wpTouchDist *= 3;
				}
			}
		}

		VectorCopy(g_entities[bs->cur_ps.clientNum].r.currentOrigin, botpos);
		VectorCopy(gWPArray[bs->wpCurrent->index]->origin, wppos);

		wppos[2] = botpos[2]; // Dont check height here...
		wpDist = VectorDistance(botpos, wppos);

		if ( wpDist < wpTouchDist || (g_RMG.integer && bs->frame_Waypoint_Len < wpTouchDist*2))
		{
			WPTouchRoutine(bs);

			if (!bs->wpDirection)
			{
				desiredIndex = bs->wpCurrent->index+1;
			}
			else
			{
				desiredIndex = bs->wpCurrent->index-1;
			}

			if (gWPArray[desiredIndex] &&
				gWPArray[desiredIndex]->inuse &&
				desiredIndex < gWPNum &&
				desiredIndex >= 0 &&
				PassWayCheck(bs, desiredIndex))
			{
				bs->wpCurrent = gWPArray[desiredIndex];
			}
			else
			{
				if (bs->wpDestination)
				{
					bs->wpDestination = NULL;
					bs->destinationGrabTime = level.time + 10000;
				}

				if (bs->wpDirection)
				{
					bs->wpDirection = 0;
				}
				else
				{
					bs->wpDirection = 1;
				}
			}
		}
	}
	else //We can't find a waypoint, going to need a fallback routine.
	{
		{ //helps them get out of messy situations
		}
		doingFallback = AOTC_BotFallbackNavigation(bs);
	}

	if (bs->currentEnemy && bs->entitynum < MAX_CLIENTS 
		&& (g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] == GCLASS_JEDI || g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] == GCLASS_FORCEMASTER) )
	{
		if (g_entities[bs->entitynum].health <= 0 || bs->currentEnemy->health <= 0 )
		{

		}
		else if ( mod_classes.integer == 2 
		&& bs->settings.team == TEAM_RED
		&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_JEDI
		&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_FORCEMASTER )
		{// No jumping for other red team players...

		}
		else
		{
			vec3_t p1, p2, dir;
			float xy;
			qboolean willFall = qfalse;

			bs->goalAngles[PITCH] = 0;
			bs->goalAngles[ROLL] = 0;

			VectorCopy(bs->goalAngles, b_angle);
	
			AngleVectors(b_angle, fwd, NULL, NULL);
*/
			/*if (bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& CheckFall_By_Vectors(bs->cur_ps.origin, fwd, &g_entities[bs->entitynum]) == qtrue) // We're gonna fall!!!
			{
				if (bs->wpCurrent)
				{// Fall back to the old waypoint info... Already set above.		

				}
				else
				{
					bs->beStill = level.time + 1000;
				}
			}
			else *//*if (!gWPArray && bs->BOTjumpState <= JS_WAITING) // Not in a jump.
			{
				VectorCopy(bs->currentEnemy->r.currentOrigin, trto);
			}

			bs->goalAngles[PITCH] = 0;
			bs->goalAngles[ROLL] = 0;

			VectorCopy(bs->goalAngles, b_angle);
	
			AngleVectors(b_angle, fwd, NULL, NULL);

			if (CheckFall_By_Vectors(bs->origin, fwd, &g_entities[bs->entitynum]) == qtrue)
				willFall = qtrue;

			if ( bs->origin[2] > bs->currentEnemy->r.currentOrigin[2] )
			{
				VectorCopy( bs->origin, p1 );
				VectorCopy( bs->currentEnemy->r.currentOrigin, p2 );
			}
			else if ( bs->origin[2] < bs->currentEnemy->r.currentOrigin[2] )
			{
				VectorCopy( bs->currentEnemy->r.currentOrigin, p1 );
				VectorCopy( bs->origin, p2 );
			}
			else
			{
				VectorCopy( bs->origin, p1 );
				VectorCopy( bs->currentEnemy->r.currentOrigin, p2 );
			}

			VectorSubtract( p2, p1, dir );
			dir[2] = 0;

			//Get xy and z diffs
			xy = VectorNormalize( dir );

			// Jumping Stuff.
			if (bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& xy < 300 
				&& bs->currentEnemy->r.currentOrigin[2] > bs->origin[2] + 32
				&& bs->currentEnemy->health > 0
				&& bs->currentEnemy->client
				&& bs->currentEnemy->client->ps.groundEntityNum	!= ENTITYNUM_NONE)
			{// Jump to enemy. NPC style! They are above us. Do a Jump.
				bs->BOTjumpState = JS_FACING;
				AIMod_Jump ( bs );
				
				VectorCopy(bs->currentEnemy->r.currentOrigin, jumpPos[bs->cur_ps.clientNum]);
			}
			else if (bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& xy > 700 
				&& bs->currentEnemy->r.currentOrigin[2] < bs->origin[2] - 32
				&& bs->currentEnemy->health > 0
				&& bs->currentEnemy->client
				&& bs->currentEnemy->client->ps.groundEntityNum	!= ENTITYNUM_NONE )
			{// Jump to enemy. NPC style! They are below us.
				bs->BOTjumpState = JS_FACING;
				AIMod_Jump ( bs );
				
				VectorCopy(bs->currentEnemy->r.currentOrigin, jumpPos[bs->cur_ps.clientNum]);
			}
			else if (bs->BOTjumpState >= JS_CROUCHING)
			{// Continue any jumps.
				AIMod_Jump ( bs );
			}
		
			if (trto)
			{// We have a possibility.. Check it and maybe use it.
				VectorSubtract(trto, bs->origin, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);

				if (bs->jumpTime > level.time // In a jump.
					|| willFall) // or we wouldn't fall!
				{
					VectorCopy(trto, bs->goalPosition);			

					VectorSubtract(bs->goalPosition, bs->origin, a);
					vectoangles(a, ang);
					VectorCopy(ang, bs->goalAngles);
				}
			}
			else
			{// No location found.
				if (bs->wpCurrent)
				{
					//int wp = GetSharedVisibleWP(&g_entities[bs->cur_ps.clientNum], bs->currentEnemy);
					//bs->wpCurrent = gWPArray[wp];
				}
				else if ( mod_classes.integer == 2 
					&& bs->settings.team == TEAM_RED
					&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_JEDI
					&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_FORCEMASTER )
				{// No jumping for other red team players...

				}
				else if (bs->jumpTime > level.time) // In a jump.
				{// We could be in jump.. Head to safepos if we have one.
					if (safePos[bs->cur_ps.clientNum])
					{
						VectorCopy(safePos[bs->cur_ps.clientNum], bs->goalPosition);
						VectorSubtract(bs->goalPosition, bs->origin, a);
						vectoangles(a, ang);
						VectorCopy(ang, bs->goalAngles);
					}
					else
					{
						VectorCopy(bs->origin, bs->goalPosition);
						bs->beStill = level.time + 1000; //nowhere to go.
					}
				}
				else
				{
					VectorCopy(bs->origin, bs->goalPosition);
					bs->beStill = level.time + 1000; //nowhere to go.
				}
			}
		}
	}
	//end of new aimod ai

	if (bs->currentEnemy && bs->BOTjumpState < JS_CROUCHING) // Force bots to walk while they have an enemy visible... Unique1
		g_entities[bs->cur_ps.clientNum].client->pers.cmd.buttons |= BUTTON_WALKING;

	if (g_RMG.integer)
	{ //for RMG if the bot sticks around an area too long, jump around randomly some to spread to a new area (horrible hacky method)
		vec3_t vSubDif;

		VectorSubtract(bs->origin, bs->lastSignificantAreaChange, vSubDif);
		if (VectorLength(vSubDif) > 1500)
		{
			VectorCopy(bs->origin, bs->lastSignificantAreaChange);
			bs->lastSignificantChangeTime = level.time + 20000;
		}

		if (bs->lastSignificantChangeTime < level.time)
		{
			bs->iHaveNoIdeaWhereIAmGoing = level.time + 17000;
		}
	}

	if (bs->iHaveNoIdeaWhereIAmGoing > level.time && !bs->currentEnemy)
	{
		VectorCopy(preFrameGAngles, bs->goalAngles);
		bs->wpCurrent = NULL;
		bs->wpSwitchTime = level.time + 150;
		doingFallback = AOTC_BotFallbackNavigation(bs);
		bs->jumpTime = level.time + 150;
		bs->jumpHoldTime = level.time + 150;
		bs->jDelay = 0;
		bs->lastSignificantChangeTime = level.time + 25000;
	}

	if (isCapturing)
	{

	}
	else if (bs->wpCurrent && g_RMG.integer)
	{
		qboolean doJ = qfalse;

		if (bs->wpCurrent->origin[2]-192 > bs->origin[2])
		{
			doJ = qtrue;
		}
		else if ((bs->wpTravelTime - level.time) < 5000 && bs->wpCurrent->origin[2]-64 > bs->origin[2])
		{
			doJ = qtrue;
		}
		else if ((bs->wpTravelTime - level.time) < 7000 && (bs->wpCurrent->flags & WPFLAG_RED_FLAG))
		{
			if ((level.time - bs->jumpTime) > 200)
			{
				bs->jumpTime = level.time + 100;
				bs->jumpHoldTime = level.time + 100;
				bs->jDelay = 0;
			}
		}
		else if ((bs->wpTravelTime - level.time) < 7000 && (bs->wpCurrent->flags & WPFLAG_BLUE_FLAG))
		{
			if ((level.time - bs->jumpTime) > 200)
			{
				bs->jumpTime = level.time + 100;
				bs->jumpHoldTime = level.time + 100;
				bs->jDelay = 0;
			}
		}
		else if (bs->wpCurrent->index > 0)
		{
			if ((bs->wpTravelTime - level.time) < 7000)
			{
				if ((gWPArray[bs->wpCurrent->index-1]->flags & WPFLAG_RED_FLAG) ||
					(gWPArray[bs->wpCurrent->index-1]->flags & WPFLAG_BLUE_FLAG))
				{
					if ((level.time - bs->jumpTime) > 200)
					{
						bs->jumpTime = level.time + 100;
						bs->jumpHoldTime = level.time + 100;
						bs->jDelay = 0;
					}
				}
			}
		}

		if (doJ)
		{
			bs->jumpTime = level.time + 1500;
			bs->jumpHoldTime = level.time + 1500;
			bs->jDelay = 0;
		}
	}

	if (doingFallback)
	{
		bs->doingFallback = qtrue;
	}
	else
	{
		bs->doingFallback = qfalse;
	}

	if (bs->timeToReact < level.time && bs->currentEnemy && bs->enemySeenTime > level.time + (ENEMY_FORGET_MS - (ENEMY_FORGET_MS*0.2)))
	{
		if (bs->frame_Enemy_Vis)
		{
			cBAI = CombatBotAI(bs, thinktime);
		}
		else if (bs->cur_ps.weaponstate == WEAPON_CHARGING_ALT)
		{ //keep charging in case we see him again before we lose track of him
			bs->doAltAttack = 1;
		}
		else if (bs->cur_ps.weaponstate == WEAPON_CHARGING)
		{ //keep charging in case we see him again before we lose track of him
			bs->doAttack = 1;
		}

		if (bs->destinationGrabTime > level.time + 100)
		{
			bs->destinationGrabTime = level.time + 100; //assures that we will continue staying within a general area of where we want to be in a combat situation
		}

		if (bs->currentEnemy->client)
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, headlevel);
			headlevel[2] += bs->currentEnemy->client->ps.viewheight;
		}
		else
		{
			VectorCopy(bs->currentEnemy->client->ps.origin, headlevel);
		}

		if (!bs->frame_Enemy_Vis)
		{
			//if (!bs->hitSpotted && VectorLength(a) > 256)
			//if (OrgVisible(bs->eye, bs->lastEnemySpotted, -1))
			if (scenario_enemy_visible[bs->entitynum])
			{
				VectorCopy(bs->lastEnemySpotted, headlevel);
				VectorSubtract(headlevel, bs->eye, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);

				if (bs->cur_ps.weapon == WP_FLECHETTE &&
					bs->cur_ps.weaponstate == WEAPON_READY &&
					bs->currentEnemy && bs->currentEnemy->client)
				{
					mLen = VectorLength(a) > 128;
					if (mLen > 128 && mLen < 1024)
					{
						VectorSubtract(bs->currentEnemy->client->ps.origin, bs->lastEnemySpotted, a);

						bs->frame_Enemy_Vis = qtrue;

						if (VectorLength(a) < 300)
						{
							bs->doAltAttack = 1;
						}
					}
				}
			}
		}
		else
		{
			bLeadAmount = BotWeaponCanLead(bs);
			if ((bs->skills.accuracy/bs->settings.skill) <= 8 &&
				bLeadAmount)
			{
				BotAimLeading(bs, headlevel, bLeadAmount);
			}
			else
			{
				VectorSubtract(headlevel, bs->eye, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);
			}

			BotAimOffsetGoalAngles(bs);
		}
	}

	if (bs->cur_ps.saberInFlight)
	{
		bs->saberThrowTime = level.time + Q_irand(4000, 10000);
	}

	if (bs->currentEnemy)
	{
		if (BotGetWeaponRange(bs) == BWEAPONRANGE_SABER)
		{
			int saberRange = SABER_ATTACK_RANGE;

			VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, a_fo);
			vectoangles(a_fo, a_fo);

			if (bs->saberPowerTime < level.time)
			{ //Don't just use strong attacks constantly, switch around a bit
				if (Q_irand(1, 10) <= 5)
				{
					bs->saberPower = qtrue;
				}
				else
				{
					bs->saberPower = qfalse;
				}

				bs->saberPowerTime = level.time + Q_irand(3000, 15000);
			}

			if ( (bs->cur_ps.weapon == WP_SABER || BG_Is_Staff_Weapon(bs->cur_ps.weapon))
				&& Q_irand(0,10000) < 2 ) 
			{// Switch stance every so often...
				Cmd_SaberAttackCycle_f(&g_entities[bs->client]);
			}

			if (bs->frame_Enemy_Len <= saberRange)
			{
				SaberCombatHandling(bs);

				if (bs->frame_Enemy_Len < 80)
				{
					meleestrafe = 1;
				}
			}
			else if (bs->saberThrowTime < level.time && !bs->cur_ps.saberInFlight &&
				(bs->cur_ps.fd.forcePowersKnown & (1 << FP_SABERTHROW)) &&
				InFieldOfVision(bs->viewangles, 30, a_fo) &&
				bs->frame_Enemy_Len < BOT_SABER_THROW_RANGE &&
				bs->cur_ps.fd.saberAnimLevel != SS_STAFF)
			{
				bs->doAltAttack = 1;
				bs->doAttack = 0;
			}
			else if (bs->cur_ps.saberInFlight && bs->frame_Enemy_Len > 300 && bs->frame_Enemy_Len < BOT_SABER_THROW_RANGE)
			{
				bs->doAltAttack = 1;
				bs->doAttack = 0;
			}
		}
		else if (BotGetWeaponRange(bs) == BWEAPONRANGE_MELEE)
		{
			if (bs->frame_Enemy_Len <= MELEE_ATTACK_RANGE)
			{
				MeleeCombatHandling(bs);
				meleestrafe = 1;
			}
		}
	}
*/
/*	if (doingFallback && bs->currentEnemy
		&& g_entities[bs->entitynum].client->ps.weapon != WP_SABER) //just stand and fire if we have no idea where we are
	{
		VectorCopy(bs->origin, bs->goalPosition);
	}*/
	/*
	if ( mod_classes.integer == 2 
		&& bs->settings.team == TEAM_RED
		&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_JEDI
		&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_FORCEMASTER )
	{// No jumping for other red team players...

	}
	else if (bs->forceJumping > level.time)
	{
		VectorCopy(bs->origin, noz_x);
		VectorCopy(bs->goalPosition, noz_y);

		noz_x[2] = noz_y[2];

		VectorSubtract(noz_x, noz_y, noz_x);

		if (VectorLength(noz_x) < 32)
		{
			fjHalt = 1;
		}
	}

	if (bs->doChat && bs->chatTime > level.time && (!bs->currentEnemy || !bs->frame_Enemy_Vis))
	{
		return;
	}
	else if (bs->doChat && bs->currentEnemy && bs->frame_Enemy_Vis)
	{
		//bs->chatTime = level.time + bs->chatTime_stored;
		bs->doChat = 0; //do we want to keep the bot waiting to chat until after the enemy is gone?
		bs->chatTeam = 0;
	}
	else if (bs->doChat && bs->chatTime <= level.time)
	{
		if (bs->chatTeam)
		{
			trap_EA_SayTeam(bs->client, bs->currentChat);
			bs->chatTeam = 0;
		}
		else
		{
			trap_EA_Say(bs->client, bs->currentChat);
		}
		if (bs->doChat == 2)
		{
			BotReplyGreetings(bs);
		}
		bs->doChat = 0;
	}

	if ( bs->shootGoal &&
		bs->shootGoal->health > 0 && bs->shootGoal->takedamage)
	{
		dif[0] = (bs->shootGoal->r.absmax[0]+bs->shootGoal->r.absmin[0])/2;
		dif[1] = (bs->shootGoal->r.absmax[1]+bs->shootGoal->r.absmin[1])/2;
		dif[2] = (bs->shootGoal->r.absmax[2]+bs->shootGoal->r.absmin[2])/2;

		if (!bs->currentEnemy || bs->frame_Enemy_Len > 256)
		{ //if someone is close then don't stop shooting them for this
			VectorSubtract(dif, bs->eye, a);
			vectoangles(a, a);
			VectorCopy(a, bs->goalAngles);

			if (InFieldOfVision(bs->viewangles, 30, a) &&
				scenario_enemy_visible[bs->entitynum])
			{
				bs->doAttack = 1;
			}
		}
	}

	if (bs->cur_ps.hasDetPackPlanted)
	{ //check if our enemy gets near it and detonate if he does
		BotCheckDetPacks(bs);
	}
	else if (bs->currentEnemy && bs->lastVisibleEnemyIndex == bs->currentEnemy->s.number && !bs->frame_Enemy_Vis && bs->plantTime < level.time &&
		!bs->doAttack && !bs->doAltAttack)
	{
		VectorSubtract(bs->origin, bs->hereWhenSpotted, a);

		if (bs->plantDecided > level.time || (bs->frame_Enemy_Len < BOT_PLANT_DISTANCE*2 && VectorLength(a) < BOT_PLANT_DISTANCE))
		{
			mineSelect = BotSelectChoiceWeapon(bs, WP_TRIP_MINE, 0);
			detSelect = BotSelectChoiceWeapon(bs, WP_DET_PACK, 0);
			if (bs->cur_ps.hasDetPackPlanted)
			{
				detSelect = 0;
			}

			if (bs->plantDecided > level.time && bs->forceWeaponSelect &&
				bs->cur_ps.weapon == bs->forceWeaponSelect)
			{
				bs->doAttack = 1;
				bs->plantDecided = 0;
				bs->plantTime = level.time + BOT_PLANT_INTERVAL;
				bs->plantContinue = level.time + 500;
				bs->beStill = level.time + 500;
			}
			else if (mineSelect || detSelect)
			{
				if (BotSurfaceNear(bs))
				{
					if (!mineSelect)
					{ //if no mines use detpacks, otherwise use mines
						mineSelect = WP_DET_PACK;
					}
					else
					{
						mineSelect = WP_TRIP_MINE;
					}

					detSelect = BotSelectChoiceWeapon(bs, mineSelect, 1);

					if (detSelect && detSelect != 2)
					{ //We have it and it is now our weapon
						bs->plantDecided = level.time + 1000;
						bs->forceWeaponSelect = mineSelect;
						return;
					}
					else if (detSelect == 2)
					{
						bs->forceWeaponSelect = mineSelect;
						return;
					}
				}
			}
		}
	}
	else if (bs->plantContinue < level.time)
	{
		bs->forceWeaponSelect = 0;
	}

	if (isCapturing)
	{

	}
	else if (bs->beStill < level.time && !WaitingForNow(bs, bs->goalPosition) && !fjHalt)
	{
		VectorSubtract(bs->goalPosition, bs->origin, bs->goalMovedir);
		VectorNormalize(bs->goalMovedir);

		if (bs->jumpTime > level.time && bs->jDelay < level.time &&
			level.clients[bs->client].pers.cmd.upmove > 0)
		{
		//	trap_EA_Move(bs->client, bs->origin, 5000);
			bs->beStill = level.time + 200;
		}
		else
		{
			if (bs->currentEnemy)
				trap_EA_Move(bs->client, bs->goalMovedir, 2500);
			else
				trap_EA_Move(bs->client, bs->goalMovedir, 5000);
		}

		if (meleestrafe)
		{
			StrafeTracing(bs);
		}

		if (bs->meleeStrafeDir && meleestrafe && bs->meleeStrafeDisable < level.time)
		{
			trap_EA_MoveRight(bs->client);
		}
		else if (meleestrafe && bs->meleeStrafeDisable < level.time)
		{
			trap_EA_MoveLeft(bs->client);
		}

		if (BotTrace_Jump(bs, bs->goalPosition))
		{
			bs->jumpTime = level.time + 100;
		}
		else if (BotTrace_Duck(bs, bs->goalPosition))
		{
			bs->duckTime = level.time + 100;
		}
#ifdef BOT_STRAFE_AVOIDANCE
		else
		{
			int strafeAround = BotTrace_Strafe(bs, bs->goalPosition);

			if (strafeAround == STRAFEAROUND_RIGHT)
			{
				trap_EA_MoveRight(bs->client);
			}
			else if (strafeAround == STRAFEAROUND_LEFT)
			{
				trap_EA_MoveLeft(bs->client);
			}
		}
#endif
	}

#ifndef FORCEJUMP_INSTANTMETHOD
	if (bs->forceJumpChargeTime > level.time)
	{
		bs->jumpTime = 0;
	}
#endif

	if (bs->jumpPrep > level.time)
	{
		bs->forceJumpChargeTime = 0;
	}

	if (bs->forceJumpChargeTime > level.time)
	{
		bs->jumpHoldTime = ((bs->forceJumpChargeTime - level.time)/2) + level.time;
		bs->forceJumpChargeTime = 0;
	}

	if (bs->jumpHoldTime > level.time)
	{
		bs->jumpTime = bs->jumpHoldTime;
	}

    if (isCapturing)
	{

	}
	else if (bs->jumpTime > level.time && bs->jDelay < level.time)
	{
		gclient_t *client = g_entities[bs->cur_ps.clientNum].client;

		if ( g_gametype.integer == GT_COOP && classnumber[client->ps.clientNum] == CLASS_SOLDIER )
		{// Jumping for coop soldiers...
			if (client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
			{
				client->ps.eFlags |= EF_JETPACK;
			}
			else
			{
				client->ps.eFlags &= ~EF_JETPACK;
			}

			client->jetPackOn = qtrue;

			client->ps.pm_type = PM_JETPACK;
			client->ps.eFlags |= EF_JETPACK_ACTIVE;

			if (bs->jumpHoldTime > level.time)
			{
				if (bs->client > MAX_CLIENTS)
					g_entities[bs->client].client->ps.velocity[2] = 128;
				else
					trap_EA_Jump(bs->client);

				if (bs->wpCurrent)
				{
					if ((bs->wpCurrent->origin[2] - bs->origin[2]) < 64)
					{
						trap_EA_MoveForward(bs->client);
					}
				}
				else
				{
					trap_EA_MoveForward(bs->client);
				}
				if (g_entities[bs->client].client->ps.groundEntityNum == ENTITYNUM_NONE)
				{
					g_entities[bs->client].client->ps.pm_flags |= PMF_JUMP_HELD;
				}
			}
			else if (!(bs->cur_ps.pm_flags & PMF_JUMP_HELD))
			{
				if (bs->client > MAX_CLIENTS)
					g_entities[bs->client].client->ps.velocity[2] = 128;
				else
					trap_EA_Jump(bs->client);
			}
		}
		else if ( mod_classes.integer == 2 && bs->settings.team == TEAM_BLUE && g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] == GCLASS_SOLDIER )
		{
			if (client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK))
			{
				client->ps.eFlags |= EF_JETPACK;
			}
			else
			{
				client->ps.eFlags &= ~EF_JETPACK;
			}

			client->jetPackOn = qtrue;

			client->ps.pm_type = PM_JETPACK;
			client->ps.eFlags |= EF_JETPACK_ACTIVE;

			if (bs->jumpHoldTime > level.time)
			{
				if (bs->client > MAX_CLIENTS)
					g_entities[bs->client].client->ps.velocity[2] = 128;
				else
					trap_EA_Jump(bs->client);

				if (bs->wpCurrent)
				{
					if ((bs->wpCurrent->origin[2] - bs->origin[2]) < 64)
					{
						trap_EA_MoveForward(bs->client);
					}
				}
				else
				{
					trap_EA_MoveForward(bs->client);
				}
				if (g_entities[bs->client].client->ps.groundEntityNum == ENTITYNUM_NONE)
				{
					g_entities[bs->client].client->ps.pm_flags |= PMF_JUMP_HELD;
				}
			}
			else if (!(bs->cur_ps.pm_flags & PMF_JUMP_HELD))
			{
				if (bs->client > MAX_CLIENTS)
					g_entities[bs->client].client->ps.velocity[2] = 128;
				else
					trap_EA_Jump(bs->client);
			}
		}
		else
		{
			if (bs->jumpHoldTime > level.time)
			{
				trap_EA_Jump(bs->client);
				if (bs->wpCurrent)
				{
					if ((bs->wpCurrent->origin[2] - bs->origin[2]) < 64)
					{
						trap_EA_MoveForward(bs->client);
					}
				}
				else
				{
					trap_EA_MoveForward(bs->client);
				}
				if (g_entities[bs->client].client->ps.groundEntityNum == ENTITYNUM_NONE)
				{
					g_entities[bs->client].client->ps.pm_flags |= PMF_JUMP_HELD;
				}
			}
			else if (!(bs->cur_ps.pm_flags & PMF_JUMP_HELD))
			{
				trap_EA_Jump(bs->client);
			}
		}
	}

	if (bs->duckTime > level.time)
	{
		trap_EA_Crouch(bs->client);
	}
*/
	/*if ( bs->dangerousObject && bs->dangerousObject->inuse && bs->dangerousObject->health > 0 &&
		bs->dangerousObject->takedamage && (!bs->frame_Enemy_Vis || !bs->currentEnemy) &&
		(BotGetWeaponRange(bs) == BWEAPONRANGE_MID || BotGetWeaponRange(bs) == BWEAPONRANGE_LONG) &&
		bs->cur_ps.weapon != WP_DET_PACK && bs->cur_ps.weapon != WP_TRIP_MINE && bs->cur_ps.weapon != WP_TRIP_MINE_2 &&
		!bs->shootGoal )
	{
		float danLen;

		VectorSubtract(bs->dangerousObject->r.currentOrigin, bs->eye, a);

		danLen = VectorLength(a);

		if (danLen > 256)
		{
			vectoangles(a, a);
			VectorCopy(a, bs->goalAngles);

			if (Q_irand(1, 10) < 5)
			{
				bs->goalAngles[YAW] += Q_irand(0, 3);
				bs->goalAngles[PITCH] += Q_irand(0, 3);
			}
			else
			{
				bs->goalAngles[YAW] -= Q_irand(0, 3);
				bs->goalAngles[PITCH] -= Q_irand(0, 3);
			}

			if (InFieldOfVision(bs->viewangles, 30, a) &&
				EntityVisibleBox(bs->origin, NULL, NULL, bs->dangerousObject->r.currentOrigin, bs->client, bs->dangerousObject->s.number))
			{
				bs->doAttack = 1;
			}			
		}
	}*/
	/*

	if (PrimFiring(bs) ||
		AltFiring(bs))
	{
		friendInLOF = CheckForFriendInLOF(bs);

		if (friendInLOF)
		{
			if (PrimFiring(bs))
			{
				KeepPrimFromFiring(bs);
			}
			if (AltFiring(bs))
			{
				KeepAltFromFiring(bs);
			}
			if (useTheForce && forceHostile)
			{
				useTheForce = 0;
			}

			if (!useTheForce && friendInLOF->client)
			{ //we have a friend here and are not currently using force powers, see if we can help them out
				if (friendInLOF->health <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_HEAL;
					useTheForce = 1;
					forceHostile = 0;
				}
				else if (friendInLOF->client->ps.fd.forcePower <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_FORCE;
					useTheForce = 1;
					forceHostile = 0;
				}
			}
		}
	}
	else
	{ //still check for anyone to help..
		friendInLOF = CheckForFriendInLOF(bs);

		if (!useTheForce && friendInLOF)
		{
			if (friendInLOF->health <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_HEAL]][FP_TEAM_HEAL])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_HEAL;
				useTheForce = 1;
				forceHostile = 0;
			}
			else if (friendInLOF->client->ps.fd.forcePower <= 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TEAM_FORCE]][FP_TEAM_FORCE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_TEAM_FORCE;
				useTheForce = 1;
				forceHostile = 0;
			}
		}
	}

	if (bs->doAttack && bs->cur_ps.weapon == WP_DET_PACK &&
		bs->cur_ps.hasDetPackPlanted)
	{ //maybe a bit hackish, but bots only want to plant one of these at any given time to avoid complications
		bs->doAttack = 0;
	}

	if (bs->doAttack && bs->cur_ps.weapon == WP_SABER &&
		bs->saberDefending && bs->currentEnemy && bs->currentEnemy->client &&
		BotWeaponBlockable(bs->currentEnemy->client->ps.weapon) )
	{
		bs->doAttack = 0;
	}

	if (bs->cur_ps.saberLockTime > level.time)
	{
		if (rand()%10 < 5)
		{
			bs->doAttack = 1;
		}
		else
		{
			bs->doAttack = 0;
		}
	}

	if (bs->botChallengingTime > level.time)
	{
		bs->doAttack = 0;
		bs->doAltAttack = 0;
	}

	if (bs->cur_ps.weapon == WP_SABER &&
		bs->cur_ps.saberInFlight &&
		!bs->cur_ps.saberEntityNum)
	{ //saber knocked away, keep trying to get it back
		bs->doAttack = 1;
		bs->doAltAttack = 0;
	}

	if (bs->doAttack)
	{
		trap_EA_Attack(bs->client);
	}
	else if (bs->doAltAttack)
	{
		trap_EA_Alt_Attack(bs->client);
	}

	if (useTheForce && forceHostile && bs->botChallengingTime > level.time)
	{
		useTheForce = qfalse;
	}

	if (useTheForce)
	{
#ifndef FORCEJUMP_INSTANTMETHOD
		if (bs->forceJumpChargeTime > level.time)
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_LEVITATION;
			trap_EA_ForcePower(bs->client);
		}
		else
		{
#endif
			if (bot_forcepowers.integer && !g_forcePowerDisable.integer)
			{
				trap_EA_ForcePower(bs->client);
			}
#ifndef FORCEJUMP_INSTANTMETHOD
		}
#endif
	}

	MoveTowardIdealAngles(bs);
}
*/
//[/NewGameTypes][EnhancedImpliment]
//[/AotCAI]


