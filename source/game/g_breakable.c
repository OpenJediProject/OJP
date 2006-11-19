//[CoOp]
//SP code for breakables

#include "g_local.h"

#define MDL_OTHER			0
#define MDL_ARMOR_HEALTH	1
#define MDL_AMMO			2

extern void G_MiscModelExplosion( vec3_t mins, vec3_t maxs, int size, material_t chunkType );
extern void G_Chunks( int owner, vec3_t origin, const vec3_t normal, const vec3_t mins, const vec3_t maxs, 
						float speed, int numChunks, material_t chunkType, int customChunk, float baseScale );
void misc_model_breakable_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath )
//void misc_model_breakable_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath,int dFlags,int hitLoc ) 
{
	int		numChunks;
	float	size = 0, scale;
	vec3_t	dir, up, dis;

	if (self->die == NULL)	//i was probably already killed since my die func was removed
	{
#ifndef FINAL_BUILD
		G_Printf(S_COLOR_YELLOW"Recursive misc_model_breakable_die.  Use targets probably pointing back at self.\n");
#endif
		return;	//this happens when you have a cyclic target chain!
	}
	//NOTE: Stop any scripts that are currently running (FLUSH)... ?
	//Turn off animation
	self->s.frame = self->startFrame = self->endFrame = 0;
	self->r.svFlags &= ~SVF_ANIMATING;
			
	self->health = 0;
	//Throw some chunks
	AngleVectors( self->s.apos.trBase, dir, NULL, NULL );
	VectorNormalize( dir );

	numChunks = random() * 6 + 20;

	VectorSubtract( self->r.absmax, self->r.absmin, dis );

	// This formula really has no logical basis other than the fact that it seemed to be the closest to yielding the results that I wanted.
	// Volume is length * width * height...then break that volume down based on how many chunks we have
	scale = sqrt( sqrt( dis[0] * dis[1] * dis[2] )) * 1.75f;

	if ( scale > 48 )
	{
		size = 2;
	}
	else if ( scale > 24 )
	{
		size = 1;
	}

	scale = scale / numChunks;

	if ( self->radius > 0.0f )
	{
		// designer wants to scale number of chunks, helpful because the above scale code is far from perfect
		//	I do this after the scale calculation because it seems that the chunk size generally seems to be very close, it's just the number of chunks is a bit weak
		numChunks *= self->radius;
	}

	VectorAdd( self->r.absmax, self->r.absmin, dis );
	VectorScale( dis, 0.5f, dis );

	//RAFIXME - impliment custom chunk models
	G_Chunks( self->s.number, dis, dir, self->r.absmin, self->r.absmax, 300, numChunks, self->material, 0, scale );
	//G_Chunks( self->s.number, dis, dir, self->r.absmin, self->r.absmax, 300, numChunks, self->material, self->s.modelindex3, scale );

	self->pain = NULL;
	self->die  = NULL;
//	self->e_UseFunc  = useF_NULL;

	self->takedamage = qfalse;

	if ( !(self->spawnflags & 4) )
	{//We don't want to stay solid
		self->s.solid = 0;
		self->r.contents = 0;
		self->clipmask = 0;
		/* RAFIXME - WTF?
		if (self!=0)
		{
			NAV::WayEdgesNowClear(self);
		}
		*/
		trap_LinkEntity(self);
	}

	VectorSet(up, 0, 0, 1);

	if(self->target)
	{
		G_UseTargets(self, attacker);
	}

	if(inflictor->client)
	{
		VectorSubtract( self->r.currentOrigin, inflictor->r.currentOrigin, dir );
		VectorNormalize( dir );
	}
	else
	{
		VectorCopy(up, dir);
	}

	if ( !(self->spawnflags & 2048) ) // NO_EXPLOSION
	{
		// Ok, we are allowed to explode, so do it now!
		if(self->splashDamage > 0 && self->splashRadius > 0)
		{//explode
			vec3_t org;
			AddSightEvent( attacker, self->r.currentOrigin, 256, AEL_DISCOVERED, 100 );
			//[CoOp]
			//Added sight/sound event based on SP code.
			AddSoundEvent( attacker, self->r.currentOrigin, 128, AEL_DISCOVERED, qfalse, qtrue );//FIXME: am I on ground or not?
			//AddSoundEvent( attacker, self->r.currentOrigin, 128, AEL_DISCOVERED, qfalse/*, qtrue*/ );//FIXME: am I on ground or not?
			//[/CoOp]
			//FIXME: specify type of explosion?  (barrel, electrical, etc.)  Also, maybe just use the explosion effect below since it's
			//				a bit better?
			// up the origin a little for the damage check, because several models have their origin on the ground, so they don't alwasy do damage, not the optimal solution...
			VectorCopy( self->r.currentOrigin, org );
			if ( self->r.mins[2] > -4 )
			{//origin is going to be below it or very very low in the model
				//center the origin
				org[2] = self->r.currentOrigin[2] + self->r.mins[2] + (self->r.maxs[2] - self->r.mins[2])/2.0f;
			}
			G_RadiusDamage( org, self, self->splashDamage, self->splashRadius, self, self, MOD_UNKNOWN );

			if ( self->model && ( Q_stricmp( "models/map_objects/ships/tie_fighter.md3", self->model ) == 0 ||
								  Q_stricmp( "models/map_objects/ships/tie_bomber.md3", self->model ) == 0 ) )
			{//TEMP HACK for Tie Fighters- they're HUGE
				G_PlayEffect( G_EffectIndex("explosions/fighter_explosion2"), self->r.currentOrigin, self->r.currentAngles );
				G_Sound( self, CHAN_AUTO, G_SoundIndex( "sound/weapons/tie_fighter/TIEexplode.wav" ) );
				self->s.loopSound = 0;
			}
			else
			{
				G_MiscModelExplosion( self->r.absmin, self->r.absmax, size, self->material );
				G_Sound( self, CHAN_AUTO, G_SoundIndex("sound/weapons/explosions/cargoexplode.wav") );
				self->s.loopSound = 0;
			}
		}
		else
		{//just break
			AddSightEvent( attacker, self->r.currentOrigin, 128, AEL_DISCOVERED, 0 );
			//[CoOp]
			//Added sight/sound event based on SP code.
			AddSoundEvent( attacker, self->r.currentOrigin, 64, AEL_SUSPICIOUS, qfalse, qtrue );//FIXME: am I on ground or not?
			//AddSoundEvent( attacker, self->r.currentOrigin, 64, AEL_SUSPICIOUS, qfalse/*, qtrue*/ );//FIXME: am I on ground or not?
			//[/CoOp]
			// This is the default explosion
			G_MiscModelExplosion( self->r.absmin, self->r.absmax, size, self->material );
			G_Sound(self, CHAN_AUTO, G_SoundIndex("sound/weapons/explosions/cargoexplode.wav"));
		}
	}

	self->think = NULL;
	self->nextthink = -1;

	if(self->s.modelindex2 != -1 && !(self->spawnflags & 8))
	{//FIXME: modelindex doesn't get set to -1 if the damage model doesn't exist
		//RAFIXME - impliment this flag?
		//self->svFlags |= SVF_BROKEN;
		self->s.modelindex = self->s.modelindex2;
		G_ActivateBehavior( self, BSET_DEATH );
	}
	else
	{
		G_FreeEntity( self );
	}
}


void misc_model_breakable_touch(gentity_t *self, gentity_t *other, trace_t *trace)
{//touch function for model breakable.  doesn't actually do anything, but we need one to prevent crashs like the one on taspir2
}


void misc_model_throw_at_target4( gentity_t *self, gentity_t *activator )
{
	vec3_t	pushDir, kvel;
	float	knockback = 200;
	float	mass = self->mass;
	gentity_t *target = G_Find( NULL, FOFS(targetname), self->target4 );
	if ( !target )
	{//nothing to throw ourselves at...
		return;
	}
	VectorSubtract( target->r.currentOrigin, self->r.currentOrigin, pushDir );
	knockback -= VectorNormalize( pushDir );
	if ( knockback < 100 )
	{
		knockback = 100;
	}
	VectorCopy( self->r.currentOrigin, self->s.pos.trBase );
	self->s.pos.trTime = level.time;								// move a bit on the very first frame
	if ( self->s.pos.trType != TR_INTERPOLATE )
	{//don't do this to rolling missiles
		self->s.pos.trType = TR_GRAVITY;
	}

	if ( mass < 50 )
	{//???
		mass = 50;
	}

	if ( g_gravity.value > 0 )
	{
		VectorScale( pushDir, g_knockback.value * knockback / mass * 0.8, kvel );
		kvel[2] = pushDir[2] * g_knockback.value * knockback / mass * 1.5;
	}
	else
	{
		VectorScale( pushDir, g_knockback.value * knockback / mass, kvel );
	}

	VectorAdd( self->s.pos.trDelta, kvel, self->s.pos.trDelta );
	if ( g_gravity.value > 0 )
	{
		if ( self->s.pos.trDelta[2] < knockback )
		{
			self->s.pos.trDelta[2] = knockback;
		}
	}
	//no trDuration?
	if ( self->think != G_RunObject )
	{//objects spin themselves?
		//spin it
		//FIXME: messing with roll ruins the rotational center???
		self->s.apos.trTime = level.time;
		self->s.apos.trType = TR_LINEAR;
		VectorClear( self->s.apos.trDelta );
		self->s.apos.trDelta[1] = Q_irand( -800, 800 );
	}

	//RAFIXME - impliment this stuff
	/*
	self->forcePushTime = level.time + 600; // let the push effect last for 600 ms
	if ( activator )
	{
		
		self->forcePuller = activator->s.number;//remember this regardless
	}
	else
	{
		self->forcePuller = NULL;
	}
	*/
}


void misc_model_use (gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if ( self->target4 )
	{//throw me at my target!
		misc_model_throw_at_target4( self, activator );
		return;
	}

	if ( self->health <= 0 && self->maxHealth > 0)
	{//used while broken fired target3
		G_UseTargets2( self, activator, self->target3 );
		return;
	}

	// Become solid again.
	if ( !self->count )
	{
		self->count = 1;
		self->activator = activator;
		self->r.svFlags &= ~SVF_NOCLIENT;
		self->s.eFlags &= ~EF_NODRAW;
	}

	G_ActivateBehavior( self, BSET_USE );
	//Don't explode if they've requested it to not
	if ( self->spawnflags & 64 )
	{//Usemodels toggling
		if ( self->spawnflags & 32 )
		{
			if( self->s.modelindex == self->sound1to2 )
			{
				self->s.modelindex = self->sound2to1;
			}
			else
			{
				self->s.modelindex = self->sound1to2;
			}
		}

		return;
	}

	self->die  = misc_model_breakable_die;
	misc_model_breakable_die( self, other, activator, self->health, MOD_UNKNOWN );
}


//pain function for model_breakables
void misc_model_breakable_pain (gentity_t *self, gentity_t *attacker, int damage)
{
	if ( self->health > 0 )
	{
		// still alive, react to the pain
		if ( self->paintarget )
		{
			G_UseTargets2 (self, self->activator, self->paintarget);
		}

		// Don't do script if dead
		G_ActivateBehavior( self, BSET_PAIN );
	}
}


void health_shutdown( gentity_t *self )
{
	if (!(self->s.eFlags & EF_ANIM_ONCE))
	{
		self->s.eFlags &= ~ EF_ANIM_ALLFAST;
		self->s.eFlags |= EF_ANIM_ONCE;

		// Switch to and animate its used up model.
		if (!Q_stricmp(self->model,"models/mapobjects/stasis/plugin2.md3"))	
		{
			self->s.modelindex = self->s.modelindex2;
		}
		else if (!Q_stricmp(self->model,"models/mapobjects/borg/plugin2.md3"))	
		{
			self->s.modelindex = self->s.modelindex2;
		}                                
		else if (!Q_stricmp(self->model,"models/mapobjects/stasis/plugin2_floor.md3"))	
		{
			self->s.modelindex = self->s.modelindex2;
//			G_Sound(self, G_SoundIndex("sound/ambience/stasis/shrinkage1.wav") );
		}
		else if (!Q_stricmp(self->model,"models/mapobjects/forge/panels.md3"))
		{
			self->s.modelindex = self->s.modelindex2;
		}

		trap_LinkEntity (self);
	}
}


//add amount number of health to given player
qboolean ITM_AddHealth( gentity_t *ent, int amount)
{
	if(ent->health == ent->client->ps.stats[STAT_MAX_HEALTH])
	{//maxed out health as is
		return qfalse;
	}

	ent->health += amount;
	if(ent->health > ent->client->ps.stats[STAT_MAX_HEALTH])
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];

	ent->client->ps.stats[STAT_HEALTH] = ent->health;
	return qtrue;
}


qboolean ITM_AddArmor( gentity_t *ent, int amount)
{
	if(ent->client->ps.stats[STAT_ARMOR] == ent->client->ps.stats[STAT_MAX_HEALTH])
	{//maxed out health as is
		return qfalse;
	}

	ent->client->ps.stats[STAT_ARMOR] += amount;
	if(ent->client->ps.stats[STAT_ARMOR] > ent->client->ps.stats[STAT_MAX_HEALTH])
		ent->client->ps.stats[STAT_ARMOR] = ent->client->ps.stats[STAT_MAX_HEALTH];

	return qtrue;
}


void health_use( gentity_t *self, gentity_t *other, gentity_t *activator);
void health_think( gentity_t *ent )
{
	int dif;

	// He's dead, Jim. Don't give him health
	if (ent->enemy->health<1)
	{
		ent->count = 0;
		ent->think = NULL;
	}

	// Still has power to give
	if (ent->count > 0)
	{
		// For every 3 points of health, you get 1 point of armor
		// BUT!!! after health is filled up, you get the full energy going to armor

		dif = ent->enemy->client->ps.stats[STAT_MAX_HEALTH] - ent->enemy->health;

		if (dif > 3 )
		{
			dif= 3;
		}
		else if (dif < 0) 
		{
			dif= 0;	
		}

		if (dif > ent->count)	// Can't give more than count
		{
			dif = ent->count;
		}

		if ((ITM_AddHealth (ent->enemy,dif)) && (dif>0))		
		{
			ITM_AddArmor (ent->enemy,1);	// 1 armor for every 3 health

			ent->count-=dif;
			ent->nextthink = level.time + 10;
		}
		else	// User has taken all health he can hold, see about giving it all to armor
		{
			dif = ent->enemy->client->ps.stats[STAT_MAX_HEALTH] - 
				ent->enemy->client->ps.stats[STAT_ARMOR];

			if (dif > 3)
			{
				dif = 3;
			}
			else if (dif < 0) 
			{
				dif= 0;	
			}

			if (ent->count < dif)	// Can't give more than count
			{
				dif = ent->count;
			}

			if ((!ITM_AddArmor(ent->enemy,dif)) || (dif<=0))
			{
				ent->use = health_use;	
				ent->think = NULL;
			}
			else
			{
				ent->count-=dif;
				ent->nextthink = level.time + 10;
			}
		}
	}

	if (ent->count < 1)
	{
		health_shutdown(ent);
	}
}


void health_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{//FIXME: Heal entire team?  Or only those that are undying...?
	int dif;
	int dif2;
	int hold;

	G_ActivateBehavior(self,BSET_USE);

	if (self->think != NULL)
	{
		self->think = NULL;
	}
	else
	{

		if (other->client)
		{
			// He's dead, Jim. Don't give him health
			if (other->client->ps.stats[STAT_HEALTH]<1)
			{
				dif = 1;
				self->count = 0;
			}
			else 
			{	// Health
				dif = other->client->ps.stats[STAT_MAX_HEALTH] - other->client->ps.stats[STAT_HEALTH];
				// Armor
				dif2 = other->client->ps.stats[STAT_MAX_HEALTH] - other->client->ps.stats[STAT_ARMOR];
				hold = (dif2 - dif);
				// For every 3 points of health, you get 1 point of armor
				// BUT!!! after health is filled up, you get the full energy going to armor
				if (hold>0)	// Need more armor than health
				{
					// Calculate total amount of station energy needed.
					
					hold = dif / 3;	//	For every 3 points of health, you get 1 point of armor
					dif2 -= hold;
					dif2 += dif;	
					
					dif = dif2;
				}
			}
		}
		else
		{	// Being triggered to be used up
			dif = 1;
			self->count = 0;
		}
		
		// Does player already have full health and full armor?
		if (dif > 0)
		{
//			G_Sound(self, G_SoundIndex("sound/player/suithealth.wav") );

			if ((dif >= self->count) || (self->count<1)) // use it all up?
			{
				health_shutdown(self);
			}
			// Use target when used
			if (self->spawnflags & 8)
			{
				G_UseTargets( self, activator );	
			}

			self->use = NULL;	
			self->enemy = other;
			self->think = health_think;
			self->nextthink = level.time + 50;
		}
		else
		{
//			G_Sound(self, G_SoundIndex("sound/weapons/noammo.wav") );
		}
	}	
}


void ammo_shutdown( gentity_t *self )
{
	if (!(self->s.eFlags & EF_ANIM_ONCE))
	{
		self->s.eFlags &= ~ EF_ANIM_ALLFAST;
		self->s.eFlags |= EF_ANIM_ONCE;

		trap_LinkEntity (self);
	}
}


qboolean Add_Ammo2(gentity_t *ent, int ammotype, int amount)
{
	if( ent->client->ps.ammo[ammotype] == ammoData[ammotype].max )
	{//ammo maxed
		return qfalse;
	}

	ent->client->ps.ammo[ammotype] += amount;

	if(ent->client->ps.ammo[ammotype] > ammoData[ammotype].max)
		ent->client->ps.ammo[ammotype] = ammoData[ammotype].max;

	return qtrue;
}


void ammo_use( gentity_t *self, gentity_t *other, gentity_t *activator);
void ammo_think( gentity_t *ent )
{
	int dif;

	// Still has ammo to give
	if (ent->count > 0 && ent->enemy )
	{
		dif = ammoData[AMMO_BLASTER].max  - ent->enemy->client->ps.ammo[AMMO_BLASTER];

		if (dif > 2 )
		{
			dif= 2;
		}
		else if (dif < 0) 
		{
			dif= 0;	
		}

		if (ent->count < dif)	// Can't give more than count
		{
			dif = ent->count;
		}

		// Give player ammo 
		if (Add_Ammo2(ent->enemy,AMMO_BLASTER,dif) && (dif!=0))	
		{
			ent->count-=dif;
			ent->nextthink = level.time + 10;
		}
		else	// User has taken all ammo he can hold
		{
			ent->use = ammo_use;	
			ent->think = NULL;
		}
	}

	if (ent->count < 1)
	{
		ammo_shutdown(ent);
	}
}


void ammo_use( gentity_t *self, gentity_t *other, gentity_t *activator)
{
	int dif;

	G_ActivateBehavior(self,BSET_USE);

	if (self->think != NULL)
	{
		if (self->use != NULL)
		{
			self->think = NULL;
		}
	}
	else
	{
		if (other->client)
		{
			dif = ammoData[AMMO_BLASTER].max - other->client->ps.ammo[AMMO_BLASTER];
		}
		else
		{	// Being triggered to be used up
			dif = 1;
			self->count = 0;
		}

		// Does player already have full ammo?
		if (dif > 0)
		{
//			G_Sound(self, G_SoundIndex("sound/player/suitenergy.wav") );

			if ((dif >= self->count) || (self->count<1)) // use it all up?
			{
				ammo_shutdown(self);
			}
		}	
		else
		{
//			G_Sound(self, G_SoundIndex("sound/weapons/noammo.wav") );
		}
		// Use target when used
		if (self->spawnflags & 8)
		{
			G_UseTargets( self, activator );	
		}

		self->use = NULL;	
		G_SetEnemy( self, other );
		self->think = ammo_think;
		self->nextthink = level.time + 50;
	}	
}


//initization for misc_model_breakable
void misc_model_breakable_init( gentity_t *ent )
{
	int		type;

	type = MDL_OTHER;

	if (!ent->model) {
		G_Error("no model set on %s at (%.1f %.1f %.1f)\n", ent->classname, ent->s.origin[0],ent->s.origin[1],ent->s.origin[2]);
	}
	//Main model
	ent->s.modelindex = ent->sound2to1 = G_ModelIndex( ent->model );

	if ( ent->spawnflags & 1 )
	{//Blocks movement
		ent->r.contents = CONTENTS_SOLID|CONTENTS_OPAQUE|CONTENTS_BODY|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP;//Was CONTENTS_SOLID, but only architecture should be this
		ent->s.solid = 2; //SOLID_BBOX
		ent->clipmask = MASK_PLAYERSOLID;
	}
	else if ( ent->health )
	{//Can only be shot
		ent->r.contents = CONTENTS_SHOTCLIP;
	}

	if (type == MDL_OTHER)
	{
		ent->use = misc_model_use;	
	}
	else if ( type == MDL_ARMOR_HEALTH )
	{
//		G_SoundIndex("sound/player/suithealth.wav");
		ent->use = health_use;
		if (!ent->count)
		{
			ent->count = 100;
		}
		ent->health = 60;
	}
	else if ( type == MDL_AMMO )
	{
//		G_SoundIndex("sound/player/suitenergy.wav");
		//RAFIXME: add this use function
		ent->use = ammo_use;
		if (!ent->count)
		{
			ent->count = 100;
		}
		ent->health = 60;
	}

	if ( ent->health ) 
	{
		G_SoundIndex("sound/weapons/explosions/cargoexplode.wav");
		ent->maxHealth = ent->health;
		G_ScaleNetHealth(ent);
		ent->takedamage = qtrue;
		ent->pain = misc_model_breakable_pain;

		//RACC - I think should be ->die
		ent->die  = misc_model_breakable_die;
		//ent->think  = misc_model_breakable_die;

	}

	ent->touch = misc_model_breakable_touch;
}


void misc_model_breakable_gravity_init( gentity_t *ent, qboolean dropToFloor )
{
	trace_t		tr;
	vec3_t		top, bottom;

	//G_SoundIndex( "sound/movers/objects/objectHurt.wav" );
	G_EffectIndex( "melee/kick_impact" );
	G_EffectIndex( "melee/kick_impact_silent" );
	//G_SoundIndex( "sound/weapons/melee/punch1.mp3" );
	//G_SoundIndex( "sound/weapons/melee/punch2.mp3" );
	//G_SoundIndex( "sound/weapons/melee/punch3.mp3" );
	//G_SoundIndex( "sound/weapons/melee/punch4.mp3" );
	G_SoundIndex( "sound/movers/objects/objectHit.wav" );
	G_SoundIndex( "sound/movers/objects/objectHitHeavy.wav" );
	G_SoundIndex( "sound/movers/objects/objectBreak.wav" );
	//FIXME: dust impact effect when hits ground?
	ent->s.eType = ET_GENERAL;
	//RAFIXME:  figure out the MP equivilent for this guy.
	//ent->s.eFlags |= EF_BOUNCE_HALF;
	ent->clipmask = MASK_SOLID|CONTENTS_BODY|CONTENTS_MONSTERCLIP|CONTENTS_BOTCLIP;//?
	if ( !ent->mass )
	{//not overridden by designer
		ent->mass = VectorLength( ent->r.maxs ) + VectorLength( ent->r.mins );
	}
	ent->physicsBounce = ent->mass;
	//drop to floor
	if ( dropToFloor )
	{
		VectorCopy( ent->r.currentOrigin, top );
		top[2] += 1;
		VectorCopy( ent->r.currentOrigin, bottom );
		bottom[2] = MIN_WORLD_COORD;
		trap_Trace( &tr, top, ent->r.mins, ent->r.maxs, bottom, ent->s.number, MASK_NPCSOLID );
		if ( !tr.allsolid && !tr.startsolid && tr.fraction < 1.0 )
		{
			G_SetOrigin( ent, tr.endpos );
			trap_LinkEntity( ent );
		}
	}
	else
	{
		G_SetOrigin( ent, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}
	//set up for object thinking
	if ( VectorCompare( ent->s.pos.trDelta, vec3_origin ) )
	{//not moving
		ent->s.pos.trType = TR_STATIONARY;
	}
	else
	{
		ent->s.pos.trType = TR_GRAVITY;
	}
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	VectorClear( ent->s.pos.trDelta );
	ent->s.pos.trTime = level.time;
	if ( VectorCompare( ent->s.apos.trDelta, vec3_origin ) )
	{//not moving
		ent->s.apos.trType = TR_STATIONARY;
	}
	else
	{
		ent->s.apos.trType = TR_LINEAR;
	}
	VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );
	VectorClear( ent->s.apos.trDelta );
	ent->s.apos.trTime = level.time;
	ent->nextthink = level.time + FRAMETIME;
	ent->think = G_RunObject;
}


extern void CacheChunkEffects( material_t material );
extern stringID_table_t TeamTable[];
/*QUAKED misc_model_breakable (1 0 0) (-16 -16 -16) (16 16 16) SOLID AUTOANIMATE DEADSOLID NO_DMODEL NO_SMOKE USE_MODEL USE_NOT_BREAK PLAYER_USE NO_EXPLOSION START_OFF
SOLID - Movement is blocked by it, if not set, can still be broken by explosions and shots if it has health
AUTOANIMATE - Will cycle it's anim
DEADSOLID - Stay solid even when destroyed (in case damage model is rather large).
NO_DMODEL - Makes it NOT display a damage model when destroyed, even if one exists
USE_MODEL - When used, will toggle to it's usemodel (model + "_u1.md3")... this obviously does nothing if USE_NOT_BREAK is not checked
USE_NOT_BREAK - Using it, doesn't make it break, still can be destroyed by damage
PLAYER_USE - Player can use it with the use button
NO_EXPLOSION - By default, will explode when it dies...this is your override.
START_OFF - Will start off and will not appear until used.

"model"		arbitrary .md3 file to display
"modelscale"	"x" uniform scale
"modelscale_vec" "x y z" scale model in each axis - height, width and length - bbox will scale with it
"health"	how much health to have - default is zero (not breakable)  If you don't set the SOLID flag, but give it health, it can be shot but will not block NPCs or players from moving
"targetname" when used, dies and displays damagemodel (model + "_d1.md3"), if any (if not, removes itself)
"target" What to use when it dies
"target2" What to use when it's repaired
"target3" What to use when it's used while it's broken
"paintarget" target to fire when hit (but not destroyed)
"count"  the amount of armor/health/ammo given (default 50)
"radius"  Chunk code tries to pick a good volume of chunks, but you can alter this to scale the number of spawned chunks. (default 1)  (.5) is half as many chunks, (2) is twice as many chunks
"NPC_targetname" - Only the NPC with this name can damage this
"forcevisible" - When you turn on force sight (any level), you can see these draw through the entire level...
"redCrosshair" - crosshair turns red when you look at this

"gravity"	if set to 1, this will be affected by gravity
"throwtarget" if set (along with gravity), this thing, when used, will throw itself at the entity whose targetname matches this string
"mass"		if gravity is on, this determines how much damage this thing does when it hits someone.  Default is the size of the object from one corner to the other, that works very well.  Only override if this is an object whose mass should be very high or low for it's size (density)

Damage: default is none
"splashDamage" - damage to do (will make it explode on death)
"splashRadius" - radius for above damage

"team" - This cannot take damage from members of this team:
	"player"
	"neutral"
	"enemy"

"material" - default is "8 - MAT_NONE" - choose from this list:
0 = MAT_METAL		(grey metal)
1 = MAT_GLASS		
2 = MAT_ELECTRICAL	(sparks only)
3 = MAT_ELEC_METAL	(METAL chunks and sparks)
4 =	MAT_DRK_STONE	(brown stone chunks)
5 =	MAT_LT_STONE	(tan stone chunks)
6 =	MAT_GLASS_METAL (glass and METAL chunks)
7 = MAT_METAL2		(blue/grey metal)
8 = MAT_NONE		(no chunks-DEFAULT)
9 = MAT_GREY_STONE	(grey colored stone)
10 = MAT_METAL3		(METAL and METAL2 chunk combo)
11 = MAT_CRATE1		(yellow multi-colored crate chunks)
12 = MAT_GRATE1		(grate chunks--looks horrible right now)
13 = MAT_ROPE		(for yavin_trial, no chunks, just wispy bits )
14 = MAT_CRATE2		(red multi-colored crate chunks)
15 = MAT_WHITE_METAL (white angular chunks for Stu, NS_hideout )
*/
void SP_misc_model_breakable( gentity_t *ent ) 
{
	char	damageModel[MAX_QPATH];
	char	chunkModel[MAX_QPATH];
	char	useModel[MAX_QPATH];
	int		len;
	qboolean bHasScale;

	float grav = 0;
	//[CoOp]
	int forceVisible = 0;
	//[/CoOp]
	int redCrosshair = 0;
	
	// Chris F. requested default for misc_model_breakable to be NONE...so don't arbitrarily change this.
	G_SpawnInt( "material", "8", (int*)&ent->material );
	G_SpawnFloat( "radius", "1", &ent->radius ); // used to scale chunk code if desired by a designer
	bHasScale = G_SpawnVector("modelscale_vec", "0 0 0", ent->modelScale);
	if (!bHasScale)
	{
		float temp;
		G_SpawnFloat( "modelscale", "0", &temp);
		if (temp != 0.0f)
		{
			ent->modelScale[ 0 ] = ent->modelScale[ 1 ] = ent->modelScale[ 2 ] = temp;
			bHasScale = qtrue;
		}
	}

	CacheChunkEffects( ent->material );
	misc_model_breakable_init( ent );

	len = strlen( ent->model ) - 4;
	assert(ent->model[len]=='.');//we're expecting ".md3"
	strncpy( damageModel, ent->model, sizeof(damageModel) );
	damageModel[len] = 0;	//chop extension
	strncpy( chunkModel, damageModel, sizeof(chunkModel));
	strncpy( useModel, damageModel, sizeof(useModel));
	
	if (ent->takedamage) {
		//Dead/damaged model
		if( !(ent->spawnflags & 8) ) {	//no dmodel
			strcat( damageModel, "_d1.md3" );
			ent->s.modelindex2 = G_ModelIndex( damageModel );
		}
		
		/* RAFIXME - add modelindex3
		//Chunk model
		strcat( chunkModel, "_c1.md3" );
		ent->s.modelindex3 = G_ModelIndex( chunkModel );
		*/
	}

	//Use model
	if( ent->spawnflags & 32 ) {	//has umodel
		strcat( useModel, "_u1.md3" );
		ent->sound1to2 = G_ModelIndex( useModel );
	}

	G_SpawnVector("mins", "-16 -16 -16", ent->r.mins);
	G_SpawnVector("maxs", "16 16 16", ent->r.maxs);
	/*  This code wasn't working right
	if ( !ent->r.mins[0] && !ent->r.mins[1] && !ent->r.mins[2] )
	{
		VectorSet (ent->r.mins, -16, -16, -16);
	}
	if ( !ent->r.maxs[0] && !ent->r.maxs[1] && !ent->r.maxs[2] )
	{
		VectorSet (ent->r.maxs, 16, 16, 16);
	}
	*/

	// Scale up the tie-bomber bbox a little.
	if ( ent->model && Q_stricmp( "models/map_objects/ships/tie_bomber.md3", ent->model ) == 0 )
	{
		VectorSet (ent->r.mins, -80, -80, -80);
		VectorSet (ent->r.maxs, 80, 80, 80); 

		//ent->s.modelScale[ 0 ] = ent->s.modelScale[ 1 ] = ent->s.modelScale[ 2 ] *= 2.0f;
		//bHasScale = qtrue;
	}

	if (bHasScale)
	{
		float oldMins2;

		//scale the x axis of the bbox up.
		ent->r.maxs[0] *= ent->modelScale[0];//*scaleFactor;
		ent->r.mins[0] *= ent->modelScale[0];//*scaleFactor;
		
		//scale the y axis of the bbox up.
		ent->r.maxs[1] *= ent->modelScale[1];//*scaleFactor;
		ent->r.mins[1] *= ent->modelScale[1];//*scaleFactor;
		
		//scale the z axis of the bbox up and adjust origin accordingly
		ent->r.maxs[2] *= ent->modelScale[2];
		oldMins2 = ent->r.mins[2];
		ent->r.mins[2] *= ent->modelScale[2];
		ent->s.origin[2] += (oldMins2-ent->r.mins[2]);
	}

	if ( ent->spawnflags & 2 )
	{
		ent->s.eFlags |= EF_ANIM_ALLFAST;
	}

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngles( ent, ent->s.angles );
	trap_LinkEntity(ent);

	if ( ent->spawnflags & 128 )
	{//Can be used by the player's BUTTON_USE
		ent->r.svFlags |= SVF_PLAYER_USABLE;
	}

	if ( ent->team && ent->team[0] )
	{
		ent->teamnodmg= (team_t)GetIDForString( TeamTable, ent->team );
		if ( ent->teamnodmg == TEAM_FREE )
		{
			G_Error("team name %s not recognized\n", ent->team);
		}
	}
	
	ent->team = NULL;

	//HACK
	if ( ent->model && Q_stricmp( "models/map_objects/ships/x_wing_nogear.md3", ent->model ) == 0 )
	{
		if( ent->splashDamage > 0 && ent->splashRadius > 0 )
		{
			ent->s.loopSound = G_SoundIndex( "sound/vehicles/x-wing/loop.wav" );
			/* RAFIXME - impliment this flag?
			ent->s.eFlags |= EF_LESS_ATTEN;
			*/
		}
	}
	else if ( ent->model && Q_stricmp( "models/map_objects/ships/tie_fighter.md3", ent->model ) == 0 )
	{//run a think
		G_EffectIndex( "explosions/fighter_explosion2" );
		G_SoundIndex( "sound/weapons/tie_fighter/tiepass1.wav" );
/*		G_SoundIndex( "sound/weapons/tie_fighter/tiepass2.wav" );
		G_SoundIndex( "sound/weapons/tie_fighter/tiepass3.wav" );
		G_SoundIndex( "sound/weapons/tie_fighter/tiepass4.wav" );
		G_SoundIndex( "sound/weapons/tie_fighter/tiepass5.wav" );*/
		G_SoundIndex( "sound/weapons/tie_fighter/tie_fire.wav" );
/*		G_SoundIndex( "sound/weapons/tie_fighter/tie_fire2.wav" );
		G_SoundIndex( "sound/weapons/tie_fighter/tie_fire3.wav" );*/
		G_SoundIndex( "sound/weapons/tie_fighter/TIEexplode.wav" );

		//RAFIXME - add Tie Fighter weapon?
		//RegisterItem( BG_FindItemForWeapon( WP_TIE_FIGHTER ));
		/* RAFIXME - impliment this flag?
		ent->s.eFlags |= EF_LESS_ATTEN;
		*/

		if( ent->splashDamage > 0 && ent->splashRadius > 0 )
		{
			// Yeah, I could have just made this value changable from the editor, but I
			// need it immediately!
			float		light;
			vec3_t		color;
			qboolean	lightSet, colorSet;

			ent->s.loopSound = G_SoundIndex( "sound/vehicles/tie-bomber/loop.wav" );
			//ent->e_ThinkFunc = thinkF_TieFighterThink;
			//ent->e_UseFunc = thinkF_TieFighterThink;
			//ent->nextthink = level.time + FRAMETIME;
			//RAFIXME: create this use
			//ent->use = useF_TieFighterUse;

			// if the "color" or "light" keys are set, setup constantLight
			lightSet = qtrue;//G_SpawnFloat( "light", "100", &light );
			light = 255;
			//colorSet = "1 1 1"//G_SpawnVector( "color", "1 1 1", color );
			colorSet = qtrue;
			color[0] = 1;	color[1] = 1;	color[2] = 1;
			if ( lightSet || colorSet ) 
			{
				int		r, g, b, i;

				r = color[0] * 255;
				if ( r > 255 ) {
					r = 255;
				}
				g = color[1] * 255;
				if ( g > 255 ) {
					g = 255;
				}
				b = color[2] * 255;
				if ( b > 255 ) {
					b = 255;
				}
				i = light / 4;
				if ( i > 255 ) {
					i = 255;
				}
				ent->s.constantLight = r | ( g << 8 ) | ( b << 16 ) | ( i << 24 );
			}
		}
	}
	else if ( ent->model && Q_stricmp( "models/map_objects/ships/tie_bomber.md3", ent->model ) == 0 )
	{
		G_EffectIndex( "ships/tiebomber_bomb_falling" );
		G_EffectIndex( "ships/tiebomber_explosion2" );
		G_EffectIndex( "explosions/fighter_explosion2" );
		G_SoundIndex( "sound/weapons/tie_fighter/TIEexplode.wav" );
		//RAFIXME: create this function
		//ent->e_ThinkFunc = thinkF_TieBomberThink;
		ent->nextthink = level.time + FRAMETIME;
		ent->attackDebounceTime = level.time + 1000;
		// We only take damage from a heavy weapon class missiles.
		ent->flags |= FL_DMG_BY_HEAVY_WEAP_ONLY;
		ent->s.loopSound = G_SoundIndex( "sound/vehicles/tie-bomber/loop.wav" );
		//RAFIXME: create this
		//ent->s.eFlags |= EF_LESS_ATTEN;
	}

	
	G_SpawnFloat( "gravity", "0", &grav );
	if ( grav )
	{//affected by gravity
		G_SetAngles( ent, ent->s.angles );
		G_SetOrigin( ent, ent->r.currentOrigin );
		G_SpawnString( "throwtarget", NULL, &ent->target4 ); // used to throw itself at something
		misc_model_breakable_gravity_init( ent, qtrue );
	}

	// Start off.
	if ( ent->spawnflags & 4096 )
	{
		//RAFIXME - need to fix this guy somehow
		//ent->spawnContents = ent->contents;	// It Navs can temporarly turn it "on"

		ent->s.solid = 0;
		ent->r.contents = 0;
		ent->clipmask = 0;
		ent->r.svFlags |= SVF_NOCLIENT;
		ent->s.eFlags |= EF_NODRAW;
		ent->count = 0;
	}

	G_SpawnInt( "forcevisible", "0", &forceVisible );
	if ( forceVisible )
	{//can see these through walls with force sight, so must be broadcast
		//[CoOp]
		ent->r.svFlags |= SVF_BROADCAST;
		//RAFIXME - impliment this flag
		ent->s.eFlags |= EF_FORCE_VISIBLE;
		//[/CoOp]
	}

	G_SpawnInt( "redCrosshair", "0", &redCrosshair );
	if ( redCrosshair )
	{//can see these through walls with force sight, so must be broadcast
		//RAFIXME - impliment this
		//ent->flags |= FL_RED_CROSSHAIR;
	}
}
//[/CoOp]

