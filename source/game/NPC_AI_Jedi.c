//[SPPortComplete]
#include "b_local.h"
#include "g_nav.h"
#include "anims.h"
#include "w_saber.h"

#include "../namespace_begin.h"
extern qboolean BG_SabersOff( playerState_t *ps );
#include "../namespace_end.h"

extern void CG_DrawAlert( vec3_t origin, float rating );
extern void G_AddVoiceEvent( gentity_t *self, int event, int speakDebounceTime );
extern void ForceJump( gentity_t *self, usercmd_t *ucmd );
extern vmCvar_t	g_saberRealisticCombat;
extern vmCvar_t	d_slowmodeath;

void G_StartMatrixEffect( gentity_t *ent )
{ //perhaps write this at some point?

}

#define	MAX_VIEW_DIST		2048
#define MAX_VIEW_SPEED		100
#define	JEDI_MAX_LIGHT_INTENSITY 64
#define	JEDI_MIN_LIGHT_THRESHOLD 10
#define	JEDI_MAX_LIGHT_THRESHOLD 50

#define	DISTANCE_SCALE		0.25f
//#define	DISTANCE_THRESHOLD	0.075f
#define	SPEED_SCALE			0.25f
#define	FOV_SCALE			0.5f
#define	LIGHT_SCALE			0.25f

#define	REALIZE_THRESHOLD	0.6f
#define CAUTIOUS_THRESHOLD	( REALIZE_THRESHOLD * 0.3 )

#define	MAX_CHECK_THRESHOLD	1

extern void NPC_ClearLookTarget( gentity_t *self );
extern void NPC_SetLookTarget( gentity_t *self, int entNum, int clearTime );
extern void NPC_TempLookTarget( gentity_t *self, int lookEntNum, int minLookTime, int maxLookTime );
extern qboolean G_ExpandPointToBBox( vec3_t point, const vec3_t mins, const vec3_t maxs, int ignore, int clipmask );
extern qboolean NPC_CheckEnemyStealth( void );
extern void G_SoundOnEnt( gentity_t *ent, soundChannel_t channel, const char *soundPath );

#include "../namespace_begin.h"
extern gitem_t	*BG_FindItemForAmmo( ammo_t ammo );
#include "../namespace_end.h"

extern void ForceThrow( gentity_t *self, qboolean pull );
extern void ForceLightning( gentity_t *self );
extern void ForceHeal( gentity_t *self );
extern void ForceRage( gentity_t *self );
extern void ForceProtect( gentity_t *self );
extern void ForceAbsorb( gentity_t *self );
extern int WP_MissileBlockForBlock( int saberBlock );
extern qboolean G_GetHitLocFromSurfName( gentity_t *ent, const char *surfName, int *hitLoc, vec3_t point, vec3_t dir, vec3_t bladeDir, int mod );
extern qboolean WP_ForcePowerUsable( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
extern qboolean WP_ForcePowerAvailable( gentity_t *self, forcePowers_t forcePower, int overrideAmt );
extern void WP_ForcePowerStop( gentity_t *self, forcePowers_t forcePower );
extern void WP_DeactivateSaber( gentity_t *self, qboolean clearLength ); //clearLength = qfalse
extern void WP_ActivateSaber( gentity_t *self );
//extern void WP_SaberBlock(gentity_t *saber, vec3_t hitloc);

#include "../namespace_begin.h"
extern qboolean PM_SaberInStart( int move );
extern qboolean BG_SaberInSpecialAttack( int anim );
extern qboolean BG_SaberInAttack( int move );
extern qboolean PM_SaberInBounce( int move );
extern qboolean PM_SaberInParry( int move );
extern qboolean PM_SaberInKnockaway( int move );
extern qboolean PM_SaberInBrokenParry( int move );
extern qboolean PM_SaberInDeflect( int move );
extern qboolean BG_SpinningSaberAnim( int anim );
extern qboolean BG_FlippingAnim( int anim );
extern qboolean PM_RollingAnim( int anim );
extern qboolean PM_InKnockDown( playerState_t *ps );
extern qboolean BG_InRoll( playerState_t *ps, int anim );
extern qboolean BG_CrouchAnim( int anim );
#include "../namespace_end.h"

extern qboolean NPC_SomeoneLookingAtMe(gentity_t *ent);

extern int WP_GetVelocityForForceJump( gentity_t *self, vec3_t jumpVel, usercmd_t *ucmd );

extern void G_TestLine(vec3_t start, vec3_t end, int color, int time);

static void Jedi_Aggression( gentity_t *self, int change );
qboolean Jedi_WaitingAmbush( gentity_t *self );

#include "../namespace_begin.h"
extern int bg_parryDebounce[];
#include "../namespace_end.h"

static int	jediSpeechDebounceTime[TEAM_NUM_TEAMS];//used to stop several jedi from speaking all at once
//Local state enums
enum
{
	LSTATE_NONE = 0,
	LSTATE_UNDERFIRE,
	LSTATE_INVESTIGATE,
};

//[CoOp]
void NPC_CultistDestroyer_Precache( void )
{//precashe for cultist destroyer
	G_SoundIndex( "sound/movers/objects/green_beam_lp2.wav" );
	G_EffectIndex( "force/destruction_exp" );
}
//[/CoOp]

void NPC_ShadowTrooper_Precache( void )
{
	RegisterItem( BG_FindItemForAmmo( AMMO_FORCE ) );
	G_SoundIndex( "sound/chars/shadowtrooper/cloak.wav" );
	G_SoundIndex( "sound/chars/shadowtrooper/decloak.wav" );
}

//[CoOp]
void NPC_Rosh_Dark_Precache( void )
{//precashe for Rosh boss
	G_EffectIndex( "force/kothos_recharge.efx" );
	G_EffectIndex( "force/kothos_beam.efx" );
}
//[/CoOp]

void Jedi_ClearTimers( gentity_t *ent )
{
	TIMER_Set( ent, "roamTime", 0 );
	TIMER_Set( ent, "chatter", 0 );
	TIMER_Set( ent, "strafeLeft", 0 );
	TIMER_Set( ent, "strafeRight", 0 );
	TIMER_Set( ent, "noStrafe", 0 );
	TIMER_Set( ent, "walking", 0 );
	TIMER_Set( ent, "taunting", 0 );
	TIMER_Set( ent, "parryTime", 0 );
	TIMER_Set( ent, "parryReCalcTime", 0 );
	TIMER_Set( ent, "forceJumpChasing", 0 );
	TIMER_Set( ent, "jumpChaseDebounce", 0 );
	TIMER_Set( ent, "moveforward", 0 );
	TIMER_Set( ent, "moveback", 0 );
	TIMER_Set( ent, "movenone", 0 );
	TIMER_Set( ent, "moveright", 0 );
	TIMER_Set( ent, "moveleft", 0 );
	TIMER_Set( ent, "movecenter", 0 );
	TIMER_Set( ent, "saberLevelDebounce", 0 );
	TIMER_Set( ent, "noRetreat", 0 );
	TIMER_Set( ent, "holdLightning", 0 );
	TIMER_Set( ent, "gripping", 0 );
	TIMER_Set( ent, "draining", 0 );
	TIMER_Set( ent, "noturn", 0 );
	//[CoOp]
	TIMER_Set( ent, "specialEvasion", 0 );
	//[/CoOp]
}

//[CoOp]
//ported from SP
qboolean Jedi_CultistDestroyer( gentity_t *self )
{ 
	if ( !self || !self->client )
	{
		return qfalse;
	}
	//FIXME: just make a flag, dude!
	if ( self->client->NPC_class == CLASS_REBORN
		&& self->s.weapon == WP_MELEE
		&& Q_stricmp( "cultist_destroyer", self->NPC_type ) == 0 )
	{
		return qtrue;
	}
	return qfalse;
}
//[/CoOp]

void Jedi_PlayBlockedPushSound( gentity_t *self )
{
	if ( !self->s.number )
	{
		G_AddVoiceEvent( self, EV_PUSHFAIL, 3000 );
	}
	else if ( self->health > 0 && self->NPC && self->NPC->blockedSpeechDebounceTime < level.time )
	{
		G_AddVoiceEvent( self, EV_PUSHFAIL, 3000 );
		self->NPC->blockedSpeechDebounceTime = level.time + 3000;
	}
}

void Jedi_PlayDeflectSound( gentity_t *self )
{
	if ( !self->s.number )
	{
		G_AddVoiceEvent( self, Q_irand( EV_DEFLECT1, EV_DEFLECT3 ), 3000 );
	}
	else if ( self->health > 0 && self->NPC && self->NPC->blockedSpeechDebounceTime < level.time )
	{
		G_AddVoiceEvent( self, Q_irand( EV_DEFLECT1, EV_DEFLECT3 ), 3000 );
		self->NPC->blockedSpeechDebounceTime = level.time + 3000;
	}
}

void NPC_Jedi_PlayConfusionSound( gentity_t *self )
{
	if ( self->health > 0 )
	{
		if ( self->client
			//[CoOp]
			//added Alora class.
			&& ( self->client->NPC_class == CLASS_ALORA 
				|| self->client->NPC_class == CLASS_TAVION 
				|| self->client->NPC_class == CLASS_DESANN ) )
			//&& ( self->client->NPC_class == CLASS_TAVION || self->client->NPC_class == CLASS_DESANN ) )
			//[/CoOp]
		{
			G_AddVoiceEvent( self, Q_irand( EV_CONFUSE1, EV_CONFUSE3 ), 2000 );
		}
		else if ( Q_irand( 0, 1 ) )
		{
			G_AddVoiceEvent( self, Q_irand( EV_TAUNT1, EV_TAUNT3 ), 2000 );
		}
		else
		{
			G_AddVoiceEvent( self, Q_irand( EV_GLOAT1, EV_GLOAT3 ), 2000 );
		}
	}
}

//[CoOp]
qboolean Jedi_StopKnockdown( gentity_t *self, gentity_t *pusher, const vec3_t pushDir )
{//certain NPCs can avoid knockdowns, check for that.
	vec3_t	pDir, fwd, right, ang;
	float	fDot, rDot;
	usercmd_t	tempCmd;
	int		strafeTime = Q_irand( 1000, 2000 );

	if ( self->s.number < MAX_CLIENTS || !self->NPC )
	{//only NPCs
		return qfalse;
	}

	if ( self->client->ps.fd.forcePowerLevel[FP_LEVITATION] < FORCE_LEVEL_1 )
	{//only force-users
		return qfalse;
	}

	if ( self->client->ps.eFlags2&EF2_FLYING ) //moveType == MT_FLYSWIM )
	{//can't knock me down when I'm flying
		return qtrue;
	}

	if ( self->NPC && (self->NPC->aiFlags&NPCAI_BOSS_CHARACTER) )
	{//bosses always get out of a knockdown
	}
	else if ( Q_irand( 0, RANK_CAPTAIN+5 ) > self->NPC->rank )
	{//lower their rank, the more likely they are fall down
		return qfalse;
	}

	VectorSet(ang, 0, self->r.currentAngles[YAW], 0);

	AngleVectors( ang, fwd, right, NULL );
	VectorNormalize2( pushDir, pDir );
	fDot = DotProduct( pDir, fwd );
	rDot = DotProduct( pDir, right );

	//flip or roll with it
	if ( fDot >= 0.4f )
	{
		tempCmd.forwardmove = 127;
		TIMER_Set( self, "moveforward", strafeTime );
	}
	else if ( fDot <= -0.4f )
	{
		tempCmd.forwardmove = -127;
		TIMER_Set( self, "moveback", strafeTime );
	}
	else if ( rDot > 0 )
	{
		tempCmd.rightmove = 127;
		TIMER_Set( self, "strafeRight", strafeTime );
		TIMER_Set( self, "strafeLeft", -1 );
	}
	else
	{
		tempCmd.rightmove = -127;
		TIMER_Set( self, "strafeLeft", strafeTime );
		TIMER_Set( self, "strafeRight", -1 );
	}
	G_AddEvent( self, EV_JUMP, 0 );
	if ( !Q_irand( 0, 1 ) )
	{//flip
		self->client->ps.fd.forceJumpCharge = 280;//FIXME: calc this intelligently?
		ForceJump( self, &tempCmd );
	}
	else
	{//roll
		TIMER_Set( self, "duck", strafeTime );
	}
	self->painDebounceTime = 0;//so we do something

	return qtrue;
}


//===============================================================================================
//TAVION BOSS
//===============================================================================================
void NPC_TavionScepter_Precache( void )
{//precashe tavion boss scepter effects
	G_EffectIndex( "scepter/beam_warmup.efx" );
	G_EffectIndex( "scepter/beam.efx" );
	G_EffectIndex( "scepter/slam_warmup.efx" );
	G_EffectIndex( "scepter/slam.efx" );
	G_EffectIndex( "scepter/impact.efx" );
	G_SoundIndex( "sound/weapons/scepter/loop.wav" );
	G_SoundIndex( "sound/weapons/scepter/slam_warmup.wav" );
	G_SoundIndex( "sound/weapons/scepter/beam_warmup.wav" );
}

void NPC_TavionSithSword_Precache( void )
{//precashe tavion boss sithsword effects
	G_EffectIndex( "scepter/recharge.efx" );
	G_EffectIndex( "scepter/invincibility.efx" );
	G_EffectIndex( "scepter/sword.efx" );
	G_SoundIndex( "sound/weapons/scepter/recharge.wav" );
}

extern void G_Knockdown( gentity_t *victim );
extern float Q_flrand(float min, float max);
void Tavion_ScepterDamage( void )
{//Damage code for tavion's scepter weapon.
	if(!NPC->ghoul2 || !NPC->client->weaponGhoul2[1])
	{//sanity checks
		return;
	}

	if ( NPC->NPC->genericBolt1 != -1 )
	{
		int time;
		int curTime = level.time;
		qboolean hit = qfalse;
		int	lastHit = ENTITYNUM_NONE;
		
		for ( time = curTime-25; time <= curTime+25&&!hit; time += 25 )
		{
			mdxaBone_t	boltMatrix;
			vec3_t		tip, dir, base, angles;
			trace_t		trace;

			VectorSet(angles, 0, NPC->r.currentAngles[YAW], 0);

			trap_G2API_GetBoltMatrix( NPC->client->weaponGhoul2[1], 0, 
						NPC->NPC->genericBolt1,
						&boltMatrix, angles, NPC->r.currentOrigin, time,
						NULL, NPC->modelScale );
			BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, base );
			BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_X, dir );
			VectorMA( base, 512, dir, tip );
	/* debugging stuff we don't need at the moment
	#ifndef FINAL_BUILD
			if ( d_saberCombat.integer > 1 )
			{
				G_DebugLine(base, tip, 1000, 0x000000ff, qtrue);
			}
	#endif
	*/
			trap_Trace( &trace, base, vec3_origin, vec3_origin, tip, NPC->s.number, MASK_SHOT);
			if ( trace.fraction < 1.0f )
			{//hit something
				gentity_t *traceEnt = &g_entities[trace.entityNum];

				//FIXME: too expensive!
				//if ( time == curTime )
				{//UGH
					G_PlayEffect( G_EffectIndex( "scepter/impact.efx" ), trace.endpos, trace.plane.normal );
				}

				if ( traceEnt->takedamage 
					&& trace.entityNum != lastHit
					&& (!traceEnt->client || traceEnt == NPC->enemy || traceEnt->client->NPC_class != NPC->client->NPC_class) )
				{//smack
					int dmg = Q_irand( 10, 20 )*(g_spskill.integer+1);//NOTE: was 6-12
					//FIXME: debounce?
					//FIXME: do dismemberment
					G_Damage( traceEnt, NPC, NPC, vec3_origin, trace.endpos, dmg, DAMAGE_NO_KNOCKBACK, MOD_SABER );//MOD_MELEE );
					if ( traceEnt->client )
					{
						if ( !Q_irand( 0, 2 ) )
						{
							G_AddVoiceEvent( NPC, Q_irand( EV_CONFUSE1, EV_CONFUSE2 ), 10000 );
						}
						else
						{
							G_AddVoiceEvent( NPC, EV_JDETECTED3, 10000 );
						}
						G_Throw( traceEnt, dir, Q_flrand( 50, 80 ) );
						if ( traceEnt->health > 0 && !Q_irand( 0, 2 ) )//FIXME: base on skill!
						{//do pain on enemy
							//RAFIXME:  impliment multi directional knockdowns
							G_Knockdown( traceEnt );
							//G_Knockdown( traceEnt, NPC, dir, 300, qtrue );
						}
					}
					hit = qtrue;
					lastHit = trace.entityNum;
				}
			}
		}
	}
}

extern qboolean G_EntIsBreakable( int entityNum );
void Tavion_ScepterSlam( void )
{//Scepter slam attack
	int	boltIndex;

	if ( !NPC->ghoul2 || !NPC->client->weaponGhoul2[1])
	{
		return;
	}

	boltIndex = trap_G2API_AddBolt(NPC->client->weaponGhoul2[1], 0, "*weapon");
	if ( boltIndex != -1 )
	{
		mdxaBone_t	boltMatrix;
		vec3_t		handle, bottom, angles;
		trace_t		trace;
		int			numEnts;
		const float	radius = 300.0f;
		const float	halfRad = (radius/2);
		float		dist;
		int			i;
		vec3_t		mins, maxs, entDir;
		int			radiusEnts[MAX_GENTITIES];
		gentity_t	*radiusEnt;

		VectorSet(angles, 0, NPC->r.currentAngles[YAW], 0);

		trap_G2API_GetBoltMatrix( NPC->ghoul2, 2, 
					boltIndex,
					&boltMatrix, angles, NPC->r.currentOrigin, level.time,
					NULL, NPC->modelScale );
		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, handle );
		VectorCopy( handle, bottom );
		bottom[2] -= 128.0f;

		trap_Trace( &trace, handle, vec3_origin, vec3_origin, bottom, NPC->s.number, MASK_SHOT );
		G_PlayEffect( G_EffectIndex( "scepter/slam.efx" ), trace.endpos, trace.plane.normal );
		
		//Setup the bbox to search in
		for ( i = 0; i < 3; i++ )
		{
			mins[i] = trace.endpos[i] - radius;
			maxs[i] = trace.endpos[i] + radius;
		}

		//Get the number of entities in a given space
		numEnts = trap_EntitiesInBox( mins, maxs, radiusEnts, 128 );

		for ( i = 0; i < numEnts; i++ )
		{
			radiusEnt = &g_entities[radiusEnts[i]];
			if ( !radiusEnt->inuse )
			{
				continue;
			}
			
			if ( (radiusEnt->flags&FL_NO_KNOCKBACK) )
			{//don't throw them back
				continue;
			}

			if ( radiusEnt == NPC )
			{//Skip myself
				continue;
			}
			
			if ( radiusEnt->client == NULL )
			{//must be a client
				//RAFIXME: impliment
				if ( G_EntIsBreakable( radiusEnt->s.number) )
				//if ( G_EntIsBreakable( radiusEnt->s.number, NPC ) )
				{//damage breakables within range, but not as much
					G_Damage( radiusEnt, NPC, NPC, vec3_origin, radiusEnt->r.currentOrigin, 100, 0, MOD_ROCKET_SPLASH/*RAFIXME - reimpliment MOD_EXPLOSIVE_SPLASH*/ );
				}
				continue;
			}

			if(radiusEnt->client->ps.eFlags2&EF2_HELD_BY_MONSTER)	
			/*
			if ( (radiusEnts[i]->client->ps.eFlags&EF_HELD_BY_RANCOR) 
				|| (radiusEnts[i]->client->ps.eFlags&EF_HELD_BY_WAMPA) )
			*/
			{//can't be one being held
				continue;
			}
			
			VectorSubtract( radiusEnt->r.currentOrigin, trace.endpos, entDir );
			dist = VectorNormalize( entDir );
			if ( dist <= radius )
			{
				if ( dist < halfRad )
				{//close enough to do damage, too
					G_Damage( radiusEnt, NPC, NPC, vec3_origin, radiusEnt->r.currentOrigin, Q_irand( 20, 30 ), DAMAGE_NO_KNOCKBACK, MOD_ROCKET_SPLASH/* RAFIXME - reimpliment MOD_EXPLOSIVE_SPLASH*/ );
				}
				if ( radiusEnt->client
					&& radiusEnt->client->NPC_class != CLASS_RANCOR
					&& radiusEnt->client->NPC_class != CLASS_ATST )
				{
					float throwStr = 0.0f; 
					if ( g_spskill.integer > 1 )
					{
						throwStr = 10.0f+((radius-dist)/2.0f);
						if ( throwStr > 150.0f )
						{
							throwStr = 150.0f;
						}
					}
					else
					{
						throwStr = 10.0f+((radius-dist)/4.0f);
						if ( throwStr > 85.0f )
						{
							throwStr = 85.0f;
						}
					}
					entDir[2] += 0.1f;
					VectorNormalize( entDir );
					G_Throw( radiusEnt, entDir, throwStr );
					if ( radiusEnt->health > 0 )
					{
						if ( dist < halfRad
							|| radiusEnt->client->ps.groundEntityNum != ENTITYNUM_NONE )
						{//within range of my fist or within ground-shaking range and not in the air
							//RAFIXME: impliment
							G_Knockdown( radiusEnt );
							//G_Knockdown( radiusEnt, NPC, vec3_origin, 500, qtrue );
						}
					}
				}
			}
		}
	}
}


void Tavion_StartScepterBeam( void )
{//Activate the scepter beam for the current NPC.
	//RAFIXME:  This probably needs to be moved to client side.
	mdxaBone_t	boltMatrix;
	vec3_t		dir, base, angles;
	
	VectorSet(angles, 0, NPC->r.currentAngles[YAW], 0);

	trap_G2API_GetBoltMatrix( NPC->client->weaponGhoul2[1], 0, 
				NPC->NPC->genericBolt1,
				&boltMatrix, angles, NPC->r.currentOrigin, level.time,
				NULL, NPC->modelScale );
	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, base );
	BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_X, dir );
	G_PlayEffect( G_EffectIndex( "scepter/beam_warmup.efx" ), base, dir );
	//G_PlayEffect( G_EffectIndex( "scepter/beam_warmup.efx" ), NPC->client->weaponGhoul2[1], NPC->NPC->genericBolt1, NPC->s.number, NPC->r.currentOrigin, 0, qtrue );
	G_SoundOnEnt( NPC, CHAN_ITEM, "sound/weapons/scepter/beam_warmup.wav" );
	NPC->client->ps.legsTimer = NPC->client->ps.torsoTimer = 0;
	NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_SCEPTER_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	NPC->client->ps.torsoTimer += 200;
	NPC->painDebounceTime = level.time + NPC->client->ps.torsoTimer;
	NPC->client->ps.pm_time = NPC->client->ps.torsoTimer;
	NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	VectorClear( NPC->client->ps.velocity );
	VectorClear( NPC->client->ps.moveDir );
}


void Tavion_StartScepterSlam( void )
{//Activate the scepter slam for the current NPC.
	//RAFIXME:  This probably needs to be moved to client side.
	mdxaBone_t	boltMatrix;
	vec3_t		dir, base, angles;

	VectorSet(angles, 0, NPC->r.currentAngles[YAW], 0);

	trap_G2API_GetBoltMatrix( NPC->client->weaponGhoul2[1], 0, 
				NPC->NPC->genericBolt1,
				&boltMatrix, angles, NPC->r.currentOrigin, level.time,
				NULL, NPC->modelScale );
	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, base );
	BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_X, dir );
	G_PlayEffect( G_EffectIndex( "scepter/slam_warmup.efx" ), base, dir );
	//G_PlayEffect( G_EffectIndex( "scepter/slam_warmup.efx" ), NPC->weaponModel[1], NPC->genericBolt1, NPC->s.number, NPC->currentOrigin, 0, qtrue );
	G_SoundOnEnt( NPC, CHAN_ITEM, "sound/weapons/scepter/slam_warmup.wav" );
	NPC->client->ps.legsTimer = NPC->client->ps.torsoTimer = 0;
	NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_TAVION_SCEPTERGROUND, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	NPC->painDebounceTime = level.time + NPC->client->ps.torsoTimer;
	NPC->client->ps.pm_time = NPC->client->ps.torsoTimer;
	NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	VectorClear( NPC->client->ps.velocity );
	VectorClear( NPC->client->ps.moveDir );
	NPC->count = 0;
}

void Tavion_SithSwordRecharge( void )
{
	if ( NPC->client->ps.torsoAnim != BOTH_TAVION_SWORDPOWER 
		&& NPC->count 
		&& TIMER_Done( NPC, "rechargeDebounce" ) 
		&& NPC->client->weaponGhoul2[0] )
	{
		int			boltIndex;
		mdxaBone_t	boltMatrix;
		vec3_t		dir, base, angles;

		VectorSet(angles, 0, NPC->r.currentAngles[YAW], 0);

		NPC->s.loopSound = G_SoundIndex( "sound/weapons/scepter/recharge.wav" );
		boltIndex = trap_G2API_AddBolt(NPC->client->weaponGhoul2[0], 0, "*weapon");
		NPC->client->ps.legsTimer = NPC->client->ps.torsoTimer = 0;
		NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_TAVION_SWORDPOWER, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		
		//RAFIXME:  This probably needs to be moved to client side.
		trap_G2API_GetBoltMatrix( NPC->client->weaponGhoul2[0], 0, 
				boltIndex,
				&boltMatrix, angles, NPC->r.currentOrigin, level.time,
				NULL, NPC->modelScale );
		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, base );
		BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_X, dir );
		G_PlayEffect( G_EffectIndex( "scepter/recharge.efx" ), base, dir );
		//G_PlayEffect( G_EffectIndex( "scepter/recharge.efx" ), NPC->weaponModel[0], boltIndex, NPC->s.number, NPC->currentOrigin, NPC->client->ps.torsoAnimTimer, qtrue );
		NPC->painDebounceTime = level.time + NPC->client->ps.torsoTimer;
		NPC->client->ps.pm_time = NPC->client->ps.torsoTimer;
		NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		VectorClear( NPC->client->ps.velocity );
		VectorClear( NPC->client->ps.moveDir );
		//REFIXME:  impliment this.
		//NPC->client->ps.powerups[PW_INVINCIBLE] = level.time + NPC->client->ps.torsoTimer + 10000;
		//RAFIXME:  This probably needs to be moved to client side.
		G_PlayEffect( G_EffectIndex( "scepter/invincibility.efx" ), NPC->r.currentOrigin, NPC->r.currentAngles );
		//G_PlayEffect( G_EffectIndex( "scepter/invincibility.efx" ), NPC->playerModel, 0, NPC->s.number, NPC->currentOrigin, NPC->client->ps.torsoAnimTimer + 10000, qfalse );
		TIMER_Set( NPC, "rechargeDebounce", NPC->client->ps.torsoTimer + 10000 + Q_irand(10000,20000) );
		NPC->count--;
		//now you have a chance of killing her
		NPC->flags &= ~FL_UNDYING;
	}
}

//======================================================================================
//END TAVION BOSS
//======================================================================================


void Jedi_Cloak( gentity_t *self )
{//cloak this entity
	if ( self && self->client )
	{
		if ( !self->client->ps.powerups[PW_CLOAKED] )
		{//cloak
			self->client->ps.powerups[PW_CLOAKED] = Q3_INFINITE;
			//that's this?
			//self->client->ps.powerups[PW_UNCLOAKING] = level.time + 2000;
			//FIXME: debounce attacks?
			//FIXME: temp sound
			G_SoundOnEnt( self, CHAN_ITEM, "sound/chars/shadowtrooper/cloak.wav" );
		}
	}
}

void Jedi_Decloak( gentity_t *self )
{//decloak this entity
	if ( self && self->client )
	{
		if ( self->client->ps.powerups[PW_CLOAKED] )
		{//Uncloak
			self->client->ps.powerups[PW_CLOAKED] = 0;
			//self->client->ps.powerups[PW_UNCLOAKING] = level.time + 2000;
			//FIXME: temp sound
			G_SoundOnEnt( self, CHAN_ITEM, "sound/chars/shadowtrooper/decloak.wav" );
		}
	}
}


void Jedi_CheckCloak( void )
{//Check for cloak/decloak conditions for this NPC
	if ( NPC 
		&& NPC->client 
		&& NPC->client->NPC_class == CLASS_SHADOWTROOPER
		&& Q_stricmpn("shadowtrooper", NPC->NPC_type, 13 ) == 0 )
	{
		if ( NPC->client->ps.saberHolstered != 2 ||
			NPC->health <= 0 || 
			NPC->client->ps.saberInFlight ||
			(NPC->client->ps.fd.forceGripBeingGripped > level.time) ||
			//(NPC->client->ps.eFlags&EF_FORCE_DRAINED) || //I think that the above covers
			//this.
			NPC->painDebounceTime > level.time )
		{//can't be cloaked if saber is on, or dead or saber in flight or taking pain or being gripped
			Jedi_Decloak( NPC );
		}
		else if ( NPC->health > 0 
			&& !NPC->client->ps.saberInFlight 
			&& !(NPC->client->ps.fd.forceGripBeingGripped > level.time) 
			//&& !(NPC->client->ps.eFlags&EF_FORCE_DRAINED) //I think that the above covers
			//this.
			&& NPC->painDebounceTime < level.time )
		{//still alive, have saber in hand, not taking pain and not being gripped
			Jedi_Cloak( NPC );
		}
	}
}
//[/CoOp]

void Boba_Precache( void )
{
	G_SoundIndex( "sound/boba/jeton.wav" );
	G_SoundIndex( "sound/boba/jethover.wav" );
	G_SoundIndex( "sound/effects/combustfire.mp3" );
	G_EffectIndex( "boba/jet" );
	G_EffectIndex( "boba/fthrw" );
}

extern void G_CreateG2AttachedWeaponModel( gentity_t *ent, const char *weaponModel, int boltNum, int weaponNum );
extern void ChangeWeapon( gentity_t *ent, int newWeapon );
void Boba_ChangeWeapon( int wp )
{
	if ( NPC->s.weapon == wp )
	{
		return;
	}
	NPC_ChangeWeapon( wp );
	G_AddEvent( NPC, EV_GENERAL_SOUND, G_SoundIndex( "sound/weapons/change.wav" ));
}

void WP_ResistForcePush( gentity_t *self, gentity_t *pusher, qboolean noPenalty )
{
	int parts;
	qboolean runningResist = qfalse;

	if ( !self || self->health <= 0 || !self->client || !pusher || !pusher->client ) 
	{
		return;
	}
	if ( (!self->s.number || self->client->NPC_class == CLASS_DESANN || !Q_stricmp("Yoda",self->NPC_type) || self->client->NPC_class == CLASS_LUKE)
		&& (VectorLengthSquared( self->client->ps.velocity ) > 10000 || self->client->ps.fd.forcePowerLevel[FP_PUSH] >= FORCE_LEVEL_3 || self->client->ps.fd.forcePowerLevel[FP_PULL] >= FORCE_LEVEL_3 ) )
	{
		runningResist = qtrue;
	}
	if ( !runningResist
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE 
		&& !BG_SpinningSaberAnim( self->client->ps.legsAnim ) 
		&& !BG_FlippingAnim( self->client->ps.legsAnim ) 
		&& !PM_RollingAnim( self->client->ps.legsAnim ) 
		&& !PM_InKnockDown( &self->client->ps ) 
		&& !BG_CrouchAnim( self->client->ps.legsAnim ))
	{//if on a surface and not in a spin or flip, play full body resist
		parts = SETANIM_BOTH;
	}
	else
	{//play resist just in torso
		parts = SETANIM_TORSO;
	}
	NPC_SetAnim( self, parts, BOTH_RESISTPUSH, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	if ( !noPenalty )
	{
		char buf[128];
		float tFVal = 0;

		trap_Cvar_VariableStringBuffer("timescale", buf, sizeof(buf));

		tFVal = atof(buf);

		if ( !runningResist )
		{
			VectorClear( self->client->ps.velocity );
			//still stop them from attacking or moving for a bit, though
			//FIXME: maybe push just a little (like, slide)?
			self->client->ps.weaponTime = 1000;
			if ( self->client->ps.fd.forcePowersActive&(1<<FP_SPEED) )
			{
				self->client->ps.weaponTime = floor( self->client->ps.weaponTime * tFVal );
			}
			self->client->ps.pm_time = self->client->ps.weaponTime;
			self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			//play the full body push effect on me
			//self->forcePushTime = level.time + 600; // let the push effect last for 600 ms
			//rwwFIXMEFIXME: Do this?
		}
		else
		{
			self->client->ps.weaponTime = 600;
			if ( self->client->ps.fd.forcePowersActive&(1<<FP_SPEED) )
			{
				self->client->ps.weaponTime = floor( self->client->ps.weaponTime * tFVal );
			}
		}
	}
	//play my force push effect on my hand
	self->client->ps.powerups[PW_DISINT_4] = level.time + self->client->ps.torsoTimer + 500;
	self->client->ps.powerups[PW_PULL] = 0;
	Jedi_PlayBlockedPushSound( self );
}

qboolean Boba_StopKnockdown( gentity_t *self, gentity_t *pusher, vec3_t pushDir, qboolean forceKnockdown ) //forceKnockdown = qfalse
{
	vec3_t	pDir, fwd, right, ang;
	float	fDot, rDot;
	int		strafeTime;

	if ( self->client->NPC_class != CLASS_BOBAFETT )
	{
		return qfalse;
	}

	if ( (self->client->ps.eFlags2&EF2_FLYING) )//moveType == MT_FLYSWIM )
	{//can't knock me down when I'm flying
		return qtrue;
	}

	VectorSet(ang, 0, self->r.currentAngles[YAW], 0);
	strafeTime = Q_irand( 1000, 2000 );

	AngleVectors( ang, fwd, right, NULL );
	VectorNormalize2( pushDir, pDir );
	fDot = DotProduct( pDir, fwd );
	rDot = DotProduct( pDir, right );

	if ( Q_irand( 0, 2 ) )
	{//flip or roll with it
		usercmd_t	tempCmd;
		if ( fDot >= 0.4f )
		{
			tempCmd.forwardmove = 127;
			TIMER_Set( self, "moveforward", strafeTime );
		}
		else if ( fDot <= -0.4f )
		{
			tempCmd.forwardmove = -127;
			TIMER_Set( self, "moveback", strafeTime );
		}
		else if ( rDot > 0 )
		{
			tempCmd.rightmove = 127;
			TIMER_Set( self, "strafeRight", strafeTime );
			TIMER_Set( self, "strafeLeft", -1 );
		}
		else
		{
			tempCmd.rightmove = -127;
			TIMER_Set( self, "strafeLeft", strafeTime );
			TIMER_Set( self, "strafeRight", -1 );
		}
		G_AddEvent( self, EV_JUMP, 0 );
		if ( !Q_irand( 0, 1 ) )
		{//flip
			self->client->ps.fd.forceJumpCharge = 280;//FIXME: calc this intelligently?
			ForceJump( self, &tempCmd );
		}
		else
		{//roll
			TIMER_Set( self, "duck", strafeTime );
		}
		self->painDebounceTime = 0;//so we do something
	}
	else if ( !Q_irand( 0, 1 ) && forceKnockdown )
	{//resist
		WP_ResistForcePush( self, pusher, qtrue );
	}
	else
	{//fall down
		return qfalse;
	}

	return qtrue;
}

void Boba_FlyStart( gentity_t *self )
{//switch to seeker AI for a while
	if ( TIMER_Done( self, "jetRecharge" ) )
	{
		self->client->ps.gravity = 0;
		if ( self->NPC )
		{
			self->NPC->aiFlags |= NPCAI_CUSTOM_GRAVITY;
		}
		self->client->ps.eFlags2 |= EF2_FLYING;//moveType = MT_FLYSWIM;
		self->client->jetPackTime = level.time + Q_irand( 3000, 10000 );
		//take-off sound
		G_SoundOnEnt( self, CHAN_ITEM, "sound/boba/jeton.wav" );
		//jet loop sound
		self->s.loopSound = G_SoundIndex( "sound/boba/jethover.wav" );
		if ( self->NPC )
		{
			self->count = Q3_INFINITE; // SEEKER shot ammo count
		}
	}
}

void Boba_FlyStop( gentity_t *self )
{
	self->client->ps.gravity = g_gravity.value;
	if ( self->NPC )
	{
		self->NPC->aiFlags &= ~NPCAI_CUSTOM_GRAVITY;
	}
	self->client->ps.eFlags2 &= ~EF2_FLYING;
	self->client->jetPackTime = 0;
	//stop jet loop sound
	self->s.loopSound = 0;
	if ( self->NPC )
	{
		self->count = 0; // SEEKER shot ammo count
		TIMER_Set( self, "jetRecharge", Q_irand( 1000, 5000 ) );
		TIMER_Set( self, "jumpChaseDebounce", Q_irand( 500, 2000 ) );
	}
}

qboolean Boba_Flying( gentity_t *self )
{
	return ((qboolean)(self->client->ps.eFlags2&EF2_FLYING));//moveType==MT_FLYSWIM));
}

void Boba_FireFlameThrower( gentity_t *self )
{
	int		damage	= Q_irand( 20, 30 );
	trace_t		tr;
	gentity_t	*traceEnt = NULL;
	mdxaBone_t	boltMatrix;
	vec3_t		start, end, dir, traceMins = {-4, -4, -4}, traceMaxs = {4, 4, 4};

	trap_G2API_GetBoltMatrix( self->ghoul2, 0, self->client->renderInfo.handLBolt,
			&boltMatrix, self->r.currentAngles, self->r.currentOrigin, level.time,
			NULL, self->modelScale );

	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, start );
	BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, dir );
	//G_PlayEffect( "boba/fthrw", start, dir );
	VectorMA( start, 128, dir, end );

	trap_Trace( &tr, start, traceMins, traceMaxs, end, self->s.number, MASK_SHOT );

	traceEnt = &g_entities[tr.entityNum];
	if ( tr.entityNum < ENTITYNUM_WORLD && traceEnt->takedamage )
	{
		G_Damage( traceEnt, self, self, dir, tr.endpos, damage, DAMAGE_NO_ARMOR|DAMAGE_NO_KNOCKBACK|/*DAMAGE_NO_HIT_LOC|*/DAMAGE_IGNORE_TEAM, MOD_LAVA );
		//rwwFIXMEFIXME: add DAMAGE_NO_HIT_LOC?
	}
}

//extern void SP_fx_explosion_trail( gentity_t *ent );
void Boba_StartFlameThrower( gentity_t *self )
{
	int	flameTime = 4000;//Q_irand( 1000, 3000 );
	mdxaBone_t	boltMatrix;
	vec3_t		org, dir;

	self->client->ps.torsoTimer = flameTime;//+1000;
	if ( self->NPC )
	{
		TIMER_Set( self, "nextAttackDelay", flameTime );
		TIMER_Set( self, "walking", 0 );
	}
	TIMER_Set( self, "flameTime", flameTime );
	/*
	gentity_t *fire = G_Spawn();
	if ( fire != NULL )
	{
		mdxaBone_t	boltMatrix;
		vec3_t		org, dir, ang;
		gi.G2API_GetBoltMatrix( NPC->ghoul2, NPC->playerModel, NPC->handRBolt,
				&boltMatrix, NPC->r.currentAngles, NPC->r.currentOrigin, (cg.time?cg.time:level.time),
				NULL, NPC->s.modelScale );

		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, org );
		gi.G2API_GiveMeVectorFromMatrix( boltMatrix, NEGATIVE_Y, dir );
		vectoangles( dir, ang );

		VectorCopy( org, fire->s.origin );
		VectorCopy( ang, fire->s.angles );

		fire->targetname = "bobafire";
		SP_fx_explosion_trail( fire );
		fire->damage = 1;
		fire->radius = 10;
		fire->speed = 200;
		fire->fxID = G_EffectIndex( "boba/fthrw" );//"env/exp_fire_trail" );//"env/small_fire"
		fx_explosion_trail_link( fire );
		fx_explosion_trail_use( fire, NPC, NPC );

	}
	*/
	G_SoundOnEnt( self, CHAN_WEAPON, "sound/effects/combustfire.mp3" );

	trap_G2API_GetBoltMatrix(NPC->ghoul2, 0, NPC->client->renderInfo.handRBolt, &boltMatrix, NPC->r.currentAngles,
		NPC->r.currentOrigin, level.time, NULL, NPC->modelScale);

	BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, org );
	BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, dir );

	G_PlayEffectID( G_EffectIndex("boba/fthrw"), org, dir);
}

void Boba_DoFlameThrower( gentity_t *self )
{
	NPC_SetAnim( self, SETANIM_TORSO, BOTH_FORCELIGHTNING_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	if ( TIMER_Done( self, "nextAttackDelay" ) && TIMER_Done( self, "flameTime" ) )
	{
		Boba_StartFlameThrower( self );
	}
	Boba_FireFlameThrower( self );
}

//[CoOp]
//[SuperDindon]
//find the distance to the ground
float G_GroundDistance( gentity_t *self )
{
	trace_t tr;
	vec3_t down;

	if (!self)
		return 0.0f;

	if (!self->client)
		return 0.0f;

	VectorCopy(self->client->ps.origin, down);

	down[2] -= 4096;

	trap_Trace(&tr, self->client->ps.origin, self->r.mins, self->r.maxs, down, self->s.clientNum, MASK_SOLID);

	VectorSubtract(self->client->ps.origin, tr.endpos, down);

	if (tr.entityNum != ENTITYNUM_NONE)
		if (g_entities[tr.entityNum].classname)
			if (g_entities[tr.entityNum].classname[0])
				if (!Q_stricmp(g_entities[tr.entityNum].classname,"trigger_hurt"))
					return 65536;

	return VectorLength(down);
}
//[/CoOp]

void Boba_FireDecide( void )
{
	qboolean enemyLOS = qfalse;
	qboolean enemyCS = qfalse;
	qboolean enemyInFOV = qfalse;
	//qboolean move = qtrue;
	qboolean faceEnemy = qfalse;
	qboolean shoot = qfalse;
	qboolean hitAlly = qfalse;
	vec3_t	impactPos;
	float	enemyDist;
	float	dot;
	vec3_t	enemyDir, shootDir;

	if ( NPC->client->ps.groundEntityNum == ENTITYNUM_NONE 
		&& NPC->client->ps.fd.forceJumpZStart
		&& !BG_FlippingAnim( NPC->client->ps.legsAnim )
		&& !Q_irand( 0, 10 ) )
	{//take off
		Boba_FlyStart( NPC );
	}

	if ( !NPC->enemy )
	{
		return;
	}

	/*
	if ( NPC->enemy->enemy != NPC && NPC->health == NPC->client->pers.maxHealth )
	{
		NPCInfo->scriptFlags |= SCF_ALT_FIRE;
		Boba_ChangeWeapon( WP_DISRUPTOR );
	}
	else */if ( NPC->enemy->s.weapon == WP_SABER )
	{
		NPCInfo->scriptFlags &= ~SCF_ALT_FIRE;
		Boba_ChangeWeapon( WP_ROCKET_LAUNCHER );
	}
	else
	{
		if ( NPC->health < NPC->client->pers.maxHealth*0.5f )
		{
			NPCInfo->scriptFlags |= SCF_ALT_FIRE;
			Boba_ChangeWeapon( WP_BLASTER );
			NPCInfo->burstMin = 3;
			NPCInfo->burstMean = 12;
			NPCInfo->burstMax = 20;
			NPCInfo->burstSpacing = Q_irand( 300, 750 );//attack debounce
		}
		else
		{
			NPCInfo->scriptFlags &= ~SCF_ALT_FIRE;
			Boba_ChangeWeapon( WP_BLASTER );
		}
	}

	VectorClear( impactPos );
	enemyDist = DistanceSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );

	VectorSubtract( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, enemyDir );
	VectorNormalize( enemyDir );
	AngleVectors( NPC->client->ps.viewangles, shootDir, NULL, NULL );
	dot = DotProduct( enemyDir, shootDir );
	if ( dot > 0.5f ||( enemyDist * (1.0f-dot)) < 10000 )
	{//enemy is in front of me or they're very close and not behind me
		enemyInFOV = qtrue;
	}

	if ( (enemyDist < (128*128)&&enemyInFOV) || !TIMER_Done( NPC, "flameTime" ) )
	{//flamethrower
		Boba_DoFlameThrower( NPC );
		enemyCS = qfalse;
		shoot = qfalse;
		NPCInfo->enemyLastSeenTime = level.time;
		faceEnemy = qtrue;
		ucmd.buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK);
	}
	else if ( enemyDist < MIN_ROCKET_DIST_SQUARED )//128
	{//enemy within 128
		if ( (NPC->client->ps.weapon == WP_FLECHETTE || NPC->client->ps.weapon == WP_REPEATER) && 
			(NPCInfo->scriptFlags & SCF_ALT_FIRE) )
		{//shooting an explosive, but enemy too close, switch to primary fire
			NPCInfo->scriptFlags &= ~SCF_ALT_FIRE;
			//FIXME: we can never go back to alt-fire this way since, after this, we don't know if we were initially supposed to use alt-fire or not...
		}
	}
	else if ( enemyDist > 65536 )//256 squared
	{
		if ( NPC->client->ps.weapon == WP_DISRUPTOR )
		{//sniping... should be assumed
			if ( !(NPCInfo->scriptFlags&SCF_ALT_FIRE) )
			{//use primary fire
				NPCInfo->scriptFlags |= SCF_ALT_FIRE;
				//reset fire-timing variables
				NPC_ChangeWeapon( WP_DISRUPTOR );
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
		}
	}

	//can we see our target?
	if ( TIMER_Done( NPC, "nextAttackDelay" ) && TIMER_Done( NPC, "flameTime" ) )
	{
		if ( NPC_ClearLOS4( NPC->enemy ) )
		{
			NPCInfo->enemyLastSeenTime = level.time;
			enemyLOS = qtrue;

			if ( NPC->client->ps.weapon == WP_NONE )
			{
				enemyCS = qfalse;//not true, but should stop us from firing
			}
			else
			{//can we shoot our target?
				if ( (NPC->client->ps.weapon == WP_ROCKET_LAUNCHER || (NPC->client->ps.weapon == WP_FLECHETTE && (NPCInfo->scriptFlags&SCF_ALT_FIRE))) && enemyDist < MIN_ROCKET_DIST_SQUARED )//128*128
				{
					enemyCS = qfalse;//not true, but should stop us from firing
					hitAlly = qtrue;//us!
					//FIXME: if too close, run away!
				}
				else if ( enemyInFOV )
				{//if enemy is FOV, go ahead and check for shooting
					int hit = NPC_ShotEntity( NPC->enemy, impactPos );
					gentity_t *hitEnt = &g_entities[hit];

					if ( hit == NPC->enemy->s.number 
						|| ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPC->client->enemyTeam )
						|| ( hitEnt && hitEnt->takedamage && ((hitEnt->r.svFlags&SVF_GLASS_BRUSH)||hitEnt->health < 40||NPC->s.weapon == WP_EMPLACED_GUN) ) )
					{//can hit enemy or enemy ally or will hit glass or other minor breakable (or in emplaced gun), so shoot anyway
						enemyCS = qtrue;
						//NPC_AimAdjust( 2 );//adjust aim better longer we have clear shot at enemy
						VectorCopy( NPC->enemy->r.currentOrigin, NPCInfo->enemyLastSeenLocation );
					}
					else
					{//Hmm, have to get around this bastard
						//NPC_AimAdjust( 1 );//adjust aim better longer we can see enemy
						if ( hitEnt && hitEnt->client && hitEnt->client->playerTeam == NPC->client->playerTeam )
						{//would hit an ally, don't fire!!!
							hitAlly = qtrue;
						}
						else
						{//Check and see where our shot *would* hit... if it's not close to the enemy (within 256?), then don't fire
						}
					}
				}
				else
				{
					enemyCS = qfalse;//not true, but should stop us from firing
				}
			}
		}
		else if ( trap_InPVS( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin ) )
		{
			NPCInfo->enemyLastSeenTime = level.time;
			faceEnemy = qtrue;
			//NPC_AimAdjust( -1 );//adjust aim worse longer we cannot see enemy
		}

		if ( NPC->client->ps.weapon == WP_NONE )
		{
			faceEnemy = qfalse;
			shoot = qfalse;
		}
		else
		{
			if ( enemyLOS )
			{//FIXME: no need to face enemy if we're moving to some other goal and he's too far away to shoot?
				faceEnemy = qtrue;
			}
			if ( enemyCS )
			{
				shoot = qtrue;
			}
		}

		if ( !enemyCS )
		{//if have a clear shot, always try
			//See if we should continue to fire on their last position
			//!TIMER_Done( NPC, "stick" ) || 
			if ( !hitAlly //we're not going to hit an ally
				&& enemyInFOV //enemy is in our FOV //FIXME: or we don't have a clear LOS?
				&& NPCInfo->enemyLastSeenTime > 0 )//we've seen the enemy
			{
				if ( level.time - NPCInfo->enemyLastSeenTime < 10000 )//we have seem the enemy in the last 10 seconds
				{
					if ( !Q_irand( 0, 10 ) )
					{
						//Fire on the last known position
						vec3_t	muzzle, dir, angles;
						qboolean tooClose = qfalse;
						qboolean tooFar = qfalse;
						float distThreshold;
						float dist;

						CalcEntitySpot( NPC, SPOT_HEAD, muzzle );
						if ( VectorCompare( impactPos, vec3_origin ) )
						{//never checked ShotEntity this frame, so must do a trace...
							trace_t tr;
							//vec3_t	mins = {-2,-2,-2}, maxs = {2,2,2};
							vec3_t	forward, end;
							AngleVectors( NPC->client->ps.viewangles, forward, NULL, NULL );
							VectorMA( muzzle, 8192, forward, end );
							trap_Trace( &tr, muzzle, vec3_origin, vec3_origin, end, NPC->s.number, MASK_SHOT );
							VectorCopy( tr.endpos, impactPos );
						}

						//see if impact would be too close to me
						distThreshold = 16384/*128*128*/;//default
						switch ( NPC->s.weapon )
						{
						case WP_ROCKET_LAUNCHER:
						case WP_FLECHETTE:
						case WP_THERMAL:
						case WP_TRIP_MINE:
						case WP_DET_PACK:
							distThreshold = 65536/*256*256*/;
							break;
						case WP_REPEATER:
							if ( NPCInfo->scriptFlags&SCF_ALT_FIRE )
							{
								distThreshold = 65536/*256*256*/;
							}
							break;
						default:
							break;
						}

						dist = DistanceSquared( impactPos, muzzle );

						if ( dist < distThreshold )
						{//impact would be too close to me
							tooClose = qtrue;
						}
						else if ( level.time - NPCInfo->enemyLastSeenTime > 5000 ||
							(NPCInfo->group && level.time - NPCInfo->group->lastSeenEnemyTime > 5000 ))
						{//we've haven't seen them in the last 5 seconds
							//see if it's too far from where he is
							distThreshold = 65536/*256*256*/;//default
							switch ( NPC->s.weapon )
							{
							case WP_ROCKET_LAUNCHER:
							case WP_FLECHETTE:
							case WP_THERMAL:
							case WP_TRIP_MINE:
							case WP_DET_PACK:
								distThreshold = 262144/*512*512*/;
								break;
							case WP_REPEATER:
								if ( NPCInfo->scriptFlags&SCF_ALT_FIRE )
								{
									distThreshold = 262144/*512*512*/;
								}
								break;
							default:
								break;
							}
							dist = DistanceSquared( impactPos, NPCInfo->enemyLastSeenLocation );
							if ( dist > distThreshold )
							{//impact would be too far from enemy
								tooFar = qtrue;
							}
						}

						if ( !tooClose && !tooFar )
						{//okay too shoot at last pos
							VectorSubtract( NPCInfo->enemyLastSeenLocation, muzzle, dir );
							VectorNormalize( dir );
							vectoangles( dir, angles );

							NPCInfo->desiredYaw		= angles[YAW];
							NPCInfo->desiredPitch	= angles[PITCH];

							shoot = qtrue;
							faceEnemy = qfalse;
						}
					}
				}
			}
		}

		//FIXME: don't shoot right away!
		if ( NPC->client->ps.weaponTime > 0 )
		{
			if ( NPC->s.weapon == WP_ROCKET_LAUNCHER )
			{
				if ( !enemyLOS || !enemyCS )
				{//cancel it
					NPC->client->ps.weaponTime = 0;
				}
				else
				{//delay our next attempt
					TIMER_Set( NPC, "nextAttackDelay", Q_irand( 500, 1000 ) );
				}
			}
		}
		else if ( shoot )
		{//try to shoot if it's time
			if ( TIMER_Done( NPC, "nextAttackDelay" ) )
			{
				if( !(NPCInfo->scriptFlags & SCF_FIRE_WEAPON) ) // we've already fired, no need to do it again here
				{
					WeaponThink( qtrue );
				}
				//NASTY
				if ( NPC->s.weapon == WP_ROCKET_LAUNCHER 
					&& (ucmd.buttons&BUTTON_ATTACK) 
					&& !Q_irand( 0, 3 ) )
				{//every now and then, shoot a homing rocket
					ucmd.buttons &= ~BUTTON_ATTACK;
					ucmd.buttons |= BUTTON_ALT_ATTACK;
					NPC->client->ps.weaponTime = Q_irand( 500, 1500 );
				}
			}
		}
	}
}

//[CoOp]
/* replaced this stuff with SP version above.
void Jedi_Cloak( gentity_t *self )
{
	if ( self )
	{
		self->flags |= FL_NOTARGET;
		if ( self->client )
		{
			if ( !self->client->ps.powerups[PW_CLOAKED] )
			{//cloak
				self->client->ps.powerups[PW_CLOAKED] = Q3_INFINITE;

				//FIXME: debounce attacks?
				//FIXME: temp sound
				G_Sound( self, CHAN_ITEM, G_SoundIndex("sound/chars/shadowtrooper/cloak.wav") );
			}
		}
	}
}

void Jedi_Decloak( gentity_t *self )
{
	if ( self )
	{
		self->flags &= ~FL_NOTARGET;
		if ( self->client )
		{
			if ( self->client->ps.powerups[PW_CLOAKED] )
			{//Uncloak
				self->client->ps.powerups[PW_CLOAKED] = 0;

				G_Sound( self, CHAN_ITEM, G_SoundIndex("sound/chars/shadowtrooper/decloak.wav") );
			}
		}
	}
}


void Jedi_CheckCloak( void )
{
	if ( NPC && NPC->client && NPC->client->NPC_class == CLASS_SHADOWTROOPER )
	{
		if ( !NPC->client->ps.saberHolstered ||
			NPC->health <= 0 || 
			NPC->client->ps.saberInFlight ||
		//	(NPC->client->ps.eFlags&EF_FORCE_GRIPPED) ||
		//	(NPC->client->ps.eFlags&EF_FORCE_DRAINED) ||
			NPC->painDebounceTime > level.time )
		{//can't be cloaked if saber is on, or dead or saber in flight or taking pain or being gripped
			Jedi_Decloak( NPC );
		}
		else if ( NPC->health > 0 
			&& !NPC->client->ps.saberInFlight 
		//	&& !(NPC->client->ps.eFlags&EF_FORCE_GRIPPED) 
		//	&& !(NPC->client->ps.eFlags&EF_FORCE_DRAINED) 
			&& NPC->painDebounceTime < level.time )
		{//still alive, have saber in hand, not taking pain and not being gripped
			Jedi_Cloak( NPC );
		}
	}
}
*/
//[/CoOp]

/*
==========================================================================================
AGGRESSION
==========================================================================================
*/
static void Jedi_Aggression( gentity_t *self, int change )
{
	int	upper_threshold, lower_threshold;

	self->NPC->stats.aggression += change;
	
	//FIXME: base this on initial NPC stats
	if ( self->client->playerTeam == NPCTEAM_PLAYER )
	{//good guys are less aggressive
		upper_threshold = 7;
		lower_threshold = 1;
	}
	else
	{//bad guys are more aggressive
		if ( self->client->NPC_class == CLASS_DESANN )
		{
			upper_threshold = 20;
			lower_threshold = 5;
		}
		else
		{
			upper_threshold = 10;
			lower_threshold = 3;
		}
	}

	if ( self->NPC->stats.aggression > upper_threshold )
	{
		self->NPC->stats.aggression = upper_threshold;
	}
	else if ( self->NPC->stats.aggression < lower_threshold )
	{
		self->NPC->stats.aggression = lower_threshold;
	}
	//Com_Printf( "(%d) %s agg %d change: %d\n", level.time, self->NPC_type, self->NPC->stats.aggression, change );
}

static void Jedi_AggressionErosion( int amt )
{
	if ( TIMER_Done( NPC, "roamTime" ) )
	{//the longer we're not alerted and have no enemy, the more our aggression goes down
		TIMER_Set( NPC, "roamTime", Q_irand( 2000, 5000 ) );
		Jedi_Aggression( NPC, amt );
	}
	
	if ( NPCInfo->stats.aggression < 4 || (NPCInfo->stats.aggression < 6&&NPC->client->NPC_class == CLASS_DESANN))
	{//turn off the saber
		WP_DeactivateSaber( NPC, qfalse );
	}
}

void NPC_Jedi_RateNewEnemy( gentity_t *self, gentity_t *enemy )
{//racc - set the Jedi's aggression level for a new enemy
	float healthAggression;
	float weaponAggression;
	int newAggression;

	switch( enemy->s.weapon )
	{
	case WP_SABER:
		healthAggression = (float)self->health/200.0f*6.0f;
		weaponAggression = 7;//go after him
		break;
	case WP_BLASTER:
		if ( DistanceSquared( self->r.currentOrigin, enemy->r.currentOrigin ) < 65536 )//256 squared
		{
			healthAggression = (float)self->health/200.0f*8.0f;
			weaponAggression = 8;//go after him
		}
		else
		{
			healthAggression = 8.0f - ((float)self->health/200.0f*8.0f);
			weaponAggression = 2;//hang back for a second
		}
		break;
	default:
		healthAggression = (float)self->health/200.0f*8.0f;
		weaponAggression = 6;//approach
		break;
	}
	//Average these with current aggression
	newAggression = ceil( (healthAggression + weaponAggression + (float)self->NPC->stats.aggression )/3.0f);
	//Com_Printf( "(%d) new agg %d - new enemy\n", level.time, newAggression );
	Jedi_Aggression( self, newAggression - self->NPC->stats.aggression );

	//don't taunt right away
	TIMER_Set( self, "chatter", Q_irand( 4000, 7000 ) );
}

static void Jedi_Rage( void )
{//racc - activate Force Rage
	Jedi_Aggression( NPC, 10 - NPCInfo->stats.aggression + Q_irand( -2, 2 ) );
	TIMER_Set( NPC, "roamTime", 0 );
	TIMER_Set( NPC, "chatter", 0 );
	TIMER_Set( NPC, "walking", 0 );
	TIMER_Set( NPC, "taunting", 0 );
	TIMER_Set( NPC, "jumpChaseDebounce", 0 );
	TIMER_Set( NPC, "movenone", 0 );
	TIMER_Set( NPC, "movecenter", 0 );
	TIMER_Set( NPC, "noturn", 0 );
	ForceRage( NPC );
}

void Jedi_RageStop( gentity_t *self )
{//racc - deactivate force rage
	if ( self->NPC )
	{//calm down and back off
		TIMER_Set( self, "roamTime", 0 );
		Jedi_Aggression( self, Q_irand( -5, 0 ) );
	}
}
/*
==========================================================================================
SPEAKING
==========================================================================================
*/

static qboolean Jedi_BattleTaunt( void )
{//racc - check/do taunting
	if ( TIMER_Done( NPC, "chatter" ) 
		&& !Q_irand( 0, 3 ) 
		&& NPCInfo->blockedSpeechDebounceTime < level.time 
		&& jediSpeechDebounceTime[NPC->client->playerTeam] < level.time )
	{//racc - try to taunt
		int event = -1;
		//[CoOp]
		if ( NPC->enemy 
			&& NPC->enemy->client 
			&& (NPC->enemy->client->NPC_class == CLASS_RANCOR
				|| NPC->enemy->client->NPC_class == CLASS_WAMPA
				|| NPC->enemy->client->NPC_class == CLASS_SAND_CREATURE) )
		{//never taunt these mindless creatures
			//NOTE: howlers?  tusken?  etc?  Only reborn?
		}
		else
		{
			if ( NPC->client->playerTeam == NPCTEAM_PLAYER 
				&& NPC->enemy && NPC->enemy->client && NPC->enemy->client->NPC_class == CLASS_JEDI )
			{//a jedi fighting a jedi - training
				if ( NPC->client->NPC_class == CLASS_JEDI && NPCInfo->rank == RANK_COMMANDER )
				{//only trainer taunts
					event = EV_TAUNT1;
				}
			}
			else
			{//reborn or a jedi fighting an enemy
				event = Q_irand( EV_TAUNT1, EV_TAUNT3 );
			}

			if ( event != -1 )
			{
				G_AddVoiceEvent( NPC, event, 3000 );
				jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.time + 6000;
				//[CoOp]
				if ( (NPCInfo->aiFlags&NPCAI_ROSH) )
				{//Rosh taunts less often
					TIMER_Set( NPC, "chatter", Q_irand( 8000, 20000 ) );
				}
				else
				{
					TIMER_Set( NPC, "chatter", Q_irand( 5000, 10000 ) );
				}
				//TIMER_Set( NPC, "chatter", Q_irand( 5000, 10000 ) );
				//[/CoOp]

				if ( NPC->enemy && NPC->enemy->NPC && NPC->enemy->s.weapon == WP_SABER && NPC->enemy->client && NPC->enemy->client->NPC_class == CLASS_JEDI )
				{//Have the enemy jedi say something in response when I'm done?
				}
				return qtrue;
			}
		}
		//[/CoOp]
	}
	return qfalse;
}

/*
==========================================================================================
MOVEMENT
==========================================================================================
*/
static qboolean Jedi_ClearPathToSpot( vec3_t dest, int impactEntNum )
{//racc - checks for a clear path between the destination point and the Jedi NPC.
	//This includes a check for floor between the NPC and the target point
	trace_t	trace;
	vec3_t	mins, start, end, dir;
	float	dist, drop;
	float	i;

	//Offset the step height
	VectorSet( mins, NPC->r.mins[0], NPC->r.mins[1], NPC->r.mins[2] + STEPSIZE );
	
	trap_Trace( &trace, NPC->r.currentOrigin, mins, NPC->r.maxs, dest, NPC->s.number, NPC->clipmask );

	//Do a simple check
	if ( trace.allsolid || trace.startsolid )
	{//inside solid
		return qfalse;
	}

	if ( trace.fraction < 1.0f )
	{//hit something
		if ( impactEntNum != ENTITYNUM_NONE && trace.entityNum == impactEntNum )
		{//hit what we're going after
			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}

	//otherwise, clear path in a straight line.  
	//Now at intervals of my size, go along the trace and trace down STEPSIZE to make sure there is a solid floor.
	VectorSubtract( dest, NPC->r.currentOrigin, dir );
	dist = VectorNormalize( dir );
	if ( dest[2] > NPC->r.currentOrigin[2] )
	{//going up, check for steps
		drop = STEPSIZE;
	}
	else
	{//going down or level, check for moderate drops
		drop = 64;
	}
	for ( i = NPC->r.maxs[0]*2; i < dist; i += NPC->r.maxs[0]*2 )
	{//FIXME: does this check the last spot, too?  We're assuming that should be okay since the enemy is there?
		VectorMA( NPC->r.currentOrigin, i, dir, start );
		VectorCopy( start, end );
		end[2] -= drop;
		trap_Trace( &trace, start, mins, NPC->r.maxs, end, NPC->s.number, NPC->clipmask );//NPC->r.mins?
		if ( trace.fraction < 1.0f || trace.allsolid || trace.startsolid )
		{//good to go
			continue;
		}
		//no floor here! (or a long drop?)
		return qfalse;
	}
	//we made it!
	return qtrue;
}

qboolean NPC_MoveDirClear( int forwardmove, int rightmove, qboolean reset )
{//racc - Checks for immediate move obsticales, if reset is true, also change the ucmd to avoid.
	vec3_t	forward, right, testPos, angles, mins;
	trace_t	trace;
	float	fwdDist, rtDist;
	float	bottom_max = -STEPSIZE*4 - 1;

	if ( !forwardmove && !rightmove )
	{//not even moving
		//Com_Printf( "%d skipping walk-cliff check (not moving)\n", level.time );
		return qtrue;
	}

	if ( ucmd.upmove > 0 || NPC->client->ps.fd.forceJumpCharge )
	{//Going to jump
		//Com_Printf( "%d skipping walk-cliff check (going to jump)\n", level.time );
		return qtrue;
	}

	if ( NPC->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//in the air
		//Com_Printf( "%d skipping walk-cliff check (in air)\n", level.time );
		return qtrue;
	}
	/*
	if ( fabs( AngleDelta( NPC->r.currentAngles[YAW], NPCInfo->desiredYaw ) ) < 5.0 )//!ucmd.angles[YAW] )
	{//Not turning much, don't do this
		//NOTE: Should this not happen only if you're not turning AT ALL?
		//	You could be turning slowly but moving fast, so that would
		//	still let you walk right off a cliff...
		//NOTE: Or maybe it is a good idea to ALWAYS do this, regardless
		//	of whether ot not we're turning?  But why would we be walking
		//  straight into a wall or off	a cliff unless we really wanted to?
		return;
	}
	*/

	//FIXME: to really do this right, we'd have to actually do a pmove to predict where we're
	//going to be... maybe this should be a flag and pmove handles it and sets a flag so AI knows
	//NEXT frame?  Or just incorporate current velocity, runspeed and possibly friction?
	VectorCopy( NPC->r.mins, mins );
	mins[2] += STEPSIZE;
	angles[PITCH] = angles[ROLL] = 0;
	angles[YAW] = NPC->client->ps.viewangles[YAW];//Add ucmd.angles[YAW]?
	AngleVectors( angles, forward, right, NULL );
	fwdDist = ((float)forwardmove)/2.0f;
	rtDist = ((float)rightmove)/2.0f;
	VectorMA( NPC->r.currentOrigin, fwdDist, forward, testPos );
	VectorMA( testPos, rtDist, right, testPos );
	trap_Trace( &trace, NPC->r.currentOrigin, mins, NPC->r.maxs, testPos, NPC->s.number, NPC->clipmask|CONTENTS_BOTCLIP );
	if ( trace.allsolid || trace.startsolid )
	{//hmm, trace started inside this brush... how do we decide if we should continue?
		//FIXME: what do we do if we start INSIDE a CONTENTS_BOTCLIP? Try the trace again without that in the clipmask?
		if ( reset )
		{
			trace.fraction = 1.0f;
		}
		VectorCopy( testPos, trace.endpos );
		//return qtrue;
	}
	if ( trace.fraction < 0.6 )
	{//Going to bump into something very close, don't move, just turn
		if ( (NPC->enemy && trace.entityNum == NPC->enemy->s.number) || (NPCInfo->goalEntity && trace.entityNum == NPCInfo->goalEntity->s.number) )
		{//okay to bump into enemy or goal
			//Com_Printf( "%d bump into enemy/goal okay\n", level.time );
			return qtrue;
		}
		else if ( reset )
		{//actually want to screw with the ucmd
			//Com_Printf( "%d avoiding walk into wall (entnum %d)\n", level.time, trace.entityNum );
			ucmd.forwardmove = 0;
			ucmd.rightmove = 0;
			VectorClear( NPC->client->ps.moveDir );
		}
		return qfalse;
	}

	if ( NPCInfo->goalEntity )
	{
		if ( NPCInfo->goalEntity->r.currentOrigin[2] < NPC->r.currentOrigin[2] )
		{//goal is below me, okay to step off at least that far plus stepheight
			bottom_max += NPCInfo->goalEntity->r.currentOrigin[2] - NPC->r.currentOrigin[2];
		}
	}
	VectorCopy( trace.endpos, testPos );
	testPos[2] += bottom_max;

	trap_Trace( &trace, trace.endpos, mins, NPC->r.maxs, testPos, NPC->s.number, NPC->clipmask );

	//FIXME:Should we try to see if we can still get to our goal using the waypoint network from this trace.endpos?
	//OR: just put NPC clip brushes on these edges (still fall through when die)

	if ( trace.allsolid || trace.startsolid )
	{//Not going off a cliff
		//Com_Printf( "%d walk off cliff okay (droptrace in solid)\n", level.time );
		return qtrue;
	}

	if ( trace.fraction < 1.0 )
	{//Not going off a cliff
		//FIXME: what if plane.normal is sloped?  We'll slide off, not land... plus this doesn't account for slide-movement... 
		//Com_Printf( "%d walk off cliff okay will hit entnum %d at dropdist of %4.2f\n", level.time, trace.entityNum, (trace.fraction*bottom_max) );
		return qtrue;
	}

	//going to fall at least bottom_max, don't move, just turn... is this bad, though?  What if we want them to drop off?
	if ( reset )
	{//actually want to screw with the ucmd
		//Com_Printf( "%d avoiding walk off cliff\n", level.time );
		ucmd.forwardmove *= -1.0;//= 0;
		ucmd.rightmove *= -1.0;//= 0;
		VectorScale( NPC->client->ps.moveDir, -1, NPC->client->ps.moveDir );
	}
	return qfalse;
}
/*
-------------------------
Jedi_HoldPosition
-------------------------
*/

static void Jedi_HoldPosition( void )
{//RACC - Jedi holds position?
	//NPCInfo->squadState = SQUAD_STAND_AND_SHOOT;
	NPCInfo->goalEntity = NULL;
	
	/*
	if ( TIMER_Done( NPC, "stand" ) )
	{
		TIMER_Set( NPC, "duck", Q_irand( 2000, 4000 ) );
	}
	*/
}

/*
-------------------------
Jedi_Move
-------------------------
*/

//[CoOp]
//changed function to act like SP version.
static qboolean Jedi_Move( gentity_t *goal, qboolean retreat )
//static void Jedi_Move( gentity_t *goal, qboolean retreat )
//[/CoOp]
{//move towards or away from the goal entity based on your retreat flag.
	qboolean	moved;
	//[CoOp] NUAM
	//navInfo_t	info;
	//[/CoOp]

	NPCInfo->combatMove = qtrue;
	NPCInfo->goalEntity = goal;

	moved = NPC_MoveToGoal( qtrue );

	//[CoOp]
	if (!moved)
	{//can't move towards target, hold here.
		Jedi_HoldPosition();
	}
	//[/CoOp]

	//FIXME: temp retreat behavior- should really make this toward a safe spot or maybe to outflank enemy
	if ( retreat )
	{//FIXME: should we trace and make sure we can go this way?  Or somehow let NPC_MoveToGoal know we want to retreat and have it handle it?
		ucmd.forwardmove *= -1;
		ucmd.rightmove *= -1;
		VectorScale( NPC->client->ps.moveDir, -1, NPC->client->ps.moveDir );
	}
	
	//[CoOp]
	return moved;

	//this stuff isn't in the SP version.
	/*
	//Get the move info
	NAV_GetLastMove( &info );

	//If we hit our target, then stop and fire!
	if ( ( info.flags & NIF_COLLISION ) && ( info.blocker == NPC->enemy ) )
	{
		Jedi_HoldPosition();
	}

	//If our move failed, then reset
	if ( moved == qfalse )
	{
		Jedi_HoldPosition();
	}
	*/
	//[/CoOp]
}

static qboolean Jedi_Hunt( void )
{//racc - chase our enemy if our agression is high enough. 
	//Com_Printf( "Hunting\n" );
	//if we're at all interested in fighting, go after him
	if ( NPCInfo->stats.aggression > 1 )
	{//approach enemy
		NPCInfo->combatMove = qtrue;
		if ( !(NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) )
		{//racc - don't chase enemies if our chase flag isn't set.  However, keep watch it.
			NPC_UpdateAngles( qtrue, qtrue );
			return qtrue;
		}
		else
		{
			//[CoOp]
			NPCInfo->goalEntity = NPC->enemy; 
			NPCInfo->goalRadius = 40.0f;

			/* This was different than the SP version
			if ( NPCInfo->goalEntity == NULL )
			{//hunt
				NPCInfo->goalEntity = NPC->enemy;
			}
			*/
			//[/CoOp]
			//Jedi_Move( NPC->enemy, qfalse );
			if ( NPC_MoveToGoal( qfalse ) )
			{
				NPC_UpdateAngles( qtrue, qtrue );
				return qtrue;
			}
		}
	}
	return qfalse;
}

/*
static qboolean Jedi_Track( void )
{
	//if we're at all interested in fighting, go after him
	if ( NPCInfo->stats.aggression > 1 )
	{//approach enemy
		NPCInfo->combatMove = qtrue;
		NPC_SetMoveGoal( NPC, NPCInfo->enemyLastSeenLocation, 16, qtrue );
		if ( NPC_MoveToGoal( qfalse ) )
		{
			NPC_UpdateAngles( qtrue, qtrue );
			return qtrue;
		}
	}
	return qfalse;
}
*/

//[CoOp]
extern qboolean PM_PainAnim( int anim );
static void Jedi_StartBackOff( void )
{//start to evade a spin attack.
	TIMER_Set( NPC, "roamTime", -level.time );
	TIMER_Set( NPC, "strafeLeft", -level.time );
	TIMER_Set( NPC, "strafeRight", -level.time );
	TIMER_Set( NPC, "walking", -level.time );
	TIMER_Set( NPC, "moveforward", -level.time );
	TIMER_Set( NPC, "movenone", -level.time );
	TIMER_Set( NPC, "moveright", -level.time );
	TIMER_Set( NPC, "moveleft", -level.time );
	TIMER_Set( NPC, "movecenter", -level.time );
	TIMER_Set( NPC, "moveback", 1000 );
	ucmd.forwardmove = -127; 
	ucmd.rightmove = 0;
	ucmd.upmove = 0;
	if ( d_JediAI.integer )
	{
		Com_Printf( "%s backing off from spin attack!\n", NPC->NPC_type );
	}
	TIMER_Set( NPC, "specialEvasion", 1000 );
	TIMER_Set( NPC, "noRetreat", -level.time );
	if ( PM_PainAnim(NPC->client->ps.legsAnim) )
	{//override leg pain animation if running
		NPC->client->ps.legsTimer = 0;
	}
	VectorClear( NPC->client->ps.moveDir );
}
//[/CoOp]

//[CoOp]
static qboolean Jedi_Retreat( void )
//static void Jedi_Retreat( void )
//[/CoOp]
{//RACC - Retreat!
	if ( !TIMER_Done( NPC, "noRetreat" ) )
	{//don't actually move
		//[CoOp]
		return qfalse;
		//return;
		//[/CoOp]
	}
	//FIXME: when retreating, we should probably see if we can retreat 
	//in the direction we want.  If not...?  Evade?
	//Com_Printf( "Retreating\n" );
	//[CoOp]
	return Jedi_Move( NPC->enemy, qtrue );
	//[/CoOp]
}

static void Jedi_Advance( void )
{//racc - advance on our enemy
	//[CoOp]
	if ( (NPCInfo->aiFlags&NPCAI_HEAL_ROSH) )
	{//Boss Rosh's Dark Jedi healers don't attack.
		return;
	}
	//[/CoOp]

	if ( !NPC->client->ps.saberInFlight )
	{
		//NPC->client->ps.SaberActivate();
		WP_ActivateSaber(NPC);
	}
	//Com_Printf( "Advancing\n" );
	Jedi_Move( NPC->enemy, qfalse );
		
	//TIMER_Set( NPC, "roamTime", Q_irand( 2000, 4000 ) );
	//TIMER_Set( NPC, "attackDelay", Q_irand( 250, 500 ) );
	//TIMER_Set( NPC, "duck", 0 );
}

static void Jedi_AdjustSaberAnimLevel( gentity_t *self, int newLevel )
{	
	if ( !self || !self->client )
	{
		return;
	}
	//FIXME: each NPC shold have a unique pattern of behavior for the order in which they
	//[CoOp]
	//these aren't in the SP code so I assume they're just MP junk.
	/*
	if ( self->client->NPC_class == CLASS_TAVION )
	{//special attacks
		self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_5;
		return;
	}
	else if ( self->client->NPC_class == CLASS_DESANN )
	{//special attacks
		self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_4;
		return;
	}
	*/
	//[/CoOp]
	if ( self->client->playerTeam == NPCTEAM_ENEMY )
	{//racc - force stance types based on rank or NPC class
		//[CoOp]
		//special stances for specific cultist types
		//FIXME: CLASS_CULTIST + self->NPC->rank instead of these Q_stricmps?
		if ( !Q_stricmp( "cultist_saber_all", self->NPC_type )
			|| !Q_stricmp( "cultist_saber_all_throw", self->NPC_type ) )
		{//use any, regardless of rank, etc.
		}
		else if ( !Q_stricmp( "cultist_saber", self->NPC_type )
				|| !Q_stricmp( "cultist_saber_throw", self->NPC_type ) )
		{//fast only
			self->client->ps.fd.saberAnimLevel = SS_FAST;
		}
		else if ( !Q_stricmp( "cultist_saber_med", self->NPC_type )
					|| !Q_stricmp( "cultist_saber_med_throw", self->NPC_type ) )
		{//med only
			self->client->ps.fd.saberAnimLevel = SS_MEDIUM;
		}
		else if ( !Q_stricmp( "cultist_saber_strong", self->NPC_type )
				|| !Q_stricmp( "cultist_saber_strong_throw", self->NPC_type ) )
		{//strong only
			self->client->ps.fd.saberAnimLevel = SS_STRONG;
		}
		else
		{
			if ( self->NPC->rank == RANK_CIVILIAN || self->NPC->rank == RANK_LT_JG )
			{//grunt and fencer always uses quick attacks
				self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
				return;
			}
			if ( self->NPC->rank == RANK_CREWMAN 
				|| self->NPC->rank == RANK_ENSIGN )
			{//acrobat & force-users always use medium attacks
				self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_2;
				return;
			}
			/*
			if ( self->NPC->rank == RANK_LT ) 
			{//boss always uses strong attacks
				self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_3;
				return;
			}
			*/
		}
		//[/CoOp]
	}
	//use the different attacks, how often they switch and under what circumstances
	if ( newLevel > self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE] )
	{//cap it
		self->client->ps.fd.saberAnimLevel = self->client->ps.fd.forcePowerLevel[FP_SABER_OFFENSE];
	}
	else if ( newLevel < FORCE_LEVEL_1 )
	{
		self->client->ps.fd.saberAnimLevel = FORCE_LEVEL_1;
	}
	else
	{//go ahead and set it
		self->client->ps.fd.saberAnimLevel = newLevel;
	}

	if ( d_JediAI.integer )
	{
		switch ( self->client->ps.fd.saberAnimLevel )
		{
		case FORCE_LEVEL_1:
			Com_Printf( S_COLOR_GREEN"%s Saber Attack Set: fast\n", self->NPC_type );
			break;
		case FORCE_LEVEL_2:
			Com_Printf( S_COLOR_YELLOW"%s Saber Attack Set: medium\n", self->NPC_type );
			break;
		case FORCE_LEVEL_3:
			Com_Printf( S_COLOR_RED"%s Saber Attack Set: strong\n", self->NPC_type );
			break;
		}
	}
}

static void Jedi_CheckDecreaseSaberAnimLevel( void )
{//racc - check to see if we want to change saber stance.
	if ( !NPC->client->ps.weaponTime && !(ucmd.buttons&(BUTTON_ATTACK|BUTTON_ALT_ATTACK)) )
	{//not attacking
		if ( TIMER_Done( NPC, "saberLevelDebounce" ) && !Q_irand( 0, 10 ) )
		{
			//Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.fd.saberAnimLevel-1) );//drop
			Jedi_AdjustSaberAnimLevel( NPC, Q_irand( FORCE_LEVEL_1, FORCE_LEVEL_3 ));//random
			TIMER_Set( NPC, "saberLevelDebounce", Q_irand( 3000, 10000 ) );
		}
	}
	else
	{
		TIMER_Set( NPC, "saberLevelDebounce", Q_irand( 1000, 5000 ) );
	}
}


//[CoOp]
extern qboolean G_InGetUpAnim(playerState_t *ps);
static qboolean Jedi_DecideKick( void )
{//check to see if we feel like kicking
	if ( PM_InKnockDown( &NPC->client->ps ) )
	{
		return qfalse;
	}
	if ( BG_InRoll( &NPC->client->ps, NPC->client->ps.legsAnim ) )
	{
		return qfalse;
	}
	if ( G_InGetUpAnim( &NPC->client->ps ) )
	{
		return qfalse;
	}
	if ( !NPC->enemy || (NPC->enemy->s.number < MAX_CLIENTS&&NPC->enemy->health<=0) )
	{//have no enemy or enemy is a dead player
		return qfalse;
	}
	//FIXME: check FP_SABER_OFFENSE?
	//FIXME: check for saber staff style only?
	//FIXME: g_spskill?
	if ( Q_irand( 0, RANK_CAPTAIN+5 ) > NPCInfo->rank )
	{//low chance, based on rank
		return qfalse;
	}
	if ( Q_irand( 0, 10 ) > NPCInfo->stats.aggression )
	{//the madder the better
		return qfalse;
	}
	if ( !TIMER_Done( NPC, "kickDebounce" ) )
	{//just did one
		return qfalse;
	}
	if ( NPC->client->ps.weapon == WP_SABER )
	{
		if ( (NPC->client->saber[0].saberFlags&SFL_NO_KICKS) )
		{//can't kick with this saber
			return qfalse;
		}
		else if ( NPC->client->saber[1].model 
			&& NPC->client->saber[1].model[0] 
			&& (NPC->client->saber[1].saberFlags&SFL_NO_KICKS) )
		{//can't kick with this saber
			return qfalse;
		}
	}
	//go for it!
	return qtrue;
}

/*
==========================================================================================
KYLE GRAPPLE CODE
==========================================================================================
*/

void Kyle_GrabEnemy( void )
{
	//RAFIXME - this is broken until we impliment the grapples in Basic I guess.
	//WP_SabersCheckLock2( NPC, NPC->enemy, Q_irand(LOCK_KYLE_GRAB1,LOCK_KYLE_GRAB2) );//LOCK_KYLE_GRAB3
	TIMER_Set( NPC, "grabEnemyDebounce", NPC->client->ps.torsoTimer + Q_irand( 4000, 20000 ) );
}

void Kyle_TryGrab( void )
{//have the kyle boss NPC attempt a grab.
	NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_KYLE_GRAB, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	NPC->client->ps.torsoTimer += 200;
	NPC->client->ps.weaponTime= NPC->client->ps.torsoTimer;
	NPC->client->ps.saberMove /* not sure what this is = NPC->client->ps.saberMoveNext*/ = LS_READY;
	VectorClear( NPC->client->ps.velocity );
	VectorClear( NPC->client->ps.moveDir );
	ucmd.rightmove = ucmd.forwardmove = ucmd.upmove = 0;
	NPC->painDebounceTime = level.time + NPC->client->ps.torsoTimer;
	//WTF?
	NPC->client->ps.saberHolstered = 2;
}

extern qboolean PM_InOnGroundAnim ( int anim );
qboolean Kyle_CanDoGrab( void )
{//Kyle can do grab on this enemy
	if ( NPC->client->NPC_class == CLASS_KYLE && (NPC->spawnflags&1) )
	{//Boss Kyle
		if ( NPC->enemy && NPC->enemy->client )
		{//have a valid enemy
			if ( TIMER_Done( NPC, "grabEnemyDebounce" ) )
			{//okay to grab again
				if ( NPC->client->ps.groundEntityNum != ENTITYNUM_NONE
					&& NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{//me and enemy are on ground
					if ( !PM_InOnGroundAnim( NPC->enemy->client->ps.legsAnim ) )
					{
						if ( (NPC->client->ps.weaponTime <= 200||NPC->client->ps.torsoAnim==BOTH_KYLE_GRAB)
							&& !NPC->client->ps.saberInFlight )
						{
							if ( fabs(NPC->enemy->r.currentOrigin[2]-NPC->r.currentOrigin[2])<=8.0f )
							{//close to same level of ground
								if ( DistanceSquared( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin ) <= 10000.0f )
								{
									return qtrue;
								}
							}
						}
					}
				}
			}
		}
	}
	return qfalse;
}

/*
==========================================================================================
END KYLE GRAPPLE CODE
==========================================================================================
*/
//[/CoOp]

//[CoOp]
saberMoveName_t G_PickAutoMultiKick( gentity_t *self, qboolean allowSingles, qboolean storeMove )
{
	//RAFIXME - impliment multikicks
	return LS_NONE;
}


extern qboolean G_CanKickEntity( gentity_t *self, gentity_t *target );
extern saberMoveName_t G_PickAutoKick( gentity_t *self, gentity_t *enemy, qboolean storeMove );
//[/CoOp]
extern float NPC_EnemyRangeFromBolt( int boltIndex );
static void Jedi_CombatDistance( int enemy_dist )
{//FIXME: for many of these checks, what we really want is horizontal distance to enemy
	//[CoOp]
	if ( Jedi_CultistDestroyer( NPC ) )
	{//destroyers suicide charge
		Jedi_Advance();
		//always run, regardless of what navigation tells us to do!
		NPC->client->ps.speed = NPCInfo->stats.runSpeed;
		ucmd.buttons &= ~BUTTON_WALKING;
		return;
	}
	if ( enemy_dist < 128 
		&& NPC->enemy
		&& NPC->enemy->client
		&& (NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK6
			|| NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK7 ) )
	{//whoa, back off!!! attempt to dodge spin attacks
		if ( Q_irand( -3, NPCInfo->rank ) > RANK_CREWMAN )
		{
			Jedi_StartBackOff();
			return;
		}
	}
	//[/CoOp]

	if ( NPC->client->ps.fd.forcePowersActive&(1<<FP_GRIP) &&
		NPC->client->ps.fd.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
	{//when gripping, don't move
		return;
	}
	else if ( !TIMER_Done( NPC, "gripping" ) )
	{//stopped gripping, clear timers just in case
		TIMER_Set( NPC, "gripping", -level.time );
		TIMER_Set( NPC, "attackDelay", Q_irand( 0, 1000 ) );
	}
	
	if ( NPC->client->ps.fd.forcePowersActive&(1<<FP_DRAIN) &&
		NPC->client->ps.fd.forcePowerLevel[FP_DRAIN] > FORCE_LEVEL_1 )
	{//when draining, don't move
		return;
	}
	else if ( !TIMER_Done( NPC, "draining" ) )
	{//stopped draining, clear timers just in case
		TIMER_Set( NPC, "draining", -level.time );
		TIMER_Set( NPC, "attackDelay", Q_irand( 0, 1000 ) );
	}
	//racc - handle combat distance for boba fett
	if ( NPC->client->NPC_class == CLASS_BOBAFETT )
	{
		if ( !TIMER_Done( NPC, "flameTime" ) )
		{
			if ( enemy_dist > 50 )
			{
				Jedi_Advance();
			}
			else if ( enemy_dist <= 0 )
			{
				Jedi_Retreat();
			}
		}
		else if ( enemy_dist < 200 )
		{
			Jedi_Retreat();
		}
		else if ( enemy_dist > 1024 )
		{
			Jedi_Advance();
		}
	}
	//[CoOp]
	else if ( NPC->client->ps.legsAnim == BOTH_ALORA_SPIN_THROW )
	{//don't move at all when alora is doing her spin throw attack.
		//FIXME: sabers need trails
	}
	else if ( NPC->client->ps.torsoAnim == BOTH_KYLE_GRAB )
	{//see if we grabbed enemy
		if ( NPC->client->ps.torsoTimer <= 200 )
		{
			if ( Kyle_CanDoGrab()
				&& NPC_EnemyRangeFromBolt( 0 ) <= 72.0f )
			{//grab him!
				Kyle_GrabEnemy();
				return;
			}
			else
			{//biffed it!
				NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
				NPC->client->ps.weaponTime = NPC->client->ps.torsoTimer;
				return;
			}
		}
		//else just sit here?
		return;
	}
	//[/CoOp]
	else if ( NPC->client->ps.saberInFlight &&
		!PM_SaberInBrokenParry( NPC->client->ps.saberMove )
		&& NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
	{//racc - maintain distance while your saber has been tossed.
		if ( enemy_dist < NPC->client->ps.saberEntityDist )
		{//racc - enemy is closer than your saber
			Jedi_Retreat();
		}
		else if ( enemy_dist > NPC->client->ps.saberEntityDist && enemy_dist > 100 )
		{//racc - enemy is far away and our saber closer than the enemy
			Jedi_Advance();
		}
		if ( NPC->client->ps.weapon == WP_SABER //using saber
			&& NPC->client->ps.saberEntityState == SES_LEAVING  //not returning yet
			&& NPC->client->ps.fd.forcePowerLevel[FP_SABERTHROW] > FORCE_LEVEL_1 //2nd or 3rd level lightsaber
			&& !(NPC->client->ps.fd.forcePowersActive&(1 << FP_SPEED))
			&& !(NPC->client->ps.saberEventFlags&SEF_INWATER) )//saber not in water
		{//hold it out there
			ucmd.buttons |= BUTTON_ALT_ATTACK;
			//FIXME: time limit?
		}
	}
	else if ( !TIMER_Done( NPC, "taunting" ) )
	{//racc - we're taunting
		if ( enemy_dist <= 64 )
		{//he's getting too close
			ucmd.buttons |= BUTTON_ATTACK;
			if ( !NPC->client->ps.saberInFlight )
			{
				WP_ActivateSaber(NPC);
			}
			TIMER_Set( NPC, "taunting", -level.time );
		}
		//racc - this is different than SP but I think this is for the better.
		//else if ( NPC->client->ps.torsoAnim == BOTH_GESTURE1 && NPC->client->ps.torsoTimer < 2000 )
		else if (NPC->client->ps.forceHandExtend == HANDEXTEND_JEDITAUNT && (NPC->client->ps.forceHandExtendTime - level.time) < 200)
		{//we're almost done with our special taunt
			//FIXME: this doesn't always work, for some reason
			if ( !NPC->client->ps.saberInFlight )
			{
				WP_ActivateSaber(NPC);
			}
		}
	}
	else if ( NPC->client->ps.saberEventFlags&SEF_LOCK_WON )
	{//we won a saber lock, press the advantage
		if ( enemy_dist > 0 )
		{//get closer so we can hit!
			Jedi_Advance();
		}
		if ( enemy_dist > 128 )
		{//lost 'em
			NPC->client->ps.saberEventFlags &= ~SEF_LOCK_WON;
		}
		if ( NPC->enemy->painDebounceTime + 2000 < level.time )
		{//the window of opportunity is gone
			NPC->client->ps.saberEventFlags &= ~SEF_LOCK_WON;
		}
		//don't strafe?
		TIMER_Set( NPC, "strafeLeft", -1 );
		TIMER_Set( NPC, "strafeRight", -1 );
	}
	else if ( NPC->enemy->client 
		&& NPC->enemy->s.weapon == WP_SABER 
		&& NPC->enemy->client->ps.saberLockTime > level.time 
		&& NPC->client->ps.saberLockTime < level.time )
	{//enemy is in a saberLock and we are not
		if ( enemy_dist < 64 )
		{//FIXME: maybe just pick another enemy?
			Jedi_Retreat();
		}
	}
	//[CoOp]
	//reenabled this.  I think it's for attacking turrets
	else if ( NPC->enemy->s.weapon == WP_TURRET 
		&& !Q_stricmp( "PAS", NPC->enemy->classname ) 
		&& NPC->enemy->s.apos.trType == TR_STATIONARY )
	{
		int	testlevel;
		if ( enemy_dist > forcePushPullRadius[FORCE_LEVEL_1] - 16 )
		{
			Jedi_Advance();
		}
		if ( NPC->client->ps.fd.forcePowerLevel[FP_PUSH] < FORCE_LEVEL_1 )
		{//
			testlevel = FORCE_LEVEL_1;
		}
		else
		{
			testlevel = NPC->client->ps.fd.forcePowerLevel[FP_PUSH];
		}
		if ( enemy_dist < forcePushPullRadius[testlevel] - 16 )
		{//close enough to push
			if ( InFront( NPC->enemy->r.currentOrigin, NPC->client->renderInfo.eyePoint, NPC->client->renderInfo.eyeAngles, 0.6f ) )
			{//knock it down
				//RAFIXME impliment this function
				//WP_KnockdownTurret( NPC, NPC->enemy );
				//do the forcethrow call just for effect
				ForceThrow( NPC, qfalse );
			}
		}
	}
	//[/CoOp]
	//rwwFIXMEFIXME: Give them the ability to do this again (turret needs to be fixed up to allow it)
	else if ( enemy_dist <= 64 
		&& ((NPCInfo->scriptFlags&SCF_DONT_FIRE)||(!Q_stricmp("Yoda",NPC->NPC_type)&&!Q_irand(0,10))) )
	{//can't use saber and they're in striking range
		if ( !Q_irand( 0, 5 ) && InFront( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, NPC->client->ps.viewangles, 0.2f ) )
		{
			if ( ((NPCInfo->scriptFlags&SCF_DONT_FIRE)||NPC->client->pers.maxHealth - NPC->health > NPC->client->pers.maxHealth*0.25f)//lost over 1/4 of our health or not firing
				//[CoOp]
				&& WP_ForcePowerUsable( NPC, FP_DRAIN, 20 )//know how to drain and have enough power
				//&& NPC->client->ps.fd.forcePowersKnown&(1<<FP_DRAIN) //know how to drain
				//&& WP_ForcePowerAvailable( NPC, FP_DRAIN, 20 )//have enough power
				//[/CoOp]
				&& !Q_irand( 0, 2 ) )
			{//drain
				TIMER_Set( NPC, "draining", 3000 );
				TIMER_Set( NPC, "attackDelay", 3000 );
				Jedi_Advance();
				return;
			}
			//[CoOp]
			else
			{//ok, we can't drain at the moment
				//RAFIXME - impliment this later
				if ( Jedi_DecideKick() )
				{//let's try a kick
					if ( G_PickAutoMultiKick( NPC, qfalse, qtrue ) != LS_NONE 
						|| (G_CanKickEntity(NPC, NPC->enemy) && G_PickAutoKick( NPC, NPC->enemy, qtrue ) != LS_NONE) ) 
					{//kicked!
						TIMER_Set( NPC, "kickDebounce", Q_irand( 3000, 10000 ) );
						return;
					}
				}
				ForceThrow( NPC, qfalse );
			}
			/*
			else
			{
				ForceThrow( NPC, qfalse );
			}
			*/
			//[/CoOp]
		}
		Jedi_Retreat();
	}
	else if ( enemy_dist <= 64 
		&& NPC->client->pers.maxHealth - NPC->health > NPC->client->pers.maxHealth*0.25f//lost over 1/4 of our health
		&& NPC->client->ps.fd.forcePowersKnown&(1<<FP_DRAIN) //know how to drain
		&& WP_ForcePowerAvailable( NPC, FP_DRAIN, 20 )//have enough power
		&& !Q_irand( 0, 10 ) 
		&& InFront( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, NPC->client->ps.viewangles, 0.2f ) )
	{//racc - try to drain some health
		TIMER_Set( NPC, "draining", 3000 );
		TIMER_Set( NPC, "attackDelay", 3000 );
		Jedi_Advance();
		return;
	}
	else if ( enemy_dist <= -16 )
	{//we're too damn close!
		//[CoOp]
		if ( !Q_irand( 0, 30 )
			&& Kyle_CanDoGrab() )
		{//try to grapple
			Kyle_TryGrab();
			return;
		}
		/* RAFIXME - this weapon doesn't exist yet
		else if ( NPC->client->ps.stats[STAT_WEAPONS]&(1<<WP_SCEPTER) 
			&& !Q_irand( 0, 20 ) )
		{
			Tavion_StartScepterSlam();
			return;
		}
		*/
		if ( Jedi_DecideKick() )
		{//let's try a kick
			if ( G_PickAutoMultiKick( NPC, qfalse, qtrue ) != LS_NONE
				|| (G_CanKickEntity(NPC, NPC->enemy ) && G_PickAutoKick( NPC, NPC->enemy, qtrue ) != LS_NONE ) )
			{//kicked!
				TIMER_Set( NPC, "kickDebounce", Q_irand( 3000, 10000 ) );
				return;
			}
		}
		//[/CoOp]
		Jedi_Retreat();
	}
	else if ( enemy_dist <= 0 )
	{//we're within striking range
		//if we are attacking, see if we should stop
		if ( NPCInfo->stats.aggression < 4 )
		{//back off and defend
			//[CoOp]
			if ( !Q_irand( 0, 30 )
				&& Kyle_CanDoGrab() )
			{//try to grapple
				Kyle_TryGrab();
				return;
			}
			/* RAFIXME - this weapon doesn't exist yet
			else if ( NPC->client->ps.stats[STAT_WEAPONS]&(1<<WP_SCEPTER) 
				&& !Q_irand( 0, 20 ) )
			{
				Tavion_StartScepterSlam();
				return;
			}
			*/
			if ( Jedi_DecideKick() )
			{//let's try a kick
				if ( G_PickAutoMultiKick( NPC, qfalse, qtrue ) != LS_NONE
					|| (G_CanKickEntity(NPC, NPC->enemy ) && G_PickAutoKick( NPC, NPC->enemy, qtrue ) != LS_NONE ) )
				{//kicked!
					TIMER_Set( NPC, "kickDebounce", Q_irand( 3000, 10000 ) );
					return;
				}
			}
			//[/CoOp]
			Jedi_Retreat();
		}
	}
	else if ( enemy_dist > 256 )
	{//we're way out of range
		qboolean usedForce = qfalse;
		if ( NPCInfo->stats.aggression < Q_irand( 0, 20 ) 
			&& NPC->health < NPC->client->pers.maxHealth*0.75f 
			&& !Q_irand( 0, 2 ) )
		{//racc - we're out of rand so let's check for a special activity to do.
			//healing, protection, etc.
			//[CoOp]
			if ( NPC->enemy 
				&& NPC->enemy->s.number < MAX_CLIENTS
				&& NPC->client->NPC_class!=CLASS_KYLE
				&& ((NPCInfo->aiFlags&NPCAI_BOSS_CHARACTER)
					|| NPC->client->NPC_class==CLASS_SHADOWTROOPER) 
				&& Q_irand(0, 3-g_spskill.integer) )
			{//hmm, bosses should do this less against the player
			}
			else if ( NPC->client->saber[0].type == SABER_SITH_SWORD 
				&& NPC->client->weaponGhoul2[0] )
			{//packing the sith sword.  we can use it to recharge our health.
				Tavion_SithSwordRecharge();
				usedForce = qtrue;
			}
			else if ( (NPC->client->ps.fd.forcePowersKnown&(1<<FP_HEAL)) != 0 
			//if ( (NPC->client->ps.fd.forcePowersKnown&(1<<FP_HEAL)) != 0 
			//[/CoOp]
				&& (NPC->client->ps.fd.forcePowersActive&(1<<FP_HEAL)) == 0 
				&& Q_irand( 0, 1 ) )
			{
				ForceHeal( NPC );
				usedForce = qtrue;
				//FIXME: check level of heal and know not to move or attack when healing
			}
			else if ( (NPC->client->ps.fd.forcePowersKnown&(1<<FP_PROTECT)) != 0 
				&& (NPC->client->ps.fd.forcePowersActive&(1<<FP_PROTECT)) == 0
				&& Q_irand( 0, 1 ) )
			{
				ForceProtect( NPC );
				usedForce = qtrue;
			}
			else if ( (NPC->client->ps.fd.forcePowersKnown&(1<<FP_ABSORB)) != 0 
				&& (NPC->client->ps.fd.forcePowersActive&(1<<FP_ABSORB)) == 0
				&& Q_irand( 0, 1 ) )
			{
				ForceAbsorb( NPC );
				usedForce = qtrue;
			}
			else if ( (NPC->client->ps.fd.forcePowersKnown&(1<<FP_RAGE)) != 0 
				&& (NPC->client->ps.fd.forcePowersActive&(1<<FP_RAGE)) == 0
				&& Q_irand( 0, 1 ) )
			{
				Jedi_Rage();
				usedForce = qtrue;
			}
			//FIXME: what about things like mind tricks and force sight?
		}
		if ( enemy_dist > 384 )
		{//FIXME: check for enemy facing away and/or moving away
			if ( !Q_irand( 0, 10 ) && NPCInfo->blockedSpeechDebounceTime < level.time && jediSpeechDebounceTime[NPC->client->playerTeam] < level.time )
			{
				if ( NPC_ClearLOS4( NPC->enemy ) )
				{//racc - yell at escaping enemy.
					G_AddVoiceEvent( NPC, Q_irand( EV_JCHASE1, EV_JCHASE3 ), 3000 );
				}
				jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
			}
		}
		//Unless we're totally hiding, go after him
		if ( NPCInfo->stats.aggression > 0 )
		{//approach enemy
			if ( !usedForce )
			{
				//[CoOp]
				if ( NPC->enemy
					&& NPC->enemy->client
					&& (NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK6
						|| NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK7 ) )
				{//stay put!  They're trying to slice us up!
				}
				else
				{
					Jedi_Advance();
				}
				//Jedi_Advance();
				//[/CoOp]
			}
		}
	}
	/*
	else if ( enemy_dist < 96 && NPC->enemy && NPC->enemy->client && NPC->enemy->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//too close and in air, so retreat
		Jedi_Retreat();
	}
	*/
	//FIXME: enemy_dist calc needs to include all blade lengths, and include distance from hand to start of blade....
	else if ( enemy_dist > 50 )//FIXME: not hardcoded- base on our reach (modelScale?) and saberLengthMax
	{//we're out of striking range and we are allowed to attack
		//first, check some tactical force power decisions
		if ( NPC->enemy && NPC->enemy->client && (NPC->enemy->client->ps.fd.forceGripBeingGripped > level.time) )
		{//They're being gripped, rush them!
			if ( NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
			{//they're on the ground, so advance
				if ( TIMER_Done( NPC, "parryTime" ) || NPCInfo->rank > RANK_LT )
				{//not parrying
					if ( enemy_dist > 200 || !(NPCInfo->scriptFlags&SCF_DONT_FIRE) )
					{//far away or allowed to use saber
						Jedi_Advance();
					}
				}
			}
			//[CoOp]
			//We'd like to be able to use saber throw when a lower ranked dude.
			if ( (NPCInfo->rank >= RANK_LT_JG||WP_ForcePowerUsable( NPC, FP_SABERTHROW, 0 ))
			//if ( NPCInfo->rank >= RANK_LT_JG 
			//[/CoOp]
				&& !Q_irand( 0, 5 ) 
				&& !(NPC->client->ps.fd.forcePowersActive&(1 << FP_SPEED))
				&& !(NPC->client->ps.saberEventFlags&SEF_INWATER) )//saber not in water
			{//throw saber
				ucmd.buttons |= BUTTON_ALT_ATTACK;
			}
		}
		else if ( NPC->enemy && NPC->enemy->client && //valid enemy
			NPC->enemy->client->ps.saberInFlight && /*NPC->enemy->client->ps.saber[0].Active()*/ NPC->enemy->client->ps.saberEntityNum && //enemy throwing saber
			NPC->client->ps.weaponTime <= 0 && //I'm not busy
			WP_ForcePowerAvailable( NPC, FP_GRIP, 0 ) && //I can use the power
			!Q_irand( 0, 10 ) && //don't do it all the time, averages to 1 check a second
			Q_irand( 0, 6 ) < g_spskill.integer && //more likely on harder diff
			//[CoOp]
			Q_irand( RANK_CIVILIAN, RANK_CAPTAIN ) < NPCInfo->rank //more likely against harder enemies
			//Q_irand( RANK_CIVILIAN, RANK_CAPTAIN ) < NPCInfo->rank )//more likely against harder enemies
			&& InFOV3( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, NPC->client->ps.viewangles, 20, 30 ) )	
			//[/CoOp]
		{//They're throwing their saber, grip them!
			//taunt
			if ( TIMER_Done( NPC, "chatter" ) && jediSpeechDebounceTime[NPC->client->playerTeam] < level.time && NPCInfo->blockedSpeechDebounceTime < level.time )
			{
				G_AddVoiceEvent( NPC, Q_irand( EV_TAUNT1, EV_TAUNT3 ), 3000 );
				jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
				//[CoOp]
				if ( (NPCInfo->aiFlags&NPCAI_ROSH) )
				{
					TIMER_Set( NPC, "chatter", 6000 );
				}
				else
				{
					TIMER_Set( NPC, "chatter", 3000 );
				}
				//TIMER_Set( NPC, "chatter", 3000 );
				//[/CoOp]
			}

			//grip
			TIMER_Set( NPC, "gripping", 3000 );
			TIMER_Set( NPC, "attackDelay", 3000 );
		}
		else
		{
			//[CoOp]
			//moved down.
			//int chanceScale;
			//[/CoOp]
			
			if ( NPC->enemy && NPC->enemy->client && (NPC->enemy->client->ps.fd.forcePowersActive&(1<<FP_GRIP)) )
			{//They're choking someone, probably an ally, run at them and do some sort of attack
				if ( NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{//they're on the ground, so advance
					if ( TIMER_Done( NPC, "parryTime" ) || NPCInfo->rank > RANK_LT )
					{//not parrying
						if ( enemy_dist > 200 || !(NPCInfo->scriptFlags&SCF_DONT_FIRE) )
						{//far away or allowed to use saber
							Jedi_Advance();
						}
					}
				}
			}

			//[CoOp]
			if ( NPC->client->NPC_class == CLASS_KYLE
				&& (NPC->spawnflags&1)
				&& (NPC->enemy&&NPC->enemy->client&&!NPC->enemy->client->ps.saberInFlight)
				&& TIMER_Done( NPC, "kyleTakesSaber" )
				&& !Q_irand( 0, 20 ) )
			{//kyle tries to pull the player's saber out of their hands
				ForceThrow( NPC, qtrue );
			}
			/*RAFIXME - scepter not implimented
			else if ( NPC->client->ps.stats[STAT_WEAPONS]&(1<<WP_SCEPTER) 
				&& !Q_irand( 0, 20 ) )
			{
				Tavion_StartScepterBeam();
				return;
			}
			*/
			else
			{
				int chanceScale = 0;
				if ( NPC->client->NPC_class == CLASS_KYLE && (NPC->spawnflags&1) )
				{
					chanceScale = 4;
				}
				else if ( NPC->enemy 
					&& NPC->enemy->s.number < MAX_CLIENTS
					&& ((NPCInfo->aiFlags&NPCAI_BOSS_CHARACTER)
						|| NPC->client->NPC_class==CLASS_SHADOWTROOPER) )
				{//hmm, bosses do this less against player
					chanceScale = 8 - g_spskill.integer*2; 
				}
				
				//chanceScale = 0;
				if ( NPC->client->NPC_class == CLASS_DESANN || !Q_stricmp("Yoda",NPC->NPC_type) )
				{
					chanceScale = 1;
				}
				else if ( NPCInfo->rank == RANK_ENSIGN )
				{
					chanceScale = 2;
				}
				else if ( NPCInfo->rank >= RANK_LT_JG )
				{
					chanceScale = 5;
				}
				if ( chanceScale 
					&& (enemy_dist > Q_irand( 100, 200 ) || (NPCInfo->scriptFlags&SCF_DONT_FIRE) || (!Q_stricmp("Yoda",NPC->NPC_type)&&!Q_irand(0,3)) )
					&& enemy_dist < 500 
					&& (Q_irand( 0, chanceScale*10 )<5 || (NPC->enemy->client && NPC->enemy->client->ps.weapon != WP_SABER && !Q_irand( 0, chanceScale ) ) ) )
				{//else, randomly try some kind of attack every now and then
						//FIXME: Cultist fencers don't have any of these fancy powers
						//			the only thing they might be able to do is throw their saber
					//racc - this should really help with the non-saber cultist being idiots.
					if ( (NPCInfo->rank == RANK_ENSIGN || NPCInfo->rank > RANK_LT_JG) 
						&& (!Q_irand( 0, 1 ) || NPC->s.weapon != WP_SABER) )
					//if ( (NPCInfo->rank == RANK_ENSIGN || NPCInfo->rank > RANK_LT_JG) && !Q_irand( 0, 1 ) )
					{
						if ( WP_ForcePowerUsable( NPC, FP_PULL, 0 ) && !Q_irand( 0, 2 ) )
						//if ( WP_ForcePowerAvailable( NPC, FP_PULL, 0 ) && !Q_irand( 0, 2 ) )
						{
							//force pull the guy to me!
							//FIXME: check forcePushRadius[NPC->client->ps.fd.forcePowerLevel[FP_PUSH]]
							ForceThrow( NPC, qtrue );
							//maybe strafe too?
							TIMER_Set( NPC, "duck", enemy_dist*3 );
							if ( Q_irand( 0, 1 ) )
							{
								ucmd.buttons |= BUTTON_ATTACK;
							}
						}
						//let lightning cultists spam the lightning.
						else if ( WP_ForcePowerUsable( NPC, FP_LIGHTNING, 0 ) 
								&& ((NPCInfo->scriptFlags&SCF_DONT_FIRE)
								&&Q_stricmp("cultist_lightning",NPC->NPC_type) 
								|| Q_irand( 0, 1 )) )
						//else if ( WP_ForcePowerAvailable( NPC, FP_LIGHTNING, 0 ) && Q_irand( 0, 1 ) )
						{
							ForceLightning( NPC );
							if ( NPC->client->ps.fd.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1 )
							{
								NPC->client->ps.weaponTime = Q_irand( 1000, 3000+(g_spskill.integer*500) );
								TIMER_Set( NPC, "holdLightning", NPC->client->ps.weaponTime );
							}
							TIMER_Set( NPC, "attackDelay", NPC->client->ps.weaponTime );
						}
						else if ( NPC->health < NPC->client->pers.maxHealth * 0.75f
							&& Q_irand( FORCE_LEVEL_0, NPC->client->ps.fd.forcePowerLevel[FP_DRAIN] ) > FORCE_LEVEL_1
							&& WP_ForcePowerAvailable( NPC, FP_DRAIN, 0 ) 
							&& Q_irand( 0, 1 ) )
						{
							//RAFIXME - ForceDrain2 isn't in MP so we're going to just hope that
							//that the "draining" timer can handle it.
							//ForceDrain2( NPC );
							NPC->client->ps.weaponTime = Q_irand( 1000, 3000+(g_spskill.integer*500) );
							TIMER_Set( NPC, "draining", NPC->client->ps.weaponTime );
							TIMER_Set( NPC, "attackDelay", NPC->client->ps.weaponTime );
						}
						//rwwFIXMEFIXME: After new drain stuff from SP is in re-enable this.
						else if ( WP_ForcePowerUsable( NPC, FP_GRIP, 0 )
								&& NPC->enemy 
								&& InFOV3( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, 
								NPC->client->ps.viewangles, 20, 30 ) )
						//else if ( WP_ForcePowerAvailable( NPC, FP_GRIP, 0 ) )
						{
							//taunt
							if ( TIMER_Done( NPC, "chatter" ) && jediSpeechDebounceTime[NPC->client->playerTeam] < level.time && NPCInfo->blockedSpeechDebounceTime < level.time )
							{
								G_AddVoiceEvent( NPC, Q_irand( EV_TAUNT1, EV_TAUNT3 ), 3000 );
								jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
								if ( (NPCInfo->aiFlags&NPCAI_ROSH) )
								{//Rosh has a longer taunt?
									TIMER_Set( NPC, "chatter", 6000 );
								}
								else
								{
									TIMER_Set( NPC, "chatter", 3000 );
								}
								//TIMER_Set( NPC, "chatter", 3000 );
								//[CoOp]
							}

							//grip
							TIMER_Set( NPC, "gripping", 3000 );
							TIMER_Set( NPC, "attackDelay", 3000 );
						}
						else
						{
							if ( WP_ForcePowerAvailable( NPC, FP_SABERTHROW, 0 ) 
								&& !(NPC->client->ps.fd.forcePowersActive&(1 << FP_SPEED))
								&& !(NPC->client->ps.saberEventFlags&SEF_INWATER) )//saber not in water
							{//throw saber
								ucmd.buttons |= BUTTON_ALT_ATTACK;
							}
						}
					}
					else
					{
						//[CoOp]
						//we want to be able to toss if we'll a lower rank
						if ( (NPCInfo->rank >= RANK_LT_JG ||WP_ForcePowerUsable( NPC, FP_SABERTHROW, 0 ))
						//if ( NPCInfo->rank >= RANK_LT_JG 	
						//[/CoOp]
							&& !(NPC->client->ps.fd.forcePowersActive&(1 << FP_SPEED))
							&& !(NPC->client->ps.saberEventFlags&SEF_INWATER) )//saber not in water
						{//throw saber
							ucmd.buttons |= BUTTON_ALT_ATTACK;
						}
					}
				}
				//see if we should advance now
				else if ( NPCInfo->stats.aggression > 5 )
				{//approach enemy
					if ( TIMER_Done( NPC, "parryTime" ) || NPCInfo->rank > RANK_LT )
					{//not parrying
						if ( !NPC->enemy->client || NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
						{//they're on the ground, so advance
							if ( enemy_dist > 200 || !(NPCInfo->scriptFlags&SCF_DONT_FIRE) )
							{//far away or allowed to use saber
								Jedi_Advance();
							}
						}
					}
				}
				else
				{//maintain this distance?
					//walk?
				}
			}
			//[/CoOp]
		}
	}
	else
	{//we're not close enough to attack, but not far enough away to be safe
		//[CoOp]
		//racc - kyle tries to grab even when out of range?
		if ( !Q_irand( 0, 30 ) 
			&& Kyle_CanDoGrab() )
		{
			Kyle_TryGrab();
			return;
		}
		//[/CoOp]
		if ( NPCInfo->stats.aggression < 4 )
		{//back off and defend
			//[CoOp]
			if ( Jedi_DecideKick() )
			{//let's try a kick
				if ( G_PickAutoMultiKick( NPC, qfalse, qtrue ) != LS_NONE
					|| (G_CanKickEntity(NPC, NPC->enemy ) 
					&& G_PickAutoKick( NPC, NPC->enemy, qtrue ) != LS_NONE ) )
				{//kicked!
					TIMER_Set( NPC, "kickDebounce", Q_irand( 3000, 10000 ) );
					return;
				}
			}
			//[/CoOp]
			Jedi_Retreat();
		}
		else if ( NPCInfo->stats.aggression > 5 )
		{//try to get closer
			if ( enemy_dist > 0 && !(NPCInfo->scriptFlags&SCF_DONT_FIRE))
			{//we're allowed to use our lightsaber, get closer
				if ( TIMER_Done( NPC, "parryTime" ) || NPCInfo->rank > RANK_LT )
				{//not parrying
					if ( !NPC->enemy->client || NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE )
					{//they're on the ground, so advance
						Jedi_Advance();
					}
				}
			}
		}
		else
		{//agression is 4 or 5... somewhere in the middle
			//what do we do here?  Nothing?
			//Move forward and back?
		}
	}
	//if really really mad, rage!
	if ( NPCInfo->stats.aggression > Q_irand( 5, 15 )
		&& NPC->health < NPC->client->pers.maxHealth*0.75f 
		&& !Q_irand( 0, 2 ) )
	{
		if ( (NPC->client->ps.fd.forcePowersKnown&(1<<FP_RAGE)) != 0 
			&& (NPC->client->ps.fd.forcePowersActive&(1<<FP_RAGE)) == 0 )
		{
			Jedi_Rage();
		}
	}
}

static qboolean Jedi_Strafe( int strafeTimeMin, int strafeTimeMax, int nextStrafeTimeMin, int nextStrafeTimeMax, qboolean walking )
{//racc - set strafe timers.
	//[CoOp]
	if ( Jedi_CultistDestroyer( NPC ) )
	{//destroyers never strafe
		return qfalse;
	}
	//[/CoOp]
	if ( NPC->client->ps.saberEventFlags&SEF_LOCK_WON && NPC->enemy && NPC->enemy->painDebounceTime > level.time )
	{//don't strafe if pressing the advantage of winning a saberLock
		return qfalse;
	}
	if ( TIMER_Done( NPC, "strafeLeft" ) && TIMER_Done( NPC, "strafeRight" ) )
	{
		qboolean strafed = qfalse;
		//TODO: make left/right choice a tactical decision rather than random:
		//		try to keep own back away from walls and ledges, 
		//		try to keep enemy's back to a ledge or wall
		//		Maybe try to strafe toward designer-placed "safe spots" or "goals"?
		int	strafeTime = Q_irand( strafeTimeMin, strafeTimeMax );

		if ( Q_irand( 0, 1 ) )
		{
			if ( NPC_MoveDirClear( ucmd.forwardmove, -127, qfalse ) )
			{
				TIMER_Set( NPC, "strafeLeft", strafeTime );
				strafed = qtrue;
			}
			else if ( NPC_MoveDirClear( ucmd.forwardmove, 127, qfalse ) )
			{
				TIMER_Set( NPC, "strafeRight", strafeTime );
				strafed = qtrue;
			}
		}
		else
		{
			if ( NPC_MoveDirClear( ucmd.forwardmove, 127, qfalse  ) )
			{
				TIMER_Set( NPC, "strafeRight", strafeTime );
				strafed = qtrue;
			}
			else if ( NPC_MoveDirClear( ucmd.forwardmove, -127, qfalse  ) )
			{
				TIMER_Set( NPC, "strafeLeft", strafeTime );
				strafed = qtrue;
			}
		}

		if ( strafed )
		{
			TIMER_Set( NPC, "noStrafe", strafeTime + Q_irand( nextStrafeTimeMin, nextStrafeTimeMax ) );
			if ( walking )
			{//should be a slow strafe
				TIMER_Set( NPC, "walking", strafeTime );
			}
			return qtrue;
		}
	}
	return qfalse;
}

/*
static void Jedi_FaceEntity( gentity_t *self, gentity_t *other, qboolean doPitch )
{
	vec3_t		entPos;
	vec3_t		muzzle;

	//Get the positions
	CalcEntitySpot( other, SPOT_ORIGIN, entPos );

	//Get the positions
	CalcEntitySpot( self, SPOT_HEAD_LEAN, muzzle );//SPOT_HEAD

	//Find the desired angles
	vec3_t	angles;

	GetAnglesForDirection( muzzle, entPos, angles );

	self->NPC->desiredYaw		= AngleNormalize360( angles[YAW] );
	if ( doPitch )
	{
		self->NPC->desiredPitch	= AngleNormalize360( angles[PITCH] );
	}
}
*/

/*
qboolean Jedi_DodgeEvasion( gentity_t *self, gentity_t *shooter, trace_t *tr, int hitLoc )

Jedi will play a dodge anim, blur, and make the force speed noise.

Right now used to dodge instant-hit weapons.

FIXME: possibly call this for saber melee evasion and/or missile evasion?
FIXME: possibly let player do this too?
*/
//rwwFIXMEFIXME: Going to use qboolean Jedi_DodgeEvasion( gentity_t *self, gentity_t *shooter, trace_t *tr, int hitLoc ) from
//w_saber.c.. maybe use seperate one for NPCs or add cases to that one?

evasionType_t Jedi_CheckFlipEvasions( gentity_t *self, float rightdot, float zdiff )
{//racc - attempt to evade thru acromatics
	if ( self->NPC && (self->NPC->scriptFlags&SCF_NO_ACROBATICS) )
	{
		return EVASION_NONE;
	}
	//[CoOp]
	if ( self->client )
	{
		if ( self->client->NPC_class == CLASS_BOBAFETT )
		{//boba can't flip
			return EVASION_NONE;
		}
		if ( self->client->ps.fd.forceRageRecoveryTime > level.time	
			|| (self->client->ps.fd.forcePowersActive&(1<<FP_RAGE)) )
		{//no fancy dodges when raging
			return EVASION_NONE;
		}
	}
	/*
	if ( self->client 
		&& (self->client->ps.fd.forceRageRecoveryTime > level.time	|| (self->client->ps.fd.forcePowersActive&(1<<FP_RAGE))) )
	{//no fancy dodges when raging
		return EVASION_NONE;
	}
	*/
	//[/CoOp]
	//Check for:
	//ARIALS/CARTWHEELS
	//WALL-RUNS
	//WALL-FLIPS
	//FIXME: if facing a wall, do forward wall-walk-backflip
	if ( self->client->ps.legsAnim == BOTH_WALL_RUN_LEFT || self->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT )
	{//already running on a wall
		vec3_t right, fwdAngles;
		int		anim = -1;
		float animLength;

		VectorSet(fwdAngles, 0, self->client->ps.viewangles[YAW], 0);

		AngleVectors( fwdAngles, NULL, right, NULL );

		animLength = BG_AnimLength( self->localAnimIndex, (animNumber_t)self->client->ps.legsAnim );
		if ( self->client->ps.legsAnim == BOTH_WALL_RUN_LEFT && rightdot < 0 )
		{//I'm running on a wall to my left and the attack is on the left
			if ( animLength - self->client->ps.legsTimer > 400
				&& self->client->ps.legsTimer > 400 )
			{//not at the beginning or end of the anim
				anim = BOTH_WALL_RUN_LEFT_FLIP;
			}
		}
		else if ( self->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT && rightdot > 0 )
		{//I'm running on a wall to my right and the attack is on the right
			if ( animLength - self->client->ps.legsTimer > 400
				&& self->client->ps.legsTimer > 400 )
			{//not at the beginning or end of the anim
				anim = BOTH_WALL_RUN_RIGHT_FLIP;
			}
		}
		if ( anim != -1 )
		{//flip off the wall!
			int parts;
			//FIXME: check the direction we will flip towards for do-not-enter/walls/drops?
			//NOTE: we presume there is still a wall there!
			if ( anim == BOTH_WALL_RUN_LEFT_FLIP )
			{
				self->client->ps.velocity[0] *= 0.5f;
				self->client->ps.velocity[1] *= 0.5f;
				VectorMA( self->client->ps.velocity, 150, right, self->client->ps.velocity );
			}
			else if ( anim == BOTH_WALL_RUN_RIGHT_FLIP )
			{
				self->client->ps.velocity[0] *= 0.5f;
				self->client->ps.velocity[1] *= 0.5f;
				VectorMA( self->client->ps.velocity, -150, right, self->client->ps.velocity );
			}
			parts = SETANIM_LEGS;
			if ( !self->client->ps.weaponTime )
			{
				parts = SETANIM_BOTH;
			}
			NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			//self->client->ps.pm_flags |= (PMF_JUMPING|PMF_SLOW_MO_FALL);
			//rwwFIXMEFIXME: Add these pm flags?
			G_AddEvent( self, EV_JUMP, 0 );
			return EVASION_OTHER;
		}
	}
	else if ( self->client->NPC_class != CLASS_DESANN //desann doesn't do these kind of frilly acrobatics
		&& (self->NPC->rank == RANK_CREWMAN || self->NPC->rank >= RANK_LT) 
		&& Q_irand( 0, 1 ) 
		&& !BG_InRoll( &self->client->ps, self->client->ps.legsAnim )
		&& !PM_InKnockDown( &self->client->ps )
		&& !BG_SaberInSpecialAttack( self->client->ps.torsoAnim ) )
	{
		vec3_t fwd, right, traceto, mins, maxs, fwdAngles;
		trace_t	trace;
		int parts, anim;
		float	speed, checkDist;
		qboolean allowCartWheels = qtrue;
		qboolean allowWallFlips = qtrue;

		if ( self->client->ps.weapon == WP_SABER )
		{
			if ( self->client->saber[0].model
				&& self->client->saber[0].model[0]
				&& (self->client->saber[0].saberFlags&SFL_NO_CARTWHEELS) )
			{
				allowCartWheels = qfalse;
			}
			else if ( self->client->saber[1].model
				&& self->client->saber[1].model[0]
				&& (self->client->saber[1].saberFlags&SFL_NO_CARTWHEELS) )
			{
				allowCartWheels = qfalse;
			}
			if ( self->client->saber[0].model
				&& self->client->saber[0].model[0]
				&& (self->client->saber[0].saberFlags&SFL_NO_WALL_FLIPS) )
			{
				allowWallFlips = qfalse;
			}
			else if ( self->client->saber[1].model
				&& self->client->saber[1].model[0]
				&& (self->client->saber[1].saberFlags&SFL_NO_WALL_FLIPS) )
			{
				allowWallFlips = qfalse;
			}
		}

		VectorSet(mins, self->r.mins[0],self->r.mins[1],0);
		VectorSet(maxs, self->r.maxs[0],self->r.maxs[1],24);
		VectorSet(fwdAngles, 0, self->client->ps.viewangles[YAW], 0);

		AngleVectors( fwdAngles, fwd, right, NULL );

		parts = SETANIM_BOTH;

		if ( BG_SaberInAttack( self->client->ps.saberMove )
			|| PM_SaberInStart( self->client->ps.saberMove ) )
		{
			parts = SETANIM_LEGS;
		}
		if ( rightdot >= 0 )
		{
			if ( Q_irand( 0, 1 ) )
			{
				anim = BOTH_ARIAL_LEFT;
			}
			else
			{
				anim = BOTH_CARTWHEEL_LEFT;
			}
			checkDist = -128;
			speed = -200;
		}
		else
		{
			if ( Q_irand( 0, 1 ) )
			{
				anim = BOTH_ARIAL_RIGHT;
			}
			else
			{
				anim = BOTH_CARTWHEEL_RIGHT;
			}
			checkDist = 128;
			speed = 200;
		}
		//trace in the dir that we want to go
		VectorMA( self->r.currentOrigin, checkDist, right, traceto );
		trap_Trace( &trace, self->r.currentOrigin, mins, maxs, traceto, self->s.number, CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP );
		if ( trace.fraction >= 1.0f && allowCartWheels )
		{//it's clear, let's do it
			//FIXME: check for drops?
			vec3_t fwdAngles, jumpRt;

			NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			self->client->ps.weaponTime = self->client->ps.legsTimer;//don't attack again until this anim is done
			VectorCopy( self->client->ps.viewangles, fwdAngles );
			fwdAngles[PITCH] = fwdAngles[ROLL] = 0;
			//do the flip
			AngleVectors( fwdAngles, NULL, jumpRt, NULL );
			VectorScale( jumpRt, speed, self->client->ps.velocity );
			self->client->ps.fd.forceJumpCharge = 0;//so we don't play the force flip anim
			self->client->ps.velocity[2] = 200;
			self->client->ps.fd.forceJumpZStart = self->r.currentOrigin[2];//so we don't take damage if we land at same height
			//self->client->ps.pm_flags |= PMF_JUMPING;
			//[CoOp]
			if ( self->client->NPC_class == CLASS_BOBAFETT
				|| (self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER))
			//if ( self->client->NPC_class == CLASS_BOBAFETT )
			//[/CoOp]
			{
				G_AddEvent( self, EV_JUMP, 0 );
			}
			else
			{
				G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/jump.wav" );
			}
			//ucmd.upmove = 0;
			return EVASION_CARTWHEEL;
		}
		else if ( !(trace.contents&CONTENTS_BOTCLIP) )
		{//hit a wall, not a do-not-enter brush
			//FIXME: before we check any of these jump-type evasions, we should check for headroom, right?
			//Okay, see if we can flip *off* the wall and go the other way
			vec3_t	idealNormal;
			gentity_t *traceEnt;

			VectorSubtract( self->r.currentOrigin, traceto, idealNormal );
			VectorNormalize( idealNormal );
			traceEnt = &g_entities[trace.entityNum];
			if ( (trace.entityNum<ENTITYNUM_WORLD&&traceEnt&&traceEnt->s.solid!=SOLID_BMODEL) || DotProduct( trace.plane.normal, idealNormal ) > 0.7f )
			{//it's a ent of some sort or it's a wall roughly facing us
				float bestCheckDist = 0;
				//hmm, see if we're moving forward
				if ( DotProduct( self->client->ps.velocity, fwd ) < 200 )
				{//not running forward very fast
					//check to see if it's okay to move the other way
					if ( (trace.fraction*checkDist) <= 32 )
					{//wall on that side is close enough to wall-flip off of or wall-run on
						bestCheckDist = checkDist;
						checkDist *= -1.0f;
						VectorMA( self->r.currentOrigin, checkDist, right, traceto );
						//trace in the dir that we want to go
						trap_Trace( &trace, self->r.currentOrigin, mins, maxs, traceto, self->s.number, CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP );
						if ( trace.fraction >= 1.0f )
						{//it's clear, let's do it
							if ( allowWallFlips )
							{//okay to do wall-flips with this saber
								int parts;

								//FIXME: check for drops?
								//turn the cartwheel into a wallflip in the other dir
								if ( rightdot > 0 )
								{
									anim = BOTH_WALL_FLIP_LEFT;
									self->client->ps.velocity[0] = self->client->ps.velocity[1] = 0;
									VectorMA( self->client->ps.velocity, 150, right, self->client->ps.velocity );
								}
								else
								{
									anim = BOTH_WALL_FLIP_RIGHT;
									self->client->ps.velocity[0] = self->client->ps.velocity[1] = 0;
									VectorMA( self->client->ps.velocity, -150, right, self->client->ps.velocity );
								}
								self->client->ps.velocity[2] = forceJumpStrength[FORCE_LEVEL_2]/2.25f;
								//animate me
								parts = SETANIM_LEGS;
								if ( !self->client->ps.weaponTime )
								{
									parts = SETANIM_BOTH;
								}
								NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
								self->client->ps.fd.forceJumpZStart = self->r.currentOrigin[2];//so we don't take damage if we land at same height
								//self->client->ps.pm_flags |= (PMF_JUMPING|PMF_SLOW_MO_FALL);
								//[CoOp]
								if ( self->client->NPC_class == CLASS_BOBAFETT 
									|| (self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER))
								//if ( self->client->NPC_class == CLASS_BOBAFETT )
								//[/CoOp]
								{
									G_AddEvent( self, EV_JUMP, 0 );
								}
								else
								{
									G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/jump.wav" );
								}
								return EVASION_OTHER;
								}
						}
						else 
						{//boxed in on both sides
							if ( DotProduct( self->client->ps.velocity, fwd ) < 0 )
							{//moving backwards
								return EVASION_NONE;
							}
							if ( (trace.fraction*checkDist) <= 32 && (trace.fraction*checkDist) < bestCheckDist )
							{
								bestCheckDist = checkDist;
							}
						}
					}
					else
					{//too far from that wall to flip or run off it, check other side
						checkDist *= -1.0f;
						VectorMA( self->r.currentOrigin, checkDist, right, traceto );
						//trace in the dir that we want to go
						trap_Trace( &trace, self->r.currentOrigin, mins, maxs, traceto, self->s.number, CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP );
						if ( (trace.fraction*checkDist) <= 32 )
						{//wall on this side is close enough
							bestCheckDist = checkDist;
						}
						else
						{//neither side has a wall within 32
							return EVASION_NONE;
						}
					}
				}
				//Try wall run?
				if ( bestCheckDist )
				{//one of the walls was close enough to wall-run on
					qboolean allowWallRuns = qtrue;
					if ( self->client->ps.weapon == WP_SABER )
					{
						if ( self->client->saber[0].model
							&& self->client->saber[0].model[0] 
							&& (self->client->saber[0].saberFlags&SFL_NO_WALL_RUNS) )
						{
							allowWallRuns = qfalse;
						}
						else if ( self->client->saber[1].model
							&& self->client->saber[1].model[0]
							&& (self->client->saber[1].saberFlags&SFL_NO_WALL_RUNS) )
						{
							allowWallRuns = qfalse;
						}
					}
					if ( allowWallRuns )
					{//okay to do wallruns with this saber
						int parts;

						//FIXME: check for long enough wall and a drop at the end?
						if ( bestCheckDist > 0 )
						{//it was to the right
							anim = BOTH_WALL_RUN_RIGHT;
						}
						else
						{//it was to the left
							anim = BOTH_WALL_RUN_LEFT;
						}
						self->client->ps.velocity[2] = forceJumpStrength[FORCE_LEVEL_2]/2.25f;
						//animate me
						parts = SETANIM_LEGS;
						if ( !self->client->ps.weaponTime )
						{
							parts = SETANIM_BOTH;
						}
						NPC_SetAnim( self, parts, anim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
						self->client->ps.fd.forceJumpZStart = self->r.currentOrigin[2];//so we don't take damage if we land at same height
						//self->client->ps.pm_flags |= (PMF_JUMPING|PMF_SLOW_MO_FALL);
						//[CoOp]
						if ( self->client->NPC_class == CLASS_BOBAFETT 
							|| (self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER))
						//if ( self->client->NPC_class == CLASS_BOBAFETT )
						//[/CoOp]
						{
							G_AddEvent( self, EV_JUMP, 0 );
						}
						else
						{
							G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/jump.wav" );
						}
						return EVASION_OTHER;
					}
				}
				//else check for wall in front, do backflip off wall
			}
		}
	}
	return EVASION_NONE;
}

int Jedi_ReCalcParryTime( gentity_t *self, evasionType_t evasionType )
{//racc - calculates a new parry time.
	if ( !self->client )
	{
		return 0;
	}
	if ( !self->s.number )
	{//player 
		return bg_parryDebounce[self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE]];
	}
	else if ( self->NPC )
	{
		//[CoOp]
		/*  Not used in SP version
		if ( !g_saberRealisticCombat.integer 
			&& ( g_spskill.integer == 2 || (g_spskill.integer == 1 && self->client->NPC_class == CLASS_TAVION) ) )
		{
			if ( self->client->NPC_class == CLASS_TAVION )
			{
				return 0;
			}
			else
			{
				return Q_irand( 0, 150 );
			}
		}
		else
		*/
		//[/CoOp]
		{
			int	baseTime;
			if ( evasionType == EVASION_DODGE )
			{
				baseTime = self->client->ps.torsoTimer;
			}
			else if ( evasionType == EVASION_CARTWHEEL )
			{
				baseTime = self->client->ps.torsoTimer;
			}
			else if ( self->client->ps.saberInFlight )
			{
				baseTime = Q_irand( 1, 3 ) * 50;
			}
			else
			{
				//[CoOp]
				//don't have different baseTime for g_saberRealisticCombat
				if ( 1 ) 
				//if ( g_saberRealisticCombat.integer )
				//[/CoOp]
				{
					baseTime = 500;

					switch ( g_spskill.integer )
					{
					case 0:
						//[CoOp] - SP value
						baseTime = 400;//was 500
						//baseTime = 500;
						//[/CoOp]
						break;
					case 1:
						//[CoOp]
						//SP value
						baseTime = 200;//was 300
						//baseTime = 300;
						//[/CoOp]
						break;
					case 2:
					default:

						baseTime = 100;
						break;
					}
				}
				else
				{
					baseTime = 150;//500;

					switch ( g_spskill.integer )
					{
					case 0:
						baseTime = 200;//500;
						break;
					case 1:
						baseTime = 100;//300;
						break;
					case 2:
					default:
						baseTime = 50;//100;
						break;
					}
				}

				//[CoOp]
				//Tavion & Alora & shadowtroopers are faster 
				if ( self->client->NPC_class == CLASS_ALORA 
					|| self->client->NPC_class == CLASS_SHADOWTROOPER
					|| self->client->NPC_class == CLASS_TAVION )
				//if ( self->client->NPC_class == CLASS_TAVION )
				//[/CoOp]
				{//Tavion is faster
					baseTime = ceil(baseTime/2.0f);
				}
				else if ( self->NPC->rank >= RANK_LT_JG )
				{//fencers, bosses, shadowtroopers, luke, desann, et al use the norm
					if ( Q_irand( 0, 2 ) )
					{//medium speed parry
						baseTime = baseTime;
					}
					else
					{//with the occasional fast parry
						baseTime = ceil(baseTime/2.0f);
					}
				}
				else if ( self->NPC->rank == RANK_CIVILIAN )
				{//grunts are slowest
					baseTime = baseTime*Q_irand(1,3);
				}
				else if ( self->NPC->rank == RANK_CREWMAN )
				{//acrobats aren't so bad
					if ( evasionType == EVASION_PARRY
						|| evasionType == EVASION_DUCK_PARRY
						|| evasionType == EVASION_JUMP_PARRY )
					{//slower with parries
						baseTime = baseTime*Q_irand(1,2);
					}
					else
					{//faster with acrobatics
						//baseTime = baseTime;
					}
				}
				else
				{//force users are kinda slow
					baseTime = baseTime*Q_irand(1,2);
				}
				if ( evasionType == EVASION_DUCK || evasionType == EVASION_DUCK_PARRY )
				{
					//[CoOp] - SP value
					baseTime += 250;//300;//100;
					//baseTime += 100;
					//[/CoOp]
				}
				else if ( evasionType == EVASION_JUMP || evasionType == EVASION_JUMP_PARRY )
				{
					//[CoOp] - SP value
					baseTime += 400;//500;//50;
					//baseTime += 50;
					//[/CoOp]
				}
				else if ( evasionType == EVASION_OTHER )
				{
					//[CoOp] - SP value
					baseTime += 50;//100;
					//baseTime += 100;
					//[/CoOp]
				}
				else if ( evasionType == EVASION_FJUMP )
				{
					//[CoOp] - SP value
					baseTime += 300;//400;//100;
					//baseTime += 100;
					//[/CoOp]
				}
			}
			
			return baseTime;
		}
	}
	return 0;
}

qboolean Jedi_QuickReactions( gentity_t *self )
{//racc - determines if this NPC do saber blocks during operations that probably won't let you
	//do so normally.
	if ( ( self->client->NPC_class == CLASS_JEDI && NPCInfo->rank == RANK_COMMANDER ) ||
		//[CoOp]
		//shadowtroopers & tavion & alora have quick reactions.
		self->client->NPC_class == CLASS_SHADOWTROOPER || 
		self->client->NPC_class == CLASS_ALORA || 
		self->client->NPC_class == CLASS_TAVION ||
		//self->client->NPC_class == CLASS_TAVION ||
		//[/CoOp]
		(self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE]>FORCE_LEVEL_1&&g_spskill.integer>1) ||
		(self->client->ps.fd.forcePowerLevel[FP_SABER_DEFENSE]>FORCE_LEVEL_2&&g_spskill.integer>0) )
	{
		return qtrue;
	}
	return qfalse;
}

qboolean Jedi_SaberBusy( gentity_t *self )
{
	if ( self->client->ps.torsoTimer > 300 
	&& ( (BG_SaberInAttack( self->client->ps.saberMove )&&self->client->ps.fd.saberAnimLevel==FORCE_LEVEL_3) 
		|| BG_SpinningSaberAnim( self->client->ps.torsoAnim ) 
		|| BG_SaberInSpecialAttack( self->client->ps.torsoAnim ) 
		//|| PM_SaberInBounce( self->client->ps.saberMove ) 
		|| PM_SaberInBrokenParry( self->client->ps.saberMove ) 
		//|| PM_SaberInDeflect( self->client->ps.saberMove ) 
		|| BG_FlippingAnim( self->client->ps.torsoAnim ) 
		|| PM_RollingAnim( self->client->ps.torsoAnim ) ) )
	{//my saber is not in a parrying position
		return qtrue;
	}
	return qfalse;
}


//[CoOp]
extern qboolean PM_InRollIgnoreTimer( playerState_t *ps );
extern qboolean PM_InAirKickingAnim( int anim );
extern qboolean BG_StabDownAnim( int anim );
extern qboolean BG_SuperBreakLoseAnim( int anim );
extern qboolean BG_SuperBreakWinAnim( int anim );
qboolean Jedi_InNoAIAnim( gentity_t *self )
{//Don't block or do any AI processing during these animations.
	if ( !self || !self->client )
	{//wtf???
		return qtrue;
	}

	//[Test]
	//SetNPCGlobals( self );
	//[/Test]

	if ( self->NPC->rank >= RANK_COMMANDER )
	{//boss-level guys can multitask, the rest need to chill out during special moves
		return qfalse;
	}

	if ( BG_KickingAnim( self->client->ps.legsAnim )
		||BG_StabDownAnim( self->client->ps.legsAnim )
		||PM_InAirKickingAnim( self->client->ps.legsAnim )
		||PM_InRollIgnoreTimer( &self->client->ps )
		||BG_SaberInKata((saberMoveName_t)self->client->ps.saberMove)
		||BG_SuperBreakWinAnim( self->client->ps.torsoAnim )
		||BG_SuperBreakLoseAnim( self->client->ps.torsoAnim ) )
	{
		return qtrue;
	}

	switch ( self->client->ps.legsAnim )
	{
	case BOTH_BUTTERFLY_LEFT:
	case BOTH_BUTTERFLY_RIGHT:
	case BOTH_BUTTERFLY_FL1:
	case BOTH_BUTTERFLY_FR1:
	case BOTH_FLIP_F:			
	case BOTH_FLIP_B:			
	case BOTH_FLIP_L:			
	case BOTH_FLIP_R:			
	case BOTH_DODGE_FL:			
	case BOTH_DODGE_FR:			
	case BOTH_DODGE_BL:			
	case BOTH_DODGE_BR:			
	case BOTH_DODGE_L:			
	case BOTH_DODGE_R:			
	case BOTH_DODGE_HOLD_FL:
	case BOTH_DODGE_HOLD_FR:
	case BOTH_DODGE_HOLD_BL:
	case BOTH_DODGE_HOLD_BR:
	case BOTH_DODGE_HOLD_L:
	case BOTH_DODGE_HOLD_R:
	case BOTH_FORCEWALLRUNFLIP_START:
	case BOTH_JUMPATTACK6:
	case BOTH_JUMPATTACK7:
	case BOTH_JUMPFLIPSLASHDOWN1:
	case BOTH_JUMPFLIPSTABDOWN:
	case BOTH_FORCELEAP2_T__B_:
	case BOTH_ROLL_STAB:
	case BOTH_SPINATTACK6:
	case BOTH_SPINATTACK7:
	case BOTH_PULL_IMPALE_STAB:
	case BOTH_PULL_IMPALE_SWING:
	case BOTH_A6_FB:
	case BOTH_A6_LR:
	case BOTH_A7_HILT:
		return qtrue;
		break;
	}
	return qfalse;
}

extern qboolean FlyingCreature( gentity_t *ent );
extern qboolean NAV_DirSafe( gentity_t *self, vec3_t dir, float dist );
extern qboolean NAV_MoveDirSafe( gentity_t *self, usercmd_t *cmd, float distScale );
void Jedi_CheckJumpEvasionSafety( gentity_t *self, usercmd_t *cmd, evasionType_t evasionType )
{//checks to make sure that the non-flip evasion are safe to do.
	if ( evasionType != EVASION_OTHER//not a FlipEvasion, which does it's own safety checks
		&& self->client->ps.groundEntityNum != ENTITYNUM_NONE ) 
	{//on terra firma right now
		if ( self->client->ps.velocity[2] > 0 
			|| self->client->ps.fd.forceJumpCharge 
			|| cmd->upmove > 0 )
		{//going to jump
			if ( !NAV_MoveDirSafe( self, cmd, self->client->ps.speed*10.0f ) )
			{//we can't jump in the dir we're pushing in
				//cancel the evasion
				self->client->ps.velocity[2] = self->client->ps.fd.forceJumpCharge = 0;
				cmd->upmove = 0;
				if ( d_JediAI.integer )
				{
					Com_Printf( S_COLOR_RED"jump not safe, cancelling!" );
				}
			}
			else if ( self->client->ps.velocity[0] || self->client->ps.velocity[1] )
			{//sliding
				vec3_t jumpDir;
				float jumpDist = VectorNormalize2( self->client->ps.velocity, jumpDir );
				if ( !NAV_DirSafe( self, jumpDir, jumpDist ) )
				{//this jump combined with our momentum would send us into a do not enter brush, so cancel it
					//cancel the evasion
					self->client->ps.velocity[2] = self->client->ps.fd.forceJumpCharge = 0;
					cmd->upmove = 0;
					if ( d_JediAI.integer )
					{
						Com_Printf( S_COLOR_RED"jump not safe, cancelling!\n" );
					}
				}
			}
			if ( d_JediAI.integer )
			{
				Com_Printf( S_COLOR_GREEN"jump checked, is safe\n" );
			}
		}
	}
}
//[/CoOp]


/*
-------------------------
Jedi_SaberBlock

Pick proper block anim

FIXME: Based on difficulty level/enemy saber combat skill, make this decision-making more/less effective

NOTE: always blocking projectiles in this func!

-------------------------
*/
//[CoOp]
extern qboolean PM_LockedAnim( int anim );
//[/CoOp]
extern qboolean G_FindClosestPointOnLineSegment( const vec3_t start, const vec3_t end, const vec3_t from, vec3_t result );
evasionType_t Jedi_SaberBlockGo( gentity_t *self, usercmd_t *cmd, vec3_t pHitloc, vec3_t phitDir, gentity_t *incoming, float dist ) //dist = 0.0f
{//racc - find the best evade, block, etc for this attack.  This doesn't nessicarily mean that 
	//the move is actually committed.
	vec3_t hitloc, hitdir, diff, fwdangles={0,0,0}, right;
	float rightdot;
	float zdiff;
	int	  duckChance = 0;
	int	  dodgeAnim = -1;
	//[CoOp]
	qboolean alwaysDodgeOrRoll = qfalse;
	qboolean doRoll = qfalse;
	//[/CoOp]
	qboolean	saberBusy = qfalse, evaded = qfalse, doDodge = qfalse;
	evasionType_t	evasionType = EVASION_NONE;

	//[CoOp]
	if ( !self || !self->client )
	{//sanity check
		return EVASION_NONE;
	}

	if ( PM_LockedAnim( self->client->ps.torsoAnim ) 
		&& self->client->ps.torsoTimer )
	{//Never interrupt these...
		return EVASION_NONE;
	}
	if ( BG_InSpecialJump( self->client->ps.legsAnim )
		&& BG_SaberInSpecialAttack( self->client->ps.torsoAnim ) )
	{
		return EVASION_NONE;
	}

	if ( Jedi_InNoAIAnim( self ) )
	{
		return EVASION_NONE;
	}
	//[/CoOp]

	//FIXME: if we don't have our saber in hand, pick the force throw option or a jump or strafe!
	//FIXME: reborn don't block enough anymore
	if ( !incoming )
	{
		VectorCopy( pHitloc, hitloc );
		VectorCopy( phitDir, hitdir );
		//FIXME: maybe base this on rank some?  And/or g_spskill?
		if ( self->client->ps.saberInFlight )
		{//DOH!  do non-saber evasion!
			saberBusy = qtrue;
		}
		//[CoOp]
		/*
		else if ( Jedi_QuickReactions( self ) )
		{//jedi trainer and tavion are must faster at parrying and can do it whenever they like
			//Also, on medium, all level 3 people can parry any time and on hard, all level 2 or 3 people can parry any time
		}
		*/
		//[/CoOp]
		else
		{
			saberBusy = Jedi_SaberBusy( self );
		}
	}
	else
	{	
		if ( incoming->s.weapon == WP_SABER )
		{//flying lightsaber, face it!
			//FIXME: for this to actually work, we'd need to call update angles too?
			//Jedi_FaceEntity( self, incoming, qtrue );
		}
		VectorCopy( incoming->r.currentOrigin, hitloc );
		VectorNormalize2( incoming->s.pos.trDelta, hitdir );
	}

	//[CoOp] - this isn't in the SP code
	/*
	if ( self->client && self->client->NPC_class == CLASS_BOBAFETT )
	{
		saberBusy = qtrue;
	}
	*/
	//[/CoOp]

	VectorSubtract( hitloc, self->client->renderInfo.eyePoint, diff );
	diff[2] = 0;
	//VectorNormalize( diff );
	fwdangles[1] = self->client->ps.viewangles[1];
	// Ultimately we might care if the shot was ahead or behind, but for now, just quadrant is fine.
	AngleVectors( fwdangles, NULL, right, NULL );

	rightdot = DotProduct(right, diff);// + flrand(-0.10f,0.10f);
	//totalHeight = self->client->renderInfo.eyePoint[2] - self->r.absmin[2];
	zdiff = hitloc[2] - self->client->renderInfo.eyePoint[2];// + Q_irand(-6,6);

	//[CoOp]
	if ( self->client->NPC_class == CLASS_BOBAFETT )
	{
		saberBusy = qtrue;
		doDodge = qtrue;
		alwaysDodgeOrRoll = qtrue;
	}
	else
	{
		if ( self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER )
		{
			saberBusy = qtrue;
			alwaysDodgeOrRoll = qtrue;
		}
		//see if we can dodge if need-be
		if ( (dist>16&&(Q_irand( 0, 2 )||saberBusy)) 
			|| self->client->ps.saberInFlight 
			|| BG_SabersOff( &self->client->ps )
			|| (self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER) )
			//|| self->client->NPC_class == CLASS_BOBAFETT )
		{//either it will miss by a bit (and 25% chance) OR our saber is not in-hand OR saber is off
			if ( self->NPC && (self->NPC->rank == RANK_CREWMAN || self->NPC->rank >= RANK_LT_JG) )
			{//acrobat or fencer or above
				if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE &&//on the ground
					!(self->client->ps.pm_flags&PMF_DUCKED)&&cmd->upmove>=0&&TIMER_Done( self, "duck" )//not ducking
					&& !BG_InRoll( &self->client->ps, self->client->ps.legsAnim )//not rolling
					&& !PM_InKnockDown( &self->client->ps )//not knocked down
					&& ( self->client->ps.saberInFlight ||
						(self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER) ||
						//self->client->NPC_class == CLASS_BOBAFETT ||
						(!BG_SaberInAttack( self->client->ps.saberMove )//not attacking
						&& !PM_SaberInStart( self->client->ps.saberMove )//not starting an attack
						&& !BG_SpinningSaberAnim( self->client->ps.torsoAnim )//not in a saber spin
						&& !BG_SaberInSpecialAttack( self->client->ps.torsoAnim ))//not in a special attack
						)
					)
				{//need to check all these because it overrides both torso and legs with the dodge
					doDodge = qtrue;
				}
			}
		}
	}

	if (	( self->client->NPC_class == CLASS_BOBAFETT //boba fett
				|| (self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER) //non-saber reborn (cultist)
			)
			&& !Q_irand( 0, 2 ) 
		)
	{
		doRoll = qtrue;
	}
	//[/CoOp]
	// Figure out what quadrant the block was in.
	if ( d_JediAI.integer )
	{
		Com_Printf( "(%d) evading attack from height %4.2f, zdiff: %4.2f, rightdot: %4.2f\n", level.time, hitloc[2]-self->r.absmin[2],zdiff,rightdot);
	}

	//UL = > -1//-6
	//UR = > -6//-9
	//TOP = > +6//+4
	//FIXME: take FP_SABER_DEFENSE into account here somehow?
	if ( zdiff >= -5 )//was 0
	{//racc - close to head level
		//[CoOp]
		if ( incoming || !saberBusy || alwaysDodgeOrRoll )
		//if ( incoming || !saberBusy )
		//[/CoOp]
		{
			if ( rightdot > 12 
				|| (rightdot > 3 && zdiff < 5) 
				|| (!incoming&&fabs(hitdir[2])<0.25f) )//was normalized, 0.3
			{//coming from right
				if ( doDodge )
				{
					//[CoOp]
					if ( doRoll )
					//if ( self->client->NPC_class == CLASS_BOBAFETT && !Q_irand( 0, 2 ) )
					//[/CoOp]
					{//roll!
						TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
						TIMER_Start( self, "strafeLeft", Q_irand( 500, 1500 ) );
						TIMER_Set( self, "strafeRight", 0 );
						evasionType = EVASION_DUCK;
						evaded = qtrue;
					}
					else if ( Q_irand( 0, 1 ) )
					{
						dodgeAnim = BOTH_DODGE_FL;
					}
					else
					{
						dodgeAnim = BOTH_DODGE_BL;
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
					evasionType = EVASION_PARRY;
					if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
					{
						if ( zdiff > 5 )
						{
							TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
							evasionType = EVASION_DUCK_PARRY;
							evaded = qtrue;
							if ( d_JediAI.integer )
							{
								Com_Printf( "duck " );
							}
						}
						else
						{
							duckChance = 6;
						}
					}
				}
				if ( d_JediAI.integer )
				{
					Com_Printf( "UR block\n" );
				}
			}
			else if ( rightdot < -12 
				|| (rightdot < -3 && zdiff < 5) 
				|| (!incoming&&fabs(hitdir[2])<0.25f) )//was normalized, -0.3
			{//coming from left
				if ( doDodge )
				{
					//[CoOp]
					if ( doRoll )
					//if ( self->client->NPC_class == CLASS_BOBAFETT && !Q_irand( 0, 2 ) )
					//[/CoOp]
					{//roll!
						TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
						TIMER_Start( self, "strafeRight", Q_irand( 500, 1500 ) );
						TIMER_Set( self, "strafeLeft", 0 );
						evasionType = EVASION_DUCK;
						evaded = qtrue;
					}
					else if ( Q_irand( 0, 1 ) )
					{
						dodgeAnim = BOTH_DODGE_FR;
					}
					else
					{
						dodgeAnim = BOTH_DODGE_BR;
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
					evasionType = EVASION_PARRY;
					if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
					{
						if ( zdiff > 5 )
						{
							TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
							evasionType = EVASION_DUCK_PARRY;
							evaded = qtrue;
							if ( d_JediAI.integer )
							{
								Com_Printf( "duck " );
							}
						}
						else
						{
							duckChance = 6;
						}
					}
				}
				if ( d_JediAI.integer )
				{
					Com_Printf( "UL block\n" );
				}
			}
			else
			{
				self->client->ps.saberBlocked = BLOCKED_TOP;
				evasionType = EVASION_PARRY;
				if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{
					duckChance = 4;
				}
				if ( d_JediAI.integer )
				{
					Com_Printf( "TOP block\n" );
				}
			}
			evaded = qtrue;
		}
		else
		{
			if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
			{
				//duckChance = 2;
				TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
				evasionType = EVASION_DUCK;
				evaded = qtrue;
				if ( d_JediAI.integer )
				{
					Com_Printf( "duck " );
				}
			}
		}
	}
	//LL = -22//= -18 to -39
	//LR = -23//= -20 to -41
	else if ( zdiff > -22 )//was-15 )
	{//racc - torso level
		if ( 1 )//zdiff < -10 )
		{//hmm, pretty low, but not low enough to use the low block, so we need to duck
			if ( self->client->ps.groundEntityNum != ENTITYNUM_NONE )
			{
				//duckChance = 2;
				TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
				evasionType = EVASION_DUCK;
				evaded = qtrue;
				if ( d_JediAI.integer )
				{
					Com_Printf( "duck " );
				}
			}
			else
			{//in air!  Ducking does no good
			}
		}
		//[CoOp]
		if ( incoming || !saberBusy || alwaysDodgeOrRoll )
		//if ( incoming || !saberBusy )
		//[/CoOp]
		{
			if ( rightdot > 8 || (rightdot > 3 && zdiff < -11) )//was normalized, 0.2
			{
				if ( doDodge )
				{
					//[CoOp]
					if ( doRoll )
					//if ( self->client->NPC_class == CLASS_BOBAFETT && !Q_irand( 0, 2 ) )
					//[/CoOp]
					{//roll!
						TIMER_Start( self, "strafeLeft", Q_irand( 500, 1500 ) );
						TIMER_Set( self, "strafeRight", 0 );
					}
					else 
					{
						dodgeAnim = BOTH_DODGE_L;
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_UPPER_RIGHT;
					if ( evasionType == EVASION_DUCK )
					{
						evasionType = EVASION_DUCK_PARRY;
					}
					else
					{
						evasionType = EVASION_PARRY;
					}
				}
				if ( d_JediAI.integer )
				{
					Com_Printf( "mid-UR block\n" );
				}
			}
			else if ( rightdot < -8 || (rightdot < -3 && zdiff < -11) )//was normalized, -0.2
			{
				if ( doDodge )
				{
					//[CoOp]
					if ( doRoll )
					//if ( self->client->NPC_class == CLASS_BOBAFETT && !Q_irand( 0, 2 ) )
					//[/CoOp]
					{//roll!
						TIMER_Start( self, "strafeLeft", Q_irand( 500, 1500 ) );
						TIMER_Set( self, "strafeRight", 0 );
					}
					else 
					{
						dodgeAnim = BOTH_DODGE_R;
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_UPPER_LEFT;
					if ( evasionType == EVASION_DUCK )
					{
						evasionType = EVASION_DUCK_PARRY;
					}
					else
					{
						evasionType = EVASION_PARRY;
					}
				}
				if ( d_JediAI.integer )
				{
					Com_Printf( "mid-UL block\n" );
				}
			}
			else
			{
				self->client->ps.saberBlocked = BLOCKED_TOP;
				if ( evasionType == EVASION_DUCK )
				{
					evasionType = EVASION_DUCK_PARRY;
				}
				else
				{
					evasionType = EVASION_PARRY;
				}
				if ( d_JediAI.integer )
				{
					Com_Printf( "mid-TOP block\n" );
				}
			}
			evaded = qtrue;
		}
	}
	//[CoOp]
	else 
	{//at the legs
		if ( saberBusy || (zdiff < -36 && ( zdiff < -44 || !Q_irand( 0, 2 ) ) ) )//was -30 and -40//2nd one was -46
	//else if ( saberBusy || (zdiff < -36 && ( zdiff < -44 || !Q_irand( 0, 2 ) ) ) )//was -30 and -40//2nd one
		{//jump!
			if ( self->client->ps.groundEntityNum == ENTITYNUM_NONE )
			{//already in air, duck to pull up legs
				TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
				evasionType = EVASION_DUCK;
				evaded = qtrue;
				if ( d_JediAI.integer )
				{
					Com_Printf( "legs up\n" );
				}
				if ( incoming || !saberBusy )
				{
					//since the jump may be cleared if not safe, set a lower block too
					if ( rightdot >= 0 )
					{
						self->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
						evasionType = EVASION_DUCK_PARRY;
						if ( d_JediAI.integer )
						{
							Com_Printf( "LR block\n" );
						}
					}
					else
					{
						self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
						evasionType = EVASION_DUCK_PARRY;
						if ( d_JediAI.integer )
						{
							Com_Printf( "LL block\n" );
						}
					}
					evaded = qtrue;
				}
			}
			else 
			{//gotta jump!
				if ( self->NPC && (self->NPC->rank == RANK_CREWMAN || self->NPC->rank > RANK_LT_JG ) &&
					(!Q_irand( 0, 10 ) || (!Q_irand( 0, 2 ) && (cmd->forwardmove || cmd->rightmove))) )
				{//superjump
					//FIXME: check the jump, if can't, then block
					if ( self->NPC 
						&& !(self->NPC->scriptFlags&SCF_NO_ACROBATICS) 
						&& self->client->ps.fd.forceRageRecoveryTime < level.time
						&& !(self->client->ps.fd.forcePowersActive&(1<<FP_RAGE)) 
						&& !PM_InKnockDown( &self->client->ps ) )
					{
						self->client->ps.fd.forceJumpCharge = 320;//FIXME: calc this intelligently
						evasionType = EVASION_FJUMP;
						evaded = qtrue;
						if ( d_JediAI.integer )
						{
							Com_Printf( "force jump + " );
						}
					}
				}
				else
				{//normal jump
					//FIXME: check the jump, if can't, then block
					if ( self->NPC 
						&& !(self->NPC->scriptFlags&SCF_NO_ACROBATICS) 
						&& self->client->ps.fd.forceRageRecoveryTime < level.time
						&& !(self->client->ps.fd.forcePowersActive&(1<<FP_RAGE)) )
					{
						//[CoOp]
						if ( self->client->NPC_class == CLASS_BOBAFETT 
							|| (self->client->NPC_class == CLASS_REBORN && self->s.weapon != WP_SABER))
						//if ( self->client->NPC_class == CLASS_BOBAFETT && !Q_irand( 0, 1 ) )
						//[/CoOp]
						{//flip!
							if ( rightdot > 0 )
							{
								TIMER_Start( self, "strafeLeft", Q_irand( 500, 1500 ) );
								TIMER_Set( self, "strafeRight", 0 );
								TIMER_Set( self, "walking", 0 );
							}
							else
							{
								TIMER_Start( self, "strafeRight", Q_irand( 500, 1500 ) );
								TIMER_Set( self, "strafeLeft", 0 );
								TIMER_Set( self, "walking", 0 );
							}
						}
						else
						{
							if ( self == NPC )
							{
								cmd->upmove = 127;
							}
							else
							{
								self->client->ps.velocity[2] = JUMP_VELOCITY;
							}
						}
						evasionType = EVASION_JUMP;
						evaded = qtrue;
						if ( d_JediAI.integer )
						{
							Com_Printf( "jump + " );
						}
					}
					//[CoOp]
					if ( self->client->NPC_class == CLASS_ALORA 
						|| self->client->NPC_class == CLASS_SHADOWTROOPER
						|| self->client->NPC_class == CLASS_TAVION )
					//if ( self->client->NPC_class == CLASS_TAVION )
					//[/CoOp]
					{
						if ( !incoming 
							&& self->client->ps.groundEntityNum < ENTITYNUM_NONE 
							&& !Q_irand( 0, 2 ) )
						{
							if ( !BG_SaberInAttack( self->client->ps.saberMove )
								&& !PM_SaberInStart( self->client->ps.saberMove ) 
								&& !BG_InRoll( &self->client->ps, self->client->ps.legsAnim )
								&& !PM_InKnockDown( &self->client->ps )
								&& !BG_SaberInSpecialAttack( self->client->ps.torsoAnim ) )
							{//do the butterfly!
								int butterflyAnim;
								//[CoOp]
								if ( self->client->NPC_class == CLASS_ALORA 
									&& !Q_irand( 0, 2 ) )
								{//alora has a special butterfly move.
									butterflyAnim = BOTH_ALORA_SPIN;
								}
								else if ( Q_irand( 0, 1 ) )
								//if ( Q_irand( 0, 1 ) )
								//[/CoOp]
								{
									butterflyAnim = BOTH_BUTTERFLY_LEFT;
								}
								else
								{
									butterflyAnim = BOTH_BUTTERFLY_RIGHT;
								}
								evasionType = EVASION_CARTWHEEL;
								NPC_SetAnim( self, SETANIM_BOTH, butterflyAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
								self->client->ps.velocity[2] = 225;
								self->client->ps.fd.forceJumpZStart = self->r.currentOrigin[2];//so we don't take damage if we land at same height
							//	self->client->ps.pm_flags |= PMF_JUMPING|PMF_SLOW_MO_FALL;
							//RAFIXME: impliment this?
							//	self->client->ps.SaberActivateTrail( 300 );//FIXME: reset this when done!
								//Ah well. No hacking from the server for now.
								//[CoOp]
								/*
								if ( self->client->NPC_class == CLASS_BOBAFETT )
								{
									G_AddEvent( self, EV_JUMP, 0 );
								}
								else
								*/
								{
									G_SoundOnEnt( self, CHAN_BODY, "sound/weapons/force/jump.wav" );
									//G_Sound( self, CHAN_BODY, G_SoundIndex("sound/weapons/force/jump.wav") );
								//[/CoOp]
								}
								cmd->upmove = 0;
								saberBusy = qtrue;
								evaded = qtrue;
							}
						}
					}
				}
				if ( ((evasionType = Jedi_CheckFlipEvasions( self, rightdot, zdiff ))!=EVASION_NONE) )
				{//racc - if the flip is valid
					if ( d_slowmodeath.integer > 5 && self->enemy && !self->enemy->s.number )
					{
						G_StartMatrixEffect( self );
					}
					saberBusy = qtrue;
					evaded = qtrue;
				}
				else if ( incoming || !saberBusy )
				{//racc - ok, let's try a saber block then.
					//since the jump may be cleared if not safe, set a lower block too
					if ( rightdot >= 0 )
					{
						self->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
						if ( evasionType == EVASION_JUMP )
						{
							evasionType = EVASION_JUMP_PARRY;
						}
						else if ( evasionType == EVASION_NONE )
						{
							evasionType = EVASION_PARRY;
						}
						if ( d_JediAI.integer )
						{
							Com_Printf( "LR block\n" );
						}
					}
					else
					{
						self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
						if ( evasionType == EVASION_JUMP )
						{
							evasionType = EVASION_JUMP_PARRY;
						}
						else if ( evasionType == EVASION_NONE )
						{
							evasionType = EVASION_PARRY;
						}
						if ( d_JediAI.integer )
						{
							Com_Printf( "LL block\n" );
						}
					}
					evaded = qtrue;
				}
			}
		}
		else
		{//saber is busy or the attack is very low
			if ( incoming || !saberBusy )
			{//try blocking
				if ( rightdot >= 0 )
				{
					self->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
					evasionType = EVASION_PARRY;
					if ( d_JediAI.integer )
					{
						Com_Printf( "LR block\n" );
					}
				}
				else
				{
					self->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
					evasionType = EVASION_PARRY;
					if ( d_JediAI.integer )
					{
						Com_Printf( "LL block\n" );
					}
				}
				if ( incoming && incoming->s.weapon == WP_SABER )
				{//thrown saber!
					if ( self->NPC && (self->NPC->rank == RANK_CREWMAN || self->NPC->rank > RANK_LT_JG ) &&
						(!Q_irand( 0, 10 ) || (!Q_irand( 0, 2 ) && (cmd->forwardmove || cmd->rightmove))) )
					{//superjump
						//FIXME: check the jump, if can't, then block
						if ( self->NPC 
							&& !(self->NPC->scriptFlags&SCF_NO_ACROBATICS) 
							&& self->client->ps.fd.forceRageRecoveryTime < level.time
							&& !(self->client->ps.fd.forcePowersActive&(1<<FP_RAGE)) 
							&& !PM_InKnockDown( &self->client->ps ) )
						{
							self->client->ps.fd.forceJumpCharge = 320;//FIXME: calc this intelligently
							evasionType = EVASION_FJUMP;
							if ( d_JediAI.integer )
							{
								Com_Printf( "force jump + " );
							}
						}
					}
					else
					{//normal jump
						//FIXME: check the jump, if can't, then block
						if ( self->NPC 
							&& !(self->NPC->scriptFlags&SCF_NO_ACROBATICS) 
							&& self->client->ps.fd.forceRageRecoveryTime < level.time
							&& !(self->client->ps.fd.forcePowersActive&(1<<FP_RAGE)))
						{
							if ( self == NPC )
							{
								cmd->upmove = 127;
							}
							else
							{
								self->client->ps.velocity[2] = JUMP_VELOCITY;
							}
							evasionType = EVASION_JUMP_PARRY;
							if ( d_JediAI.integer )
							{
								Com_Printf( "jump + " );
							}
						}
					}
				}
				evaded = qtrue;
			}
		}
	}
	//[/CoOp]
	if ( evasionType == EVASION_NONE )
	{
		return EVASION_NONE;
	}
//[CoOp]
//=======================================================================================
	//see if it's okay to jump
	Jedi_CheckJumpEvasionSafety( self, cmd, evasionType );
//=======================================================================================
//[/CoOp]
	//stop taunting
	TIMER_Set( self, "taunting", 0 );
	//stop gripping
	TIMER_Set( self, "gripping", -level.time );
	WP_ForcePowerStop( self, FP_GRIP );
	//stop draining
	TIMER_Set( self, "draining", -level.time );
	WP_ForcePowerStop( self, FP_DRAIN );

	if ( dodgeAnim != -1 )
	{//dodged
		evasionType = EVASION_DODGE;
		NPC_SetAnim( self, SETANIM_BOTH, dodgeAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		self->client->ps.weaponTime = self->client->ps.torsoTimer;
		//force them to stop moving in this case
		self->client->ps.pm_time = self->client->ps.torsoTimer;
		//FIXME: maybe make a sound?  Like a grunt?  EV_JUMP?
		self->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		//dodged, not block
		if ( d_slowmodeath.integer > 5 && self->enemy && !self->enemy->s.number )
		{
			G_StartMatrixEffect( self );
		}
	}
	else
	{
		if ( duckChance )
		{
			if ( !Q_irand( 0, duckChance ) )
			{
				TIMER_Start( self, "duck", Q_irand( 500, 1500 ) );
				if ( evasionType == EVASION_PARRY )
				{
					evasionType = EVASION_DUCK_PARRY;
				}
				else
				{
					evasionType = EVASION_DUCK;
				}
				/*
				if ( d_JediAI.integer )
				{
					Com_Printf( "duck " );
				}
				*/
			}
		}

		if ( incoming )
		{
			self->client->ps.saberBlocked = WP_MissileBlockForBlock( self->client->ps.saberBlocked );
		}

	}
	//if ( self->client->ps.saberBlocked != BLOCKED_NONE )
	{
		int parryReCalcTime = Jedi_ReCalcParryTime( self, evasionType );
		if ( self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] < level.time + parryReCalcTime )
		{
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + parryReCalcTime;
		}
	}
	return evasionType;
}


//[CoOp]
extern float WP_SaberLength( gentity_t *ent );
static evasionType_t Jedi_CheckEvadeSpecialAttacks( void )
{//evade special attacks
	if ( !NPC
		|| !NPC->client )
	{
		return EVASION_NONE;
	}

	if ( !NPC->enemy
		|| NPC->enemy->health <= 0 
		|| !NPC->enemy->client )
	{//don't keep blocking him once he's dead (or if not a client)
		return EVASION_NONE;
	}

	if ( NPC->enemy->s.number >= MAX_CLIENTS )
	{//only do these against player
		return EVASION_NONE;
	}

	if ( !TIMER_Done( NPC, "specialEvasion" ) )
	{//still evading from last time
		return EVASION_NONE;
	}

	if ( NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK6
		|| NPC->enemy->client->ps.torsoAnim == BOTH_SPINATTACK7 )
	{//back away from these
		if ( (NPCInfo->aiFlags&NPCAI_BOSS_CHARACTER)
			|| NPC->client->NPC_class == CLASS_SHADOWTROOPER
			|| NPC->client->NPC_class == CLASS_ALORA
			|| Q_irand( 0, NPCInfo->rank ) > RANK_LT_JG ) 
		{//see if we should back off
			if ( InFront( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin, NPC->enemy->r.currentAngles, 0.0f ) )
			{//facing me
				float minSafeDistSq = (NPC->r.maxs[0]*1.5f+NPC->enemy->r.maxs[0]*1.5f+WP_SaberLength(NPC->enemy)+24.0f);
				minSafeDistSq *= minSafeDistSq; 
				if ( DistanceSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin ) < minSafeDistSq )
				{//back off!
					Jedi_StartBackOff();
					return EVASION_OTHER;
				}
			}
		}
	}
	else
	{//check some other attacks?
		//check roll-stab
		if ( NPC->enemy->client->ps.torsoAnim == BOTH_ROLL_STAB
			|| (NPC->enemy->client->ps.torsoAnim == BOTH_ROLL_F && ((NPCInfo->last_ucmd.buttons&BUTTON_ATTACK)
			/* RAFIXME:  is this implimented? ||(NPC->enemy->client->ps.pm_flags&PMF_ATTACK_HELD)*/) ) )
		{//either already in a roll-stab or may go into one
			if ( (NPCInfo->aiFlags&NPCAI_BOSS_CHARACTER)
				|| NPC->client->NPC_class == CLASS_SHADOWTROOPER
				|| NPC->client->NPC_class == CLASS_ALORA
				|| Q_irand( -3, NPCInfo->rank ) > RANK_LT_JG )
			{//see if we should evade
				vec3_t yawOnlyAngles;
				VectorSet(yawOnlyAngles, 0, NPC->enemy->r.currentAngles[YAW], 0); 
				if ( InFront( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin, yawOnlyAngles, 0.25f ) )
				{//facing me
					float distSq;
					float minSafeDistSq = (NPC->r.maxs[0]*1.5f+NPC->enemy->r.maxs[0]*1.5f+WP_SaberLength(NPC->enemy)+24.0f);
					minSafeDistSq *= minSafeDistSq;
					distSq = DistanceSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin );
					if ( distSq < minSafeDistSq )
					{//evade!
						qboolean doJump = ( NPC->enemy->client->ps.torsoAnim == BOTH_ROLL_STAB || distSq < 3000.0f );//not much time left, just jump!
						if ( (NPCInfo->scriptFlags&SCF_NO_ACROBATICS)
							|| !doJump )
						{//roll?
							vec3_t enemyRight, dir2Me;
							float dot;

							AngleVectors( yawOnlyAngles, NULL, enemyRight, NULL );
							VectorSubtract( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin, dir2Me );
							VectorNormalize( dir2Me );
							dot = DotProduct( enemyRight, dir2Me );

							ucmd.forwardmove = 0;
							TIMER_Start( NPC, "duck", Q_irand( 500, 1500 ) );
							ucmd.upmove = -127;
							//NOTE: this *assumes* I'm facing him!
							if ( dot > 0 )
							{//I'm to his right
								if ( !NPC_MoveDirClear( 0, -127, qfalse  ) )
								{//fuck, jump instead
									doJump = qtrue;
								}
								else
								{//roll to the left.
									TIMER_Start( NPC, "strafeLeft", Q_irand( 500, 1500 ) );
									TIMER_Set( NPC, "strafeRight", 0 );
									ucmd.rightmove = -127;
									if ( d_JediAI.integer )
									{
										Com_Printf( "%s rolling left from roll-stab!\n", NPC->NPC_type );
									}
									if ( NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
									{//fuck it, just force it
										NPC_SetAnim(NPC,SETANIM_BOTH,BOTH_ROLL_L,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
										G_AddEvent( NPC, EV_ROLL, 0 );
										NPC->client->ps.saberMove = LS_NONE;
									}
								}
							}
							else
							{//I'm to his left
								if ( !NPC_MoveDirClear( 0, 127, qfalse  ) )
								{//fuck, jump instead
									doJump = qtrue;
								}
								else
								{//roll to the right
									TIMER_Start( NPC, "strafeRight", Q_irand( 500, 1500 ) );
									TIMER_Set( NPC, "strafeLeft", 0 );
									ucmd.rightmove = 127;
									if ( d_JediAI.integer )
									{
										Com_Printf( "%s rolling right from roll-stab!\n", NPC->NPC_type );
									}
									if ( NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
									{//fuck it, just force it
										NPC_SetAnim(NPC,SETANIM_BOTH,BOTH_ROLL_R,SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
										G_AddEvent( NPC, EV_ROLL, 0 );
										NPC->client->ps.saberMove = LS_NONE;
									}
								}
							}
							if ( !doJump )
							{
								TIMER_Set( NPC, "specialEvasion", 3000 );
								return EVASION_DUCK;
							}
						}
						//didn't roll, do jump
						if ( NPC->s.weapon != WP_SABER
							|| (NPCInfo->aiFlags&NPCAI_BOSS_CHARACTER)
							|| NPC->client->NPC_class == CLASS_SHADOWTROOPER
							|| NPC->client->NPC_class == CLASS_ALORA
							|| Q_irand( -3, NPCInfo->rank ) > RANK_CREWMAN )
						{//superjump
							NPC->client->ps.fd.forceJumpCharge = 320;//FIXME: calc this intelligently
							if ( Q_irand( 0, 2 ) )
							{//make it a backflip
								ucmd.forwardmove = -127;
								TIMER_Set( NPC, "roamTime", -level.time );
								TIMER_Set( NPC, "strafeLeft", -level.time );
								TIMER_Set( NPC, "strafeRight", -level.time );
								TIMER_Set( NPC, "walking", -level.time );
								TIMER_Set( NPC, "moveforward", -level.time );
								TIMER_Set( NPC, "movenone", -level.time );
								TIMER_Set( NPC, "moveright", -level.time );
								TIMER_Set( NPC, "moveleft", -level.time );
								TIMER_Set( NPC, "movecenter", -level.time );
								TIMER_Set( NPC, "moveback", Q_irand( 500, 1000 ) );
								if ( d_JediAI.integer )
								{
									Com_Printf( "%s backflipping from roll-stab!\n", NPC->NPC_type );
								}
							}
							else
							{
								if ( d_JediAI.integer )
								{
									Com_Printf( "%s force-jumping over roll-stab!\n", NPC->NPC_type );
								}
							}
							TIMER_Set( NPC, "specialEvasion", 3000 );
							return EVASION_FJUMP;
						}
						else
						{//normal jump
							ucmd.upmove = 127;
							if ( d_JediAI.integer )
							{
								Com_Printf( "%s jumping over roll-stab!\n", NPC->NPC_type );
							}
							TIMER_Set( NPC, "specialEvasion", 2000 );
							return EVASION_JUMP;
						}
					}
				}
			}
		}
	}
	return EVASION_NONE;
}
//[/CoOp]

extern float ShortestLineSegBewteen2LineSegs( vec3_t start1, vec3_t end1, vec3_t start2, vec3_t end2, vec3_t close_pnt1, vec3_t close_pnt2 );
extern int WPDEBUG_SaberColor( saber_colors_t saberColor );
//[CoOp]
static qboolean Jedi_SaberBlock( void )
//static qboolean Jedi_SaberBlock( int saberNum, int bladeNum ) //saberNum = 0, bladeNum = 0
//[/CoOp]
{//Block/evade attacks from an incoming saber strike from your enemy.
	vec3_t hitloc, saberTipOld, saberTip, top, bottom, axisPoint, saberPoint, dir;//saberBase, 
	vec3_t pointDir, baseDir, tipDir, saberHitPoint, saberMins, saberMaxs;
	float	pointDist, baseDirPerc, dist;
	//[CoOp]
	float bestDist = Q3_INFINITE;
	int		saberNum = 0, bladeNum = 0;
	int		closestSaberNum = 0, closestBladeNum = 0;
	//float	bladeLen = 0;
	//[/CoOp]
	trace_t	tr;
	evasionType_t	evasionType;

	//FIXME: reborn don't block enough anymore
	/*
	//maybe do this on easy only... or only on grunt-level reborn
	if ( NPC->client->ps.weaponTime )
	{//i'm attacking right now
		return qfalse;
	}
	*/

	if ( !TIMER_Done( NPC, "parryReCalcTime" ) )
	{//can't do our own re-think of which parry to use yet
		return qfalse;
	}

	if ( NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] > level.time )
	{//can't move the saber to another position yet
		return qfalse;
	}
	
	/*
	if ( NPCInfo->rank < RANK_LT_JG && Q_irand( 0, (2 - g_spskill.integer) ) )
	{//lower rank reborn have a random chance of not doing it at all
		NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 300;
		return qfalse;
	}
	*/

	if ( NPC->enemy->health <= 0 || !NPC->enemy->client )
	{//don't keep blocking him once he's dead (or if not a client)
		return qfalse;
	}
	/*
	//VectorMA( NPC->enemy->client->renderInfo.muzzlePoint, NPC->enemy->client->ps.saberLength, NPC->enemy->client->renderInfo.muzzleDir, saberTip );
	//VectorMA( NPC->enemy->client->renderInfo.muzzlePointNext, NPC->enemy->client->ps.saberLength, NPC->enemy->client->renderInfo.muzzleDirNext, saberTipNext );
	VectorMA( NPC->enemy->client->renderInfo.muzzlePointOld, NPC->enemy->client->ps.saberLength, NPC->enemy->client->renderInfo.muzzleDirOld, saberTipOld );
	VectorMA( NPC->enemy->client->renderInfo.muzzlePoint, NPC->enemy->client->ps.saberLength, NPC->enemy->client->renderInfo.muzzleDir, saberTip );

	VectorSubtract( NPC->enemy->client->renderInfo.muzzlePoint, NPC->enemy->client->renderInfo.muzzlePointOld, dir );//get the dir
	VectorAdd( dir, NPC->enemy->client->renderInfo.muzzlePoint, saberBase );//extrapolate

	VectorSubtract( saberTip, saberTipOld, dir );//get the dir
	VectorAdd( dir, saberTip, saberTipOld );//extrapolate

	VectorCopy( NPC->r.currentOrigin, top );
	top[2] = NPC->r.absmax[2];
	VectorCopy( NPC->r.currentOrigin, bottom );
	bottom[2] = NPC->r.absmin[2];

	float dist = ShortestLineSegBewteen2LineSegs( saberBase, saberTipOld, bottom, top, saberPoint, axisPoint );
	if ( 0 )//dist > NPC->r.maxs[0]*4 )//was *3
	{//FIXME: sometimes he reacts when you're too far away to actually hit him
		if ( d_JediAI.integer )
		{
			Com_Printf( "enemy saber dist: %4.2f\n", dist );
		}
		TIMER_Set( NPC, "parryTime", -1 );
		return qfalse;
	}

	//get the actual point of impact
	trace_t	tr;
	trap_Trace( &tr, saberPoint, vec3_origin, vec3_origin, axisPoint, NPC->enemy->s.number, MASK_SHOT, G2_RETURNONHIT, 10 );
	if ( tr.allsolid || tr.startsolid )
	{//estimate
		VectorSubtract( saberPoint, axisPoint, dir );
		VectorNormalize( dir );
		VectorMA( axisPoint, NPC->r.maxs[0]*1.22, dir, hitloc );
	}
	else
	{
		VectorCopy( tr.endpos, hitloc );
	}
	*/
	VectorSet(saberMins,-4,-4,-4);
	VectorSet(saberMaxs,4,4,4);

	//[CoOp]
	//FIXME: need to check against both sabers/blades now!...?

	for ( saberNum = 0; saberNum < MAX_SABERS; saberNum++ )
	{
		for ( bladeNum = 0; bladeNum < NPC->enemy->client->saber[saberNum].numBlades; bladeNum++ )
		{
			if ( NPC->enemy->client->saber[saberNum].type != SABER_NONE
				&& NPC->enemy->client->saber[saberNum].blade[bladeNum].length > 0 )
			{//valid saber and this blade is on
				VectorMA( NPC->enemy->client->saber[saberNum].blade[bladeNum].muzzlePointOld, NPC->enemy->client->saber[saberNum].blade[bladeNum].length, NPC->enemy->client->saber[saberNum].blade[bladeNum].muzzleDirOld, saberTipOld );
				VectorMA( NPC->enemy->client->saber[saberNum].blade[bladeNum].muzzlePoint, NPC->enemy->client->saber[saberNum].blade[bladeNum].length, NPC->enemy->client->saber[saberNum].blade[bladeNum].muzzleDir, saberTip );

				VectorCopy( NPC->r.currentOrigin, top );
				top[2] = NPC->r.absmax[2];
				VectorCopy( NPC->r.currentOrigin, bottom );
				bottom[2] = NPC->r.absmin[2];

				dist = ShortestLineSegBewteen2LineSegs( NPC->enemy->client->saber[saberNum].blade[bladeNum].muzzlePoint, saberTip, bottom, top, saberPoint, axisPoint );
				if ( dist < bestDist )
				{
					bestDist = dist;
					closestSaberNum = saberNum;
					closestBladeNum = bladeNum;
				}
			}
		}
	}

	if ( bestDist > NPC->r.maxs[0]*5 )//was *3
	{//FIXME: sometimes he reacts when you're too far away to actually hit him
		if ( d_JediAI.integer )
		{
			Com_Printf( S_COLOR_RED"enemy saber dist: %4.2f\n", bestDist );
		}
		TIMER_Set( NPC, "parryTime", -1 );
		return qfalse;
	}

		dist = bestDist;
	
	if ( d_JediAI.integer )
	{
		Com_Printf( S_COLOR_GREEN"enemy saber dist: %4.2f\n", dist );
	}
	
	//now use the closest blade for my evasion check
	VectorMA( NPC->enemy->client->saber[closestSaberNum].blade[closestBladeNum].muzzlePointOld, NPC->enemy->client->saber[closestSaberNum].blade[closestBladeNum].length, NPC->enemy->client->saber[closestSaberNum].blade[closestBladeNum].muzzleDirOld, saberTipOld );
	VectorMA( NPC->enemy->client->saber[closestSaberNum].blade[closestBladeNum].muzzlePoint, NPC->enemy->client->saber[closestSaberNum].blade[closestBladeNum].length, NPC->enemy->client->saber[closestSaberNum].blade[closestBladeNum].muzzleDir, saberTip );

	//VectorMA( NPC->enemy->client->saber[saberNum].blade[bladeNum].muzzlePointOld, NPC->enemy->client->saber[saberNum].blade[bladeNum].length, NPC->enemy->client->saber[saberNum].blade[bladeNum].muzzleDirOld, saberTipOld );
	//VectorMA( NPC->enemy->client->saber[saberNum].blade[bladeNum].muzzlePoint, NPC->enemy->client->saber[saberNum].blade[bladeNum].length, NPC->enemy->client->saber[saberNum].blade[bladeNum].muzzleDir, saberTip );
	//[/CoOp]
//	VectorCopy(NPC->enemy->client->lastSaberBase_Always, muzzlePoint);
//	VectorMA(muzzlePoint, GAME_SABER_LENGTH, NPC->enemy->client->lastSaberDir_Always, saberTip);
//	VectorCopy(saberTip, saberTipOld);

	VectorCopy( NPC->r.currentOrigin, top );
	top[2] = NPC->r.absmax[2];
	VectorCopy( NPC->r.currentOrigin, bottom );
	bottom[2] = NPC->r.absmin[2];

	//[CoOp]
	/*
	dist = ShortestLineSegBewteen2LineSegs( NPC->enemy->client->renderInfo.muzzlePoint, saberTip, bottom, top, saberPoint, axisPoint );
	if ( dist > NPC->r.maxs[0]*5 )//was *3
	{//FIXME: sometimes he reacts when you're too far away to actually hit him
		if ( d_JediAI.integer )
		{
			Com_Printf( S_COLOR_RED"enemy saber dist: %4.2f\n", dist );
		}
		/*
		if ( dist < 300 //close
			&& !Jedi_QuickReactions( NPC )//quick reaction people can interrupt themselves
			&& (PM_SaberInStart( NPC->enemy->client->ps.saberMove ) || BG_SaberInAttack( NPC->enemy->client->ps.saberMove )) )//enemy is swinging at me
		{//he's swinging at me and close enough to be a threat, don't start an attack right now
			TIMER_Set( NPC, "parryTime", 100 );
		}
		else
		*/
	/*
		{
			TIMER_Set( NPC, "parryTime", -1 );
		}
		return qfalse;
	}
	if ( d_JediAI.integer )
	{
		Com_Printf( S_COLOR_GREEN"enemy saber dist: %4.2f\n", dist );
	}
	*/
	
	//[CoOp]
	dist = ShortestLineSegBewteen2LineSegs( 
		NPC->enemy->client->saber[closestSaberNum].blade[closestBladeNum].muzzlePoint, 
		saberTip, bottom, top, saberPoint, axisPoint );
	VectorSubtract( saberPoint, 
		NPC->enemy->client->saber[closestSaberNum].blade[closestBladeNum].muzzlePoint, 
		pointDir );
	//VectorSubtract( saberPoint, NPC->enemy->client->renderInfo.muzzlePoint, pointDir );
	//[/CoOp]
	pointDist = VectorLength( pointDir );
	
	//[CoOp]
	if ( NPC->enemy->client->saber[closestSaberNum].blade[closestBladeNum].length <= 0 )
	{
		baseDirPerc = 0.5f;
	}
	else
	{
		baseDirPerc = pointDist/
			NPC->enemy->client->saber[closestSaberNum].blade[closestBladeNum].length;
	}
	/*
	bladeLen = NPC->enemy->client->saber[saberNum].blade[bladeNum].length;

	if ( bladeLen <= 0 )
	{
		baseDirPerc = 0.5f;
	}
	else
	{
		baseDirPerc = pointDist/bladeLen;
	}
	*/
	
	VectorSubtract( 
		NPC->enemy->client->saber[closestSaberNum].blade[closestBladeNum].muzzlePoint, 
		NPC->enemy->client->saber[closestSaberNum].blade[closestBladeNum].muzzlePointOld, 
		baseDir );
	//VectorSubtract( NPC->enemy->client->renderInfo.muzzlePoint, NPC->enemy->client->renderInfo.muzzlePointOld, baseDir );
	//[/CoOp]
	VectorSubtract( saberTip, saberTipOld, tipDir );
	VectorScale( baseDir, baseDirPerc, baseDir );
	VectorMA( baseDir, 1.0f-baseDirPerc, tipDir, dir );
	VectorMA( saberPoint, 200, dir, hitloc );

	//get the actual point of impact
	trap_Trace( &tr, saberPoint, saberMins, saberMaxs, hitloc, NPC->enemy->s.number, CONTENTS_BODY );//, G2_RETURNONHIT, 10 );
	if ( tr.allsolid || tr.startsolid || tr.fraction >= 1.0f )
	{//estimate
		vec3_t	dir2Me;
		VectorSubtract( axisPoint, saberPoint, dir2Me );
		dist = VectorNormalize( dir2Me );
		if ( DotProduct( dir, dir2Me ) < 0.2f )
		{//saber is not swinging in my direction
			/*
			if ( dist < 300 //close
				&& !Jedi_QuickReactions( NPC )//quick reaction people can interrupt themselves
				&& (PM_SaberInStart( NPC->enemy->client->ps.saberMove ) || BG_SaberInAttack( NPC->enemy->client->ps.saberMove )) )//enemy is swinging at me
			{//he's swinging at me and close enough to be a threat, don't start an attack right now
				TIMER_Set( NPC, "parryTime", 100 );
			}
			else
			*/
			{
				TIMER_Set( NPC, "parryTime", -1 );
			}
			return qfalse;
		}
		ShortestLineSegBewteen2LineSegs( saberPoint, hitloc, bottom, top, saberHitPoint, hitloc );
		/*
		VectorSubtract( saberPoint, axisPoint, dir );
		VectorNormalize( dir );
		VectorMA( axisPoint, NPC->r.maxs[0]*1.22, dir, hitloc );
		*/
	}
	else
	{
		VectorCopy( tr.endpos, hitloc );
	}
	
	if ( d_JediAI.integer )
	{
		//G_DebugLine( saberPoint, hitloc, FRAMETIME, WPDEBUG_SaberColor( NPC->enemy->client->ps.saber[saberNum].blade[bladeNum].color ), qtrue );
		G_TestLine(saberPoint, hitloc, 0x0000ff, FRAMETIME);
	}

	//FIXME: if saber is off and/or we have force speed and want to be really cocky, 
	//		and the swing misses by some amount, we can use the dodges here... :)
	//[CoOp]
	if ( (evasionType=Jedi_SaberBlockGo( NPC, &ucmd, hitloc, dir, NULL, dist )) != EVASION_NONE )
	{//did some sort of evasion
	//if ( (evasionType=Jedi_SaberBlockGo( NPC, &ucmd, hitloc, dir, NULL, dist )) != EVASION_DODGE )
	//{//we did block (not dodge)
	//[/CoOp]
		//[CoOp]
		//int parryReCalcTime;
		if ( evasionType != EVASION_DODGE )
		{//(not dodge)
			int parryReCalcTime;
			if ( !NPC->client->ps.saberInFlight )
			{//make sure saber is on
				WP_ActivateSaber(NPC);
			}

			//debounce our parry recalc time
			parryReCalcTime = Jedi_ReCalcParryTime( NPC, evasionType );
			TIMER_Set( NPC, "parryReCalcTime", Q_irand( 0, parryReCalcTime ) );
			if ( d_JediAI.integer )
			{
				Com_Printf( "Keep parry choice until: %d\n", level.time + parryReCalcTime );
			}

			//determine how long to hold this anim
			if ( TIMER_Done( NPC, "parryTime" ) )
			{
				//[CoOp]
				if ( NPC->client->NPC_class == CLASS_TAVION 
					|| NPC->client->NPC_class == CLASS_SHADOWTROOPER
					|| NPC->client->NPC_class == CLASS_ALORA )
				//if ( NPC->client->NPC_class == CLASS_TAVION )
				//[/CoOp]
				{//higher level characters hold for less time.
					TIMER_Set( NPC, "parryTime", Q_irand( parryReCalcTime/2, parryReCalcTime*1.5 ) );
				}
				else if ( NPCInfo->rank >= RANK_LT_JG )
				{//fencers and higher hold a parry less
					TIMER_Set( NPC, "parryTime", parryReCalcTime );
				}
				else
				{//others hold it longer
					TIMER_Set( NPC, "parryTime", Q_irand( 1, 2 )*parryReCalcTime );
				}
			}
		}
		else
		{//dodged.
			int dodgeTime = NPC->client->ps.torsoTimer;
			if ( NPCInfo->rank > RANK_LT_COMM && NPC->client->NPC_class != CLASS_DESANN )
			{//higher-level guys can dodge faster
				dodgeTime -= 200;
			}
			TIMER_Set( NPC, "parryReCalcTime", dodgeTime );
			TIMER_Set( NPC, "parryTime", dodgeTime );
		}
	//[CoOp]
	}
	if ( evasionType != EVASION_DUCK_PARRY
		&& evasionType != EVASION_JUMP_PARRY
		&& evasionType != EVASION_JUMP
		&& evasionType != EVASION_DUCK
		&& evasionType != EVASION_FJUMP )
	{
		if ( Jedi_CheckEvadeSpecialAttacks() != EVASION_NONE )
		{//got a new evasion!
			//see if it's okay to jump 
			Jedi_CheckJumpEvasionSafety( NPC, &ucmd, evasionType );
		}
	}
	//[/CoOp]
	return qtrue;
}
/*
-------------------------
Jedi_EvasionSaber

defend if other is using saber and attacking me!
-------------------------
*/
static void Jedi_EvasionSaber( vec3_t enemy_movedir, float enemy_dist, vec3_t enemy_dir )
{
	vec3_t	dirEnemy2Me;
	int		evasionChance = 30;//only step aside 30% if he's moving at me but not attacking
	qboolean	enemy_attacking = qfalse;
	qboolean	throwing_saber = qfalse;
	qboolean	shooting_lightning = qfalse;

	if ( !NPC->enemy->client )
	{
		return;
	}
	else if ( NPC->enemy->client 
		&& NPC->enemy->s.weapon == WP_SABER 
		&& NPC->enemy->client->ps.saberLockTime > level.time )
	{//don't try to block/evade an enemy who is in a saberLock
		return;
	}
	else if ( (NPC->client->ps.saberEventFlags&SEF_LOCK_WON)
		&& NPC->enemy->painDebounceTime > level.time )
	{//pressing the advantage of winning a saber lock
		return;
	}

	if ( NPC->enemy->client->ps.saberInFlight && !TIMER_Done( NPC, "taunting" ) )
	{//if he's throwing his saber, stop taunting
		TIMER_Set( NPC, "taunting", -level.time );
		if ( !NPC->client->ps.saberInFlight )
		{
			WP_ActivateSaber(NPC);
		}
	}

	if ( TIMER_Done( NPC, "parryTime" ) )
	{
		if ( NPC->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
			NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
		{//wasn't blocked myself
			NPC->client->ps.saberBlocked = BLOCKED_NONE;
		}
	}

	if ( NPC->enemy->client->ps.weaponTime && NPC->enemy->client->ps.weaponstate == WEAPON_FIRING )
	{//our enemy attacking with their saber
		//[CoOp]
		if ( (!NPC->client->ps.saberInFlight || //their saber isn't in the air
			(NPC->client->saber[1].model && NPC->client->saber[1].model[0]
			&& NPC->client->ps.saberHolstered != 2) ) //or they still have their second saber
			&& Jedi_SaberBlock() )
		//if ( !NPC->client->ps.saberInFlight && Jedi_SaberBlock(0, 0) )
		{//blocked/evaded
			return;
		}
	}
	else if ( Jedi_CheckEvadeSpecialAttacks() != EVASION_NONE )
	{//succeeded at evading a special attack
		return;
	}

	VectorSubtract( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin, dirEnemy2Me );
	VectorNormalize( dirEnemy2Me );

	if ( NPC->enemy->client->ps.weaponTime && NPC->enemy->client->ps.weaponstate == WEAPON_FIRING )
	{//enemy is attacking
		enemy_attacking = qtrue;
		evasionChance = 90;
	}

	if ( (NPC->enemy->client->ps.fd.forcePowersActive&(1<<FP_LIGHTNING) ) )
	{//enemy is shooting lightning
		enemy_attacking = qtrue;
		shooting_lightning = qtrue;
		evasionChance = 50;
	}

	if ( NPC->enemy->client->ps.saberInFlight 
		&& NPC->enemy->client->ps.saberEntityNum != ENTITYNUM_NONE 
		&& NPC->enemy->client->ps.saberEntityState != SES_RETURNING )
	{//enemy is shooting lightning
		enemy_attacking = qtrue;
		throwing_saber = qtrue;
	}

	//FIXME: this needs to take skill and rank(reborn type) into account much more
	if ( Q_irand( 0, 100 ) < evasionChance )
	{//check to see if he's coming at me
		float facingAmt;
		if ( VectorCompare( enemy_movedir, vec3_origin ) || shooting_lightning || throwing_saber )
		{//he's not moving (or he's using a ranged attack), see if he's facing me
			vec3_t	enemy_fwd;
			AngleVectors( NPC->enemy->client->ps.viewangles, enemy_fwd, NULL, NULL );
			facingAmt = DotProduct( enemy_fwd, dirEnemy2Me );
		}
		else
		{//he's moving
			facingAmt = DotProduct( enemy_movedir, dirEnemy2Me );
		}
			
		if ( flrand( 0.25, 1 ) < facingAmt )
		{//coming at/facing me!  
			int whichDefense = 0;
			if ( NPC->client->ps.weaponTime 
				|| NPC->client->ps.saberInFlight 
				//[CoOp]
				|| NPC->client->NPC_class == CLASS_BOBAFETT 
				|| (NPC->client->NPC_class == CLASS_REBORN && NPC->s.weapon != WP_SABER)
				|| NPC->client->NPC_class == CLASS_ROCKETTROOPER )
				//|| NPC->client->NPC_class == CLASS_BOBAFETT )
				//[/CoOp]
			{//I'm attacking or recovering from a parry, can only try to strafe/jump right now
				if ( Q_irand( 0, 10 ) < NPCInfo->stats.aggression )
				{
					return;
				}
				whichDefense = 100;
			}
			else
			{
				if ( shooting_lightning )
				{//check for lightning attack
					//only valid defense is strafe and/or jump
					whichDefense = 100;
				}
				else if ( throwing_saber )
				{//he's thrown his saber!  See if it's coming at me
					float	saberDist;
					vec3_t	saberDir2Me;
					vec3_t	saberMoveDir;
					gentity_t *saber = &g_entities[NPC->enemy->client->ps.saberEntityNum];
					VectorSubtract( NPC->r.currentOrigin, saber->r.currentOrigin, saberDir2Me );
					saberDist = VectorNormalize( saberDir2Me );
					VectorCopy( saber->s.pos.trDelta, saberMoveDir );
					VectorNormalize( saberMoveDir );
					if ( !Q_irand( 0, 3 ) )
					{//getting a saber tossed in my general direction pisses me off.
						//Com_Printf( "(%d) raise agg - enemy threw saber\n", level.time );
						Jedi_Aggression( NPC, 1 );
					}
					if ( DotProduct( saberMoveDir, saberDir2Me ) > 0.5 )
					{//it's heading towards me
						if ( saberDist < 100 )
						{//it's close
							whichDefense = Q_irand( 3, 6 );
						}
						else if ( saberDist < 200 )
						{//got some time, yet, try pushing
							whichDefense = Q_irand( 0, 8 );
						}
					}
				}
				if ( whichDefense )
				{//already chose one
				}
				else if ( enemy_dist > 80 || !enemy_attacking )
				{//he's pretty far, or not swinging, just strafe
					if ( VectorCompare( enemy_movedir, vec3_origin ) )
					{//if he's not moving, not swinging and far enough away, no evasion necc.
						return;
					}
					if ( Q_irand( 0, 10 ) < NPCInfo->stats.aggression )
					{
						return;
					}
					whichDefense = 100;
				}
				else
				{//he's getting close and swinging at me
					vec3_t	fwd;
					//see if I'm facing him
					AngleVectors( NPC->client->ps.viewangles, fwd, NULL, NULL );
					if ( DotProduct( enemy_dir, fwd ) < 0.5 )
					{//I'm not really facing him, best option is to strafe
						whichDefense = Q_irand( 5, 16 );
					}
					else if ( enemy_dist < 56 )
					{//he's very close, maybe we should be more inclined to block or throw
						whichDefense = Q_irand( NPCInfo->stats.aggression, 12 );
					}
					else
					{
						whichDefense = Q_irand( 2, 16 );
					}
				}
			}

			if ( whichDefense >= 4 && whichDefense <= 12 )
			{//would try to block
				if ( NPC->client->ps.saberInFlight )
				{//can't, saber in not in hand, so fall back to strafe/jump
					whichDefense = 100;
				}
			}

			switch( whichDefense )
			{
			case 0:
			case 1:
			case 2:
			case 3:
				//[CoOp]
				//use jedi force push? or kick?
				//FIXME: try to do this if health low or enemy back to a cliff?
				if ( Jedi_DecideKick()//let's try a kick
					&& ( G_PickAutoMultiKick( NPC, qfalse, qtrue ) != LS_NONE 
						|| (G_CanKickEntity(NPC, NPC->enemy )
						&&G_PickAutoKick( NPC, NPC->enemy, qtrue )!=LS_NONE) ) )
				{//kicked
					TIMER_Set( NPC, "kickDebounce", Q_irand( 3000, 10000 ) );
				}
				else if ( (NPCInfo->rank == RANK_ENSIGN || NPCInfo->rank > RANK_LT_JG) && TIMER_Done( NPC, "parryTime" ) )
				//if ( (NPCInfo->rank == RANK_ENSIGN || NPCInfo->rank > RANK_LT_JG) && TIMER_Done( NPC, "parryTime" ) )
				//[/CoOp]
				{//FIXME: check forcePushRadius[NPC->client->ps.fd.forcePowerLevel[FP_PUSH]]
					ForceThrow( NPC, qfalse );
				}
				break;
			case 4:
			case 5:
			case 6:
			case 7:
			case 8:
			case 9:
			case 10:
			case 11:
			case 12:
				//try to parry the blow
				//Com_Printf( "blocking\n" );
				//[CoOp]
				Jedi_SaberBlock();
				//Jedi_SaberBlock(0, 0);
				//[/CoOp]
				break;
			default:
				//Evade!
				//start a strafe left/right if not already
				if ( !Q_irand( 0, 5 ) || !Jedi_Strafe( 300, 1000, 0, 1000, qfalse ) )
				{//certain chance they will pick an alternative evasion
					//if couldn't strafe, try a different kind of evasion...
					//[CoOp]
					if ( Jedi_DecideKick() 
						&& G_CanKickEntity(NPC, NPC->enemy ) 
						&& G_PickAutoKick( NPC, NPC->enemy, qtrue ) != LS_NONE )
					{//kicked!
						TIMER_Set( NPC, "kickDebounce", Q_irand( 3000, 10000 ) );
					}
					else if ( shooting_lightning || throwing_saber || enemy_dist < 80 )
					//if ( shooting_lightning || throwing_saber || enemy_dist < 80 )
					//[/CoOp]
					{
						//FIXME: force-jump+forward - jump over the guy!
						if ( shooting_lightning || (!Q_irand( 0, 2 ) && NPCInfo->stats.aggression < 4 && TIMER_Done( NPC, "parryTime" ) ) )
						{
							if ( (NPCInfo->rank == RANK_ENSIGN || NPCInfo->rank > RANK_LT_JG) && !shooting_lightning && Q_irand( 0, 2 ) )
							{//FIXME: check forcePushRadius[NPC->client->ps.fd.forcePowerLevel[FP_PUSH]]
								ForceThrow( NPC, qfalse );
							}
							else if ( (NPCInfo->rank==RANK_CREWMAN||NPCInfo->rank>RANK_LT_JG) 
								&& !(NPCInfo->scriptFlags&SCF_NO_ACROBATICS) 
								&& NPC->client->ps.fd.forceRageRecoveryTime < level.time
								&& !(NPC->client->ps.fd.forcePowersActive&(1<<FP_RAGE)) 
								&& !PM_InKnockDown( &NPC->client->ps ) )
							{//FIXME: make this a function call?
								//FIXME: check for clearance, safety of landing spot?
								NPC->client->ps.fd.forceJumpCharge = 480;
								//Don't jump again for another 2 to 5 seconds
								TIMER_Set( NPC, "jumpChaseDebounce", Q_irand( 2000, 5000 ) );
								if ( Q_irand( 0, 2 ) )
								{
									ucmd.forwardmove = 127;
									VectorClear( NPC->client->ps.moveDir );
								}
								else
								{
									ucmd.forwardmove = -127;
									VectorClear( NPC->client->ps.moveDir );
								}
								//FIXME: if this jump is cleared, we can't block... so pick a random lower block?
								if ( Q_irand( 0, 1 ) )//FIXME: make intelligent
								{
									NPC->client->ps.saberBlocked = BLOCKED_LOWER_RIGHT;
								}
								else
								{
									NPC->client->ps.saberBlocked = BLOCKED_LOWER_LEFT;
								}
							}
						}
						else if ( enemy_attacking )
						{
							//[CoOp]
							Jedi_SaberBlock();
							//Jedi_SaberBlock(0, 0);
							//[/CoOp]
						}
					}
				}
				else
				{//strafed
					if ( d_JediAI.integer )
					{
						Com_Printf( "def strafe\n" );
					}
					if ( !(NPCInfo->scriptFlags&SCF_NO_ACROBATICS) 
						&& NPC->client->ps.fd.forceRageRecoveryTime < level.time
						&& !(NPC->client->ps.fd.forcePowersActive&(1<<FP_RAGE)) 
						&& (NPCInfo->rank == RANK_CREWMAN || NPCInfo->rank > RANK_LT_JG ) 
						&& !PM_InKnockDown( &NPC->client->ps )
						&& !Q_irand( 0, 5 ) )
					{//FIXME: make this a function call?
						//FIXME: check for clearance, safety of landing spot?
						//[CoOp]
						if ( NPC->client->NPC_class == CLASS_BOBAFETT 
							|| (NPC->client->NPC_class == CLASS_REBORN && NPC->s.weapon != WP_SABER)
							|| NPC->client->NPC_class == CLASS_ROCKETTROOPER )
						//if ( NPC->client->NPC_class == CLASS_BOBAFETT )
						//[/CoOp]
						{
							NPC->client->ps.fd.forceJumpCharge = 280;//FIXME: calc this intelligently?
						}
						else
						{
							NPC->client->ps.fd.forceJumpCharge = 320;
						}
						//Don't jump again for another 2 to 5 seconds
						TIMER_Set( NPC, "jumpChaseDebounce", Q_irand( 2000, 5000 ) );
					}
				}
				break;
			}
		
			//turn off slow walking no matter what
			TIMER_Set( NPC, "walking", -level.time );
			TIMER_Set( NPC, "taunting", -level.time );
		}
	}
}
/*
-------------------------
Jedi_Flee
-------------------------
*/
/*

static qboolean Jedi_Flee( void )
{
	return qfalse;
}
*/


/*
==========================================================================================
INTERNAL AI ROUTINES
==========================================================================================
*/
gentity_t *Jedi_FindEnemyInCone( gentity_t *self, gentity_t *fallback, float minDot )
{//racc - never called.  not updated with SP code
	vec3_t forward, mins, maxs, dir;
	float	dist, bestDist = Q3_INFINITE;
	gentity_t	*enemy = fallback;
	gentity_t	*check = NULL;
	int			entityList[MAX_GENTITIES];
	int			e, numListedEntities;
	trace_t		tr;

	if ( !self->client )
	{
		return enemy;
	}

	AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );

	for ( e = 0 ; e < 3 ; e++ ) 
	{
		mins[e] = self->r.currentOrigin[e] - 1024;
		maxs[e] = self->r.currentOrigin[e] + 1024;
	}
	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) 
	{
		check = &g_entities[entityList[e]];
		if ( check == self )
		{//me
			continue;
		}
		if ( !(check->inuse) )
		{//freed
			continue;
		}
		if ( !check->client )
		{//not a client - FIXME: what about turrets?
			continue;
		}
		if ( check->client->playerTeam != self->client->enemyTeam )
		{//not an enemy - FIXME: what about turrets?
			continue;
		}
		if ( check->health <= 0 )
		{//dead
			continue;
		}

		if ( !trap_InPVS( check->r.currentOrigin, self->r.currentOrigin ) )
		{//can't potentially see them
			continue;
		}

		VectorSubtract( check->r.currentOrigin, self->r.currentOrigin, dir );
		dist = VectorNormalize( dir );

		if ( DotProduct( dir, forward ) < minDot )
		{//not in front
			continue;
		}

		//really should have a clear LOS to this thing...
		trap_Trace( &tr, self->r.currentOrigin, vec3_origin, vec3_origin, check->r.currentOrigin, self->s.number, MASK_SHOT );
		if ( tr.fraction < 1.0f && tr.entityNum != check->s.number )
		{//must have clear shot
			continue;
		}

		if ( dist < bestDist )
		{//closer than our last best one
			dist = bestDist;
			enemy = check;
		}
	}
	return enemy;
}


static qboolean enemy_in_striking_range = qfalse;
static void Jedi_SetEnemyInfo( vec3_t enemy_dest, vec3_t enemy_dir, float *enemy_dist, vec3_t enemy_movedir, float *enemy_movespeed, int prediction )
{
	if ( !NPC || !NPC->enemy )
	{//no valid enemy
		return;
	}
	if ( !NPC->enemy->client )
	{
		VectorClear( enemy_movedir );
		*enemy_movespeed = 0;
		VectorCopy( NPC->enemy->r.currentOrigin, enemy_dest );
		enemy_dest[2] += NPC->enemy->r.mins[2] + 24;//get it's origin to a height I can work with
		VectorSubtract( enemy_dest, NPC->r.currentOrigin, enemy_dir );
		//FIXME: enemy_dist calc needs to include all blade lengths, and include distance from hand to start of blade....
		*enemy_dist = VectorNormalize( enemy_dir );// - (NPC->client->ps.saberLengthMax + NPC->r.maxs[0]*1.5 + 16);
	}
	else
	{//see where enemy is headed
		VectorCopy( NPC->enemy->client->ps.velocity, enemy_movedir );
		*enemy_movespeed = VectorNormalize( enemy_movedir );
		//figure out where he'll be, say, 3 frames from now
		VectorMA( NPC->enemy->r.currentOrigin, *enemy_movespeed * 0.001 * prediction, enemy_movedir, enemy_dest );
		//figure out what dir the enemy's estimated position is from me and how far from the tip of my saber he is
		VectorSubtract( enemy_dest, NPC->r.currentOrigin, enemy_dir );//NPC->client->renderInfo.muzzlePoint
		//FIXME: enemy_dist calc needs to include all blade lengths, and include distance from hand to start of blade....
		*enemy_dist = VectorNormalize( enemy_dir ) - (NPC->client->saber[0].blade[0].lengthMax + NPC->r.maxs[0]*1.5 + 16); //just use the blade 0 len I guess
		//FIXME: keep a group of enemies around me and use that info to make decisions...
		//		For instance, if there are multiple enemies, evade more, push them away
		//		and use medium attacks.  If enemies are using blasters, switch to fast.
		//		If one jedi enemy, use strong attacks.  Use grip when fighting one or
		//		two enemies, use lightning spread when fighting multiple enemies, etc.
		//		Also, when kill one, check rest of group instead of walking up to victim.
	}
	//[CoOp]
	//added all the enemy_in_striking_range stuff from SP.
	//It's basically really cheap attack determination stuff.
	//init this to false
	enemy_in_striking_range = qfalse;
	if ( *enemy_dist <= 0.0f )
	{
		enemy_in_striking_range = qtrue;
	}
	else
	{//if he's too far away, see if he's at least facing us or coming towards us
		if ( *enemy_dist <= 32.0f )
		{//has to be facing us
			vec3_t eAngles;
			VectorSet(eAngles, 0, NPC->r.currentAngles[YAW], 0);
			if ( InFOV3( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin, eAngles, 30, 90 ) )
			{//in striking range
				enemy_in_striking_range = qtrue;
			}
		}
		if ( *enemy_dist >= 64.0f )
		{//we have to be approaching each other
			float vDot = 1.0f;
			if ( !VectorCompare( NPC->client->ps.velocity, vec3_origin ) )
			{//I am moving, see if I'm moving toward the enemy
				vec3_t	eDir;
				VectorSubtract( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, eDir );
				VectorNormalize( eDir );
				vDot = DotProduct( eDir, NPC->client->ps.velocity );
			}
			else if ( NPC->enemy->client && !VectorCompare( NPC->enemy->client->ps.velocity, vec3_origin ) )
			{//I'm not moving, but the enemy is, see if he's moving towards me
				vec3_t	meDir;
				VectorSubtract( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin, meDir );
				VectorNormalize( meDir );
				vDot = DotProduct( meDir, NPC->enemy->client->ps.velocity );
			}
			else
			{//neither of us is moving, below check will fail, so just return
				return;
			}
			if ( vDot >= *enemy_dist )
			{//moving towards each other
				enemy_in_striking_range = qtrue;
			}
		}
	}
	//[/CoOp]
}


//[CoOp]
void NPC_EvasionSaber( void )
{//uses some cheap movement prediction to dodge saber strikes from the NPC's enemy
	if ( ucmd.upmove <= 0//not jumping
		&& (!ucmd.upmove || !ucmd.rightmove) )//either just ducking or just strafing (i.e.: not rolling
	{//see if we need to avoid their saber
		vec3_t	enemy_dir, enemy_movedir, enemy_dest;
		float	enemy_dist, enemy_movespeed;
		//set enemy
		Jedi_SetEnemyInfo( enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300 );
		Jedi_EvasionSaber( enemy_movedir, enemy_dist, enemy_dir );
	}
}
//[/CoOp]

extern float WP_SpeedOfMissileForWeapon( int wp, qboolean alt_fire );
static void Jedi_FaceEnemy( qboolean doPitch )
{//set desired viewangles to properly face toward's our enemy based on the situation.
	vec3_t	enemy_eyes, eyes, angles;

	if ( NPC == NULL )
		return;

	if ( NPC->enemy == NULL )
		return;

	if ( NPC->client->ps.fd.forcePowersActive & (1<<FP_GRIP) &&
		NPC->client->ps.fd.forcePowerLevel[FP_GRIP] > FORCE_LEVEL_1 )
	{//don't update?
		NPCInfo->desiredPitch = NPC->client->ps.viewangles[PITCH];
		NPCInfo->desiredYaw = NPC->client->ps.viewangles[YAW];
		return;
	}
	CalcEntitySpot( NPC, SPOT_HEAD, eyes );

	CalcEntitySpot( NPC->enemy, SPOT_HEAD, enemy_eyes );

	if ( NPC->client->NPC_class == CLASS_BOBAFETT 
		&& TIMER_Done( NPC, "flameTime" )
		&& NPC->s.weapon != WP_NONE
		&& NPC->s.weapon != WP_DISRUPTOR
		&& (NPC->s.weapon != WP_ROCKET_LAUNCHER||!(NPCInfo->scriptFlags&SCF_ALT_FIRE))
		&& NPC->s.weapon != WP_THERMAL
		&& NPC->s.weapon != WP_TRIP_MINE
		&& NPC->s.weapon != WP_DET_PACK
		&& NPC->s.weapon != WP_STUN_BATON
		//[CoOp]
		&& NPC->s.weapon != WP_MELEE )
		//[/CoOp]
	{//boba leads his enemy
		if ( NPC->health < NPC->client->pers.maxHealth*0.5f )
		{//lead
			float missileSpeed = WP_SpeedOfMissileForWeapon( NPC->s.weapon, ((qboolean)(NPCInfo->scriptFlags&SCF_ALT_FIRE)) );
			if ( missileSpeed )
			{
				float eDist = Distance( eyes, enemy_eyes );
				eDist /= missileSpeed;//How many seconds it will take to get to the enemy
				VectorMA( enemy_eyes, eDist*flrand(0.95f,1.25f), NPC->enemy->client->ps.velocity, enemy_eyes );
			}
		}
	}
	//racc - Determine which way you should be facing for the situation.
	//Find the desired angles
	if ( !NPC->client->ps.saberInFlight 
		&& (NPC->client->ps.legsAnim == BOTH_A2_STABBACK1 
			|| NPC->client->ps.legsAnim == BOTH_CROUCHATTACKBACK1
			|| NPC->client->ps.legsAnim == BOTH_ATTACK_BACK) 
		)
	{//point *away*
		GetAnglesForDirection( enemy_eyes, eyes, angles );
	}
	//[CoOp]
	/* RAFIXME - from SP code but looks like it was never implimented.  impliment?
	else if ( NPC->client->ps.legsAnim == BOTH_A7_KICK_R )
	{//keep enemy to right
	}
	else if ( NPC->client->ps.legsAnim == BOTH_A7_KICK_L )
	{//keep enemy to left
	}
	else if ( NPC->client->ps.legsAnim == BOTH_A7_KICK_RL 
		|| NPC->client->ps.legsAnim == BOTH_A7_KICK_BF 
		|| NPC->client->ps.legsAnim == BOTH_A7_KICK_S )
	{//???
	}
	*/
	//[/CoOp]
	else
	{//point towards him
		GetAnglesForDirection( eyes, enemy_eyes, angles );
	}

	NPCInfo->desiredYaw	= AngleNormalize360( angles[YAW] );
	/*
	if ( NPC->client->ps.saberBlocked == BLOCKED_UPPER_LEFT )
	{//temp hack- to make up for poor coverage on left side
		NPCInfo->desiredYaw += 30;
	}
	*/

	if ( doPitch )
	{
		NPCInfo->desiredPitch = AngleNormalize360( angles[PITCH] );
		if ( NPC->client->ps.saberInFlight )
		{//tilt down a little
			NPCInfo->desiredPitch += 10;
		}
	}
	//FIXME: else desiredPitch = 0?  Or keep previous?
}

static void Jedi_DebounceDirectionChanges( void )
{//debounce all the movement directional timers.
	//FIXME: check these before making fwd/back & right/left decisions?
	//Time-debounce changes in forward/back dir
	if ( ucmd.forwardmove > 0 )
	{
		if ( !TIMER_Done( NPC, "moveback" ) || !TIMER_Done( NPC, "movenone" ) )
		{
			ucmd.forwardmove = 0;
			//now we have to normalize the total movement again
			if ( ucmd.rightmove > 0 )
			{
				ucmd.rightmove = 127;
			}
			else if ( ucmd.rightmove < 0 )
			{
				ucmd.rightmove = -127;
			}
			VectorClear( NPC->client->ps.moveDir );
			TIMER_Set( NPC, "moveback", -level.time );
			if ( TIMER_Done( NPC, "movenone" ) )
			{
				TIMER_Set( NPC, "movenone", Q_irand( 1000, 2000 ) );
			}
		}
		else if ( TIMER_Done( NPC, "moveforward" ) )
		{//FIXME: should be if it's zero?
		//[CoOp]
			if ( TIMER_Done( NPC, "lastmoveforward" ) )
			{
				int holdDirTime = Q_irand( 500, 2000 );
				TIMER_Set( NPC, "moveforward", holdDirTime );
				//so we don't keep doing this over and over again - new nav stuff makes them coast to a stop, so they could be just slowing down from the last "moveback" timer's ending...
				TIMER_Set( NPC, "lastmoveforward", holdDirTime + Q_irand(1000,2000) );
			}
		}
		else
		{//NOTE: edge checking should stop me if this is bad... but what if it sends us colliding into the enemy?
			//if being forced to move forward, do a full-speed moveforward
			ucmd.forwardmove = 127;
			VectorClear( NPC->client->ps.moveDir );
		}
			//TIMER_Set( NPC, "moveforward", Q_irand( 500, 2000 ) );
		//}
		//[/CoOp]
	}
	else if ( ucmd.forwardmove < 0 )
	{
		if ( !TIMER_Done( NPC, "moveforward" ) || !TIMER_Done( NPC, "movenone" ) )
		{
			ucmd.forwardmove = 0;
			//now we have to normalize the total movement again
			if ( ucmd.rightmove > 0 )
			{
				ucmd.rightmove = 127;
			}
			else if ( ucmd.rightmove < 0 )
			{
				ucmd.rightmove = -127;
			}
			VectorClear( NPC->client->ps.moveDir );
			TIMER_Set( NPC, "moveforward", -level.time );
			if ( TIMER_Done( NPC, "movenone" ) )
			{
				TIMER_Set( NPC, "movenone", Q_irand( 1000, 2000 ) );
			}
		}
		else if ( TIMER_Done( NPC, "moveback" ) )
		{//FIXME: should be if it's zero?
		//[CoOp]
		if ( TIMER_Done( NPC, "lastmoveback" ) )
			{
				int holdDirTime = Q_irand( 500, 2000 );
				TIMER_Set( NPC, "moveback", holdDirTime );
				//so we don't keep doing this over and over again - new nav stuff makes them coast to a stop, so they could be just slowing down from the last "moveback" timer's ending...
				TIMER_Set( NPC, "lastmoveback", holdDirTime + Q_irand(1000,2000) );
			}
		}
		else
		{//NOTE: edge checking should stop me if this is bad...
			//if being forced to move back, do a full-speed moveback
			ucmd.forwardmove = -127;
			VectorClear( NPC->client->ps.moveDir );
		}

		//	TIMER_Set( NPC, "moveback", Q_irand( 250, 1000 ) );
		//}
		//[/CoOp]
	}
	else if ( !TIMER_Done( NPC, "moveforward" ) )
	{//NOTE: edge checking should stop me if this is bad... but what if it sends us colliding into the enemy?
		ucmd.forwardmove = 127;
		VectorClear( NPC->client->ps.moveDir );
	}
	else if ( !TIMER_Done( NPC, "moveback" ) )
	{//NOTE: edge checking should stop me if this is bad...
		ucmd.forwardmove = -127;
		VectorClear( NPC->client->ps.moveDir );
	}
	//Time-debounce changes in right/left dir
	if ( ucmd.rightmove > 0 )
	{
		if ( !TIMER_Done( NPC, "moveleft" ) || !TIMER_Done( NPC, "movecenter" ) )
		{
			ucmd.rightmove = 0;
			//now we have to normalize the total movement again
			if ( ucmd.forwardmove > 0 )
			{
				ucmd.forwardmove = 127;
			}
			else if ( ucmd.forwardmove < 0 )
			{
				ucmd.forwardmove = -127;
			}
			VectorClear( NPC->client->ps.moveDir );
			TIMER_Set( NPC, "moveleft", -level.time );
			if ( TIMER_Done( NPC, "movecenter" ) )
			{
				TIMER_Set( NPC, "movecenter", Q_irand( 1000, 2000 ) );
			}
		}
		else if ( TIMER_Done( NPC, "moveright" ) )
		{//FIXME: should be if it's zero?
		//[CoOp]
			if ( TIMER_Done( NPC, "lastmoveright" ) )
			{
				int holdDirTime = Q_irand( 250, 1500 );
				TIMER_Set( NPC, "moveright", holdDirTime );
				//so we don't keep doing this over and over again - new nav stuff makes them coast to a stop, so they could be just slowing down from the last "moveback" timer's ending...
				TIMER_Set( NPC, "lastmoveright", holdDirTime + Q_irand(1000,2000) );
			}
		}
		else
		{//NOTE: edge checking should stop me if this is bad...
			//if being forced to move back, do a full-speed moveright
			ucmd.rightmove = 127;
			VectorClear( NPC->client->ps.moveDir );
		}
			//TIMER_Set( NPC, "moveright", Q_irand( 250, 1500 ) );
		//}
		//[/CoOp]
	}
	else if ( ucmd.rightmove < 0 )
	{
		if ( !TIMER_Done( NPC, "moveright" ) || !TIMER_Done( NPC, "movecenter" ) )
		{
			ucmd.rightmove = 0;
			//now we have to normalize the total movement again
			if ( ucmd.forwardmove > 0 )
			{
				ucmd.forwardmove = 127;
			}
			else if ( ucmd.forwardmove < 0 )
			{
				ucmd.forwardmove = -127;
			}
			VectorClear( NPC->client->ps.moveDir );
			TIMER_Set( NPC, "moveright", -level.time );
			if ( TIMER_Done( NPC, "movecenter" ) )
			{
				TIMER_Set( NPC, "movecenter", Q_irand( 1000, 2000 ) );
			}
		}
		else if ( TIMER_Done( NPC, "moveleft" ) )
		{//FIXME: should be if it's zero?
		//[CoOp]
			if ( TIMER_Done( NPC, "lastmoveleft" ) )
			{
				int holdDirTime = Q_irand( 250, 1500 );
				TIMER_Set( NPC, "moveleft", holdDirTime );
				//so we don't keep doing this over and over again - new nav stuff makes them coast to a stop, so they could be just slowing down from the last "moveback" timer's ending...
				TIMER_Set( NPC, "lastmoveleft", holdDirTime + Q_irand(1000,2000) );
			}
		}
		else
		{//NOTE: edge checking should stop me if this is bad...
			//if being forced to move back, do a full-speed moveleft
			ucmd.rightmove = -127;
			VectorClear( NPC->client->ps.moveDir );
		}
			//TIMER_Set( NPC, "moveleft", Q_irand( 250, 1500 ) );
		//}
		//[/CoOp]
	}
	else if ( !TIMER_Done( NPC, "moveright" ) )
	{//NOTE: edge checking should stop me if this is bad... 
		ucmd.rightmove = 127;
		VectorClear( NPC->client->ps.moveDir );
	}
	else if ( !TIMER_Done( NPC, "moveleft" ) )
	{//NOTE: edge checking should stop me if this is bad...
		ucmd.rightmove = -127;
		VectorClear( NPC->client->ps.moveDir );
	}
}

static void Jedi_TimersApply( void )
{
	//[CoOp]
	//use careful anim/slower movement if not already moving
	if ( !ucmd.forwardmove && !TIMER_Done( NPC, "walking" ) )
	{
		ucmd.buttons |= (BUTTON_WALKING);
	}

	if ( !TIMER_Done( NPC, "taunting" ) )
	{
		ucmd.buttons |= (BUTTON_WALKING);
	}
	//[/CoOp]

	if ( !ucmd.rightmove )
	{//only if not already strafing
		//FIXME: if enemy behind me and turning to face enemy, don't strafe in that direction, too
		if ( !TIMER_Done( NPC, "strafeLeft" ) )
		{
			if ( NPCInfo->desiredYaw > NPC->client->ps.viewangles[YAW] + 60 )
			{//we want to turn left, don't apply the strafing
			}
			else
			{//go ahead and strafe left
				ucmd.rightmove = -127;
				VectorClear( NPC->client->ps.moveDir );
			}
		}
		else if ( !TIMER_Done( NPC, "strafeRight" ) )
		{
			if ( NPCInfo->desiredYaw < NPC->client->ps.viewangles[YAW] - 60 )
			{//we want to turn right, don't apply the strafing
			}
			else
			{//go ahead and strafe left
				ucmd.rightmove = 127;
				VectorClear( NPC->client->ps.moveDir );
			}
		}
	}

	Jedi_DebounceDirectionChanges();

	//[CoOp]
	/* moved up to prevent some possible subtle differences between this
	//and the SP code.
	//use careful anim/slower movement if not already moving
	if ( !ucmd.forwardmove && !TIMER_Done( NPC, "walking" ) )
	{
		ucmd.buttons |= (BUTTON_WALKING);
	}

	if ( !TIMER_Done( NPC, "taunting" ) )
	{
		ucmd.buttons |= (BUTTON_WALKING);
	}
	*/
	//[/CoOp]

	if ( !TIMER_Done( NPC, "gripping" ) )
	{//FIXME: what do we do if we ran out of power?  NPC's can't?
		//FIXME: don't keep turning to face enemy or we'll end up spinning around
		ucmd.buttons |= BUTTON_FORCEGRIP;
	}

	if ( !TIMER_Done( NPC, "draining" ) )
	{//FIXME: what do we do if we ran out of power?  NPC's can't?
		//FIXME: don't keep turning to face enemy or we'll end up spinning around
		ucmd.buttons |= BUTTON_FORCE_DRAIN;
	}

	if ( !TIMER_Done( NPC, "holdLightning" ) )
	{//hold down the lightning key
		ucmd.buttons |= BUTTON_FORCE_LIGHTNING;
	}
}

static void Jedi_CombatTimersUpdate( int enemy_dist )
{
	//[CoOp]
	if ( Jedi_CultistDestroyer( NPC ) )
	{
		Jedi_Aggression( NPC, 5 );
		return;
	}
	//[/CoOp]

	if ( TIMER_Done( NPC, "roamTime" ) )
	{
		TIMER_Set( NPC, "roamTime", Q_irand( 2000, 5000 ) );
		//okay, now mess with agression
		//[CoOp]
		/* rage doesn't randomly boost aggression in SP code.
		if ( NPC->client->ps.fd.forcePowersActive&(1<<FP_RAGE) )
		{//raging
			Jedi_Aggression( NPC, Q_irand( 0, 3 ) );
		}
		else if ( NPC->client->ps.fd.forceRageRecoveryTime > level.time )
		{//recovering
			Jedi_Aggression( NPC, Q_irand( 0, -2 ) );
		}
		*/
		//[/CoOp]
		if ( NPC->enemy && NPC->enemy->client )
		{
			switch( NPC->enemy->client->ps.weapon )
			{
			case WP_SABER:
				//If enemy has a lightsaber, always close in
				if ( BG_SabersOff( &NPC->enemy->client->ps ) )
				{//fool!  Standing around unarmed, charge!
					//Com_Printf( "(%d) raise agg - enemy saber off\n", level.time );
					Jedi_Aggression( NPC, 2 );
				}
				else
				{
					//Com_Printf( "(%d) raise agg - enemy saber\n", level.time );
					Jedi_Aggression( NPC, 1 );
				}
				break;
			case WP_BLASTER:
			case WP_BRYAR_PISTOL:
			case WP_DISRUPTOR:
			case WP_BOWCASTER:
			case WP_REPEATER:
			case WP_DEMP2:
			case WP_FLECHETTE:
			case WP_ROCKET_LAUNCHER:
				//if he has a blaster, move in when:
				//They're not shooting at me
				if ( NPC->enemy->attackDebounceTime < level.time )
				{//does this apply to players?
					//Com_Printf( "(%d) raise agg - enemy not shooting ranged weap\n", level.time );
					Jedi_Aggression( NPC, 1 );
				}
				//He's closer than a dist that gives us time to deflect
				if ( enemy_dist < 256 )
				{
					//Com_Printf( "(%d) raise agg - enemy ranged weap- too close\n", level.time );
					Jedi_Aggression( NPC, 1 );
				}
				break;
			default:
				break;
			}
		}
	}

	if ( TIMER_Done( NPC, "noStrafe" ) && TIMER_Done( NPC, "strafeLeft" ) && TIMER_Done( NPC, "strafeRight" ) )
	{//racc - check to see if we want to strafe if we're not currently strafing.
		//FIXME: Maybe more likely to do this if aggression higher?  Or some other stat?
		if ( !Q_irand( 0, 4 ) )
		{//start a strafe
			if ( Jedi_Strafe( 1000, 3000, 0, 4000, qtrue ) )
			{
				if ( d_JediAI.integer )
				{
					Com_Printf( "off strafe\n" );
				}
			}
		}
		else
		{//postpone any strafing for a while
			TIMER_Set( NPC, "noStrafe", Q_irand( 1000, 3000 ) );
		}
	}

	if ( NPC->client->ps.saberEventFlags )
	{//some kind of saber combat event is still pending
		int newFlags = NPC->client->ps.saberEventFlags;
		if ( NPC->client->ps.saberEventFlags&SEF_PARRIED )
		{//parried
			TIMER_Set( NPC, "parryTime", -1 );
			/*
			if ( NPCInfo->rank >= RANK_LT_JG )
			{
				NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 100;
			}
			else
			{
				NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
			}
			*/
			//[CoOp]
			if ( NPC->enemy && (!NPC->enemy->client||PM_SaberInKnockaway( NPC->enemy->client->ps.saberMove )) )
			//if ( NPC->enemy && PM_SaberInKnockaway( NPC->enemy->client->ps.saberMove ) )
			//[/CoOp]
			//enemy in a knockaway so charge them and use a faster stance.
			{//advance!
				Jedi_Aggression( NPC, 1 );//get closer
				Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.fd.saberAnimLevel-1) );//use a faster attack
			}
			else
			{//racc - after parrying, randomly switch stance and reducing aggression.
				if ( !Q_irand( 0, 1 ) )//FIXME: dependant on rank/diff?
				{
					//Com_Printf( "(%d) drop agg - we parried\n", level.time );
					Jedi_Aggression( NPC, -1 );
				}
				if ( !Q_irand( 0, 1 ) )
				{
					Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.fd.saberAnimLevel-1) );
				}
			}
			if ( d_JediAI.integer )
			{
				Com_Printf( "(%d) PARRY: agg %d, no parry until %d\n", level.time, NPCInfo->stats.aggression, level.time + 100 );
			}
			newFlags &= ~SEF_PARRIED;
		}
		if ( !NPC->client->ps.weaponTime && (NPC->client->ps.saberEventFlags&SEF_HITENEMY) )//hit enemy
		{//we hit our enemy last time we swung, drop our aggression
			if ( !Q_irand( 0, 1 ) )//FIXME: dependant on rank/diff?
			{
				//Com_Printf( "(%d) drop agg - we hit enemy\n", level.time );
				Jedi_Aggression( NPC, -1 );
				if ( d_JediAI.integer )
				{
					Com_Printf( "(%d) HIT: agg %d\n", level.time, NPCInfo->stats.aggression );
				}
				if ( !Q_irand( 0, 3 ) 
					&& NPCInfo->blockedSpeechDebounceTime < level.time 
					&& jediSpeechDebounceTime[NPC->client->playerTeam] < level.time 
					&& NPC->painDebounceTime < level.time - 1000 )
				{
					G_AddVoiceEvent( NPC, Q_irand( EV_GLOAT1, EV_GLOAT3 ), 3000 );
					jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
				}
			}
			if ( !Q_irand( 0, 2 ) )
			{
				Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.fd.saberAnimLevel+1) );
			}
			newFlags &= ~SEF_HITENEMY;
		}
		if ( (NPC->client->ps.saberEventFlags&SEF_BLOCKED) )
		{//was blocked whilst attacking
			if ( PM_SaberInBrokenParry( NPC->client->ps.saberMove )
				|| NPC->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN )
			{
				//Com_Printf( "(%d) drop agg - we were knock-blocked\n", level.time );
				if ( NPC->client->ps.saberInFlight )
				{//lost our saber, too!!!
					Jedi_Aggression( NPC, -5 );//really really really should back off!!!
				}
				else
				{
					Jedi_Aggression( NPC, -2 );//really should back off!
				}
				Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.fd.saberAnimLevel+1) );//use a stronger attack
				if ( d_JediAI.integer )
				{
					Com_Printf( "(%d) KNOCK-BLOCKED: agg %d\n", level.time, NPCInfo->stats.aggression );
				}
			}
			else
			{
				if ( !Q_irand( 0, 2 ) )//FIXME: dependant on rank/diff?
				{
					//Com_Printf( "(%d) drop agg - we were blocked\n", level.time );
					Jedi_Aggression( NPC, -1 );
					if ( d_JediAI.integer )
					{
						Com_Printf( "(%d) BLOCKED: agg %d\n", level.time, NPCInfo->stats.aggression );
					}
				}
				if ( !Q_irand( 0, 1 ) )
				{
					Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.fd.saberAnimLevel+1) );
				}
			}
			newFlags &= ~SEF_BLOCKED;
			//FIXME: based on the type of parry the enemy is doing and my skill,
			//		choose an attack that is likely to get around the parry?
			//		right now that's generic in the saber animation code, auto-picks
			//		a next anim for me, but really should be AI-controlled.
		}
		if ( NPC->client->ps.saberEventFlags&SEF_DEFLECTED )
		{//deflected a shot
			newFlags &= ~SEF_DEFLECTED;
			if ( !Q_irand( 0, 3 ) )
			{
				Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.fd.saberAnimLevel-1) );
			}
		}
		if ( NPC->client->ps.saberEventFlags&SEF_HITWALL )
		{//hit a wall
			newFlags &= ~SEF_HITWALL;
		}
		if ( NPC->client->ps.saberEventFlags&SEF_HITOBJECT )
		{//hit some other damagable object
			if ( !Q_irand( 0, 3 ) )
			{
				Jedi_AdjustSaberAnimLevel( NPC, (NPC->client->ps.fd.saberAnimLevel-1) );
			}
			newFlags &= ~SEF_HITOBJECT;
		}
		NPC->client->ps.saberEventFlags = newFlags;
	}
}

static void Jedi_CombatIdle( int enemy_dist )
{
	if ( !TIMER_Done( NPC, "parryTime" ) )
	{
		return;
	}
	if ( NPC->client->ps.saberInFlight )
	{//don't do this idle stuff if throwing saber
		return;
	}
	if ( NPC->client->ps.fd.forcePowersActive&(1<<FP_RAGE) 
		|| NPC->client->ps.fd.forceRageRecoveryTime > level.time )
	{//never taunt while raging or recovering from rage
		return;
	}
	//[CoOp]
	/* RAFIXME - need to impliment scepter
	if ( NPC->client->ps.stats[STAT_WEAPONS]&(1<<WP_SCEPTER) )
	{//never taunt when holding scepter
		return;
	}
	*/
	if ( NPC->client->saber[0].type == SABER_SITH_SWORD )
	{//never taunt when holding sith sword
		return;
	}
	//[/CoOp]
	//FIXME: make these distance numbers defines?
	if ( enemy_dist >= 64 )
	{//FIXME: only do this if standing still?
		//based on aggression, flaunt/taunt
		int chance = 20;
		if ( NPC->client->NPC_class == CLASS_SHADOWTROOPER )
		{//racc - shadow troopers taunt less.
			chance = 10;
		}
		//FIXME: possibly throw local objects at enemy?
		if ( Q_irand( 2, chance ) < NPCInfo->stats.aggression )
		{
			if ( TIMER_Done( NPC, "chatter" ) && NPC->client->ps.forceHandExtend == HANDEXTEND_NONE )
			{//FIXME: add more taunt behaviors
				//FIXME: sometimes he turns it off, then turns it right back on again???
				if ( enemy_dist > 200 
					&& NPC->client->NPC_class != CLASS_BOBAFETT
					//[CoOp]
					&& (NPC->client->NPC_class != CLASS_REBORN || NPC->s.weapon == WP_SABER)
					&& NPC->client->NPC_class != CLASS_ROCKETTROOPER
					//[/CoOp]
					&& !NPC->client->ps.saberHolstered
					&& !Q_irand( 0, 5 ) )
				{//taunt even more, turn off the saber
					//FIXME: don't do this if health low?
					//[CoOp]
					//some taunts leave the saber on, otherwise turn it off
					if ( NPC->client->ps.fd.saberAnimLevel != SS_STAFF
						&& NPC->client->ps.fd.saberAnimLevel != SS_DUAL )
					{//those taunts leave saber on
						WP_DeactivateSaber( NPC, qfalse );
					}
					//WP_DeactivateSaber( NPC, qfalse );
					//[/CoOp]
					//Don't attack for a bit
					NPCInfo->stats.aggression = 3;
					//FIXME: maybe start strafing?
					//debounce this
					if ( NPC->client->playerTeam != NPCTEAM_PLAYER && !Q_irand( 0, 1 ))
					{//player allies don't do physical taunts.
						//NPC->client->ps.taunting = level.time + 100;
						NPC->client->ps.forceHandExtend = HANDEXTEND_JEDITAUNT;
						NPC->client->ps.forceHandExtendTime = level.time + 5000;

						TIMER_Set( NPC, "chatter", Q_irand( 5000, 10000 ) );
						TIMER_Set( NPC, "taunting", 5500 );
					}
					else
					{
						Jedi_BattleTaunt();
						TIMER_Set( NPC, "taunting", Q_irand( 5000, 10000 ) );
					}
				}
				else if ( Jedi_BattleTaunt() )
				{//FIXME: pick some anims
				}
			}
		}
	}
}


static qboolean Jedi_AttackDecide( int enemy_dist )
{
	//[CoOp]
	if ( !TIMER_Done( NPC, "allyJediDelay" ) )
	{//jedi allies hold off attacking non-saber using enemies.  Probably to give the
		//player more oppurtunity to battle.
		return qfalse;
	}

	if ( Jedi_CultistDestroyer( NPC ) )
	{//destroyer
		if ( enemy_dist <= 32 )
		{//go boom!
			NPC->flags |= FL_GODMODE;
			NPC->takedamage = qfalse;

			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_FORCE_RAGE, SETANIM_FLAG_HOLD|SETANIM_FLAG_OVERRIDE );
			NPC->client->ps.fd.forcePowersActive |= ( 1 << FP_RAGE );
//RAFIXME - useDebounceTime not implimented
			NPC->painDebounceTime /*= NPC->useDebounceTime*/ = level.time + NPC->client->ps.torsoTimer;
			return qtrue;
		}
		return qfalse;
	}
	//[/CoOp]

	if ( NPC->enemy->client 
		&& NPC->enemy->s.weapon == WP_SABER 
		&& NPC->enemy->client->ps.saberLockTime > level.time 
		&& NPC->client->ps.saberLockTime < level.time )
	{//enemy is in a saberLock and we are not
		return qfalse;
	}

	if ( NPC->client->ps.saberEventFlags&SEF_LOCK_WON )
	{//we won a saber lock, press the advantage with an attack!
		int	chance = 0;
		//[CoOp]
		//bosses push the attack hardest
		if ( (NPCInfo->aiFlags&NPCAI_BOSS_CHARACTER) )
		//if ( NPC->client->NPC_class == CLASS_DESANN || NPC->client->NPC_class == CLASS_LUKE || !Q_stricmp("Yoda",NPC->NPC_type) )
		//[/CoOp]
		{//desann and luke
			chance = 20;
		}
		//[CoOp]
		//weaker bosses press the attack less
		else if ( NPC->client->NPC_class == CLASS_TAVION 
			|| NPC->client->NPC_class == CLASS_ALORA )
		//else if ( NPC->client->NPC_class == CLASS_TAVION )
		//[/CoOp]
		{//tavion
			chance = 10;
		}
		//[CoOp]
		else if ( NPC->client->NPC_class == CLASS_SHADOWTROOPER )
		{//shadowtrooper
			chance = 5;
		}
		//[/CoOp]
		else if ( NPC->client->NPC_class == CLASS_REBORN && NPCInfo->rank == RANK_LT_JG ) 
		{//fencer
			chance = 5;
		}
		else
		{
			chance = NPCInfo->rank;
		}
		if ( Q_irand( 0, 30 ) < chance )
		{//based on skill with some randomness
			NPC->client->ps.saberEventFlags &= ~SEF_LOCK_WON;//clear this now that we are using the opportunity
			TIMER_Set( NPC, "noRetreat", Q_irand( 500, 2000 ) );
			//FIXME: check enemy_dist?
			NPC->client->ps.weaponTime = NPCInfo->shotTime = NPC->attackDebounceTime = 0;
			//NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
			NPC->client->ps.saberBlocked = BLOCKED_NONE;
			WeaponThink( qtrue );
			return qtrue;
		}
	}

	if ( NPC->client->NPC_class == CLASS_TAVION ||
		//[CoOp]
		NPC->client->NPC_class == CLASS_ALORA ||
		NPC->client->NPC_class == CLASS_SHADOWTROOPER ||
		//[CoOp]
		( NPC->client->NPC_class == CLASS_REBORN && NPCInfo->rank == RANK_LT_JG ) ||
		( NPC->client->NPC_class == CLASS_JEDI && NPCInfo->rank == RANK_COMMANDER ) )
	{//tavion, fencers, jedi trainer are all good at following up a parry with an attack
		if ( ( PM_SaberInParry( NPC->client->ps.saberMove ) || PM_SaberInKnockaway( NPC->client->ps.saberMove ) )
			&& NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
		{//try to attack straight from a parry
			NPC->client->ps.weaponTime = NPCInfo->shotTime = NPC->attackDebounceTime = 0;
			//NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
			NPC->client->ps.saberBlocked = BLOCKED_NONE;
			Jedi_AdjustSaberAnimLevel( NPC, FORCE_LEVEL_1 );//try to follow-up with a quick attack
			WeaponThink( qtrue );
			return qtrue;
		}
	}

	//try to hit them if we can
	//[CoOp]
	//enemy_in_striking_range is more accurate.
	if ( !enemy_in_striking_range )
	//if ( enemy_dist >= 64 )
	//[/CoOp]
	{
		return qfalse;
	}

	if ( !TIMER_Done( NPC, "parryTime" ) )
	{
		return qfalse;
	}

	if ( (NPCInfo->scriptFlags&SCF_DONT_FIRE) )
	{//not allowed to attack
		return qfalse;
	}

	if ( !(ucmd.buttons&BUTTON_ATTACK) && !(ucmd.buttons&BUTTON_ALT_ATTACK) )
	{//not already attacking
		//Try to attack
		WeaponThink( qtrue );
	}
	
	//FIXME:  Maybe try to push enemy off a ledge?

	//close enough to step forward

	//FIXME: an attack debounce timer other than the phaser debounce time?
	//		or base it on aggression?

	//[CoOp]
	if ( ucmd.buttons&BUTTON_ATTACK && !NPC_Jumping())
	//if ( ucmd.buttons&BUTTON_ATTACK )
	//[/CoOp]
	{//attacking
		/*
		if ( enemy_dist > 32 && NPCInfo->stats.aggression >= 4 )
		{//move forward if we're too far away and we're chasing him
			ucmd.forwardmove = 127;
		}
		else if ( enemy_dist < 0 )
		{//move back if we're too close
			ucmd.forwardmove = -127;
		}
		*/
		//FIXME: based on the type of parry/attack the enemy is doing and my skill,
		//		choose an attack that is likely to get around the parry?
		//		right now that's generic in the saber animation code, auto-picks
		//		a next anim for me, but really should be AI-controlled.
		//FIXME: have this interact with/override above strafing code?
		if ( !ucmd.rightmove )
		{//not already strafing
			if ( !Q_irand( 0, 3 ) )
			{//25% chance of doing this
				vec3_t  right, dir2enemy;

				AngleVectors( NPC->r.currentAngles, NULL, right, NULL );
				VectorSubtract( NPC->enemy->r.currentOrigin, NPC->r.currentAngles, dir2enemy );
				if ( DotProduct( right, dir2enemy ) > 0 )
				{//he's to my right, strafe left
					ucmd.rightmove = -127;
					VectorClear( NPC->client->ps.moveDir );
				}
				else
				{//he's to my left, strafe right
					ucmd.rightmove = 127;
					VectorClear( NPC->client->ps.moveDir );
				}
			}
		}
		return qtrue;
	}

	return qfalse;
}

#define	APEX_HEIGHT		200.0f
#define	PARA_WIDTH		(sqrt(APEX_HEIGHT)+sqrt(APEX_HEIGHT))
#define	JUMP_SPEED		200.0f

static qboolean Jedi_Jump( vec3_t dest, int goalEntNum )
{//FIXME: if land on enemy, knock him down & jump off again
	/*
	if ( dest[2] - NPC->r.currentOrigin[2] < 64 && DistanceHorizontal( NPC->r.currentOrigin, dest ) > 256 )
	{//a pretty horizontal jump, easy to fake:
		vec3_t enemy_diff;

		VectorSubtract( dest, NPC->r.currentOrigin, enemy_diff );
		float enemy_z_diff = enemy_diff[2];
		enemy_diff[2] = 0;
		float enemy_xy_diff = VectorNormalize( enemy_diff );

		VectorScale( enemy_diff, enemy_xy_diff*0.8, NPC->client->ps.velocity );
		if ( enemy_z_diff < 64 )
		{
			NPC->client->ps.velocity[2] = enemy_xy_diff;
		}
		else
		{
			NPC->client->ps.velocity[2] = enemy_z_diff*2+enemy_xy_diff/2;
		}
	}
	else
	*/
	if ( 1 )
	{
		float	targetDist, shotSpeed = 300, travelTime, impactDist, bestImpactDist = Q3_INFINITE;//fireSpeed, 
		vec3_t	targetDir, shotVel, failCase; 
		trace_t	trace;
		trajectory_t	tr;
		qboolean	blocked;
		int		elapsedTime, timeStep = 500, hitCount = 0, maxHits = 7;
		vec3_t	lastPos, testPos, bottom;

		while ( hitCount < maxHits )
		{
			VectorSubtract( dest, NPC->r.currentOrigin, targetDir );
			targetDist = VectorNormalize( targetDir );

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
					if ( testPos[2] < lastPos[2] )
					{//going down, ignore botclip
						trap_Trace( &trace, lastPos, NPC->r.mins, NPC->r.maxs, testPos, NPC->s.number, NPC->clipmask );
					}
					else
					{//going up, check for botclip
						trap_Trace( &trace, lastPos, NPC->r.mins, NPC->r.maxs, testPos, NPC->s.number, NPC->clipmask|CONTENTS_BOTCLIP );
					}

					if ( trace.allsolid || trace.startsolid )
					{
						blocked = qtrue;
						break;
					}
					if ( trace.fraction < 1.0f )
					{//hit something
						if ( trace.entityNum == goalEntNum )
						{//hit the enemy, that's perfect!
							//Hmm, don't want to land on him, though...
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
					shotSpeed = 300 + ((hitCount-2) * 100);//from 100 to 900 (skipping 300)
					if ( hitCount >= 2 )
					{//skip 300 since that was the first value we tested
						shotSpeed += 100;
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
			//NOTE: or try failcase?
			VectorCopy( failCase, NPC->client->ps.velocity );
		}
		VectorCopy( shotVel, NPC->client->ps.velocity );
	}
	else
	{//a more complicated jump
		vec3_t		dir, p1, p2, apex;
		float		time, height, forward, z, xy, dist, apexHeight;

		if ( NPC->r.currentOrigin[2] > dest[2] )//NPCInfo->goalEntity->r.currentOrigin
		{
			VectorCopy( NPC->r.currentOrigin, p1 );
			VectorCopy( dest, p2 );//NPCInfo->goalEntity->r.currentOrigin
		}
		else if ( NPC->r.currentOrigin[2] < dest[2] )//NPCInfo->goalEntity->r.currentOrigin
		{
			VectorCopy( dest, p1 );//NPCInfo->goalEntity->r.currentOrigin
			VectorCopy( NPC->r.currentOrigin, p2 );
		}
		else
		{
			VectorCopy( NPC->r.currentOrigin, p1 );
			VectorCopy( dest, p2 );//NPCInfo->goalEntity->r.currentOrigin
		}

		//z = xy*xy
		VectorSubtract( p2, p1, dir );
		dir[2] = 0;

		//Get xy and z diffs
		xy = VectorNormalize( dir );
		z = p1[2] - p2[2];

		apexHeight = APEX_HEIGHT/2;

		//Determine most desirable apex height
		//FIXME: length of xy will change curve of parabola, need to account for this
		//somewhere... PARA_WIDTH
		/*
		apexHeight = (APEX_HEIGHT * PARA_WIDTH/xy) + (APEX_HEIGHT * z/128);
		if ( apexHeight < APEX_HEIGHT * 0.5 )
		{
			apexHeight = APEX_HEIGHT*0.5;
		}
		else if ( apexHeight > APEX_HEIGHT * 2 )
		{
			apexHeight = APEX_HEIGHT*2;
		}
		*/

		z = (sqrt(apexHeight + z) - sqrt(apexHeight));

		assert(z >= 0);

//		Com_Printf("apex is %4.2f percent from p1: ", (xy-z)*0.5/xy*100.0f);

		xy -= z;
		xy *= 0.5;
		
		assert(xy > 0);

		VectorMA( p1, xy, dir, apex );
		apex[2] += apexHeight;

		VectorCopy(apex, NPC->pos1);
		
		//Now we have the apex, aim for it
		height = apex[2] - NPC->r.currentOrigin[2];
		time = sqrt( height / ( .5 * NPC->client->ps.gravity ) );//was 0.5, but didn't work well for very long jumps
		if ( !time ) 
		{
			//Com_Printf( S_COLOR_RED"ERROR: no time in jump\n" );
			return qfalse;
		}

		VectorSubtract ( apex, NPC->r.currentOrigin, NPC->client->ps.velocity );
		NPC->client->ps.velocity[2] = 0;
		dist = VectorNormalize( NPC->client->ps.velocity );

		forward = dist / time * 1.25;//er... probably bad, but...
		VectorScale( NPC->client->ps.velocity, forward, NPC->client->ps.velocity );

		//FIXME:  Uh.... should we trace/EvaluateTrajectory this to make sure we have clearance and we land where we want?
		NPC->client->ps.velocity[2] = time * NPC->client->ps.gravity;

		//Com_Printf("Jump Velocity: %4.2f, %4.2f, %4.2f\n", NPC->client->ps.velocity[0], NPC->client->ps.velocity[1], NPC->client->ps.velocity[2] );
	}
	return qtrue;
}

static qboolean Jedi_TryJump( gentity_t *goal )
{//FIXME: never does a simple short, regular jump...
	//FIXME: I need to be on ground too!
	if ( (NPCInfo->scriptFlags&SCF_NO_ACROBATICS) )
	{
		return qfalse;
	}
	if ( TIMER_Done( NPC, "jumpChaseDebounce" ) )
	{
		if ( (!goal->client || goal->client->ps.groundEntityNum != ENTITYNUM_NONE) )
		{
			if ( !PM_InKnockDown( &NPC->client->ps ) && !BG_InRoll( &NPC->client->ps, NPC->client->ps.legsAnim ) )
			{//enemy is on terra firma
				vec3_t goal_diff;
				float goal_z_diff;
				float goal_xy_dist;
				VectorSubtract( goal->r.currentOrigin, NPC->r.currentOrigin, goal_diff );
				goal_z_diff = goal_diff[2];
				goal_diff[2] = 0;
				goal_xy_dist = VectorNormalize( goal_diff );
				if ( goal_xy_dist < 550 && goal_z_diff > -400/*was -256*/ )//for now, jedi don't take falling damage && (NPC->health > 20 || goal_z_diff > 0 ) && (NPC->health >= 100 || goal_z_diff > -128 ))//closer than @512
				{
					qboolean debounce = qfalse;
					if ( NPC->health < 150 && ((NPC->health < 30 && goal_z_diff < 0) || goal_z_diff < -128 ) )
					{//don't jump, just walk off... doesn't help with ledges, though
						debounce = qtrue;
					}
					else if ( goal_z_diff < 32 && goal_xy_dist < 200 )
					{//what is their ideal jump height?
						ucmd.upmove = 127;
						debounce = qtrue;
					}
					else
					{
						/*
						//NO!  All Jedi can jump-navigate now...
						if ( NPCInfo->rank != RANK_CREWMAN && NPCInfo->rank <= RANK_LT_JG )
						{//can't do acrobatics
							return qfalse;
						}
						*/
						if ( goal_z_diff > 0 || goal_xy_dist > 128 )
						{//Fake a force-jump
							//Screw it, just do my own calc & throw
							vec3_t dest;
							VectorCopy( goal->r.currentOrigin, dest );
							if ( goal == NPC->enemy )
							{
								int	sideTry = 0;
								while( sideTry < 10 )
								{//FIXME: make it so it doesn't try the same spot again?
									trace_t	trace;
									vec3_t	bottom;

									if ( Q_irand( 0, 1 ) )
									{
										dest[0] += NPC->enemy->r.maxs[0]*1.25;
									}
									else
									{
										dest[0] += NPC->enemy->r.mins[0]*1.25;
									}
									if ( Q_irand( 0, 1 ) )
									{
										dest[1] += NPC->enemy->r.maxs[1]*1.25;
									}
									else
									{
										dest[1] += NPC->enemy->r.mins[1]*1.25;
									}
									VectorCopy( dest, bottom );
									bottom[2] -= 128;
									trap_Trace( &trace, dest, NPC->r.mins, NPC->r.maxs, bottom, goal->s.number, NPC->clipmask );
									if ( trace.fraction < 1.0f )
									{//hit floor, okay to land here
										break;
									}
									sideTry++;
								}
								if ( sideTry >= 10 )
								{//screw it, just jump right at him?
									VectorCopy( goal->r.currentOrigin, dest );
								}
							}
							if ( Jedi_Jump( dest, goal->s.number ) )
							{
								//Com_Printf( "(%d) pre-checked force jump\n", level.time );

								//FIXME: store the dir we;re going in in case something gets in the way of the jump?
								//? = vectoyaw( NPC->client->ps.velocity );
								/*
								if ( NPC->client->ps.velocity[2] < 320 )
								{
									NPC->client->ps.velocity[2] = 320;
								}
								else
								*/
								{//FIXME: make this a function call
									int jumpAnim;
									//FIXME: this should be more intelligent, like the normal force jump anim logic
									if ( NPC->client->NPC_class == CLASS_BOBAFETT 
										||( NPCInfo->rank != RANK_CREWMAN && NPCInfo->rank <= RANK_LT_JG ) )
									{//can't do acrobatics
										jumpAnim = BOTH_FORCEJUMP1;
									}
									else
									{
										jumpAnim = BOTH_FLIP_F;
									}
									NPC_SetAnim( NPC, SETANIM_BOTH, jumpAnim, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
								}

								NPC->client->ps.fd.forceJumpZStart = NPC->r.currentOrigin[2];
								//NPC->client->ps.pm_flags |= PMF_JUMPING;

								NPC->client->ps.weaponTime = NPC->client->ps.torsoTimer;
								NPC->client->ps.fd.forcePowersActive |= ( 1 << FP_LEVITATION );
								if ( NPC->client->NPC_class == CLASS_BOBAFETT )
								{
									G_SoundOnEnt( NPC, CHAN_ITEM, "sound/boba/jeton.wav" );
									NPC->client->jetPackTime = level.time + Q_irand( 1000, 3000 );
								}
								else
								{
									G_SoundOnEnt( NPC, CHAN_BODY, "sound/weapons/force/jump.wav" );
								}

								TIMER_Set( NPC, "forceJumpChasing", Q_irand( 2000, 3000 ) );
								debounce = qtrue;
							}
						}
					}
					if ( debounce )
					{
						//Don't jump again for another 2 to 5 seconds
						TIMER_Set( NPC, "jumpChaseDebounce", Q_irand( 2000, 5000 ) );
						ucmd.forwardmove = 127;
						VectorClear( NPC->client->ps.moveDir );
						TIMER_Set( NPC, "duck", -level.time );
						return qtrue;
					}
				}
			}
		}
	}
	return qfalse;
}

//[CoOp]
//not used in SP code.
/*
static qboolean Jedi_Jumping( gentity_t *goal )
{
	if ( !TIMER_Done( NPC, "forceJumpChasing" ) && goal )
	{//force-jumping at the enemy
//		if ( !(NPC->client->ps.pm_flags & PMF_JUMPING )//forceJumpZStart )
//			&& !(NPC->client->ps.pm_flags&PMF_TRIGGER_PUSHED))
		if (NPC->client->ps.groundEntityNum != ENTITYNUM_NONE) //rwwFIXMEFIXME: Not sure if this is gonna work, use the PM flags ideally.
		{//landed
			TIMER_Set( NPC, "forceJumpChasing", 0 );
		}
		else
		{
			NPC_FaceEntity( goal, qtrue );
			//FIXME: push me torward where I was heading
			//FIXME: if hit a ledge as soon as we jumped, we need to push toward our goal... must remember original jump dir and/or original jump dest
			/*
			vec3_t	viewangles_xy={0,0,0}, goal_dir, goal_xy_dir, forward, right;
			float	goal_dist;
			
			//gert horz dir to goal
			VectorSubtract( goal->r.currentOrigin, NPC->r.currentOrigin, goal_dir );
			VectorCopy( goal_dir, goal_xy_dir );
			goal_dist = VectorNormalize( goal_dir );
			goal_xy_dir[2] = 0;
			VectorNormalize( goal_xy_dir );

			//get horz facing
			viewangles_xy[1] = NPC->client->ps.viewangles[1];
			AngleVectors( viewangles_xy, forward, right, NULL );

			//get movement commands to push me toward enemy
			float fDot = DotProduct( forward, goal_dir ) * 127;
			float rDot = DotProduct( right, goal_dir ) * 127;
		
			ucmd.forwardmove = floor(fDot);
			ucmd.rightmove = floor(rDot);
			ucmd.upmove = 0;//don't duck
			//Cheat:
			if ( goal_dist < 128 && goal->r.currentOrigin[2] > NPC->r.currentOrigin[2] && NPC->client->ps.velocity[2] <= 0 )
			{
				NPC->client->ps.velocity[2] += 320;
			}
			*/
/*
			return qtrue;
		}
	}
	return qfalse;
}
*/
//[/CoOp]

extern void G_UcmdMoveForDir( gentity_t *self, usercmd_t *cmd, vec3_t dir );
static void Jedi_CheckEnemyMovement( float enemy_dist )
{//racc - This function does a variety of tweaks to make things easier on the player
	//by making non-boss NPCs do stupid stuff against special attacks.
	if ( !NPC->enemy || !NPC->enemy->client )
	{
		return;
	}

	//[CoOp]
	if ( !(NPCInfo->aiFlags&NPCAI_BOSS_CHARACTER) )
	{//we're not a boss
	/*
	if ( NPC->client->NPC_class != CLASS_TAVION 
		&& NPC->client->NPC_class != CLASS_DESANN
		&& NPC->client->NPC_class != CLASS_LUKE 
		&& Q_stricmp("Yoda",NPC->NPC_type) )
	*/
		//[CoOp]
		if ( BG_KickingAnim( NPC->enemy->client->ps.legsAnim )
			&& NPC->client->ps.groundEntityNum != ENTITYNUM_NONE
			//FIXME: I'm relatively close to him
			&& (NPC->enemy->client->ps.legsAnim == BOTH_A7_KICK_RL
				|| NPC->enemy->client->ps.legsAnim == BOTH_A7_KICK_BF
				|| NPC->enemy->client->ps.legsAnim == BOTH_A7_KICK_S
				|| (NPC->enemy->enemy && NPC->enemy->enemy == NPC))
			)
		{//run into the kick!
			ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
			VectorClear( NPC->client->ps.moveDir );
			Jedi_Advance();
		}
		else if ( NPC->enemy->client->ps.torsoAnim == BOTH_A7_HILT
				&& NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
		{//run into the hilt bash
			//FIXME : only if in front!
			ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
			VectorClear( NPC->client->ps.moveDir );
			Jedi_Advance();
		}
		else if ( (NPC->enemy->client->ps.torsoAnim == BOTH_A6_FB
					|| NPC->enemy->client->ps.torsoAnim == BOTH_A6_LR)
				&& NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
		{//run into the attack
			//FIXME : only if on R/L or F/B?
			ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
			VectorClear( NPC->client->ps.moveDir );
			Jedi_Advance();
		}
		else if ( NPC->enemy->enemy && NPC->enemy->enemy == NPC )
		//if ( NPC->enemy->enemy && NPC->enemy->enemy == NPC )
		//[/CoOp]
		{//enemy is mad at *me*
			//[CoOp]
			if ( NPC->enemy->client->ps.legsAnim == BOTH_JUMPFLIPSLASHDOWN1
				|| NPC->enemy->client->ps.legsAnim == BOTH_JUMPFLIPSTABDOWN
				|| NPC->enemy->client->ps.legsAnim == BOTH_FLIP_ATTACK7 )
			//if ( NPC->enemy->client->ps.legsAnim == BOTH_JUMPFLIPSLASHDOWN1 ||
			//	NPC->enemy->client->ps.legsAnim == BOTH_JUMPFLIPSTABDOWN )
			//[/CoOp]
			{//enemy is flipping over me
				if ( Q_irand( 0, NPCInfo->rank ) < RANK_LT )
				{//be nice and stand still for him...
					ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
					VectorClear( NPC->client->ps.moveDir );
					NPC->client->ps.fd.forceJumpCharge = 0;
					TIMER_Set( NPC, "strafeLeft", -1 );
					TIMER_Set( NPC, "strafeRight", -1 );
					TIMER_Set( NPC, "noStrafe", Q_irand( 500, 1000 ) );
					TIMER_Set( NPC, "movenone", Q_irand( 500, 1000 ) );
					TIMER_Set( NPC, "movecenter", Q_irand( 500, 1000 ) );
				}
			}
			else if ( NPC->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_BACK1 
				|| NPC->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_RIGHT 
				|| NPC->enemy->client->ps.legsAnim == BOTH_WALL_FLIP_LEFT 
				|| NPC->enemy->client->ps.legsAnim == BOTH_WALL_RUN_LEFT_FLIP 
				|| NPC->enemy->client->ps.legsAnim == BOTH_WALL_RUN_RIGHT_FLIP )
			{//he's flipping off a wall
				if ( NPC->enemy->client->ps.groundEntityNum == ENTITYNUM_NONE )
				{//still in air
					if ( enemy_dist < 256 )
					{//close
						if ( Q_irand( 0, NPCInfo->rank ) < RANK_LT )
						{//be nice and stand still for him...
							vec3_t enemyFwd, dest, dir;

							/*
							ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
							VectorClear( NPC->client->ps.moveDir );
							NPC->client->ps.fd.forceJumpCharge = 0;
							TIMER_Set( NPC, "strafeLeft", -1 );
							TIMER_Set( NPC, "strafeRight", -1 );
							TIMER_Set( NPC, "noStrafe", Q_irand( 500, 1000 ) );
							TIMER_Set( NPC, "movenone", Q_irand( 500, 1000 ) );
							TIMER_Set( NPC, "movecenter", Q_irand( 500, 1000 ) );
							TIMER_Set( NPC, "noturn", Q_irand( 200, 500 ) );
							*/
							//stop current movement
							ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
							VectorClear( NPC->client->ps.moveDir );
							NPC->client->ps.fd.forceJumpCharge = 0;
							TIMER_Set( NPC, "strafeLeft", -1 );
							TIMER_Set( NPC, "strafeRight", -1 );
							TIMER_Set( NPC, "noStrafe", Q_irand( 500, 1000 ) );
							TIMER_Set( NPC, "noturn", Q_irand( 250, 500 )*(3-g_spskill.integer) );

							VectorCopy( NPC->enemy->client->ps.velocity, enemyFwd );
							VectorNormalize( enemyFwd );
							VectorMA( NPC->enemy->r.currentOrigin, -64, enemyFwd, dest );
							VectorSubtract( dest, NPC->r.currentOrigin, dir );
							if ( VectorNormalize( dir ) > 32 )
							{
								G_UcmdMoveForDir( NPC, &ucmd, dir );
							}
							else
							{
								TIMER_Set( NPC, "movenone", Q_irand( 500, 1000 ) );
								TIMER_Set( NPC, "movecenter", Q_irand( 500, 1000 ) );
							}
						}
					}
				}
			}
			else if ( NPC->enemy->client->ps.legsAnim == BOTH_A2_STABBACK1 )
			{//he's stabbing backwards
				if ( enemy_dist < 256 && enemy_dist > 64 )
				{//close
					if ( !InFront( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin, NPC->enemy->r.currentAngles, 0.0f ) )
					{//behind him
						if ( !Q_irand( 0, NPCInfo->rank ) )
						{//be nice and stand still for him...
							vec3_t enemyFwd, dest, dir;

							//stop current movement
							ucmd.forwardmove = ucmd.rightmove = ucmd.upmove = 0;
							VectorClear( NPC->client->ps.moveDir );
							NPC->client->ps.fd.forceJumpCharge = 0;
							TIMER_Set( NPC, "strafeLeft", -1 );
							TIMER_Set( NPC, "strafeRight", -1 );
							TIMER_Set( NPC, "noStrafe", Q_irand( 500, 1000 ) );

							AngleVectors( NPC->enemy->r.currentAngles, enemyFwd, NULL, NULL );
							VectorMA( NPC->enemy->r.currentOrigin, -32, enemyFwd, dest );
							VectorSubtract( dest, NPC->r.currentOrigin, dir );
							if ( VectorNormalize( dir ) > 64 )
							{
								G_UcmdMoveForDir( NPC, &ucmd, dir );
							}
							else
							{
								TIMER_Set( NPC, "movenone", Q_irand( 500, 1000 ) );
								TIMER_Set( NPC, "movecenter", Q_irand( 500, 1000 ) );
							}
						}
					}
				}
			}
		}
	}
	//FIXME: also:
	//		If enemy doing wall flip, keep running forward
	//		If enemy doing back-attack and we're behind him keep running forward toward his back, don't strafe
}

static void Jedi_CheckJumps( void )
{//racc - aborts jump if current jump fails safety checks.
	vec3_t	jumpVel;
	trace_t	trace;
	trajectory_t	tr;
	vec3_t	lastPos, testPos, bottom;
	int		elapsedTime;

	if ( (NPCInfo->scriptFlags&SCF_NO_ACROBATICS) )
	{
		NPC->client->ps.fd.forceJumpCharge = 0;
		ucmd.upmove = 0;
		return;
	}
	//FIXME: should probably check this before AI decides that best move is to jump?  Otherwise, they may end up just standing there and looking dumb
	//FIXME: all this work and he still jumps off ledges... *sigh*... need CONTENTS_BOTCLIP do-not-enter brushes...?
	VectorClear(jumpVel);

	if ( NPC->client->ps.fd.forceJumpCharge )
	{
		//Com_Printf( "(%d) force jump\n", level.time );
		WP_GetVelocityForForceJump( NPC, jumpVel, &ucmd );
	}
	else if ( ucmd.upmove > 0 )
	{
		//Com_Printf( "(%d) regular jump\n", level.time );
		VectorCopy( NPC->client->ps.velocity, jumpVel );
		jumpVel[2] = JUMP_VELOCITY;
	}
	else
	{
		return;
	}
	
	//NOTE: for now, we clear ucmd.forwardmove & ucmd.rightmove while in air to avoid jumps going awry...
	if ( !jumpVel[0] && !jumpVel[1] )//FIXME: && !ucmd.forwardmove && !ucmd.rightmove?
	{//we assume a jump straight up is safe
		//Com_Printf( "(%d) jump straight up is safe\n", level.time );
		return;
	}
	//Now predict where this is going
	//in steps, keep evaluating the trajectory until the new z pos is <= than current z pos, trace down from there

	VectorCopy( NPC->r.currentOrigin, tr.trBase );
	VectorCopy( jumpVel, tr.trDelta );
	tr.trType = TR_GRAVITY;
	tr.trTime = level.time;
	VectorCopy( NPC->r.currentOrigin, lastPos );
	
	VectorClear(trace.endpos); //shut the compiler up

	//This may be kind of wasteful, especially on long throws... use larger steps?  Divide the travelTime into a certain hard number of slices?  Trace just to apex and down?
	for ( elapsedTime = 500; elapsedTime <= 4000; elapsedTime += 500 )
	{
		BG_EvaluateTrajectory( &tr, level.time + elapsedTime, testPos );
		//FIXME: account for PM_AirMove if ucmd.forwardmove and/or ucmd.rightmove is non-zero...
		if ( testPos[2] < lastPos[2] )
		{//going down, don't check for BOTCLIP
			trap_Trace( &trace, lastPos, NPC->r.mins, NPC->r.maxs, testPos, NPC->s.number, NPC->clipmask );//FIXME: include CONTENTS_BOTCLIP?
		}
		else
		{//going up, check for BOTCLIP
			trap_Trace( &trace, lastPos, NPC->r.mins, NPC->r.maxs, testPos, NPC->s.number, NPC->clipmask|CONTENTS_BOTCLIP );
		}
		if ( trace.allsolid || trace.startsolid )
		{//WTF?
			//FIXME: what do we do when we start INSIDE the CONTENTS_BOTCLIP?  Do the trace again without that clipmask?
			goto jump_unsafe;
			return;
		}
		if ( trace.fraction < 1.0f )
		{//hit something
			if ( trace.contents & CONTENTS_BOTCLIP )
			{//hit a do-not-enter brush
				goto jump_unsafe;
				return;
			}
			//FIXME: trace through func_glass?
			break;
		}
		VectorCopy( testPos, lastPos );
	}
	//okay, reached end of jump, now trace down from here for a floor
	VectorCopy( trace.endpos, bottom );
	if ( bottom[2] > NPC->r.currentOrigin[2] )
	{//only care about dist down from current height or lower
		bottom[2] = NPC->r.currentOrigin[2];
	}
	else if ( NPC->r.currentOrigin[2] - bottom[2] > 400 )
	{//whoa, long drop, don't do it!
		//probably no floor at end of jump, so don't jump
		goto jump_unsafe;
		return;
	}
	bottom[2] -= 128;
	trap_Trace( &trace, trace.endpos, NPC->r.mins, NPC->r.maxs, bottom, NPC->s.number, NPC->clipmask );
	if ( trace.allsolid || trace.startsolid || trace.fraction < 1.0f )
	{//hit ground!
		if ( trace.entityNum < ENTITYNUM_WORLD )
		{//landed on an ent
			gentity_t *groundEnt = &g_entities[trace.entityNum];
			if ( groundEnt->r.svFlags&SVF_GLASS_BRUSH )
			{//don't land on breakable glass!
				goto jump_unsafe;
				return;
			}
		}
		//Com_Printf( "(%d) jump is safe\n", level.time );
		return;
	}
jump_unsafe:
	//probably no floor at end of jump, so don't jump
	//Com_Printf( "(%d) unsafe jump cleared\n", level.time );
	NPC->client->ps.fd.forceJumpCharge = 0;
	ucmd.upmove = 0;
}

//[CoOp]
extern void RT_FireDecide( void );
//[/CoOp]
static void Jedi_Combat( void )
{
	vec3_t	enemy_dir, enemy_movedir, enemy_dest;
	float	enemy_dist, enemy_movespeed;
	qboolean	enemy_lost = qfalse;

	//See where enemy will be 300 ms from now
	Jedi_SetEnemyInfo( enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300 );

	//[CoOp]
	//replaced with generic NPC_jump
	if ( NPC_Jumping( ) )
	//if ( Jedi_Jumping( NPC->enemy ) )
	//[/CoOp]
	{//I'm in the middle of a jump, so just see if I should attack
		Jedi_AttackDecide( enemy_dist );
		return;
	}

	//[CoOp]
	if ( TIMER_Done( NPC, "allyJediDelay" ) )
	{//racc - we're not holding back
		if ( !(NPC->client->ps.fd.forcePowersActive&(1<<FP_GRIP)) || NPC->client->ps.fd.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2 )
		{//not gripping
			//If we can't get straight at him
			if ( !Jedi_ClearPathToSpot( enemy_dest, NPC->enemy->s.number ) )
			{//hunt him down
				//Com_Printf( "No Clear Path\n" );
				if ( (NPC_ClearLOS4( NPC->enemy )||NPCInfo->enemyLastSeenTime>level.time-500) && NPC_FaceEnemy( qtrue ) )//( NPCInfo->rank == RANK_CREWMAN || NPCInfo->rank > RANK_LT_JG ) && 
				{
					//try to jump to him?
					/*
					vec3_t end;
					VectorCopy( NPC->r.currentOrigin, end );
					end[2] += 36;
					trap_Trace( &trace, NPC->r.currentOrigin, NPC->r.mins, NPC->r.maxs, end, NPC->s.number, NPC->clipmask|CONTENTS_BOTCLIP );
					if ( !trace.allsolid && !trace.startsolid && trace.fraction >= 1.0 )
					{
						vec3_t angles, forward;
						VectorCopy( NPC->client->ps.viewangles, angles );
						angles[0] = 0;
						AngleVectors( angles, forward, NULL, NULL );
						VectorMA( end, 64, forward, end );
						trap_Trace( &trace, NPC->r.currentOrigin, NPC->r.mins, NPC->r.maxs, end, NPC->s.number, NPC->clipmask|CONTENTS_BOTCLIP );
						if ( !trace.allsolid && !trace.startsolid )
						{
							if ( trace.fraction >= 1.0 || trace.plane.normal[2] > 0 )
							{
								ucmd.upmove = 127;
								ucmd.forwardmove = 127;
								return;
							}
						}
					}
					*/
					//FIXME: about every 1 second calc a velocity, 
					//run a loop of traces with evaluate trajectory 
					//for gravity with my size, see if it makes it...
					//this will also catch misacalculations that send you off ledges!
					//Com_Printf( "Considering Jump\n" );
					//[CoOp]
					//ok, SP doesn't try to jump to the enemy, but boba fett tries to shot
					//at enemies he can't get to.
					if (NPC->client && NPC->client->NPC_class==CLASS_BOBAFETT)
					{
						Boba_FireDecide();
					}
					/*
					if ( Jedi_TryJump( NPC->enemy ) )
					{//FIXME: what about jumping to his enemyLastSeenLocation?
						return;
					}
					*/
					//[/CoOp]
				}

				//Check for evasion
				if ( TIMER_Done( NPC, "parryTime" ) )
				{//finished parrying
					if ( NPC->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
						NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
					{//wasn't blocked myself
						NPC->client->ps.saberBlocked = BLOCKED_NONE;
					}
				}
				if ( Jedi_Hunt() && !(NPCInfo->aiFlags&NPCAI_BLOCKED) )//FIXME: have to do this because they can ping-pong forever
				{//can macro-navigate to him //racc - but we can't actually see him?
					if ( enemy_dist < 384 && !Q_irand( 0, 10 ) && NPCInfo->blockedSpeechDebounceTime < level.time && jediSpeechDebounceTime[NPC->client->playerTeam] < level.time && !NPC_ClearLOS4( NPC->enemy ) )
					{//racc - bitch about losting our enemy
						G_AddVoiceEvent( NPC, Q_irand( EV_JLOST1, EV_JLOST3 ), 3000 );
						jediSpeechDebounceTime[NPC->client->playerTeam] = NPCInfo->blockedSpeechDebounceTime = level.time + 3000;
					}
					//[CoOp]
					//blastem?
					if (NPC->client && NPC->client->NPC_class==CLASS_BOBAFETT)
					{
						Boba_FireDecide();
					}
					//[/CoOp]

					return;
				}
				//well, try to head for his last seen location
		/*
				else if ( Jedi_Track() )
				{
					return;
				}
		*/		else
				{//FIXME: try to find a waypoint that can see enemy, jump from there
					//[CoOp]
					/*  not in SP code.
					if ( NPCInfo->aiFlags & NPCAI_BLOCKED )
					{//try to jump to the blockedDest
						gentity_t *tempGoal = G_Spawn();//ugh, this is NOT good...?
						G_SetOrigin( tempGoal, NPCInfo->blockedDest );
						trap_LinkEntity( tempGoal );
						if ( Jedi_TryJump( tempGoal ) )
						{//going to jump to the dest
							G_FreeEntity( tempGoal );
							return;
						}
						G_FreeEntity( tempGoal );
					}
					*/
					//[/CoOp]

					enemy_lost = qtrue;
				}
			}
		}
		//else, we can see him or we can't track him at all

		//every few seconds, decide if we should we advance or retreat?
		Jedi_CombatTimersUpdate( enemy_dist );

		//We call this even if lost enemy to keep him moving and to update the taunting behavior
		//maintain a distance from enemy appropriate for our aggression level
		Jedi_CombatDistance( enemy_dist );
	//[CoOp]
	}
	//[/CoOp]

	//if ( !enemy_lost )
	//[CoOp]
	if (NPC->client->NPC_class != CLASS_BOBAFETT)
	//[/CoOp]
	{
		//Update our seen enemy position
		if ( !NPC->enemy->client || ( NPC->enemy->client->ps.groundEntityNum != ENTITYNUM_NONE && NPC->client->ps.groundEntityNum != ENTITYNUM_NONE ) )
		{
			VectorCopy( NPC->enemy->r.currentOrigin, NPCInfo->enemyLastSeenLocation );
		}
		NPCInfo->enemyLastSeenTime = level.time;
	}

	//Turn to face the enemy
	//[CoOp]
	if ( TIMER_Done( NPC, "noturn" ) && !NPC_Jumping() )
	//if ( TIMER_Done( NPC, "noturn" ) )
	//[/CoOp]
	{
		Jedi_FaceEnemy( qtrue );
	}
	NPC_UpdateAngles( qtrue, qtrue );
	
	//Check for evasion
	if ( TIMER_Done( NPC, "parryTime" ) )
	{//finished parrying
		if ( NPC->client->ps.saberBlocked != BLOCKED_ATK_BOUNCE &&
			NPC->client->ps.saberBlocked != BLOCKED_PARRY_BROKEN )
		{//wasn't blocked myself
			NPC->client->ps.saberBlocked = BLOCKED_NONE;
		}
	}
	if ( NPC->enemy->s.weapon == WP_SABER )
	{
		Jedi_EvasionSaber( enemy_movedir, enemy_dist, enemy_dir );
	}
	else
	{//do we need to do any evasion for other kinds of enemies?
	}

	//apply strafing/walking timers, etc.
	Jedi_TimersApply();

	//[CoOp]
	if ( TIMER_Done( NPC, "allyJediDelay" ) )
	{//not holding back
		//[CoOp]
		if ( ( !NPC->client->ps.saberInFlight	//saber in had
			|| (NPC->client->ps.fd.saberAnimLevel == SS_DUAL 
			&& NPC->client->ps.saberHolstered != 2) )	//or we have another saber in our hands
			&& (!(NPC->client->ps.fd.forcePowersActive&(1<<FP_GRIP))
			||NPC->client->ps.fd.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2) )
		//if ( !NPC->client->ps.saberInFlight && (!(NPC->client->ps.fd.forcePowersActive&(1<<FP_GRIP))||NPC->client->ps.fd.forcePowerLevel[FP_GRIP] < FORCE_LEVEL_2) )
		{//not throwing saber or using force grip
			//see if we can attack
			if ( !Jedi_AttackDecide( enemy_dist ) )
			{//we're not attacking, decide what else to do
				Jedi_CombatIdle( enemy_dist );
				//FIXME: lower aggression when actually strike offensively?  Or just when do damage?
			}
			else
			{//we are attacking
				//stop taunting
				TIMER_Set( NPC, "taunting", -level.time );
			}
		}
		else
		{
		}
	//[CoOp]
		if ( NPC->client->NPC_class == CLASS_BOBAFETT )
		{
			Boba_FireDecide();
		}
		else if ( NPC->client->NPC_class == CLASS_ROCKETTROOPER )
		{
			RT_FireDecide();
		}
	
	}
	//[/CoOp]

	//Check for certain enemy special moves
	Jedi_CheckEnemyMovement( enemy_dist );
	//Make sure that we don't jump off ledges over long drops
	Jedi_CheckJumps();
	//Just make sure we don't strafe into walls or off cliffs
	//[CoOp]
		if ( VectorCompare( NPC->client->ps.moveDir, vec3_origin )//stomped the NAV system's moveDir
		&& !NPC_MoveDirClear( ucmd.forwardmove, ucmd.rightmove, qtrue ) )//check ucmd-driven movement
	//if ( !NPC_MoveDirClear( ucmd.forwardmove, ucmd.rightmove, qtrue ) )
	//[/CoOp]
	{//uh-oh, we are going to fall or hit something
		/*
		navInfo_t	info;
		//Get the move info
		NAV_GetLastMove( &info );
		if ( !(info.flags & NIF_MACRO_NAV) )
		{//micro-navigation told us to step off a ledge, try using macronav for now
			NPC_MoveToGoal( qfalse );
		}
		*/
		//reset the timers.
		TIMER_Set( NPC, "strafeLeft", 0 );
		TIMER_Set( NPC, "strafeRight", 0 );
	}
}

/*
==========================================================================================
EXTERNALLY CALLED BEHAVIOR STATES
==========================================================================================
*/

/*
-------------------------
NPC_Jedi_Pain
-------------------------
*/

void NPC_Jedi_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	//[CoOp]
	int mod = gPainMOD;
	//[/CoOp]
	gentity_t *other = attacker;
	vec3_t point;

	VectorCopy(gPainPoint, point);

	//FIXME: base the actual aggression add/subtract on health?
	//FIXME: don't do this more than once per frame?
	//FIXME: when take pain, stop force gripping....?
	if ( other->s.weapon == WP_SABER )
	{//back off
		TIMER_Set( self, "parryTime", -1 );
		if ( self->client->NPC_class == CLASS_DESANN || !Q_stricmp("Yoda",self->NPC_type) )
		{//less for Desann
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3-g_spskill.integer)*50;
		}
		else if ( self->NPC->rank >= RANK_LT_JG )
		{
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3-g_spskill.integer)*100;//300
		}
		else
		{
			self->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + (3-g_spskill.integer)*200;//500
		}
		if ( !Q_irand( 0, 3 ) )
		{//ouch... maybe switch up which saber power level we're using
			Jedi_AdjustSaberAnimLevel( self, Q_irand( FORCE_LEVEL_1, FORCE_LEVEL_3 ) );
		}
		if ( !Q_irand( 0, 1 ) )//damage > 20 || self->health < 40 || 
		{
			//Com_Printf( "(%d) drop agg - hit by saber\n", level.time );
			Jedi_Aggression( self, -1 );
		}
		if ( d_JediAI.integer )
		{
			Com_Printf( "(%d) PAIN: agg %d, no parry until %d\n", level.time, self->NPC->stats.aggression, level.time+500 );
		}
		//for testing only
		// Figure out what quadrant the hit was in.
		if ( d_JediAI.integer )
		{
			vec3_t	diff, fwdangles, right;
			float rightdot, zdiff;

			VectorSubtract( point, self->client->renderInfo.eyePoint, diff );
			diff[2] = 0;
			fwdangles[1] = self->client->ps.viewangles[1];
			AngleVectors( fwdangles, NULL, right, NULL );
			rightdot = DotProduct(right, diff);
			zdiff = point[2] - self->client->renderInfo.eyePoint[2];
		
			Com_Printf( "(%d) saber hit at height %4.2f, zdiff: %4.2f, rightdot: %4.2f\n", level.time, point[2]-self->r.absmin[2],zdiff,rightdot);
		}
	}
	else
	{//attack
		//Com_Printf( "(%d) raise agg - hit by ranged\n", level.time );
		Jedi_Aggression( self, 1 );
	}

	self->NPC->enemyCheckDebounceTime = 0;

	WP_ForcePowerStop( self, FP_GRIP );

	//NPC_Pain( self, inflictor, other, point, damage, mod );
	NPC_Pain(self, attacker, damage);

	if ( !damage && self->health > 0 )
	{//FIXME: better way to know I was pushed
		G_AddVoiceEvent( self, Q_irand(EV_PUSHED1, EV_PUSHED3), 2000 );
	}

	//drop me from the ceiling if I'm on it
	if ( Jedi_WaitingAmbush( self ) )
	{
		self->client->noclip = qfalse;
	}
	if ( self->client->ps.legsAnim == BOTH_CEILING_CLING )
	{
		NPC_SetAnim( self, SETANIM_LEGS, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	}
	if ( self->client->ps.torsoAnim == BOTH_CEILING_CLING )
	{
		NPC_SetAnim( self, SETANIM_TORSO, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	}

	//[CoOp]
		//check special defenses
	if ( other 
		&& other->client 
		&& !OnSameTeam( self, other ))
	{//hit by a client
		//FIXME: delay this until *after* the pain anim?
		if ( mod == MOD_FORCE_DARK )
		{//see if we should turn on absorb
			if ( (self->client->ps.fd.forcePowersKnown&(1<<FP_ABSORB)) != 0 
				&& (self->client->ps.fd.forcePowersActive&(1<<FP_ABSORB)) == 0 )
			{//know absorb and not already using it
				if ( other->s.number >= MAX_CLIENTS //enemy is an NPC
					|| Q_irand( 0, g_spskill.integer+1 ) )//enemy is player
				{
					if ( Q_irand( 0, self->NPC->rank ) > RANK_ENSIGN )
					{
						if ( !Q_irand( 0, 5 ) )
						{
							ForceAbsorb( self );
						}
					}
				}
			}
		}
		else if ( damage > Q_irand( 5, 20 ) )
		{//respectable amount of normal damage
			if ( (self->client->ps.fd.forcePowersKnown&(1<<FP_PROTECT)) != 0 
				&& (self->client->ps.fd.forcePowersActive&(1<<FP_PROTECT)) == 0 )
			{//know protect and not already using it
				if ( other->s.number >= MAX_CLIENTS //enemy is an NPC
					|| Q_irand( 0, g_spskill.integer+1 ) )//enemy is player
				{
					if ( Q_irand( 0, self->NPC->rank ) > RANK_ENSIGN )
					{ 
						if ( !Q_irand( 0, 1 ) )
						{
							if ( other->s.number < MAX_CLIENTS
								&& ((self->NPC->aiFlags&NPCAI_BOSS_CHARACTER)
									|| self->client->NPC_class==CLASS_SHADOWTROOPER) 
								&& Q_irand(0, 6-g_spskill.integer) ) 
							{ 
							}
							else
							{
								ForceProtect( self );
							}
						}
					}
				}
			}
		}
	}
	//[/CoOp]
}

qboolean Jedi_CheckDanger( void )
{
	//[CoOp]
	int alertEvent = NPC_CheckAlertEvents( qtrue, qtrue, -1, qfalse, AEL_MINOR, qfalse );
	//int alertEvent = NPC_CheckAlertEvents( qtrue, qtrue, -1, qfalse, AEL_MINOR );
	//[/CoOp]
	if ( level.alertEvents[alertEvent].level >= AEL_DANGER )
	{//run away!
		if ( !level.alertEvents[alertEvent].owner 
			|| !level.alertEvents[alertEvent].owner->client 
			|| (level.alertEvents[alertEvent].owner!=NPC&&level.alertEvents[alertEvent].owner->client->playerTeam!=NPC->client->playerTeam) )
		{//no owner
			return qfalse;
		}
		G_SetEnemy( NPC, level.alertEvents[alertEvent].owner );
		NPCInfo->enemyLastSeenTime = level.time;
		TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2500 ) );
		return qtrue;
	}
	return qfalse;
}

qboolean Jedi_CheckAmbushPlayer( void )
{//racc - checks to see if we should come out out of our ambush point to attack a player.
	int i = 0;
	gentity_t *player;
	float target_dist;
	float zDiff;

	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		player = &g_entities[i];

		if ( !player || !player->client )
		{
			continue;
		}

		if ( !NPC_ValidEnemy( player ) )
		{
			continue;
		}

//		if ( NPC->client->ps.powerups[PW_CLOAKED] || g_crosshairEntNum != NPC->s.number )
		if (NPC->client->ps.powerups[PW_CLOAKED] || !NPC_SomeoneLookingAtMe(NPC)) //rwwFIXMEFIXME: Need to pay attention to who is under crosshair for each player or something.
		{//if I'm not cloaked and the player's crosshair is on me, I will wake up, otherwise do this stuff down here...
			if ( !trap_InPVS( player->r.currentOrigin, NPC->r.currentOrigin ) )
			{//must be in same room
				continue;
			}
			else
			{
				if ( !NPC->client->ps.powerups[PW_CLOAKED] )
				{
					NPC_SetLookTarget( NPC, 0, 0 );
				}
			}
			zDiff = NPC->r.currentOrigin[2]-player->r.currentOrigin[2];
			if ( zDiff <= 0 || zDiff > 512 )
			{//never ambush if they're above me or way way below me
				continue;
			}

			//If the target is this close, then wake up regardless
			if ( (target_dist = DistanceHorizontalSquared( player->r.currentOrigin, NPC->r.currentOrigin )) > 4096 )
			{//closer than 64 - always ambush
				if ( target_dist > 147456 )
				{//> 384, not close enough to ambush
					continue;
				}
				//Check FOV first
				if ( NPC->client->ps.powerups[PW_CLOAKED] )
				{
					if ( InFOV( player, NPC, 30, 90 ) == qfalse )
					{
						continue;
					}
				}
				else
				{
					if ( InFOV( player, NPC, 45, 90 ) == qfalse )
					{
						continue;
					}
				}
			}

			if ( !NPC_ClearLOS4( player ) )
			{
				continue;
			}
		}

		//Got him, return true;
		G_SetEnemy( NPC, player );
		NPCInfo->enemyLastSeenTime = level.time;
		TIMER_Set( NPC, "attackDelay", Q_irand( 500, 2500 ) );
		return qtrue;
	}

	//Didn't get anyone.
	return qfalse;
}

void Jedi_Ambush( gentity_t *self )
{//racc - drop off the wall to ambush someone.
	self->client->noclip = qfalse;
//	self->client->ps.pm_flags |= PMF_JUMPING|PMF_SLOW_MO_FALL;
	NPC_SetAnim( self, SETANIM_BOTH, BOTH_CEILING_DROP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
	self->client->ps.weaponTime = self->client->ps.torsoTimer; //NPC->client->ps.torsoTimer; //what the?
	//[CoOp]
	if ( self->client->NPC_class != CLASS_BOBAFETT 
		&& self->client->NPC_class != CLASS_ROCKETTROOPER )
	//if ( self->client->NPC_class != CLASS_BOBAFETT )
	//[/CoOp]
	{
		WP_ActivateSaber(self);
	}
	Jedi_Decloak( self );
	G_AddVoiceEvent( self, Q_irand( EV_ANGER1, EV_ANGER3 ), 1000 );
}

qboolean Jedi_WaitingAmbush( gentity_t *self )
{//racc - we're waiting to ambush someone.
	if ( (self->spawnflags&JSF_AMBUSH) && self->client->noclip )
	{
		return qtrue;
	}
	return qfalse;
}
/*
-------------------------
Jedi_Patrol
-------------------------
*/

static void Jedi_Patrol( void )
{//racc - patrol behavior
	NPC->client->ps.saberBlocked = BLOCKED_NONE;

	if ( Jedi_WaitingAmbush( NPC ) )
	{//hiding on the ceiling
		NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_CEILING_CLING, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
		if ( NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES )
		{//look for enemies
			if ( Jedi_CheckAmbushPlayer() || Jedi_CheckDanger() )
			{//found him!
				Jedi_Ambush( NPC );
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
		}
	}
	else if ( NPCInfo->scriptFlags&SCF_LOOK_FOR_ENEMIES )//NPCInfo->scriptFlags & SCF_CHASE_ENEMIES )
	{//look for enemies
		gentity_t *best_enemy = NULL;
		float	best_enemy_dist = Q3_INFINITE;
		int i;
		for ( i = 0; i < ENTITYNUM_WORLD; i++ )
		{
			gentity_t *enemy = &g_entities[i];
			float	enemy_dist;
			//[CoOp]
			if ( enemy && enemy->client && NPC_ValidEnemy( enemy ))
			//if ( enemy && enemy->client && NPC_ValidEnemy( enemy ) && enemy->client->playerTeam == NPC->client->enemyTeam )
			//[/CoOp]
			{
				if ( trap_InPVS( NPC->r.currentOrigin, enemy->r.currentOrigin ) )
				{//we could potentially see him
					enemy_dist = DistanceSquared( NPC->r.currentOrigin, enemy->r.currentOrigin );
					if ( enemy->s.eType == ET_PLAYER || enemy_dist < best_enemy_dist )
					{//racc - players have priority
						//if the enemy is close enough, or threw his saber, take him as the enemy
						//FIXME: what if he throws a thermal detonator?
						if ( enemy_dist < (220*220) || ( NPCInfo->investigateCount>= 3 && !NPC->client->ps.saberHolstered ) )
						{
							G_SetEnemy( NPC, enemy );
							//NPCInfo->behaviorState = BS_HUNT_AND_KILL;//should be auto now
							NPCInfo->stats.aggression = 3;
							break;
						}
						else if ( enemy->client->ps.saberInFlight && !enemy->client->ps.saberHolstered )
						{//threw his saber, see if it's heading toward me and close enough to consider a threat
							float	saberDist;
							vec3_t	saberDir2Me;
							vec3_t	saberMoveDir;
							gentity_t *saber = &g_entities[enemy->client->ps.saberEntityNum];
							VectorSubtract( NPC->r.currentOrigin, saber->r.currentOrigin, saberDir2Me );
							saberDist = VectorNormalize( saberDir2Me );
							VectorCopy( saber->s.pos.trDelta, saberMoveDir );
							VectorNormalize( saberMoveDir );
							if ( DotProduct( saberMoveDir, saberDir2Me ) > 0.5 )
							{//it's heading towards me
								if ( saberDist < 200 )
								{//incoming!
									G_SetEnemy( NPC, enemy );
									//NPCInfo->behaviorState = BS_HUNT_AND_KILL;//should be auto now
									NPCInfo->stats.aggression = 3;
									break;
								}
							}
						}
						best_enemy_dist = enemy_dist;
						best_enemy = enemy;
					}
				}
			}
		}
		if ( !NPC->enemy )
		{//still not mad
			if ( !best_enemy )
			{
				//Com_Printf( "(%d) drop agg - no enemy (patrol)\n", level.time );
				Jedi_AggressionErosion(-1);
				//FIXME: what about alerts?  But not if ignore alerts
			}
			else
			{//have one to consider
				if ( NPC_ClearLOS4( best_enemy ) )
				{//we have a clear (of architecture) LOS to him
					//[CoOp]
					/* racc - this flag isn't in MP and appears to be broken in SP.
					//I've included it for possible implimentation later.
					if ( (NPCInfo->aiFlags&NPCAI_NO_JEDI_DELAY) )
					{//just get mad right away
						if ( DistanceHorizontalSquared( NPC->currentOrigin, best_enemy->currentOrigin ) < (1024*1024) )
						{
							G_SetEnemy( NPC, best_enemy );
							NPCInfo->stats.aggression = 20;
						}
					}
					else if ( best_enemy->s.number )
					*/
					//[/CoOp]
					if ( best_enemy->s.number )
					{//just attack
						G_SetEnemy( NPC, best_enemy );
						NPCInfo->stats.aggression = 3;
					}
					else if ( NPC->client->NPC_class != CLASS_BOBAFETT )
					{//the player, toy with him
						//get progressively more interested over time
						if ( TIMER_Done( NPC, "watchTime" ) )
						{//we want to pick him up in stages
							if ( TIMER_Get( NPC, "watchTime" ) == -1 )
							{//this is the first time, we'll ignore him for a couple seconds
								TIMER_Set( NPC, "watchTime", Q_irand( 3000, 5000 ) );
								goto finish;
							}
							else
							{//okay, we've ignored him, now start to notice him
								if ( !NPCInfo->investigateCount )
								{
									G_AddVoiceEvent( NPC, Q_irand( EV_JDETECTED1, EV_JDETECTED3 ), 3000 );
								}
								NPCInfo->investigateCount++;
								TIMER_Set( NPC, "watchTime", Q_irand( 4000, 10000 ) );
							}
						}
						//while we're waiting, do what we need to do
						if ( best_enemy_dist < (440*440) || NPCInfo->investigateCount >= 2 )
						{//stage three: keep facing him
							NPC_FaceEntity( best_enemy, qtrue );
							if ( best_enemy_dist < (330*330) )
							{//stage four: turn on the saber
								if ( !NPC->client->ps.saberInFlight )
								{
									WP_ActivateSaber(NPC);
								}
							}
						}
						else if ( best_enemy_dist < (550*550) || NPCInfo->investigateCount == 1 )
						{//stage two: stop and face him every now and then
							if ( TIMER_Done( NPC, "watchTime" ) )
							{
								NPC_FaceEntity( best_enemy, qtrue );
							}
						}
						else
						{//stage one: look at him.
							NPC_SetLookTarget( NPC, best_enemy->s.number, 0 );
						}
					}
				}
				else if ( TIMER_Done( NPC, "watchTime" ) )
				{//haven't seen him in a bit, clear the lookTarget
					NPC_ClearLookTarget( NPC );
				}
			}
		}
	}
finish:
	//If we have somewhere to go, then do that
	if ( UpdateGoal() )
	{
		ucmd.buttons |= BUTTON_WALKING;
		//Jedi_Move( NPCInfo->goalEntity );
		NPC_MoveToGoal( qtrue );
	}

	NPC_UpdateAngles( qtrue, qtrue );

	if ( NPC->enemy )
	{//just picked one up
		NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 3000, 10000 );
	}
}

qboolean Jedi_CanPullBackSaber( gentity_t *self )
{
	if ( self->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN && !TIMER_Done( self, "parryTime" ) )
	{
		return qfalse;
	}

	//[CoOp]
	if ( self->client->NPC_class == CLASS_SHADOWTROOPER
		|| self->client->NPC_class == CLASS_ALORA
		|| ( self->NPC && (self->NPC->aiFlags&NPCAI_BOSS_CHARACTER) ) )
	/*
	if ( self->client->NPC_class == CLASS_SHADOWTROOPER
		|| self->client->NPC_class == CLASS_TAVION
		|| self->client->NPC_class == CLASS_LUKE
		|| self->client->NPC_class == CLASS_DESANN 
		|| !Q_stricmp( "Yoda", self->NPC_type ) )
	*/
	//[/CoOp]
	{
		return qtrue;
	}

	if ( self->painDebounceTime > level.time )//|| self->client->ps.weaponTime > 0 )
	{
		return qfalse;
	}

	return qtrue;
}
/*
-------------------------
NPC_BSJedi_FollowLeader
-------------------------
*/
void NPC_BSJedi_FollowLeader( void )
{//basic follow leader AI for jedi NPCs
	NPC->client->ps.saberBlocked = BLOCKED_NONE;
	if ( !NPC->enemy )
	{
		//Com_Printf( "(%d) drop agg - no enemy (follow)\n", level.time );
		Jedi_AggressionErosion(-1);
	}
	
	//did we drop our saber?  If so, go after it!
	if ( NPC->client->ps.saberInFlight )
	{//saber is not in hand
		if ( NPC->client->ps.saberEntityNum < ENTITYNUM_NONE && NPC->client->ps.saberEntityNum > 0 )//player is 0
		{//
			if ( g_entities[NPC->client->ps.saberEntityNum].s.pos.trType == TR_STATIONARY )
			{//fell to the ground, try to pick it up... 
				if ( Jedi_CanPullBackSaber( NPC ) )
				{
					//FIXME: if it's on the ground and we just pulled it back to us, should we
					//		stand still for a bit to make sure it gets to us...?
					//		otherwise we could end up running away from it while it's on its
					//		way back to us and we could lose it again.
					NPC->client->ps.saberBlocked = BLOCKED_NONE;
					NPCInfo->goalEntity = &g_entities[NPC->client->ps.saberEntityNum];
					ucmd.buttons |= BUTTON_ATTACK;
					if ( NPC->enemy && NPC->enemy->health > 0 )
					{//get our saber back NOW!
						if ( !NPC_MoveToGoal( qtrue ) )//Jedi_Move( NPCInfo->goalEntity, qfalse );
						{//can't nav to it, try jumping to it
							NPC_FaceEntity( NPCInfo->goalEntity, qtrue );
							Jedi_TryJump( NPCInfo->goalEntity );
						}
						NPC_UpdateAngles( qtrue, qtrue );
						return;
					}
				}
			}
		}
	}

	//[CoOp]
	//not in SP version of code
	/*
	if ( NPCInfo->goalEntity )
	{
		trace_t	trace;

		if ( Jedi_Jumping( NPCInfo->goalEntity ) )
		{//in mid-jump
			return;
		}

		if ( !NAV_CheckAhead( NPC, NPCInfo->goalEntity->r.currentOrigin, &trace, ( NPC->clipmask & ~CONTENTS_BODY )|CONTENTS_BOTCLIP ) )
		{//can't get straight to him
			if ( NPC_ClearLOS4( NPCInfo->goalEntity ) && NPC_FaceEntity( NPCInfo->goalEntity, qtrue ) )
			{//no line of sight
				if ( Jedi_TryJump( NPCInfo->goalEntity ) )
				{//started a jump
					return;
				}
			}
		}
		if ( NPCInfo->aiFlags & NPCAI_BLOCKED )
		{//try to jump to the blockedDest
			if ( fabs(NPCInfo->blockedDest[2]-NPC->r.currentOrigin[2]) > 64 )
			{
				gentity_t *tempGoal = G_Spawn();//ugh, this is NOT good...?
				G_SetOrigin( tempGoal, NPCInfo->blockedDest );
				trap_LinkEntity( tempGoal );
				TIMER_Set( NPC, "jumpChaseDebounce", -1 );
				if ( Jedi_TryJump( tempGoal ) )
				{//going to jump to the dest
					G_FreeEntity( tempGoal );
					return;
				}
				G_FreeEntity( tempGoal );
			}
		}
	}
	*/
	//[/CoOp]

	//try normal movement
	NPC_BSFollowLeader();

	//[CoOp]
	//we're following and are injured, heal at intervals while no enemy is around
	if (!NPC->enemy &&  
		NPC->health < NPC->client->pers.maxHealth && 
		(NPC->client->ps.fd.forcePowersKnown&(1<<FP_HEAL)) != 0  &&
        (NPC->client->ps.fd.forcePowersActive&(1<<FP_HEAL)) == 0 &&
		TIMER_Done(NPC, "FollowHealDebouncer"))
	{
		if (Q_irand(0,3)==0)
		{
			TIMER_Set(NPC, "FollowHealDebouncer", Q_irand(12000, 18000));
			ForceHeal( NPC );
		}
		else
		{
			TIMER_Set(NPC, "FollowHealDebouncer", Q_irand(1000, 2000));
		}
	}
	//[/CoOp]
}


//[CoOp]
qboolean Jedi_CheckKataAttack( void )
{//decide if we want to do a kata and do it
	if ( NPCInfo->rank >= RANK_LT_COMM )
	{//only top-level guys and bosses do this
		if ( (ucmd.buttons&BUTTON_ATTACK) )
		{//attacking
			if ( !(ucmd.buttons&BUTTON_ALT_ATTACK) )
			{//not already going to do a kata move somehow
				if ( NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
				{//on the ground
					if ( ucmd.upmove <= 0 && NPC->client->ps.fd.forceJumpCharge <= 0 )
					{//not going to try to jump
						/*
						if ( (NPCInfo->scriptFlags&SCF_NO_ACROBATICS) )
						{//uh-oh, no jumping moves!
							if ( NPC->client->ps.saberAnimLevel == SS_STAFF )
							{//this kata move has a jump in it...
								return qfalse;
							}
						}
						*/

						if ( Q_irand( 0, g_spskill.integer+1 ) //50% chance on easy, 66% on medium, 75% on hard
							&& !Q_irand( 0, 9 ) )//10% chance overall
						{//base on skill level
							ucmd.upmove = 0;
							VectorClear( NPC->client->ps.moveDir );
							ucmd.buttons |= BUTTON_ALT_ATTACK;
							return qtrue;
						}
					}
				}
			}
		}
	}
	return qfalse;
}
//[/CoOp]


/*
-------------------------
Jedi_Attack
-------------------------
*/

static void Jedi_Attack( void )
{
	//Don't do anything if we're in a pain anim
	if ( NPC->painDebounceTime > level.time )
	{
		if ( Q_irand( 0, 1 ) )
		{
			Jedi_FaceEnemy( qtrue );
		}
		NPC_UpdateAngles( qtrue, qtrue );
		//[CoOp]
		if ( NPC->client->ps.torsoAnim == BOTH_KYLE_GRAB )
		{//see if we grabbed enemy
			if ( NPC->client->ps.torsoTimer <= 200 )
			{
				if ( Kyle_CanDoGrab()
					&& NPC_EnemyRangeFromBolt( 0 ) < 88.0f )
				{//grab him!
					Kyle_GrabEnemy();
					return;
				}
				else
				{
					NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_KYLE_MISS, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					NPC->client->ps.weaponTime = NPC->client->ps.torsoTimer;
					return;
				}
			}
			//else just sit here?
		}
		//[/CoOp]
		return;
	}

	if ( NPC->client->ps.saberLockTime > level.time )
	{//racc - handle saberlock behavior.
		//FIXME: maybe if I'm losing I should try to force-push out of it?  Very rarely, though...
		if ( NPC->client->ps.fd.forcePowerLevel[FP_PUSH] > FORCE_LEVEL_2 
			&& NPC->client->ps.saberLockTime < level.time + 5000 
			&& !Q_irand( 0, 10 ))
		{
			ForceThrow( NPC, qfalse );
		}
		//based on my skill, hit attack button every other to every several frames in order to push enemy back
		else
		{
			float chance;
		
			if ( NPC->client->NPC_class == CLASS_DESANN || !Q_stricmp("Yoda",NPC->NPC_type) )
			{
				if ( g_spskill.integer )
				{
					chance = 4.0f;//he pushes *hard*
				}
				else
				{
					chance = 3.0f;//he pushes *hard*
				}
			}
			//[CoOp]
			else if ( NPC->client->NPC_class == CLASS_TAVION 
				|| NPC->client->NPC_class == CLASS_SHADOWTROOPER
				|| NPC->client->NPC_class == CLASS_ALORA
				|| (NPC->client->NPC_class == CLASS_KYLE&&(NPC->spawnflags&1)) )
			//else if ( NPC->client->NPC_class == CLASS_TAVION )
			//[/CoOp]
			{
				chance = 2.0f+g_spskill.value;//from 2 to 4
			}
			else
			{//the escalation in difficulty is nice, here, but cap it so it doesn't get *impossible* on hard
				float maxChance	= (float)(RANK_LT)/2.0f+3.0f;//5?
				if ( !g_spskill.value )
				{
					chance = (float)(NPCInfo->rank)/2.0f;
				}
				else
				{
					chance = (float)(NPCInfo->rank)/2.0f+1.0f;
				}
				if ( chance > maxChance )
				{
					chance = maxChance;
				}
			}
		//	if ( flrand( -4.0f, chance ) >= 0.0f && !(NPC->client->ps.pm_flags&PMF_ATTACK_HELD) )
		//	{
		//		ucmd.buttons |= BUTTON_ATTACK;
		//	}

			//[CoOp]
			if ( NPCInfo->aiFlags&NPCAI_BOSS_CHARACTER )
			{ 
				chance += Q_irand(0,2); 
			}
			else if ( NPCInfo->aiFlags&NPCAI_SUBBOSS_CHARACTER )
			{
				chance += Q_irand(-1,1);
			}
			//[/CoOp]
			//[CoOp]
			//racc - I'm attempting to fix the issue of the lack of a PMF_ATTACK_HELD
			else if ( flrand( /*-4.0f*/-20.0f, chance ) >= 0.0f )
			//have to let go of the attack button before we can click again.
			//if ( flrand( -4.0f, chance ) >= 0.0f )
			//[/CoOp]
			{
				ucmd.buttons |= BUTTON_ATTACK;
			}
			//rwwFIXMEFIXME: support for PMF_ATTACK_HELD
		}
		NPC_UpdateAngles( qtrue, qtrue );
		return;
	}
	//did we drop our saber?  If so, go after it!
	if ( NPC->client->ps.saberInFlight )
	{//saber is not in hand
	//	if ( NPC->client->ps.saberEntityNum < ENTITYNUM_NONE && NPC->client->ps.saberEntityNum > 0 )//player is 0
		if (!NPC->client->ps.saberEntityNum && NPC->client->saberStoredIndex) //this is valid, it's 0 when our saber is gone -rww (mp-specific)
		{
			//[CoOp]
			//switched back to SP method
			if ( g_entities[NPC->client->ps.saberEntityNum].s.pos.trType == TR_STATIONARY )
			//if (1) //no matter
			{//fell to the ground, try to pick it up
				if ( Jedi_CanPullBackSaber( NPC ) )
			//	if (1) //no matter
			//[/CoOp]
				{
					NPC->client->ps.saberBlocked = BLOCKED_NONE;
					NPCInfo->goalEntity = &g_entities[NPC->client->saberStoredIndex];
					ucmd.buttons |= BUTTON_ATTACK;
					if ( NPC->enemy && NPC->enemy->health > 0 )
					{//get our saber back NOW!
						Jedi_Move( NPCInfo->goalEntity, qfalse );
						NPC_UpdateAngles( qtrue, qtrue );
						if ( NPC->enemy->s.weapon == WP_SABER )
						{//be sure to continue evasion
							vec3_t	enemy_dir, enemy_movedir, enemy_dest;
							float	enemy_dist, enemy_movespeed;
							Jedi_SetEnemyInfo( enemy_dest, enemy_dir, &enemy_dist, enemy_movedir, &enemy_movespeed, 300 );
							Jedi_EvasionSaber( enemy_movedir, enemy_dist, enemy_dir );
						}
						return;
					}
				}
			}
		}
	}
	//see if our enemy was killed by us, gloat and turn off saber after cool down.
	//FIXME: don't do this if we have other enemies to fight...?
	if ( NPC->enemy )
	{
		if ( NPC->enemy->health <= 0 
			&& NPC->enemy->enemy == NPC 
			//[CoOp]
			//kyle gloats if he just offed a player
			&& (NPC->client->playerTeam != NPCTEAM_PLAYER||
			(NPC->client->NPC_class==CLASS_KYLE
			&&(NPC->spawnflags&1)&&NPC->s.number < MAX_CLIENTS)) )//good guys don't gloat (unless it's Kyle having just killed his student
			//&& NPC->client->playerTeam != NPCTEAM_PLAYER )//good guys don't gloat
			//[/CoOp]
		{//my enemy is dead and I killed him
			NPCInfo->enemyCheckDebounceTime = 0;//keep looking for others

			//[CoOp]
			if ( NPC->client->NPC_class == CLASS_BOBAFETT 
				|| (NPC->client->NPC_class == CLASS_REBORN && NPC->s.weapon != WP_SABER)
				|| NPC->client->NPC_class == CLASS_ROCKETTROOPER )
			//if ( NPC->client->NPC_class == CLASS_BOBAFETT )
			//[/CoOp]
			{//gloating for non saber jedi class NPCs
				if ( NPCInfo->walkDebounceTime < level.time && NPCInfo->walkDebounceTime >= 0 )
				{
					TIMER_Set( NPC, "gloatTime", 10000 );
					NPCInfo->walkDebounceTime = -1;
				}
				if ( !TIMER_Done( NPC, "gloatTime" ) )
				{//racc - walk over to the victim's body.
					if ( DistanceHorizontalSquared( NPC->client->renderInfo.eyePoint, NPC->enemy->r.currentOrigin ) > 4096 && (NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) )//64 squared
					{//racc - walk thrus the victim if we're 
						NPCInfo->goalEntity = NPC->enemy;
						Jedi_Move( NPC->enemy, qfalse );
						ucmd.buttons |= BUTTON_WALKING;
					}
					else
					{//racc - ok, turn off the gloat timer so we can talk the smack
						TIMER_Set( NPC, "gloatTime", 0 );
					}
				}
				else if ( NPCInfo->walkDebounceTime == -1 )
				{//racc - talk the smack
					NPCInfo->walkDebounceTime = -2;
					G_AddVoiceEvent( NPC, Q_irand( EV_VICTORY1, EV_VICTORY3 ), 3000 );
					jediSpeechDebounceTime[NPC->client->playerTeam] = level.time + 3000;
					NPCInfo->desiredPitch = 0;
					NPCInfo->goalEntity = NULL;
				}
				Jedi_FaceEnemy( qtrue );
				NPC_UpdateAngles( qtrue, qtrue );
				return;
			}
			else
			{//racc - saber weilder gloat code
				if ( !TIMER_Done( NPC, "parryTime" ) )
				{
					TIMER_Set( NPC, "parryTime", -1 );
					NPC->client->ps.fd.forcePowerDebounce[FP_SABER_DEFENSE] = level.time + 500;
				}
				NPC->client->ps.saberBlocked = BLOCKED_NONE;
				if ( !NPC->client->ps.saberHolstered && NPC->client->ps.saberInFlight )
				{//saber is still on (or we're trying to pull it back), count down erosion and keep facing the enemy
					//FIXME: need to stop this from happening over and over again when they're blocking their victim's saber
					//FIXME: turn off saber sooner so we get cool walk anim?
					//Com_Printf( "(%d) drop agg - enemy dead\n", level.time );
					Jedi_AggressionErosion(-3);
					if ( BG_SabersOff( &NPC->client->ps ) && !NPC->client->ps.saberInFlight )
					{//turned off saber (in hand), gloat
						G_AddVoiceEvent( NPC, Q_irand( EV_VICTORY1, EV_VICTORY3 ), 3000 );
						jediSpeechDebounceTime[NPC->client->playerTeam] = level.time + 3000;
						NPCInfo->desiredPitch = 0;
						NPCInfo->goalEntity = NULL;
					}
					TIMER_Set( NPC, "gloatTime", 10000 );
				}
				if ( !NPC->client->ps.saberHolstered || NPC->client->ps.saberInFlight || !TIMER_Done( NPC, "gloatTime" ) )
				{//keep walking
					if ( DistanceHorizontalSquared( NPC->client->renderInfo.eyePoint, NPC->enemy->r.currentOrigin ) > 4096 && (NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) )//64 squared
					{
						NPCInfo->goalEntity = NPC->enemy;
						Jedi_Move( NPC->enemy, qfalse );
						ucmd.buttons |= BUTTON_WALKING;
					}
					else
					{//got there
						//[CoOp]
						if ( NPC->health < NPC->client->pers.maxHealth )
						{//racc - add insult to injury by healing near their dead body
							if ( NPC->client->saber[0].type == SABER_SITH_SWORD 
								&& NPC->client->weaponGhoul2[0] )
							{
								Tavion_SithSwordRecharge();
							}
							else if ( (NPC->client->ps.fd.forcePowersKnown&(1<<FP_HEAL)) != 0 
								&& (NPC->client->ps.fd.forcePowersActive&(1<<FP_HEAL)) == 0 )
							{
								ForceHeal( NPC );
							}
						}
						/*
						if ( NPC->health < NPC->client->pers.maxHealth 
							&& (NPC->client->ps.fd.forcePowersKnown&(1<<FP_HEAL)) != 0 
							&& (NPC->client->ps.fd.forcePowersActive&(1<<FP_HEAL)) == 0 )
						{
							ForceHeal( NPC );
						}
						*/
						//[/CoOp]
					}
					Jedi_FaceEnemy( qtrue );
					NPC_UpdateAngles( qtrue, qtrue );
					return;
				}
			}
		}
	}

	//If we don't have an enemy, just idle
	if ( NPC->enemy->s.weapon == WP_TURRET && !Q_stricmp( "PAS", NPC->enemy->classname ) )
	{
		if ( NPC->enemy->count <= 0 )
		{//it's out of ammo
			if ( NPC->enemy->activator && NPC_ValidEnemy( NPC->enemy->activator ) )
			{
				gentity_t *turretOwner = NPC->enemy->activator;
				G_ClearEnemy( NPC );
				G_SetEnemy( NPC, turretOwner );
			}
			else
			{
				G_ClearEnemy( NPC );
			}
		}
	}
	//[CoOp]
	//charmed SP code
	if ( NPC->enemy->NPC  
		&& NPC->enemy->NPC->charmedTime > level.time )
	{//my enemy was charmed
		if ( OnSameTeam( NPC, NPC->enemy ) )
		{//has been charmed to be on my team
			G_ClearEnemy( NPC );
		}
	}
	if ( NPC->client->playerTeam == NPCTEAM_ENEMY
		&& NPC->client->enemyTeam == NPCTEAM_PLAYER
		&& NPC->enemy
		&& NPC->enemy->client
		&& NPC->enemy->client->playerTeam != NPC->client->enemyTeam 
		&& OnSameTeam( NPC, NPC->enemy ) 
		&& !(NPC->NPC->aiFlags&NPCAI_LOCKEDENEMY) )
	{//an evil jedi somehow got another evil NPC as an enemy, they were probably charmed and it's run out now
		if ( !NPC_ValidEnemy( NPC->enemy ) )
		{
			G_ClearEnemy( NPC );
		}
	}
	//[/CoOp]
	NPC_CheckEnemy( qtrue, qtrue, qtrue );

	if ( !NPC->enemy )
	{
		NPC->client->ps.saberBlocked = BLOCKED_NONE;
		if ( NPCInfo->tempBehavior == BS_HUNT_AND_KILL )
		{//lost him, go back to what we were doing before
			NPCInfo->tempBehavior = BS_DEFAULT;
			NPC_UpdateAngles( qtrue, qtrue );
			return;
		}
		Jedi_Patrol();//was calling Idle... why?
		return;
	}

	//always face enemy if have one
	NPCInfo->combatMove = qtrue;

	//Track the player and kill them if possible
	Jedi_Combat();

	if ( !(NPCInfo->scriptFlags&SCF_CHASE_ENEMIES) 
		|| ((NPC->client->ps.fd.forcePowersActive&(1<<FP_HEAL))&&NPC->client->ps.fd.forcePowerLevel[FP_HEAL]<FORCE_LEVEL_2))
	{//this is really stupid, but okay...
		//racc - either we're not to chase enemies or we're using a lower level heal.
		ucmd.forwardmove = 0;
		ucmd.rightmove = 0;
		if ( ucmd.upmove > 0 )
		{
			ucmd.upmove = 0;
		}
		NPC->client->ps.fd.forceJumpCharge = 0;
		VectorClear( NPC->client->ps.moveDir );
	}

	//NOTE: for now, we clear ucmd.forwardmove & ucmd.rightmove while in air to avoid jumps going awry...
	if ( NPC->client->ps.groundEntityNum == ENTITYNUM_NONE )
	{//don't push while in air, throws off jumps!
		//FIXME: if we are in the air over a drop near a ledge, should we try to push back towards the ledge?
		ucmd.forwardmove = 0;
		ucmd.rightmove = 0;
		VectorClear( NPC->client->ps.moveDir );
	}

	if ( !TIMER_Done( NPC, "duck" ) )
	{
		ucmd.upmove = -127;
	}

	//[CoOp]
	if ( NPC->client->NPC_class != CLASS_BOBAFETT
		&& (NPC->client->NPC_class != CLASS_REBORN || NPC->s.weapon == WP_SABER)
		&& NPC->client->NPC_class != CLASS_ROCKETTROOPER )
	//if ( NPC->client->NPC_class != CLASS_BOBAFETT )
	//[/CoOp]
	{
		if ( PM_SaberInBrokenParry( NPC->client->ps.saberMove ) || NPC->client->ps.saberBlocked == BLOCKED_PARRY_BROKEN )
		{//just make sure they don't pull their saber to them if they're being blocked
			ucmd.buttons &= ~BUTTON_ATTACK;
		}
	}

	if( (NPCInfo->scriptFlags&SCF_DONT_FIRE) //not allowed to attack
		|| ((NPC->client->ps.fd.forcePowersActive&(1<<FP_HEAL))&&NPC->client->ps.fd.forcePowerLevel[FP_HEAL]<FORCE_LEVEL_3)
		|| ((NPC->client->ps.saberEventFlags&SEF_INWATER)&&!NPC->client->ps.saberInFlight) )//saber in water
	{
		ucmd.buttons &= ~(BUTTON_ATTACK|BUTTON_ALT_ATTACK);
	}

	if ( NPCInfo->scriptFlags&SCF_NO_ACROBATICS )
	{
		ucmd.upmove = 0;
		NPC->client->ps.fd.forceJumpCharge = 0;
	}

	//[CoOp]
	if ( NPC->client->NPC_class != CLASS_BOBAFETT
		&& (NPC->client->NPC_class != CLASS_REBORN || NPC->s.weapon == WP_SABER)
		&& NPC->client->NPC_class != CLASS_ROCKETTROOPER )
	//if ( NPC->client->NPC_class != CLASS_BOBAFETT )
	//[/CoOp]
	{
		Jedi_CheckDecreaseSaberAnimLevel();
	}

	if ( ucmd.buttons & BUTTON_ATTACK && NPC->client->playerTeam == NPCTEAM_ENEMY )
	{
		if ( Q_irand( 0, NPC->client->ps.fd.saberAnimLevel ) > 0 
			&& Q_irand( 0, NPC->client->pers.maxHealth+10 ) > NPC->health 
			&& !Q_irand( 0, 3 ))
		{//the more we're hurt and the stronger the attack we're using, the more likely we are to make a anger noise when we swing
			G_AddVoiceEvent( NPC, Q_irand( EV_COMBAT1, EV_COMBAT3 ), 1000 );
		}
	}

	//[CoOp]
	//Check for trying a kata move
	//FIXME: what about force-pull attacks?
	if ( Jedi_CheckKataAttack() )
	{//doing a kata attack
	}
	else
	{//check other special combat behavior
		if ( NPC->client->NPC_class != CLASS_BOBAFETT 
			&& (NPC->client->NPC_class != CLASS_REBORN || NPC->s.weapon == WP_SABER)
			&& NPC->client->NPC_class != CLASS_ROCKETTROOPER )
		//if ( NPC->client->NPC_class != CLASS_BOBAFETT )
		{//saber wielders only.
			if ( NPC->client->NPC_class == CLASS_TAVION 
				|| NPC->client->NPC_class == CLASS_SHADOWTROOPER
				|| NPC->client->NPC_class == CLASS_ALORA
				|| (g_spskill.integer && ( NPC->client->NPC_class == CLASS_DESANN || NPCInfo->rank >= Q_irand( RANK_CREWMAN, RANK_CAPTAIN ))))
			//if ( NPC->client->NPC_class == CLASS_TAVION 
			//	|| (g_spskill.integer && ( NPC->client->NPC_class == CLASS_DESANN || NPCInfo->rank >= Q_irand( RANK_CREWMAN, RANK_CAPTAIN ))))
			{//certain NPCS will kick in force speed if the player does...
				if ( NPC->enemy 
					&& !NPC->enemy->s.number 
					&& NPC->enemy->client 
					&& (NPC->enemy->client->ps.fd.forcePowersActive & (1<<FP_SPEED)) 
					&& !(NPC->client->ps.fd.forcePowersActive & (1<<FP_SPEED)) )
				{
					int chance = 0;
					switch ( g_spskill.integer )
					{
					case 0:
						chance = 9;
					case 1:
						chance = 3;
					case 2:
						chance = 1;
						break;
					}
					if ( !Q_irand( 0, chance ) )
					{
						ForceSpeed( NPC, 0 );
					}
				}
			}
		}
		//Sometimes Alora flips towards you instead of runs
		if ( NPC->client->NPC_class == CLASS_ALORA )
		{
			if ( (ucmd.buttons&BUTTON_ALT_ATTACK) )
			{//chance of doing a special dual saber throw
				if ( NPC->client->ps.fd.saberAnimLevel == SS_DUAL
					&& !NPC->client->ps.saberInFlight )
				{//has dual sabers and haven't already tossed the saber.
					if ( Distance( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin ) >= 120 )
					{
						NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_ALORA_SPIN_THROW, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
						NPC->client->ps.weaponTime = NPC->client->ps.torsoTimer;
						//FIXME: don't move
						//FIXME: sabers need trails and sounds
					}
				}
			}
			else if ( NPC->enemy
				&& ucmd.forwardmove > 0 
				&& fabs((float)ucmd.rightmove) < 32
				&& !(ucmd.buttons&BUTTON_WALKING)
				&& !(ucmd.buttons&BUTTON_ATTACK)
				&& NPC->client->ps.saberMove == LS_READY
				&& NPC->client->ps.legsAnim == BOTH_RUN_DUAL )
			{//running at us, not attacking
				if ( Distance( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin ) > 80 )
				{
					if ( NPC->client->ps.legsAnim == BOTH_FLIP_F
						|| NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_1
						|| NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_2
						|| NPC->client->ps.legsAnim == BOTH_ALORA_FLIP_3 )
					{
						if ( NPC->client->ps.legsTimer <= 200 && Q_irand( 0, 2 ) )
						{//go ahead and start anotther
							NPC_SetAnim( NPC, SETANIM_BOTH, Q_irand(BOTH_ALORA_FLIP_1,BOTH_ALORA_FLIP_3), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
						}
					}
					else if ( !Q_irand( 0, 6 ) )
					{
						NPC_SetAnim( NPC, SETANIM_BOTH, Q_irand(BOTH_ALORA_FLIP_1,BOTH_ALORA_FLIP_3), SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD);
					}
				}
			}
		}
	}

	if ( VectorCompare( NPC->client->ps.moveDir, vec3_origin )
		&& (ucmd.forwardmove||ucmd.rightmove) )
	{//using ucmds to move this turn, not NAV
		if ( (ucmd.buttons&BUTTON_WALKING) )
		{//FIXME: NAV system screws with speed directly, so now I have to re-set it myself!
			NPC->client->ps.speed = NPCInfo->stats.walkSpeed;
		}
		else
		{
			NPC->client->ps.speed = NPCInfo->stats.runSpeed;
		}
	}
	//[/CoOp]
}


//[CoOp]
//added all SP code used for the Rosh boss fight
qboolean Rosh_BeingHealed( gentity_t *self )
{//Is this Rosh being healed?
	if ( self
		&& self->NPC 
		&& self->client
		&& (self->NPC->aiFlags&NPCAI_ROSH) 
		&& (self->flags&FL_UNDYING)
		&& ( self->health == 1 //need healing
			|| self->flags&FL_GODMODE) )
			//|| self->client->ps.powerups[PW_INVINCIBLE] > level.time ) )//being healed
	{
		return qtrue;
	}
	return qfalse;
}

qboolean Rosh_TwinPresent( gentity_t *self )
{//look for the healer twins for the Rosh boss.
	gentity_t *foundTwin = G_Find( NULL, FOFS(NPC_type), "DKothos" );
	if ( !foundTwin 
		|| foundTwin->health < 0 )
	{
		foundTwin = G_Find( NULL, FOFS(NPC_type), "VKothos" );
	}
	if ( !foundTwin 
		|| foundTwin->health < 0 )
	{//oh well, both twins are dead...
		return qfalse;
	}
	return qtrue;
}

extern qboolean G_ClearLineOfSight(const vec3_t point1, const vec3_t point2, int ignore, int clipmask);
qboolean Kothos_HealRosh( void )
{//this NPC attempts to heal the heal target (the rosh boss normally).
	if ( NPC->client
		&& NPC->client->leader
		&& NPC->client->leader->client )
	{
		if ( DistanceSquared( NPC->client->leader->r.currentOrigin, NPC->r.currentOrigin ) <= (256*256)
			&& G_ClearLineOfSight( NPC->client->leader->client->renderInfo.eyePoint, NPC->client->renderInfo.eyePoint, NPC->s.number, MASK_OPAQUE ) )
		{
			NPC_SetAnim( NPC, SETANIM_TORSO, BOTH_FORCE_2HANDEDLIGHTNING_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			NPC->client->ps.torsoTimer = 1000;
			
			if ( NPC->ghoul2 )
			{
				mdxaBone_t	boltMatrix;
				vec3_t		fxOrg, fxDir, angles;
				
				VectorSet(angles, 0, NPC->r.currentAngles[YAW], 0);

				trap_G2API_GetBoltMatrix( NPC->ghoul2, 0, 
							Q_irand(0,1),
							&boltMatrix, angles, NPC->r.currentOrigin, level.time,
							NULL, NPC->modelScale );
				BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, fxOrg );
				VectorSubtract( NPC->client->leader->r.currentOrigin, fxOrg, fxDir );
				VectorNormalize( fxDir );
				G_PlayEffect( G_EffectIndex( "force/kothos_beam.efx" ), fxOrg, fxDir );
			}
			//BEG HACK LINE
			//RAFIXME:  impliment this effect
			//gentity_t *tent = G_TempEntity( NPC->r.currentOrigin, EV_KOTHOS_BEAM );
			//tent->svFlags |= SVF_BROADCAST;
			//tent->s.otherEntityNum = NPC->s.number;
			//tent->s.otherEntityNum2 = NPC->client->leader->s.number;
			//END HACK LINE

			NPC->client->leader->health += Q_irand( 1+g_spskill.integer*2, 4+g_spskill.integer*3 );//from 1-5 to 4-10
			if ( NPC->client->leader->client )
			{
				if ( NPC->client->leader->client->ps.legsAnim == BOTH_FORCEHEAL_START 
					&& NPC->client->leader->health >= NPC->client->leader->client->pers.maxHealth )
				{//let him get up now
					NPC_SetAnim( NPC->client->leader, SETANIM_BOTH, BOTH_FORCEHEAL_STOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
					//FIXME: temp effect, effect over?
					//racc - impliment this effect somehow.
					//G_PlayEffect( G_EffectIndex( "force/kothos_recharge.efx" ), 0, 0, NPC->client->leader->s.number, NPC->client->leader->r.currentOrigin, NPC->client->leader->client->ps.torsoTimer, qfalse );
					//make him invincible while we recharge him
					//racc - man I hope this works....
					NPC->client->leader->flags &= ~FL_GODMODE;
					//NPC->client->leader->client->ps.powerups[PW_INVINCIBLE] = level.time + NPC->client->leader->client->ps.torsoTimer;
					NPC->client->leader->NPC->ignorePain = qfalse;
					NPC->client->leader->health = NPC->client->leader->client->pers.maxHealth;
				}
				else
				{//start effect?
					//racc - impliment this effect somehow.
					//G_PlayEffect( G_EffectIndex( "force/kothos_recharge.efx" ), 0, 0, NPC->client->leader->s.number, NPC->client->leader->r.currentOrigin, 500, qfalse );
					NPC->client->leader->flags |= FL_GODMODE;
					//NPC->client->leader->client->ps.powerups[PW_INVINCIBLE] = level.time + 500;
				}
			}
			//decrement
			NPC->count--;
			if ( !NPC->count )
			{
				TIMER_Set( NPC, "healRoshDebounce", Q_irand( 5000, 10000 ) );
				NPC->count = 100;
			}
			//now protect me, too
			if ( g_spskill.integer )
			{//not on easy
				//RAFIXME - impliment this effect function.
				//G_PlayEffect( G_EffectIndex( "force/kothos_recharge.efx" ), 0, 0, NPC->s.number, NPC->r.currentOrigin, 500, qfalse );
				NPC->client->leader->flags |= FL_GODMODE;
				//NPC->client->ps.powerups[PW_INVINCIBLE] = level.time + 500;
			}
			return qtrue;
		}
	}
	return qfalse;
}
/* not used anymore by SP but was interesting so I included it.
void Kothos_PowerRosh( void )
{
	if ( NPC->client
		&& NPC->client->leader )
	{
		if ( Distance( NPC->client->leader->r.currentOrigin, NPC->r.currentOrigin ) <= 512.0f 
			&& G_ClearLineOfSight( NPC->client->leader->client->renderInfo.eyePoint, NPC->client->renderInfo.eyePoint, NPC->s.number, MASK_OPAQUE ) )
		{
			NPC_FaceEntity( NPC->client->leader, qtrue );
			NPC_SetAnim( NPC, SETANIM_TORSO, BOTH_FORCELIGHTNING_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			NPC->client->ps.torsoTimer = 500;
			//RAFIXME - impliment this effect function.
			//G_PlayEffect( G_EffectIndex( "force/kothos_beam.efx" ), 0, 1, NPC->s.number, NPC->r.currentOrigin, 500, qfalse );
			if ( NPC->client->leader->client )
			{//hmm, give him some force?
				NPC->client->leader->client->ps.fd.forcePower++;
			}
		}
	}
}
*/

qboolean Kothos_Retreat( void )
{//racc - I think this is suppose to be the movement code for the Kothos healers when
	//they aren't healing Rosh.
	//RAFIXME - impliment that
	/*
	STEER::Activate( NPC );
	STEER::Evade( NPC, NPC->enemy );
	STEER::AvoidCollisions( NPC, NPC->client->leader );
	STEER::DeActivate( NPC, &ucmd );
	*/
	if ( (NPCInfo->aiFlags&NPCAI_BLOCKED) )
	{
		if ( level.time - NPCInfo->blockedDebounceTime > 1000 )
		{
			return qfalse;
		}
	}
	return qtrue;
}

#define TWINS_DANGER_DIST_EASY (128.0f*128.0f)
#define TWINS_DANGER_DIST_MEDIUM (192.0f*192.0f)
#define TWINS_DANGER_DIST_HARD (256.0f*256.0f)
float Twins_DangerDist( void )
{//determines the distance at which the Kothos twins engage enemies.
	switch ( g_spskill.integer )
	{
	case 0:
		return TWINS_DANGER_DIST_EASY;
		break;
	case 1:
		return TWINS_DANGER_DIST_MEDIUM;
		break;
	case 2:
	default:
		return TWINS_DANGER_DIST_HARD;
		break;
	}
}

extern void WP_Explode( gentity_t *self );
qboolean Jedi_InSpecialMove( void )
{//handles Jedi NPC behavior during special moves. returns true if we're doing something
	//special.
	if ( NPC->client->ps.torsoAnim == BOTH_KYLE_PA_1
		|| NPC->client->ps.torsoAnim == BOTH_KYLE_PA_2 
		|| NPC->client->ps.torsoAnim == BOTH_KYLE_PA_3 
		|| NPC->client->ps.torsoAnim == BOTH_PLAYER_PA_1
		|| NPC->client->ps.torsoAnim == BOTH_PLAYER_PA_2
		|| NPC->client->ps.torsoAnim == BOTH_PLAYER_PA_3
		|| NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_END
		|| NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRABBED )
	{//grappling someone or are being grappled.
		NPC_UpdateAngles( qtrue, qtrue );
		return qtrue;
	}

	if ( Jedi_InNoAIAnim( NPC ) )
	{//in special anims, don't do force powers or attacks, just face the enemy
		if ( NPC->enemy )
		{
			NPC_FaceEnemy( qtrue );
		}
		else
		{
			NPC_UpdateAngles( qtrue, qtrue );
		}
		return qtrue;
	}

	if ( NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_START
		|| NPC->client->ps.torsoAnim == BOTH_FORCE_DRAIN_GRAB_HOLD )
	{//using the drain grapple move
		if ( !TIMER_Done( NPC, "draining" ) )
		{//FIXME: what do we do if we ran out of power?  NPC's can't?
			//FIXME: don't keep turning to face enemy or we'll end up spinning around
			ucmd.buttons |= BUTTON_FORCE_DRAIN;
		}
		NPC_UpdateAngles( qtrue, qtrue );
		return qtrue;
	}

	if ( NPC->client->ps.torsoAnim == BOTH_TAVION_SWORDPOWER )
	{//using the sithsword to heal.
		NPC->health += Q_irand( 1, 2 ); 
		if ( NPC->health > NPC->client->pers.maxHealth )
		{
			NPC->health = NPC->client->pers.maxHealth;
		}
		NPC_UpdateAngles( qtrue, qtrue );
		return qtrue;
	}

	if ( NPC->client->ps.torsoAnim == BOTH_SCEPTER_START )
	{//going into the scepter laser attack
		if ( NPC->client->ps.torsoTimer <= 100 )
		{//go into the hold
			NPC->s.loopSound = G_SoundIndex( "sound/weapons/scepter/loop.wav" );
			//RAFIXME - impliment
			//G_PlayEffect( G_EffectIndex( "scepter/beam.efx" ), NPC->client->weaponGhoul2[1], NPC->NPC->genericBolt1, NPC->s.number, NPC->r.currentOrigin, 10000, qtrue );
			NPC->client->ps.legsTimer = NPC->client->ps.torsoTimer = 0;
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_SCEPTER_HOLD, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			NPC->client->ps.torsoTimer += 200;
			NPC->painDebounceTime = level.time + NPC->client->ps.torsoTimer;
			NPC->client->ps.pm_time = NPC->client->ps.torsoTimer;
			NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			VectorClear( NPC->client->ps.velocity );
			VectorClear( NPC->client->ps.moveDir );
		}
		if ( NPC->enemy )
		{
			NPC_FaceEnemy( qtrue );
		}
		else
		{
			NPC_UpdateAngles( qtrue, qtrue );
		}
		return qtrue;
	}
	else if ( NPC->client->ps.torsoAnim == BOTH_SCEPTER_HOLD )
	{//in the scepter laser attack
		if ( NPC->client->ps.torsoTimer <= 100 )
		{
			NPC->s.loopSound = 0;
			//RAFIXME - impliment
			//G_StopEffect( G_EffectIndex( "scepter/beam.efx" ), NPC->client->weaponGhoul2[1], NPC->NPC->genericBolt1, NPC->s.number );
			NPC->client->ps.legsTimer = NPC->client->ps.torsoTimer = 0;
			NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_SCEPTER_STOP, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
			NPC->painDebounceTime = level.time + NPC->client->ps.torsoTimer;
			NPC->client->ps.pm_time = NPC->client->ps.torsoTimer;
			NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			VectorClear( NPC->client->ps.velocity );
			VectorClear( NPC->client->ps.moveDir );
		}
		else
		{
			Tavion_ScepterDamage();
		}
		if ( NPC->enemy )
		{
			NPC_FaceEnemy( qtrue );
		}
		else
		{
			NPC_UpdateAngles( qtrue, qtrue );
		}
		return qtrue;
	}
	else if ( NPC->client->ps.torsoAnim == BOTH_SCEPTER_STOP )
	{//coming out of the scepter laser attack
		if ( NPC->enemy )
		{
			NPC_FaceEnemy( qtrue );
		}
		else
		{
			NPC_UpdateAngles( qtrue, qtrue );
		}
		return qtrue;
	}
	else if ( NPC->client->ps.torsoAnim == BOTH_TAVION_SCEPTERGROUND )
	{//specter slamming
		if ( NPC->client->ps.torsoTimer <= 1200 
			&& !NPC->count ) 
		{
			Tavion_ScepterSlam();
			NPC->count = 1;
		} 
		NPC_UpdateAngles( qtrue, qtrue );
		return qtrue;
	}

	if ( Jedi_CultistDestroyer( NPC ) )
	{//cultist destroyer behavior.
		if ( !NPC->takedamage )
		{//ready to explode
			//RAFIXME - this is a hack, I hope this works
			if ( NPC->painDebounceTime <= level.time )
			//if ( NPC->useDebounceTime <= level.time )
			{
				//this should damage everyone - FIXME: except other destroyers?
				NPC->client->playerTeam = TEAM_FREE;//FIXME: will this destroy wampas, tusken & rancors?
				WP_Explode( NPC );
				return qtrue;
			}
			if ( NPC->enemy )
			{
				NPC_FaceEnemy( qfalse );
			}
			return qtrue;
		}
	}

	if ( NPC->client->NPC_class == CLASS_REBORN )
	{//reborn!
		if ( (NPCInfo->aiFlags&NPCAI_HEAL_ROSH) )
		{//NPC heals Rosh
			if ( !NPC->client->leader )
			{//find Rosh
				NPC->client->leader = G_Find( NULL, FOFS(NPC_type), "rosh_dark" );
			}
			if ( NPC->client->leader )
			{//have our heal target
				qboolean helpingRosh = qfalse;
				//RAFIXME - impliment this?
				//NPC->flags |= FL_LOCK_PLAYER_WEAPONS;
				NPC->client->leader->flags |= FL_UNDYING;
				/* RAFIXME - have no idea what this is supposed to do.  Impliment?
				if ( NPC->client->leader->client )
				{
					NPC->client->leader->client->ps.fd.forcePowersKnown |= FORCE_POWERS_ROSH_FROM_TWINS;
				}
				*/
				if ( NPC->client->leader->client->ps.legsAnim == BOTH_FORCEHEAL_START
					&& TIMER_Done( NPC, "healRoshDebounce" ) )
				{//our heal target is needing a heal
					if ( Kothos_HealRosh() )
					{
						helpingRosh = qtrue;
					}
					else
					{//not able to heal, probably not close enough, try to follow him to
						//get closer
						NPC_BSJedi_FollowLeader();
						NPC_UpdateAngles( qtrue, qtrue );
						return qtrue;
					}
				}

				/*
				if ( !helpingRosh 
					&& !TIMER_Done( NPC->client->leader, "chargeMeUp" ) 
					&& NPC->client->leader->health > 0)
				{
					Kothos_PowerRosh();
					helpingRosh = qtrue;
				}
				*/

				if ( helpingRosh )
				{//if we're healing Rosh, quit using force powers and face him.
					WP_ForcePowerStop( NPC, FP_LIGHTNING );
					WP_ForcePowerStop( NPC, FP_DRAIN );
					WP_ForcePowerStop( NPC, FP_GRIP );
					NPC_FaceEntity( NPC->client->leader, qtrue );
					return qtrue;
				}
				else if ( NPC->enemy && DistanceSquared( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin ) < Twins_DangerDist() )
				{//enemies to attack and close enough to our heal target.
					if ( NPC->enemy && Kothos_Retreat() )
					{//not blocked in movement
						NPC_FaceEnemy( qtrue );
						if ( TIMER_Done( NPC, "attackDelay" ) )
						{//attack!
							if ( NPC->painDebounceTime > level.time  //just get hurt
								|| (NPC->health < 100 && Q_irand(-20, (g_spskill.integer+1)*10) > 0 ) //injured
								|| !Q_irand( 0, 80-(g_spskill.integer*20) ) )//just randomly
							{
								//RAFIXME - impliment
								//NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
								switch ( Q_irand( 0, 7+g_spskill.integer ) )//on easy: no lightning
								{
								case 0:
								case 1:
								case 2:
								case 3:
									//Push the bastard.
									ForceThrow( NPC, qfalse );
									NPC->client->ps.weaponTime = Q_irand( 1000, 3000 )+(2-g_spskill.integer)*1000;
									if ( NPC->painDebounceTime <= level.time 
										&& NPC->health >= 100 )
									{//have an attack delay if this attack isn't the result
										//of injury
										TIMER_Set( NPC, "attackDelay", NPC->client->ps.weaponTime );
									}
									break;
								case 4:
								case 5:
									//drain the bastard.
									//RAFIXME - ForceDrain2 isn't in MP so we're going to just hope that
									//that the "draining" timer can handle it.
									//ForceDrain2( NPC );
									NPC->client->ps.weaponTime = Q_irand( 3000, 6000 )+(2-g_spskill.integer)*2000;
									TIMER_Set( NPC, "draining", NPC->client->ps.weaponTime );
									if ( NPC->painDebounceTime <= level.time 
										&& NPC->health >= 100 )
									{//have an attack delay if this attack isn't the result
										//of injury
										TIMER_Set( NPC, "attackDelay", NPC->client->ps.weaponTime );
									}
									break;
								case 6:
								case 7:
									//try to grip them
									if ( NPC->enemy && InFOV3( NPC->enemy->r.currentOrigin, NPC->r.currentOrigin, NPC->client->ps.viewangles, 20, 30 ) )
									{
										NPC->client->ps.weaponTime = Q_irand( 3000, 6000 )+(2-g_spskill.integer)*2000;
										TIMER_Set( NPC, "gripping", 3000 );
										if ( NPC->painDebounceTime <= level.time 
											&& NPC->health >= 100 )
										{//have an attack delay if this attack isn't the result
										//of injury
											TIMER_Set( NPC, "attackDelay", NPC->client->ps.weaponTime );
										}
									}
									break;
								case 8:
								case 9:
								default:
									//fly their ass.
									ForceLightning( NPC );
									if ( NPC->client->ps.fd.forcePowerLevel[FP_LIGHTNING] > FORCE_LEVEL_1 )
									{
										NPC->client->ps.weaponTime = Q_irand( 3000, 6000 )+(2-g_spskill.integer)*2000;
										TIMER_Set( NPC, "holdLightning", NPC->client->ps.weaponTime );
									}
									if ( NPC->painDebounceTime <= level.time 
										&& NPC->health >= 100 )
									{
										TIMER_Set( NPC, "attackDelay", NPC->client->ps.weaponTime );
									}
									break;
								}
							}
						}
						else
						{
							//RAFIXME - impliment
							//NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
						}
						Jedi_TimersApply();
						return qtrue;
					}
					else
					{
						//RAFIXME - impliment
						//NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
					}
				}
				else if ( !G_ClearLOS4( NPC, NPC->client->leader ) 
					|| DistanceSquared( NPC->r.currentOrigin, NPC->client->leader->r.currentOrigin ) > (512*512) )
				{//can't see Rosh or too far away, catch up with him
					if ( !TIMER_Done( NPC, "attackDelay" ) )
					{
						//RAFIXME - impliment
						//NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
					}
					NPC_BSJedi_FollowLeader();
					NPC_UpdateAngles( qtrue, qtrue );
					return qtrue;
				}
				else
				{//no enemies and close to our heal target
					if ( !TIMER_Done( NPC, "attackDelay" ) )
					{
						//RAFIXME - impliment
						//NPC->flags &= ~FL_LOCK_PLAYER_WEAPONS;
					}
					/* RAFIXME - movement code?  impliment?
					STEER::Activate( NPC );
					STEER::Stop( NPC );
					STEER::DeActivate( NPC, &ucmd );
					*/
					NPC_FaceEnemy( qtrue );
					//NPC_UpdateAngles( qtrue, qtrue );
					return qtrue;
					//NPC_BSJedi_FollowLeader();
				}
			}
			NPC_UpdateAngles( qtrue, qtrue );
			//NPC->client->ps.eFlags &= ~EF_POWERING_ROSH;
			//G_StopEffect( G_EffectIndex( "force/kothos_beam.efx" ), NPC->playerModel, NPC->handLBolt, NPC->s.number );
		}
		else if ( (NPCInfo->aiFlags&NPCAI_ROSH) )
		{//I'm rosh!
			if ( (NPC->flags&FL_UNDYING) )
			{//Vil and/or Dasariah still around to heal me
				if ( NPC->health == 1 //need healing
					|| NPC->flags&FL_GODMODE )//being healed
				{//FIXME: custom anims
					if ( Rosh_TwinPresent( NPC ) )
					{//twins are still alive
						if ( !NPC->client->ps.weaponTime )
						{//not attacking
							if ( NPC->client->ps.legsAnim != BOTH_FORCEHEAL_START 
								&& NPC->client->ps.legsAnim != BOTH_FORCEHEAL_STOP )
							{//get down and wait for Vil or Dasariah to help us
								//FIXME: sound?
								NPC->client->ps.legsTimer = NPC->client->ps.torsoTimer = 0;
								NPC_SetAnim( NPC, SETANIM_BOTH, BOTH_FORCEHEAL_START, SETANIM_FLAG_OVERRIDE|SETANIM_FLAG_HOLD );
								NPC->client->ps.torsoTimer = NPC->client->ps.legsTimer = -1; //hold the animation
								NPC->client->ps.saberHolstered = 2; //turn saber off.
								//NPC->client->ps.SaberDeactivate();
								NPCInfo->ignorePain = qtrue;
							}
						}
						NPC->client->ps.saberBlocked = BLOCKED_NONE;
						NPC->client->ps.saberMove /* not sure what this is = NPC->client->ps.saberMoveNext*/ = LS_NONE;
						NPC->painDebounceTime = level.time + 500;
						NPC->client->ps.pm_time = 500;
						NPC->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
						VectorClear( NPC->client->ps.velocity );
						VectorClear( NPC->client->ps.moveDir );
						return qtrue;
					}
				}
			}
		}
	}

	if ( BG_SuperBreakWinAnim( NPC->client->ps.torsoAnim ) )
	{//striking down our saberlock victim.
		NPC_FaceEnemy( qtrue );
		if ( NPC->client->ps.groundEntityNum != ENTITYNUM_NONE )
		{//on the ground, don't move
			VectorClear( NPC->client->ps.velocity );
		}
		VectorClear( NPC->client->ps.moveDir );
		ucmd.rightmove = ucmd.forwardmove = ucmd.upmove = 0;
		return qtrue;
	}

	return qfalse;
}
//[/CoOp]

extern void NPC_BSST_Patrol( void );
extern void NPC_BSSniper_Default( void );
void NPC_BSJedi_Default( void )
{
	//[CoOp]
	if ( Jedi_InSpecialMove() )
	{//doing some special move behavior.
		return;
	}


	Jedi_CheckCloak();
	if( !NPC->enemy )
	{//don't have an enemy, look for one
		//[CoOp]
		if ( NPC->client->NPC_class == CLASS_BOBAFETT 
			|| (NPC->client->NPC_class == CLASS_REBORN && NPC->s.weapon != WP_SABER)
			|| NPC->client->NPC_class == CLASS_ROCKETTROOPER )
		//if ( NPC->client->NPC_class == CLASS_BOBAFETT )
		//[/CoOp]
		{//non-saber using Jedi NPCs use stormtrooper patrol code. 
			NPC_BSST_Patrol();
		}
		else
		{
			Jedi_Patrol();
		}
	}
	else//if ( NPC->enemy )
	{//have an enemy
		if ( Jedi_WaitingAmbush( NPC ) )
		{//we were still waiting to drop down - must have had enemy set on me outside my AI
			Jedi_Ambush( NPC );
		}
		//[CoOp]
		if ( Jedi_CultistDestroyer( NPC ) 
			&& !NPCInfo->charmedTime )
		{//destroyer rages and makes sound when it sees an enemy
			//permanent effect
			NPCInfo->charmedTime = Q3_INFINITE;
			NPC->client->ps.fd.forcePowersActive |= ( 1 << FP_RAGE );
			NPC->client->ps.fd.forcePowerDuration[FP_RAGE] = Q3_INFINITE;
			//NPC->client->ps.eFlags |= EF_FORCE_DRAINED;
			//FIXME: precache me!
			NPC->s.loopSound = G_SoundIndex( "sound/movers/objects/green_beam_lp2.wav" );//test/charm.wav" );
		}
		/* not in SP code.
		if ( NPC->client->NPC_class == CLASS_BOBAFETT )
		{//racc - check for boba fett sniper behavior
			if ( NPC->enemy->enemy != NPC && NPC->health == NPC->client->pers.maxHealth && DistanceSquared( NPC->r.currentOrigin, NPC->enemy->r.currentOrigin )>(800*800) )
			{//boba fett likes to snipe at far away targets that don't see him.
				NPCInfo->scriptFlags |= SCF_ALT_FIRE;
				Boba_ChangeWeapon( WP_DISRUPTOR );
				NPC_BSSniper_Default();
				return;
			}
		}
		*/
		//[/CoOp]

		Jedi_Attack();
		//racc - check for a better enemy target.
		//if we have multiple-jedi combat, probably need to keep checking (at certain debounce intervals) for a better (closer, more active) enemy and switch if needbe...
		if ( ((!ucmd.buttons&&!NPC->client->ps.fd.forcePowersActive)||(NPC->enemy&&NPC->enemy->health<=0)) && NPCInfo->enemyCheckDebounceTime < level.time )
		{//not doing anything (or walking toward a vanquished enemy - fixme: always taunt the player?), not using force powers and it's time to look again
			//FIXME: build a list of all local enemies (since we have to find best anyway) for other AI factors- like when to use group attacks, determine when to change tactics, when surrounded, when blocked by another in the enemy group, etc.  Should we build this group list or let the enemies maintain their own list and we just access it?
			gentity_t *sav_enemy = NPC->enemy;//FIXME: what about NPC->lastEnemy?
			gentity_t *newEnemy;

			NPC->enemy = NULL;
			newEnemy = NPC_CheckEnemy( NPCInfo->confusionTime < level.time, qfalse, qfalse );
			NPC->enemy = sav_enemy;
			if ( newEnemy && newEnemy != sav_enemy )
			{//picked up a new enemy!
				NPC->lastEnemy = NPC->enemy;
				G_SetEnemy( NPC, newEnemy );
			}
			NPCInfo->enemyCheckDebounceTime = level.time + Q_irand( 1000, 3000 );
		}
	}
	//[CoOp]
	if ( NPC->client->saber[0].type == SABER_SITH_SWORD 
			&& NPC->client->weaponGhoul2[0] )
	{//have sith sword, think about healing.
		if ( NPC->health < 100 
			&& !Q_irand( 0, 20 ) )
		{
			Tavion_SithSwordRecharge();
		}
	}
	//[/CoOp]
}
//[/SPPortComplete]
