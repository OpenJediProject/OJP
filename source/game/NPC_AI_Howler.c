#include "b_local.h"

// These define the working combat range for these suckers
#define MIN_DISTANCE		54
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define MAX_DISTANCE		128
#define MAX_DISTANCE_SQR	( MAX_DISTANCE * MAX_DISTANCE )

#define LSTATE_CLEAR		0
#define LSTATE_WAITING		1
//[CoOp]
#define LSTATE_FLEE			2
#define LSTATE_BERZERK		3

#define HOWLER_RETREAT_DIST	300.0f
#define HOWLER_PANIC_HEALTH	10
//[/CoOp]

//[CoOp]
static void Howler_Attack( float enemyDist, qboolean howl );
extern qboolean NPC_TryJump_Gent(gentity_t *goal,	float max_xy_dist, float max_z_diff);
extern void G_GetBoltPosition( gentity_t *self, int boltIndex, vec3_t pos, int modelIndex ); //NPC_utils.c
extern qboolean NAV_DirSafe( gentity_t *self, vec3_t dir, float dist );
//[/CoOp]

/*
-------------------------
NPC_Howler_Precache
-------------------------
*/
void NPC_Howler_Precache( void )
{
	//[CoOp]
	//added howler precashe from SP
	int i;
	G_EffectIndex( "howler/sonic" );
	G_SoundIndex( "sound/chars/howler/howl.mp3" );
	for ( i = 1; i < 3; i++ )
	{
		G_SoundIndex( va( "sound/chars/howler/idle_hiss%d.mp3", i ) );
	}
	for ( i = 1; i < 6; i++ )
	{
		G_SoundIndex( va( "sound/chars/howler/howl_talk%d.mp3", i ) );
		G_SoundIndex( va( "sound/chars/howler/howl_yell%d.mp3", i ) );
	}
	//[/CoOp]
}

//[CoOp]
//added from SP code
void Howler_ClearTimers( gentity_t *self )
{
	//clear all my timers
	TIMER_Set( self, "flee", -level.time );
	TIMER_Set( self, "retreating", -level.time );
	TIMER_Set( self, "standing", -level.time );
	TIMER_Set( self, "walking", -level.time );
	TIMER_Set( self, "running", -level.time );
	TIMER_Set( self, "aggressionDecay", -level.time );
	TIMER_Set( self, "speaking", -level.time );
}


//added from SP
static qboolean NPC_Howler_Move( int randomJumpChance )
{
	if ( !TIMER_Done( NPC, "standing" ) )
	{//standing around
		return qfalse;
	}
	if ( NPC->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//in air, don't do anything
		return qfalse;
	}
	if ( (!NPC->enemy&&TIMER_Done( NPC, "running" )) || !TIMER_Done( NPC, "walking" ) )
	{
		ucmd.buttons |= BUTTON_WALKING;
	}
	if ( (!randomJumpChance||Q_irand( 0, randomJumpChance ))
		&& NPC_MoveToGoal( qtrue ) )
	{
		if ( VectorCompare( NPC->client->ps.moveDir, vec3_origin )
			|| !NPC->client->ps.speed )
		{//uh.... wtf?  Got there?
			if ( NPCInfo->goalEntity )
			{
				NPC_FaceEntity( NPCInfo->goalEntity, qfalse );
			}
			else
			{
				NPC_UpdateAngles( qfalse, qtrue );
			}
			return qtrue;
		}
		//TEMP: don't want to strafe
		VectorClear( NPC->client->ps.moveDir );
		ucmd.rightmove = 0.0f;
//		Com_Printf( "Howler moving %d\n",ucmd.forwardmove );
		//if backing up, go slow...
		if ( ucmd.forwardmove < 0.0f )
		{
			ucmd.buttons |= BUTTON_WALKING;
			//if ( NPC->client->ps.speed > NPCInfo->stats.walkSpeed )
			{//don't walk faster than I'm allowed to
				NPC->client->ps.speed = NPCInfo->stats.walkSpeed;
			}
		}
		else
		{
			if ( (ucmd.buttons&BUTTON_WALKING) )
			{
				NPC->client->ps.speed = NPCInfo->stats.walkSpeed;
			}
			else
			{
				NPC->client->ps.speed = NPCInfo->stats.runSpeed;
			}
		}
		NPCInfo->lockedDesiredYaw = NPCInfo->desiredYaw = NPCInfo->lastPathAngles[YAW];
		NPC_UpdateAngles( qfalse, qtrue );
	}
	else if ( NPCInfo->goalEntity )
	{//couldn't get where we wanted to go, try to jump there
		NPC_FaceEntity( NPCInfo->goalEntity, qfalse );
		NPC_TryJump_Gent( NPCInfo->goalEntity, 400.0f, -256.0f );
	}
	return qtrue;
}
//[/CoOp]


/*
-------------------------
Howler_Idle
-------------------------
*/
void Howler_Idle( void )
{
}


/*
-------------------------
Howler_Patrol
-------------------------
*/
//[CoOp]
//Replaced with SP version
static void Howler_Patrol( void )
{
	gentity_t *ClosestPlayer = FindClosestPlayer(NPC->r.currentOrigin, NPC->client->enemyTeam);

	NPCInfo->localState = LSTATE_CLEAR;

	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		NPC_Howler_Move( 100 );
	}

	if(ClosestPlayer)
	{//attack enemy players that are close.
		if(Distance(ClosestPlayer->r.currentOrigin, NPC->r.currentOrigin) < 256 * 256)
		{
			G_SetEnemy( NPC, ClosestPlayer );
		}
	}

	if ( NPC_CheckEnemyExt( qtrue ) == qfalse )
	{
		Howler_Idle();
		return;
	}

	Howler_Attack( 0.0f, qtrue );
}

/* 
void Howler_Patrol( void )
{
	vec3_t dif;

	NPCInfo->localState = LSTATE_CLEAR;

	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		ucmd.buttons &= ~BUTTON_WALKING;
		NPC_MoveToGoal( qtrue );
	}
	else
	{
		if ( TIMER_Done( NPC, "patrolTime" ))
		{
			TIMER_Set( NPC, "patrolTime", crandom() * 5000 + 5000 );
		}
	}

	//rwwFIXMEFIXME: Care about all clients, not just client 0
	VectorSubtract( g_entities[0].r.currentOrigin, NPC->r.currentOrigin, dif );

	if ( VectorLengthSquared( dif ) < 256 * 256 )
	{
		G_SetEnemy( NPC, &g_entities[0] );
	}

	if ( NPC_CheckEnemyExt( qtrue ) == qfalse )
	{
		Howler_Idle();
		return;
	}
}
*/

 
/*
-------------------------
Howler_Move
-------------------------
*/
//replaced with SP version
static qboolean Howler_Move( qboolean visible )
{
	if ( NPCInfo->localState != LSTATE_WAITING )
	{
		NPCInfo->goalEntity = NPC->enemy;
		NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range
		return NPC_Howler_Move( 30 );
	}
	return qfalse;
}
/*
void Howler_Move( qboolean visible )
{
	if ( NPCInfo->localState != LSTATE_WAITING )
	{
		NPCInfo->goalEntity = NPC->enemy;
		NPC_MoveToGoal( qtrue );
		NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range
	}
}
*/


//---------------------------------------------------------
//replaced with SP version
extern qboolean PM_InKnockDown( playerState_t *ps );
extern void G_Knockdown( gentity_t *victim );
static void Howler_TryDamage( int damage, qboolean tongue, qboolean knockdown )
{
	vec3_t	start, end, dir;
	trace_t	tr;
	float dist;

	if ( tongue )
	{
		G_GetBoltPosition( NPC, NPC->NPC->genericBolt1, start, 0 );
		G_GetBoltPosition( NPC, NPC->NPC->genericBolt2, end, 0 );
		VectorSubtract( end, start, dir );
		dist = VectorNormalize( dir );
		VectorMA( start, dist+16, dir, end );
	}
	else
	{
		VectorCopy( NPC->r.currentOrigin, start );
		AngleVectors( NPC->r.currentAngles, dir, NULL, NULL );
		VectorMA( start, MIN_DISTANCE*2, dir, end );
	}
/* RACC - don't need this for now.
#ifndef FINAL_BUILD
	if ( d_saberCombat.integer > 1 )
	{
		G_DebugLine(start, end, 1000, 0x000000ff, qtrue);
	}
#endif
*/
	// Should probably trace from the mouth, but, ah well.
	trap_Trace( &tr, start, vec3_origin, vec3_origin, end, NPC->s.number, MASK_SHOT );

	if ( tr.entityNum < ENTITYNUM_WORLD )
	{//hit *something*
		gentity_t *victim = &g_entities[tr.entityNum];
		if ( !victim->client
			|| victim->client->NPC_class != CLASS_HOWLER )
		{//not another howler

			if ( knockdown && victim->client )
			{//only do damage if victim isn't knocked down.  If he isn't, knock him down
				if ( PM_InKnockDown( &victim->client->ps ) )
				{
					return;
				}
			}
			//FIXME: some sort of damage effect (claws and tongue are cutting you... blood?)
			G_Damage( victim, NPC, NPC, dir, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
			if ( knockdown && victim->health > 0 )
			{//victim still alive
				//RACC - only changed this until we fix up knockdowns
				G_Knockdown(victim);
				//G_Knockdown( victim, NPC, NPC->client->ps.velocity, 500, qfalse );
			}
		}
	}
}
/*
void Howler_TryDamage( gentity_t *enemy, int damage )
{
	vec3_t	end, dir;
	trace_t	tr;

	if ( !enemy )
	{
		return;
	}

	AngleVectors( NPC->client->ps.viewangles, dir, NULL, NULL );
	VectorMA( NPC->r.currentOrigin, MIN_DISTANCE, dir, end );

	// Should probably trace from the mouth, but, ah well.
	trap_Trace( &tr, NPC->r.currentOrigin, vec3_origin, vec3_origin, end, NPC->s.number, MASK_SHOT );

	if ( tr.entityNum != ENTITYNUM_WORLD )
	{
		G_Damage( &g_entities[tr.entityNum], NPC, NPC, dir, tr.endpos, damage, DAMAGE_NO_KNOCKBACK, MOD_MELEE );
	}
}
*/


//Moved in from SP
extern int NPC_GetEntsNearBolt( int *radiusEnts, float radius, int boltIndex, vec3_t boltOrg );
extern float NPC_EntRangeFromBolt( gentity_t *targEnt, int boltIndex );
static void Howler_Howl( void )
{
	//gentity_t	*radiusEnts[ 128 ];
	int			radiusEntsNums[128];
	gentity_t	*ent;
	int			numEnts;
	const float	radius = (NPC->spawnflags&1)?256:128;
	const float	halfRadSquared = ((radius/2)*(radius/2));
	const float	radiusSquared = (radius*radius);
	float		distSq;
	int			i;
	vec3_t		boltOrg;

	AddSoundEvent( NPC, NPC->r.currentOrigin, 512, AEL_DANGER, qfalse, qtrue );

	//RACC - handLBolt never defined for Howlers? *shrug* just use the tongue base
	numEnts = NPC_GetEntsNearBolt( radiusEntsNums, radius, NPC->NPC->genericBolt1, boltOrg );
	//numEnts = NPC_GetEntsNearBolt( radiusEnts, radius, NPC->handLBolt, boltOrg );

	for ( i = 0; i < numEnts; i++ )
	{
		ent = &g_entities[radiusEntsNums[i]];

		if ( !ent->inuse )
		{
			continue;
		}
		
		if ( ent == NPC )
		{//Skip the rancor ent
			//RACC - assume this means the source howler
			continue;
		}
		
		if ( ent->client == NULL )
		{//must be a client
			continue;
		}

		if ( ent->client->NPC_class == CLASS_HOWLER )
		{//other howlers immune
			continue;
		}

		distSq = DistanceSquared( ent->r.currentOrigin, boltOrg );
		if ( distSq <= radiusSquared )
		{
			if ( distSq < halfRadSquared )
			{//close enough to do damage, too
				if ( Q_irand( 0, g_spskill.integer ) )
				{//does no damage on easy, does 1 point every other frame on medium, more often on hard
					//RACC - Impact isn't a MOD in MP.
					G_Damage( ent, NPC, NPC, vec3_origin, NPC->r.currentOrigin, 1, DAMAGE_NO_KNOCKBACK, MOD_CRUSH );
					//G_Damage( ent, NPC, NPC, vec3_origin, NPC->r.currentOrigin, 1, DAMAGE_NO_KNOCKBACK, MOD_IMPACT );
				}
			}
			if ( ent->health > 0 
				&& ent->client
				&& ent->client->NPC_class != CLASS_RANCOR
				&& ent->client->NPC_class != CLASS_ATST 
				&& !PM_InKnockDown( &ent->client->ps ) )
			{
				if ( BG_HasAnimation( ent->localAnimIndex, BOTH_SONICPAIN_START ) )
				{
					if ( ent->client->ps.torsoAnim != BOTH_SONICPAIN_START 
						&& ent->client->ps.torsoAnim != BOTH_SONICPAIN_HOLD )
					{
						NPC_SetAnim( ent, SETANIM_LEGS, BOTH_SONICPAIN_START, SETANIM_FLAG_NORMAL );
						NPC_SetAnim( ent, SETANIM_TORSO, BOTH_SONICPAIN_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						ent->client->ps.torsoTimer += 100;
						ent->client->ps.weaponTime = ent->client->ps.torsoTimer;
					}
					else if ( ent->client->ps.torsoTimer <= 100 )
					{//at the end of the sonic pain start or hold anim
						NPC_SetAnim( ent, SETANIM_LEGS, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_NORMAL );
						NPC_SetAnim( ent, SETANIM_TORSO, BOTH_SONICPAIN_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						ent->client->ps.torsoTimer += 100; 
						ent->client->ps.weaponTime = ent->client->ps.torsoTimer;
					}
				}
				/*
				else if ( distSq < halfRadSquared 
					&& radiusEnts[i]->client->ps.groundEntityNum != ENTITYNUM_NONE 
					&& !Q_irand( 0, 10 ) )//FIXME: base on skill
				{//within range
					G_Knockdown( radiusEnts[i], NPC, vec3_origin, 500, qfalse );
				}
				*/
			}
		}

		//camera shaking
		distSq = Distance( boltOrg, ent->r.currentOrigin );
		if ( distSq < 256.0f )
		{
			G_ScreenShake(boltOrg, ent, 1.0f*distSq/128.0f, 200, qfalse);
		}
	}

	//RACC - changed to work for multiple players
	/*
	playerDist = NPC_EntRangeFromBolt( player, NPC->genericBolt1 );
	if ( playerDist < 256.0f )
	{
		CGCam_Shake( 1.0f*playerDist/128.0f, 200 );
	}
	*/
}


//------------------------------
//replaced with SP version
void G_SoundOnEnt( gentity_t *ent, int channel, const char *soundPath );
static void Howler_Attack( float enemyDist, qboolean howl )
{
	int dmg = (NPCInfo->localState==LSTATE_BERZERK)?5:2;

	vec3_t boltOrg;
	vec3_t fwd;

	if ( !TIMER_Exists( NPC, "attacking" ))
	{
		int attackAnim = BOTH_GESTURE1;
		// Going to do an attack
		if ( NPC->enemy && NPC->enemy->client && PM_InKnockDown( &NPC->enemy->client->ps )
			&& enemyDist <= MIN_DISTANCE )
		{
			attackAnim = BOTH_ATTACK2;
		}
		else if ( !Q_irand( 0, 4 ) || howl )
		{//howl attack
			//G_SoundOnEnt( NPC, CHAN_VOICE, "sound/chars/howler/howl.mp3" );
		}
		else if ( enemyDist > MIN_DISTANCE && Q_irand( 0, 1 ) )
		{//lunge attack
			//jump foward
			vec3_t	fwd, yawAng;
			
			VectorSet( yawAng, 0, NPC->client->ps.viewangles[YAW], 0 );
			AngleVectors( yawAng, fwd, NULL, NULL );
			VectorScale( fwd, (enemyDist*3.0f), NPC->client->ps.velocity );
			NPC->client->ps.velocity[2] = 200;
			NPC->client->ps.groundEntityNum = ENTITYNUM_NONE;
			
			attackAnim = BOTH_ATTACK1;
		}
		else
		{//tongue attack
			attackAnim = BOTH_ATTACK2;
		}

		NPC_SetAnim( NPC, SETANIM_BOTH, attackAnim, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART );
		if ( NPCInfo->localState == LSTATE_BERZERK )
		{//attack again right away
			TIMER_Set( NPC, "attacking", NPC->client->ps.legsTimer );
		}
		else
		{
			TIMER_Set( NPC, "attacking", NPC->client->ps.legsTimer + Q_irand( 0, 1500 ) );//FIXME: base on skill
			TIMER_Set( NPC, "standing", -level.time );
			TIMER_Set( NPC, "walking", -level.time );
			TIMER_Set( NPC, "running", NPC->client->ps.legsTimer + 5000 );
		}

		TIMER_Set( NPC, "attack_dmg", 200 ); // level two damage
	}

	// Need to do delayed damage since the attack animations encapsulate multiple mini-attacks
	switch ( NPC->client->ps.legsAnim )
	{
	case BOTH_ATTACK1:
	case BOTH_MELEE1:
		if ( NPC->client->ps.legsTimer > 650//more than 13 frames left
			&& BG_AnimLength( NPC->localAnimIndex, (animNumber_t)NPC->client->ps.legsAnim ) - NPC->client->ps.legsTimer >= 800 )//at least 16 frames into anim
		{
			Howler_TryDamage( dmg, qfalse, qfalse );
		}
		break;
	case BOTH_ATTACK2:
	case BOTH_MELEE2:
		if ( NPC->client->ps.legsTimer > 350//more than 7 frames left
			&& BG_AnimLength( NPC->localAnimIndex, (animNumber_t)NPC->client->ps.legsAnim ) - NPC->client->ps.legsTimer >= 550 )//at least 11 frames into anim
		{
			Howler_TryDamage( dmg, qtrue, qfalse );
		}
		break;
	case BOTH_GESTURE1:
		{
			if ( NPC->client->ps.legsTimer > 1800//more than 36 frames left
				&& BG_AnimLength( NPC->localAnimIndex, (animNumber_t)NPC->client->ps.legsAnim ) - NPC->client->ps.legsTimer >= 950 )//at least 19 frames into anim
			{
				Howler_Howl();
				if ( !NPC->count )
				{
					//RAFIXME - this probably won't work correctly.
					G_GetBoltPosition(NPC, NPC->NPC->genericBolt1, boltOrg, 0);
					AngleVectors( NPC->client->ps.viewangles, fwd, NULL, NULL );
					G_PlayEffectID( G_EffectIndex( "howler/sonic" ), boltOrg, fwd);
					//G_PlayEffect( G_EffectIndex( "howler/sonic" ), NPC->ghoul2, NPC->NPC->genericBolt1, NPC->s.number, NPC->currentOrigin, 4750, qtrue );
					G_SoundOnEnt( NPC, CHAN_VOICE, "sound/chars/howler/howl.mp3" );
					NPC->count = 1;
				}
			}
		}
		break;
	default:
		//anims seem to get reset after a load, so just stop attacking and it will restart as needed.
		TIMER_Remove( NPC, "attacking" );
		break;
	}

	// Just using this to remove the attacking flag at the right time
	TIMER_Done2( NPC, "attacking", qtrue );
}
/*
void Howler_Attack( void )
{
	if ( !TIMER_Exists( NPC, "attacking" ))
	{
		// Going to do ATTACK1
		TIMER_Set( NPC, "attacking", 1700 + random() * 200 );
		NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_ATTACK1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );

		TIMER_Set( NPC, "attack_dmg", 200 ); // level two damage
	}

	// Need to do delayed damage since the attack animations encapsulate multiple mini-attacks
	if ( TIMER_Done2( NPC, "attack_dmg", qtrue ))
	{
		Howler_TryDamage( NPC->enemy, 5 );
	}

	// Just using this to remove the attacking flag at the right time
	TIMER_Done2( NPC, "attacking", qtrue );
}
*/


//----------------------------------
//replaced with SP version.
static void Howler_Combat( void )
{
	qboolean	faced = qfalse;
	float		distance;	
	qboolean	advance = qfalse;
	if ( NPC->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//not on the ground
		if ( NPC->client->ps.legsAnim == BOTH_JUMP1
			|| NPC->client->ps.legsAnim == BOTH_INAIR1 )
		{//flying through the air with the greatest of ease, etc
			Howler_TryDamage( 10, qfalse, qfalse );
		}
	}
	else
	{//not in air, see if we should attack or advance
		// If we cannot see our target or we have somewhere to go, then do that
		if ( !NPC_ClearLOS4( NPC->enemy ) )//|| UpdateGoal( ))
		{
			NPCInfo->goalEntity = NPC->enemy;
			NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range

			if ( NPCInfo->localState == LSTATE_BERZERK )
			{
				NPC_Howler_Move( 3 );
			}
			else
			{
				NPC_Howler_Move( 10 );
			}
			NPC_UpdateAngles( qfalse, qtrue );
			return;
		}

		distance = DistanceHorizontal( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );	

		if ( NPC->enemy && NPC->enemy->client && PM_InKnockDown( &NPC->enemy->client->ps ) )
		{//get really close to knocked down enemies
			advance = (qboolean)( distance > MIN_DISTANCE ? qtrue : qfalse  );
		}
		else
		{
			advance = (qboolean)( distance > MAX_DISTANCE ? qtrue : qfalse  );//MIN_DISTANCE
		}

		if (( advance || NPCInfo->localState == LSTATE_WAITING ) && TIMER_Done( NPC, "attacking" )) // waiting monsters can't attack
		{
			if ( TIMER_Done2( NPC, "takingPain", qtrue ))
			{
				NPCInfo->localState = LSTATE_CLEAR;
			}
			else if ( TIMER_Done( NPC, "standing" ) )
			{
				faced = Howler_Move( 1 );
			}
		}
		else
		{
			Howler_Attack( distance, qfalse );
		}
	}

	if ( !faced )
	{
		if ( //TIMER_Done( NPC, "standing" ) //not just standing there
			//!advance //not moving
			TIMER_Done( NPC, "attacking" ) )// not attacking
		{//not standing around
			// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
			NPC_FaceEnemy( qtrue );
		}
		else
		{
			NPC_UpdateAngles( qfalse, qtrue );
		}
	}
}

/*
void Howler_Combat( void )
{
	float distance;
	qboolean advance;

	// If we cannot see our target or we have somewhere to go, then do that
	if ( !NPC_ClearLOS4( NPC->enemy ) || UpdateGoal( ))
	{
		NPCInfo->combatMove = qtrue;
		NPCInfo->goalEntity = NPC->enemy;
		NPCInfo->goalRadius = MAX_DISTANCE;	// just get us within combat range

		NPC_MoveToGoal( qtrue );
		return;
	}

	// Sometimes I have problems with facing the enemy I'm attacking, so force the issue so I don't look dumb
	NPC_FaceEnemy( qtrue );

	distance	= DistanceHorizontalSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );	
	advance = (qboolean)( distance > MIN_DISTANCE_SQR ? qtrue : qfalse  );

	if (( advance || NPCInfo->localState == LSTATE_WAITING ) && TIMER_Done( NPC, "attacking" )) // waiting monsters can't attack
	{
		if ( TIMER_Done2( NPC, "takingPain", qtrue ))
		{
			NPCInfo->localState = LSTATE_CLEAR;
		}
		else
		{
			Howler_Move( 1 );
		}
	}
	else
	{
		Howler_Attack();
	}
}
*/

/*
-------------------------
NPC_Howler_Pain
-------------------------
*/
//replaced with SP code
void NPC_Howler_Pain( gentity_t *self, gentity_t *inflictor, gentity_t *other, const vec3_t point, int damage, int mod,int hitLoc ) 
{
	if ( !self || !self->NPC )
	{
		return;
	}

	if ( self->NPC->localState != LSTATE_BERZERK )//damage >= 10 )
	{
		self->NPC->stats.aggression += damage;
		self->NPC->localState = LSTATE_WAITING;

		TIMER_Remove( self, "attacking" );

		VectorCopy( self->NPC->lastPathAngles, self->s.angles );

		//if ( self->client->ps.legsAnim == BOTH_GESTURE1 )
		//RAFIXME - effects can't be stopped at this point
		/*
		{
			G_StopEffect( G_EffectIndex( "howler/sonic" ), self->playerModel, self->genericBolt1, self->s.number );
		}
		*/

		NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );
		TIMER_Set( self, "takingPain", self->client->ps.legsTimer );//2900 );

		if ( self->health > HOWLER_PANIC_HEALTH )
		{//still have some health left
			if ( Q_irand( 0, self->NPC->stats.health ) > self->health )//FIXME: or check damage?
			{//back off!
				TIMER_Set( self, "standing", -level.time );
				TIMER_Set( self, "running", -level.time );
				TIMER_Set( self, "walking", -level.time );
				TIMER_Set( self, "retreating", Q_irand( 1000, 5000 ) );
			}
			else
			{//go after him!
				TIMER_Set( self, "standing", -level.time );
				TIMER_Set( self, "running", self->client->ps.legsTimer+Q_irand(3000,6000) );
				TIMER_Set( self, "walking", -level.time );
				TIMER_Set( self, "retreating", -level.time );
			}
		}
		else if ( self->NPC )
		{//panic!
			if ( Q_irand( 0, 1 ) )
			{//berzerk
				self->NPC->localState = LSTATE_BERZERK;
			}
			else
			{//flee
				self->NPC->localState = LSTATE_FLEE;
				TIMER_Set( self, "flee", Q_irand( 10000, 30000 ) );
			}
		}
	}
}

/*
void NPC_Howler_Pain( gentity_t *self, gentity_t *attacker, int damage ) 
{
	if ( damage >= 10 )
	{
		TIMER_Remove( self, "attacking" );
		TIMER_Set( self, "takingPain", 2900 );

		VectorCopy( self->NPC->lastPathAngles, self->s.angles );

		NPC_SetAnim( self, SETANIM_BOTH, BOTH_PAIN1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD );

		if ( self->NPC )
		{
			self->NPC->localState = LSTATE_WAITING;
		}
	}
}
*/


/*
-------------------------
NPC_BSHowler_Default
-------------------------
*/
//replaced with SP version.
void NPC_BSHowler_Default( void )
{
	if ( NPC->client->ps.legsAnim != BOTH_GESTURE1 )
	{
		NPC->count = 0;
	}
	//FIXME: if in jump, do damage in front and maybe knock them down?
	if ( !TIMER_Done( NPC, "attacking" ) )
	{
		if ( NPC->enemy )
		{
			//NPC_FaceEnemy( qfalse );
			Howler_Attack( Distance( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin ), qfalse );
		}
		else
		{
			//NPC_UpdateAngles( qfalse, qtrue );
			Howler_Attack( 0.0f, qfalse );
		}
		NPC_UpdateAngles( qfalse, qtrue );
		return;
	}

	if ( NPC->enemy )
	{
		if ( NPCInfo->stats.aggression > 0 )
		{
			if ( TIMER_Done( NPC, "aggressionDecay" ) )
			{
				NPCInfo->stats.aggression--;
				TIMER_Set( NPC, "aggressionDecay", 500 );
			}
		}
		//RAFIXME - No fleeing, need to fix this at some point.
		/*
		if ( !TIMER_Done( NPC, "flee" ) 
			&& NPC_BSFlee() )	//this can clear ENEMY
		{//successfully trying to run away
			return;
		}
		*/
		if ( NPC->enemy == NULL)
		{
			NPC_UpdateAngles( qfalse, qtrue );
			return;
		}
		if ( NPCInfo->localState == LSTATE_FLEE )
		{//we were fleeing, now done (either timer ran out or we cannot flee anymore
			if ( NPC_ClearLOS4( NPC->enemy ) )
			{//if enemy is still around, go berzerk
				NPCInfo->localState = LSTATE_BERZERK;
			}
			else
			{//otherwise, lick our wounds?
				NPCInfo->localState = LSTATE_CLEAR;
				TIMER_Set( NPC, "standing", Q_irand( 3000, 10000 ) );
			}
		}
		else if ( NPCInfo->localState == LSTATE_BERZERK )
		{//go nuts!
		}
		else if ( NPCInfo->stats.aggression >= Q_irand( 75, 125 ) )
		{//that's it, go nuts!
			NPCInfo->localState = LSTATE_BERZERK;
		}
		else if ( !TIMER_Done( NPC, "retreating" ) )
		{//trying to back off
			NPC_FaceEnemy( qtrue );
			if ( NPC->client->ps.speed > NPCInfo->stats.walkSpeed )
			{
				NPC->client->ps.speed = NPCInfo->stats.walkSpeed;
			}
			ucmd.buttons |= BUTTON_WALKING;
			if ( Distance( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin ) < HOWLER_RETREAT_DIST )
			{//enemy is close
				vec3_t moveDir;
				AngleVectors( NPC->r.currentAngles, moveDir, NULL, NULL );
				VectorScale( moveDir, -1, moveDir );
				if ( !NAV_DirSafe( NPC, moveDir, 8 ) )
				{//enemy is backing me up against a wall or ledge!  Start to get really mad!
					NPCInfo->stats.aggression += 2;
				}
				else
				{//back off
					ucmd.forwardmove = -127;
				}
				//enemy won't leave me alone, get mad...
				NPCInfo->stats.aggression++;
			}
			return;
		}
		else if ( TIMER_Done( NPC, "standing" ) )
		{//not standing around
			if ( !(NPCInfo->last_ucmd.forwardmove)
				&& !(NPCInfo->last_ucmd.rightmove) )
			{//stood last frame
				if ( TIMER_Done( NPC, "walking" ) 
					&& TIMER_Done( NPC, "running" ) )
				{//not walking or running
					if ( Q_irand( 0, 2 ) )
					{//run for a while
						TIMER_Set( NPC, "walking", Q_irand( 4000, 8000 ) );
					}
					else
					{//walk for a bit
						TIMER_Set( NPC, "running", Q_irand( 2500, 5000 ) );
					}
				}
			}
			else if ( (NPCInfo->last_ucmd.buttons&BUTTON_WALKING) )
			{//walked last frame
				if ( TIMER_Done( NPC, "walking" ) )
				{//just finished walking
					if ( Q_irand( 0, 5 ) || DistanceSquared( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin ) < MAX_DISTANCE_SQR )
					{//run for a while
						TIMER_Set( NPC, "running", Q_irand( 4000, 20000 ) );
					}
					else
					{//stand for a bit
						TIMER_Set( NPC, "standing", Q_irand( 2000, 6000 ) );
					}
				}
			}
			else
			{//ran last frame
				if ( TIMER_Done( NPC, "running" ) )
				{//just finished running
					if ( Q_irand( 0, 8 ) || DistanceSquared( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin ) < MAX_DISTANCE_SQR )
					{//walk for a while
						TIMER_Set( NPC, "walking", Q_irand( 3000, 10000 ) );
					}
					else
					{//stand for a bit
						TIMER_Set( NPC, "standing", Q_irand( 2000, 6000 ) );
					}
				}
			}
		}
		if ( NPC_ValidEnemy( NPC->enemy ) == qfalse )
		{
			TIMER_Remove( NPC, "lookForNewEnemy" );//make them look again right now
			if ( !NPC->enemy->inuse || level.time - NPC->enemy->s.time > Q_irand( 10000, 15000 ) )
			{//it's been a while since the enemy died, or enemy is completely gone, get bored with him
				NPC->enemy = NULL;
				Howler_Patrol();
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
		}
		if ( TIMER_Done( NPC, "lookForNewEnemy" ) )
		{
			gentity_t *sav_enemy = NPC->enemy;//FIXME: what about NPC->lastEnemy?
			gentity_t *newEnemy;
			NPC->enemy = NULL;
			newEnemy = NPC_CheckEnemy( NPCInfo->confusionTime < level.time, qfalse, qfalse );
			NPC->enemy = sav_enemy;
			if ( newEnemy && newEnemy != sav_enemy )
			{//picked up a new enemy!
				NPC->lastEnemy = NPC->enemy;
				G_SetEnemy( NPC, newEnemy );
				if ( NPC->enemy != NPC->lastEnemy )
				{//clear this so that we only sniff the player the first time we pick them up
					//RACC - this doesn't appear to get used for Howlers
					//NPC->useDebounceTime = 0;
				}
				//hold this one for at least 5-15 seconds
				TIMER_Set( NPC, "lookForNewEnemy", Q_irand( 5000, 15000 ) );
			}
			else
			{//look again in 2-5 secs
				TIMER_Set( NPC, "lookForNewEnemy", Q_irand( 2000, 5000 ) );
			}
		}
		Howler_Combat();
		if ( TIMER_Done( NPC, "speaking" ) )
		{
			if ( !TIMER_Done( NPC, "standing" )
				|| !TIMER_Done( NPC, "retreating" ))
			{
				G_SoundOnEnt( NPC, CHAN_VOICE, va( "sound/chars/howler/idle_hiss%d.mp3", Q_irand( 1, 2 ) ) );
			}
			else if ( !TIMER_Done( NPC, "walking" ) 
				|| NPCInfo->localState == LSTATE_FLEE )
			{
				G_SoundOnEnt( NPC, CHAN_VOICE, va( "sound/chars/howler/howl_talk%d.mp3", Q_irand( 1, 5 ) ) );
			}
			else
			{
				G_SoundOnEnt( NPC, CHAN_VOICE, va( "sound/chars/howler/howl_yell%d.mp3", Q_irand( 1, 5 ) ) );
			}
			if ( NPCInfo->localState == LSTATE_BERZERK
				|| NPCInfo->localState == LSTATE_FLEE )
			{
				TIMER_Set( NPC, "speaking", Q_irand( 1000, 4000 ) );
			}
			else
			{
				TIMER_Set( NPC, "speaking", Q_irand( 3000, 8000 ) );
			}
		}
		return;
	}
	else
	{
		if ( TIMER_Done( NPC, "speaking" ) )
		{
			if ( !Q_irand( 0, 3 ) )
			{
				G_SoundOnEnt( NPC, CHAN_VOICE, va( "sound/chars/howler/idle_hiss%d.mp3", Q_irand( 1, 2 ) ) );
			}
			else
			{
				G_SoundOnEnt( NPC, CHAN_VOICE, va( "sound/chars/howler/howl_talk%d.mp3", Q_irand( 1, 5 ) ) );
			}
			TIMER_Set( NPC, "speaking", Q_irand( 4000, 12000 ) );
		}
		if ( NPCInfo->stats.aggression > 0 )
		{
			if ( TIMER_Done( NPC, "aggressionDecay" ) )
			{
				NPCInfo->stats.aggression--;
				TIMER_Set( NPC, "aggressionDecay", 200 );
			}
		}
		if ( TIMER_Done( NPC, "standing" ) )
		{//not standing around
			if ( !(NPCInfo->last_ucmd.forwardmove)
				&& !(NPCInfo->last_ucmd.rightmove) )
			{//stood last frame
				if ( TIMER_Done( NPC, "walking" ) 
					&& TIMER_Done( NPC, "running" ) )
				{//not walking or running
					if ( NPCInfo->goalEntity )
					{//have somewhere to go
						if ( Q_irand( 0, 2 ) )
						{//walk for a while
							TIMER_Set( NPC, "walking", Q_irand( 3000, 10000 ) );
						}
						else
						{//run for a bit
							TIMER_Set( NPC, "running", Q_irand( 2500, 5000 ) );
						}
					}
				}
			}
			else if ( (NPCInfo->last_ucmd.buttons&BUTTON_WALKING) )
			{//walked last frame
				if ( TIMER_Done( NPC, "walking" ) )
				{//just finished walking
					if ( Q_irand( 0, 3 ) )
					{//run for a while
						TIMER_Set( NPC, "running", Q_irand( 3000, 6000 ) );
					}
					else
					{//stand for a bit
						TIMER_Set( NPC, "standing", Q_irand( 2500, 5000 ) );
					}
				}
			}
			else
			{//ran last frame
				if ( TIMER_Done( NPC, "running" ) )
				{//just finished running
					if ( Q_irand( 0, 2 ) )
					{//walk for a while
						TIMER_Set( NPC, "walking", Q_irand( 6000, 15000 ) );
					}
					else
					{//stand for a bit
						TIMER_Set( NPC, "standing", Q_irand( 4000, 6000 ) );
					}
				}
			}
		}
		if ( NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
		{
			Howler_Patrol();
		}
		else
		{
			Howler_Idle();
		}
	}

	NPC_UpdateAngles( qfalse, qtrue );
}

/*
void NPC_BSHowler_Default( void )
{
	if ( NPC->enemy )
	{
		Howler_Combat();
	}
	else if ( NPCInfo->scriptFlags & SCF_LOOK_FOR_ENEMIES )
	{
		Howler_Patrol();
	}
	else
	{
		Howler_Idle();
	}

	NPC_UpdateAngles( qtrue, qtrue );
}
*/
