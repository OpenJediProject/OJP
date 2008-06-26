// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"
#include "w_saber.h"
#include "q_shared.h"

#define	MISSILE_PRESTEP_TIME	50

extern void laserTrapStick( gentity_t *ent, vec3_t endpos, vec3_t normal );
extern void Jedi_Decloak( gentity_t *self );

#include "../namespace_begin.h"
extern qboolean FighterIsLanded( Vehicle_t *pVeh, playerState_t *parentPS );
#include "../namespace_end.h"

/*
================
G_ReflectMissile

  Reflect the missile roughly back at it's owner
================
*/
float RandFloat(float min, float max);
void G_ReflectMissile( gentity_t *ent, gentity_t *missile, vec3_t forward ) 
{
	vec3_t	bounce_dir;
	int		i;
	float	speed;
	gentity_t	*owner = ent;
	int		isowner = 0;

	if ( ent->r.ownerNum )
	{
		owner = &g_entities[ent->r.ownerNum];
	}

	if (missile->r.ownerNum == ent->s.number)
	{ //the original owner is bouncing the missile, so don't try to bounce it back at him
		isowner = 1;
	}

	//save the original speed
	speed = VectorNormalize( missile->s.pos.trDelta );

	//if ( ent && owner && owner->NPC && owner->enemy && Q_stricmp( "Tavion", owner->NPC_type ) == 0 && Q_irand( 0, 3 ) )
	if ( &g_entities[missile->r.ownerNum] && missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART && !isowner )
	{//bounce back at them if you can
		VectorSubtract( g_entities[missile->r.ownerNum].r.currentOrigin, missile->r.currentOrigin, bounce_dir );
		VectorNormalize( bounce_dir );
	}
	else if (isowner)
	{ //in this case, actually push the missile away from me, and since we're giving boost to our own missile by pushing it, up the velocity
		vec3_t missile_dir;

		speed *= 1.5;

		VectorSubtract( missile->r.currentOrigin, ent->r.currentOrigin, missile_dir );
		VectorCopy( missile->s.pos.trDelta, bounce_dir );
		VectorScale( bounce_dir, DotProduct( forward, missile_dir ), bounce_dir );
		VectorNormalize( bounce_dir );
	}
	else
	{
		vec3_t missile_dir;

		VectorSubtract( ent->r.currentOrigin, missile->r.currentOrigin, missile_dir );
		VectorCopy( missile->s.pos.trDelta, bounce_dir );
		VectorScale( bounce_dir, DotProduct( forward, missile_dir ), bounce_dir );
		VectorNormalize( bounce_dir );
	}
	for ( i = 0; i < 3; i++ )
	{
		bounce_dir[i] += RandFloat( -0.2f, 0.2f );
	}

	VectorNormalize( bounce_dir );
	VectorScale( bounce_dir, speed, missile->s.pos.trDelta );
	missile->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( missile->r.currentOrigin, missile->s.pos.trBase );
	if ( missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART )
	{//you are mine, now!
		missile->r.ownerNum = ent->s.number;
	}
	if ( missile->s.weapon == WP_ROCKET_LAUNCHER )
	{//stop homing
		missile->think = 0;
		missile->nextthink = 0;
	}
}
int NaturalBoltReflectRate[NUM_FORCE_POWER_LEVELS];//[Morerandom]

void G_DeflectMissile( gentity_t *ent, gentity_t *missile, vec3_t forward ) 
{
	vec3_t	bounce_dir;
	int		i;
	float	speed;
	int		isowner = 0;
	vec3_t missile_dir;
	//[MoreRandom]
	float   slopFactor=0.0f;
	int		defLevel=0;
	float distance =0;
	gentity_t *prevOwner = &g_entities[missile->r.ownerNum];
	//[/MoreRandom]
	if (missile->r.ownerNum == ent->s.number)
	{ //the original owner is bouncing the missile, so don't try to bounce it back at him
		isowner = 1;
	}

	//save the original speed
	speed = VectorNormalize( missile->s.pos.trDelta );
	if (ent->client)
	{
		//VectorSubtract( ent->r.currentOrigin, missile->r.currentOrigin, missile_dir );
		AngleVectors(ent->client->ps.viewangles, missile_dir, 0, 0);
		VectorCopy(missile_dir, bounce_dir);
		//VectorCopy( missile->s.pos.trDelta, bounce_dir );
		VectorScale( bounce_dir, DotProduct( forward, missile_dir ), bounce_dir );
		VectorNormalize( bounce_dir );
	}
	else
	{
		VectorCopy(forward, bounce_dir);
		VectorNormalize(bounce_dir);
	}

	for ( i = 0; i < 3; i++ )
	{
		bounce_dir[i] += RandFloat( -1.0f, 1.0f );
	}

	VectorNormalize( bounce_dir );
	VectorScale( bounce_dir, speed, missile->s.pos.trDelta );
	missile->s.pos.trTime = level.time;		// move a bit on the very first frame
	VectorCopy( missile->r.currentOrigin, missile->s.pos.trBase );
	if ( missile->s.weapon != WP_SABER && missile->s.weapon != G2_MODEL_PART )
	{//you are mine, now!
		missile->r.ownerNum = ent->s.number;
	}
	if ( missile->s.weapon == WP_ROCKET_LAUNCHER )
	{//stop homing
		missile->think = 0;
		missile->nextthink = 0;
	}
}

/*
================
G_BounceMissile

================
*/
void G_BounceMissile( gentity_t *ent, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );


	if ( ent->flags & FL_BOUNCE_SHRAPNEL ) 
	{
		VectorScale( ent->s.pos.trDelta, 0.25f, ent->s.pos.trDelta );
		ent->s.pos.trType = TR_GRAVITY;

		// check for stop
		if ( trace->plane.normal[2] > 0.7 && ent->s.pos.trDelta[2] < 40 ) //this can happen even on very slightly sloped walls, so changed it from > 0 to > 0.7
		{
			G_SetOrigin( ent, trace->endpos );
			ent->nextthink = level.time + 100;
			return;
		}
	}
	else if ( ent->flags & FL_BOUNCE_HALF ) 
	{
		VectorScale( ent->s.pos.trDelta, 0.65, ent->s.pos.trDelta );
		// check for stop
		if ( trace->plane.normal[2] > 0.2 && VectorLength( ent->s.pos.trDelta ) < 40 ) 
		{
			G_SetOrigin( ent, trace->endpos );
			return;
		}
	}

	if (ent->s.weapon == WP_THERMAL)
	{ //slight hack for hit sound
		G_Sound(ent, CHAN_BODY, G_SoundIndex(va("sound/weapons/thermal/bounce%i.wav", Q_irand(1, 2))));
	}
	else if (ent->s.weapon == WP_SABER)
	{
		G_Sound(ent, CHAN_BODY, G_SoundIndex(va("sound/weapons/saber/bounce%i.wav", Q_irand(1, 3))));
	}
	else if (ent->s.weapon == G2_MODEL_PART)
	{
		//Limb bounce sound?
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;

	if (ent->bounceCount != -5)
	{
		ent->bounceCount--;
	}
}


/*
================
G_ExplodeMissile

Explode a missile without an impact
================
*/
void G_ExplodeMissile( gentity_t *ent ) {
	vec3_t		dir;
	vec3_t		origin;

	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );
	SnapVector( origin );
	G_SetOrigin( ent, origin );

	// we don't have a valid direction, so just point straight up
	dir[0] = dir[1] = 0;
	dir[2] = 1;

	ent->s.eType = ET_GENERAL;
	G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( dir ) );

	ent->freeAfterEvent = qtrue;

	ent->takedamage = qfalse;
	// splash damage
	if ( ent->splashDamage ) {
		//[Asteroids]
		//NOTE: vehicle missiles don't have an ent->parent set, so check that here and set it
		if ( ent->s.eType == ET_MISSILE//missile
			&& (ent->s.eFlags&EF_JETPACK_ACTIVE)//vehicle missile
			&& ent->r.ownerNum < MAX_CLIENTS )//valid client owner
		{//set my parent to my owner for purposes of damage credit...
			ent->parent = &g_entities[ent->r.ownerNum];
		}
		//[/Asteroids]
		if( G_RadiusDamage( ent->r.currentOrigin, ent->parent, ent->splashDamage, ent->splashRadius, ent, 
				ent, ent->splashMethodOfDeath ) ) 
		{
			if (ent->parent)
			{
				g_entities[ent->parent->s.number].client->accuracy_hits++;
			}
			else if (ent->activator)
			{
				g_entities[ent->activator->s.number].client->accuracy_hits++;
			}
		}
	}

	trap_LinkEntity( ent );
}

void G_RunStuckMissile( gentity_t *ent )
{
	if ( ent->takedamage )
	{
		if ( ent->s.groundEntityNum >= 0 && ent->s.groundEntityNum < ENTITYNUM_WORLD )
		{
			gentity_t *other = &g_entities[ent->s.groundEntityNum];

			if ( (!VectorCompare( vec3_origin, other->s.pos.trDelta ) && other->s.pos.trType != TR_STATIONARY) || 
				(!VectorCompare( vec3_origin, other->s.apos.trDelta ) && other->s.apos.trType != TR_STATIONARY) )
			{//thing I stuck to is moving or rotating now, kill me
				G_Damage( ent, other, other, NULL, NULL, 99999, 0, MOD_CRUSH );
				return;
			}
		}
	}
	// check think function
	G_RunThink( ent );
}

/*
================
G_BounceProjectile
================
*/
void G_BounceProjectile( vec3_t start, vec3_t impact, vec3_t dir, vec3_t endout ) {
	vec3_t v, newv;
	float dot;

	VectorSubtract( impact, start, v );
	dot = DotProduct( v, dir );
	VectorMA( v, -2*dot, dir, newv );

	VectorNormalize(newv);
	VectorMA(impact, 8192, newv, endout);
}


//-----------------------------------------------------------------------------
gentity_t *CreateMissile( vec3_t org, vec3_t dir, float vel, int life, 
							gentity_t *owner, qboolean altFire)
//-----------------------------------------------------------------------------
{
	gentity_t	*missile;

	missile = G_Spawn();
	
	missile->nextthink = level.time + life;
	missile->think = G_FreeEntity;
	missile->s.eType = ET_MISSILE;
	missile->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	missile->parent = owner;
	missile->r.ownerNum = owner->s.number;

	if (altFire)
	{
		missile->s.eFlags |= EF_ALT_FIRING;
	}

	missile->s.pos.trType = TR_LINEAR;
	missile->s.pos.trTime = level.time;// - MISSILE_PRESTEP_TIME;	// NOTENOTE This is a Quake 3 addition over JK2
	missile->target_ent = NULL;

	SnapVector(org);
	VectorCopy( org, missile->s.pos.trBase );
	VectorScale( dir, vel, missile->s.pos.trDelta );
	VectorCopy( org, missile->r.currentOrigin);
	SnapVector(missile->s.pos.trDelta);

	return missile;
}

void G_MissileBounceEffect( gentity_t *ent, vec3_t org, vec3_t dir )
{
	//FIXME: have an EV_BOUNCE_MISSILE event that checks the s.weapon and does the appropriate effect
	switch( ent->s.weapon )
	{
	case WP_BOWCASTER:
		G_PlayEffectID( G_EffectIndex("bowcaster/deflect"), ent->r.currentOrigin, dir );
		break;
	case WP_BLASTER:
	case WP_BRYAR_PISTOL:
		G_PlayEffectID( G_EffectIndex("blaster/deflect"), ent->r.currentOrigin, dir );
		break;
	default:
		{
			gentity_t *te = G_TempEntity( org, EV_SABER_BLOCK );
			VectorCopy(org, te->s.origin);
			VectorCopy(dir, te->s.angles);
			te->s.eventParm = 0;
			te->s.weapon = 0;//saberNum
			te->s.legsAnim = 0;//bladeNum
		}
		break;
	}
}

/*
================
G_MissileImpact
================
*/
void WP_SaberBlockNonRandom( gentity_t *self, vec3_t hitloc, qboolean missileBlock );
//[BoltBlockSys]
void OJP_HandleBoltBlock(gentity_t *bolt, gentity_t *player, trace_t *trace);
extern int OJP_SaberCanBlock(gentity_t *self, gentity_t *atk, qboolean checkBBoxBlock, vec3_t point, int rSaberNum, int rBladeNum);
extern qboolean GAME_INLINE WalkCheck( gentity_t * self );
//[/BoltBlockSys]
//[DodgeSys]
extern qboolean G_DoDodge( gentity_t *self, gentity_t *shooter, vec3_t dmgOrigin, int hitLoc, int * dmg, int mod );
//G_MissileImpact now returns qfalse if and only if the player physically dodged the damage.
//this allows G_RunMissile to properly handle he 
qboolean G_MissileImpact( gentity_t *ent, trace_t *trace ) {
//void G_MissileImpact( gentity_t *ent, trace_t *trace ) {
//[/DodgeSys]
	gentity_t		*other;
	qboolean		hitClient = qfalse;
	qboolean		isKnockedSaber = qfalse;
	//[DodgeSys]
	int missileDmg;
	//[/DodgeSys]

	other = &g_entities[trace->entityNum];

	// check for bounce
	//[WeaponSys]
	//allow thermals to bounce off players and such.
	if ( (!other->takedamage || ent->s.weapon == WP_THERMAL) &&
	//if ( !other->takedamage &&
	//[/WeaponSys]
		(ent->bounceCount > 0 || ent->bounceCount == -5) &&
		( ent->flags & ( FL_BOUNCE | FL_BOUNCE_HALF ) ) ) {
		G_BounceMissile( ent, trace );
		G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
		//[DodgeSys]
		return qtrue;
		//return;
		//[/DodgeSys]
	}
	else if (ent->neverFree && ent->s.weapon == WP_SABER && (ent->flags & FL_BOUNCE_HALF))
	{ //this is a knocked-away saber
		if (ent->bounceCount > 0 || ent->bounceCount == -5)
		{
			G_BounceMissile( ent, trace );
			G_AddEvent( ent, EV_GRENADE_BOUNCE, 0 );
			//[DodgeSys]
			return qtrue;
			//return;
			//[/DodgeSys]
		}

		isKnockedSaber = qtrue;
	}
	
	// I would glom onto the FL_BOUNCE code section above, but don't feel like risking breaking something else
	if (/* (!other->takedamage && (ent->bounceCount > 0 || ent->bounceCount == -5) && ( ent->flags&(FL_BOUNCE_SHRAPNEL) ) ) ||*/ ((trace->surfaceFlags&SURF_FORCEFIELD)&&!ent->splashDamage&&!ent->splashRadius&&(ent->bounceCount > 0 || ent->bounceCount == -5)) ) 
	{
		G_BounceMissile( ent, trace );

		if ( ent->bounceCount < 1 )
		{
			ent->flags &= ~FL_BOUNCE_SHRAPNEL;
		}
		//[DodgeSys]
		return qtrue;
		//return;
		//[/DodgeSys]
	}

	/*
	if ( !other->takedamage && ent->s.weapon == WP_THERMAL && !ent->alt_fire )
	{//rolling thermal det - FIXME: make this an eFlag like bounce & stick!!!
		//G_BounceRollMissile( ent, trace );
		if ( ent->owner && ent->owner->s.number == 0 ) 
		{
			G_MissileAddAlerts( ent );
		}
		//gi.linkentity( ent );
		return;
	}
	*/

	if ((other->r.contents & CONTENTS_LIGHTSABER) && !isKnockedSaber)
	{ //hit this person's saber, so..
		gentity_t *otherOwner = &g_entities[other->r.ownerNum];

		if (otherOwner->takedamage && otherOwner->client && otherOwner->client->ps.duelInProgress &&
			otherOwner->client->ps.duelIndex != ent->r.ownerNum)
		{
			goto killProj;
		}
	}
	else if (!isKnockedSaber)
	{
		if (other->takedamage && other->client && other->client->ps.duelInProgress &&
			other->client->ps.duelIndex != ent->r.ownerNum)
		{
			goto killProj;
		}
	}

	if (other->flags & FL_DMG_BY_HEAVY_WEAP_ONLY)
	{
		if (ent->methodOfDeath != MOD_REPEATER_ALT &&
			ent->methodOfDeath != MOD_ROCKET &&
			ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
			ent->methodOfDeath != MOD_ROCKET_HOMING &&
			ent->methodOfDeath != MOD_THERMAL &&
			ent->methodOfDeath != MOD_THERMAL_SPLASH &&
			ent->methodOfDeath != MOD_TRIP_MINE_SPLASH &&
			ent->methodOfDeath != MOD_TIMED_MINE_SPLASH &&
			ent->methodOfDeath != MOD_DET_PACK_SPLASH &&
			ent->methodOfDeath != MOD_VEHICLE &&
			ent->methodOfDeath != MOD_CONC &&
			ent->methodOfDeath != MOD_CONC_ALT &&
			ent->methodOfDeath != MOD_SABER &&
			//[Asteroids]
			ent->methodOfDeath != MOD_TURBLAST &&
			ent->methodOfDeath != MOD_TARGET_LASER)
			//[/Asteroids]
		{
			vec3_t fwd;

			if (trace)
			{
				VectorCopy(trace->plane.normal, fwd);
			}
			else
			{ //oh well
				AngleVectors(other->r.currentAngles, fwd, NULL, NULL);
			}

			G_DeflectMissile(other, ent, fwd);
			G_MissileBounceEffect(ent, ent->r.currentOrigin, fwd);
			//[DodgeSys]
			return qtrue;
			//return;
			//[/DodgeSys]
		}
	}

	//ROP VEHICLE_IMP START
	if((other->s.NPC_class == CLASS_VEHICLE && other->m_pVehicle 
		&& !other->m_pVehicle->m_pVehicleInfo->AllWeaponsDoDamageToShields
		&& other->client->ps.stats[STAT_ARMOR] > 0) || other->flags & FL_SHIELDED)
	{
		if (ent->s.weapon != WP_ROCKET_LAUNCHER &&
			ent->s.weapon != WP_THERMAL &&
			ent->s.weapon != WP_TRIP_MINE &&
			ent->s.weapon != WP_DET_PACK &&
			ent->s.weapon != WP_DEMP2 &&
			ent->s.weapon != WP_EMPLACED_GUN &&
			ent->s.weapon != WP_TURRET &&
			ent->methodOfDeath != MOD_REPEATER_ALT &&
			ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH && 
			ent->methodOfDeath != MOD_TURBLAST &&
			ent->methodOfDeath != MOD_VEHICLE &&
			ent->methodOfDeath != MOD_CONC &&
			ent->methodOfDeath != MOD_CONC_ALT &&
			!(ent->dflags&DAMAGE_HEAVY_WEAP_CLASS) )
		{
			vec3_t fwd;

			if (other->client)
			{
				AngleVectors(other->client->ps.viewangles, fwd, NULL, NULL);
			}
			else
			{
				AngleVectors(other->r.currentAngles, fwd, NULL, NULL);
			}

			G_DeflectMissile(other, ent, fwd);
			G_MissileBounceEffect(ent, ent->r.currentOrigin, fwd);
			//[DodgeSys]
			return qtrue;
			//return;
			//[/DodgeSys]
		}
	}
	//ROP VEHICLE_IMP END
	
	//[BoltBlockSys]
	if (OJP_SaberCanBlock(other, ent, qfalse, trace->endpos, -1, -1))
	/*
	if (other->takedamage && other->client &&
		ent->s.weapon != WP_ROCKET_LAUNCHER &&
		ent->s.weapon != WP_THERMAL &&
		ent->s.weapon != WP_TRIP_MINE &&
		ent->s.weapon != WP_DET_PACK &&
		//[BoltBlockSys]
		//ent->s.weapon != WP_DEMP2 &&
		//[/BoltBlockSys]
		ent->methodOfDeath != MOD_REPEATER_ALT &&
		ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
		ent->methodOfDeath != MOD_CONC &&
		ent->methodOfDeath != MOD_CONC_ALT &&
		other->client->ps.saberBlockTime < level.time &&
		!isKnockedSaber &&
		//[BoltBlockSys]
		//use the OJP version of the sabercanblock.
		OJP_SaberCanBlock(other, ent, qfalse, vec3_origin, -1, -1) )
		//WP_SaberCanBlock(other, ent->r.currentOrigin, 0, 0, qtrue, 0))
		//[/BoltBlockSys]
	*/
	//[/BoltBlockSys]
	{ //only block one projectile per 200ms (to prevent giant swarms of projectiles being blocked)
		//[BoltBlockSys]
		//racc - missile hit the actual player and it's a type of missile that you can deflect/ref with the saber.

		//racc - play projectile block animation
		other->client->ps.weaponTime = 0;
		WP_SaberBlockNonRandom(other, ent->r.currentOrigin, qtrue);

		OJP_HandleBoltBlock(ent, other, trace);
		//G_Printf("%i: Auto bolt block.\n", other->s.number);
		/*
		vec3_t fwd;
		gentity_t *te;
		int otherDefLevel = other->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE];

		te = G_TempEntity( ent->r.currentOrigin, EV_SABER_BLOCK );
		VectorCopy(ent->r.currentOrigin, te->s.origin);
		VectorCopy(trace->plane.normal, te->s.angles);
		te->s.eventParm = 0;
		te->s.weapon = 0;//saberNum
		te->s.legsAnim = 0;//bladeNum

		/*if (other->client->ps.velocity[2] > 0 ||
			other->client->pers.cmd.forwardmove ||
			other->client->pers.cmd.rightmove)
			*//*

		if (other->client->ps.velocity[2] > 0 ||
			other->client->pers.cmd.forwardmove < 0) //now we only do it if jumping or running backward. Should be able to full-on charge.
		{
			otherDefLevel -= 1;
			if (otherDefLevel < 0)
			{
				otherDefLevel = 0;
			}
		}

		AngleVectors(other->client->ps.viewangles, fwd, NULL, NULL);
		if (otherDefLevel == FORCE_LEVEL_1)
		{
			//if def is only level 1, instead of deflecting the shot it should just die here
		}
		else if (otherDefLevel == FORCE_LEVEL_2)
		{
			G_DeflectMissile(other, ent, fwd);
		}
		else
		{
			G_ReflectMissile(other, ent, fwd);
		}
		other->client->ps.saberBlockTime = level.time + (350 - (otherDefLevel*100)); //200;

		//For jedi AI
		other->client->ps.saberEventFlags |= SEF_DEFLECTED;

		if (otherDefLevel == FORCE_LEVEL_3)
		{
			other->client->ps.saberBlockTime = 0; //^_^
		}

		if (otherDefLevel == FORCE_LEVEL_1)
		{
			goto killProj;
		}
		*/
		//[DodgeSys]
		return qtrue;
		//return;
		//[/DodgeSys]
		//[/BoltBlockSys]
	}
	else if ((other->r.contents & CONTENTS_LIGHTSABER) && !isKnockedSaber)
	{ //hit this person's saber, so..
		gentity_t *otherOwner = &g_entities[other->r.ownerNum];

		if (otherOwner->takedamage && otherOwner->client &&
			ent->s.weapon != WP_ROCKET_LAUNCHER &&
			ent->s.weapon != WP_THERMAL &&
			ent->s.weapon != WP_TRIP_MINE &&
			ent->s.weapon != WP_DET_PACK &&
			//[BoltBlockSys]
			//ent->s.weapon != WP_DEMP2 &&
			//[BoltBlockSys]
			ent->methodOfDeath != MOD_REPEATER_ALT &&
			ent->methodOfDeath != MOD_FLECHETTE_ALT_SPLASH &&
			ent->methodOfDeath != MOD_CONC &&
			ent->methodOfDeath != MOD_CONC_ALT /*&&
			otherOwner->client->ps.saberBlockTime < level.time*/)
		{ //for now still deflect even if saberBlockTime >= level.time because it hit the actual saber
			//[BoltBlockSys]
			/* racc - retooled into the unified OJP_HandleBoltBlock function.
			vec3_t fwd;
			gentity_t *te;
			int otherDefLevel = otherOwner->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE];
			*/
			//[/BoltBlockSys]

			//in this case, deflect it even if we can't actually block it because it hit our saber
			//WP_SaberCanBlock(otherOwner, ent->r.currentOrigin, 0, 0, qtrue, 0);
			if ((otherOwner->client && !BG_SaberInAttack(otherOwner->client->ps.saberMove))
				|| (otherOwner->client && (pm->cmd.buttons & BUTTON_FORCEPOWER || pm->cmd.buttons & BUTTON_FORCEGRIP
		         || pm->cmd.buttons & BUTTON_FORCE_LIGHTNING) ))
			{//racc - play projectile block animation even in .
				otherOwner->client->ps.weaponTime = 0;
				WP_SaberBlockNonRandom(otherOwner, ent->r.currentOrigin, qtrue);
			}

			//[BoltBlockSys]
			OJP_HandleBoltBlock(ent, otherOwner, trace);
			//G_Printf("%i: Bolt deflected since it hit the actual saber\n", other->s.number);

			/*
			te = G_TempEntity( ent->r.currentOrigin, EV_SABER_BLOCK );
			VectorCopy(ent->r.currentOrigin, te->s.origin);
			VectorCopy(trace->plane.normal, te->s.angles);
			te->s.eventParm = 0;
			te->s.weapon = 0;//saberNum
			te->s.legsAnim = 0;//bladeNum

			/*if (otherOwner->client->ps.velocity[2] > 0 ||
				otherOwner->client->pers.cmd.forwardmove ||
				otherOwner->client->pers.cmd.rightmove)*//*
			if (otherOwner->client->ps.velocity[2] > 0 ||
				otherOwner->client->pers.cmd.forwardmove < 0) //now we only do it if jumping or running backward. Should be able to full-on charge.
			{
				otherDefLevel -= 1;
				if (otherDefLevel < 0)
				{
					otherDefLevel = 0;
				}
			}

			AngleVectors(otherOwner->client->ps.viewangles, fwd, NULL, NULL);

			if (otherDefLevel == FORCE_LEVEL_1)
			{
				//if def is only level 1, instead of deflecting the shot it should just die here
			}
			else if (otherDefLevel == FORCE_LEVEL_2)
			{
				G_DeflectMissile(otherOwner, ent, fwd);
			}
			else
			{
				G_ReflectMissile(otherOwner, ent, fwd);
			}
			otherOwner->client->ps.saberBlockTime = level.time + (350 - (otherDefLevel*100));//200;

			//For jedi AI
			otherOwner->client->ps.saberEventFlags |= SEF_DEFLECTED;

			if (otherDefLevel == FORCE_LEVEL_3)
			{
				otherOwner->client->ps.saberBlockTime = 0; //^_^
			}

			if (otherDefLevel == FORCE_LEVEL_1)
			{
				goto killProj;
			}
			*/
			//[/BoltBlockSys]
			//[DodgeSys]
			return qtrue;
			//return;
			//[/DodgeSys]
		}
	}

	// check for sticking
	//[SaberThrowSys]
	if ( !other->takedamage && ( ent->s.eFlags & EF_MISSILE_STICK ) 
		&& ent->s.weapon != WP_SABER)
	//if ( !other->takedamage && ( ent->s.eFlags & EF_MISSILE_STICK ) ) 
	//[/SaberThrowSys]
	{
		laserTrapStick( ent, trace->endpos, trace->plane.normal );
		G_AddEvent( ent, EV_MISSILE_STICK, 0 );
		//[DodgeSys]
		return qtrue;
		//return;
		//[/DodgeSys]
	}

	// impact damage
	if (other->takedamage && !isKnockedSaber) {
		//[DodgeSys]
		//make players be able to dodge projectiles.
		missileDmg = ent->damage;
		if(G_DoDodge(other, &g_entities[other->r.ownerNum], trace->endpos, -1, &missileDmg, ent->methodOfDeath))
		{//player dodged the damage, have missile continue moving.
			return qfalse;
		}
		//[/DodgeSys]

		// FIXME: wrong damage direction?
		//[DodgeSys]
		if ( missileDmg ) {
		//if ( ent->damage ) {
		//[/DodgeSys]
			vec3_t	velocity;
			qboolean didDmg = qfalse;

			if( LogAccuracyHit( other, &g_entities[ent->r.ownerNum] ) ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
				hitClient = qtrue;
			}
			BG_EvaluateTrajectoryDelta( &ent->s.pos, level.time, velocity );
			if ( VectorLength( velocity ) == 0 ) {
				velocity[2] = 1;	// stepped on a grenade
			}

			if (ent->s.weapon == WP_BOWCASTER || ent->s.weapon == WP_FLECHETTE ||
				ent->s.weapon == WP_ROCKET_LAUNCHER)
			{
				if (ent->s.weapon == WP_FLECHETTE && (ent->s.eFlags & EF_ALT_FIRING))
				{
					ent->think(ent);
				}
				else
				{
					G_Damage (other, ent, &g_entities[ent->r.ownerNum], velocity,
						//[DodgeSys]
						/*ent->s.origin*/ent->r.currentOrigin, missileDmg, 
						/*ent->s.origin*///ent->r.currentOrigin, ent->damage, 
						//[/DodgeSys]
						DAMAGE_HALF_ABSORB, ent->methodOfDeath);
					didDmg = qtrue;
				}
			}
			else
			//if(ent->s.weapon == WP_BLASTER || ent->s.weapon == WP_REPEATER
			//|| ent->s.weapon == WP_BOWCASTER || ent->s.weapon == WP_BRYAR_PISTOL)
				{
					gentity_t *owner = &g_entities[ent->r.ownerNum];
				float distance = VectorDistance(owner->r.currentOrigin,other->r.currentOrigin);
				//G_Printf("Distance: %f\n",distance);
				if(distance <= 100.0f)
				{
						G_Damage (other, ent, owner, velocity,
						//[DodgeSys]
						/*ent->s.origin*/ent->r.currentOrigin, missileDmg * 2,
						/*ent->s.origin*///ent->r.currentOrigin, ent->damage, 
						//[/DodgeSys]
						0, ent->methodOfDeath);
//G_Printf("Damage: %i\n",missileDmg * 2);
				}
				else if (distance <= 300.0f)
				{
						G_Damage (other, ent, owner, velocity,
						//[DodgeSys]
						/*ent->s.origin*/ent->r.currentOrigin, missileDmg * 1.5,
						/*ent->s.origin*///ent->r.currentOrigin, ent->damage, 
						//[/DodgeSys]
						0, ent->methodOfDeath);
//G_Printf("Damage: %f\n",missileDmg * 1.5);
				}
				else
				{
				G_Damage (other, ent, &g_entities[ent->r.ownerNum], velocity,
					//[DodgeSys]
					/*ent->s.origin*/ent->r.currentOrigin, missileDmg,
					/*ent->s.origin*///ent->r.currentOrigin, ent->damage, 
					//[/DodgeSys]
					0, ent->methodOfDeath);
				}
				didDmg = qtrue;
			}
			//else
			//{
				
				//G_Damage (other, ent, &g_entities[ent->r.ownerNum], velocity, //previous code
					//[DodgeSys]
					/*ent->s.origin*///ent->r.currentOrigin, missileDmg,
					/*ent->s.origin*///ent->r.currentOrigin, ent->damage, 
					//[/DodgeSys]
					//0, ent->methodOfDeath);
				//didDmg = qtrue;
				
			//}


			if (didDmg && other && other->client)
			{ //What I'm wondering is why this isn't in the NPC pain funcs. But this is what SP does, so whatever.
				class_t	npc_class = other->client->NPC_class;

				// If we are a robot and we aren't currently doing the full body electricity...
				if ( npc_class == CLASS_SEEKER || npc_class == CLASS_PROBE || npc_class == CLASS_MOUSE ||
					   npc_class == CLASS_GONK || npc_class == CLASS_R2D2 || npc_class == CLASS_R5D2 || npc_class == CLASS_REMOTE ||
					   npc_class == CLASS_MARK1 || npc_class == CLASS_MARK2 || //npc_class == CLASS_PROTOCOL ||//no protocol, looks odd
					   npc_class == CLASS_INTERROGATOR || npc_class == CLASS_ATST || npc_class == CLASS_SENTRY )
				{
					// special droid only behaviors
					if ( other->client->ps.electrifyTime < level.time + 100 )
					{
						// ... do the effect for a split second for some more feedback
						other->client->ps.electrifyTime = level.time + 450;
					}
					//FIXME: throw some sparks off droids,too
				}
			}
		}

		if ( ent->s.weapon == WP_DEMP2 )
		{//a hit with demp2 decloaks people, disables ships
			if ( other && other->client && other->client->NPC_class == CLASS_VEHICLE )
			{//hit a vehicle
				if ( other->m_pVehicle //valid vehicle ent
					&& other->m_pVehicle->m_pVehicleInfo//valid stats
					&& (other->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER//always affect speeders
						||(other->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER && ent->classname && Q_stricmp("vehicle_proj", ent->classname ) == 0) )//only vehicle ion weapons affect a fighter in this manner
					&& !FighterIsLanded( other->m_pVehicle , &other->client->ps )//not landed
					&& !(other->spawnflags&2) )//and not suspended
				{//vehicles hit by "ion cannons" lose control
					if ( other->client->ps.electrifyTime > level.time )
					{//add onto it
						//FIXME: extern the length of the "out of control" time?
						other->client->ps.electrifyTime += Q_irand(200,500);
						if ( other->client->ps.electrifyTime > level.time + 4000 )
						{//cap it
							other->client->ps.electrifyTime = level.time + 4000;
						}
					}
					else
					{//start it
						//FIXME: extern the length of the "out of control" time?
						other->client->ps.electrifyTime = level.time + Q_irand(200,500);
					}
				}
			}
			else if ( other && other->client && other->client->ps.powerups[PW_CLOAKED] )
			{
				Jedi_Decloak( other );
				if ( ent->methodOfDeath == MOD_DEMP2_ALT )
				{//direct hit with alt disables cloak forever
					//permanently disable the saboteur's cloak
					other->client->cloakToggleTime = Q3_INFINITE;
				}
				else
				{//temp disable
					other->client->cloakToggleTime = level.time + Q_irand( 3000, 10000 );
				}
			}
		}
	}
killProj:
	// is it cheaper in bandwidth to just remove this ent and create a new
	// one, rather than changing the missile into the explosion?

	if ( other->takedamage && other->client && !isKnockedSaber ) {
		G_AddEvent( ent, EV_MISSILE_HIT, DirToByte( trace->plane.normal ) );
		ent->s.otherEntityNum = other->s.number;
	} else if( trace->surfaceFlags & SURF_METALSTEPS ) {
		G_AddEvent( ent, EV_MISSILE_MISS_METAL, DirToByte( trace->plane.normal ) );
	} else if (ent->s.weapon != G2_MODEL_PART && !isKnockedSaber) {
		G_AddEvent( ent, EV_MISSILE_MISS, DirToByte( trace->plane.normal ) );
	}

	if (!isKnockedSaber)
	{
		ent->freeAfterEvent = qtrue;

		// change over to a normal entity right at the point of impact
		ent->s.eType = ET_GENERAL;
	}

	SnapVectorTowards( trace->endpos, ent->s.pos.trBase );	// save net bandwidth

	G_SetOrigin( ent, trace->endpos );

	ent->takedamage = qfalse;
	// splash damage (doesn't apply to person directly hit)
	if ( ent->splashDamage ) {
		if( G_RadiusDamage( trace->endpos, ent->parent, ent->splashDamage, ent->splashRadius, 
			other, ent, ent->splashMethodOfDeath ) ) {
			if( !hitClient 
				&& g_entities[ent->r.ownerNum].client ) {
				g_entities[ent->r.ownerNum].client->accuracy_hits++;
			}
		}
	}

	if (ent->s.weapon == G2_MODEL_PART)
	{
		ent->freeAfterEvent = qfalse; //it will free itself
	}

	trap_LinkEntity( ent );

	//[DodgeSys]
	return qtrue;
	//[/DodgeSys]
}

/*
================
G_RunMissile
================
*/
//[RealTrace]
extern int G_RealTrace(gentity_t *SaberAttacker, trace_t *tr, vec3_t start, vec3_t mins, 
										vec3_t maxs, vec3_t end, int passEntityNum, 
										int contentmask, int rSaberNum, int rBladeNum);
//[/RealTrace]
void G_RunMissile( gentity_t *ent ) {
	vec3_t		origin, groundSpot;
	trace_t		tr;
	int			passent;
	qboolean	isKnockedSaber = qfalse;

	if (ent->neverFree && ent->s.weapon == WP_SABER && (ent->flags & FL_BOUNCE_HALF))
	{
		isKnockedSaber = qtrue;
		//[SaberThrowSys]
		if(!(ent->s.eFlags & EF_MISSILE_STICK) )
		{//only go into gravity mode if we're not stuck to something
			ent->s.pos.trType = TR_GRAVITY;
		}
		//ent->s.pos.trType = TR_GRAVITY;
		//[/SaberThrowSys]
	}

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// if this missile bounced off an invulnerability sphere
	if ( ent->target_ent ) {
		passent = ent->target_ent->s.number;
	}
	else {
		// ignore interactions with the missile owner
		if ( (ent->r.svFlags&SVF_OWNERNOTSHARED) 
			&& (ent->s.eFlags&EF_JETPACK_ACTIVE) )
		{//A vehicle missile that should be solid to its owner
			//I don't care about hitting my owner
			passent = ent->s.number;
		}
		else
		{
			passent = ent->r.ownerNum;
		}
	}
	// trace a line from the previous position to the current position
	//[RealTrace]
	G_RealTrace( ent, &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask, -1, -1 );

	/*
	if (d_projectileGhoul2Collision.integer)
	{
		trap_G2Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask, G2TRFLAG_DOGHOULTRACE|G2TRFLAG_GETSURFINDEX|G2TRFLAG_THICK|G2TRFLAG_HITCORPSES, g_g2TraceLod.integer );

		if (tr.fraction != 1.0 && tr.entityNum < ENTITYNUM_WORLD)
		{
			gentity_t *g2Hit = &g_entities[tr.entityNum];

			if (g2Hit->inuse && g2Hit->client && g2Hit->ghoul2)
			{ //since we used G2TRFLAG_GETSURFINDEX, tr.surfaceFlags will actually contain the index of the surface on the ghoul2 model we collided with.
				g2Hit->client->g2LastSurfaceHit = tr.surfaceFlags;
				g2Hit->client->g2LastSurfaceTime = level.time;
				//[BugFix12]
				//BUGFIX12RAFIXME - ugh, can't seem to get the model index on the 
				//trap_G2Traces.  These probably need to be replaced with the more
				//indepth G2traces.  For now, just assume that the player model was hit.
				g2Hit->client->g2LastSurfaceModel = G2MODEL_PLAYER;
				//[/BugFix12]
			}

			if (g2Hit->ghoul2)
			{
				tr.surfaceFlags = 0; //clear the surface flags after, since we actually care about them in here.
			}
		}
	}
	else
	{
		trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin, passent, ent->clipmask );
	}
	*/
	//[/RealTrace]

	if ( tr.startsolid || tr.allsolid ) {
		// make sure the tr.entityNum is set to the entity we're stuck in
		//[RealTrace]
		//trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, passent, ent->clipmask );
		//tr.fraction = 0;
		//[/RealTrace]
	}
	else {
		VectorCopy( tr.endpos, ent->r.currentOrigin );
	}

	if (ent->passThroughNum && tr.entityNum == (ent->passThroughNum-1))
	{
		VectorCopy( origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
		goto passthrough;
	}

	trap_LinkEntity( ent );
	//racc - assign groundEntityNum for body parts.
	if (ent->s.weapon == G2_MODEL_PART && !ent->bounceCount)
	{
		vec3_t lowerOrg;
		trace_t trG;

		VectorCopy(ent->r.currentOrigin, lowerOrg);
		lowerOrg[2] -= 1;
		trap_Trace( &trG, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, lowerOrg, passent, ent->clipmask );

		VectorCopy(trG.endpos, groundSpot);

		if (!trG.startsolid && !trG.allsolid && trG.entityNum == ENTITYNUM_WORLD)
		{
			ent->s.groundEntityNum = trG.entityNum;
		}
		else
		{
			ent->s.groundEntityNum = ENTITYNUM_NONE;
		}
	}

	if ( tr.fraction != 1) {
		// never explode or bounce on sky
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// If grapple, reset owner
			if (ent->parent && ent->parent->client && ent->parent->client->hook == ent) {
				ent->parent->client->hook = NULL;
			}

			//racc - make dropped sabers think when they hit a non-impact surface.
			if ((ent->s.weapon == WP_SABER && ent->isSaberEntity) || isKnockedSaber)
			{
				G_RunThink( ent );
				return;
			}
			//just kill off other weapon shots when they hit a non-impact surface.
			else if (ent->s.weapon != G2_MODEL_PART)
			{
				G_FreeEntity( ent );
				return;
			}
		}

#if 0 //will get stomped with missile impact event...
		if (ent->s.weapon > WP_NONE && ent->s.weapon < WP_NUM_WEAPONS &&
			(tr.entityNum < MAX_CLIENTS || g_entities[tr.entityNum].s.eType == ET_NPC))
		{ //player or NPC, try making a mark on him
			/*
			gentity_t *evEnt = G_TempEntity(ent->r.currentOrigin, EV_GHOUL2_MARK);

			evEnt->s.owner = tr.entityNum; //the entity the mark should be placed on
			evEnt->s.weapon = ent->s.weapon; //the weapon used (to determine mark type)
			VectorCopy(ent->r.currentOrigin, evEnt->s.origin); //the point of impact

			//origin2 gets the predicted trajectory-based position.
			BG_EvaluateTrajectory( &ent->s.pos, level.time, evEnt->s.origin2 );

			//If they are the same, there will be problems.
			if (VectorCompare(evEnt->s.origin, evEnt->s.origin2))
			{
				evEnt->s.origin2[2] += 2; //whatever, at least it won't mess up.
			}
			*/
			//ok, let's try adding it to the missile ent instead (tempents bad!)
			G_AddEvent(ent, EV_GHOUL2_MARK, 0);

			//copy current pos to s.origin, and current projected traj to origin2
			VectorCopy(ent->r.currentOrigin, ent->s.origin);
			BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->s.origin2 );

			//the index for whoever we are hitting
			ent->s.otherEntityNum = tr.entityNum;

			if (VectorCompare(ent->s.origin, ent->s.origin2))
			{
				ent->s.origin2[2] += 2.0f; //whatever, at least it won't mess up.
			}
		}
#else
		if (ent->s.weapon > WP_NONE && ent->s.weapon < WP_NUM_WEAPONS &&
			(tr.entityNum < MAX_CLIENTS || g_entities[tr.entityNum].s.eType == ET_NPC))
		{ //player or NPC, try making a mark on him
			//copy current pos to s.origin, and current projected traj to origin2
			VectorCopy(ent->r.currentOrigin, ent->s.origin);
			BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->s.origin2 );

			if (VectorCompare(ent->s.origin, ent->s.origin2))
			{//racc - don't allow the current origin/predicted origin be the same.
				ent->s.origin2[2] += 2.0f; //whatever, at least it won't mess up.
			}
		}
#endif

		//[DodgeSys]
		//changed G_MissileImpact to qboolean so that dodges will cause passthru behavor.
		if(!G_MissileImpact( ent, &tr ))
		{//target dodged the damage.
			VectorCopy( origin, ent->r.currentOrigin );  
			trap_LinkEntity( ent );
			return;
		}
		//G_MissileImpact( ent, &tr );
		//[/DodgeSys]

		if (tr.entityNum == ent->s.otherEntityNum)
		{ //if the impact event other and the trace ent match then it's ok to do the g2 mark
			ent->s.trickedentindex = 1;
		}

		if ( ent->s.eType != ET_MISSILE && ent->s.weapon != G2_MODEL_PART )
		{
			return;		// exploded
		}
	}

passthrough:
	if ( ent->s.pos.trType == TR_STATIONARY && (ent->s.eFlags&EF_MISSILE_STICK) )
	{//stuck missiles should check some special stuff
		G_RunStuckMissile( ent );
		return;
	}

	if (ent->s.weapon == G2_MODEL_PART)
	{
		if (ent->s.groundEntityNum == ENTITYNUM_WORLD)
		{
			ent->s.pos.trType = TR_LINEAR;
			VectorClear(ent->s.pos.trDelta);
			ent->s.pos.trTime = level.time;

			VectorCopy(groundSpot, ent->s.pos.trBase);
			VectorCopy(groundSpot, ent->r.currentOrigin);

			if (ent->s.apos.trType != TR_STATIONARY)
			{
				ent->s.apos.trType = TR_STATIONARY;
				ent->s.apos.trTime = level.time;

				ent->s.apos.trBase[ROLL] = 0;
				ent->s.apos.trBase[PITCH] = 0;
			}
		}
	}

	if(ent->damageDecreaseTime && ent->damageDecreaseTime <= level.time)
	{
		ent->damage-=4;
		ent->damageDecreaseTime = level.time + 300;
	}

	// check think function after bouncing
	G_RunThink( ent );
}


//=============================================================================

//[BoltBlockSys]
//bolt reflection rate without manual reflections being attempted.
int NaturalBoltReflectRate[NUM_FORCE_POWER_LEVELS] =
{
	0,	//FORCE_LEVEL_0
	10,//10,	//FORCE_LEVEL_1
	20,//20, //FORCE_LEVEL_2
	33//25
};

//bolt reflection rate while attempting manual reflections.
int ManualBoltReflectRate[NUM_FORCE_POWER_LEVELS] =
{
	0,	//FORCE_LEVEL_0
	33,//20,	//FORCE_LEVEL_1
	66,//40, //FORCE_LEVEL_2
	100//50
};
qboolean PM_SaberInStart( int move );
extern int OJP_SaberBlockCost(gentity_t *defender, gentity_t *attacker, vec3_t hitLoc);
extern qboolean PM_SaberInReturn( int move );
extern void BG_AddFatigue( playerState_t * ps, int Fatigue);
void OJP_HandleBoltBlock(gentity_t *bolt, gentity_t *player, trace_t *trace)
{//handles all the behavior needed to saber block a blaster bolt.  
	//I created this function to unite the duplicated code in G_MissileImpact
	vec3_t fwd;
	gentity_t *te;
	int otherDefLevel;
	gentity_t *prevOwner = &g_entities[bolt->r.ownerNum]; //previous owner of the bolt.  Used for awarding experience to attacker.
	float distance = VectorDistance(prevOwner->r.currentOrigin,player->r.currentOrigin);
	//create the bolt saber block effect
	te = G_TempEntity( bolt->r.currentOrigin, EV_SABER_BLOCK );
	VectorCopy(bolt->r.currentOrigin, te->s.origin);
	VectorCopy(trace->plane.normal, te->s.angles);
	te->s.eventParm = 0;
	te->s.weapon = 0;//saberNum
	te->s.legsAnim = 0;//bladeNum

	//determine reflection level.
	if((BG_SaberInAttack(player->client->ps.saberMove)
	|| PM_SaberInStart(player->client->ps.saberMove)
	|| PM_SaberInReturn(player->client->ps.saberMove)) 
	&& Q_irand(0, 99) <  ManualBoltReflectRate[player->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE]])
	{//manual reflection, bounce to the crosshair, roughly
		otherDefLevel = FORCE_LEVEL_3;
	}
	else if(Q_irand(0, 99) <  NaturalBoltReflectRate[player->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE]])
	{//natural reflection, bounce back to the attacker.
		otherDefLevel = FORCE_LEVEL_2;
	}
	else
	{//just deflect the attack
		otherDefLevel = FORCE_LEVEL_1;
	}

	//G_Printf("%i: Bounced Bolt @ Level %i\n", player->s.number, otherDefLevel);

	AngleVectors(player->client->ps.viewangles, fwd, NULL, NULL);
	if (otherDefLevel <= FORCE_LEVEL_1)
	{//only randomly deflect away the bolt
		G_DeflectMissile(player, bolt, fwd);
	}
	else if (otherDefLevel == FORCE_LEVEL_2)
	{//bounce the bolt back to sender
		//G_Printf("%i: %i: Level 2 Reflect\n", level.time, player->s.number);
		G_DeflectMissile(player, bolt, fwd);
	}/*
	else if(otherDefLevel == FORCE_LEVEL_3 
		&& distance < 80.0f
		&&(BG_SaberInAttack( player->client->ps.saberMove )
		|| PM_SaberInStart( player->client->ps.saberMove)))
		{//manual reflection, bounce to the crosshair, roughly
			G_DeflectMissile(player, bolt, fwd);;
		}*/
	else if(player->client->ps.fd.saberAnimLevel == SS_FAST
		|| player->client->ps.fd.saberAnimLevel == SS_MEDIUM
		|| player->client->ps.fd.saberAnimLevel == SS_TAVION)
	{
		G_DeflectMissile(player, bolt, fwd);
	}
	else
	{//FORCE_LEVEL_3, reflect the bolt to whereever the player is aiming.
		gentity_t *owner = &g_entities[player->r.ownerNum];
				float distance = VectorDistance(owner->r.currentOrigin, prevOwner->r.currentOrigin);
			if(distance < 80.0f)
		{//attempted upclose exception
			G_DeflectMissile(player, bolt, fwd);;
		}
		else
		{
		vec3_t	bounce_dir, angs;
		float	speed;
		//gentity_t	*owner = ent;
		//int		isowner = 0;
        
		//add some slop factor to the manual reflections.
		if(player->client->pers.cmd.forwardmove >= 0)
		{
			float slopFactor = (MISHAP_MAXINACCURACY-6) * (FORCE_LEVEL_3 - player->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE])/FORCE_LEVEL_3;
			//[MoreRandom]
			float distance = VectorDistance(player->r.currentOrigin,prevOwner->r.currentOrigin);
				slopFactor += Q_irand(1,5);
			vectoangles( fwd, angs );
			angs[PITCH] += flrand(-slopFactor, slopFactor);
			angs[YAW] += flrand(-slopFactor, slopFactor);
			AngleVectors( angs, fwd, NULL, NULL );
		}
		else
		{
			vectoangles( fwd, angs );
			AngleVectors( angs, fwd, NULL, NULL );
		}
		
		//G_Printf("%i: %i: Level 3 Reflect\n", level.time, player->s.number);

		//save the original speed
		speed = VectorNormalize( bolt->s.pos.trDelta );

		VectorCopy(fwd, bounce_dir);

		VectorScale( bounce_dir, speed, bolt->s.pos.trDelta );
		bolt->s.pos.trTime = level.time;		// move a bit on the very first frame
		VectorCopy( bolt->r.currentOrigin, bolt->s.pos.trBase );
		if ( bolt->s.weapon != WP_SABER && bolt->s.weapon != G2_MODEL_PART )
		{//you are mine, now!
			bolt->r.ownerNum = player->s.number;
		}
		if ( bolt->s.weapon == WP_ROCKET_LAUNCHER )
		{//stop homing
			bolt->think = 0;
			bolt->nextthink = 0;
		}
		}
		
	}

	//For jedi AI
	player->client->ps.saberEventFlags |= SEF_DEFLECTED;

	//deduce DP cost
	//[ExpSys]
	bolt->activator = prevOwner;

	if(otherDefLevel == FORCE_LEVEL_3
		&& player->client->pers.cmd.forwardmove < 0)
	{
		int amount = OJP_SaberBlockCost(player, bolt, trace->endpos);
		amount/=100*40;
		G_DodgeDrain(player, prevOwner, amount);
	}
	else
		G_DodgeDrain(player, prevOwner, OJP_SaberBlockCost(player, bolt, trace->endpos));
	//[ExpSys]

	//[SaberLockSys]
	player->client->ps.saberLockFrame = 0; //break out of saberlocks.
	//[/SaberLockSys]

	//debounce time between blocks.
	player->client->ps.saberBlockTime = level.time + (600 - (player->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE]*200));
}
//[/BoltBlockSys]




