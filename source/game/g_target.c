// Copyright (C) 1999-2000 Id Software, Inc.
//
#include "g_local.h"

//==========================================================

/*QUAKED target_give (1 0 0) (-8 -8 -8) (8 8 8)
Gives the activator all the items pointed to.
*/
void Use_Target_Give( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t	*t;
	trace_t		trace;

	if ( !activator->client ) {
		return;
	}

	if ( !ent->target ) {
		return;
	}

	memset( &trace, 0, sizeof( trace ) );
	t = NULL;
	while ( (t = G_Find (t, FOFS(targetname), ent->target)) != NULL ) {
		if ( !t->item ) {
			continue;
		}
		Touch_Item( t, activator, &trace );

		// make sure it isn't going to respawn or show any events
		t->nextthink = 0;
		trap_UnlinkEntity( t );
	}
}

void SP_target_give( gentity_t *ent ) {
	ent->use = Use_Target_Give;
}


//==========================================================

/*QUAKED target_remove_powerups (1 0 0) (-8 -8 -8) (8 8 8)
takes away all the activators powerups.
Used to drop flight powerups into death puts.
*/
void Use_target_remove_powerups( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if( !activator->client ) {
		return;
	}

	if( activator->client->ps.powerups[PW_REDFLAG] ) {
		Team_ReturnFlag( TEAM_RED );
	} else if( activator->client->ps.powerups[PW_BLUEFLAG] ) {
		Team_ReturnFlag( TEAM_BLUE );
	} else if( activator->client->ps.powerups[PW_NEUTRALFLAG] ) {
		Team_ReturnFlag( TEAM_FREE );
	}

	memset( activator->client->ps.powerups, 0, sizeof( activator->client->ps.powerups ) );
}

void SP_target_remove_powerups( gentity_t *ent ) {
	ent->use = Use_target_remove_powerups;
}


//==========================================================

/*QUAKED target_delay (1 0 0) (-8 -8 -8) (8 8 8) NO_RETRIGGER

NO_RETRIGGER - Keeps the delay from resetting the time if it is
activated again while it is counting down to an event.

"wait" seconds to pause before firing targets.
"random" delay variance, total delay = delay +/- random seconds
*/
void Think_Target_Delay( gentity_t *ent ) {
	G_UseTargets( ent, ent->activator );
}

void Use_Target_Delay( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if (ent->nextthink > level.time && (ent->spawnflags & 1))
	{ //Leave me alone, I am thinking.
		return;
	}
	G_ActivateBehavior(ent,BSET_USE);
	ent->nextthink = level.time + ( ent->wait + ent->random * crandom() ) * 1000;
	ent->think = Think_Target_Delay;
	ent->activator = activator;
}

void SP_target_delay( gentity_t *ent ) {
	// check delay for backwards compatability
	if ( !G_SpawnFloat( "delay", "0", &ent->wait ) ) {
		G_SpawnFloat( "wait", "1", &ent->wait );
	}

	if ( !ent->wait ) {
		ent->wait = 1;
	}
	ent->use = Use_Target_Delay;
}


//==========================================================

/*QUAKED target_score (1 0 0) (-8 -8 -8) (8 8 8)
"count" number of points to add, default 1

The activator is given this many points.
*/
void Use_Target_Score (gentity_t *ent, gentity_t *other, gentity_t *activator) {
	AddScore( activator, ent->r.currentOrigin, ent->count );
}

void SP_target_score( gentity_t *ent ) {
	if ( !ent->count ) {
		ent->count = 1;
	}
	ent->use = Use_Target_Score;
}


//==========================================================

/*QUAKED target_print (1 0 0) (-8 -8 -8) (8 8 8) redteam blueteam private
"message"	text to print
"wait"		don't fire off again if triggered within this many milliseconds ago
If "private", only the activator gets the message.  If no checks, all clients get the message.
*/
void Use_Target_Print (gentity_t *ent, gentity_t *other, gentity_t *activator)
{
	if (!ent || !ent->inuse)
	{
		Com_Printf("ERROR: Bad ent in Use_Target_Print");
		return;
	}

	if (ent->wait)
	{
		if (ent->genericValue14 >= level.time)
		{
			return;
		}
		ent->genericValue14 = level.time + ent->wait;
	}

#ifndef FINAL_BUILD
	if (!ent || !ent->inuse)
	{
		Com_Error(ERR_DROP, "Bad ent in Use_Target_Print");
	}
	else if (!activator || !activator->inuse)
	{
		Com_Error(ERR_DROP, "Bad activator in Use_Target_Print");
	}

	if (ent->genericValue15 > level.time)
	{
		Com_Printf("TARGET PRINT ERRORS:\n");
		if (activator && activator->classname && activator->classname[0])
		{
			Com_Printf("activator classname: %s\n", activator->classname);
		}
		if (activator && activator->target && activator->target[0])
		{
			Com_Printf("activator target: %s\n", activator->target);
		}
		if (activator && activator->targetname && activator->targetname[0])
		{
			Com_Printf("activator targetname: %s\n", activator->targetname);
		}
		if (ent->targetname && ent->targetname[0])
		{
			Com_Printf("print targetname: %s\n", ent->targetname);
		}
		Com_Error(ERR_DROP, "target_print used in quick succession, fix it! See the console for details.");
	}
	ent->genericValue15 = level.time + 5000;
#endif

	G_ActivateBehavior(ent,BSET_USE);
	if ( ( ent->spawnflags & 4 ) ) 
	{//private, to one client only
		if (!activator || !activator->inuse)
		{
			Com_Printf("ERROR: Bad activator in Use_Target_Print");
		}
		if ( activator && activator->client )
		{//make sure there's a valid client ent to send it to
			if (ent->message[0] == '@' && ent->message[1] != '@')
			{
				trap_SendServerCommand( activator-g_entities, va("cps \"%s\"", ent->message ));
			}
			else
			{
				trap_SendServerCommand( activator-g_entities, va("cp \"%s\"", ent->message ));
			}
		}
		//NOTE: change in functionality - if there *is* no valid client ent, it won't send it to anyone at all
		return;
	}

	if ( ent->spawnflags & 3 ) {
		if ( ent->spawnflags & 1 ) {
			if (ent->message[0] == '@' && ent->message[1] != '@')
			{
				G_TeamCommand( TEAM_RED, va("cps \"%s\"", ent->message) );
			}
			else
			{
				G_TeamCommand( TEAM_RED, va("cp \"%s\"", ent->message) );
			}
		}
		if ( ent->spawnflags & 2 ) {
			if (ent->message[0] == '@' && ent->message[1] != '@')
			{
				G_TeamCommand( TEAM_BLUE, va("cps \"%s\"", ent->message) );
			}
			else
			{
				G_TeamCommand( TEAM_BLUE, va("cp \"%s\"", ent->message) );
			}
		}
		return;
	}

	if (ent->message[0] == '@' && ent->message[1] != '@')
	{
		trap_SendServerCommand( -1, va("cps \"%s\"", ent->message ));
	}
	else
	{
		trap_SendServerCommand( -1, va("cp \"%s\"", ent->message ));
	}
}

void SP_target_print( gentity_t *ent ) {
	ent->use = Use_Target_Print;
}


//==========================================================


/*QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) looped-on looped-off global activator
"noise"		wav file to play

A global sound will play full volume throughout the level.
Activator sounds will play on the player that activated the target.
Global and activator sounds can't be combined with looping.
Normal sounds play each time the target is used.
Looped sounds will be toggled by use functions.
Multiple identical looping sounds will just increase volume without any speed cost.
"wait" : Seconds between auto triggerings, 0 = don't auto trigger
"random"	wait variance, default is 0
*/
void Use_Target_Speaker (gentity_t *ent, gentity_t *other, gentity_t *activator) {
	G_ActivateBehavior(ent,BSET_USE);

	if (ent->spawnflags & 3) {	// looping sound toggles
		if (ent->s.loopSound)
		{
			ent->s.loopSound = 0;	// turn it off
			ent->s.loopIsSoundset = qfalse;
			ent->s.trickedentindex = 1;
		}
		else
		{
			ent->s.loopSound = ent->noise_index;	// start it
			ent->s.loopIsSoundset = qfalse;
			ent->s.trickedentindex = 0;
		}
	}else {	// normal sound
		if ( ent->spawnflags & 8 ) {
			G_AddEvent( activator, EV_GENERAL_SOUND, ent->noise_index );
		} else if (ent->spawnflags & 4) {
			G_AddEvent( ent, EV_GLOBAL_SOUND, ent->noise_index );
		} else {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->noise_index );
		}
	}
}

void SP_target_speaker( gentity_t *ent ) {
	char	buffer[MAX_QPATH];
	char	*s;

	G_SpawnFloat( "wait", "0", &ent->wait );
	G_SpawnFloat( "random", "0", &ent->random );

	if ( G_SpawnString ( "soundSet", "", &s ) )
	{	// this is a sound set
		ent->s.soundSetIndex = G_SoundSetIndex(s);
		ent->s.eFlags = EF_PERMANENT;
		VectorCopy( ent->s.origin, ent->s.pos.trBase );
		trap_LinkEntity (ent);
		return;
	}

	if ( !G_SpawnString( "noise", "NOSOUND", &s ) ) {
		G_Error( "target_speaker without a noise key at %s", vtos( ent->s.origin ) );
	}

	// force all client reletive sounds to be "activator" speakers that
	// play on the entity that activates it
	if ( s[0] == '*' ) {
		ent->spawnflags |= 8;
	}

	Q_strncpyz( buffer, s, sizeof(buffer) );

	ent->noise_index = G_SoundIndex(buffer);

	// a repeating speaker can be done completely client side
	ent->s.eType = ET_SPEAKER;
	ent->s.eventParm = ent->noise_index;
	ent->s.frame = ent->wait * 10;
	ent->s.clientNum = ent->random * 10;


	// check for prestarted looping sound
	if ( ent->spawnflags & 1 ) {
		ent->s.loopSound = ent->noise_index;
		ent->s.loopIsSoundset = qfalse;
	}

	ent->use = Use_Target_Speaker;

	if (ent->spawnflags & 4) {
		ent->r.svFlags |= SVF_BROADCAST;
	}

	VectorCopy( ent->s.origin, ent->s.pos.trBase );

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	trap_LinkEntity( ent );
}



//==========================================================

/*QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON
When triggered, fires a laser.  You can either set a target or a direction.
*/
void target_laser_think (gentity_t *self) {
	vec3_t	end;
	trace_t	tr;
	vec3_t	point;

	// if pointed at another entity, set movedir to point at it
	if ( self->enemy ) {
		VectorMA (self->enemy->s.origin, 0.5, self->enemy->r.mins, point);
		VectorMA (point, 0.5, self->enemy->r.maxs, point);
		VectorSubtract (point, self->s.origin, self->movedir);
		VectorNormalize (self->movedir);
	}

	// fire forward and see what we hit
	VectorMA (self->s.origin, 2048, self->movedir, end);

	trap_Trace( &tr, self->s.origin, NULL, NULL, end, self->s.number, CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_CORPSE);

	if ( tr.entityNum ) {
		// hurt it if we can
		G_Damage ( &g_entities[tr.entityNum], self, self->activator, self->movedir, 
			tr.endpos, self->damage, DAMAGE_NO_KNOCKBACK, MOD_TARGET_LASER);
	}

	VectorCopy (tr.endpos, self->s.origin2);

	trap_LinkEntity( self );
	self->nextthink = level.time + FRAMETIME;
}

void target_laser_on (gentity_t *self)
{
	if (!self->activator)
		self->activator = self;
	target_laser_think (self);
}

void target_laser_off (gentity_t *self)
{
	trap_UnlinkEntity( self );
	self->nextthink = 0;
}

void target_laser_use (gentity_t *self, gentity_t *other, gentity_t *activator)
{
	self->activator = activator;
	if ( self->nextthink > 0 )
		target_laser_off (self);
	else
		target_laser_on (self);
}

void target_laser_start (gentity_t *self)
{
	gentity_t *ent;

	self->s.eType = ET_BEAM;

	if (self->target) {
		ent = G_Find (NULL, FOFS(targetname), self->target);
		if (!ent) {
			G_Printf ("%s at %s: %s is a bad target\n", self->classname, vtos(self->s.origin), self->target);
		}
		self->enemy = ent;
	} else {
		G_SetMovedir (self->s.angles, self->movedir);
	}

	self->use = target_laser_use;
	self->think = target_laser_think;

	if ( !self->damage ) {
		self->damage = 1;
	}

	if (self->spawnflags & 1)
		target_laser_on (self);
	else
		target_laser_off (self);
}

void SP_target_laser (gentity_t *self)
{
	// let everything else get spawned before we start firing
	self->think = target_laser_start;
	self->nextthink = level.time + FRAMETIME;
}


//==========================================================

void target_teleporter_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	gentity_t	*dest;

	if (!activator->client)
		return;

	G_ActivateBehavior(self,BSET_USE);

	dest = 	G_PickTarget( self->target );
	if (!dest) {
		G_Printf ("Couldn't find teleporter destination\n");
		return;
	}

	TeleportPlayer( activator, dest->s.origin, dest->s.angles );
}

/*QUAKED target_teleporter (1 0 0) (-8 -8 -8) (8 8 8)
The activator will be teleported away.
*/
void SP_target_teleporter( gentity_t *self ) {
	if (!self->targetname)
		G_Printf("untargeted %s at %s\n", self->classname, vtos(self->s.origin));

	self->use = target_teleporter_use;
}

//==========================================================


/*QUAKED target_relay (.5 .5 .5) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY RANDOM x x x x INACTIVE
This doesn't perform any actions except fire its targets.
The activator can be forced to be from a certain team.
if RANDOM is checked, only one of the targets will be fired, not all of them

  INACTIVE  Can't be used until activated

wait - set to -1 to use it only once
*/
void target_relay_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	qboolean ranscript = qfalse;
	if ( ( self->spawnflags & 1 ) && activator->client 
		&& activator->client->sess.sessionTeam != TEAM_RED ) {
		return;
	}
	if ( ( self->spawnflags & 2 ) && activator->client 
		&& activator->client->sess.sessionTeam != TEAM_BLUE ) {
		return;
	}

	if ( self->flags & FL_INACTIVE )
	{//set by target_deactivate
		return;
	}

	ranscript = G_ActivateBehavior( self, BSET_USE );
	if ( self->wait == -1 )
	{//never use again
		if ( ranscript )
		{//crap, can't remove!
			self->use = NULL;
		}
		else
		{//remove
			self->think = G_FreeEntity;
			self->nextthink = level.time + FRAMETIME;
		}
	}
	if ( self->spawnflags & 4 ) {
		gentity_t	*ent;

		ent = G_PickTarget( self->target );
		if ( ent && ent->use ) {
			GlobalUse( ent, self, activator );
		}
		return;
	}
	G_UseTargets (self, activator);
}

void SP_target_relay (gentity_t *self) {
	self->use = target_relay_use;
	if ( self->spawnflags&128 )
	{
		self->flags |= FL_INACTIVE;
	}
}


//==========================================================

/*QUAKED target_kill (.5 .5 .5) (-8 -8 -8) (8 8 8)
Kills the activator.
*/
void target_kill_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	G_ActivateBehavior(self,BSET_USE);
	G_Damage ( activator, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
}

void SP_target_kill( gentity_t *self ) {
	self->use = target_kill_use;
}

/*QUAKED target_position (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
*/
void SP_target_position( gentity_t *self ){
	G_SetOrigin( self, self->s.origin );
	/*
	G_SetAngles( self, self->s.angles );
	self->s.eType = ET_INVISIBLE;
	*/
}

static void target_location_linkup(gentity_t *ent)
{
	int i;
	int n;

	if (level.locationLinked) 
		return;

	level.locationLinked = qtrue;

	level.locationHead = NULL;

	trap_SetConfigstring( CS_LOCATIONS, "unknown" );

	for (i = 0, ent = g_entities, n = 1;
			i < level.num_entities;
			i++, ent++) {
		if (ent->classname && !Q_stricmp(ent->classname, "target_location")) {
			// lets overload some variables!
			ent->health = n; // use for location marking
			trap_SetConfigstring( CS_LOCATIONS + n, ent->message );
			n++;
			ent->nextTrain = level.locationHead;
			level.locationHead = ent;
		}
	}

	// All linked together now
}

/*QUAKED target_location (0 0.5 0) (-8 -8 -8) (8 8 8)
Set "message" to the name of this location.
Set "count" to 0-7 for color.
0:white 1:red 2:green 3:yellow 4:blue 5:cyan 6:magenta 7:white

Closest target_location in sight used for the location, if none
in site, closest in distance
*/
void SP_target_location( gentity_t *self ){
	self->think = target_location_linkup;
	self->nextthink = level.time + 200;  // Let them all spawn first

	G_SetOrigin( self, self->s.origin );
}

/*QUAKED target_counter (1.0 0 0) (-4 -4 -4) (4 4 4) x x x x x x x INACTIVE
Acts as an intermediary for an action that takes multiple inputs.

INACTIVE cannot be used until used by a target_activate

target2 - what the counter should fire each time it's incremented and does NOT reach it's count

After the counter has been triggered "count" times (default 2), it will fire all of it's targets and remove itself.

bounceCount - number of times the counter should reset to it's full count when it's done
*/
extern void G_DebugPrint( int level, const char *format, ... );
void target_counter_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->count == 0 )
	{
		return;
	}
	
	//gi.Printf("target_counter %s used by %s, entnum %d\n", self->targetname, activator->targetname, activator->s.number );
	self->count--;

	if ( activator )
	{
		G_DebugPrint( WL_VERBOSE, "target_counter %s used by %s (%d/%d)\n", self->targetname, activator->targetname, (self->genericValue1-self->count), self->genericValue1 );
	}

	if ( self->count )
	{
		if ( self->target2 )
		{
			//gi.Printf("target_counter %s firing target2 from %s, entnum %d\n", self->targetname, activator->targetname, activator->s.number );
			G_UseTargets2( self, activator, self->target2 );
		}
		return;
	}
	
	G_ActivateBehavior( self,BSET_USE );

	if ( self->spawnflags & 128 )
	{
		self->flags |= FL_INACTIVE;
	}

	self->activator = activator;
	G_UseTargets( self, activator );

	if ( self->count == 0 )
	{
		if ( self->bounceCount == 0 )
		{
			return;
		}
		self->count = self->genericValue1;
		if ( self->bounceCount > 0 )
		{//-1 means bounce back forever
			self->bounceCount--; 
		}
	}
}

void SP_target_counter (gentity_t *self)
{
	self->wait = -1;
	if (!self->count)
	{
		self->count = 2;
	}
	//if ( self->bounceCount > 0 )//let's always set this anyway
	{//we will reset when we use up our count, remember our initial count
		self->genericValue1 = self->count;
	}

	self->use = target_counter_use;
}

/*QUAKED target_random (.5 .5 .5) (-4 -4 -4) (4 4 4) USEONCE
Randomly fires off only one of it's targets each time used

USEONCE	set to never fire again
*/

void target_random_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	int			t_count = 0, pick;
	gentity_t	*t = NULL;

	//gi.Printf("target_random %s used by %s (entnum %d)\n", self->targetname, activator->targetname, activator->s.number );
	G_ActivateBehavior(self,BSET_USE);

	if(self->spawnflags & 1)
	{
		self->use = 0;
	}

	while ( (t = G_Find (t, FOFS(targetname), self->target)) != NULL )
	{
		if (t != self)
		{
			t_count++;
		}
	}

	if(!t_count)
	{
		return;
	}

	if(t_count == 1)
	{
		G_UseTargets (self, activator);
		return;
	}

	//FIXME: need a seed
	pick = Q_irand(1, t_count);
	t_count = 0;
	while ( (t = G_Find (t, FOFS(targetname), self->target)) != NULL )
	{
		if (t != self)
		{
			t_count++;
		}
		else
		{
			continue;
		}
		
		if (t == self)
		{
//				gi.Printf ("WARNING: Entity used itself.\n");
		}
		else if(t_count == pick)
		{
			if (t->use != NULL)	// check can be omitted
			{
				GlobalUse(t, self, activator);
				return;
			}
		}

		if (!self->inuse)
		{
			Com_Printf("entity was removed while using targets\n");
			return;
		}
	}
}

void SP_target_random (gentity_t *self)
{
	self->use = target_random_use;
}

int	numNewICARUSEnts = 0;
void scriptrunner_run (gentity_t *self)
{
	/*
	if (self->behaviorSet[BSET_USE])
	{	
		char	newname[MAX_FILENAME_LENGTH];

		sprintf((char *) &newname, "%s/%s", Q3_SCRIPT_DIR, self->behaviorSet[BSET_USE] );

		ICARUS_RunScript( self, newname );
	}
	*/

	if ( self->count != -1 )
	{
		if ( self->count <= 0 )
		{
			self->use = 0;
			self->behaviorSet[BSET_USE] = NULL;
			return;
		}
		else
		{
			--self->count;
		}
	}

	if (self->behaviorSet[BSET_USE])
	{
		if ( self->spawnflags & 1 )
		{
			if ( !self->activator )
			{
				if (g_developer.integer)
				{
					Com_Printf("target_scriptrunner tried to run on invalid entity!\n");
				}
				return;
			}

			//if ( !self->activator->sequencer || !self->activator->taskManager )
			if (!trap_ICARUS_IsInitialized(self->s.number))
			{//Need to be initialized through ICARUS
				if ( !self->activator->script_targetname || !self->activator->script_targetname[0] )
				{
					//We don't have a script_targetname, so create a new one
					self->activator->script_targetname = va( "newICARUSEnt%d", numNewICARUSEnts++ );
				}

				if ( trap_ICARUS_ValidEnt( self->activator ) )
				{
					trap_ICARUS_InitEnt( self->activator );
				}
				else
				{
					if (g_developer.integer)
					{
						Com_Printf("target_scriptrunner tried to run on invalid ICARUS activator!\n");
					}
					return;
				}
			}

			if (g_developer.integer)
			{
				Com_Printf( "target_scriptrunner running %s on activator %s\n", self->behaviorSet[BSET_USE], self->activator->targetname );
			}
			//[CoOp]
			//make the code handle the case of the scripts directory already being given
			if(!strncmp(self->behaviorSet[BSET_USE], va( "%s/", Q3_SCRIPT_DIR ), 8 ))
			{//already has script directory specified.
				trap_ICARUS_RunScript( self->activator, self->behaviorSet[BSET_USE] );
			}
			else
			{
				trap_ICARUS_RunScript( self->activator, va( "%s/%s", Q3_SCRIPT_DIR, self->behaviorSet[BSET_USE] ) );
			}
			//[/CoOp]
		}
		else
		{
			if ( g_developer.integer && self->activator )
			{
				Com_Printf( "target_scriptrunner %s used by %s\n", self->targetname, self->activator->targetname );
			}
			G_ActivateBehavior( self, BSET_USE );
		}
	}

	if ( self->wait )
	{
		self->nextthink = level.time + self->wait;
	}
}

void target_scriptrunner_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if ( self->nextthink > level.time )
	{
		return;
	}

	self->activator = activator;
	self->enemy = other;
	if ( self->delay )
	{//delay before firing scriptrunner
		self->think = scriptrunner_run;
		self->nextthink = level.time + self->delay;
	}
	else
	{
		scriptrunner_run (self);
	}
}

/*QUAKED target_scriptrunner (1 0 0) (-4 -4 -4) (4 4 4) runonactivator x x x x x x INACTIVE
--- SPAWNFLAGS ---
runonactivator - Will run the script on the entity that used this or tripped the trigger that used this
INACTIVE - start off

----- KEYS ------
Usescript - Script to run when used
count - how many times to run, -1 = infinite.  Default is once
wait - can't be used again in this amount of seconds (Default is 1 second if it's multiple-use)
delay - how long to wait after use to run script

*/
void SP_target_scriptrunner( gentity_t *self )
{
	float v;
	if ( self->spawnflags & 128 )
	{
		self->flags |= FL_INACTIVE;
	}

	if ( !self->count )
	{
		self->count = 1;//default 1 use only
	}
	/*
	else if ( !self->wait )
	{
		self->wait = 1;//default wait of 1 sec
	}
	*/
	// FIXME: this is a hack... because delay is read in as an int, so I'm bypassing that because it's too late in the project to change it and I want to be able to set less than a second delays
	// no one should be setting a radius on a scriptrunner, if they are this would be bad, take this out for the next project
	v = 0.0f;
	G_SpawnFloat( "delay", "0", &v );
	self->delay = v * 1000;//sec to ms
	self->wait *= 1000;//sec to ms

	G_SetOrigin( self, self->s.origin );
	self->use = target_scriptrunner_use;
}

void G_SetActiveState(char *targetstring, qboolean actState)
{
	gentity_t	*target = NULL;
	while( NULL != (target = G_Find(target, FOFS(targetname), targetstring)) )
	{
		target->flags = actState ? (target->flags&~FL_INACTIVE) : (target->flags|FL_INACTIVE);
	}
}

#define ACT_ACTIVE		qtrue
#define ACT_INACTIVE	qfalse

void target_activate_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);

	G_SetActiveState(self->target, ACT_ACTIVE);
}

void target_deactivate_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);

	G_SetActiveState(self->target, ACT_INACTIVE);
}

//FIXME: make these apply to doors, etc too?
/*QUAKED target_activate (1 0 0) (-4 -4 -4) (4 4 4)
Will set the target(s) to be usable/triggerable
*/
void SP_target_activate( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
	self->use = target_activate_use;
}

/*QUAKED target_deactivate (1 0 0) (-4 -4 -4) (4 4 4)
Will set the target(s) to be non-usable/triggerable
*/
void SP_target_deactivate( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
	self->use = target_deactivate_use;
}

//[CoOp]
extern void ExitLevel (void);
//[/CoOp]
void target_level_change_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	//[CoOp]
	vmCvar_t	mapname;
	//[/CoOp]

	G_ActivateBehavior(self,BSET_USE);

	//[CoOp]
	//Add this map's name to the tiers_complete cvar
	trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	strcpy( tiers_complete.string, va("%s|%s", tiers_complete.string, mapname.string) );

	if(g_autoMapCycle.integer)
	{//cycle to whatever this map is supposed to go to.
		if(self->message[0] == '+')
		{//a menu, open it
			char *s = self->message;
			s++;

			trap_SendServerCommand(-1, va("briefmenu %s", s));
		}
		else
		{//a menu, open it
			trap_Cvar_Set( "nextmap", va("map %s", self->message));
			ExitLevel();
		}
	}
	else
	{//otherwise, just go to the next map in the mapcycle
		ExitLevel();
	}	
	//[/CoOp]
}

/*QUAKED target_level_change (1 0 0) (-4 -4 -4) (4 4 4)
"mapname" - Name of map to change to
//[CoOp]
"storyhead"	 - which head to show on menu [luke, kyle, or prot]

*/
void SP_target_level_change( gentity_t *self )
{
	char *s;
	vmCvar_t		mapname;
	
	trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	G_SpawnString( "mapname", "", &s );
	self->message = G_NewString(s);

	//set custom storyhead varible or use the mapname by default.
	G_SpawnString( "storyhead", mapname.string, &s );
	trap_Cvar_Set( "storyhead", s );

	//set up tier_storyinfo.
	G_SpawnString( "tier_storyinfo", "++", &s );
	if(s[0] == '+' && s[1] == '+')
	{//increment tier_storyinfo
		trap_Cvar_Set( "tier_storyinfo", va("%i", (tier_storyinfo.integer + 1) ) );
	}
	else
	{//override with new value
		trap_Cvar_Set( "tier_storyinfo", s );
	}
//[/CoOp]

	if ( !self->message || !self->message[0] )
	{
		G_Error( "target_level_change with no mapname!\n");
		return;
	}

	G_SetOrigin( self, self->s.origin );
	self->use = target_level_change_use;
}

void target_play_music_use(gentity_t *self, gentity_t *other, gentity_t *activator)
{
	G_ActivateBehavior(self,BSET_USE);
	trap_SetConfigstring( CS_MUSIC, self->message );
}

/*QUAKED target_play_music (1 0 0) (-4 -4 -4) (4 4 4)
target_play_music
Plays the requested music files when this target is used.

"targetname"
"music"		music WAV or MP3 file ( music/introfile.mp3 [optional]  music/loopfile.mp3 )

If an intro file and loop file are specified, the intro plays first, then the looping
portion will start and loop indefinetly.  If no introfile is entered, only the loopfile
will play.
*/
void SP_target_play_music( gentity_t *self )
{
	char *s;

	G_SetOrigin( self, self->s.origin );
	if (!G_SpawnString( "music", "", &s ))
	{
		G_Error( "target_play_music without a music key at %s", vtos( self->s.origin ) );
	}

	self->message = G_NewString(s);

	self->use = target_play_music_use;
}

//[CoOp]
void Use_Secret( gentity_t *ent, gentity_t *other, gentity_t *activator ) 
{
	if(!activator || !activator->client)
	{
		G_Printf("Error: Non-Player activated target_secret\n.");
		return;
	}

	ent->count--;

	trap_SendServerCommand(-1, va("cp \"%s^7 discovered a secret area.\n\"", activator->client->pers.netname));
	G_Sound( activator, CHAN_AUTO, G_SoundIndex("sound/interface/secret_area"));

	if(ent->count == 1)
	{
		trap_SendServerCommand(-1, "cp \"^1One secret remains.\n\"");
	}
	else if(!ent->count)
	{
		trap_SendServerCommand(-1, "cp \"^1All secrets have been found!\n\"");

		//disable target
		ent->use = 0;
	}
	else
	{
		trap_SendServerCommand(-1, va("cp \"^4%i / %i^1 Secrets Found\n\"", ent->genericValue1-ent->count, ent->genericValue1));
	}
}

/*QUAKED target_secret - keeps count of the number of secret areas left undiscovered
on the map.
count - number of secret areas on map
*/
void SP_target_secret( gentity_t *self)
{
	self->use = Use_Secret;
	self->genericValue1 = self->count;

	//precash sound
	G_SoundIndex("sound/interface/secret_area");
}


/*
check to see if this is a good spawnpoint if not, return false.  In addition, move the 
spawn location vector down to the ground.
*/
qboolean CheckforGoodSpawnPoint(vec3_t location, qboolean playersolidcheck)
{
	trace_t tr;
	vec3_t locend;
	vec3_t	playerMins = {-15, -15, DEFAULT_MINS_2};
	vec3_t	playerMaxs = {15, 15, DEFAULT_MAXS_2};

	VectorCopy(location, locend);
	locend[2] -= 900;
	location[2] += 9;

	if(playersolidcheck)
	{//use a playersolid check
		trap_Trace(&tr, location, playerMins, playerMaxs, locend, -1, MASK_PLAYERSOLID);
	}
	else
	{//otherwise just check solid
		trap_Trace(&tr, location, playerMins, playerMaxs, locend, -1, MASK_SOLID);
	}

	if(tr.startsolid || tr.fraction == 1)
	{//either inside a solid or over a cliff
		return qfalse;
	}

	VectorCopy(tr.endpos, location);

	location[2] += 9;
	return qtrue;
}

//[CoOpEditor]
//flag for autosave map entities that makes all players teleport to the savepoint when
//used.
#define FLAG_TELETOSAVE		1
extern vmCvar_t bot_wp_edit;
extern qboolean SPSpawnpointCheck( vec3_t spawnloc );
//[/CoOpEditor]
void Use_Autosave( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	gentity_t *oldspawn = NULL;
	vec3_t spawnloc;
	vec3_t spawnang;
	gentity_t *spawn;

	if(activator && activator->client && activator->NPC)
	{//we don't want NPCs to trigger autosaves by accident.
		return;
	}

	if(activator && activator->client)
	{
		VectorCopy(activator->client->ps.origin, spawnloc);
		VectorSet(spawnang, 0, activator->client->ps.viewangles[YAW], 0);
	}
	else
	{
		VectorCopy(ent->s.origin, spawnloc);
		VectorSet(spawnang, 0, ent->s.angles[YAW], 0);
	}

	while ( (oldspawn = G_Find (oldspawn, FOFS(classname), "info_player_deathmatch")) != NULL )
	{//increase the other spawn's sequencial number
		oldspawn->genericValue1++;
	}

	if(!CheckforGoodSpawnPoint(spawnloc, qfalse))
	{
		G_Error("Bad spawnpoint in Use_Autosave().\n");
	}

	spawn = G_Spawn();

	if(spawn)
	{
		spawn->classname = "info_player_deathmatch";
		VectorCopy(spawnang, spawn->s.angles);
		VectorCopy(spawnloc, spawn->s.origin);
	}

	if(activator->client && activator->client->pers.netname)
	{
		trap_SendServerCommand(-1, va("cp \"%s^1 Reached Checkpoint.\n\"", activator->client->pers.netname));
	}
	else
	{
		trap_SendServerCommand(-1, "cp \"^1Reached Checkpoint.\n\"");
	}

	//[CoOpEditor]
	if(ent->spawnflags & FLAG_TELETOSAVE)
	{//teleport all players to the savepoint to prevent issues with scripting/blocked paths,
		//etc.
		int counter;
		gentity_t* player;

		for(counter = 0; counter < MAX_CLIENTS; counter++)
		{
			player = &g_entities[counter];
			if(!player->inuse || !player->client //player invalid
				|| player->client->pers.connected != CON_CONNECTED //not connected
				|| player->client->sess.sessionTeam == TEAM_SPECTATOR //spectator
				|| player->health <= 0 //dead
				|| (activator && activator->client 
				&& activator->s.number == player->s.number) ) //this player used the autosave.
			{//don't teleport this dude
				continue;
			}
		
			if(SPSpawnpointCheck(spawnloc))
			{//teleport them
				int flags;

				//flip the teleport flag so this dude doesn't client lerp
				flags = player->client->ps.eFlags & (EF_TELEPORT_BIT );
				flags ^= EF_TELEPORT_BIT;
				player->client->ps.eFlags = flags;

				//set origin
				G_SetOrigin(player, spawnloc);
				VectorCopy( spawnloc, player->client->ps.origin );

				//set angles
				SetClientViewAngle( player, spawnang );
			}
			else
			{//hmm, couldn't teleport.  
				G_Printf("Warning: Client %i couldn't spawn at savegame.\n", player->s.number);
			}
		}
	}

	if(bot_wp_edit.integer)
	{//just disable the savepoint while in the autosave editor
		ent->touch = 0;
		ent->use = 0;
	}
	else
	{
		ent->think = G_FreeEntity;
		ent->nextthink = level.time + FRAMETIME;
	}
	//[/CoOpEditor]
}


/*QUAKED target_autosave (1 0 0) (-4 -4 -4) (4 4 4)
SP Behavior - Auto save the game in two frames.
	 Make sure it won't trigger during dialogue or cinematic or it will stutter!

MP Behavior - Now used to create sequencial created spawnpoints for CoOp mode.  This will create
	 create a new spawnpoint at the current location and then increase the spawn sequencial
	 counter for the other spawns.  This only works for the CoOp gametype.

	 Use spawnflag = 1 to teleport all the other players to the autosave point when the autosave 
	 is triggered.
*/
void SP_target_autosave(gentity_t *self)
{
	self->use = Use_Autosave;
	//[CoOpEditor]
	//need classname so we can save/delete/load this entity.
	self->classname = "target_autosave";
	//[/CoOpEditor]
}
//[/CoOp]






