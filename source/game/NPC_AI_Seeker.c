//[SPPortComplete]
#include "b_local.h"
#include "g_nav.h"

extern void Boba_FireDecide( void );

void Seeker_Strafe( void );

#define VELOCITY_DECAY		0.7f

#define	MIN_MELEE_RANGE		320
#define	MIN_MELEE_RANGE_SQR	( MIN_MELEE_RANGE * MIN_MELEE_RANGE )

#define MIN_DISTANCE		80
#define MIN_DISTANCE_SQR	( MIN_DISTANCE * MIN_DISTANCE )

#define SEEKER_STRAFE_VEL	100
#define SEEKER_STRAFE_DIS	200
#define SEEKER_UPWARD_PUSH	32

#define SEEKER_FORWARD_BASE_SPEED	10
#define SEEKER_FORWARD_MULTIPLIER	2

#define SEEKER_SEEK_RADIUS			1024

//------------------------------------
void NPC_Seeker_Precache(void)
{
	G_SoundIndex("sound/chars/seeker/misc/fire.wav");
	G_SoundIndex( "sound/chars/seeker/misc/hiss.wav");
	G_EffectIndex( "env/small_explode");
}

//------------------------------------
void NPC_Seeker_Pain(gentity_t *self, gentity_t *attacker, int damage)
{

	if ( !(self->NPC->aiFlags&NPCAI_CUSTOM_GRAVITY ))
	{//void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod, int hitLoc=HL_NONE );
		//[CoOp]
		//I assume this means the seeker dies if it doesn't have it's custom gravity.
		//changed this to look like the SP version of this call
		G_Damage( self, NULL, NULL, vec3_origin, (float*)vec3_origin, 999, 0, MOD_FALLING );
		//G_Damage( self, NULL, NULL, (float*)vec3_origin, (float*)vec3_origin, 999, 0, MOD_FALLING );
		//[/CoOp]
	}

	//[SeekerItemNpc]
	//if we die, remove the control from our owner
	if(self->health < 0 && self->activator && self->activator->client){
		self->activator->client->remote = NULL;
		self->activator->client->ps.stats[STAT_HOLDABLE_ITEMS] &= ~(1 << HI_SEEKER);
	}
	//[/SeekerItemNpc]

	SaveNPCGlobals();
	SetNPCGlobals( self );
	Seeker_Strafe();
	RestoreNPCGlobals();
	NPC_Pain( self, attacker, damage );
}


//[CoOp]
extern float Q_flrand(float min, float max);
//[/CoOp]

//------------------------------------
void Seeker_MaintainHeight( void )
{	
	float	dif;

	// Update our angles regardless
	NPC_UpdateAngles( qtrue, qtrue );

	// If we have an enemy, we should try to hover at or a little below enemy eye level
	if ( NPC->enemy )
	{
		if (TIMER_Done( NPC, "heightChange" ))
		{
			float difFactor;

			TIMER_Set( NPC,"heightChange",Q_irand( 1000, 3000 ));

			// Find the height difference
			//[CoOp]
			//changed to use same function call as SP.
			dif = (NPC->enemy->r.currentOrigin[2] +  Q_flrand( NPC->enemy->r.maxs[2]/2, NPC->enemy->r.maxs[2]+8 )) - NPC->r.currentOrigin[2]; 
			//dif = (NPC->enemy->r.currentOrigin[2] +  flrand( NPC->enemy->r.maxs[2]/2, NPC->enemy->r.maxs[2]+8 )) - NPC->r.currentOrigin[2]; 
			//[/CoOp]

			difFactor = 1.0f;
			if ( NPC->client->NPC_class == CLASS_BOBAFETT )
			{
				if ( TIMER_Done( NPC, "flameTime" ) )
				{
					difFactor = 10.0f;
				}
			}

			// cap to prevent dramatic height shifts
			if ( fabs( dif ) > 2*difFactor )
			{
				if ( fabs( dif ) > 24*difFactor )
				{
					dif = ( dif < 0 ? -24*difFactor : 24*difFactor );
				}

				NPC->client->ps.velocity[2] = (NPC->client->ps.velocity[2]+dif)/2;
			}
			if ( NPC->client->NPC_class == CLASS_BOBAFETT )
			{
				//[CoOp]
				//changed to use same function call as SP.
				NPC->client->ps.velocity[2] *= Q_flrand( 0.85f, 3.0f );
				//NPC->client->ps.velocity[2] *= flrand( 0.85f, 3.0f );
				//[/CoOp]
			}
		}
	}
	else
	{
		gentity_t *goal = NULL;

		if ( NPCInfo->goalEntity )	// Is there a goal?
		{
			goal = NPCInfo->goalEntity;
		}
		else
		{
			goal = NPCInfo->lastGoalEntity;
		}
		if ( goal )
		{
			dif = goal->r.currentOrigin[2] - NPC->r.currentOrigin[2];

			if ( fabs( dif ) > 24 )
			{
				ucmd.upmove = ( ucmd.upmove < 0 ? -4 : 4 );
			}
			else
			{
				if ( NPC->client->ps.velocity[2] )
				{
					NPC->client->ps.velocity[2] *= VELOCITY_DECAY;

					if ( fabs( NPC->client->ps.velocity[2] ) < 2 )
					{
						NPC->client->ps.velocity[2] = 0;
					}
				}
			}
		}
	}

	// Apply friction
	if ( NPC->client->ps.velocity[0] )
	{
		NPC->client->ps.velocity[0] *= VELOCITY_DECAY;

		if ( fabs( NPC->client->ps.velocity[0] ) < 1 )
		{
			NPC->client->ps.velocity[0] = 0;
		}
	}

	if ( NPC->client->ps.velocity[1] )
	{
		NPC->client->ps.velocity[1] *= VELOCITY_DECAY;

		if ( fabs( NPC->client->ps.velocity[1] ) < 1 )
		{
			NPC->client->ps.velocity[1] = 0;
		}
	}
}

//------------------------------------
void Seeker_Strafe( void )
{
	int		side;
	vec3_t	end, right, dir;
	trace_t	tr;

	if ( random() > 0.7f || !NPC->enemy || !NPC->enemy->client )
	{
		// Do a regular style strafe
		AngleVectors( NPC->client->renderInfo.eyeAngles, NULL, right, NULL );

		// Pick a random strafe direction, then check to see if doing a strafe would be
		//	reasonably valid
		side = ( rand() & 1 ) ? -1 : 1;
		VectorMA( NPC->r.currentOrigin, SEEKER_STRAFE_DIS * side, right, end );

		trap_Trace( &tr, NPC->r.currentOrigin, NULL, NULL, end, NPC->s.number, MASK_SOLID );

		// Close enough
		if ( tr.fraction > 0.9f )
		{
			float vel = SEEKER_STRAFE_VEL;
			float upPush = SEEKER_UPWARD_PUSH;
			if ( NPC->client->NPC_class != CLASS_BOBAFETT )
			{
				G_Sound( NPC, CHAN_AUTO, G_SoundIndex( "sound/chars/seeker/misc/hiss" ));
			}
			else
			{
				vel *= 3.0f;
				upPush *= 4.0f;
			}
			VectorMA( NPC->client->ps.velocity, vel*side, right, NPC->client->ps.velocity );
			// Add a slight upward push
			NPC->client->ps.velocity[2] += upPush;

			NPCInfo->standTime = level.time + 1000 + random() * 500;
		}
	}
	else
	{
		float stDis;

		// Do a strafe to try and keep on the side of their enemy
		AngleVectors( NPC->enemy->client->renderInfo.eyeAngles, dir, right, NULL );

		// Pick a random side
		side = ( rand() & 1 ) ? -1 : 1;
		stDis = SEEKER_STRAFE_DIS;
		if ( NPC->client->NPC_class == CLASS_BOBAFETT )
		{
			stDis *= 2.0f;
		}
		VectorMA( NPC->enemy->r.currentOrigin, stDis * side, right, end );

		// then add a very small bit of random in front of/behind the player action
		VectorMA( end, crandom() * 25, dir, end );

		trap_Trace( &tr, NPC->r.currentOrigin, NULL, NULL, end, NPC->s.number, MASK_SOLID );

		// Close enough
		if ( tr.fraction > 0.9f )
		{
			float dis, upPush;

			VectorSubtract( tr.endpos, NPC->r.currentOrigin, dir );
			dir[2] *= 0.25; // do less upward change
			dis = VectorNormalize( dir );

			// Try to move the desired enemy side
			VectorMA( NPC->client->ps.velocity, dis, dir, NPC->client->ps.velocity );

			upPush = SEEKER_UPWARD_PUSH;
			if ( NPC->client->NPC_class != CLASS_BOBAFETT )
			{
				G_Sound( NPC, CHAN_AUTO, G_SoundIndex( "sound/chars/seeker/misc/hiss" ));
			}
			else
			{
				upPush *= 4.0f;
			}

			// Add a slight upward push
			NPC->client->ps.velocity[2] += upPush;

			NPCInfo->standTime = level.time + 2500 + random() * 500;
		}
	}
}

//------------------------------------
void Seeker_Hunt( qboolean visible, qboolean advance )
{
	float	distance, speed;
	vec3_t	forward;

	NPC_FaceEnemy( qtrue );

	//[SeekerItemNpc]
	if(NPC->genericValue3){
		if ( TIMER_Done( NPC, "seekerAlert" )){
			//TODO: different sound for 'found enemy' or 'searching' ?
			/*
			if(visible)
				G_Sound( NPC, CHAN_AUTO, G_SoundIndex( "sound/chars/seeker/misc/hiss" ));
			else
			*/
			TIMER_Set(NPC, "seekerAlert", level.time + 900);
			G_Sound(NPC, CHAN_AUTO, NPC->genericValue3);
		}
	}
	//[/SeekerItemNpc]


	// If we're not supposed to stand still, pursue the player
	if ( NPCInfo->standTime < level.time )
	{
		// Only strafe when we can see the player
		if ( visible )
		{
			Seeker_Strafe();
			return;
		}
	}

	// If we don't want to advance, stop here
	if ( advance == qfalse )
	{
		return;
	}

	// Only try and navigate if the player is visible
	if ( visible == qfalse )
	{
		// Move towards our goal
		NPCInfo->goalEntity = NPC->enemy;
		NPCInfo->goalRadius = 24;

		//[CoOp]
		NPC_MoveToGoal(qtrue);
		return;
		/*
		// Get our direction from the navigator if we can't see our target
		if ( NPC_GetMoveDirection( forward, &distance ) == qfalse )
		{
			return;
		}
		*/
		//[/CoOp]
	}
	else
	{
		VectorSubtract( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, forward );
		distance = VectorNormalize( forward );
	}

	speed = SEEKER_FORWARD_BASE_SPEED + SEEKER_FORWARD_MULTIPLIER * g_spskill.integer;
	VectorMA( NPC->client->ps.velocity, speed, forward, NPC->client->ps.velocity );
}

//------------------------------------
qboolean Seeker_Fire( void )
{

	//[SeekerItemNpc]
#if 1


	vec3_t		dir, enemy_org, muzzle;
	gentity_t	*missile;
	trace_t tr;
	int randomnum = irand(1,5);

	if (randomnum == 1)
	{
	CalcEntitySpot( NPC->enemy, SPOT_HEAD, enemy_org );
	}
	else if (randomnum == 2)
	{
	CalcEntitySpot( NPC->enemy,SPOT_WEAPON,enemy_org);
	}
	else if (randomnum == 3)
	{
	CalcEntitySpot(NPC->enemy,SPOT_ORIGIN,enemy_org);
	}
	else if (randomnum == 4)
	{
	CalcEntitySpot(NPC->enemy,SPOT_CHEST,enemy_org);
	}
	else
	{
	CalcEntitySpot(NPC->enemy,SPOT_LEGS,enemy_org);
	}

	enemy_org[0]+=irand(2,15);
	enemy_org[1]+=irand(2,15);
	
	//calculate everything based on our model offset
	VectorCopy(NPC->r.currentOrigin, muzzle);
	//correct for our model offset
	muzzle[2] -= 22;
	muzzle[2] -= randomnum;

	VectorSubtract( enemy_org, NPC->r.currentOrigin, dir );
	VectorNormalize( dir );

	// move a bit forward in the direction we shall shoot in so that the bolt doesn't poke out the other side of the seeker
	VectorMA( muzzle, 15, dir, muzzle );

	trap_Trace(&tr, muzzle, NULL, NULL, enemy_org, NPC->s.number, MASK_PLAYERSOLID);
	//tr.fraction wont be 1.0f, since we hit our enemy.
	if(tr.entityNum != NPC->enemy->s.number)
	//if(tr.fraction >= 1.0f)
		//we cant reach our target
		return qfalse;


	//our player should get the kill, if any
	//FIXME: we have a special case here.  we want the kill to be given to the activator, BUT we want the missile to NOT hurt the player
	//attempting to leave missile->parent == NPC, but missile->r.ownerNum to NPC->activator->s.number, no idea if it will work, just doing a guess.
	//if(NPC->activator)
	//	missile = CreateMissile( muzzle, dir, 1000, 10000, NPC->activator, qfalse );
	//else
		missile = CreateMissile( muzzle, dir, 1000, 10000, NPC, qfalse );

	//missile = CreateMissile( muzzle, dir, 1000, 10000, NPC, qfalse );

	G_PlayEffectID( G_EffectIndex("blaster/muzzle_flash"), muzzle, dir );

	missile->classname = "blaster";
	missile->s.weapon = WP_BLASTER;

	missile->damage = NPC->damage;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;

	//[SeekerItemNPC]
	//seeker items have different MOD 
	if(NPC->client->leader //has leader
		&& NPC->client->leader->client //leader is a client
		&& NPC->client->leader->client->remote == NPC) //has us as their remote.
	{//yep, this is a player's seeker, use different MOD
		missile->methodOfDeath = MOD_SEEKER;
	}
	else
	{
		missile->methodOfDeath = MOD_BLASTER;
	}
	//missile->methodOfDeath = MOD_BLASTER;
	//[/SeekerItemNPC]
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	//[CoOp]
	/* Not in SP version of code.
	if ( NPC->r.ownerNum < ENTITYNUM_NONE )
	{
		missile->r.ownerNum = NPC->r.ownerNum;
	}
	*/
	//[/CoOp]

	////no, it isnt in SP version, but will it let the player get the kill but not let the seeker shoot itself?
	//if(NPC->activator)
	//	missile->r.ownerNum = NPC->activator->s.number;

	//according to the old q3 source, if the missile has the activators ownerNum and the seeker also has that ownerNum, 
	//the seeker shouldnt shoot itself.
	//wait... the silly thing wasnt shooting itself, but was out of ammo (and self destructed)!


	return qtrue;

#else
	vec3_t		dir, enemy_org, muzzle;
	gentity_t	*missile;

	CalcEntitySpot( NPC->enemy, SPOT_HEAD, enemy_org );
	VectorSubtract( enemy_org, NPC->r.currentOrigin, dir );
	VectorNormalize( dir );

	// move a bit forward in the direction we shall shoot in so that the bolt doesn't poke out the other side of the seeker
	VectorMA( NPC->r.currentOrigin, 15, dir, muzzle );

	missile = CreateMissile( muzzle, dir, 1000, 10000, NPC, qfalse );

	G_PlayEffectID( G_EffectIndex("blaster/muzzle_flash"), NPC->r.currentOrigin, dir );

	missile->classname = "blaster";
	missile->s.weapon = WP_BLASTER;

	missile->damage = 5;
	missile->dflags = DAMAGE_DEATH_KNOCKBACK;
	missile->methodOfDeath = MOD_BLASTER;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	//[CoOp]
	/* Not in SP version of code.
	if ( NPC->r.ownerNum < ENTITYNUM_NONE )
	{
		missile->r.ownerNum = NPC->r.ownerNum;
	}
	*/
	//[/CoOp]
#endif
}

//------------------------------------
void Seeker_Ranged( qboolean visible, qboolean advance )
{
	if ( NPC->client->NPC_class != CLASS_BOBAFETT )
	{//racc - boba fett doesn't run out of ammo.
		if ( NPC->count > 0 || NPC->count == -1)
		{
			//[SeekerItemNpc]
			//better than using the timer, and if the dynamic music is ever used, then we can apply it to them using the shootTime
			//meh, just using TIMER_ stuff for now, in case standard npc ai messes it up
			//if (NPCInfo->shotTime < level.time)	// Attack?
			if ( TIMER_Done( NPC, "attackDelay" ))	// Attack?
			{
				//NPCInfo->shotTime = level.time + NPC->delay + Q_irand(0, NPC->random);
				TIMER_Set( NPC, "attackDelay", Q_irand(NPC->genericValue1, NPC->genericValue2));
				Seeker_Fire();
				if(NPC->count != -1)
					NPC->count--;
			}
			/*
			if ( TIMER_Done( NPC, "attackDelay" ))	// Attack?
			{
				TIMER_Set( NPC, "attackDelay", Q_irand( 250, 2500 ));
				Seeker_Fire();
				NPC->count--;
			}
			*/
			//[/SeekerItemNpc]
		}
		else
		{
			//[SeekerItemNpc] what is wrong with this?  re-enabling...
			//hmm, somewhere I saw code that handles the final death, but I cant find it anymore...
			//meh, disabling again

			// out of ammo, so let it die...give it a push up so it can fall more and blow up on impact
			//NPC->client->ps.gravity = 900;
			//NPC->svFlags &= ~SVF_CUSTOM_GRAVITY;
			//NPC->client->ps.velocity[2] += 16;
			G_Damage( NPC, NPC, NPC, NULL, NULL, NPC->health/*999*/, 0, MOD_UNKNOWN );
			//[/SeekerItemNpc]
		}
	}

	if ( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
	{
		Seeker_Hunt( visible, advance );
	}
} 

//------------------------------------

void Seeker_Attack( void )
{
	float		distance;
	qboolean	visible;
	qboolean	advance;

	// Always keep a good height off the ground
	Seeker_MaintainHeight();

	// Rate our distance to the target, and our visibilty
	distance	= DistanceHorizontalSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );	
	visible		= NPC_ClearLOS4( NPC->enemy );
	advance		= (qboolean)(distance > MIN_DISTANCE_SQR);

	//[SeekerItemNpc]
	//dont shoot at dead people
	if(!NPC->enemy->inuse || NPC->enemy->health <= 0){
		NPC->enemy = NULL;
		return;
	}
	//[/SeekerItemNpc]

	if ( NPC->client->NPC_class == CLASS_BOBAFETT )
	{
		advance = (qboolean)(distance>(200.0f*200.0f));
	}

	// If we cannot see our target, move to see it
	if ( visible == qfalse )
	{
		if ( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
		{
			Seeker_Hunt( visible, advance );
			return;
		}
		//[SeekerItemNpc]
		else{
			//we cant chase them?  then return to the follow target
			NPC->enemy = NULL;
			if(NPC->client->leader)
				NPCInfo->goalEntity = NPC->client->leader;
			return;
		}	
		//[/SeekerItemNpc]

	}

	Seeker_Ranged( visible, advance );
}

//------------------------------------
void Seeker_FindEnemy( void )
{
	int			numFound;
	float		dis, bestDis = SEEKER_SEEK_RADIUS * SEEKER_SEEK_RADIUS + 1;
	vec3_t		mins, maxs;
	int			entityList[MAX_GENTITIES];
	gentity_t	*ent, *best = NULL;
	int			i;
	//[SeekerItemNpc]
	float closestDist = SEEKER_SEEK_RADIUS * SEEKER_SEEK_RADIUS + 1;
	//[/SeekerItemNpc]
	
	if(NPC->activator && NPC->activator->client->ps.weapon == WP_SABER && !NPC->activator->client->ps.saberHolstered)
	{
		NPC->enemy=NULL;
		return;
	}

	VectorSet( maxs, SEEKER_SEEK_RADIUS, SEEKER_SEEK_RADIUS, SEEKER_SEEK_RADIUS );
	VectorScale( maxs, -1, mins );

	//[CoOp]
	//without this, the seekers are just scanning in terms of the world coordinates instead of around themselves, which is bad.
	VectorAdd(maxs, NPC->r.currentOrigin, maxs);
	VectorAdd(mins, NPC->r.currentOrigin, mins);
	//[/CoOp]

	numFound = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( i = 0 ; i < numFound ; i++ ) 
	{
		ent = &g_entities[entityList[i]];

		if ( ent->s.number == NPC->s.number 
			|| !ent->client //&& || !ent->NPC 
			//[CoOp]
			//SP says this.  Don't attack non-NPCs?!
			//|| !ent->NPC 
			//[/CoOp]
			|| ent->health <= 0 
			|| !ent->inuse )
		{
			continue;
		}

		//[SeekerItemNpc]
		if(OnSameTeam(NPC->activator, ent))
		{//our owner is on the same team as this entity, don't target them.
			continue;
		}

		//dont attack our owner
		if(NPC->activator == ent)
			continue;

		if(ent->s.NPC_class == CLASS_VEHICLE)
			continue;
		//[/SeekerItemNpc]

		dis = DistanceHorizontalSquared( NPC->r.currentOrigin, ent->r.currentOrigin );

		if ( dis <= closestDist )
			closestDist = dis;

		// try to find the closest visible one
		if ( !NPC_ClearLOS4( ent ))
		{
			continue;
		}

		if ( dis <= bestDis )
		{
			bestDis = dis;
			best = ent;
		}
		//[/SeekerItemNpc]

	}

	if ( best )
	{
		//[SeekerItemNpc] because we can run even if we already have an enemy
		if(!NPC->enemy){
			// used to offset seekers around a circle so they don't occupy the same spot.  This is not a fool-proof method.
			NPC->random = random() * 6.3f; // roughly 2pi

			NPC->enemy = best;
		}
		//[/SeekerItemNpc]
	}

	//[SeekerItemNpc]
	//positive radius, check with los in mind
	if(NPC->radius > 0){
		if(best && bestDis <= NPC->radius)
			NPC->fly_sound_debounce_time = level.time + (int)floor(2500.0f * (bestDis / (float)NPC->radius)) + 500;
		else
			NPC->fly_sound_debounce_time = -1;
	}
	//negitive radius, check only closest without los
	else if(NPC->radius < 0){
		if(closestDist <= -NPC->radius)
			NPC->fly_sound_debounce_time = level.time + (int)floor(2500.0f * (closestDist / -(float)NPC->radius)) + 500;
		else
			NPC->fly_sound_debounce_time = -1;
	}
	//[/SeekerItemNpc]

}

//[CoOp]
//------------------------------------
void Seeker_FollowPlayer( void )
{//hover around the closest player

	//[SeekerItemNpc]
#if 1
	vec3_t	pt, dir;
	float	dis;
	float	minDistSqr = MIN_DISTANCE_SQR;
	gentity_t *target;

	Seeker_MaintainHeight();

	if(NPC->activator && NPC->activator->client)
	{
		if(NPC->activator->client->remote != NPC || NPC->activator->health <= 0){
			//have us fall down and explode.
			NPC->NPC->aiFlags |= NPCAI_CUSTOM_GRAVITY;
			return;
		}
		target = NPCInfo->goalEntity;
		if(!target)
			target = NPC->client->leader;
	}
	else{
		target = FindClosestPlayer(NPC->r.currentOrigin, NPC->client->playerTeam);
	}

	if(!target)
	{//in MP it's actually possible that there's no players on our team at the moment.
		return;
	}

	dis	= DistanceHorizontalSquared( NPC->r.currentOrigin, target->r.currentOrigin );

	if ( NPC->client->NPC_class == CLASS_BOBAFETT )
	{
		if ( TIMER_Done( NPC, "flameTime" ) )
		{
			minDistSqr = 200*200;
		}
	}

	if ( dis < minDistSqr )
	{
		// generally circle the player closely till we take an enemy..this is our target point
		if ( NPC->client->NPC_class == CLASS_BOBAFETT )
		{
			pt[0] = target->r.currentOrigin[0] + cos( level.time * 0.001f + NPC->random ) * 250;
			pt[1] = target->r.currentOrigin[1] + sin( level.time * 0.001f + NPC->random ) * 250;
			if ( NPC->client->jetPackTime < level.time )
			{
				pt[2] = target->r.currentOrigin[2] - 64;
			}
			else
			{
				pt[2] = target->r.currentOrigin[2] + 200;
			}
		}
		else
		{
			pt[0] = target->r.currentOrigin[0] + cos( level.time * 0.001f + NPC->random ) * 56;
			pt[1] = target->r.currentOrigin[1] + sin( level.time * 0.001f + NPC->random ) * 56;
			pt[2] = target->r.currentOrigin[2] + 40;
		}

		VectorSubtract( pt, NPC->r.currentOrigin, dir );
		VectorMA( NPC->client->ps.velocity, 0.8f, dir, NPC->client->ps.velocity );
	}
	else
	{
		if ( NPC->client->NPC_class != CLASS_BOBAFETT )
		{
			if ( TIMER_Done( NPC, "seekerhiss" ))
			{
				TIMER_Set( NPC, "seekerhiss", 1000 + random() * 1000 );
				G_Sound( NPC, CHAN_AUTO, G_SoundIndex( "sound/chars/seeker/misc/hiss" ));
			}
		}

		// Hey come back!
		NPCInfo->goalEntity = target;
		if(target == NPC->enemy)
			NPCInfo->goalRadius = 60;
		else
			NPCInfo->goalRadius = 32;
		if(!NPC_MoveToGoal(qtrue)){
			//cant go there on our first try, abort.
			//this really isnt the best way... but if it cant reach the point, it will just sit there doing nothing.
			NPCInfo->goalEntity = NPC->client->leader;
			//stop chasing the enemy if we were told to, and return to the player
			NPCInfo->scriptFlags &= ~SCF_CHASE_ENEMIES;
		}
	}

	//call this even if we do have an enemy, for enemy proximity detection
	if ( /*!NPC->enemy && */ NPCInfo->enemyCheckDebounceTime < level.time )
	{
		// check twice a second to find a new enemy
		Seeker_FindEnemy();
		NPCInfo->enemyCheckDebounceTime = level.time + 500;
	}

	//play our proximity beep
	if(NPC->genericValue3 && NPC->fly_sound_debounce_time > 0 && NPC->fly_sound_debounce_time < level.time){
		G_Sound(NPC, CHAN_AUTO, NPC->genericValue3);
		NPC->fly_sound_debounce_time = -1;
	}


	NPC_UpdateAngles( qtrue, qtrue );

#else
		vec3_t	pt, dir;
	float	dis;
	float	minDistSqr = MIN_DISTANCE_SQR;
	gentity_t *closestPlayer = NULL;

	Seeker_MaintainHeight();

	closestPlayer = FindClosestPlayer(NPC->r.currentOrigin, NPC->client->playerTeam);

	if(!closestPlayer)
	{//in MP it's actually possible that there's no players on our team at the moment.
		return;
	}


	dis	= DistanceHorizontalSquared( NPC->r.currentOrigin, closestPlayer->r.currentOrigin );

	if ( NPC->client->NPC_class == CLASS_BOBAFETT )
	{
		if ( TIMER_Done( NPC, "flameTime" ) )
		{
			minDistSqr = 200*200;
		}
	}

	if ( dis < minDistSqr )
	{
		// generally circle the player closely till we take an enemy..this is our target point
		if ( NPC->client->NPC_class == CLASS_BOBAFETT )
		{
			pt[0] = closestPlayer->r.currentOrigin[0] + cos( level.time * 0.001f + NPC->random ) * 250;
			pt[1] = closestPlayer->r.currentOrigin[1] + sin( level.time * 0.001f + NPC->random ) * 250;
			if ( NPC->client->jetPackTime < level.time )
			{
				pt[2] = closestPlayer->r.currentOrigin[2] - 64;
			}
			else
			{
				pt[2] = closestPlayer->r.currentOrigin[2] + 200;
			}
		}
		else
		{
			pt[0] = closestPlayer->r.currentOrigin[0] + cos( level.time * 0.001f + NPC->random ) * 56;
			pt[1] = closestPlayer->r.currentOrigin[1] + sin( level.time * 0.001f + NPC->random ) * 56;
			pt[2] = closestPlayer->r.currentOrigin[2] + 40;
		}

		VectorSubtract( pt, NPC->r.currentOrigin, dir );
		VectorMA( NPC->client->ps.velocity, 0.8f, dir, NPC->client->ps.velocity );
	}
	else
	{
		if ( NPC->client->NPC_class != CLASS_BOBAFETT )
		{
			if ( TIMER_Done( NPC, "seekerhiss" ))
			{
				TIMER_Set( NPC, "seekerhiss", 1000 + random() * 1000 );
				G_Sound( NPC, CHAN_AUTO, G_SoundIndex( "sound/chars/seeker/misc/hiss" ));
			}
		}

		// Hey come back!
		NPCInfo->goalEntity = closestPlayer;
		NPCInfo->goalRadius = 32;
		NPC_MoveToGoal( qtrue );
		NPC->s.owner = closestPlayer->s.number;
	}

	if ( NPCInfo->enemyCheckDebounceTime < level.time )
	{
		// check twice a second to find a new enemy
		Seeker_FindEnemy();
		NPCInfo->enemyCheckDebounceTime = level.time + 500;
	}

	NPC_UpdateAngles( qtrue, qtrue );
#endif
	//[/SeekerItemNpc]
}


/* This is equivilent to void Seeker_FollowPlayer( void ) but isn't set up to work right
//------------------------------------
void Seeker_FollowOwner( void )
{
	float	dis, minDistSqr;
	vec3_t	pt, dir;
	gentity_t	*owner = &g_entities[NPC->s.owner];

	Seeker_MaintainHeight();

	if ( NPC->client->NPC_class == CLASS_BOBAFETT )
	{
		owner = NPC->enemy;
	}
	if ( !owner || owner == NPC || !owner->client )
	{
		return;
	}
	//rwwFIXMEFIXME: Care about all clients not just 0
	dis	= DistanceHorizontalSquared( NPC->r.currentOrigin, owner->r.currentOrigin );
	
	minDistSqr = MIN_DISTANCE_SQR;

	if ( NPC->client->NPC_class == CLASS_BOBAFETT )
	{
		if ( TIMER_Done( NPC, "flameTime" ) )
		{
			minDistSqr = 200*200;
		}
	}

	if ( dis < minDistSqr )
	{
		// generally circle the player closely till we take an enemy..this is our target point
		if ( NPC->client->NPC_class == CLASS_BOBAFETT )
		{
			pt[0] = owner->r.currentOrigin[0] + cos( level.time * 0.001f + NPC->random ) * 250;
			pt[1] = owner->r.currentOrigin[1] + sin( level.time * 0.001f + NPC->random ) * 250;
			if ( NPC->client->jetPackTime < level.time )
			{
				pt[2] = NPC->r.currentOrigin[2] - 64;
			}
			else
			{
				pt[2] = owner->r.currentOrigin[2] + 200;
			}
		}
		else
		{
			pt[0] = owner->r.currentOrigin[0] + cos( level.time * 0.001f + NPC->random ) * 56;
			pt[1] = owner->r.currentOrigin[1] + sin( level.time * 0.001f + NPC->random ) * 56;
			pt[2] = owner->r.currentOrigin[2] + 40;
		}

		VectorSubtract( pt, NPC->r.currentOrigin, dir );
		VectorMA( NPC->client->ps.velocity, 0.8f, dir, NPC->client->ps.velocity );
	}
	else
	{
		if ( NPC->client->NPC_class != CLASS_BOBAFETT )
		{
			if ( TIMER_Done( NPC, "seekerhiss" ))
			{
				TIMER_Set( NPC, "seekerhiss", 1000 + random() * 1000 );
				G_Sound( NPC, CHAN_AUTO, G_SoundIndex( "sound/chars/seeker/misc/hiss" ));
			}
		}

		// Hey come back!
		NPCInfo->goalEntity = owner;
		NPCInfo->goalRadius = 32;
		NPC_MoveToGoal( qtrue );
		NPC->parent = owner;
	}

	if ( NPCInfo->enemyCheckDebounceTime < level.time )
	{
		// check twice a second to find a new enemy
		Seeker_FindEnemy();
		NPCInfo->enemyCheckDebounceTime = level.time + 500;
	}

	NPC_UpdateAngles( qtrue, qtrue );
}
*/
//[/CoOp]

//[CoOp]
extern qboolean in_camera;
void NPC_BSST_Patrol( void );
//[/CoOp]

//------------------------------------
void NPC_BSSeeker_Default( void )
{
	//[CoOp]
	//reenabled since we now have cutscene camera support
	if ( in_camera )
	{
		if ( NPC->client->NPC_class != CLASS_BOBAFETT )
		{
			// cameras make me commit suicide....
			G_Damage( NPC, NPC, NPC, NULL, NULL, 999, 0, MOD_UNKNOWN );
			//[SeekerItemNpc]
			//dont continue this run because we are dead
			return;
			//[/SeekerItemNpc]
		}
	}

	//[SeekerItemNpc]
	if ( NPC->client->NPC_class != CLASS_BOBAFETT && NPC->activator->health <= 0){
		G_Damage( NPC, NPC, NPC, NULL, NULL, NPC->health, 0, MOD_UNKNOWN );
		return;
	}
	//[/SeekerItemNpc]


	/*
	//N/A for MP.
	if ( NPC->r.ownerNum < ENTITYNUM_NONE )
	{
		gentity_t *owner = &g_entities[0];
		if ( owner->health <= 0 
			|| (owner->client && owner->client->pers.connected == CON_DISCONNECTED) )
		{//owner is dead or gone
			//remove me
			G_Damage( NPC, NULL, NULL, NULL, NULL, 10000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );
			return;
		}
	}
	*/
	//[/CoOp]

	if ( NPC->random == 0.0f )
	{
		// used to offset seekers around a circle so they don't occupy the same spot.  This is not a fool-proof method.
		NPC->random = random() * 6.3f; // roughly 2pi
	}

	if ( NPC->enemy && NPC->enemy->health && NPC->enemy->inuse )
	{
		if ( NPC->client->NPC_class != CLASS_BOBAFETT
			//[CoOp]
			//[SeekerItemNpc] - dont attack our owner or leader, and dont shoot at dead people
			&& ( NPC->enemy == NPC->activator || NPC->enemy == NPC->client->leader || !NPC->enemy->inuse || NPC->health <= 0 ||
			( NPC->enemy->client && NPC->enemy->client->NPC_class == CLASS_SEEKER )) )
			//&& ( NPC->enemy->s.number < MAX_CLIENTS || ( NPC->enemy->client && NPC->enemy->client->NPC_class == CLASS_SEEKER )) )
			//[/SeekerItemNpc]
			//&& ( NPC->enemy->s.number == 0 || ( NPC->enemy->client && NPC->enemy->client->NPC_class == CLASS_SEEKER )) )
			//[/CoOp]
		{
			//hacked to never take the player as an enemy, even if the player shoots at it
			NPC->enemy = NULL;
		}
		else
		{
			Seeker_Attack();
			if ( NPC->client->NPC_class == CLASS_BOBAFETT )
			{
				Boba_FireDecide();
			}
			//[SeekerItemNpc]
			//still follow our target, be it a targeted enemy or our owner, but only if we aren't allowed to hunt down enemies.
			if ( NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
				return;
			//[/SeekerItemNpc]
		}
	}
	//[CoOp]
	else if ( NPC->client->NPC_class == CLASS_BOBAFETT )
	{//boba does a ST patrol if it doesn't have an enemy.
		NPC_BSST_Patrol();
		return;
	}
	//[/CoOp]

	//[CoOp]
	// In all other cases, follow the player and look for enemies to take on
	Seeker_FollowPlayer();
	//Seeker_FollowOwner();
	//[/CoOp]
}
//[/SPPortComplete]
