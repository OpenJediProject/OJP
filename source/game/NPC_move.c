//
// NPC_move.cpp
//
#include "b_local.h"
#include "g_nav.h"
#include "anims.h"

//[CoOp]
//needed for math.h functions
//#include "q_shared.h"
//[/CoOp]

void G_Cylinder( vec3_t start, vec3_t end, float radius, vec3_t color );

qboolean G_BoundsOverlap(const vec3_t mins1, const vec3_t maxs1, const vec3_t mins2, const vec3_t maxs2);
int NAV_Steer( gentity_t *self, vec3_t dir, float distance );
extern int GetTime ( int lastTime );

navInfo_t	frameNavInfo;
extern qboolean FlyingCreature( gentity_t *ent );

#include "../namespace_begin.h"
extern qboolean PM_InKnockDown( playerState_t *ps );
#include "../namespace_end.h"

//[CoOp]
static qboolean NPC_TryJump_Final();
extern void G_DrawEdge( vec3_t start, vec3_t end, int type );

static qboolean NPC_Jump( vec3_t dest, int goalEntNum )
{//FIXME: if land on enemy, knock him down & jump off again
	float	targetDist, travelTime, impactDist, bestImpactDist = Q3_INFINITE;//fireSpeed, 
	float originalShotSpeed, shotSpeed, speedStep = 50.0f, minShotSpeed = 30.0f, maxShotSpeed = 500.0f;
	qboolean belowBlocked = qfalse, aboveBlocked = qfalse;
	vec3_t	targetDir, shotVel, failCase; 
	trace_t	trace;
	trajectory_t	tr;
	qboolean	blocked;
	int		elapsedTime, timeStep = 250, hitCount = 0, aboveTries = 0, belowTries = 0, maxHits = 10;
	vec3_t	lastPos, testPos, bottom;

	VectorSubtract( dest, NPC->r.currentOrigin, targetDir );
	targetDist = VectorNormalize( targetDir );
	//make our shotSpeed reliant on the distance
	originalShotSpeed = targetDist;//DistanceHorizontal( dest, NPC->currentOrigin )/2.0f;
	if ( originalShotSpeed > maxShotSpeed )
	{
		originalShotSpeed = maxShotSpeed;
	}
	else if ( originalShotSpeed < minShotSpeed )
	{
		originalShotSpeed = minShotSpeed;
	}
	shotSpeed = originalShotSpeed;

	while ( hitCount < maxHits )
	{
		VectorScale( targetDir, shotSpeed, shotVel );
		travelTime = targetDist/shotSpeed;
		shotVel[2] += travelTime * 0.5 * NPC->client->ps.gravity;

		if ( !hitCount )		
		{//save the first one as the worst case scenario
			VectorCopy( shotVel, failCase );
		}

		if ( 1 )//tracePath )
		{//do a rough trace of the path
			blocked = qfalse;

			VectorCopy( NPC->r.currentOrigin, tr.trBase );
			VectorCopy( shotVel, tr.trDelta );
			tr.trType = TR_GRAVITY;
			tr.trTime = level.time;
			travelTime *= 1000.0f;
			VectorCopy( NPC->r.currentOrigin, lastPos );
			
			//This may be kind of wasteful, especially on long throws... use larger steps?  Divide the travelTime into a certain hard number of slices?  Trace just to apex and down?
			for ( elapsedTime = timeStep; elapsedTime < floor(travelTime)+timeStep; elapsedTime += timeStep )
			{
				if ( (float)elapsedTime > travelTime )
				{//cap it
					elapsedTime = floor( travelTime );
				}
				BG_EvaluateTrajectory( &tr, level.time + elapsedTime, testPos );
				//FUCK IT, always check for do not enter...
				trap_Trace( &trace, lastPos, NPC->r.mins, NPC->r.maxs, testPos, NPC->s.number, NPC->clipmask|CONTENTS_BOTCLIP );
				/*
				if ( testPos[2] < lastPos[2] 
					&& elapsedTime < floor( travelTime ) )
				{//going down, haven't reached end, ignore botclip
					gi.trace( &trace, lastPos, NPC->mins, NPC->maxs, testPos, NPC->s.number, NPC->clipmask );
				}
				else
				{//going up, check for botclip
					gi.trace( &trace, lastPos, NPC->mins, NPC->maxs, testPos, NPC->s.number, NPC->clipmask|CONTENTS_BOTCLIP );
				}
				*/

				if ( trace.allsolid || trace.startsolid )
				{//started in solid
					if ( NAVDEBUG_showCollision )
					{
						G_DrawEdge( lastPos, trace.endpos, EDGE_RED_TWOSECOND );
					}
					return qfalse;//you're hosed, dude
				}
				if ( trace.fraction < 1.0f )
				{//hit something
					if ( NAVDEBUG_showCollision )
					{
						G_DrawEdge( lastPos, trace.endpos, EDGE_RED_TWOSECOND );	// TryJump
					}
					if ( trace.entityNum == goalEntNum )
					{//hit the enemy, that's bad!
						blocked = qtrue;
						/*
						if ( g_entities[goalEntNum].client && g_entities[goalEntNum].client->ps.groundEntityNum == ENTITYNUM_NONE )
						{//bah, would collide in mid-air, no good
							blocked = qtrue;
						}
						else
						{//he's on the ground, good enough, I guess
							//Hmm, don't want to land on him, though...?
						}
						*/
						break;
					}
					else 
					{
						if ( trace.contents & CONTENTS_BOTCLIP )
						{//hit a do-not-enter brush
							blocked = qtrue;
							break;
						}
						if ( trace.plane.normal[2] > 0.7 && DistanceSquared( trace.endpos, dest ) < 4096 )//hit within 64 of desired location, should be okay
						{//close enough!
							break;
						}
						else
						{//FIXME: maybe find the extents of this brush and go above or below it on next try somehow?
							impactDist = DistanceSquared( trace.endpos, dest );
							if ( impactDist < bestImpactDist )
							{
								bestImpactDist = impactDist;
								VectorCopy( shotVel, failCase );
							}
							blocked = qtrue;
							break;
						}
					}
				}
				else
				{
					if ( NAVDEBUG_showCollision )
					{
						G_DrawEdge( lastPos, testPos, EDGE_WHITE_TWOSECOND );	// TryJump
					}
				}
				if ( elapsedTime == floor( travelTime ) )
				{//reached end, all clear
					if ( trace.fraction >= 1.0f )
					{//hmm, make sure we'll land on the ground...
						//FIXME: do we care how far below ourselves or our dest we'll land?
						VectorCopy( trace.endpos, bottom );
						bottom[2] -= 128;
						trap_Trace( &trace, trace.endpos, NPC->r.mins, NPC->r.maxs, bottom, NPC->s.number, NPC->clipmask );
						if ( trace.fraction >= 1.0f )
						{//would fall too far
							blocked = qtrue;
						}
					}
					break;
				}
				else
				{
					//all clear, try next slice
					VectorCopy( testPos, lastPos );
				}
			}
			if ( blocked )
			{//hit something, adjust speed (which will change arc)
				hitCount++;
				//alternate back and forth between trying an arc slightly above or below the ideal
				if ( (hitCount%2) && !belowBlocked )
				{//odd
					belowTries++;
					shotSpeed = originalShotSpeed - (belowTries*speedStep);
				}
				else if ( !aboveBlocked )
				{//even
					aboveTries++;
					shotSpeed = originalShotSpeed + (aboveTries*speedStep);
				}
				else
				{//can't go any higher or lower
					hitCount = maxHits;
					break;
				}
				if ( shotSpeed > maxShotSpeed )
				{
					shotSpeed = maxShotSpeed;
					aboveBlocked = qtrue;
				}
				else if ( shotSpeed < minShotSpeed )
				{
					shotSpeed = minShotSpeed;
					belowBlocked = qtrue;
				}
			}
			else
			{//made it!
				break;
			}
		}
		else
		{//no need to check the path, go with first calc
			break;
		}
	}

	if ( hitCount >= maxHits )
	{//NOTE: worst case scenario, use the one that impacted closest to the target (or just use the first try...?)
		return qfalse;
		//NOTE: or try failcase?
		//VectorCopy( failCase, NPC->client->ps.velocity );
		//return qtrue;
	}
	VectorCopy( shotVel, NPC->client->ps.velocity );
	return qtrue;
}

#define NPC_JUMP_PREP_BACKUP_DIST 34.0f

trace_t		mJumpTrace;

qboolean NPC_CanTryJump()
{
	if (!(NPCInfo->scriptFlags&SCF_NAV_CAN_JUMP)	||		// Can't Jump
		(NPCInfo->scriptFlags&SCF_NO_ACROBATICS)	||		// If Can't Jump At All
		(level.time<NPCInfo->jumpBackupTime)		||		// If Backing Up, Don't Try The Jump Again
		(level.time<NPCInfo->jumpNextCheckTime)		||		// Don't Even Try To Jump Again For This Amount Of Time
		(NPCInfo->jumpTime)							||		// Don't Jump If Already Going
		(PM_InKnockDown(&NPC->client->ps))			||		// Don't Jump If In Knockdown
		(BG_InRoll(&NPC->client->ps, NPC->client->ps.legsAnim))	||		// ... Or Roll
		(NPC->client->ps.groundEntityNum==ENTITYNUM_NONE)	// ... Or In The Air
		)
	{
		return qfalse;
	}
	return qtrue;
}


//[CoOp]
//[SPPortComplete]
qboolean NPC_TryJump(void);
qboolean NPC_TryJump3(const vec3_t pos,	float max_xy_dist, float max_z_diff)
{
	if (NPC_CanTryJump())
	{
		NPCInfo->jumpNextCheckTime	= level.time + Q_irand(1000, 2000);

		VectorCopy(pos, NPCInfo->jumpDest);

		// Can't Try To Jump At A Point In The Air
		//-----------------------------------------
		{
			vec3_t	groundTest;
			VectorCopy(pos, groundTest);
			groundTest[2]	+= (NPC->r.mins[2]*3);
			trap_Trace(&mJumpTrace, NPCInfo->jumpDest, vec3_origin, vec3_origin, 
				groundTest, NPC->s.number, NPC->clipmask );
			if (mJumpTrace.fraction >= 1.0f)
			{
				return qfalse;	//no ground = no jump
			}
		}
		NPCInfo->jumpTarget			= 0;
		NPCInfo->jumpMaxXYDist		= (max_xy_dist)?(max_xy_dist):((NPC->client->NPC_class==CLASS_ROCKETTROOPER)?1200:750);
		NPCInfo->jumpMazZDist		= (max_z_diff)?(max_z_diff):((NPC->client->NPC_class==CLASS_ROCKETTROOPER)?-1000:-450);
		NPCInfo->jumpTime			= 0;
		NPCInfo->jumpBackupTime		= 0;
		return NPC_TryJump();
	}
	return qfalse;
}

qboolean NPC_TryJump2(gentity_t *goal, float max_xy_dist, float max_z_diff)
{
	if (NPC_CanTryJump())
	{
		NPCInfo->jumpNextCheckTime	= level.time + Q_irand(1000, 3000);

		// Can't Jump At Targets In The Air
		//---------------------------------
		if (goal->client && goal->client->ps.groundEntityNum==ENTITYNUM_NONE)
		{
			return qfalse;
		}
		VectorCopy(goal->r.currentOrigin, NPCInfo->jumpDest);
		NPCInfo->jumpTarget			= goal;
		NPCInfo->jumpMaxXYDist		= (max_xy_dist)?(max_xy_dist):((NPC->client->NPC_class==CLASS_ROCKETTROOPER)?1200:750);
		NPCInfo->jumpMazZDist		= (max_z_diff)?(max_z_diff):((NPC->client->NPC_class==CLASS_ROCKETTROOPER)?-1000:-400);
		NPCInfo->jumpTime			= 0;
		NPCInfo->jumpBackupTime		= 0;
		return NPC_TryJump();
	}
	return qfalse;
}

void	 NPC_JumpSound();
void	 NPC_JumpAnimation();
qboolean NPC_TryJump(void)
{//
	vec3_t	targetDirection;
	float	targetDistanceXY;
	float	targetDistanceZ;
	qboolean	WithinForceJumpRange;

	// Get The Direction And Distances To The Target
	//-----------------------------------------------
	VectorSubtract(NPCInfo->jumpDest, NPC->r.currentOrigin, targetDirection);
	targetDirection[2]	= 0.0f;
	targetDistanceXY	= VectorNormalize(targetDirection);
	targetDistanceZ		= NPCInfo->jumpDest[2] - NPC->r.currentOrigin[2];

	if ((targetDistanceXY>NPCInfo->jumpMaxXYDist) ||
		(targetDistanceZ<NPCInfo->jumpMazZDist))
	{
		return qfalse;
	}


	// Test To See If There Is A Wall Directly In Front Of Actor, If So, Backup Some
	//-------------------------------------------------------------------------------
	if (TIMER_Done(NPC, "jumpBackupDebounce"))
	{
		vec3_t	actorProjectedTowardTarget;
		VectorMA(NPC->r.currentOrigin, NPC_JUMP_PREP_BACKUP_DIST, targetDirection, actorProjectedTowardTarget);
		trap_Trace(&mJumpTrace, NPC->r.currentOrigin, vec3_origin, vec3_origin, actorProjectedTowardTarget, NPC->s.number, NPC->clipmask);
		if ((mJumpTrace.fraction < 1.0f) ||
			(mJumpTrace.allsolid) ||
			(mJumpTrace.startsolid))
		{
			if (NAVDEBUG_showCollision)
			{
				G_DrawEdge(NPC->r.currentOrigin, actorProjectedTowardTarget, EDGE_RED_TWOSECOND);	// TryJump
			}

			// TODO: We may want to test to see if it is safe to back up here?
			NPCInfo->jumpBackupTime = level.time + 1000;
			TIMER_Set(NPC, "jumpBackupDebounce", 5000);
			return qtrue;
		}
	}


//	bool	Wounded					= (NPC->health < 150);
//	bool	OnLowerLedge			= ((targetDistanceZ<-80.0f) && (targetDistanceZ>-200.0f));
//	bool	WithinNormalJumpRange	= ((targetDistanceZ<32.0f)  && (targetDistanceXY<200.0f));
	
	//WithinForceJumpRange	= ((fabsf(targetDistanceZ)>0) || (targetDistanceXY>128));
	WithinForceJumpRange	= ((fabs(targetDistanceZ)>0) || (targetDistanceXY>128));

/*	if (Wounded && OnLowerLedge)
	{
		ucmd.forwardmove	= 127;
		VectorClear(NPC->client->ps.moveDir);
		TIMER_Set(NPC, "duck", -level.time);
		return qtrue;
	}
	
	if (WithinNormalJumpRange)
	{
		ucmd.upmove			= 127;
		ucmd.forwardmove	= 127;
		VectorClear(NPC->client->ps.moveDir);
		TIMER_Set(NPC, "duck", -level.time);
		return qtrue;
	}
*/

	if (!WithinForceJumpRange)
	{
		return qfalse;
	}



	// If There Is Any Chance That This Jump Will Land On An Enemy, Try 8 Different Traces Around The Target
	//-------------------------------------------------------------------------------------------------------
	if (NPCInfo->jumpTarget)
	{//racc - We're jumping to an entity
		float	minSafeRadius	= (NPC->r.maxs[0]*1.5f) + (NPCInfo->jumpTarget->r.maxs[0]*1.5f);
		float	minSafeRadiusSq	= (minSafeRadius * minSafeRadius);

		if (DistanceSquared(NPCInfo->jumpDest, NPCInfo->jumpTarget->r.currentOrigin)<minSafeRadiusSq)
		{
			int		sideTryCount;
			vec3_t	startPos;
			vec3_t	floorPos;
			VectorCopy(NPCInfo->jumpDest, startPos);

			floorPos[2] = NPCInfo->jumpDest[2] + (NPC->r.mins[2]-32);

			for (sideTryCount=0; sideTryCount<8; sideTryCount++)
			{
				NPCInfo->jumpSide++;
				if ( NPCInfo->jumpSide > 7 )
				{
					NPCInfo->jumpSide = 0;
				}

				switch ( NPCInfo->jumpSide )
				{
				case 0:
					NPCInfo->jumpDest[0] = startPos[0] + minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1];
					break;
				case 1:
					NPCInfo->jumpDest[0] = startPos[0] + minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1] + minSafeRadius;
					break;
				case 2:
					NPCInfo->jumpDest[0] = startPos[0];
					NPCInfo->jumpDest[1] = startPos[1] + minSafeRadius;
					break;
				case 3:
					NPCInfo->jumpDest[0] = startPos[0] - minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1] + minSafeRadius;
					break;
				case 4:
					NPCInfo->jumpDest[0] = startPos[0] - minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1];
					break;
				case 5:
					NPCInfo->jumpDest[0] = startPos[0] - minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1] - minSafeRadius;
					break;
				case 6:
					NPCInfo->jumpDest[0] = startPos[0];
					NPCInfo->jumpDest[1] = startPos[1] - minSafeRadius;
					break;
				case 7:
					NPCInfo->jumpDest[0] = startPos[0] + minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1] -=minSafeRadius;
					break;
				};

				floorPos[0] = NPCInfo->jumpDest[0];
				floorPos[1] = NPCInfo->jumpDest[1];

				trap_Trace(&mJumpTrace, NPCInfo->jumpDest, NPC->r.mins, NPC->r.maxs, floorPos, (NPCInfo->jumpTarget)?(NPCInfo->jumpTarget->s.number):(NPC->s.number), (NPC->clipmask|CONTENTS_BOTCLIP));
				if ((mJumpTrace.fraction<1.0f) && 
					(!mJumpTrace.allsolid) && 
					(!mJumpTrace.startsolid))
				{
					break;
				}

				if ( NAVDEBUG_showCollision )
				{	
					G_DrawEdge( NPCInfo->jumpDest, floorPos, EDGE_RED_TWOSECOND );
				}
			}

			// If All Traces Failed, Just Try Going Right Back At The Target Location
			//------------------------------------------------------------------------
			if ((mJumpTrace.fraction>=1.0f) || 
				(mJumpTrace.allsolid) || 
				(mJumpTrace.startsolid))
			{
				VectorCopy(startPos, NPCInfo->jumpDest);
			}
		}
	}
	
	// Now, Actually Try The Jump To The Dest Target
	//-----------------------------------------------
	if (NPC_Jump(NPCInfo->jumpDest, (NPCInfo->jumpTarget)?(NPCInfo->jumpTarget->s.number):(NPC->s.number)))
	{
		// We Made IT!
		//-------------
		NPC_JumpAnimation();
		NPC_JumpSound();

		NPC->client->ps.fd.forceJumpZStart	  = NPC->r.currentOrigin[2];
		//RAFIXME - Impliment flag?
		//NPC->client->ps.pm_flags			 |= PMF_JUMPING;
		NPC->client->ps.weaponTime			  = NPC->client->ps.torsoTimer;
		NPC->client->ps.fd.forcePowersActive |= ( 1 << FP_LEVITATION );
		ucmd.forwardmove					  = 0;
		NPCInfo->jumpTime					  = 1;

		VectorClear(NPC->client->ps.moveDir);
		TIMER_Set(NPC, "duck", -level.time);

		return qtrue;
	}
	return qfalse;
}
//[/SPPortComplete]
//[/CoOp]


qboolean NPC_TryJump_Pos(const vec3_t pos,	float max_xy_dist, float max_z_diff)
{
	if (NPC_CanTryJump())
	{
		NPCInfo->jumpNextCheckTime	= level.time + Q_irand(1000, 2000);

		VectorCopy(pos, NPCInfo->jumpDest);

		// Can't Try To Jump At A Point In The Air
		//-----------------------------------------
		{
			vec3_t	groundTest;
			VectorCopy(pos, groundTest);
			groundTest[2]	+= (NPC->r.mins[2]*3);
			trap_Trace(&mJumpTrace, NPCInfo->jumpDest, vec3_origin, vec3_origin, groundTest, NPC->s.number, NPC->clipmask );
			if (mJumpTrace.fraction >= 1.0f)
			{
				return qfalse;	//no ground = no jump
			}
		}
		NPCInfo->jumpTarget			= 0;
		NPCInfo->jumpMaxXYDist		= (max_xy_dist)?(max_xy_dist):((NPC->client->NPC_class==CLASS_ROCKETTROOPER)?1200:750);
		NPCInfo->jumpMazZDist		= (max_z_diff)?(max_z_diff):((NPC->client->NPC_class==CLASS_ROCKETTROOPER)?-1000:-450);
		NPCInfo->jumpTime			= 0;
		NPCInfo->jumpBackupTime		= 0;
		return NPC_TryJump_Final();
	}
	return qfalse;
}

qboolean NPC_TryJump_Gent(gentity_t *goal,	float max_xy_dist, float max_z_diff)
{
	if (NPC_CanTryJump())
	{
		NPCInfo->jumpNextCheckTime	= level.time + Q_irand(1000, 3000);

		// Can't Jump At Targets In The Air
		//---------------------------------
		if (goal->client && goal->client->ps.groundEntityNum==ENTITYNUM_NONE)
		{
			return qfalse;
		}
		VectorCopy(goal->r.currentOrigin, NPCInfo->jumpDest);
		NPCInfo->jumpTarget			= goal;
		NPCInfo->jumpMaxXYDist		= (max_xy_dist)?(max_xy_dist):((NPC->client->NPC_class==CLASS_ROCKETTROOPER)?1200:750);
		NPCInfo->jumpMazZDist		= (max_z_diff)?(max_z_diff):((NPC->client->NPC_class==CLASS_ROCKETTROOPER)?-1000:-400);
		NPCInfo->jumpTime			= 0;
		NPCInfo->jumpBackupTime		= 0;
		return NPC_TryJump_Final();
	}
	return qfalse;
}


//do animation for jump
void	 NPC_JumpAnimation()
{
	int	jumpAnim = BOTH_JUMP1;

	if ( NPC->client->NPC_class == CLASS_BOBAFETT 
		|| (NPC->client->NPC_class == CLASS_REBORN && NPC->s.weapon != WP_SABER)
		|| NPC->client->NPC_class == CLASS_ROCKETTROOPER
		||( NPCInfo->rank != RANK_CREWMAN && NPCInfo->rank <= RANK_LT_JG ) )
	{//can't do acrobatics
		jumpAnim = BOTH_FORCEJUMP1;
	}
	else if (NPC->client->NPC_class != CLASS_HOWLER)
	{
		if ( NPC->client->NPC_class == CLASS_ALORA && Q_irand( 0, 3 ) )
		{
			jumpAnim = Q_irand( BOTH_ALORA_FLIP_1, BOTH_ALORA_FLIP_3 );
		}
		else
		{
			jumpAnim = BOTH_FLIP_F;
		}
	}
	NPC_SetAnim( NPC, SETANIM_BOTH, jumpAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
}

//RAFIXME - CLASS_ROCKETTROOPER|CLASS_BOBAFETT
//extern void JET_FlyStart(gentity_t* actor);

//make sound for jump
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );
void	 NPC_JumpSound()
{
	if ( NPC->client->NPC_class == CLASS_HOWLER )
	{
		//FIXME: can I delay the actual jump so that it matches the anim...?
	}
	else if ( NPC->client->NPC_class == CLASS_BOBAFETT 
		|| NPC->client->NPC_class == CLASS_ROCKETTROOPER )
	{
		// does this really need to be here?
		//RAFIXME - CLASS_ROCKETTROOPER|CLASS_BOBAFETT
		//JET_FlyStart(NPC);
	}
	else
	{
		G_SoundOnEnt( NPC, CHAN_BODY, "sound/weapons/force/jump.wav" );
	}
}


//[CoOp]
qboolean NPC_Jumping()
{//checks to see if we're jumping
	if ( NPCInfo->jumpTime )
	{
		if(NPC->client->ps.groundEntityNum != ENTITYNUM_NONE)
		/* RAFIXME - not exactly sure how to do this.
		if ( !(NPC->client->ps.pm_flags & PMF_JUMPING )//forceJumpZStart )
			&& !(NPC->client->ps.pm_flags&PMF_TRIGGER_PUSHED))
		*/
		{//landed
			NPCInfo->jumpTime = 0;
		}
		else
		{
			NPC_FacePosition(NPCInfo->jumpDest, qtrue);
			return qtrue;
		}
	}
	return qfalse;
}


//[SPPortComplete]
qboolean NPC_JumpBackingUp()
{//check for and back up before a large jump if we're supposed to.
	if (NPCInfo->jumpBackupTime)
	{
		if (level.time<NPCInfo->jumpBackupTime)
		{
			/*  RAFIXME - impliment nav code.
			STEER::Activate(NPC);
			STEER::Flee(NPC, NPCInfo->jumpDest);
			STEER::DeActivate(NPC, &ucmd);
			*/
			NPC_FacePosition(NPCInfo->jumpDest, qtrue);
			NPC_UpdateAngles( qfalse, qtrue );
			return qtrue;
		}

		NPCInfo->jumpBackupTime = 0;
		return NPC_TryJump();
	}
	return qfalse;
}
//[/SPPortComplete]
//[/CoOp]


qboolean NPC_TryJump_Final()
{
	vec3_t	targetDirection;
	float	targetDistanceXY;
	float	targetDistanceZ;
	int		sideTryCount;

	qboolean	WithinForceJumpRange;

	// Get The Direction And Distances To The Target
	//-----------------------------------------------
	VectorSubtract(NPCInfo->jumpDest, NPC->r.currentOrigin, targetDirection);
	targetDirection[2]	= 0.0f;
	targetDistanceXY	= VectorNormalize(targetDirection);
	targetDistanceZ		= NPCInfo->jumpDest[2] - NPC->r.currentOrigin[2];

	if ((targetDistanceXY>NPCInfo->jumpMaxXYDist) ||
		(targetDistanceZ<NPCInfo->jumpMazZDist))
	{
		return qfalse;
	}


	// Test To See If There Is A Wall Directly In Front Of Actor, If So, Backup Some
	//-------------------------------------------------------------------------------
	if (TIMER_Done(NPC, "jumpBackupDebounce"))
	{
		vec3_t	actorProjectedTowardTarget;
		VectorMA(NPC->r.currentOrigin, NPC_JUMP_PREP_BACKUP_DIST, targetDirection, actorProjectedTowardTarget);
		trap_Trace(&mJumpTrace, NPC->r.currentOrigin, vec3_origin, vec3_origin, actorProjectedTowardTarget, NPC->s.number, NPC->clipmask);
		if ((mJumpTrace.fraction < 1.0f) ||
			(mJumpTrace.allsolid) ||
			(mJumpTrace.startsolid))
		{
			if (NAVDEBUG_showCollision)
			{
				G_DrawEdge(NPC->r.currentOrigin, actorProjectedTowardTarget, EDGE_RED_TWOSECOND);	// TryJump
			}

			// TODO: We may want to test to see if it is safe to back up here?
			NPCInfo->jumpBackupTime = level.time + 1000;
			TIMER_Set(NPC, "jumpBackupDebounce", 5000);
			return qtrue;
		}
	}


//	bool	Wounded					= (NPC->health < 150);
//	bool	OnLowerLedge			= ((targetDistanceZ<-80.0f) && (targetDistanceZ>-200.0f));
//	bool	WithinNormalJumpRange	= ((targetDistanceZ<32.0f)  && (targetDistanceXY<200.0f));
	WithinForceJumpRange = (((float)fabs(targetDistanceZ)>0) || (targetDistanceXY>128));

/*	if (Wounded && OnLowerLedge)
	{
		ucmd.forwardmove	= 127;
		VectorClear(NPC->client->ps.moveDir);
		TIMER_Set(NPC, "duck", -level.time);
		return qtrue;
	}
	
	if (WithinNormalJumpRange)
	{
		ucmd.upmove			= 127;
		ucmd.forwardmove	= 127;
		VectorClear(NPC->client->ps.moveDir);
		TIMER_Set(NPC, "duck", -level.time);
		return qtrue;
	}
*/

	if (!WithinForceJumpRange)
	{
		return qfalse;
	}



	// If There Is Any Chance That This Jump Will Land On An Enemy, Try 8 Different Traces Around The Target
	//-------------------------------------------------------------------------------------------------------
	if (NPCInfo->jumpTarget)
	{
		float	minSafeRadius	= (NPC->r.maxs[0]*1.5f) + (NPCInfo->jumpTarget->r.maxs[0]*1.5f);
		float	minSafeRadiusSq	= (minSafeRadius * minSafeRadius);

		if (DistanceSquared(NPCInfo->jumpDest, NPCInfo->jumpTarget->r.currentOrigin)<minSafeRadiusSq)
		{
			vec3_t	startPos;
			vec3_t	floorPos;
			VectorCopy(NPCInfo->jumpDest, startPos);

			floorPos[2] = NPCInfo->jumpDest[2] + (NPC->r.mins[2]-32);

			for (sideTryCount=0; sideTryCount<8; sideTryCount++)
			{
				NPCInfo->jumpSide++;
				if ( NPCInfo->jumpSide > 7 )
				{
					NPCInfo->jumpSide = 0;
				}

				switch ( NPCInfo->jumpSide )
				{
				case 0:
					NPCInfo->jumpDest[0] = startPos[0] + minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1];
					break;
				case 1:
					NPCInfo->jumpDest[0] = startPos[0] + minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1] + minSafeRadius;
					break;
				case 2:
					NPCInfo->jumpDest[0] = startPos[0];
					NPCInfo->jumpDest[1] = startPos[1] + minSafeRadius;
					break;
				case 3:
					NPCInfo->jumpDest[0] = startPos[0] - minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1] + minSafeRadius;
					break;
				case 4:
					NPCInfo->jumpDest[0] = startPos[0] - minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1];
					break;
				case 5:
					NPCInfo->jumpDest[0] = startPos[0] - minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1] - minSafeRadius;
					break;
				case 6:
					NPCInfo->jumpDest[0] = startPos[0];
					NPCInfo->jumpDest[1] = startPos[1] - minSafeRadius;
					break;
				case 7:
					NPCInfo->jumpDest[0] = startPos[0] + minSafeRadius;
					NPCInfo->jumpDest[1] = startPos[1] -=minSafeRadius;
					break;
				}

				floorPos[0] = NPCInfo->jumpDest[0];
				floorPos[1] = NPCInfo->jumpDest[1];

				trap_Trace(&mJumpTrace, NPCInfo->jumpDest, NPC->r.mins, NPC->r.maxs, floorPos, (NPCInfo->jumpTarget)?(NPCInfo->jumpTarget->s.number):(NPC->s.number), (NPC->clipmask|CONTENTS_BOTCLIP));
				if ((mJumpTrace.fraction<1.0f) && 
					(!mJumpTrace.allsolid) && 
					(!mJumpTrace.startsolid))
				{
					break;
				}

				if ( NAVDEBUG_showCollision )
				{
					G_DrawEdge( NPCInfo->jumpDest, floorPos, EDGE_RED_TWOSECOND );
				}
			}

			// If All Traces Failed, Just Try Going Right Back At The Target Location
			//------------------------------------------------------------------------
			if ((mJumpTrace.fraction>=1.0f) || 
				(mJumpTrace.allsolid) || 
				(mJumpTrace.startsolid))
			{
				VectorCopy(startPos, NPCInfo->jumpDest);
			}
		}
	}
	
	// Now, Actually Try The Jump To The Dest Target
	//-----------------------------------------------
	if (NPC_Jump(NPCInfo->jumpDest, (NPCInfo->jumpTarget)?(NPCInfo->jumpTarget->s.number):(NPC->s.number)))
	{
		// We Made IT!
		//-------------
		NPC_JumpAnimation();
		NPC_JumpSound();

		NPC->client->ps.fd.forceJumpZStart	 = NPC->r.currentOrigin[2];
		//NPC->client->ps.pm_flags			|= PMF_JUMPING;
		NPC->client->ps.weaponTime			 = NPC->client->ps.torsoTimer;
		NPC->client->ps.fd.forcePowersActive	|= ( 1 << FP_LEVITATION );
		ucmd.forwardmove					 = 0;
		NPCInfo->jumpTime					 = 1;

		VectorClear(NPC->client->ps.moveDir);
		TIMER_Set(NPC, "duck", -level.time);

		return qtrue;
	}
	return qfalse;
}
//[/CoOp]

/*
-------------------------
NPC_ClearPathToGoal
-------------------------
*/

qboolean NPC_ClearPathToGoal( vec3_t dir, gentity_t *goal )
{
	trace_t	trace;
	float radius, dist, tFrac;

	//FIXME: What does do about area portals?  THIS IS BROKEN
	//if ( gi.inPVS( NPC->r.currentOrigin, goal->r.currentOrigin ) == qfalse )
	//	return qfalse;

	//Look ahead and see if we're clear to move to our goal position
	if ( NAV_CheckAhead( NPC, goal->r.currentOrigin, &trace, ( NPC->clipmask & ~CONTENTS_BODY )|CONTENTS_BOTCLIP ) )
	{
		//VectorSubtract( goal->r.currentOrigin, NPC->r.currentOrigin, dir );
		return qtrue;
	}

	if (!FlyingCreature(NPC))
	{
		//See if we're too far above
		if ( fabs( NPC->r.currentOrigin[2] - goal->r.currentOrigin[2] ) > 48 )
			return qfalse;
	}

	//This is a work around
	radius = ( NPC->r.maxs[0] > NPC->r.maxs[1] ) ? NPC->r.maxs[0] : NPC->r.maxs[1];
	dist = Distance( NPC->r.currentOrigin, goal->r.currentOrigin );
	tFrac = 1.0f - ( radius / dist );

	if ( trace.fraction >= tFrac )
		return qtrue;

	//See if we're looking for a navgoal
	if ( goal->flags & FL_NAVGOAL )
	{
		//Okay, didn't get all the way there, let's see if we got close enough:
		if ( NAV_HitNavGoal( trace.endpos, NPC->r.mins, NPC->r.maxs, goal->r.currentOrigin, NPCInfo->goalRadius, FlyingCreature( NPC ) ) )
		{
			//VectorSubtract(goal->r.currentOrigin, NPC->r.currentOrigin, dir);
			return qtrue;
		}
	}

	return qfalse;
}


//[CoOp]
qboolean NAV_DirSafe( gentity_t *self, vec3_t dir, float dist )
{//check to see if this NPC can move in the given direction and distance.
	vec3_t	mins, end;
	trace_t	trace;

	VectorMA( self->r.currentOrigin, dist, dir, end );

	//Offset the step height
	VectorSet( mins, self->r.mins[0], self->r.mins[1], self->r.mins[2] + STEPSIZE );
	
	trap_Trace( &trace, self->r.currentOrigin, mins, self->r.maxs, end, self->s.number, CONTENTS_BOTCLIP );

	//Do a simple check
	if ( ( trace.allsolid == qfalse ) && ( trace.startsolid == qfalse ) && ( trace.fraction == 1.0f ) )
	{
		return qtrue;
	}

	return qfalse;
}
//[/CoOp]

/*
-------------------------
NPC_CheckCombatMove
-------------------------
*/

ID_INLINE qboolean NPC_CheckCombatMove( void )
{
	//return NPCInfo->combatMove;
	if ( ( NPCInfo->goalEntity && NPC->enemy && NPCInfo->goalEntity == NPC->enemy ) || ( NPCInfo->combatMove ) )
	{
		return qtrue;
	}

	if ( NPCInfo->goalEntity && NPCInfo->watchTarget )
	{
		if ( NPCInfo->goalEntity != NPCInfo->watchTarget )
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
-------------------------
NPC_LadderMove
-------------------------
*/

static void NPC_LadderMove( vec3_t dir )
{
	//FIXME: this doesn't guarantee we're facing ladder
	//ALSO: Need to be able to get off at top
	//ALSO: Need to play an anim
	//ALSO: Need transitionary anims?
	
	if ( ( dir[2] > 0 ) || ( dir[2] < 0 && NPC->client->ps.groundEntityNum == ENTITYNUM_NONE ) )
	{
		//Set our movement direction
		ucmd.upmove = (dir[2] > 0) ? 127 : -127;

		//Don't move around on XY
		ucmd.forwardmove = ucmd.rightmove = 0;
	}
}

/*
-------------------------
NPC_GetMoveInformation
-------------------------
*/

ID_INLINE qboolean NPC_GetMoveInformation( vec3_t dir, float *distance )
{
	//NOTENOTE: Use path stacks!

	//Make sure we have somewhere to go
	if ( NPCInfo->goalEntity == NULL )
		return qfalse;

	//Get our move info
	VectorSubtract( NPCInfo->goalEntity->r.currentOrigin, NPC->r.currentOrigin, dir );
	*distance = VectorNormalize( dir );
	
	VectorCopy( NPCInfo->goalEntity->r.currentOrigin, NPCInfo->blockedDest );

	return qtrue;
}

/*
-------------------------
NAV_GetLastMove
-------------------------
*/

void NAV_GetLastMove( navInfo_t *info )
{
	*info = frameNavInfo;
}

/*
-------------------------
NPC_GetMoveDirection
-------------------------
*/

qboolean NPC_GetMoveDirection( vec3_t out, float *distance )
{
	vec3_t		angles;

	//Clear the struct
	memset( &frameNavInfo, 0, sizeof( frameNavInfo ) );

	//Get our movement, if any
	if ( NPC_GetMoveInformation( frameNavInfo.direction, &frameNavInfo.distance ) == qfalse )
		return qfalse;

	//Setup the return value
	*distance = frameNavInfo.distance;

	//For starters
	VectorCopy( frameNavInfo.direction, frameNavInfo.pathDirection );

	//If on a ladder, move appropriately
	if ( NPC->watertype & CONTENTS_LADDER )
	{
		NPC_LadderMove( frameNavInfo.direction );
		return qtrue;
	}

	//Attempt a straight move to goal
	if ( NPC_ClearPathToGoal( frameNavInfo.direction, NPCInfo->goalEntity ) == qfalse )
	{
		//See if we're just stuck
		if ( NAV_MoveToGoal( NPC, &frameNavInfo ) == WAYPOINT_NONE )
		{
			//Can't reach goal, just face
			vectoangles( frameNavInfo.direction, angles );
			NPCInfo->desiredYaw	= AngleNormalize360( angles[YAW] );		
			VectorCopy( frameNavInfo.direction, out );
			*distance = frameNavInfo.distance;
			return qfalse;
		}

		frameNavInfo.flags |= NIF_MACRO_NAV;
	}

	//Avoid any collisions on the way
	if ( NAV_AvoidCollision( NPC, NPCInfo->goalEntity, &frameNavInfo ) == qfalse )
	{
		//FIXME: Emit a warning, this is a worst case scenario
		//FIXME: if we have a clear path to our goal (exluding bodies), but then this
		//			check (against bodies only) fails, shouldn't we fall back 
		//			to macro navigation?  Like so:
		if ( !(frameNavInfo.flags&NIF_MACRO_NAV) )
		{//we had a clear path to goal and didn't try macro nav, but can't avoid collision so try macro nav here
			//See if we're just stuck
			if ( NAV_MoveToGoal( NPC, &frameNavInfo ) == WAYPOINT_NONE )
			{
				//Can't reach goal, just face
				vectoangles( frameNavInfo.direction, angles );
				NPCInfo->desiredYaw	= AngleNormalize360( angles[YAW] );		
				VectorCopy( frameNavInfo.direction, out );
				*distance = frameNavInfo.distance;
				return qfalse;
			}

			frameNavInfo.flags |= NIF_MACRO_NAV;
		}
	}

	//Setup the return values
	VectorCopy( frameNavInfo.direction, out );
	*distance = frameNavInfo.distance;

	return qtrue;
}

/*
-------------------------
NPC_GetMoveDirectionAltRoute
-------------------------
*/
extern int	NAVNEW_MoveToGoal( gentity_t *self, navInfo_t *info );
extern qboolean NAVNEW_AvoidCollision( gentity_t *self, gentity_t *goal, navInfo_t *info, qboolean setBlockedInfo, int blockedMovesLimit );
qboolean NPC_GetMoveDirectionAltRoute( vec3_t out, float *distance, qboolean tryStraight )
{
	vec3_t		angles;

	NPCInfo->aiFlags &= ~NPCAI_BLOCKED;

	//Clear the struct
	memset( &frameNavInfo, 0, sizeof( frameNavInfo ) );

	//Get our movement, if any
	if ( NPC_GetMoveInformation( frameNavInfo.direction, &frameNavInfo.distance ) == qfalse )
		return qfalse;

	//Setup the return value
	*distance = frameNavInfo.distance;

	//For starters
	VectorCopy( frameNavInfo.direction, frameNavInfo.pathDirection );

	//If on a ladder, move appropriately
	if ( NPC->watertype & CONTENTS_LADDER )
	{
		NPC_LadderMove( frameNavInfo.direction );
		return qtrue;
	}

	//Attempt a straight move to goal
	if ( !tryStraight || NPC_ClearPathToGoal( frameNavInfo.direction, NPCInfo->goalEntity ) == qfalse )
	{//blocked
		//Can't get straight to goal, use macro nav
		if ( NAVNEW_MoveToGoal( NPC, &frameNavInfo ) == WAYPOINT_NONE )
		{
			//Can't reach goal, just face
			vectoangles( frameNavInfo.direction, angles );
			NPCInfo->desiredYaw	= AngleNormalize360( angles[YAW] );		
			VectorCopy( frameNavInfo.direction, out );
			*distance = frameNavInfo.distance;
			return qfalse;
		}
		//else we are on our way
		frameNavInfo.flags |= NIF_MACRO_NAV;
	}
	else
	{//we have no architectural problems, see if there are ents inthe way and try to go around them
		//not blocked
		if ( d_altRoutes.integer )
		{//try macro nav
			navInfo_t	tempInfo;
			memcpy( &tempInfo, &frameNavInfo, sizeof( tempInfo ) );
			if ( NAVNEW_AvoidCollision( NPC, NPCInfo->goalEntity, &tempInfo, qtrue, 5 ) == qfalse )
			{//revert to macro nav
				//Can't get straight to goal, dump tempInfo and use macro nav
				if ( NAVNEW_MoveToGoal( NPC, &frameNavInfo ) == WAYPOINT_NONE )
				{
					//Can't reach goal, just face
					vectoangles( frameNavInfo.direction, angles );
					NPCInfo->desiredYaw	= AngleNormalize360( angles[YAW] );		
					VectorCopy( frameNavInfo.direction, out );
					*distance = frameNavInfo.distance;
					return qfalse;
				}
				//else we are on our way
				frameNavInfo.flags |= NIF_MACRO_NAV;
			}
			else
			{//otherwise, either clear or can avoid
				memcpy( &frameNavInfo, &tempInfo, sizeof( frameNavInfo ) );
			}
		}
		else
		{//OR: just give up
			if ( NAVNEW_AvoidCollision( NPC, NPCInfo->goalEntity, &frameNavInfo, qtrue, 30 ) == qfalse )
			{//give up
				return qfalse;
			}
		}
	}

	//Setup the return values
	VectorCopy( frameNavInfo.direction, out );
	*distance = frameNavInfo.distance;

	return qtrue;
}

void G_UcmdMoveForDir( gentity_t *self, usercmd_t *cmd, vec3_t dir )
{
	vec3_t	forward, right;
	float	fDot, rDot;

	AngleVectors( self->r.currentAngles, forward, right, NULL );

	dir[2] = 0;
	VectorNormalize( dir );
	//NPCs cheat and store this directly because converting movement into a ucmd loses precision
	VectorCopy( dir, self->client->ps.moveDir );

	fDot = DotProduct( forward, dir ) * 127.0f;
	rDot = DotProduct( right, dir ) * 127.0f;
	//Must clamp this because DotProduct is not guaranteed to return a number within -1 to 1, and that would be bad when we're shoving this into a signed byte
	if ( fDot > 127.0f )
	{
		fDot = 127.0f;
	}
	if ( fDot < -127.0f )
	{
		fDot = -127.0f;
	}
	if ( rDot > 127.0f )
	{
		rDot = 127.0f;
	}
	if ( rDot < -127.0f )
	{
		rDot = -127.0f;
	}
	cmd->forwardmove = floor(fDot);
	cmd->rightmove = floor(rDot);

	/*
	vec3_t	wishvel;
	for ( int i = 0 ; i < 3 ; i++ ) 
	{
		wishvel[i] = forward[i]*cmd->forwardmove + right[i]*cmd->rightmove;
	}
	VectorNormalize( wishvel );
	if ( !VectorCompare( wishvel, dir ) )
	{
		Com_Printf( "PRECISION LOSS: %s != %s\n", vtos(wishvel), vtos(dir) );
	}
	*/
}

/*
-------------------------
NPC_MoveToGoal

  Now assumes goal is goalEntity, was no reason for it to be otherwise
-------------------------
*/
#if	AI_TIMERS
extern int navTime;
#endif//	AI_TIMERS
qboolean NPC_MoveToGoal( qboolean tryStraight ) 
{
	float	distance;
	vec3_t	dir;

#if	AI_TIMERS
	int	startTime = GetTime(0);
#endif//	AI_TIMERS
	//If taking full body pain, don't move
	if ( PM_InKnockDown( &NPC->client->ps ) || ( ( NPC->s.legsAnim >= BOTH_PAIN1 ) && ( NPC->s.legsAnim <= BOTH_PAIN18 ) ) )
	{
		return qtrue;
	}

	/*
	if( NPC->s.eFlags & EF_LOCKED_TO_WEAPON )
	{//If in an emplaced gun, never try to navigate!
		return qtrue;
	}
	*/
	//rwwFIXMEFIXME: emplaced support

	//FIXME: if can't get to goal & goal is a target (enemy), try to find a waypoint that has line of sight to target, at least?
	//Get our movement direction
#if 1
	if ( NPC_GetMoveDirectionAltRoute( dir, &distance, tryStraight ) == qfalse )
#else
	if ( NPC_GetMoveDirection( dir, &distance ) == qfalse )
#endif
		return qfalse;

	NPCInfo->distToGoal		= distance;

	//Convert the move to angles
	vectoangles( dir, NPCInfo->lastPathAngles );
	if ( (ucmd.buttons&BUTTON_WALKING) )
	{
		NPC->client->ps.speed = NPCInfo->stats.walkSpeed;
	}
	else
	{
		NPC->client->ps.speed = NPCInfo->stats.runSpeed;
	}

	//FIXME: still getting ping-ponging in certain cases... !!!  Nav/avoidance error?  WTF???!!!
	//If in combat move, then move directly towards our goal
	if ( NPC_CheckCombatMove() )
	{//keep current facing
		G_UcmdMoveForDir( NPC, &ucmd, dir );
	}
	else
	{//face our goal
		//FIXME: strafe instead of turn if change in dir is small and temporary
		NPCInfo->desiredPitch	= 0.0f;
		NPCInfo->desiredYaw		= AngleNormalize360( NPCInfo->lastPathAngles[YAW] );
		
		//Pitch towards the goal and also update if flying or swimming
		if ( (NPC->client->ps.eFlags2&EF2_FLYING) )//moveType == MT_FLYSWIM )
		{
			NPCInfo->desiredPitch = AngleNormalize360( NPCInfo->lastPathAngles[PITCH] );
			
			if ( dir[2] )
			{
				float scale = (dir[2] * distance);
				if ( scale > 64 )
				{
					scale = 64;
				}
				else if ( scale < -64 )
				{
					scale = -64;
				}
				NPC->client->ps.velocity[2] = scale;
				//NPC->client->ps.velocity[2] = (dir[2] > 0) ? 64 : -64;
			}
		}

		//Set any final info
		ucmd.forwardmove = 127;
	}

#if	AI_TIMERS
	navTime += GetTime( startTime );
#endif//	AI_TIMERS
	return qtrue;
}

/*
-------------------------
void NPC_SlideMoveToGoal( void )

  Now assumes goal is goalEntity, if want to use tempGoal, you set that before calling the func
-------------------------
*/
qboolean NPC_SlideMoveToGoal( void )
{
	float	saveYaw = NPC->client->ps.viewangles[YAW];
	qboolean ret;

	NPCInfo->combatMove = qtrue;
	
	ret = NPC_MoveToGoal( qtrue );

	NPCInfo->desiredYaw	= saveYaw;

	return ret;
}


/*
-------------------------
NPC_ApplyRoff
-------------------------
*/

void NPC_ApplyRoff(void)
{
	BG_PlayerStateToEntityState( &NPC->client->ps, &NPC->s, qfalse );
	//VectorCopy ( NPC->r.currentOrigin, NPC->lastOrigin );
	//rwwFIXMEFIXME: Any significance to this?

	// use the precise origin for linking
	trap_LinkEntity(NPC);
}
