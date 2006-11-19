// Copyright (C) 1999-2000 Id Software, Inc.
//
// g_misc.c

#include "g_local.h"
#include "../ghoul2/G2.h"

#include "ai_main.h" //for the g2animents

#define HOLOCRON_RESPAWN_TIME 30000
#define MAX_AMMO_GIVE 2
#define STATION_RECHARGE_TIME 100

void HolocronThink(gentity_t *ent);
extern vmCvar_t g_MaxHolocronCarry;

/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.  They are turned into normal brushes by the utilities.
*/


/*QUAKED info_camp (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void SP_info_camp( gentity_t *self ) {
	G_SetOrigin( self, self->s.origin );
}


//[CoOp]
//[basejkasp] - Mike was nice enough to grab this code from the SP code so that the 
//SP maps won't overload the max entity limit.
/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4) LIGHT
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.

LIGHT - If this info_null is only targeted by a non-switchable light (a light without a targetname), it does NOT spawn in at all and doesn't count towards the # of entities on the map, even at map spawn/load
*/
void SP_info_null( gentity_t *self ) 
{
	if ( (self->spawnflags&1) )
	{//only used as a light target, so bugger off
	G_FreeEntity( self );
	return;
	}
	//FIXME: store targetname and vector (origin) in a list for further reference... remove after 1st second of game?
	G_SetOrigin( self, self->s.origin );
	self->think = G_FreeEntity; //this is different from SP I guess
	//self->e_ThinkFunc = thinkF_G_FreeEntity; 

	//Give other ents time to link
	self->nextthink = level.time + START_TIME_REMOVE_ENTS;
}
//[/basejkasp]
//[/CoOp]


/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
target_position does the same thing
*/
void SP_info_notnull( gentity_t *self ){
	G_SetOrigin( self, self->s.origin );
}


/*QUAKED lightJunior (0 0.7 0.3) (-8 -8 -8) (8 8 8) nonlinear angle negative_spot negative_point
Non-displayed light that only affects dynamic game models, but does not contribute to lightmaps
"light" overrides the default 300 intensity.
Nonlinear checkbox gives inverse square falloff instead of linear 
Angle adds light:surface angle calculations (only valid for "Linear" lights) (wolf)
Lights pointed at a target will be spotlights.
"radius" overrides the default 64 unit radius of a spotlight at the target point.
"fade" falloff/radius adjustment value. multiply the run of the slope by "fade" (1.0f default) (only valid for "Linear" lights) (wolf)
*/

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) linear noIncidence START_OFF
Non-displayed light.
"light" overrides the default 300 intensity. - affects size
a negative "light" will subtract the light's color
'Linear' checkbox gives linear falloff instead of inverse square
'noIncidence' checkbox makes lighting smoother
Lights pointed at a target will be spotlights.
"radius" overrides the default 64 unit radius of a spotlight at the target point.
"scale" multiplier for the light intensity - does not affect size (default 1)
		greater than 1 is brighter, between 0 and 1 is dimmer.
"color" sets the light's color
"targetname" to indicate a switchable light - NOTE that all lights with the same targetname will be grouped together and act as one light (ie: don't mix colors, styles or start_off flag)
"style" to specify a specify light style, even for switchable lights!
"style_off" light style to use when switched off (Only for switchable lights)

   1 FLICKER (first variety)
   2 SLOW STRONG PULSE
   3 CANDLE (first variety)
   4 FAST STROBE
   5 GENTLE PULSE 1
   6 FLICKER (second variety)
   7 CANDLE (second variety)
   8 CANDLE (third variety)
   9 SLOW STROBE (fourth variety)
   10 FLUORESCENT FLICKER
   11 SLOW PULSE NOT FADE TO BLACK
   12 FAST PULSE FOR JEREMY
   13 Test Blending
*/
static void misc_lightstyle_set ( gentity_t *ent)
{
	const int mLightStyle = ent->count;
	const int mLightSwitchStyle = ent->bounceCount;
	const int mLightOffStyle = ent->fly_sound_debounce_time;
	if (!ent->alt_fire)
	{	//turn off
		if (mLightOffStyle)	//i have a light style i'd like to use when off
		{
			char lightstyle[32];
			trap_GetConfigstring(CS_LIGHT_STYLES + (mLightOffStyle*3)+0, lightstyle, 32);
			trap_SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+0, lightstyle);

			trap_GetConfigstring(CS_LIGHT_STYLES + (mLightOffStyle*3)+1, lightstyle, 32);
			trap_SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+1, lightstyle);

			trap_GetConfigstring(CS_LIGHT_STYLES + (mLightOffStyle*3)+2, lightstyle, 32);
			trap_SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+2, lightstyle);
		}else
		{
			trap_SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+0, "a");
			trap_SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+1, "a");
			trap_SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+2, "a");
		}
	} 
	else
	{	//Turn myself on now
		if (mLightSwitchStyle)	//i have a light style i'd like to use when on
		{
			char lightstyle[32];
			trap_GetConfigstring(CS_LIGHT_STYLES + (mLightSwitchStyle*3)+0, lightstyle, 32);
			trap_SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+0, lightstyle);

			trap_GetConfigstring(CS_LIGHT_STYLES + (mLightSwitchStyle*3)+1, lightstyle, 32);
			trap_SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+1, lightstyle);

			trap_GetConfigstring(CS_LIGHT_STYLES + (mLightSwitchStyle*3)+2, lightstyle, 32);
			trap_SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+2, lightstyle);
		}
		else
		{
			trap_SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+0, "z");
			trap_SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+1, "z");
			trap_SetConfigstring(CS_LIGHT_STYLES + (mLightStyle*3)+2, "z");
		}
	}
}

void misc_dlight_use ( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	G_ActivateBehavior(ent,BSET_USE);

	ent->alt_fire = !ent->alt_fire;	//toggle
	misc_lightstyle_set (ent);
}

void SP_light( gentity_t *self ) {
	if (!self->targetname )
	{//if i don't have a light style switch, the i go away
		G_FreeEntity( self );
		return;
	}

	G_SpawnInt( "style", "0", &self->count );
	G_SpawnInt( "switch_style", "0", &self->bounceCount );
	G_SpawnInt( "style_off", "0", &self->fly_sound_debounce_time );
	G_SetOrigin( self, self->s.origin );
	trap_LinkEntity( self );

	self->use = misc_dlight_use;

	self->s.eType = ET_GENERAL;
	self->alt_fire = qfalse;
	self->r.svFlags |= SVF_NOCLIENT;

	if ( !(self->spawnflags & 4) )
	{	//turn myself on now
		self->alt_fire = qtrue;
	}
	misc_lightstyle_set (self);
}


/*
=================================================================================

TELEPORTERS

=================================================================================
*/

void TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles ) {
	gentity_t	*tent;
	qboolean	isNPC = qfalse;
	if (player->s.eType == ET_NPC)
	{
		isNPC = qtrue;
	}

	// use temp events at source and destination to prevent the effect
	// from getting dropped by a second player event
	if ( player->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		tent = G_TempEntity( player->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = player->s.clientNum;

		tent = G_TempEntity( origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = player->s.clientNum;
	}

	// unlink to make sure it can't possibly interfere with G_KillBox
	trap_UnlinkEntity (player);

	VectorCopy ( origin, player->client->ps.origin );
	player->client->ps.origin[2] += 1;

	// spit the player out
	AngleVectors( angles, player->client->ps.velocity, NULL, NULL );
	VectorScale( player->client->ps.velocity, 400, player->client->ps.velocity );
	player->client->ps.pm_time = 160;		// hold time
	player->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;

	// toggle the teleport bit so the client knows to not lerp
	player->client->ps.eFlags ^= EF_TELEPORT_BIT;

	// set angles
	SetClientViewAngle( player, angles );

	// kill anything at the destination
	if ( player->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		G_KillBox (player);
	}

	// save results of pmove
	BG_PlayerStateToEntityState( &player->client->ps, &player->s, qtrue );
	if (isNPC)
	{
		player->s.eType = ET_NPC;
	}

	// use the precise origin for linking
	VectorCopy( player->client->ps.origin, player->r.currentOrigin );

	if ( player->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		trap_LinkEntity (player);
	}
}


/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
Now that we don't have teleport destination pads, this is just
an info_notnull
*/
void SP_misc_teleporter_dest( gentity_t *ent ) {
}


//===========================================================

/*QUAKED misc_model (1 0 0) (-16 -16 -16) (16 16 16)
"model"		arbitrary .md3 or .ase file to display
turns into map triangles - not solid
*/
void SP_misc_model( gentity_t *ent ) {

#if 0
	ent->s.modelindex = G_ModelIndex( ent->model );
	VectorSet (ent->r.mins, -16, -16, -16);
	VectorSet (ent->r.maxs, 16, 16, 16);
	trap_LinkEntity (ent);

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
#else
	G_FreeEntity( ent );
#endif
}


//[CoOp]
/*QUAKED misc_model_static (1 0 0) (-16 -16 0) (16 16 16)
"model"		arbitrary .md3 file to display
"_frame"	"x" which frame from an animated md3
"modelscale"	"x" uniform scale
"modelscale_vec" "x y z" scale model in each axis
"zoffset"	units to offset vertical culling position by, can be
			negative or positive. This does not affect the actual
			position of the model, only the culling position. Use
			it for models with stupid origins that go below the
			ground and whatnot.

loaded as a model in the renderer - does not take up precious bsp space!
*/
void SP_misc_model_static(gentity_t *ent)
{
	G_FreeEntity( ent );
}
//[/CoOp]


//[CoOp]
/* replaced all this fun stuff with real SP code
extern void G_MiscModelExplosion( vec3_t mins, vec3_t maxs, int size, material_t chunkType );
extern void G_Chunks( int owner, vec3_t origin, const vec3_t normal, const vec3_t mins, const vec3_t maxs, 
						float speed, int numChunks, material_t chunkType, int customChunk, float baseScale );

//kill a model_breakable
void model_breakable_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	vec3_t		org, dir, up;
	int			i, numChunks, size = 0;
	material_t	chunkType = self->material;
	float		scale;

	self->takedamage = qfalse;//stop chain reaction runaway loops

	self->enemy = attacker;

	// if a missile is stuck to us, blow it up so we don't look dumb
	for ( i = 0; i < MAX_GENTITIES; i++ )
	{
		if ( g_entities[i].s.groundEntityNum == self->s.number && ( g_entities[i].s.eFlags & EF_MISSILE_STICK ))
		{
			G_Damage( &g_entities[i], self, self, NULL, NULL, 99999, 0, MOD_CRUSH ); //?? MOD?
		}
	}

	//So chunks don't get stuck inside me
	self->s.solid = 0;
	self->r.contents = 0;
	self->clipmask = 0;
	trap_LinkEntity(self); 

	VectorSet(up, 0, 0, 1);

	if ( self->target && attacker != NULL )
	{
		G_UseTargets(self, attacker);
	}

	VectorSubtract( self->r.absmax, self->r.absmin, org );// size

	numChunks = random() * 6 + 18;

	// This formula really has no logical basis other than the fact that it seemed to be the closest to yielding the results that I wanted.
	// Volume is length * width * height...then break that volume down based on how many chunks we have
	scale = sqrt( sqrt( org[0] * org[1] * org[2] )) * 1.75f;

	if ( scale > 48 )
	{
		size = 2;
	}
	else if ( scale > 24 )
	{
		size = 1;
	}

	scale = scale / numChunks;

	VectorMA( self->r.absmin, 0.5, org, org );
	VectorAdd( self->r.absmin,self->r.absmax, org );
	VectorScale( org, 0.5f, org );

	if ( attacker != NULL && attacker->client )
	{
		VectorSubtract( org, attacker->r.currentOrigin, dir );
		VectorNormalize( dir );
	}
	else
	{
		VectorCopy(up, dir);
	} 

	if ( !(self->spawnflags & 2048) ) // NO_EXPLOSION
	{
		// we are allowed to explode
		G_MiscModelExplosion( self->r.absmin, self->r.absmax, size, chunkType );
	}

	if ( self->splashDamage > 0 && self->splashRadius > 0 )
	{
		gentity_t *te;
		//explode
		G_RadiusDamage( org, self, self->splashDamage, self->splashRadius, self, NULL, MOD_UNKNOWN );

		te = G_TempEntity( org, EV_GENERAL_SOUND );
		te->s.eventParm = G_SoundIndex("sound/weapons/explosions/cargoexplode.wav");
	}

	G_Chunks( self->s.number, org, dir, self->r.absmin, self->r.absmax, 300, numChunks, chunkType, 0, (scale*self->mass) );

	self->think = G_FreeEntity;
	self->nextthink = level.time + 50;
}


//using a model_breakable kills it.
void model_breakable_use (gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior( self, BSET_USE );
	model_breakable_die(self, other, activator, self->health, MOD_UNKNOWN);
}


//creates a plain old model in the world
void InitModel(gentity_t *ent)
{
	ent->s.modelindex = G_ModelIndex( ent->model );
	
	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );

	trap_LinkEntity (ent);
}


//creats a model_breakable without setting solidness.
void InitModelBreakable(gentity_t *ent)
{
	G_SoundIndex("sound/weapons/explosions/cargoexplode.wav");//precaching

	if(ent->health)
	{
		ent->takedamage = qtrue;
	}

	ent->use = model_breakable_use;
	ent->die = model_breakable_die;

	InitModel(ent);	
}


//creates a model_breakable that's solid
void InitModelBreakable_Solid(gentity_t *ent)
{
	ent->r.contents = CONTENTS_SOLID;
	ent->s.solid = 2; //SOLID_BBOX
	ent->clipmask = MASK_PLAYERSOLID;
	InitModelBreakable(ent);
}
*/


//moved breakable code to g_breakable.  Converted to SP version of code
/*QUAKED misc_model_breakable (1 0 0) (-16 -16 -16) (16 16 16) SOLID AUTOANIMATE DEADSOLID 

NO_EXPLOSION (2048) - don't explode when killed

"model"		arbitrary .md3 file to display
"health"	how much health to have - default is zero (not breakable)  If you don't set the SOLID flag, but give it health, it can be shot but will not block NPCs or players from moving
"target" What to use when it dies
"paintarget" target to fire when hit (but not destroyed)

"material" - default is "0 - MAT_METAL" - choose from this list:
0 = MAT_METAL		(basic blue-grey scorched-DEFAULT)
1 = MAT_GLASS		
2 = MAT_ELECTRICAL	(sparks only)
3 = MAT_ELEC_METAL	(METAL2 chunks and sparks)
4 =	MAT_DRK_STONE	(brown stone chunks)
5 =	MAT_LT_STONE	(tan stone chunks)
6 =	MAT_GLASS_METAL (glass and METAL2 chunks)
7 = MAT_METAL2		(electronic type of metal)
8 = MAT_NONE		(no chunks)
9 = MAT_GREY_STONE	(grey colored stone)
10 = MAT_METAL3		(METAL and METAL2 chunk combo)
11 = MAT_CRATE1		(yellow multi-colored crate chunks)
12 = MAT_GRATE1		(grate chunks--looks horrible right now)
13 = MAT_ROPE		(for yavin_trial, no chunks, just wispy bits )
14 = MAT_CRATE2		(red multi-colored crate chunks)
15 = MAT_WHITE_METAL (white angular chunks for Stu, NS_hideout )
16 = MAT_SNOWY_ROCK	 (mix of gray and brown rocks)
*/
/*
void SP_misc_model_breakable(gentity_t *ent)
{
	//ent->s.modelindex = G_ModelIndex( ent->model );
	
	if(ent->spawnflags & 1)
	{//make solid
		ent->r.contents = CONTENTS_SOLID;
		ent->s.solid = 2; //SOLID_BBOX
		ent->clipmask = MASK_PLAYERSOLID;

		G_SpawnVector("mins", "-16 -16 -16", ent->r.mins);
		G_SpawnVector("maxs", "16 16 16", ent->r.maxs);
	}

	G_SpawnVector("modelscale_vec", "1 1 1", ent->modelScale);

	if(!ent->material)
	{//don't seem to have a given material, trying looking for one
		G_SpawnInt( "material", "0", (int*)&ent->material );
	}

	G_SpawnInt( "splashDamage", "0", &ent->splashDamage );
	G_SpawnInt( "splashRadius", "0", &ent->splashRadius );

	//G_SoundIndex("sound/weapons/explosions/cargoexplode.wav");//precaching

	//G_SetOrigin( ent, ent->s.origin );
	//VectorCopy( ent->s.angles, ent->s.apos.trBase );

	//ent->use = model_breakable_use;
	//ent->die = model_breakable_die;

	//if(ent->health)
	//{
	//	ent->takedamage = qtrue;
	//}

	//trap_LinkEntity (ent);
	InitModelBreakable(ent);

}
*/

extern void misc_model_breakable_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath );
void misc_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	misc_model_breakable_die( self, other, activator, 100, MOD_UNKNOWN );
}


/*QUAKED misc_exploding_crate (1 0 0.25) (-24 -24 0) (24 24 64)
model="models/map_objects/nar_shaddar/crate_xplode.md3"
Basic exploding crate

"health" - how much health the model has - default 40 (zero makes non-breakable)

"splashRadius" - radius to do damage in - default 128
"splashDamage" - amount of damage to do when it explodes - default 50

"targetname" - auto-explodes
"target" - what to use when it dies

*/
void SP_misc_exploding_crate( gentity_t *ent )
{
	G_SpawnInt( "health", "40", &ent->health );
	G_SpawnInt( "splashRadius", "128", &ent->splashRadius );
	G_SpawnInt( "splashDamage", "50", &ent->splashDamage );

	ent->s.modelindex = G_ModelIndex( "models/map_objects/nar_shaddar/crate_xplode.md3" );
	G_SoundIndex("sound/weapons/explosions/cargoexplode.wav");
	G_EffectIndex( "chunks/metalexplode" );
	
	VectorSet( ent->r.mins, -24, -24, 0 );
	VectorSet( ent->r.maxs, 24, 24, 64 );

	ent->r.contents = CONTENTS_SOLID|CONTENTS_OPAQUE|CONTENTS_BODY|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP;//CONTENTS_SOLID;
	ent->takedamage = qtrue;

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	trap_LinkEntity (ent);

	if ( ent->targetname )
	{
		ent->use = misc_use;
	}

	ent->material = MAT_CRATE1;
	ent->die = misc_model_breakable_die;//ExplodeDeath;
}

/* replaced with SP version of same code
void SP_misc_exploding_crate(gentity_t *ent)
{
	ent->model = "models/map_objects/nar_shaddar/crate_xplode.md3";

	VectorSet(ent->r.mins, -24, -24, 0);
	VectorSet(ent->r.maxs, 24, 24, 64);

	ent->material = 11;

	G_SpawnInt( "health", "40", &ent->health );

	G_SpawnInt( "splashDamage", "128", &ent->splashDamage );
	G_SpawnInt( "splashRadius", "50", &ent->splashRadius );
	InitModelBreakable_Solid(ent);
}
*/


//blast out small jet of frame when hurt.  This flame will hurt new by entities
void GasBurst(gentity_t *self, gentity_t *attacker, int damage)
{
	vec3_t pt;

	VectorCopy( self->r.currentOrigin, pt );
	pt[2] += 46;

	G_PlayEffectID( G_EffectIndex("env/mini_flamejet"), pt, self->r.currentAngles );

	// do some damage to anything that may be standing on top of it when it bursts into flame
	pt[2] += 32;
	G_RadiusDamage( pt, self, 32, 32, self, self, MOD_UNKNOWN );

	//  only get one burst
	self->pain = NULL;
}


//very rarely fire off a jet of gas
void gas_random_jet( gentity_t *self )
{
	vec3_t pt;

	VectorCopy( self->r.currentOrigin, pt );
	pt[2] += 50;

	G_PlayEffectID( G_EffectIndex("env/mini_gasjet"), pt, self->r.currentAngles );

	self->nextthink = level.time + random() * 16000 + 12000; // do this rarely
}


/*QUAKED misc_gas_tank (1 0 0.25) (-4 -4 0) (4 4 40)
model="models/map_objects/imp_mine/tank.md3"
Basic exploding oxygen tank

"health" - how much health the model has - default 20 (zero makes non-breakable)

"splashRadius" - radius to do damage in - default 48
"splashDamage" - amount of damage to do when it explodes - default 32

"targetname" - auto-explodes
"target" - what to use when it dies

*/
void SP_misc_gas_tank( gentity_t *ent )
{
	G_SpawnInt( "health", "20", &ent->health );
	G_SpawnInt( "splashRadius", "48", &ent->splashRadius );
	G_SpawnInt( "splashDamage", "32", &ent->splashDamage );

	ent->s.modelindex = G_ModelIndex( "models/map_objects/imp_mine/tank.md3" );
	G_SoundIndex("sound/weapons/explosions/cargoexplode.wav");
	G_EffectIndex( "chunks/metalexplode" );
	G_EffectIndex( "env/mini_flamejet" );
	G_EffectIndex( "env/mini_gasjet" );

	VectorSet( ent->r.mins, -4, -4, 0 );
	VectorSet( ent->r.maxs, 4, 4, 40 );

	ent->r.contents = CONTENTS_SOLID;
	ent->takedamage = qtrue;

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	trap_LinkEntity (ent);

	ent->pain = GasBurst;

	if ( ent->targetname )
	{
		ent->use = misc_use;
	}

	ent->material = MAT_METAL3;

	ent->die = misc_model_breakable_die;

	ent->think = gas_random_jet;
	ent->nextthink = level.time + random() * 12000 + 6000; // do this rarely
}

/* replaced with SP version of same code
void SP_misc_gas_tank(gentity_t *ent)
{
	ent->model = "models/map_objects/imp_mine/tank.md3";
	
	VectorSet(ent->r.mins, -4, -4, 0);
	VectorSet(ent->r.maxs, 4, 4, 40);

	ent->material = 11;

	G_SpawnInt( "health", "20", &ent->health );

	G_SpawnInt( "splashDamage", "48", &ent->splashDamage );
	G_SpawnInt( "splashRadius", "32", &ent->splashRadius );

	InitModelBreakable_Solid(ent);
}
*/
//[/CoOp]


/*QUAKED misc_G2model (1 0 0) (-16 -16 -16) (16 16 16)
"model"		arbitrary .glm file to display
*/
void SP_misc_G2model( gentity_t *ent ) {

#if 0
	char name1[200] = "models/players/kyle/modelmp.glm";
	trap_G2API_InitGhoul2Model(&ent->s, name1, G_ModelIndex( name1 ), 0, 0, 0, 0);
	trap_G2API_SetBoneAnim(ent->s.ghoul2, 0, "model_root", 0, 12, BONE_ANIM_OVERRIDE_LOOP, 1.0f, level.time, -1, -1);
	ent->s.radius = 150;
//	VectorSet (ent->r.mins, -16, -16, -16);
//	VectorSet (ent->r.maxs, 16, 16, 16);
	trap_LinkEntity (ent);

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
#else
	G_FreeEntity( ent );
#endif
}

//===========================================================

void locateCamera( gentity_t *ent ) {
	vec3_t		dir;
	gentity_t	*target;
	gentity_t	*owner;

	owner = G_PickTarget( ent->target );
	if ( !owner ) {
		G_Printf( "Couldn't find target for misc_partal_surface\n" );
		G_FreeEntity( ent );
		return;
	}
	ent->r.ownerNum = owner->s.number;

	// frame holds the rotate speed
	if ( owner->spawnflags & 1 ) {
		ent->s.frame = 25;
	} else if ( owner->spawnflags & 2 ) {
		ent->s.frame = 75;
	}

	// swing camera ?
	if ( owner->spawnflags & 4 ) {
		// set to 0 for no rotation at all
		ent->s.powerups = 0;
	}
	else {
		ent->s.powerups = 1;
	}

	// clientNum holds the rotate offset
	ent->s.clientNum = owner->s.clientNum;

	VectorCopy( owner->s.origin, ent->s.origin2 );

	// see if the portal_camera has a target
	target = G_PickTarget( owner->target );
	if ( target ) {
		VectorSubtract( target->s.origin, owner->s.origin, dir );
		VectorNormalize( dir );
	} else {
		G_SetMovedir( owner->s.angles, dir );
	}

	ent->s.eventParm = DirToByte( dir );
}

/*QUAKED misc_portal_surface (0 0 1) (-8 -8 -8) (8 8 8)
The portal surface nearest this entity will show a view from the targeted misc_portal_camera, or a mirror view if untargeted.
This must be within 64 world units of the surface!
*/
void SP_misc_portal_surface(gentity_t *ent) {
	VectorClear( ent->r.mins );
	VectorClear( ent->r.maxs );
	trap_LinkEntity (ent);

	ent->r.svFlags = SVF_PORTAL;
	ent->s.eType = ET_PORTAL;

	if ( !ent->target ) {
		VectorCopy( ent->s.origin, ent->s.origin2 );
	} else {
		ent->think = locateCamera;
		ent->nextthink = level.time + 100;
	}
}

/*QUAKED misc_portal_camera (0 0 1) (-8 -8 -8) (8 8 8) slowrotate fastrotate noswing
The target for a misc_portal_director.  You can set either angles or target another entity to determine the direction of view.
"roll" an angle modifier to orient the camera around the target vector;
*/
void SP_misc_portal_camera(gentity_t *ent) {
	float	roll;

	VectorClear( ent->r.mins );
	VectorClear( ent->r.maxs );
	trap_LinkEntity (ent);

	G_SpawnFloat( "roll", "0", &roll );

	ent->s.clientNum = roll/360.0 * 256;
}

/*QUAKED misc_bsp (1 0 0) (-16 -16 -16) (16 16 16)
"bspmodel"		arbitrary .bsp file to display
*/
void SP_misc_bsp(gentity_t *ent) 
{
	char	temp[MAX_QPATH];
	char	*out;
	float	newAngle;
	int		tempint;

	G_SpawnFloat( "angle", "0", &newAngle );
	if (newAngle != 0.0)
	{
		ent->s.angles[1] = newAngle;
	}
	// don't support rotation any other way
	ent->s.angles[0] = 0.0;
	ent->s.angles[2] = 0.0;
	
	G_SpawnString("bspmodel", "", &out);

	ent->s.eFlags = EF_PERMANENT;

	// Mainly for debugging
	G_SpawnInt( "spacing", "0", &tempint);
	ent->s.time2 = tempint;
	G_SpawnInt( "flatten", "0", &tempint);
	ent->s.time = tempint;

	Com_sprintf(temp, MAX_QPATH, "#%s", out);
	trap_SetBrushModel( ent, temp );  // SV_SetBrushModel -- sets mins and maxs
	G_BSPIndex(temp);

	level.mNumBSPInstances++;
	Com_sprintf(temp, MAX_QPATH, "%d-", level.mNumBSPInstances);
	VectorCopy(ent->s.origin, level.mOriginAdjust);
	level.mRotationAdjust = ent->s.angles[1];
	level.mTargetAdjust = temp;
	//level.hasBspInstances = qtrue; //rww - also not referenced anywhere.
	level.mBSPInstanceDepth++;
	/*
	G_SpawnString("filter", "", &out);
	strcpy(level.mFilter, out);
	*/
	G_SpawnString("teamfilter", "", &out);
	strcpy(level.mTeamFilter, out);

	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	VectorCopy( ent->s.angles, ent->r.currentAngles );

	ent->s.eType = ET_MOVER;

	trap_LinkEntity (ent);

	trap_SetActiveSubBSP(ent->s.modelindex);
	G_SpawnEntitiesFromString(qtrue);
	trap_SetActiveSubBSP(-1);

	level.mBSPInstanceDepth--;
	//level.mFilter[0] = level.mTeamFilter[0] = 0;
	level.mTeamFilter[0] = 0;

	/*
	if ( g_debugRMG.integer )
	{
		G_SpawnDebugCylinder ( ent->s.origin, ent->s.time2, &g_entities[0], 2000, COLOR_WHITE );

		if ( ent->s.time )
		{
			G_SpawnDebugCylinder ( ent->s.origin, ent->s.time, &g_entities[0], 2000, COLOR_RED );
		}
	}
	*/
}

/*QUAKED terrain (1.0 1.0 1.0) ? NOVEHDMG

NOVEHDMG - don't damage vehicles upon impact with this terrain

Terrain entity
It will stretch to the full height of the brush

numPatches - integer number of patches to split the terrain brush into (default 200)
terxels - integer number of terxels on a patch side (default 4) (2 <= count <= 8)
seed - integer seed for random terrain generation (default 0)
textureScale - float scale of texture (default 0.005)
heightmap - name of heightmap data image to use, located in heightmaps/*.png. (must be PNG format)
terrainDef - defines how the game textures the terrain (file is base/ext_data/rmg/*.terrain - default is grassyhills)
instanceDef - defines which bsp instances appear
miscentDef - defines which client models spawn on the terrain (file is base/ext_data/rmg/*.miscents)
densityMap - how dense the client models are packed

*/
void AddSpawnField(char *field, char *value);
#define MAX_INSTANCE_TYPES		16
void SP_terrain(gentity_t *ent) 
{
	char				temp[MAX_INFO_STRING];
	char				final[MAX_QPATH];
	char				seed[MAX_QPATH];
	char				missionType[MAX_QPATH];
	//char				soundSet[MAX_QPATH];
	int					shaderNum, i;
	char				*value;
	int					terrainID;

	//Force it to 1 when there is terrain on the level.
	trap_Cvar_Set("RMG", "1");
	g_RMG.integer = 1;

	VectorClear (ent->s.angles);
	trap_SetBrushModel( ent, ent->model );

	// Get the shader from the top of the brush
//	shaderNum = gi.CM_GetShaderNum(s.modelindex);
	shaderNum = 0;

	if (g_RMG.integer)
	{
		/*
		// Grab the default terrain file from the RMG cvar
		trap_Cvar_VariableStringBuffer("RMG_terrain", temp, MAX_QPATH);
		Com_sprintf(final, MAX_QPATH, "%s", temp);
		AddSpawnField("terrainDef", temp);
 
		trap_Cvar_VariableStringBuffer("RMG_instances", temp, MAX_QPATH);
		Com_sprintf(final, MAX_QPATH, "%s", temp);
		AddSpawnField("instanceDef", temp);

		trap_Cvar_VariableStringBuffer("RMG_miscents", temp, MAX_QPATH);
		Com_sprintf(final, MAX_QPATH, "%s", temp);
		AddSpawnField("miscentDef", temp);
		*/
		//rww - disabled for now, don't want cvar overrides.

		trap_Cvar_VariableStringBuffer("RMG_seed", seed, MAX_QPATH);
		trap_Cvar_VariableStringBuffer("RMG_mission", missionType, MAX_QPATH);

		//rww - May want to implement these at some point.
		//trap_Cvar_VariableStringBuffer("RMG_soundset", soundSet, MAX_QPATH);
		//trap_SetConfigstring(CS_AMBIENT_SOUNDSETS, soundSet );
	}

	// Get info required for the common init
	temp[0] = 0;
	G_SpawnString("heightmap", "", &value);
	Info_SetValueForKey(temp, "heightMap", value);

	G_SpawnString("numpatches", "400", &value);
	Info_SetValueForKey(temp, "numPatches", va("%d", atoi(value)));

	G_SpawnString("terxels", "4", &value);
	Info_SetValueForKey(temp, "terxels", va("%d", atoi(value)));

	Info_SetValueForKey(temp, "seed", seed);
	Info_SetValueForKey(temp, "minx", va("%f", ent->r.mins[0]));
	Info_SetValueForKey(temp, "miny", va("%f", ent->r.mins[1]));
	Info_SetValueForKey(temp, "minz", va("%f", ent->r.mins[2]));
	Info_SetValueForKey(temp, "maxx", va("%f", ent->r.maxs[0]));
	Info_SetValueForKey(temp, "maxy", va("%f", ent->r.maxs[1]));
	Info_SetValueForKey(temp, "maxz", va("%f", ent->r.maxs[2]));

	Info_SetValueForKey(temp, "modelIndex", va("%d", ent->s.modelindex));

	G_SpawnString("terraindef", "grassyhills", &value);
	Info_SetValueForKey(temp, "terrainDef", value);

	G_SpawnString("instancedef", "", &value);
	Info_SetValueForKey(temp, "instanceDef", value);

	G_SpawnString("miscentdef", "", &value);
	Info_SetValueForKey(temp, "miscentDef", value);

	Info_SetValueForKey(temp, "missionType", missionType);
	
	for(i = 0; i < MAX_INSTANCE_TYPES; i++)
	{
		trap_Cvar_VariableStringBuffer(va("RMG_instance%d", i), final, MAX_QPATH);
		if(strlen(final))
		{
			Info_SetValueForKey(temp, va("inst%d", i), final);
		}
	}

	// Set additional data required on the client only
	G_SpawnString("densitymap", "", &value);
	Info_SetValueForKey(temp, "densityMap", value);

	Info_SetValueForKey(temp, "shader", va("%d", shaderNum));
	G_SpawnString("texturescale", "0.005", &value);
	Info_SetValueForKey(temp, "texturescale", va("%f", atof(value)));

	// Initialise the common aspects of the terrain
	terrainID = trap_CM_RegisterTerrain(temp);
//	SetCommon(common);

	Info_SetValueForKey(temp, "terrainId", va("%d", terrainID));

	// Let the entity know if it is random generated or not
//	SetIsRandom(common->GetIsRandom());

	// Let the game remember everything
	//level.landScapes[terrainID] = ent; //rww - also not referenced

	// Send all the data down to the client
	trap_SetConfigstring(CS_TERRAINS + terrainID, temp);

	// Make sure the contents are properly set
	ent->r.contents = CONTENTS_TERRAIN;
	ent->r.svFlags = SVF_NOCLIENT;
	ent->s.eFlags = EF_PERMANENT;
	ent->s.eType = ET_TERRAIN;

	// Hook into the world so physics will work
	trap_LinkEntity(ent);

	// If running RMG then initialize the terrain and handle team skins
	if ( g_RMG.integer ) 
	{
		trap_RMG_Init(terrainID);

		/*
		if ( level.gametypeData->teams )
		{
			char temp[MAX_QPATH];

			// Red team change from RMG ?
			trap_GetConfigstring ( CS_GAMETYPE_REDTEAM, temp, MAX_QPATH );
			if ( Q_stricmp ( temp, level.gametypeTeam[TEAM_RED] ) )
			{
				level.gametypeTeam[TEAM_RED] = trap_VM_LocalStringAlloc ( temp );
			}

			// Blue team change from RMG ?
			trap_GetConfigstring ( CS_GAMETYPE_BLUETEAM, temp, MAX_QPATH );
			if ( Q_stricmp ( temp, level.gametypeTeam[TEAM_BLUE] ) )
			{
				level.gametypeTeam[TEAM_BLUE] = trap_VM_LocalStringAlloc ( temp );
			}
		}
		*/
	}
}

//rww - Called by skyportal entities. This will check through entities and flag them
//as portal ents if they are in the same pvs as a skyportal entity and pass
//a direct point trace check between origins. I really wanted to use an eFlag for
//flagging portal entities, but too many entities like to reset their eFlags.
//Note that this was not part of the original wolf sky portal stuff.
void G_PortalifyEntities(gentity_t *ent)
{
	int i = 0;
	gentity_t *scan = NULL;

	while (i < MAX_GENTITIES)
	{
		scan = &g_entities[i];

		if (scan && scan->inuse && scan->s.number != ent->s.number && trap_InPVS(ent->s.origin, scan->r.currentOrigin))
		{
			trace_t tr;

			trap_Trace(&tr, ent->s.origin, vec3_origin, vec3_origin, scan->r.currentOrigin, ent->s.number, CONTENTS_SOLID);

			if (tr.fraction == 1.0 || (tr.entityNum == scan->s.number && tr.entityNum != ENTITYNUM_NONE && tr.entityNum != ENTITYNUM_WORLD))
			{
				if (!scan->client || scan->s.eType == ET_NPC)
				{ //making a client a portal entity would be bad.
					scan->s.isPortalEnt = qtrue; //he's flagged now
				}
			}
		}

		i++;
	}

	ent->think = G_FreeEntity; //the portal entity is no longer needed because its information is stored in a config string.
	ent->nextthink = level.time;
}

/*QUAKED misc_skyportal_orient (.6 .7 .7) (-8 -8 0) (8 8 16)
point from which to orient the sky portal cam in relation
to the regular view position.

"modelscale"			the scale at which to scale positions
*/
void SP_misc_skyportal_orient (gentity_t *ent)
{
	G_FreeEntity(ent);
}


/*QUAKED misc_skyportal (.6 .7 .7) (-8 -8 0) (8 8 16)
"fov" for the skybox default is 80
To have the portal sky fogged, enter any of the following values:
"onlyfoghere" if non-0 allows you to set a global fog, but will only use that fog within this sky portal.

Also note that entities in the same PVS and visible (via point trace) from this
object will be flagged as portal entities. This means they will be sent and
updated from the server for every client every update regardless of where
they are, and they will essentially be added to the scene twice if the client
is in the same PVS as them (only once otherwise, but still once no matter
where the client is). In other words, don't go overboard with it or everything
will explode.
*/
void SP_misc_skyportal (gentity_t *ent)
{
	char	*fov;
	vec3_t	fogv;	//----(SA)	
	int		fogn;	//----(SA)	
	int		fogf;	//----(SA)	
	int		isfog = 0;	// (SA)

	float	fov_x;

	G_SpawnString ("fov", "80", &fov);
	fov_x = atof (fov);

	isfog += G_SpawnVector ("fogcolor", "0 0 0", fogv);
	isfog += G_SpawnInt ("fognear", "0", &fogn);
	isfog += G_SpawnInt ("fogfar", "300", &fogf);

	trap_SetConfigstring( CS_SKYBOXORG, va("%.2f %.2f %.2f %.1f %i %.2f %.2f %.2f %i %i", ent->s.origin[0], ent->s.origin[1], ent->s.origin[2], fov_x, (int)isfog, fogv[0], fogv[1], fogv[2], fogn, fogf ) );

	ent->think = G_PortalifyEntities;
	ent->nextthink = level.time + 1050; //give it some time first so that all other entities are spawned.
}

/*QUAKED misc_holocron (0 0 1) (-8 -8 -8) (8 8 8)
count	Set to type of holocron (based on force power value)
	HEAL = 0
	JUMP = 1
	SPEED = 2
	PUSH = 3
	PULL = 4
	TELEPATHY = 5
	GRIP = 6
	LIGHTNING = 7
	RAGE = 8
	PROTECT = 9
	ABSORB = 10
	TEAM HEAL = 11
	TEAM FORCE = 12
	DRAIN = 13
	SEE = 14
	SABERATTACK = 15
	SABERDEFEND = 16
	SABERTHROW = 17
*/

/*char *holocronTypeModels[] = {
	"models/chunks/rock/rock_big.md3",//FP_HEAL,
	"models/chunks/rock/rock_big.md3",//FP_LEVITATION,
	"models/chunks/rock/rock_big.md3",//FP_SPEED,
	"models/chunks/rock/rock_big.md3",//FP_PUSH,
	"models/chunks/rock/rock_big.md3",//FP_PULL,
	"models/chunks/rock/rock_big.md3",//FP_TELEPATHY,
	"models/chunks/rock/rock_big.md3",//FP_GRIP,
	"models/chunks/rock/rock_big.md3",//FP_LIGHTNING,
	"models/chunks/rock/rock_big.md3",//FP_RAGE,
	"models/chunks/rock/rock_big.md3",//FP_PROTECT,
	"models/chunks/rock/rock_big.md3",//FP_ABSORB,
	"models/chunks/rock/rock_big.md3",//FP_TEAM_HEAL,
	"models/chunks/rock/rock_big.md3",//FP_TEAM_FORCE,
	"models/chunks/rock/rock_big.md3",//FP_DRAIN,
	"models/chunks/rock/rock_big.md3",//FP_SEE
	"models/chunks/rock/rock_big.md3",//FP_SABER_OFFENSE
	"models/chunks/rock/rock_big.md3",//FP_SABER_DEFENSE
	"models/chunks/rock/rock_big.md3"//FP_SABERTHROW
};*/

void HolocronRespawn(gentity_t *self)
{
	self->s.modelindex = (self->count - 128);
}

void HolocronPopOut(gentity_t *self)
{
	if (Q_irand(1, 10) < 5)
	{
		self->s.pos.trDelta[0] = 150 + Q_irand(1, 100);
	}
	else
	{
		self->s.pos.trDelta[0] = -150 - Q_irand(1, 100);
	}
	if (Q_irand(1, 10) < 5)
	{
		self->s.pos.trDelta[1] = 150 + Q_irand(1, 100);
	}
	else
	{
		self->s.pos.trDelta[1] = -150 - Q_irand(1, 100);
	}
	self->s.pos.trDelta[2] = 150 + Q_irand(1, 100);
}

void HolocronTouch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	int i = 0;
	int othercarrying = 0;
	float time_lowest = 0;
	int index_lowest = -1;
	int hasall = 1;
	int forceReselect = WP_NONE;

	if (trace)
	{
		self->s.groundEntityNum = trace->entityNum;
	}

	if (!other || !other->client || other->health < 1)
	{
		return;
	}

	if (!self->s.modelindex)
	{
		return;
	}

	if (self->enemy)
	{
		return;
	}

	if (other->client->ps.holocronsCarried[self->count])
	{
		return;
	}

	if (other->client->ps.holocronCantTouch == self->s.number && other->client->ps.holocronCantTouchTime > level.time)
	{
		return;
	}

	while (i < NUM_FORCE_POWERS)
	{
		if (other->client->ps.holocronsCarried[i])
		{
			othercarrying++;

			if (index_lowest == -1 || other->client->ps.holocronsCarried[i] < time_lowest)
			{
				index_lowest = i;
				time_lowest = other->client->ps.holocronsCarried[i];
			}
		}
		else if (i != self->count)
		{
			hasall = 0;
		}
		i++;
	}

	if (hasall)
	{ //once we pick up this holocron we'll have all of them, so give us super special best prize!
		//G_Printf("You deserve a pat on the back.\n");
	}

	if (!(other->client->ps.fd.forcePowersActive & (1 << other->client->ps.fd.forcePowerSelected)))
	{ //If the player isn't using his currently selected force power, select this one
		if (self->count != FP_SABER_OFFENSE && self->count != FP_SABER_DEFENSE && self->count != FP_SABERTHROW && self->count != FP_LEVITATION)
		{
			other->client->ps.fd.forcePowerSelected = self->count;
		}
	}

	if (g_MaxHolocronCarry.integer && othercarrying >= g_MaxHolocronCarry.integer)
	{ //make the oldest holocron carried by the player pop out to make room for this one
		other->client->ps.holocronsCarried[index_lowest] = 0;

		/*
		if (index_lowest == FP_SABER_OFFENSE && !HasSetSaberOnly())
		{ //you lost your saberattack holocron, so no more saber for you
			other->client->ps.stats[STAT_WEAPONS] |= (1 << WP_STUN_BATON);
			other->client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_SABER);

			if (other->client->ps.weapon == WP_SABER)
			{
				forceReselect = WP_SABER;
			}
		}
		*/
		//NOTE: No longer valid as we are now always giving a force level 1 saber attack level in holocron
	}

	//G_Sound(other, CHAN_AUTO, G_SoundIndex("sound/weapons/w_pkup.wav"));
	G_AddEvent( other, EV_ITEM_PICKUP, self->s.number );

	other->client->ps.holocronsCarried[self->count] = level.time;
	self->s.modelindex = 0;
	self->enemy = other;

	self->pos2[0] = 1;
	self->pos2[1] = level.time + HOLOCRON_RESPAWN_TIME;

	/*
	if (self->count == FP_SABER_OFFENSE && !HasSetSaberOnly())
	{ //player gets a saber
		other->client->ps.stats[STAT_WEAPONS] |= (1 << WP_SABER);
		other->client->ps.stats[STAT_WEAPONS] &= ~(1 << WP_STUN_BATON);

		if (other->client->ps.weapon == WP_STUN_BATON)
		{
			forceReselect = WP_STUN_BATON;
		}
	}
	*/

	if (forceReselect != WP_NONE)
	{
		G_AddEvent(other, EV_NOAMMO, forceReselect);
	}

	//G_Printf("DON'T TOUCH ME\n");
}

void HolocronThink(gentity_t *ent)
{
	if (ent->pos2[0] && (!ent->enemy || !ent->enemy->client || ent->enemy->health < 1))
	{
		if (ent->enemy && ent->enemy->client)
		{
			HolocronRespawn(ent);
			VectorCopy(ent->enemy->client->ps.origin, ent->s.pos.trBase);
			VectorCopy(ent->enemy->client->ps.origin, ent->s.origin);
			VectorCopy(ent->enemy->client->ps.origin, ent->r.currentOrigin);
			//copy to person carrying's origin before popping out of them
			HolocronPopOut(ent);
			ent->enemy->client->ps.holocronsCarried[ent->count] = 0;
			ent->enemy = NULL;
			
			goto justthink;
		}
	}
	else if (ent->pos2[0] && ent->enemy && ent->enemy->client)
	{
		ent->pos2[1] = level.time + HOLOCRON_RESPAWN_TIME;
	}

	if (ent->enemy && ent->enemy->client)
	{
		if (!ent->enemy->client->ps.holocronsCarried[ent->count])
		{
			ent->enemy->client->ps.holocronCantTouch = ent->s.number;
			ent->enemy->client->ps.holocronCantTouchTime = level.time + 5000;

			HolocronRespawn(ent);
			VectorCopy(ent->enemy->client->ps.origin, ent->s.pos.trBase);
			VectorCopy(ent->enemy->client->ps.origin, ent->s.origin);
			VectorCopy(ent->enemy->client->ps.origin, ent->r.currentOrigin);
			//copy to person carrying's origin before popping out of them
			HolocronPopOut(ent);
			ent->enemy = NULL;

			goto justthink;
		}

		if (!ent->enemy->inuse || (ent->enemy->client && ent->enemy->client->ps.fallingToDeath))
		{
			if (ent->enemy->inuse && ent->enemy->client)
			{
				ent->enemy->client->ps.holocronBits &= ~(1 << ent->count);
				ent->enemy->client->ps.holocronsCarried[ent->count] = 0;
			}
			ent->enemy = NULL;
			HolocronRespawn(ent);
			VectorCopy(ent->s.origin2, ent->s.pos.trBase);
			VectorCopy(ent->s.origin2, ent->s.origin);
			VectorCopy(ent->s.origin2, ent->r.currentOrigin);

			ent->s.pos.trTime = level.time;

			ent->pos2[0] = 0;

			trap_LinkEntity(ent);

			goto justthink;
		}
	}

	if (ent->pos2[0] && ent->pos2[1] < level.time)
	{ //isn't in original place and has been there for (HOLOCRON_RESPAWN_TIME) seconds without being picked up, so respawn
		VectorCopy(ent->s.origin2, ent->s.pos.trBase);
		VectorCopy(ent->s.origin2, ent->s.origin);
		VectorCopy(ent->s.origin2, ent->r.currentOrigin);

		ent->s.pos.trTime = level.time;

		ent->pos2[0] = 0;

		trap_LinkEntity(ent);
	}

justthink:
	ent->nextthink = level.time + 50;

	if (ent->s.pos.trDelta[0] || ent->s.pos.trDelta[1] || ent->s.pos.trDelta[2])
	{
		G_RunObject(ent);
	}
}

void SP_misc_holocron(gentity_t *ent)
{
	vec3_t dest;
	trace_t tr;

	if (g_gametype.integer != GT_HOLOCRON)
	{
		G_FreeEntity(ent);
		return;
	}

	if (HasSetSaberOnly())
	{
		if (ent->count == FP_SABER_OFFENSE ||
			ent->count == FP_SABER_DEFENSE ||
			ent->count == FP_SABERTHROW)
		{ //having saber holocrons in saber only mode is pointless
			G_FreeEntity(ent);
			return;
		}
	}

	ent->s.isJediMaster = qtrue;

	VectorSet( ent->r.maxs, 8, 8, 8 );
	VectorSet( ent->r.mins, -8, -8, -8 );

	ent->s.origin[2] += 0.1;
	ent->r.maxs[2] -= 0.1;

	VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
	trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID );
	if ( tr.startsolid )
	{
		G_Printf ("SP_misc_holocron: misc_holocron startsolid at %s\n", vtos(ent->s.origin));
		G_FreeEntity( ent );
		return;
	}

	//add the 0.1 back after the trace
	ent->r.maxs[2] += 0.1;

	// allow to ride movers
//	ent->s.groundEntityNum = tr.entityNum;

	G_SetOrigin( ent, tr.endpos );

	if (ent->count < 0)
	{
		ent->count = 0;
	}

	if (ent->count >= NUM_FORCE_POWERS)
	{
		ent->count = NUM_FORCE_POWERS-1;
	}
/*
	if (g_forcePowerDisable.integer &&
		(g_forcePowerDisable.integer & (1 << ent->count)))
	{
		G_FreeEntity(ent);
		return;
	}
*/
	//No longer doing this, causing too many complaints about accidentally setting no force powers at all
	//and starting a holocron game (making it basically just FFA)

	ent->enemy = NULL;

	ent->flags = FL_BOUNCE_HALF;

	ent->s.modelindex = (ent->count - 128);//G_ModelIndex(holocronTypeModels[ent->count]);
	ent->s.eType = ET_HOLOCRON;
	ent->s.pos.trType = TR_GRAVITY;
	ent->s.pos.trTime = level.time;

	ent->r.contents = CONTENTS_TRIGGER;
	ent->clipmask = MASK_SOLID;

	ent->s.trickedentindex4 = ent->count;

	if (forcePowerDarkLight[ent->count] == FORCE_DARKSIDE)
	{
		ent->s.trickedentindex3 = 1;
	}
	else if (forcePowerDarkLight[ent->count] == FORCE_LIGHTSIDE)
	{
		ent->s.trickedentindex3 = 2;
	}
	else
	{
		ent->s.trickedentindex3 = 3;
	}

	ent->physicsObject = qtrue;

	VectorCopy(ent->s.pos.trBase, ent->s.origin2); //remember the spawn spot

	ent->touch = HolocronTouch;

	trap_LinkEntity(ent);

	ent->think = HolocronThink;
	ent->nextthink = level.time + 50;
}

/*
======================================================================

  SHOOTERS

======================================================================
*/

void Use_Shooter( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	vec3_t		dir;
	float		deg;
	vec3_t		up, right;

	// see if we have a target
	if ( ent->enemy ) {
		VectorSubtract( ent->enemy->r.currentOrigin, ent->s.origin, dir );
		VectorNormalize( dir );
	} else {
		VectorCopy( ent->movedir, dir );
	}

	// randomize a bit
	PerpendicularVector( up, dir );
	CrossProduct( up, dir, right );

	deg = crandom() * ent->random;
	VectorMA( dir, deg, up, dir );

	deg = crandom() * ent->random;
	VectorMA( dir, deg, right, dir );

	VectorNormalize( dir );

	switch ( ent->s.weapon ) {
	case WP_BLASTER:
		WP_FireBlasterMissile( ent, ent->s.origin, dir, qfalse );
		break;
	}

	G_AddEvent( ent, EV_FIRE_WEAPON, 0 );
}


static void InitShooter_Finish( gentity_t *ent ) {
	ent->enemy = G_PickTarget( ent->target );
	ent->think = 0;
	ent->nextthink = 0;
}

void InitShooter( gentity_t *ent, int weapon ) {
	ent->use = Use_Shooter;
	ent->s.weapon = weapon;

	RegisterItem( BG_FindItemForWeapon( weapon ) );

	G_SetMovedir( ent->s.angles, ent->movedir );

	if ( !ent->random ) {
		ent->random = 1.0;
	}
	ent->random = sin( M_PI * ent->random / 180 );
	// target might be a moving object, so we can't set movedir for it
	if ( ent->target ) {
		ent->think = InitShooter_Finish;
		ent->nextthink = level.time + 500;
	}
	trap_LinkEntity( ent );
}

/*QUAKED shooter_blaster (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" is the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_blaster( gentity_t *ent ) {
	InitShooter( ent, WP_BLASTER);
}

void check_recharge(gentity_t *ent)
{
	if (ent->fly_sound_debounce_time < level.time ||
		!ent->activator ||
		!ent->activator->client ||
		!(ent->activator->client->pers.cmd.buttons & BUTTON_USE))
	{
		if (ent->activator)
		{
			G_Sound(ent, CHAN_AUTO, ent->genericValue7);
		}
		ent->s.loopSound = 0;
		ent->s.loopIsSoundset = qfalse;
		ent->activator = NULL;
		ent->fly_sound_debounce_time = 0;
	}
	
	if (!ent->activator)
	{ //don't recharge during use
		if (ent->genericValue8 < level.time)
		{
			if (ent->count < ent->genericValue4)
			{
				ent->count++;
			}
			ent->genericValue8 = level.time + ent->genericValue5;
		}
	}
	ent->s.health = ent->count; //the "health bar" is gonna be how full we are
	ent->nextthink = level.time;
}

/*
================
EnergyShieldStationSettings
================
*/
void EnergyShieldStationSettings(gentity_t *ent)
{
	G_SpawnInt( "count", "200", &ent->count );

	G_SpawnInt("chargerate", "0", &ent->genericValue5);

	if (!ent->genericValue5)
	{
		ent->genericValue5 = STATION_RECHARGE_TIME;
	}
}

/*
================
shield_power_converter_use
================
*/
void shield_power_converter_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	int dif,add;
	int stop = 1;

	if (!activator || !activator->client)
	{
		return;
	}

	if ( g_gametype.integer == GT_SIEGE 
		&& other 
		&& other->client 
		&& other->client->siegeClass )
	{
		if ( !bgSiegeClasses[other->client->siegeClass].maxarmor )
		{//can't use it!
			G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/interface/shieldcon_empty.mp3"));
			return;
		}
	}

	if (self->setTime < level.time)
	{
		int	maxArmor;
		if (!self->s.loopSound)
		{
			self->s.loopSound = G_SoundIndex("sound/interface/shieldcon_run.wav");
			self->s.loopIsSoundset = qfalse;
		}
		self->setTime = level.time + 100;

		if ( g_gametype.integer == GT_SIEGE 
			&& other 
			&& other->client 
			&& other->client->siegeClass != -1 )
		{
			maxArmor = bgSiegeClasses[other->client->siegeClass].maxarmor;
		}
		else
		{
			maxArmor = activator->client->ps.stats[STAT_MAX_HEALTH];
		}
		dif = maxArmor - activator->client->ps.stats[STAT_ARMOR];

		if (dif > 0)					// Already at full armor?
		{
			if (dif >MAX_AMMO_GIVE)
			{
				add = MAX_AMMO_GIVE;
			}
			else
			{
				add = dif;
			}

			if (self->count<add)
			{
				add = self->count;
			}

		    if (!self->genericValue12)
			{
				self->count -= add;
			}
			if (self->count <= 0)
			{
				self->setTime = 0;
			}
			stop = 0;

			self->fly_sound_debounce_time = level.time + 500;
			self->activator = activator;

			activator->client->ps.stats[STAT_ARMOR] += add;
		}
	}

	if (stop || self->count <= 0)
	{
		if (self->s.loopSound && self->setTime < level.time)
		{
			if (self->count <= 0)
			{
				G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/interface/shieldcon_empty.mp3"));
			}
			else
			{
				G_Sound(self, CHAN_AUTO, self->genericValue7);
			}
		}
		self->s.loopSound = 0;
		self->s.loopIsSoundset = qfalse;
		if (self->setTime < level.time)
		{
			self->setTime = level.time + self->genericValue5+100;
		}
	}
}

//[AmmoSys]
qboolean HasValidWeaponThatUsesAmmo( gentity_t *ent, int ammotype ) {
	switch ( ammotype ) {
		case AMMO_BLASTER:
			if ( (ent->client->ps.stats[STAT_WEAPONS] & ( 1 << WP_BLASTER ) ) )
				return qtrue;
			break;
		case AMMO_POWERCELL:
			if ( (ent->client->ps.stats[STAT_WEAPONS] & ( 1 << WP_DISRUPTOR ) ) )
				return qtrue;
			if ( (ent->client->ps.stats[STAT_WEAPONS] & ( 1 << WP_BOWCASTER ) ) )
				return qtrue;
			if ( (ent->client->ps.stats[STAT_WEAPONS] & ( 1 << WP_DEMP2 ) ) )
				return qtrue;

			break;
		case AMMO_METAL_BOLTS:
			if ( (ent->client->ps.stats[STAT_WEAPONS] & ( 1 << WP_REPEATER ) ) )
				return qtrue;
			if ( (ent->client->ps.stats[STAT_WEAPONS] & ( 1 << WP_FLECHETTE ) ) )
				return qtrue;
			if ( (ent->client->ps.stats[STAT_WEAPONS] & ( 1 << WP_CONCUSSION ) ) )
				return qtrue;

			break;
		case AMMO_ROCKETS:
			if ( (ent->client->ps.stats[STAT_WEAPONS] & ( 1 << WP_ROCKET_LAUNCHER ) ) )
				return qtrue;
			break;
		case AMMO_THERMAL:
			if ( (ent->client->ps.stats[STAT_WEAPONS] & ( 1 << WP_THERMAL ) ) || 
				  ent->client->ps.ammo[AMMO_THERMAL] > 0 )
				return qtrue;
			break;
		case AMMO_TRIPMINE:
			if ( (ent->client->ps.stats[STAT_WEAPONS] & ( 1 << WP_TRIP_MINE ) ) || 
				  ent->client->ps.ammo[AMMO_TRIPMINE] > 0 )
				return qtrue;
			break;
		case AMMO_DETPACK:
			if ( (ent->client->ps.stats[STAT_WEAPONS] & ( 1 << WP_DET_PACK ) ) || 
				  ent->client->ps.ammo[AMMO_DETPACK] > 0 )
				return qtrue;
			break;
		default:
			break;
	}

	return qfalse;
}

qboolean AllAmmosFull (gentity_t *ent) {
	int i = 0;
	int max;
	qboolean maxBlaster=qfalse;
	qboolean maxPowercell=qfalse;
	qboolean maxMetalBolts=qfalse;
	qboolean maxRockets=qfalse;
	qboolean maxThermals=qfalse;
	qboolean maxTripMines=qfalse;
	qboolean maxDetpacks=qfalse;

	for ( i = AMMO_BLASTER ; i < AMMO_MAX ; i++ ) {
		if ( i == AMMO_EMPLACED ) continue;

		if ( !HasValidWeaponThatUsesAmmo(ent, i) ) continue;

		if ( ent->client->ps.eFlags & EF_DOUBLE_AMMO ) {
			max = ammoData[i].max*2;
		} else {
			max = ammoData[i].max;
		}

		if ( i == AMMO_BLASTER && ent->client->ps.ammo[AMMO_BLASTER] >= max ) maxBlaster = qtrue;
		if ( i == AMMO_POWERCELL && ent->client->ps.ammo[AMMO_POWERCELL] >= max ) maxPowercell = qtrue;
		if ( i == AMMO_METAL_BOLTS && ent->client->ps.ammo[AMMO_METAL_BOLTS] >= max ) maxMetalBolts = qtrue;
		if ( i == AMMO_ROCKETS && ent->client->ps.ammo[AMMO_ROCKETS] >= max ) maxRockets = qtrue;
		if ( i == AMMO_THERMAL && ent->client->ps.ammo[AMMO_THERMAL] >= max ) maxThermals = qtrue;
		if ( i == AMMO_TRIPMINE && ent->client->ps.ammo[AMMO_TRIPMINE] >= max ) maxTripMines = qtrue;
		if ( i == AMMO_DETPACK && ent->client->ps.ammo[AMMO_DETPACK] >= max ) maxDetpacks = qtrue;
	}

	if ( maxBlaster && maxMetalBolts && maxRockets && maxThermals && maxTripMines && maxDetpacks )
		return qtrue;

	return qfalse;
}

//dispense generic ammo
extern void Add_Ammo3 (gentity_t *ent, int weapon, int count, int *stop, qboolean *gaveSome);
void ammo_generic_power_converter_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	int /*dif,*/ add;
	//int ammoType;
	int stop = 1;

	if (!activator || !activator->client)
	{
		return;
	}

	if (self->setTime < level.time)
	{
		qboolean gaveSome = qfalse;
		int i = AMMO_BLASTER;

		if (!self->s.loopSound)
		{
			self->s.loopSound = G_SoundIndex("sound/interface/ammocon_run.wav");
			self->s.loopIsSoundset = qfalse;
		}
		//self->setTime = level.time + 100;
		self->fly_sound_debounce_time = level.time + 500;
		self->activator = activator;
		for ( i = AMMO_BLASTER ; i < AMMO_MAX ; i++ ) {
			if ( i == AMMO_EMPLACED ) continue; // screw you emplaced

			if ( !HasValidWeaponThatUsesAmmo(activator, i) ) continue;

				if ( activator->client->ps.eFlags & EF_DOUBLE_AMMO ) {
					add = ((ammoData[i].max*2)*0.05);
				} else {
					add = ammoData[i].max*0.05;
				}

			if (add < 1) add = 1;

			Add_Ammo3(activator, i, add, &stop, &gaveSome);

			if (!self->genericValue12 && gaveSome)
			{
				int sub = (add*0.2);
				if (sub < 1)
				{
					sub = 1;
				}
				self->count -= sub;
				if (self->count <= 0)
				{
					self->count = 0;
					//stop = 1;
					break;
				}
			}
		}
	}

	if (stop || self->count <= 0)
	{
		if (self->s.loopSound && self->setTime < level.time)
		{
			if (self->count <= 0)
			{
				G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/interface/ammocon_empty.mp3"));
			}
			else
			{
				G_Sound(self, CHAN_AUTO, self->genericValue7);
			}
		}
		self->s.loopSound = 0;
		self->s.loopIsSoundset = qfalse;
		if (self->setTime < level.time)
		{
			self->setTime = level.time + self->genericValue5+100;
		}
	}
}

//dispense generic ammo
#if 0
void ammo_generic_power_converter_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	int /*dif,*/ add;
	//int ammoType;
	int stop = 1;

	if (!activator || !activator->client)
	{
		return;
	}

	if (self->setTime < level.time)
	{
		qboolean gaveSome = qfalse;
		/*
		while (i < 3)
		{
			if (!self->s.loopSound)
			{
				self->s.loopSound = G_SoundIndex("sound/interface/ammocon_run");
				self->s.loopIsSoundset = qfalse;
			}
			self->setTime = level.time + 100;

			//dif = activator->client->ps.stats[STAT_MAX_HEALTH] - activator->client->ps.stats[STAT_ARMOR];
			switch (i)
			{ //don't give rockets I guess
			case 0:
				ammoType = AMMO_BLASTER;
				break;
			case 1:
				ammoType = AMMO_POWERCELL;
				break;
			case 2:
				ammoType = AMMO_METAL_BOLTS;
				break;
			default:
				ammoType = -1;
				break;
			}

			if (ammoType != -1)
			{
				dif = ammoData[ammoType].max - activator->client->ps.ammo[ammoType];
			}
			else
			{
				dif = 0;
			}

			if (dif > 0)
			{ //only give if not full
				if (dif > MAX_AMMO_GIVE)
				{
					add = MAX_AMMO_GIVE;
				}
				else
				{
					add = dif;
				}

				if (self->count<add)
				{
					add = self->count;
				}

				self->count -= add;
				if (self->count <= 0)
				{
					self->setTime = 0;
					break;
				}
				stop = 0;

				self->fly_sound_debounce_time = level.time + 500;
				self->activator = activator;

				activator->client->ps.ammo[ammoType] += add;
			}

			i++;
		}
		*/
		int i = AMMO_BLASTER;
		if (!self->s.loopSound)
		{
			self->s.loopSound = G_SoundIndex("sound/interface/ammocon_run");
			self->s.loopIsSoundset = qfalse;
		}
		//self->setTime = level.time + 100;
		self->fly_sound_debounce_time = level.time + 500;
		self->activator = activator;
		while (i < AMMO_MAX)
		{
			add = ammoData[i].max*0.05;
			if (add < 1)
			{
				add = 1;
			}
			if ( ( (activator->client->ps.eFlags & EF_DOUBLE_AMMO) && (activator->client->ps.ammo[i] < ammoData[i].max*2)) ||
				( activator->client->ps.ammo[i] < ammoData[i].max ) )
			{
				gaveSome = qtrue;
				if ( g_gametype.integer == GT_SIEGE  && i == AMMO_ROCKETS && activator->client->ps.ammo[i] >= 10 )
				{ //this stuff is already a freaking mess, so..
					gaveSome = qfalse;
				}
				activator->client->ps.ammo[i] += add;
				if ( g_gametype.integer == GT_SIEGE  && i == AMMO_ROCKETS && activator->client->ps.ammo[i] >= 10 )
				{	// fixme - this should SERIOUSLY be externed.
					activator->client->ps.ammo[i] = 10;
				}
				else if ( activator->client->ps.eFlags & EF_DOUBLE_AMMO )
				{
					if (activator->client->ps.ammo[i] >= ammoData[i].max * 2)
					{	// yuck.
						activator->client->ps.ammo[i] = ammoData[i].max * 2;
					}
					else
					{
						stop = 0;
					}
				}
				else
				{
					if (activator->client->ps.ammo[i] >= ammoData[i].max)
					{
						activator->client->ps.ammo[i] = ammoData[i].max;
					}
					else
					{
						stop = 0;
					}
				}
			}
			i++;
			if (!self->genericValue12 && gaveSome)
			{
				int sub = (add*0.2);
				if (sub < 1)
				{
					sub = 1;
				}
				self->count -= sub;
				if (self->count <= 0)
				{
					self->count = 0;
					stop = 1;
					break;
				}
			}
		}
	}

	if (stop || self->count <= 0)
	{
		if (self->s.loopSound && self->setTime < level.time)
		{
			if (self->count <= 0)
			{
				G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/interface/ammocon_empty"));
			}
			else
			{
				G_Sound(self, CHAN_AUTO, self->genericValue7);
			}
		}
		self->s.loopSound = 0;
		self->s.loopIsSoundset = qfalse;
		if (self->setTime < level.time)
		{
			self->setTime = level.time + self->genericValue5+100;
		}
	}
}
#endif
//[/AmmoSys]

/*QUAKED misc_ammo_floor_unit (1 0 0) (-16 -16 0) (16 16 40)
model="/models/items/a_pwr_converter.md3"
Gives generic ammo when used

"count" - max charge value (default 200)
"chargerate" - rechage 1 point every this many milliseconds (default 2000)
"nodrain" - don't drain power from station if 1
*/
void SP_misc_ammo_floor_unit(gentity_t *ent)
{
	vec3_t dest;
	trace_t tr;

	VectorSet( ent->r.mins, -16, -16, 0 );
	VectorSet( ent->r.maxs, 16, 16, 40 );

	ent->s.origin[2] += 0.1f;
	ent->r.maxs[2] -= 0.1f;

	VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
	trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID );
	if ( tr.startsolid )
	{
		G_Printf ("SP_misc_ammo_floor_unit: misc_ammo_floor_unit startsolid at %s\n", vtos(ent->s.origin));
		G_FreeEntity( ent );
		return;
	}

	//add the 0.1 back after the trace
	ent->r.maxs[2] += 0.1f;

	// allow to ride movers
	ent->s.groundEntityNum = tr.entityNum;

	G_SetOrigin( ent, tr.endpos );

	if (!ent->health)
	{
		ent->health = 60;
	}

	if (!ent->model || !ent->model[0])
	{
		ent->model = "/models/items/a_pwr_converter.md3";
	}

	ent->s.modelindex = G_ModelIndex( ent->model );

	ent->s.eFlags = 0;
	ent->r.svFlags |= SVF_PLAYER_USABLE;
	ent->r.contents = CONTENTS_SOLID;
	ent->clipmask = MASK_SOLID;

	EnergyShieldStationSettings(ent);

	ent->genericValue4 = ent->count; //initial value
	ent->think = check_recharge;

	G_SpawnInt("nodrain", "0", &ent->genericValue12);

	if (!ent->genericValue12)
	{
		ent->s.maxhealth = ent->s.health = ent->count;
	}
	ent->s.shouldtarget = qtrue;
	ent->s.teamowner = 0;
	ent->s.owner = ENTITYNUM_NONE;

	ent->nextthink = level.time + 200;// + STATION_RECHARGE_TIME;

	ent->use = ammo_generic_power_converter_use;

	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	trap_LinkEntity (ent);

	G_SoundIndex("sound/interface/ammocon_run.wav");
	ent->genericValue7 = G_SoundIndex("sound/interface/ammocon_done.mp3");
	G_SoundIndex("sound/interface/ammocon_empty.mp3");

	if (g_gametype.integer == GT_SIEGE)
	{ //show on radar from everywhere
		ent->r.svFlags |= SVF_BROADCAST;
		ent->s.eFlags |= EF_RADAROBJECT;
		ent->s.genericenemyindex = G_IconIndex("gfx/mp/siegeicons/desert/weapon_recharge");
	}
	
	//[CoOp]
	if (g_gametype.integer == GT_SINGLE_PLAYER)
	{ //always visible with force sight in single player
		ent->r.svFlags |= SVF_BROADCAST;
		ent->s.eFlags |= EF_FORCE_VISIBLE;
	}
	//[CoOp]
}

/*QUAKED misc_shield_floor_unit (1 0 0) (-16 -16 0) (16 16 40)
model="/models/items/a_shield_converter.md3"
Gives shield energy when used.

"count" - max charge value (default 50)
"chargerate" - rechage 1 point every this many milliseconds (default 3000)
"nodrain" - don't drain power from me
*/
void SP_misc_shield_floor_unit( gentity_t *ent )
{
	vec3_t dest;
	trace_t tr;

	if (g_gametype.integer != GT_CTF &&
		g_gametype.integer != GT_CTY &&
		//[CoOp]
		g_gametype.integer != GT_SIEGE &&
		g_gametype.integer != GT_SINGLE_PLAYER)
		//g_gametype.intger != GT_SIEGE)
		//[/CoOp]
	{
		G_FreeEntity( ent );
		return;
	}

	VectorSet( ent->r.mins, -16, -16, 0 );
	VectorSet( ent->r.maxs, 16, 16, 40 );

	ent->s.origin[2] += 0.1;
	ent->r.maxs[2] -= 0.1;

	VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
	trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID );
	if ( tr.startsolid )
	{
		G_Printf ("SP_misc_shield_floor_unit: misc_shield_floor_unit startsolid at %s\n", vtos(ent->s.origin));
		G_FreeEntity( ent );
		return;
	}

	//add the 0.1 back after the trace
	ent->r.maxs[2] += 0.1;

	// allow to ride movers
	ent->s.groundEntityNum = tr.entityNum;

	G_SetOrigin( ent, tr.endpos );

	if (!ent->health)
	{
		ent->health = 60;
	}

	if (!ent->model || !ent->model[0])
	{
		ent->model = "/models/items/a_shield_converter.md3";
	}

	ent->s.modelindex = G_ModelIndex( ent->model );

	ent->s.eFlags = 0;
	ent->r.svFlags |= SVF_PLAYER_USABLE;
	ent->r.contents = CONTENTS_SOLID;
	ent->clipmask = MASK_SOLID;

	EnergyShieldStationSettings(ent);

	ent->genericValue4 = ent->count; //initial value
	ent->think = check_recharge;

	G_SpawnInt("nodrain", "0", &ent->genericValue12);

    if (!ent->genericValue12)
	{
		ent->s.maxhealth = ent->s.health = ent->count;
	}
	ent->s.shouldtarget = qtrue;
	ent->s.teamowner = 0;
	ent->s.owner = ENTITYNUM_NONE;

	ent->nextthink = level.time + 200;// + STATION_RECHARGE_TIME;

	ent->use = shield_power_converter_use;

	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	trap_LinkEntity (ent);

	G_SoundIndex("sound/interface/shieldcon_run.wav");
	ent->genericValue7 = G_SoundIndex("sound/interface/shieldcon_done.mp3");
	G_SoundIndex("sound/interface/shieldcon_empty.mp3");

	if (g_gametype.integer == GT_SIEGE)
	{ //show on radar from everywhere
		ent->r.svFlags |= SVF_BROADCAST;
		ent->s.eFlags |= EF_RADAROBJECT;
		ent->s.genericenemyindex = G_IconIndex("gfx/mp/siegeicons/desert/shield_recharge");
	}

	//[CoOp]
	if (g_gametype.integer == GT_SINGLE_PLAYER)
	{ //always visible with force sight
		ent->r.svFlags |= SVF_BROADCAST;
		ent->s.eFlags |= EF_FORCE_VISIBLE;
	}
	//[/CoOp]
}


/*QUAKED misc_model_shield_power_converter (1 0 0) (-16 -16 -16) (16 16 16)
model="models/items/psd_big.md3"
Gives shield energy when used.

"count" - the amount of ammo given when used (default 200)
*/
//------------------------------------------------------------
void SP_misc_model_shield_power_converter( gentity_t *ent )
{
	if (!ent->health)
	{
		ent->health = 60;
	}

	VectorSet (ent->r.mins, -16, -16, -16);
	VectorSet (ent->r.maxs, 16, 16, 16);

	ent->s.modelindex = G_ModelIndex( ent->model );

	ent->s.eFlags = 0;
	ent->r.svFlags |= SVF_PLAYER_USABLE;
	ent->r.contents = CONTENTS_SOLID;
	ent->clipmask = MASK_SOLID;

	EnergyShieldStationSettings(ent);

	ent->genericValue4 = ent->count; //initial value
	ent->think = check_recharge;

	ent->s.maxhealth = ent->s.health = ent->count;
	ent->s.shouldtarget = qtrue;
	ent->s.teamowner = 0;
	ent->s.owner = ENTITYNUM_NONE;

	ent->nextthink = level.time + 200;// + STATION_RECHARGE_TIME;

	ent->use = shield_power_converter_use;

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	trap_LinkEntity (ent);

	//G_SoundIndex("sound/movers/objects/useshieldstation.wav");

	ent->s.modelindex2 = G_ModelIndex("/models/items/psd_big.md3");	// Precache model
}


/*
================
EnergyAmmoShieldStationSettings
================
*/
void EnergyAmmoStationSettings(gentity_t *ent)
{
	G_SpawnInt( "count", "200", &ent->count );
}

/*
================
ammo_power_converter_use
================
*/
//[AmmoSys]
void ammo_power_converter_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	int			add = 0.0f;//,highest;
//	qboolean	overcharge; // ensiform - not used
//	int			difBlaster,difPowerCell,difMetalBolts;
	int			stop = 1;

	if (!activator || !activator->client)
	{
		return;
	}

	if (self->setTime < level.time)
	{
		//overcharge = qfalse; // ensiform - not rly used

		if (!self->s.loopSound)
		{
			self->s.loopSound = G_SoundIndex("sound/player/pickupshield.wav");
		}

		self->setTime = level.time + 100;

		if (self->count)	// Has it got any power left?
		{
			int i = AMMO_BLASTER;
			for ( i = AMMO_BLASTER ; i < AMMO_MAX ; i++ ) {
				if ( i == AMMO_EMPLACED ) continue; // screw you emplaced

				if ( !HasValidWeaponThatUsesAmmo(activator, i) ) continue;

				if ( activator->client->ps.eFlags & EF_DOUBLE_AMMO ) {
					add = ((ammoData[i].max*2)*0.1);
				} else {
					add = ammoData[i].max*0.1;
				}

				if (add < 1) add = 1;

				Add_Ammo3(activator, i, add, &stop, (qboolean *)qfalse);
			}
			if (!self->genericValue12)
			{
				self->count -= add;
			}
			stop = 0;

			self->fly_sound_debounce_time = level.time + 500;
			self->activator = activator;
		}
	}

	if (stop)
	{
		self->s.loopSound = 0;
		self->s.loopIsSoundset = qfalse;
	}
}

#if 0
void ammo_power_converter_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	int			add = 0.0f;//,highest;
	qboolean	overcharge;
//	int			difBlaster,difPowerCell,difMetalBolts;
	int			stop = 1;

	if (!activator || !activator->client)
	{
		return;
	}

	if (self->setTime < level.time)
	{
		overcharge = qfalse;

		if (!self->s.loopSound)
		{
			self->s.loopSound = G_SoundIndex("sound/player/pickupshield.wav");
		}

		self->setTime = level.time + 100;

		if (self->count)	// Has it got any power left?
		{
			int i = AMMO_BLASTER;
			while (i < AMMO_MAX)
			{
				add = ammoData[i].max*0.1;
				if (add < 1)
				{
					add = 1;
				}
				if (activator->client->ps.ammo[i] < ammoData[i].max)
				{
					activator->client->ps.ammo[i] += add;
					if (activator->client->ps.ammo[i] > ammoData[i].max)
					{
						activator->client->ps.ammo[i] = ammoData[i].max;
					}
				}
				i++;
			}
			if (!self->genericValue12)
			{
				self->count -= add;
			}
			stop = 0;

			self->fly_sound_debounce_time = level.time + 500;
			self->activator = activator;

			/*
			if (self->count > MAX_AMMO_GIVE)
			{
				add = MAX_AMMO_GIVE;
			}
			else if (self->count<0)
			{
				add = 0;
			}
			else
			{
				add = self->count;
			}

			activator->client->ps.ammo[AMMO_BLASTER] += add;
			activator->client->ps.ammo[AMMO_POWERCELL] += add;
			activator->client->ps.ammo[AMMO_METAL_BOLTS] += add;

			self->count -= add;
			stop = 0;

			self->fly_sound_debounce_time = level.time + 500;
			self->activator = activator;

			difBlaster = activator->client->ps.ammo[AMMO_BLASTER] - ammoData[AMMO_BLASTER].max;
			difPowerCell = activator->client->ps.ammo[AMMO_POWERCELL] - ammoData[AMMO_POWERCELL].max;
			difMetalBolts = activator->client->ps.ammo[AMMO_METAL_BOLTS] - ammoData[AMMO_METAL_BOLTS].max;

			// Find the highest one
			highest = difBlaster;
			if (difPowerCell>difBlaster)
			{
				highest = difPowerCell;
			}

			if (difMetalBolts > highest)
			{
				highest = difMetalBolts;
			}
			*/
		}
	}

	if (stop)
	{
		self->s.loopSound = 0;
		self->s.loopIsSoundset = qfalse;
	}
}
#endif
//[/AmmoSys]

/*QUAKED misc_model_ammo_power_converter (1 0 0) (-16 -16 -16) (16 16 16)
model="models/items/power_converter.md3"
Gives ammo energy when used.

"count" - the amount of ammo given when used (default 200)
"nodrain" - don't drain power from me
*/
//------------------------------------------------------------
void SP_misc_model_ammo_power_converter( gentity_t *ent )
{
	if (!ent->health)
	{
		ent->health = 60;
	}

	VectorSet (ent->r.mins, -16, -16, -16);
	VectorSet (ent->r.maxs, 16, 16, 16);

	ent->s.modelindex = G_ModelIndex( ent->model );

	ent->s.eFlags = 0;
	ent->r.svFlags |= SVF_PLAYER_USABLE;
	ent->r.contents = CONTENTS_SOLID;
	ent->clipmask = MASK_SOLID;

	G_SpawnInt("nodrain", "0", &ent->genericValue12);
	ent->use = ammo_power_converter_use;

	EnergyAmmoStationSettings(ent);

	ent->genericValue4 = ent->count; //initial value
	ent->think = check_recharge;

	if (!ent->genericValue12)
	{
		ent->s.maxhealth = ent->s.health = ent->count;
	}
	ent->s.shouldtarget = qtrue;
	ent->s.teamowner = 0;
	ent->s.owner = ENTITYNUM_NONE;

	ent->nextthink = level.time + 200;// + STATION_RECHARGE_TIME;

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	trap_LinkEntity (ent);

	//G_SoundIndex("sound/movers/objects/useshieldstation.wav");
}

/*
================
EnergyHealthStationSettings
================
*/
void EnergyHealthStationSettings(gentity_t *ent)
{
	G_SpawnInt( "count", "200", &ent->count );
}

/*
================
health_power_converter_use
================
*/
void health_power_converter_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	int dif,add;
	int stop = 1;

	if (!activator || !activator->client)
	{
		return;
	}

	if (self->setTime < level.time)
	{
		if (!self->s.loopSound)
		{
			self->s.loopSound = G_SoundIndex("sound/player/pickuphealth.wav");
		}
		self->setTime = level.time + 100;

		dif = activator->client->ps.stats[STAT_MAX_HEALTH] - activator->health;

		if (dif > 0)					// Already at full armor?
		{
			if (dif >/*MAX_AMMO_GIVE*/5)
			{
				add = 5;//MAX_AMMO_GIVE;
			}
			else
			{
				add = dif;
			}

			if (self->count<add)
			{
				add = self->count;
			}

			//self->count -= add;
			stop = 0;

			self->fly_sound_debounce_time = level.time + 500;
			self->activator = activator;

			activator->health += add;
		}
	}

	if (stop)
	{
		self->s.loopSound = 0;
		self->s.loopIsSoundset = qfalse;
	}
}


/*QUAKED misc_model_health_power_converter (1 0 0) (-16 -16 -16) (16 16 16)
model="models/items/power_converter.md3"
Gives ammo energy when used.

"count" - the amount of ammo given when used (default 200)
*/
//------------------------------------------------------------
void SP_misc_model_health_power_converter( gentity_t *ent )
{
	if (!ent->health)
	{
		ent->health = 60;
	}

	VectorSet (ent->r.mins, -16, -16, -16);
	VectorSet (ent->r.maxs, 16, 16, 16);

	ent->s.modelindex = G_ModelIndex( ent->model );

	ent->s.eFlags = 0;
	ent->r.svFlags |= SVF_PLAYER_USABLE;
	ent->r.contents = CONTENTS_SOLID;
	ent->clipmask = MASK_SOLID;

	ent->use = health_power_converter_use;

	EnergyHealthStationSettings(ent);

	ent->genericValue4 = ent->count; //initial value
	ent->think = check_recharge;

	//ent->s.maxhealth = ent->s.health = ent->count;
	ent->s.shouldtarget = qtrue;
	ent->s.teamowner = 0;
	ent->s.owner = ENTITYNUM_NONE;

	ent->nextthink = level.time + 200;// + STATION_RECHARGE_TIME;

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	trap_LinkEntity (ent);

	//G_SoundIndex("sound/movers/objects/useshieldstation.wav");
	G_SoundIndex("sound/player/pickuphealth.wav");
	ent->genericValue7 = G_SoundIndex("sound/interface/shieldcon_done");

	if (g_gametype.integer == GT_SIEGE)
	{ //show on radar from everywhere
		ent->r.svFlags |= SVF_BROADCAST;
		ent->s.eFlags |= EF_RADAROBJECT;
		ent->s.genericenemyindex = G_IconIndex("gfx/mp/siegeicons/desert/bacta");
	}
}

#if 0 //damage box stuff
void DmgBoxHit( gentity_t *self, gentity_t *other, trace_t *trace )
{
	return;
}

void DmgBoxUpdateSelf(gentity_t *self)
{
	gentity_t *owner = &g_entities[self->r.ownerNum];

	if (!owner || !owner->client || !owner->inuse)
	{
		goto killMe;
	}

	if (self->damageRedirect == DAMAGEREDIRECT_HEAD &&
		owner->client->damageBoxHandle_Head != self->s.number)
	{
		goto killMe;
	}

	if (self->damageRedirect == DAMAGEREDIRECT_RLEG &&
		owner->client->damageBoxHandle_RLeg != self->s.number)
	{
		goto killMe;
	}

	if (self->damageRedirect == DAMAGEREDIRECT_LLEG &&
		owner->client->damageBoxHandle_LLeg != self->s.number)
	{
		goto killMe;
	}

	if (owner->health < 1)
	{
		goto killMe;
	}

	//G_TestLine(self->r.currentOrigin, owner->client->ps.origin, 0x0000ff, 100);

	trap_LinkEntity(self);

	self->nextthink = level.time;
	return;

killMe:
	self->think = G_FreeEntity;
	self->nextthink = level.time;
}

void DmgBoxAbsorb_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod )
{
	self->health = 1;
}

void DmgBoxAbsorb_Pain(gentity_t *self, gentity_t *attacker, int damage)
{
	self->health = 1;
}

gentity_t *CreateNewDamageBox( gentity_t *ent )
{
	gentity_t *dmgBox;

	//We do not want the client to have any real knowledge of the entity whatsoever. It will only
	//ever be used on the server.
	dmgBox = G_Spawn();
	dmgBox->classname = "dmg_box";
			
	dmgBox->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	dmgBox->r.ownerNum = ent->s.number;

	dmgBox->clipmask = 0;
	dmgBox->r.contents = MASK_PLAYERSOLID;

	dmgBox->mass = 5000;

	dmgBox->s.eFlags |= EF_NODRAW;
	dmgBox->r.svFlags |= SVF_NOCLIENT;

	dmgBox->touch = DmgBoxHit;

	dmgBox->takedamage = qtrue;

	dmgBox->health = 1;

	dmgBox->pain = DmgBoxAbsorb_Pain;
	dmgBox->die = DmgBoxAbsorb_Die;

	dmgBox->think = DmgBoxUpdateSelf;
	dmgBox->nextthink = level.time + 50;

	return dmgBox;
}

void ATST_ManageDamageBoxes(gentity_t *ent)
{
	vec3_t headOrg, lLegOrg, rLegOrg;
	vec3_t fwd, right, up, flatAngle;

	if (!ent->client->damageBoxHandle_Head)
	{
		gentity_t *dmgBox = CreateNewDamageBox(ent);

		if (dmgBox)
		{
			VectorSet( dmgBox->r.mins, ATST_MINS0, ATST_MINS1, ATST_MINS2 );
			VectorSet( dmgBox->r.maxs, ATST_MAXS0, ATST_MAXS1, ATST_HEADSIZE );

			ent->client->damageBoxHandle_Head = dmgBox->s.number;
			dmgBox->damageRedirect = DAMAGEREDIRECT_HEAD;
			dmgBox->damageRedirectTo = ent->s.number;
		}
	}
	if (!ent->client->damageBoxHandle_RLeg)
	{
		gentity_t *dmgBox = CreateNewDamageBox(ent);

		if (dmgBox)
		{
			VectorSet( dmgBox->r.mins, ATST_MINS0/4, ATST_MINS1/4, ATST_MINS2 );
			VectorSet( dmgBox->r.maxs, ATST_MAXS0/4, ATST_MAXS1/4, ATST_MAXS2-ATST_HEADSIZE );

			ent->client->damageBoxHandle_RLeg = dmgBox->s.number;
			dmgBox->damageRedirect = DAMAGEREDIRECT_RLEG;
			dmgBox->damageRedirectTo = ent->s.number;
		}
	}
	if (!ent->client->damageBoxHandle_LLeg)
	{
		gentity_t *dmgBox = CreateNewDamageBox(ent);

		if (dmgBox)
		{
			VectorSet( dmgBox->r.mins, ATST_MINS0/4, ATST_MINS1/4, ATST_MINS2 );
			VectorSet( dmgBox->r.maxs, ATST_MAXS0/4, ATST_MAXS1/4, ATST_MAXS2-ATST_HEADSIZE );

			ent->client->damageBoxHandle_LLeg = dmgBox->s.number;
			dmgBox->damageRedirect = DAMAGEREDIRECT_LLEG;
			dmgBox->damageRedirectTo = ent->s.number;
		}
	}

	if (!ent->client->damageBoxHandle_Head ||
		!ent->client->damageBoxHandle_LLeg ||
		!ent->client->damageBoxHandle_RLeg)
	{
		return;
	}

	VectorCopy(ent->client->ps.origin, headOrg);
	headOrg[2] += (ATST_MAXS2-ATST_HEADSIZE);

	VectorCopy(ent->client->ps.viewangles, flatAngle);
	flatAngle[PITCH] = 0;
	flatAngle[ROLL] = 0;

	AngleVectors(flatAngle, fwd, right, up);

	VectorCopy(ent->client->ps.origin, lLegOrg);
	VectorCopy(ent->client->ps.origin, rLegOrg);

	lLegOrg[0] -= right[0]*32;
	lLegOrg[1] -= right[1]*32;
	lLegOrg[2] -= right[2]*32;

	rLegOrg[0] += right[0]*32;
	rLegOrg[1] += right[1]*32;
	rLegOrg[2] += right[2]*32;

	G_SetOrigin(&g_entities[ent->client->damageBoxHandle_Head], headOrg);
	G_SetOrigin(&g_entities[ent->client->damageBoxHandle_LLeg], lLegOrg);
	G_SetOrigin(&g_entities[ent->client->damageBoxHandle_RLeg], rLegOrg);
}

int G_PlayerBecomeATST(gentity_t *ent)
{
	if (!ent || !ent->client)
	{
		return 0;
	}

	if (ent->client->ps.weaponTime > 0)
	{
		return 0;
	}

	if (ent->client->ps.forceHandExtend != HANDEXTEND_NONE)
	{
		return 0;
	}

	if (ent->client->ps.zoomMode)
	{
		return 0;
	}

	if (ent->client->ps.usingATST)
	{
		ent->client->ps.usingATST = qfalse;
		ent->client->ps.forceHandExtend = HANDEXTEND_WEAPONREADY;
	}
	else
	{
		ent->client->ps.usingATST = qtrue;
	}

	ent->client->ps.weaponTime = 1000;

	return 1;
}
#endif

//----------------------------------------------------------

/*QUAKED fx_runner (0 0 1) (-8 -8 -8) (8 8 8) STARTOFF ONESHOT DAMAGE
Runs the specified effect, can also be targeted at an info_notnull to orient the effect

	STARTOFF - effect starts off, toggles on/off when used
	ONESHOT - effect fires only when used
	DAMAGE - does radius damage around effect every "delay" milliseonds

	"fxFile" - name of the effect file to play
	"target" - direction to aim the effect in, otherwise defaults to up
	"target2" - uses its target2 when the fx gets triggered
	"delay"  - how often to call the effect, don't over-do this ( default 200 )
	"random" - random amount of time to add to delay, ( default 0, 200 = 0ms to 200ms )
	"splashRadius" - only works when damage is checked ( default 16 )
	"splashDamage" - only works when damage is checked ( default 5 )
	"soundset"	- bmodel set to use, plays start sound when toggled on, loop sound while on ( doesn't play on a oneshot), and a stop sound when turned off
*/
#define FX_RUNNER_RESERVED 0x800000
#define FX_ENT_RADIUS 32
extern int	BMS_START;
extern int	BMS_MID;
extern int	BMS_END;
//----------------------------------------------------------
void fx_runner_think( gentity_t *ent )
{
	BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->r.currentOrigin );
	BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->r.currentAngles );

	// call the effect with the desired position and orientation
	if (ent->s.isPortalEnt)
	{
//		G_AddEvent( ent, EV_PLAY_PORTAL_EFFECT_ID, ent->genericValue5 );
	}
	else
	{
//		G_AddEvent( ent, EV_PLAY_EFFECT_ID, ent->genericValue5 );
	}

	// start the fx on the client (continuous)
	ent->s.modelindex2 = FX_STATE_CONTINUOUS;

	VectorCopy(ent->r.currentAngles, ent->s.angles);
	VectorCopy(ent->r.currentOrigin, ent->s.origin);

	ent->nextthink = level.time + ent->delay + random() * ent->random;

	if ( ent->spawnflags & 4 ) // damage
	{
		G_RadiusDamage( ent->r.currentOrigin, ent, ent->splashDamage, ent->splashRadius, ent, ent, MOD_UNKNOWN );
	}

	if ( ent->target2 && ent->target2[0] )
	{
		// let our target know that we have spawned an effect
		G_UseTargets2( ent, ent, ent->target2 );
	}

	if ( !(ent->spawnflags & 2 ) && !ent->s.loopSound ) // NOT ONESHOT...this is an assy thing to do
	{
		if ( ent->soundSet && ent->soundSet[0] )
		{
			ent->s.soundSetIndex = G_SoundSetIndex(ent->soundSet);
			ent->s.loopIsSoundset = qtrue;
			ent->s.loopSound = BMS_MID;
		}
	}

}

//----------------------------------------------------------
void fx_runner_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if (self->s.isPortalEnt)
	{ //rww - mark it as broadcast upon first use if it's within the area of a skyportal
		self->r.svFlags |= SVF_BROADCAST;
	}

	if ( self->spawnflags & 2 ) // ONESHOT
	{
		// call the effect with the desired position and orientation, as a safety thing,
		//	make sure we aren't thinking at all.
		int		saveState = self->s.modelindex2 + 1;

		fx_runner_think( self );
		self->nextthink = -1;
		// one shot indicator
		self->s.modelindex2 = saveState;
		if (self->s.modelindex2 > FX_STATE_ONE_SHOT_LIMIT)
		{
			self->s.modelindex2 = FX_STATE_ONE_SHOT;
		}

		if ( self->target2 )
		{
			// let our target know that we have spawned an effect
			G_UseTargets2( self, self, self->target2 );
		}

		if ( self->soundSet && self->soundSet[0] )
		{
			self->s.soundSetIndex = G_SoundSetIndex(self->soundSet);
			G_AddEvent( self, EV_BMODEL_SOUND, BMS_START);
		}
	}
	else
	{
		// ensure we are working with the right think function
		self->think = fx_runner_think;

		// toggle our state
		if ( self->nextthink == -1 )
		{
			// NOTE: we fire the effect immediately on use, the fx_runner_think func will set
			//	up the nextthink time.
			fx_runner_think( self );

			if ( self->soundSet && self->soundSet[0] )
			{
				self->s.soundSetIndex = G_SoundSetIndex(self->soundSet);
				G_AddEvent( self, EV_BMODEL_SOUND, BMS_START);
				self->s.loopSound = BMS_MID;
				self->s.loopIsSoundset = qtrue;
			}
		}
		else
		{
			// turn off for now
			self->nextthink = -1;

			// turn off fx on client
			self->s.modelindex2 = FX_STATE_OFF;

			if ( self->soundSet && self->soundSet[0] )
			{
				self->s.soundSetIndex = G_SoundSetIndex(self->soundSet);
				G_AddEvent( self, EV_BMODEL_SOUND, BMS_END );
				self->s.loopSound = 0;
				self->s.loopIsSoundset = qfalse;
			}
		}
	}
}

//----------------------------------------------------------
void fx_runner_link( gentity_t *ent )
{
	vec3_t	dir;

	if ( ent->target && ent->target[0] )
	{
		// try to use the target to override the orientation
		gentity_t	*target = NULL;

		target = G_Find( target, FOFS(targetname), ent->target );

		if ( !target )
		{
			// Bah, no good, dump a warning, but continue on and use the UP vector
			Com_Printf( "fx_runner_link: target specified but not found: %s\n", ent->target );
			Com_Printf( "  -assuming UP orientation.\n" );
		}
		else
		{
			// Our target is valid so let's override the default UP vector
			VectorSubtract( target->s.origin, ent->s.origin, dir );
			VectorNormalize( dir );
			vectoangles( dir, ent->s.angles );
		}
	}

	// don't really do anything with this right now other than do a check to warn the designers if the target2 is bogus
	if ( ent->target2 && ent->target2[0] )
	{
		gentity_t	*target = NULL;

		target = G_Find( target, FOFS(targetname), ent->target2 );

		if ( !target )
		{
			// Target2 is bogus, but we can still continue
			Com_Printf( "fx_runner_link: target2 was specified but is not valid: %s\n", ent->target2 );
		}
	}

	G_SetAngles( ent, ent->s.angles );

	if ( ent->spawnflags & 1 || ent->spawnflags & 2 ) // STARTOFF || ONESHOT
	{
		// We won't even consider thinking until we are used
		ent->nextthink = -1;
	}
	else
	{
		if ( ent->soundSet && ent->soundSet[0] )
		{
			ent->s.soundSetIndex = G_SoundSetIndex(ent->soundSet);
			ent->s.loopSound = BMS_MID;
			ent->s.loopIsSoundset = qtrue;
		}

		// Let's get to work right now!
		ent->think = fx_runner_think;
		ent->nextthink = level.time + 200; // wait a small bit, then start working
	}

	// make us useable if we can be targeted
	if ( ent->targetname && ent->targetname[0] )
	{
		ent->use = fx_runner_use;
	}
}

//----------------------------------------------------------
void SP_fx_runner( gentity_t *ent )
{
	char *fxFile;

	G_SpawnString( "fxFile", "", &fxFile );
	// Get our defaults
	G_SpawnInt( "delay", "200", &ent->delay );
	G_SpawnFloat( "random", "0", &ent->random );
	G_SpawnInt( "splashRadius", "16", &ent->splashRadius );
	G_SpawnInt( "splashDamage", "5", &ent->splashDamage );

	if (!ent->s.angles[0] && !ent->s.angles[1] && !ent->s.angles[2])
	{
		// didn't have angles, so give us the default of up
		VectorSet( ent->s.angles, -90, 0, 0 );
	}

	if ( !fxFile || !fxFile[0] )
	{
		Com_Printf( S_COLOR_RED"ERROR: fx_runner %s at %s has no fxFile specified\n", ent->targetname, vtos(ent->s.origin) );
		G_FreeEntity( ent );
		return;
	}

	// Try and associate an effect file, unfortunately we won't know if this worked or not 
	//	until the CGAME trys to register it...
	ent->s.modelindex = G_EffectIndex( fxFile );

	// important info transmitted
	ent->s.eType = ET_FX;
	ent->s.speed = ent->delay;
	ent->s.time = ent->random;
	ent->s.modelindex2 = FX_STATE_OFF;

	// Give us a bit of time to spawn in the other entities, since we may have to target one of 'em
	ent->think = fx_runner_link; 
	ent->nextthink = level.time + 400;

	// Save our position and link us up!
	G_SetOrigin( ent, ent->s.origin );

	VectorSet( ent->r.maxs, FX_ENT_RADIUS, FX_ENT_RADIUS, FX_ENT_RADIUS );
	VectorScale( ent->r.maxs, -1, ent->r.mins );

	trap_LinkEntity( ent );
}

/*QUAKED fx_spacedust (1 0 0) (-16 -16 -16) (16 16 16)
This world effect will spawn space dust globally into the level.

"count" the number of snow particles (default of 1000)
*/
//----------------------------------------------------------
void SP_CreateSpaceDust( gentity_t *ent )
{ 
	G_EffectIndex(va("*spacedust %i", ent->count));
	//G_EffectIndex("*constantwind ( 10 -10 0 )");
}


/*QUAKED fx_snow (1 0 0) (-16 -16 -16) (16 16 16)
This world effect will spawn snow globally into the level.

"count" the number of snow particles (default of 1000)
*/
//----------------------------------------------------------
void SP_CreateSnow( gentity_t *ent )
{ 
	G_EffectIndex("*snow");
	G_EffectIndex("*fog");
	G_EffectIndex("*constantwind (100 100 -100)");
}

/*QUAKED fx_rain (1 0 0) (-16 -16 -16) (16 16 16)
This world effect will spawn rain globally into the level.

"count" the number of rain particles (default of 500)
*/
//----------------------------------------------------------
void SP_CreateRain( gentity_t *ent )
{ 
	G_EffectIndex(va("*rain init %i", ent->count));
}

qboolean gEscaping = qfalse;
int gEscapeTime = 0;

void Use_Target_Screenshake( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	qboolean bGlobal = qfalse;

	if (ent->genericValue6)
	{
		bGlobal = qtrue;
	}

	G_ScreenShake(ent->s.origin, NULL, ent->speed, ent->genericValue5, bGlobal);
}

void SP_target_screenshake(gentity_t *ent)
{
	G_SpawnFloat( "intensity", "10", &ent->speed );
	//intensity of the shake
	G_SpawnInt( "duration", "800", &ent->genericValue5 );
	//duration of the shake
	G_SpawnInt( "globalshake", "1", &ent->genericValue6 );
	//non-0 if shake should be global (all clients). Otherwise, only in the PVS.

	ent->use = Use_Target_Screenshake;
}

void LogExit( const char *string );

void Use_Target_Escapetrig( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	if (!ent->genericValue6)
	{
		gEscaping = qtrue;
		gEscapeTime = level.time + ent->genericValue5;
	}
	else if (gEscaping)
	{
		int i = 0;
		gEscaping = qfalse;
		while (i < MAX_CLIENTS)
		{ //all of the survivors get 100 points!
			if (g_entities[i].inuse && g_entities[i].client && g_entities[i].health > 0 &&
				g_entities[i].client->sess.sessionTeam != TEAM_SPECTATOR &&
				!(g_entities[i].client->ps.pm_flags & PMF_FOLLOW))
			{
				AddScore(&g_entities[i], g_entities[i].client->ps.origin, 100);
			}
			i++;
		}
		if (activator && activator->inuse && activator->client)
		{ //the one who escaped gets 500
			AddScore(activator, activator->client->ps.origin, 500);
		}

		LogExit("Escaped!");
	}
}

void SP_target_escapetrig(gentity_t *ent)
{
	if (g_gametype.integer != GT_SINGLE_PLAYER)
	{
		G_FreeEntity(ent);
		return;
	}

	G_SpawnInt( "escapetime", "60000", &ent->genericValue5);
	//time given (in ms) for the escape
	G_SpawnInt( "escapegoal", "0", &ent->genericValue6);
	//if non-0, when used, will end an ongoing escape instead of start it

	ent->use = Use_Target_Escapetrig;
}

/*QUAKED misc_maglock (0 .5 .8) (-8 -8 -8) (8 8 8) x x x x x x x x
Place facing a door (using the angle, not a targetname) and it will lock that door.  Can only be destroyed by lightsaber and will automatically unlock the door it's attached to

NOTE: place these half-way in the door to make it flush with the door's surface.

"target"	thing to use when destoryed (not doors - it automatically unlocks the door it was angled at)
"health"	default is 10
*/
void maglock_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	//unlock our door if we're the last lock pointed at the door
	if ( self->activator )
	{
		self->activator->lockCount--;
		if ( !self->activator->lockCount )
		{
			self->activator->flags &= ~FL_INACTIVE;
		}
	}
	
	//use targets
	G_UseTargets( self, attacker );
	//die
	//rwwFIXMEFIXME - weap expl func
//	WP_Explode( self );
}

void maglock_link( gentity_t *self );
gentity_t *G_FindDoorTrigger( gentity_t *ent );

void SP_misc_maglock ( gentity_t *self )
{
	//NOTE: May have to make these only work on doors that are either untargeted 
	//		or are targeted by a trigger, not doors fired off by scripts, counters 
	//		or other such things?
	self->s.modelindex = G_ModelIndex( "models/map_objects/imp_detention/door_lock.md3" );
	self->genericValue1 = G_EffectIndex( "maglock/explosion" );

	G_SetOrigin( self, self->s.origin );

	self->think = maglock_link;
	//FIXME: for some reason, when you re-load a level, these fail to find their doors...?  Random?  Testing an additional 200ms after the START_TIME_FIND_LINKS
	self->nextthink = level.time + START_TIME_FIND_LINKS+200;//START_TIME_FIND_LINKS;//because we need to let the doors link up and spawn their triggers first!
}
void maglock_link( gentity_t *self )
{
	//find what we're supposed to be attached to
	vec3_t	forward, start, end;
	trace_t	trace;
	gentity_t *traceEnt;

	AngleVectors( self->s.angles, forward, NULL, NULL );
	VectorMA( self->s.origin, 128, forward, end );
	VectorMA( self->s.origin, -4, forward, start );

	trap_Trace( &trace, start, vec3_origin, vec3_origin, end, self->s.number, MASK_SHOT );

	if ( trace.allsolid || trace.startsolid )
	{
		Com_Error( ERR_DROP,"misc_maglock at %s in solid\n", vtos(self->s.origin) );
		G_FreeEntity( self );
		return;
	}
	if ( trace.fraction == 1.0 )
	{
		self->think = maglock_link;
		self->nextthink = level.time + 100;
		/*
		Com_Error( ERR_DROP,"misc_maglock at %s pointed at no surface\n", vtos(self->s.origin) );
		G_FreeEntity( self );
		*/
		return;
	}
	traceEnt = &g_entities[trace.entityNum];
	if ( trace.entityNum >= ENTITYNUM_WORLD || !traceEnt || Q_stricmp( "func_door", traceEnt->classname ) )
	{
		self->think = maglock_link;
		self->nextthink = level.time + 100;
		//Com_Error( ERR_DROP,"misc_maglock at %s not pointed at a door\n", vtos(self->s.origin) );
		//G_FreeEntity( self );
		return;
	}

	//check the traceEnt, make sure it's a door and give it a lockCount and deactivate it
	//find the trigger for the door
	self->activator = G_FindDoorTrigger( traceEnt );
	if ( !self->activator )
	{
		self->activator = traceEnt;
	}
	self->activator->lockCount++;
	self->activator->flags |= FL_INACTIVE;

	//now position and orient it
	vectoangles( trace.plane.normal, end );
	G_SetOrigin( self, trace.endpos );
	G_SetAngles( self, end );

	//make it hittable
	//FIXME: if rotated/inclined this bbox may be off... but okay if we're a ghoul model?
	//self->s.modelindex = G_ModelIndex( "models/map_objects/imp_detention/door_lock.md3" );
	VectorSet( self->r.mins, -8, -8, -8 );
	VectorSet( self->r.maxs, 8, 8, 8 );
	self->r.contents = CONTENTS_CORPSE;

	//make it destroyable
	self->flags |= FL_SHIELDED;//only damagable by lightsabers
	self->takedamage = qtrue;
	self->health = 10;
	self->die = maglock_die;
	//self->fxID = G_EffectIndex( "maglock/explosion" );

	trap_LinkEntity( self );
}

void faller_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	if (self->epVelocity[2] < -100 && self->genericValue7 < level.time)
	{
		int r = Q_irand(1, 3);

		if (r == 1)
		{
			self->genericValue11 = G_SoundIndex("sound/chars/stofficer1/misc/pain25");
		}
		else if (r == 2)
		{
			self->genericValue11 = G_SoundIndex("sound/chars/stofficer1/misc/pain50");
		}
		else
		{
			self->genericValue11 = G_SoundIndex("sound/chars/stofficer1/misc/pain75");
		}

		G_EntitySound(self, CHAN_VOICE, self->genericValue11);
		G_EntitySound(self, CHAN_AUTO, self->genericValue10);

		self->genericValue6 = level.time + 3000;

		self->genericValue7 = level.time + 200;
	}
}

void faller_think(gentity_t *ent)
{
	float gravity = 3.0f;
	float mass = 0.09f;
	float bounce = 1.1f;

	if (ent->genericValue6 < level.time)
	{
		ent->think = G_FreeEntity;
		ent->nextthink = level.time;
		return;
	}

	if (ent->epVelocity[2] < -100)
	{
		if (!ent->genericValue8)
		{
			G_EntitySound(ent, CHAN_VOICE, ent->genericValue9);
			ent->genericValue8 = 1;
		}
	}
	else
	{
		ent->genericValue8 = 0;
	}

	G_RunExPhys(ent, gravity, mass, bounce, qtrue, NULL, 0);
	VectorScale(ent->epVelocity, 10.0f, ent->s.pos.trDelta);
	ent->nextthink = level.time + 25;
}

void misc_faller_create( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	gentity_t *faller = G_Spawn();

	faller->genericValue10 = G_SoundIndex("sound/player/fallsplat");
	faller->genericValue9 = G_SoundIndex("sound/chars/stofficer1/misc/falling1");
	faller->genericValue8 = 0;
	faller->genericValue7 = 0;

	faller->genericValue6 = level.time + 15000;

	G_SetOrigin(faller, ent->s.origin);

	faller->s.modelGhoul2 = 1;
	faller->s.modelindex = G_ModelIndex("models/players/stormtrooper/model.glm");
	faller->s.g2radius = 100;
	
	faller->s.customRGBA[0]=Q_irand(1,255);
	faller->s.customRGBA[1]=Q_irand(1,255);
	faller->s.customRGBA[2]=Q_irand(1,255);
	faller->s.customRGBA[3]=255;

	VectorSet(faller->r.mins, -15, -15, DEFAULT_MINS_2);
	VectorSet(faller->r.maxs, 15, 15, DEFAULT_MAXS_2);
	
	faller->clipmask = MASK_PLAYERSOLID;
	faller->r.contents = MASK_PLAYERSOLID;

	faller->s.eFlags = (EF_RAG|EF_CLIENTSMOOTH);

	faller->think = faller_think;
	faller->nextthink = level.time;

	faller->touch = faller_touch;

	faller->epVelocity[0] = flrand(-256.0f, 256.0f);
	faller->epVelocity[1] = flrand(-256.0f, 256.0f);

	trap_LinkEntity(faller);
}

void misc_faller_think(gentity_t *ent)
{
	misc_faller_create(ent, ent, ent);
	ent->nextthink = level.time + ent->genericValue1 + Q_irand(0, ent->genericValue2);
}

/*QUAKED misc_faller (1 0 0) (-8 -8 -8) (8 8 8)
Falling stormtrooper - spawned every interval+random fudgefactor,
or if specified, when used.

targetname	- if specified, will only spawn when used
interval	- spawn every so often (milliseconds)
fudgefactor	- milliseconds between 0 and this number randomly added to interval
*/
void SP_misc_faller(gentity_t *ent)
{
	G_ModelIndex("models/players/stormtrooper/model.glm");
	G_SoundIndex("sound/chars/stofficer1/misc/pain25");
	G_SoundIndex("sound/chars/stofficer1/misc/pain50");
	G_SoundIndex("sound/chars/stofficer1/misc/pain75");
	G_SoundIndex("sound/chars/stofficer1/misc/falling1");
	G_SoundIndex("sound/player/fallsplat");

	G_SpawnInt("interval", "500", &ent->genericValue1);
	G_SpawnInt("fudgefactor", "0", &ent->genericValue2);

	if (!ent->targetname || !ent->targetname[0])
	{
		ent->think = misc_faller_think;
		ent->nextthink = level.time + ent->genericValue1 + Q_irand(0, ent->genericValue2);
	}
	else
	{
		ent->use = misc_faller_create;
	}
}

//rww - ref tag stuff ported from SP (and C-ified)
#define	TAG_GENERIC_NAME	"__WORLD__"	//If a designer chooses this name, cut a finger off as an example to the others

//MAX_TAG_OWNERS is 16 for now in order to not use too much VM memory.
//Each tag owner has preallocated space for tags up to MAX_TAGS.
//As is this means 16*256 sizeof(reference_tag_t)'s in addition to name+inuse*16.
#define MAX_TAGS 256
#define MAX_TAG_OWNERS 16

//Maybe I should use my trap_TrueMalloc/trap_TrueFree stuff with this.
//But I am not yet confident that it can be used without exploding at some point.

typedef struct tagOwner_s
{
	char			name[MAX_REFNAME];
	reference_tag_t	tags[MAX_TAGS];
	qboolean		inuse;
} tagOwner_t;

tagOwner_t refTagOwnerMap[MAX_TAG_OWNERS];

tagOwner_t *FirstFreeTagOwner(void)
{
	int i = 0;

	while (i < MAX_TAG_OWNERS)
	{
		if (!refTagOwnerMap[i].inuse)
		{
			return &refTagOwnerMap[i];
		}
		i++;
	}

	Com_Printf("WARNING: MAX_TAG_OWNERS (%i) REF TAG LIMIT HIT\n", MAX_TAG_OWNERS);
	return NULL;
}

reference_tag_t *FirstFreeRefTag(tagOwner_t *tagOwner)
{
	int i = 0;

	assert(tagOwner);

	while (i < MAX_TAGS)
	{
		if (!tagOwner->tags[i].inuse)
		{
			return &tagOwner->tags[i];
		}
		i++;
	}

	Com_Printf("WARNING: MAX_TAGS (%i) REF TAG LIMIT HIT\n", MAX_TAGS);
	return NULL;
}

/*
-------------------------
TAG_Init
-------------------------
*/

void TAG_Init( void )
{
	int i = 0;
	int x = 0;

	while (i < MAX_TAG_OWNERS)
	{
		while (x < MAX_TAGS)
		{
			memset(&refTagOwnerMap[i].tags[x], 0, sizeof(refTagOwnerMap[i].tags[x]));
			x++;
		}
		memset(&refTagOwnerMap[i], 0, sizeof(refTagOwnerMap[i]));
		i++;
	}
}

/*
-------------------------
TAG_FindOwner
-------------------------
*/

tagOwner_t	*TAG_FindOwner( const char *owner )
{
	int i = 0;

	while (i < MAX_TAG_OWNERS)
	{
		if (refTagOwnerMap[i].inuse && !Q_stricmp(refTagOwnerMap[i].name, owner))
		{
			return &refTagOwnerMap[i];
		}
		i++;
	}

	return NULL;
}

/*
-------------------------
TAG_Find
-------------------------
*/

reference_tag_t	*TAG_Find( const char *owner, const char *name )
{
	tagOwner_t	*tagOwner = NULL;
	int i = 0;

	if (owner && owner[0])
	{
		tagOwner = TAG_FindOwner(owner);
	}
	if (!tagOwner)
	{
		tagOwner = TAG_FindOwner(TAG_GENERIC_NAME);
	}

	//Not found...
	if (!tagOwner)
	{
		tagOwner = TAG_FindOwner( TAG_GENERIC_NAME );

		if (!tagOwner)
		{
			return NULL;
		}
	}

	while (i < MAX_TAGS)
	{
		if (tagOwner->tags[i].inuse && !Q_stricmp(tagOwner->tags[i].name, name))
		{
			return &tagOwner->tags[i];
		}
		i++;
	}

	//Try the generic owner instead
	tagOwner = TAG_FindOwner( TAG_GENERIC_NAME );

	if (!tagOwner)
	{
		return NULL;
	}

	i = 0;
	while (i < MAX_TAGS)
	{
		if (tagOwner->tags[i].inuse && !Q_stricmp(tagOwner->tags[i].name, name))
		{
			return &tagOwner->tags[i];
		}
		i++;
	}

	return NULL;
}

/*
-------------------------
TAG_Add
-------------------------
*/

reference_tag_t	*TAG_Add( const char *name, const char *owner, vec3_t origin, vec3_t angles, int radius, int flags )
{
	reference_tag_t	*tag = NULL;
	tagOwner_t	*tagOwner = NULL;

	//Make sure this tag's name isn't alread in use
	if ( TAG_Find( owner, name ) )
	{
		Com_Printf(S_COLOR_RED"Duplicate tag name \"%s\"\n", name );
		return NULL;
	}

	//Attempt to add this to the owner's list
	if ( !owner || !owner[0] )
	{
		//If the owner isn't found, use the generic world name
		owner = TAG_GENERIC_NAME;
	}

	tagOwner = TAG_FindOwner( owner );
	
	if (!tagOwner)
	{
		//Create a new owner list
		tagOwner = FirstFreeTagOwner();//new	tagOwner_t;

		if (!tagOwner)
		{
			assert(0);
			return 0;
		}
	}
	
	//This is actually reverse order of how SP does it because of the way we're storing/allocating.
	//Now that we have the owner, we want to get the first free reftag on the owner itself.
	tag = FirstFreeRefTag(tagOwner);

	if (!tag)
	{
		assert(0);
		return NULL;
	}

	//Copy the information
	VectorCopy( origin, tag->origin );
	VectorCopy( angles, tag->angles );
	tag->radius = radius;
	tag->flags	= flags;

	if ( !name || !name[0] )
	{
		Com_Printf(S_COLOR_RED"ERROR: Nameless ref_tag found at (%i %i %i)\n", (int)origin[0], (int)origin[1], (int)origin[2]);
		return NULL;
	}


	//Copy the name
	Q_strncpyz( (char *) tagOwner->name, owner, MAX_REFNAME );
	Q_strlwr( (char *) tagOwner->name );	//NOTENOTE: For case insensitive searches on a map

	//Copy the name
	Q_strncpyz( (char *) tag->name, name, MAX_REFNAME );
	Q_strlwr( (char *) tag->name );	//NOTENOTE: For case insensitive searches on a map

	tagOwner->inuse = qtrue;
	tag->inuse = qtrue;

	return tag;
}

/*
-------------------------
TAG_GetOrigin
-------------------------
*/

int	TAG_GetOrigin( const char *owner, const char *name, vec3_t origin )
{
	reference_tag_t	*tag = TAG_Find( owner, name );

	if (!tag)
	{
		VectorClear(origin);
		return 0;
	}

	VectorCopy( tag->origin, origin );

	return 1;
}

/*
-------------------------
TAG_GetOrigin2
Had to get rid of that damn assert for dev
-------------------------
*/

int	TAG_GetOrigin2( const char *owner, const char *name, vec3_t origin )
{
	reference_tag_t	*tag = TAG_Find( owner, name );

	if( tag == NULL )
	{
		return 0;
	}

	VectorCopy( tag->origin, origin );

	return 1;
}
/*
-------------------------
TAG_GetAngles
-------------------------
*/

int	TAG_GetAngles( const char *owner, const char *name, vec3_t angles )
{
	reference_tag_t	*tag = TAG_Find( owner, name );

	if (!tag)
	{
		assert(0);
		return 0;
	}

	VectorCopy( tag->angles, angles );
	
	return 1;
}

/*
-------------------------
TAG_GetRadius
-------------------------
*/

int TAG_GetRadius( const char *owner, const char *name )
{
	reference_tag_t	*tag = TAG_Find( owner, name );

	if (!tag)
	{
		assert(0);
		return 0;
	}

	return tag->radius;
}

/*
-------------------------
TAG_GetFlags
-------------------------
*/

int TAG_GetFlags( const char *owner, const char *name )
{
	reference_tag_t	*tag = TAG_Find( owner, name );

	if (!tag)
	{
		assert(0);
		return 0;
	}

	return tag->flags;
}

/*
==============================================================================

Spawn functions

==============================================================================
*/

/*QUAKED ref_tag_huge (0.5 0.5 1) (-128 -128 -128) (128 128 128)
SAME AS ref_tag, JUST BIGGER SO YOU CAN SEE THEM IN EDITOR ON HUGE MAPS!

Reference tags which can be positioned throughout the level.
These tags can later be refered to by the scripting system
so that their origins and angles can be referred to.

If you set angles on the tag, these will be retained.

If you target a ref_tag at an entity, that will set the ref_tag's
angles toward that entity.

If you set the ref_tag's ownername to the ownername of an entity,
it makes that entity is the owner of the ref_tag.  This means 
that the owner, and only the owner, may refer to that tag.

Tags may not have the same name as another tag with the same
owner.  However, tags with different owners may have the same
name as one another.  In this way, scripts can generically 
refer to tags by name, and their owners will automatically
specifiy which tag is being referred to.

targetname	- the name of this tag
ownername	- the owner of this tag
target		- use to point the tag at something for angles
*/

/*QUAKED ref_tag (0.5 0.5 1) (-8 -8 -8) (8 8 8)

Reference tags which can be positioned throughout the level.
These tags can later be refered to by the scripting system
so that their origins and angles can be referred to.

If you set angles on the tag, these will be retained.

If you target a ref_tag at an entity, that will set the ref_tag's
angles toward that entity.

If you set the ref_tag's ownername to the ownername of an entity,
it makes that entity is the owner of the ref_tag.  This means 
that the owner, and only the owner, may refer to that tag.

Tags may not have the same name as another tag with the same
owner.  However, tags with different owners may have the same
name as one another.  In this way, scripts can generically 
refer to tags by name, and their owners will automatically
specifiy which tag is being referred to.

targetname	- the name of this tag
ownername	- the owner of this tag
target		- use to point the tag at something for angles
*/

void ref_link ( gentity_t *ent )
{
	reference_tag_t	*tag;

	if ( ent->target )
	{
		//TODO: Find the target and set our angles to that direction
		gentity_t	*target = G_Find( NULL, FOFS(targetname), ent->target );
		vec3_t	dir;

		if ( target )
		{
			//Find the direction to the target
			VectorSubtract( target->s.origin, ent->s.origin, dir );
			VectorNormalize( dir );
			vectoangles( dir, ent->s.angles );
			
			//FIXME: Does pitch get flipped?
		}
		else
		{
			//[CoOp]
			//added newline
			Com_Printf( S_COLOR_RED"ERROR: ref_tag (%s) has invalid target (%s)\n", ent->targetname, ent->target );
			//[/CoOp]
		}
	}
	
	//Add the tag
	tag = TAG_Add( ent->targetname, ent->ownername, ent->s.origin, ent->s.angles, 16, 0 );

	//Delete immediately, cannot be refered to as an entity again
	//NOTE: this means if you wanted to link them in a chain for, say, a path, you can't
	G_FreeEntity( ent );
}

void SP_reference_tag ( gentity_t *ent )
{
	if ( ent->target )
	{
		//Init cannot occur until all entities have been spawned
		ent->think = ref_link;
		ent->nextthink = level.time + START_TIME_LINK_ENTS;
	}
	else
	{
		ref_link( ent );
	}
}

/*QUAKED misc_weapon_shooter (1 0 0) (-8 -8 -8) (8 8 8) ALTFIRE TOGGLE
ALTFIRE - fire the alt-fire of the chosen weapon
TOGGLE - keep firing until used again (fires at intervals of "wait")

"wait" - debounce time between refires (defaults to 500)
//[CoOp]
//SP entity attributes
"delay" - speed of WP_THERMAL (default is 900)
"random" - ranges from 0 to random, added to wait (defaults to 0)
//[/CoOp]

"target" - what to aim at (will update aim every frame if it's a moving target)  

"weapon" - specify the weapon to use (default is WP_BLASTER)
	WP_BRYAR_PISTOL
	WP_BLASTER
	WP_DISRUPTOR
	WP_BOWCASTER
	WP_REPEATER
	WP_DEMP2
	WP_FLECHETTE
	WP_ROCKET_LAUNCHER
	WP_THERMAL
	WP_TRIP_MINE
	WP_DET_PACK
	WP_STUN_BATON
	WP_EMPLACED_GUN
	WP_BOT_LASER
	WP_TURRET
	WP_ATST_MAIN
	WP_ATST_SIDE
	WP_TIE_FIGHTER
	WP_RAPID_FIRE_CONC
	WP_BLASTER_PISTOL
*/
//kind of hacky, but we have to do this with no dynamic allocation
#define MAX_SHOOTERS		16
typedef struct shooterClient_s
{
	gclient_t		cl;
	qboolean		inuse;
} shooterClient_t;
static shooterClient_t g_shooterClients[MAX_SHOOTERS];
static qboolean g_shooterClientInit = qfalse;

gclient_t *G_ClientForShooter(void)
{
	int i = 0;

	if (!g_shooterClientInit)
	{ //in theory it should be initialized to 0 on the stack, but just in case.
		memset(g_shooterClients, 0, sizeof(shooterClient_t)*MAX_SHOOTERS);
		g_shooterClientInit = qtrue;
	}

	while (i < MAX_SHOOTERS)
	{
		if (!g_shooterClients[i].inuse)
		{
			return &g_shooterClients[i].cl;
		}
		i++;
	}

	Com_Error(ERR_DROP, "No free shooter clients - hit MAX_SHOOTERS");
	return NULL;
}

void G_FreeClientForShooter(gclient_t *cl)
{
	int i = 0;
	while (i < MAX_SHOOTERS)
	{
		if (&g_shooterClients[i].cl == cl)
		{
			g_shooterClients[i].inuse = qfalse;
			return;
		}
		i++;
	}
}

//[SPPortComplete]
void misc_weapon_shooter_fire( gentity_t *self )
{
	FireWeapon( self, (self->spawnflags&1) );
	if ( (self->spawnflags&2) )
	{//repeat
		self->think = misc_weapon_shooter_fire;
		//[CoOp]
		if (self->random)
		{
			self->nextthink = level.time + self->wait + (int)(random()*self->random);
		}
		else
		{
			self->nextthink = level.time + self->wait;
		}
		//self->nextthink = level.time + self->wait;
		//[/CoOp]
	}
}


void misc_weapon_shooter_use ( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->think == misc_weapon_shooter_fire )
	{//repeating fire, stop
		/*
		G_FreeClientForShooter(self->client);
		self->think = G_FreeEntity;
		self->nextthink = level.time;
		*/
		self->nextthink = 0;
		return;
	}
	//otherwise, fire
	misc_weapon_shooter_fire( self );
}


void misc_weapon_shooter_aim( gentity_t *self )
{
	//update my aim
	if ( self->target )
	{
		gentity_t *targ = G_Find( NULL, FOFS(targetname), self->target );
		if ( targ )
		{
			self->enemy = targ;
			//[CoOp]
			//SP Code
			VectorSubtract( targ->r.currentOrigin, self->r.currentOrigin, self->client->renderInfo.muzzleDir );
			VectorCopy( targ->r.currentOrigin, self->pos1 );
			vectoangles( self->client->renderInfo.muzzleDir, self->client->ps.viewangles );
			/*
			VectorSubtract( targ->r.currentOrigin, self->r.currentOrigin, self->pos1 );
			VectorCopy( targ->r.currentOrigin, self->pos1 );
			vectoangles( self->pos1, self->client->ps.viewangles );
			*/
			//[/CoOp]
			SetClientViewAngle( self, self->client->ps.viewangles );
			//FIXME: don't keep doing this unless target is a moving target?
			self->nextthink = level.time + FRAMETIME;
		}
		else
		{
			self->enemy = NULL;
		}
	}
}


#include "../namespace_begin.h"
extern stringID_table_t WPTable[];
#include "../namespace_end.h"

void SP_misc_weapon_shooter( gentity_t *self )
{
	char *s;

	//alloc a client just for the weapon code to use
	self->client = G_ClientForShooter();//(gclient_s *)gi.Malloc(sizeof(gclient_s), TAG_G_ALLOC, qtrue);

	G_SpawnString("weapon", "", &s);

	//set weapon
	self->s.weapon = self->client->ps.weapon = WP_BLASTER;
	if ( s && s[0] )
	{//use a different weapon
		//[CoOp]
		//added sanity check to prevent a -1 weapon being set
		int newWeap = GetIDForString( WPTable, s );

		if(newWeap != -1)
		{
			self->s.weapon = self->client->ps.weapon = newWeap;
		}
		//self->s.weapon = self->client->ps.weapon = GetIDForString( WPTable, s );
		//[/CoOp]
	}

	//racc - load the weapon assets on the clients for this weapon.
	RegisterItem(BG_FindItemForWeapon(self->s.weapon));

	//set where our muzzle is
	VectorCopy( self->s.origin, self->client->renderInfo.muzzlePoint );
	//permanently updated (don't need for MP)
	//self->client->renderInfo.mPCalcTime = Q3_INFINITE; //racc - is must be the update time for the renderInfo.  Cool!

	//set up to link
	if ( self->target )
	{
        self->think = misc_weapon_shooter_aim;
		self->nextthink = level.time + START_TIME_LINK_ENTS;
	}
	else
	{//just set aim angles
		VectorCopy( self->s.angles, self->client->ps.viewangles );
		//[CoOp]
		AngleVectors( self->s.angles, self->client->renderInfo.muzzleDir, NULL, NULL );
		//AngleVectors( self->s.angles, self->pos1, NULL, NULL );
		//[/CoOp]
	}

	//set up to fire when used
    self->use = misc_weapon_shooter_use;

	if ( !self->wait )
	{
		self->wait = 500;
	}
}
//[/SPPortComplete]

/*QUAKED misc_weather_zone (0 .5 .8) ? 
Determines a region to check for weather contents - will significantly reduce load time
*/
void SP_misc_weather_zone( gentity_t *ent )
{
	G_FreeEntity(ent);
}

//[CoOp]
/*QUAKED misc_trip_mine (0.2 0.8 0.2) (-4 -4 -4) (4 4 4) START_ON BROADCAST START_OFF
Place in a map and point the angles at whatever surface you want it to attach to.

START_ON - If you give it a targetname to make it toggle-able, but want it to start on, set this flag
BROADCAST - ONLY USE THIS IF YOU HAVE TO!  causes the trip wire and loop sound to always happen, use this if the beam drops out in certain situations
START_OFF - If you give it a targetname, it will start completely off (laser AND base unit)  until used.

The trip mine will attach to that surface and fire it's beam away from the surface at an angle perpendicular to it.

targetname - starts off, when used, turns on (toggles)

FIXME: sometimes we want these to not be shootable... maybe just put them behind a force field?
*/
void CreateLaserTrap( gentity_t *laserTrap, vec3_t start, gentity_t *owner );
void laserTrapStick( gentity_t *ent, vec3_t endpos, vec3_t normal );
gitem_t	*BG_FindItemForAmmo( ammo_t ammo );
void SP_misc_trip_mine(gentity_t *ent)
{	
	//RAFIXME:  Add toggling ability
	gentity_t *laserTrap = G_Spawn();
	vec3_t fwd;
	
	//pre load the tripmine effects.
	RegisterItem(BG_FindItemForWeapon(WP_TRIP_MINE));

	CreateLaserTrap(laserTrap, ent->s.origin, &g_entities[ENTITYNUM_WORLD]);
	laserTrap->count = 1;
	trap_LinkEntity(laserTrap);

	AngleVectors(ent->s.angles, fwd, NULL, NULL);
	VectorScale(fwd, -1, fwd);
	laserTrapStick(laserTrap, ent->s.origin, fwd);
	laserTrap->r.svFlags |= SVF_BROADCAST; // these should show up through areaportals
}

#define RACK_BLASTER	1
#define RACK_REPEATER	2
#define RACK_ROCKET		4

void GunRackAddItem( gitem_t *gun, vec3_t org, vec3_t angs, float ffwd, float fright, float fup )
{
	vec3_t		fwd, right;
	gentity_t	*it_ent = G_Spawn();
	qboolean	rotate = qtrue;
	int skill =(int) trap_Cvar_VariableValue( "g_spSkill" );
	int t;

	AngleVectors( angs, fwd, right, NULL );

	if ( it_ent && gun )
	{
		// FIXME: scaling the ammo will probably need to be tweaked to a reasonable amount...adjust as needed
		// Set base ammo per type
		if ( gun->giType == IT_WEAPON )
		{
			switch( gun->giTag )
			{
			case WP_BLASTER:
				it_ent->count = 15;
				break;
			case WP_REPEATER:
				it_ent->count = 100;
				break;
			case WP_ROCKET_LAUNCHER:
				it_ent->count = 4;
				break;
			}
		}
		else
		{
			rotate = qfalse;

			// must deliberately make it small, or else the objects will spawn inside of each other.
			VectorSet( it_ent->r.maxs, 6.75f, 6.75f, 6.75f );
			VectorScale( it_ent->r.maxs, -1, it_ent->r.mins );
		}

		it_ent->spawnflags |= 1; //ITMSF_SUSPEND
		it_ent->classname = G_NewString(gun->classname);	//copy it so it can be freed safely
		G_SpawnItem( it_ent, gun );

		if(!it_ent->item)
		{//this occurs when this particular weapon has been disabled in this game.
			//The entity has already been freed so just quit.
			return;
		}

		// FinishSpawningItem handles everything, so clear the thinkFunc that was set in G_SpawnItem
		FinishSpawningItem( it_ent );

		if ( gun->giType == IT_AMMO )
		{
			if ( gun->giTag == AMMO_BLASTER ) // I guess this just has to use different logic??
			{
				if ( skill >= 2 )
				{
					it_ent->count += 10; // give more on higher difficulty because there will be more/harder enemies?
				}
			}
			else
			{
				// scale ammo based on skill
				switch ( skill )
				{
				case 0: // do default
					break;
				case 1:
					it_ent->count *= 0.75f;
					break;
				case 2:
					it_ent->count *= 0.5f;
					break;
				}
			}
		}
		else if( gun->giType == IT_WEAPON )
		{//count as dropped weapon to prevent the magical respawn effects
			it_ent->s.eFlags |= EF_DROPPEDWEAPON;
		}

		it_ent->nextthink = 0;

		VectorCopy( org, it_ent->s.origin );
		VectorMA( it_ent->s.origin, fright, right, it_ent->s.origin );
		VectorMA( it_ent->s.origin, ffwd, fwd, it_ent->s.origin );
		it_ent->s.origin[2] += fup;

		if(gun->giType == IT_WEAPON)
		{//guns need to be up a little bit more
			it_ent->s.origin[2] += 10;
		}

		VectorCopy( angs, it_ent->s.angles );

		// by doing this, we can force the amount of ammo we desire onto the weapon for when it gets picked-up
		it_ent->flags |= ( FL_DROPPED_ITEM /*| FL_FORCE_PULLABLE_ONLY*/ );
		it_ent->physicsBounce = 0.1f;

		for ( t = 0; t < 3; t++ )
		{
			if ( rotate )
			{
				if ( t == YAW )
				{
					it_ent->s.angles[t] = AngleNormalize180( it_ent->s.angles[t] + 180 + crandom() * 14 );
				}
				else if( t == PITCH && gun->giType == IT_WEAPON)
				{
					it_ent->s.angles[t] = AngleNormalize180( it_ent->s.angles[t] + crandom() * 4 - 75 );
				}
				else
				{
					it_ent->s.angles[t] = AngleNormalize180( it_ent->s.angles[t] + crandom() * 4 );
				}
			}
			else
			{
				if ( t == YAW )
				{
					it_ent->s.angles[t] = AngleNormalize180( it_ent->s.angles[t] + 90 + crandom() * 4 );
				}
			}
		}

		G_SetAngles( it_ent, it_ent->s.angles );
		G_SetOrigin( it_ent, it_ent->s.origin );
		trap_LinkEntity( it_ent );
	}
}

/*QUAKED misc_model_gun_rack (1 0 0.25) (-14 -14 -4) (14 14 30) BLASTER REPEATER ROCKET
model="models/map_objects/kejim/weaponsrack.md3"

NOTE: can mix and match these spawnflags to get multi-weapon racks.  If only one type is checked the rack will be full of those weapons
BLASTER - Puts one or more blaster guns on the rack.
REPEATER - Puts one or more repeater guns on the rack.
ROCKET - Puts one or more rocket launchers on the rack.
*/
void SP_misc_model_gun_rack( gentity_t *ent )
{
	int i;
	gitem_t		*blaster = NULL, *repeater = NULL, *rocket = NULL;
	int			ct = 0;
	float		ofz[3];
	gitem_t		*itemList[3];

	// If BLASTER is checked...or nothing is checked then we'll do blasters
	if (( ent->spawnflags & RACK_BLASTER ) || !(ent->spawnflags & ( RACK_BLASTER | RACK_REPEATER | RACK_ROCKET )))
	{
		blaster	= BG_FindItemForWeapon( WP_BLASTER );
	}

	if (( ent->spawnflags & RACK_REPEATER ))
	{
		repeater = BG_FindItemForWeapon( WP_REPEATER );
	}

	if (( ent->spawnflags & RACK_ROCKET ))
	{
		rocket = BG_FindItemForWeapon( WP_ROCKET_LAUNCHER );
	}

	//---------weapon types
	if ( blaster )
	{
		ofz[ct] = 23.0f;
		itemList[ct++] = blaster;
	}

	if ( repeater )
	{
		ofz[ct] = 24.5f;
		itemList[ct++] = repeater;
	}

	if ( rocket )
	{
		ofz[ct] = 25.5f;
		itemList[ct++] = rocket;
	}

	if ( ct ) //..should always have at least one item on their, but just being safe
	{
		for ( ; ct < 3 ; ct++ )
		{
			ofz[ct] = ofz[0];
			itemList[ct] = itemList[0]; // first weapon ALWAYS propagates to fill up the shelf
		}
	}

	// now actually add the items to the shelf...validate that we have a list to add
	if ( ct )
	{
		for ( i = 0; i < ct; i++ )
		{
			GunRackAddItem( itemList[i], ent->s.origin, ent->s.angles, crandom() * 2, ( i - 1 ) * 9 + crandom() * 2, ofz[i] );
		}
	}

	ent->s.modelindex = G_ModelIndex( "models/map_objects/kejim/weaponsrack.md3" );

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );

	ent->r.contents = CONTENTS_SOLID;

	trap_LinkEntity( ent );
}


#define RACK_METAL_BOLTS	2
#define RACK_ROCKETS		4
#define RACK_WEAPONS		8
#define RACK_HEALTH			16
#define RACK_PWR_CELL		32
#define RACK_NO_FILL		64
// AMMO RACK!!
void spawn_rack_goods( gentity_t *ent )
{
	int i;
	float		v_off = 0;
	gitem_t		*blaster = NULL, *metal_bolts = NULL, *rockets = NULL, *it = NULL;
	gitem_t		*am_blaster = NULL, *am_metal_bolts = NULL, *am_rockets = NULL, *am_pwr_cell = NULL;
	gitem_t		*health = NULL;
	int			pos = 0, ct = 0;
	gitem_t		*itemList[4]; // allocating 4, but we only use 3.  done so I don't have to validate that the array isn't full before I add another

	trap_UnlinkEntity( ent );

	// If BLASTER is checked...or nothing is checked then we'll do blasters
	if (( ent->spawnflags & RACK_BLASTER ) || !(ent->spawnflags & ( RACK_BLASTER | RACK_METAL_BOLTS | RACK_ROCKETS | RACK_PWR_CELL )))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			blaster	= BG_FindItemForWeapon( WP_BLASTER );
		}
		am_blaster	= BG_FindItemForAmmo( AMMO_BLASTER );
	}

	if (( ent->spawnflags & RACK_METAL_BOLTS ))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			metal_bolts = BG_FindItemForWeapon( WP_REPEATER );
		}
		am_metal_bolts = BG_FindItemForAmmo( AMMO_METAL_BOLTS );
	}

	if (( ent->spawnflags & RACK_ROCKETS ))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			rockets = BG_FindItemForWeapon( WP_ROCKET_LAUNCHER );
		}
		am_rockets = BG_FindItemForAmmo( AMMO_ROCKETS );
	}

	if (( ent->spawnflags & RACK_PWR_CELL ))
	{
		am_pwr_cell = BG_FindItemForAmmo( AMMO_POWERCELL );
	}

	if (( ent->spawnflags & RACK_HEALTH ))
	{
		health = BG_FindItem( "item_medpak_instant" );
		RegisterItem( health );
	}

	//---------Ammo types
	if ( am_blaster )
	{
		itemList[ct++] = am_blaster;
	}

	if ( am_metal_bolts )
	{
		itemList[ct++] = am_metal_bolts;
	}

	if ( am_pwr_cell )
	{
		itemList[ct++] = am_pwr_cell;
	}

	if ( am_rockets )
	{
		itemList[ct++] = am_rockets;
	}

	if ( !(ent->spawnflags & RACK_NO_FILL) && ct ) //double negative..should always have at least one item on there, but just being safe
	{
		for ( ; ct < 3 ; ct++ )
		{
			itemList[ct] = itemList[0]; // first item ALWAYS propagates to fill up the shelf
		}
	}

	// now actually add the items to the shelf...validate that we have a list to add
	if ( ct )
	{
		for (i = 0; i < ct; i++ )
		{
			GunRackAddItem( itemList[i], ent->s.origin, ent->s.angles, crandom() * 0.5f, (i-1)* 8, 7.0f );
		}
	}

	// -----Weapon option
	if ( ent->spawnflags & RACK_WEAPONS )
	{
		if ( !(ent->spawnflags & ( RACK_BLASTER | RACK_METAL_BOLTS | RACK_ROCKETS | RACK_PWR_CELL )))
		{
			// nothing was selected, so we assume blaster pack
			it = blaster;
		}
		else
		{
			// if weapon is checked...and so are one or more ammo types, then pick a random weapon to display..always give weaker weapons first
			if ( blaster )
			{
				it = blaster;
				v_off = 25.5f;
			}
			else if ( metal_bolts )
			{
				it = metal_bolts;
				v_off = 27.0f;
			}
			else if ( rockets )
			{
				it = rockets;
				v_off = 28.0f;
			}
		}

		if ( it )
		{
			// since we may have to put up a health pack on the shelf, we should know where we randomly put
			//	the gun so we don't put the pack on the same spot..so pick either the left or right side
			pos = ( random() > .5 ) ? -1 : 1;

			GunRackAddItem( it, ent->s.origin, ent->s.angles, crandom() * 2, ( random() * 6 + 4 ) * pos, v_off );
		}
	}

	// ------Medpack
	if (( ent->spawnflags & RACK_HEALTH ) && health )
	{
		if ( !pos )
		{
			// we haven't picked a side already...
			pos = ( random() > .5 ) ? -1 : 1;
		}
		else
		{
			// switch to the opposite side
			pos *= -1;
		}

		GunRackAddItem( health, ent->s.origin, ent->s.angles, crandom() * 0.5f, ( random() * 4 + 4 ) * pos, 24 );
	}

	ent->s.modelindex = G_ModelIndex( "models/map_objects/kejim/weaponsrung.md3" );

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );

	trap_LinkEntity( ent );
}


/*QUAKED misc_model_ammo_rack (1 0 0.25) (-14 -14 -4) (14 14 30) BLASTER METAL_BOLTS ROCKETS WEAPON HEALTH PWR_CELL NO_FILL
model="models/map_objects/kejim/weaponsrung.md3"

NOTE: can mix and match these spawnflags to get multi-ammo racks.  If only one type is checked the rack will be full of that ammo.  Only three ammo packs max can be displayed.


BLASTER - Adds one or more ammo packs that are compatible with Blasters and the Bryar pistol.
METAL_BOLTS - Adds one or more metal bolt ammo packs that are compatible with the heavy repeater and the flechette gun
ROCKETS - Puts one or more rocket packs on a rack.
WEAPON - adds a weapon matching a selected ammo type to the rack.
HEALTH - adds a health pack to the top shelf of the ammo rack
PWR_CELL - Adds one or more power cell packs that are compatible with the Disuptor, bowcaster, and demp2
NO_FILL - Only puts selected ammo on the rack, it never fills up all three slots if only one or two items were checked
*/

extern gitem_t	*FindItemForAmmo( ammo_t ammo );
void SP_misc_model_ammo_rack( gentity_t *ent )
{
// If BLASTER is checked...or nothing is checked then we'll do blasters
	if (( ent->spawnflags & RACK_BLASTER ) || !(ent->spawnflags & ( RACK_BLASTER | RACK_METAL_BOLTS | RACK_ROCKETS | RACK_PWR_CELL )))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			RegisterItem( BG_FindItemForWeapon( WP_BLASTER ));
		}
		RegisterItem( BG_FindItemForAmmo( AMMO_BLASTER ));
	}

	if (( ent->spawnflags & RACK_METAL_BOLTS ))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			RegisterItem( BG_FindItemForWeapon( WP_REPEATER ));
		}
		RegisterItem( BG_FindItemForAmmo( AMMO_METAL_BOLTS ));
	}

	if (( ent->spawnflags & RACK_ROCKETS ))
	{
		if ( ent->spawnflags & RACK_WEAPONS )
		{
			RegisterItem( BG_FindItemForWeapon( WP_ROCKET_LAUNCHER ));
		}
		RegisterItem( BG_FindItemForAmmo( AMMO_ROCKETS ));
	}

	if (( ent->spawnflags & RACK_PWR_CELL ))
	{
		RegisterItem( BG_FindItemForAmmo( AMMO_POWERCELL ));
	}

	if (( ent->spawnflags & RACK_HEALTH ))
	{
		RegisterItem( BG_FindItem( "item_medpak_instant" ));
	}

	ent->think = spawn_rack_goods;
	ent->nextthink = level.time + 100;

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );

	ent->r.contents = CONTENTS_SHOTCLIP|CONTENTS_PLAYERCLIP|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP;//CONTENTS_SOLID;//so use traces can go through them

	trap_LinkEntity( ent );
}

#define DROP_MEDPACK	1
#define DROP_SHIELDS	2
#define DROP_BACTA		4
#define DROP_BATTERIES	8

/*QUAKED misc_model_gun_rack (1 0 0.25) (-14 -14 -4) (14 14 30) BLASTER REPEATER ROCKET
model="models/map_objects/kejim/weaponsrack.md3"

NOTE: can mix and match these spawnflags to get multi-weapon racks.  If only one type is checked the rack will be full of those weapons
BLASTER - Puts one or more blaster guns on the rack.
REPEATER - Puts one or more repeater guns on the rack.
ROCKET - Puts one or more rocket launchers on the rack.
*/
/* RACC - my attempt at the rack entity code.  not used since I got my hands on the real stuff
void SP_misc_model_gun_rack(gentity_t *ent)
{
	//RAFIXME - for now we're just going to create the model
	ent->model="models/map_objects/kejim/weaponsrack.md3";

	VectorSet (ent->r.mins, -14, -14, -4);
	VectorSet (ent->r.maxs, 14, 14, 30);

	InitModel(ent);
}
*/


/*QUAKED misc_model_ammo_rack (1 0 0.25) (-14 -14 -4) (14 14 30) BLASTER METAL_BOLTS ROCKETS WEAPON HEALTH PWR_CELL NO_FILL
model="models/map_objects/kejim/weaponsrung.md3"

NOTE: can mix and match these spawnflags to get multi-ammo racks.  If only one type is checked the rack will be full of that ammo.  Only three ammo packs max can be displayed.


BLASTER - Adds one or more ammo packs that are compatible with Blasters and the Bryar pistol.
METAL_BOLTS - Adds one or more metal bolt ammo packs that are compatible with the heavy repeater and the flechette gun
ROCKETS - Puts one or more rocket packs on a rack.
WEAPON - adds a weapon matching a selected ammo type to the rack.
HEALTH - adds a health pack to the top shelf of the ammo rack
PWR_CELL - Adds one or more power cell packs that are compatible with the Disuptor, bowcaster, and demp2
NO_FILL - Only puts selected ammo on the rack, it never fills up all three slots if only one or two items were checked
*/
/* RACC - my attempt at the rack entity code.  not used since I got my hands on the real stuff
void misc_model_ammo_rack(gentity_t *ent)
{
	//RAFIXME - for now we're just going to create the model
	ent->model="models/map_objects/kejim/weaponsrack.md3";

	VectorSet (ent->r.mins, -14, -14, -4);
	VectorSet (ent->r.maxs, 14, 14, 30);

	InitModel(ent);
}
*/


/*QUAKED misc_camera (0 0 1) (-8 -8 -12) (8 8 16) VULNERABLE
A model in the world that can be used by the player to look through it's viewpoint

There will be a video overlay instead of the regular HUD and the FOV will be wider

VULNERABLE - allow camera to be destroyed

"target" - camera will remain pointed at this entity (if it's a train or some other moving object, it will keep following it)
"target2" - when player is in this camera and hits the use button, it will cycle to this next camera (if no target2, returns to normal view )
"target3" - thing to use when player enters this camera view
"target4" - thing to use when player leaves this camera view
"closetarget" - (sigh...) yet another target, fired this when it's destroyed
"wait"    - how long to wait between being used (default 0.5)
*/
//all this stuff ported from SP
extern void ICam_Disable( void );
void camera_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod)
{
	vec3_t ang;
	
	if ( self->genericValue1 )
	{//I'm dead so boot the player out of the view if he's already in it.
		G_UseTargets2( self, &g_entities[self->genericValue2], self->target4 );
		ICam_Disable();
		G_AddEvent( self, EV_GLOBAL_SOUND, self->soundPos2 );
	}

	G_UseTargets2( self, &g_entities[self->genericValue2], self->closetarget );
	//FIXME: explosion fx/sound
	//leave sparks at origin- where base's pole is still at?

	VectorCopy(self->s.angles, ang);
	ang[0] = 180;
	G_PlayEffectID( G_EffectIndex("sparks/spark"), self->s.origin, ang);
	/* RACC - implimented above
	gentity_t *sparks = G_Spawn();
	if ( sparks )
	{
		sparks->fxFile = "sparks/spark";
		sparks->delay = 100;
		sparks->random = 500;
		sparks->s.angles[0] = 180;//point down
		VectorCopy( self->s.origin, sparks->s.origin );
		SP_fx_runner( sparks );
	}
	*/

	//bye!
	self->takedamage = qfalse;
	self->r.contents = 0;
	self->s.eFlags |= EF_NODRAW;
	self->s.modelindex = 0;
}


//ported from SP
extern void ICam_Move( vec3_t dest, float duration );
extern void ICam_Pan( vec3_t dest, vec3_t panDirection, float duration );
extern void ICam_Enable(void);
void camera_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( !activator || !activator->client || activator->s.number )
	{//really only usable by the player
		return;
	}
	self->painDebounceTime = level.time + (self->wait*1000);//FRAMETIME*5;//don't check for player buttons for 500 ms

	// FIXME: I guess we are allowing them to switch to a dead camera.  Maybe we should conditionally do this though?
	if ( self->genericValue1 )
	{//I'm already viewEntity, or I'm destroyed, find next
		gentity_t *next = NULL;
		if ( self->target2 != NULL )
		{
			next = G_Find( NULL, FOFS(targetname), self->target2 );
		}
		if ( next )
		{//found another one
			if ( !Q_stricmp( "misc_camera", next->classname ) )
			{//make sure it's another camera
				camera_use( next, other, activator );
				//I'm not the camera anymore
				self->genericValue1 = 0;
			}
		}
		else //if ( self->health > 0 )
		{//I was the last (only?) one, clear out the viewentity
			G_UseTargets2( self, &g_entities[self->genericValue2], self->target4 );
			ICam_Disable();
			G_AddEvent( self, EV_GLOBAL_SOUND, self->soundPos2 );
		}
	}
	else
	{//set me as view entity
		G_UseTargets2( self, activator, self->target3 );
		self->s.eFlags |= EF_NODRAW;
		self->s.modelindex = 0;
		//G_SetViewEntity( activator, self );
		ICam_Enable();
		ICam_Move(self->s.origin, 0);
		ICam_Pan(self->s.angles, vec3_origin, 0);
		G_AddEvent( self, EV_GLOBAL_SOUND, self->soundPos1 );
		//RACC - we're makign this global instead
		//G_Sound( activator, self->soundPos1 );
		self->genericValue1 = 1;
		self->genericValue2 = activator->client->ps.clientNum;
	}
}


//ported from SP
extern qboolean InFOV3( vec3_t spot, vec3_t from, vec3_t fromAngles, int hFOV, int vFOV );
void camera_aim( gentity_t *self )
{
	int i;
	gentity_t *player;
	self->nextthink = level.time + FRAMETIME;
	if ( self->genericValue1 )
	{//I am the viewEntity
		player = &g_entities[self->genericValue2];
		if ( player->client->pers.cmd.forwardmove || player->client->pers.cmd.rightmove || player->client->pers.cmd.upmove )
		{//player wants to back out of camera
			G_UseTargets2( self, player, self->target4 );
			ICam_Disable();
			G_AddEvent( self, EV_GLOBAL_SOUND, self->soundPos1 );
			//G_Sound( player, self->soundPos2 );
			self->painDebounceTime = level.time + (self->wait*1000);//FRAMETIME*5;//don't check for player buttons for 500 ms
			if ( player->client->pers.cmd.upmove > 0 )
			{//stop player from doing anything for a half second after
				player->aimDebounceTime = level.time + 500;
			}
			self->genericValue1 = 0;
			self->genericValue2 = 0;
		}
		else if ( self->painDebounceTime < level.time )
		{//check for use button
			if ( (player->client->pers.cmd.buttons&BUTTON_USE) )
			{//player pressed use button, wants to cycle to next
				camera_use( self, player, player );
			}
		}
		else
		{//don't draw me when being looked through
			self->s.eFlags |= EF_NODRAW;
			self->s.modelindex = 0;
		}
	}
	else if ( self->health > 0 )
	{//still alive, can draw me again
		self->s.eFlags &= ~EF_NODRAW;
		//RAFIXME - modelindex3?
		//self->s.modelindex = self->s.modelindex3;
	}
	//update my aim
	if ( self->target )
	{
		gentity_t *targ = G_Find( NULL, FOFS(targetname), self->target );
		if ( targ )
		{
			vec3_t angles, dir;
			VectorSubtract( targ->r.currentOrigin, self->r.currentOrigin, dir );
			vectoangles( dir, angles );
			//FIXME: if a G2 model, do a bone override..???
			VectorCopy( self->r.currentAngles, self->s.apos.trBase );

			for( i = 0; i < 3; i++ )
			{
				angles[i] = AngleNormalize180( angles[i] );
				self->s.apos.trDelta[i] = AngleNormalize180( (angles[i]-self->r.currentAngles[i])*10 );
			}
			//VectorSubtract( angles, self->currentAngles, self->s.apos.trDelta );
			//VectorScale( self->s.apos.trDelta, 10, self->s.apos.trDelta );
			self->s.apos.trTime = level.time;
			self->s.apos.trDuration = FRAMETIME;
			VectorCopy( angles, self->r.currentAngles );

			if ( DistanceSquared( self->r.currentAngles, self->pos1 ) > 0.01f ) // if it moved at all, start a loop sound? not exactly the "bestest" solution
			{
				self->s.loopSound = G_SoundIndex( "sound/movers/objects/cameramove_lp2" );
			}
			else
			{
				self->s.loopSound = 0; // not moving so don't bother
			}

			VectorCopy( self->r.currentAngles, self->pos1 );
			//G_SetAngles( self, angles );
		}

		if(self->genericValue1)
		{//players in camera, update angles
			ICam_Pan(self->r.currentAngles, vec3_origin, 50);
		}
	}

	if(self->target2 && !strcmp(self->classname, "misc_spotlight"))
	{//spotlights check their fov for players and fire a trigger if a player is spoted.
		gentity_t *player;
		int i = 0;
		for(; i < MAX_CLIENTS; i++)
		{
			player = &g_entities[i];

			if(!player->inuse || !player->client)
				continue;

			if(player->client->sess.sessionTeam == TEAM_SPECTATOR)
				continue;

			if(InFOV3(player->client->ps.origin, self->r.currentOrigin, 
				self->r.currentAngles, 15, 15))
			{//found a player,
				G_UseTargets2(self, player, self->target2);
				//G_Printf("Player %i: Found by spotlight.\n", player->client->ps.clientNum);
			}
		}
	}
}


//ported from SP
void SP_misc_camera( gentity_t *self ) 
{
	//FIXME: spawn base, too
	gentity_t *base = G_Spawn();

	G_SpawnFloat( "wait", "0.5", &self->wait );

	if ( base )
	{
		//[CoOp]
		//set things up so that the classname for the camera base has a classname
		base->classname = "misc_camera_base";
		//[/CoOp]
		base->s.modelindex = G_ModelIndex( "models/map_objects/kejim/impcam_base.md3" );
		VectorCopy( self->s.origin, base->s.origin );
		base->s.origin[2] += 16;
		G_SetOrigin( base, base->s.origin );
		G_SetAngles( base, self->s.angles );
		trap_LinkEntity( base );
	}
	//RAFIXME - impliment modelindex3?
	/*self->s.modelindex3 = */self->s.modelindex = G_ModelIndex( "models/map_objects/kejim/impcam.md3" );
	self->soundPos1 = G_SoundIndex( "sound/movers/camera_on.mp3" );
	self->soundPos2 = G_SoundIndex( "sound/movers/camera_off.mp3" );
	G_SoundIndex( "sound/movers/objects/cameramove_lp2" );

	G_SetOrigin( self, self->s.origin );
	G_SetAngles( self, self->s.angles );
	self->s.apos.trType = TR_LINEAR_STOP;//TR_INTERPOLATE;//
	self->alt_fire = qtrue;
	VectorSet( self->r.mins, -8, -8, -12 );
	VectorSet( self->r.maxs, 8, 8, 0 );
	self->r.contents = CONTENTS_SOLID;
	trap_LinkEntity( self );

	/*RAFIXME - impliment the sparks effect for when the camera bites it.
	self->fxID = G_EffectIndex( "sparks/spark" );
	*/

	if ( self->spawnflags & 1 ) // VULNERABLE
	{
		self->takedamage = qtrue;
	}

	self->health = 10;
	self->die = camera_die;
	
	self->use = camera_use;

	self->think = camera_aim;
	self->nextthink = level.time + START_TIME_LINK_ENTS;
}


//[CoOp]
/*QUAKED misc_spotlight (1 0 0) (-10 -10 0) (10 10 10) START_OFF
model="models/map_objects/imp_mine/spotlight.md3"
Search spotlight that must be targeted at a func_train or other entity
Uses its target2 when it detects the player

  START_OFF - Starts off

  targetname - Toggles it on/off
  target - What to point at
  target2 - What to use when detects player
*/
void SP_misc_spotlight( gentity_t *self ) 
{//racc - acts a lot like a misc_camera.
	//FIXME: spawn base, too
	//[CoOp]
	//gentity_t *base = G_Spawn();
	//[/CoOp]

	G_SpawnFloat( "wait", "0.5", &self->wait );

	/*
	if ( base )
	{
		//[CoOp]
		//set things up so that the classname for the camera base has a classname
		base->classname = "misc_camera_base";
		//[/CoOp]
		base->s.modelindex = G_ModelIndex( "models/map_objects/kejim/impcam_base.md3" );
		VectorCopy( self->s.origin, base->s.origin );
		base->s.origin[2] += 16;
		G_SetOrigin( base, base->s.origin );
		G_SetAngles( base, self->s.angles );
		trap_LinkEntity( base );
	}
	*/

	//RAFIXME - impliment modelindex3?
	/*self->s.modelindex3 = */self->s.modelindex = G_ModelIndex( "models/map_objects/imp_mine/spotlight.md3" );
	//self->soundPos1 = G_SoundIndex( "sound/movers/camera_on.mp3" );
	//self->soundPos2 = G_SoundIndex( "sound/movers/camera_off.mp3" );
	G_SoundIndex( "sound/movers/objects/cameramove_lp2" );

	G_SetOrigin( self, self->s.origin );
	G_SetAngles( self, self->s.angles );
	self->s.apos.trType = TR_LINEAR_STOP;//TR_INTERPOLATE;//
	self->alt_fire = qtrue;
	VectorSet( self->r.mins, -8, -8, -12 );
	VectorSet( self->r.maxs, 8, 8, 0 );
	self->r.contents = CONTENTS_SOLID;
	trap_LinkEntity( self );

	/*RAFIXME - impliment the sparks effect for when the camera bites it.
	self->fxID = G_EffectIndex( "sparks/spark" );
	*/

	/* I'll assume that spotlights can't die for the moment.
	if ( self->spawnflags & 1 ) // VULNERABLE
	{
		self->takedamage = qtrue;
	}

	self->health = 10;
	self->die = camera_die;
	*/
	
	self->use = NULL;
	//self->use = camera_use;

	self->think = camera_aim;
	self->nextthink = level.time + START_TIME_LINK_ENTS;
}


/*QUAKED misc_security_panel (0 .5 .8) (-8 -8 -8) (8 8 8) x x x x x x x INACTIVE
model="models/map_objects/kejim/sec_panel.md3"
  A model that just sits there and opens when a player uses it and has right key

INACTIVE - Start off, has to be activated to be usable

"message"	name of the key player must have
"target"	thing to use when successfully opened
"target2"	thing to use when player uses the panel without the key
*/
char KeyPool[10];	//key pool for security keys
void panel_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{
	if(self->genericValue1 < level.time)
	{
		if(!strcmp(KeyPool, self->message))
		{//we have the key to this door.
			//for now, just unlock when used
			G_UseTargets2(self, other, self->target);
			G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/movers/sec_panel_pass.mp3"));
		}
		else
		{//don't have the key.
			G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/movers/sec_panel_fail.mp3"));
		}

		self->genericValue1 = level.time + 5000; //don't use button for 5 seconds.
	}
}

void SP_misc_security_panel(gentity_t *self)
{
	if(self->spawnflags & 128)
	{
		self->flags |= FL_INACTIVE;
	}

	//precashe sounds
	G_SoundIndex("sound/movers/sec_panel_pass.mp3");
	G_SoundIndex("sound/movers/sec_panel_fail.mp3");

	G_SetAngles(self, self->s.angles);
	self->s.modelindex = G_ModelIndex( "models/map_objects/kejim/sec_panel.md3" );
	self->r.svFlags |= SVF_PLAYER_USABLE;
	self->r.contents |= CONTENTS_TRIGGER;
	VectorSet (self->r.mins, -16, -16, -16);
	VectorSet (self->r.maxs, 16, 16, 16);
	self->touch = panel_touch;

	self->genericValue1 = 0;	//set initial touch value.

	trap_LinkEntity(self);
}

/*QUAKED item_security_key (.3 .3 1) (-8 -8 0) (8 8 16) suspended
message - used to differentiate one key from another.
*/
void Precashe_Key(void)
{//precashe the stuff for the key objects
	G_ModelIndex( "models/items/key.md3" );
	G_SoundIndex( "sound/weapons/key_pkup.wav" );
}

int GoodieKeys = 0;	//number of goodie keys currently held.
qboolean INV_GoodieKeyGive( gentity_t *target )
{//add a goodie key to the goodie pool
	GoodieKeys++;
	return qtrue;
}

qboolean INV_SecurityKeyGive( gentity_t *target, const char *keyname )
{//add this key to the key pool
	strcpy(KeyPool, keyname);
	return qtrue;
}

void key_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{//touch code for keys
	if(other && other->s.number < MAX_CLIENTS)
	{//we got touched by a player.  Pick up with the key.
		INV_SecurityKeyGive( other, self->message );
		G_FreeEntity(self);	//remove key from game.
	}
}

void SP_item_security_key(gentity_t *self)
{
	//make sure all the nessicary key object data is precashed.
	Precashe_Key();

	G_SetAngles(self, self->s.angles);
	self->s.modelindex = G_ModelIndex( "models/items/key.md3" );
	self->r.svFlags |= SVF_PLAYER_USABLE;
	self->r.contents |= CONTENTS_TRIGGER;
	self->s.pos.trType = TR_LINEAR;
	self->s.eType = ET_GENERAL;
	VectorSet (self->r.mins, -16, -16, -16);
	VectorSet (self->r.maxs, 16, 16, 16);
	self->touch = key_touch;
	trap_LinkEntity(self);
}

void bomb_planted_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if ( self->count == 2 )
	{
		self->s.eFlags &= ~EF_NODRAW;
		self->r.contents = CONTENTS_SOLID;
		self->clipmask = MASK_SOLID;
		self->count = 1;
		self->s.loopSound = self->noise_index;
	}
	else if ( self->count == 1 )
	{
		self->count = 0;
		// play disarm sound
		self->setTime = level.time + 1000; // extra debounce so that the sounds don't overlap too much
		G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/overchargeend"));
		self->s.loopSound = 0;

		// this pauses the shader on one frame (more or less)
		self->s.eFlags |= EF_DISABLE_SHADER_ANIM;

		// this starts the animation for the model
		self->s.eFlags |= EF_ANIM_ONCE;
		self->s.frame = 0;

		//use targets
		G_UseTargets( self, activator );
	}
}

/*QUAKED misc_model_bomb_planted (1 0 0) (-16 -16 0) (16 16 70) x x x USETARGET
model="models/map_objects/factory/bomb_new_deact.md3"
Planted by evil men for evil purposes.

"health" - please don't shoot the thermonuclear device
"forcevisible" - When you turn on force sight (any level), you can see these draw through the entire level...
*/
//------------------------------------------------------------
void SP_misc_model_bomb_planted( gentity_t *ent )
{
	int forceVisible = 0;
	VectorSet( ent->r.mins, -16, -16, 0 );
	VectorSet( ent->r.maxs, 16, 16, 70 );

	ent->s.modelindex = G_ModelIndex("models/map_objects/factory/bomb_new_deact.md3");	// Precache model
	ent->s.modelindex2 = G_ModelIndex("models/map_objects/factory/bomb_new_deact.md3");	// Precache model
	ent->noise_index = G_SoundIndex("sound/interface/ammocon_run");

	ent->s.eFlags = 0;
	ent->r.svFlags |= SVF_PLAYER_USABLE;
	ent->r.contents = CONTENTS_SOLID;
	ent->clipmask = MASK_SOLID;

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	trap_LinkEntity (ent);

	ent->use = bomb_planted_use;

	G_SpawnInt( "material", "4", (int *)&ent->material );

	ent->takedamage = qfalse;

	G_SoundIndex("sound/weapons/overchargeend");

	ent->s.loopSound = ent->noise_index;
	//ent->s.eFlags |= EF_SHADER_ANIM;
	//ent->s.frame = ent->startFrame = 0;
	ent->count = 1;

	// If we have a targetname, we're are invisible until we are spawned in by being used.
	if ( ent->targetname )
	{
		ent->s.eFlags = EF_NODRAW;
		ent->r.contents = 0;
		ent->clipmask = 0;
		ent->count = 2;
		ent->s.loopSound = 0;
	}

	G_SpawnInt( "forcevisible", "0", &forceVisible );
	if ( forceVisible )
	{//can see these through walls with force sight, so must be broadcast
		ent->r.svFlags |= SVF_BROADCAST;
		ent->s.eFlags |= EF_FORCE_VISIBLE;
	}
}

extern void beacon_think( gentity_t *ent );
void beacon_deploy( gentity_t *ent )
{
	ent->think = beacon_think;
	ent->nextthink = level.time + FRAMETIME * 0.5f;

	ent->s.frame = 0;
	ent->startFrame = 0;
	ent->endFrame = 30;
	ent->loopAnim = qfalse;
}

void beacon_think( gentity_t *ent )
{
	ent->nextthink = level.time + FRAMETIME * 0.5f;

	// Deploy animation complete? Stop thinking and just animate signal forever.
	if ( ent->s.frame == 30 ) 
	{
		ent->think = NULL;
		ent->nextthink = -1;

		ent->startFrame = 31;
		ent->endFrame = 60;
		ent->loopAnim = qtrue;

		ent->s.loopSound = ent->noise_index;
	}
}

void beacon_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	// Every time it's used it will be toggled on or off.
	if ( self->count == 0 )
	{
		self->s.eFlags &= ~EF_NODRAW;
		self->r.contents = CONTENTS_SOLID;
		self->clipmask = MASK_SOLID;
		self->count = 1;
		self->r.svFlags = SVF_ANIMATING;
		beacon_deploy( self );
	}
	else
	{
		self->s.eFlags = EF_NODRAW;
		self->r.contents = 0; 
		self->clipmask = 0;
		self->count = 0;
		self->s.loopSound = 0;
		self->r.svFlags = 0;
	}
}

/*QUAKED misc_model_beacon (1 0 0) (-16 -16 0) (16 16 24) x x x
model="models/map_objects/wedge/beacon.md3"
An animating beacon model.

"forcevisible" - When you turn on force sight (any level), you can see these draw through the entire level...
*/
//------------------------------------------------------------
void SP_misc_model_beacon( gentity_t *ent )
{
	int forceVisible = 0;
	VectorSet( ent->r.mins, -16, -16, 0 );
	VectorSet( ent->r.maxs, 16, 16, 24 );

	ent->s.modelindex = G_ModelIndex("models/map_objects/wedge/beacon.md3");	// Precache model
	ent->s.modelindex2 = G_ModelIndex("models/map_objects/wedge/beacon.md3");	// Precache model
	ent->noise_index = G_SoundIndex("sound/interface/ammocon_run");

	ent->s.eFlags = 0;
	ent->r.svFlags |= SVF_PLAYER_USABLE;
	ent->r.contents = CONTENTS_SOLID;
	ent->clipmask = MASK_SOLID;

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	trap_LinkEntity (ent);

	ent->use = beacon_use;

	G_SpawnInt( "material", "4", (int *)&ent->material );

	ent->takedamage = qfalse;

	// If we have a targetname, we're are invisible until we are spawned in by being used.
	if ( ent->targetname )
	{
		ent->s.eFlags = EF_NODRAW;
		ent->r.contents = 0;
		ent->clipmask = 0;
		ent->s.loopSound = 0;
		ent->count = 0;
	}
	else
	{
		ent->count = 1;
		beacon_deploy( ent );
	}

	G_SpawnInt( "forcevisible", "0", &forceVisible );
	if ( forceVisible )
	{//can see these through walls with force sight, so must be broadcast
		ent->r.svFlags |= SVF_BROADCAST;
		ent->s.eFlags |= EF_FORCE_VISIBLE;
	}
}
//[/CoOp]
