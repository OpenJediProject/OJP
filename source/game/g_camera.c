#include "q_shared.h"
#include "g_camera.h"
#include "g_local.h"

//prototypes
void EnablePlayerCameraPos(gentity_t *player);

qboolean	in_camera = qfalse;
camera_t	client_camera={0};

extern int g_TimeSinceLastFrame;

vec3_t camerapos;
vec3_t cameraang;

void GCam_Update( void )
{
	int	i;
	qboolean	checkFollow = qfalse;
	qboolean	checkTrack = qfalse;

	/* ROFF not enabled yet
	// Apply new roff data to the camera as needed
	if ( client_camera.info_state & CAMERA_ROFFING )
	{
		CGCam_Roff();
	}
	*/

	/* don't need the zoom stuff
	//Check for a zoom
	if (client_camera.info_state & CAMERA_ACCEL)
	{
		// x = x0 + vt + 0.5*a*t*t
		float	actualFOV_X = client_camera.FOV;
		float	sanityMin = 1, sanityMax = 180;
		float	t = (level.time - client_camera.FOV_time)*0.001; // mult by 0.001 cuz otherwise t is too darned big
		float	fovDuration = client_camera.FOV_duration;

//RAFIXME - looks like a debugger cvar or something.

#ifndef FINAL_BUILD
		if (cg_roffval4.integer)
		{
			fovDuration = cg_roffval4.integer;
		}
#endif

		
		if ( client_camera.FOV_time + fovDuration < cg.time )
		{
			client_camera.info_state &= ~CAMERA_ACCEL;
		}
		else
		{
			float	initialPosVal = client_camera.FOV2;
			float	velVal = client_camera.FOV_vel;
			float	accVal = client_camera.FOV_acc;
 RAFIXME - another debugger cvar
#ifndef FINAL_BUILD
			if (cg_roffdebug.integer)
			{
				if (fabs(cg_roffval1.value) > 0.001f)
				{
					initialPosVal = cg_roffval1.value;
				}
				if (fabs(cg_roffval2.value) > 0.001f)
				{
					velVal = cg_roffval2.value;
				}
				if (fabs(cg_roffval3.value) > 0.001f)
				{
					accVal = cg_roffval3.value;
				}
			}
#endif

			float	initialPos = initialPosVal;
			float	vel = velVal*t;
			float	acc = 0.5*accVal*t*t;

			actualFOV_X = initialPos + vel + acc;
			RAFIXME - another debugger cvar
			if (cg_roffdebug.integer)
			{
				Com_Printf("%d: fovaccel from %2.1f using vel = %2.4f, acc = %2.4f (current fov calc = %5.6f)\n",
					cg.time, initialPosVal, velVal, accVal, actualFOV_X);
			}

			if (actualFOV_X < sanityMin)
			{
				actualFOV_X = sanityMin;
			}
			else if (actualFOV_X > sanityMax)
			{
				actualFOV_X = sanityMax;
			}
			client_camera.FOV = actualFOV_X;
		}
		CG_CalcFOVFromX( actualFOV_X );
	}
	else if ( client_camera.info_state & CAMERA_ZOOMING )
	{
		float	actualFOV_X;

		if ( client_camera.FOV_time + client_camera.FOV_duration < cg.time )
		{
			actualFOV_X = client_camera.FOV = client_camera.FOV2;
			client_camera.info_state &= ~CAMERA_ZOOMING;
		}
		else
		{
			actualFOV_X = client_camera.FOV + (( ( client_camera.FOV2 - client_camera.FOV ) ) / client_camera.FOV_duration ) * ( cg.time - client_camera.FOV_time );
		}
		CG_CalcFOVFromX( actualFOV_X );
	}
	else
	{
		CG_CalcFOVFromX( client_camera.FOV );
	}
	*/

	//Check for roffing angles
	if ( (client_camera.info_state & CAMERA_ROFFING) && !(client_camera.info_state & CAMERA_FOLLOWING) )
	{
		if (client_camera.info_state & CAMERA_CUT)
		{
			// we're doing a cut, so just go to the new angles. none of this hifalutin lerping business.
			for ( i = 0; i < 3; i++ )
			{
				cameraang[i] = AngleNormalize360( ( client_camera.angles[i] + client_camera.angles2[i] ) );
			}
		}
		else
		{
			for ( i = 0; i < 3; i++ )
			{
				cameraang[i] =  client_camera.angles[i] + ( client_camera.angles2[i] / client_camera.pan_duration ) * ( level.time - client_camera.pan_time );
			}
		}
	}
	else if ( client_camera.info_state & CAMERA_PANNING )
	{
		if (client_camera.info_state & CAMERA_CUT)
		{
			// we're doing a cut, so just go to the new angles. none of this hifalutin lerping business.
			for ( i = 0; i < 3; i++ )
			{
				cameraang[i] = AngleNormalize360( ( client_camera.angles[i] + client_camera.angles2[i] ) );
			}
		}
		else
		{
			//Note: does not actually change the camera's angles until the pan time is done!
			if ( client_camera.pan_time + client_camera.pan_duration < level.time )
			{//finished panning
				for ( i = 0; i < 3; i++ )
				{
					cameraang[i] = AngleNormalize360( ( client_camera.angles[i] + client_camera.angles2[i] ) );
				}

				client_camera.info_state &= ~CAMERA_PANNING;
				VectorCopy(client_camera.angles, cameraang );
			}
			else
			{//still panning
				for ( i = 0; i < 3; i++ )
				{
					//NOTE: does not store the resultant angle in client_camera.angles until pan is done
					cameraang[i] = client_camera.angles[i] + ( client_camera.angles2[i] / client_camera.pan_duration ) * ( level.time - client_camera.pan_time );
				}
			}
		}
	}
	else 
	{
		checkFollow = qtrue;
	}

	//Check for movement
	if ( client_camera.info_state & CAMERA_MOVING )
	{
		//NOTE: does not actually move the camera until the movement time is done!
		if ( client_camera.move_time + client_camera.move_duration < level.time )
		{
			VectorCopy( client_camera.origin2, client_camera.origin );
			client_camera.info_state &= ~CAMERA_MOVING;
			VectorCopy( client_camera.origin, camerapos );
		}
		else
		{
			if (client_camera.info_state & CAMERA_CUT)
			{
				// we're doing a cut, so just go to the new origin. none of this fancypants lerping stuff.
				for ( i = 0; i < 3; i++ )
				{
					camerapos[i] = client_camera.origin2[i];
				}
			}
			else
			{
				for ( i = 0; i < 3; i++ )
				{
					camerapos[i] = client_camera.origin[i] + (( ( client_camera.origin2[i] - client_camera.origin[i] ) ) / client_camera.move_duration ) * ( level.time - client_camera.move_time );
				}
			}
		}
	}
	else
	{
		checkTrack = qtrue;
	}

	if ( checkFollow )
	{
		if ( client_camera.info_state & CAMERA_FOLLOWING )
		{//This needs to be done after camera movement
			GCam_FollowUpdate();
		}
		VectorCopy(client_camera.angles, cameraang );
	}

	/* tracking not enabled yet
	if ( checkTrack )
	{
		if ( client_camera.info_state & CAMERA_TRACKING )
		{//This has to run AFTER Follow if the camera is following a cameraGroup
			CGCam_TrackUpdate();
		}

		VectorCopy( client_camera.origin, cg.refdef.vieworg );
	}

	/* no fading needed
	//Bar fading
	if ( client_camera.info_state & CAMERA_BAR_FADING )
	{
		CGCam_UpdateBarFade();
	}

	//Normal fading - separate call because can finish after camera is disabled
	CGCam_UpdateFade();

	//Update shaking if there's any
	//CGCam_UpdateSmooth( cg.refdef.vieworg, cg.refdefViewAngles );
	CGCam_UpdateShake( cg.refdef.vieworg, cg.refdef.viewangles );
	AnglesToAxis( cg.refdef.viewangles, cg.refdef.viewaxis );
	*/
}


void GCam_FollowUpdate ( void )
{
	vec3_t		center, dir, cameraAngles, vec, focus[MAX_CAMERA_GROUP_SUBJECTS];//No more than 16 subjects in a cameraGroup
	gentity_t	*from = NULL;
	//centity_t	*fromCent = NULL;
	int			num_subjects = 0, i;
	qboolean	focused = qfalse;
	
	if ( client_camera.cameraGroup[0] == -1 )
	{//follow disabled
		return;
	}

	for( i = 0; i < MAX_CAMERA_GROUP_SUBJECTS; i++ )
	{
		//fromCent = &cg_entities[client_camera.cameraGroup[i]];
		from = &g_entities[client_camera.cameraGroup[i]];
		if ( !from )
		{
			continue;
		}

		focused = qfalse;
		
		if ( (from->s.eType == ET_PLAYER 
			|| from->s.eType == ET_NPC 
			|| from->s.number < MAX_CLIENTS)
			&& client_camera.cameraGroupTag && client_camera.cameraGroupTag[0] )
		{
			int newBolt = trap_G2API_AddBolt( &from->ghoul2, 0, client_camera.cameraGroupTag );
			if ( newBolt != -1 )
			{
				mdxaBone_t	boltMatrix;
				vec3_t		angle;

				VectorSet(angle, 0, from->client->ps.viewangles[YAW], 0);

				trap_G2API_GetBoltMatrix( &from->ghoul2, 0, newBolt, &boltMatrix, angle, from->client->ps.origin, level.time, NULL, from->modelScale );
				BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, focus[num_subjects] );

				focused = qtrue;
			}
		}
		if ( !focused )
		{
			VectorCopy(from->r.currentOrigin, focus[num_subjects]);
			/*
			if ( fromCent->currentState.pos.trType != TR_STATIONARY )
//				if ( from->s.pos.trType == TR_INTERPOLATE )
			{//use interpolated origin?
				if ( VectorCompare( vec3_origin, fromCent->lerpOrigin ) )
				{//we can't cheat like we could for SP
					continue;
				}
				else
				{
					VectorCopy( fromCent->lerpOrigin, focus[num_subjects] );
				}
			}
			else
			{
				VectorCopy(fromCent->currentState.origin, focus[num_subjects]);
			} 
			*/
			if ( from->s.eType == ET_PLAYER 
				|| from->s.eType == ET_NPC 
				|| from->s.number < MAX_CLIENTS )
			{//Track to their eyes - FIXME: maybe go off a tag?
				focus[num_subjects][2] += from->client->ps.viewheight;
			}
		}
		if ( client_camera.cameraGroupZOfs )
		{
			focus[num_subjects][2] += client_camera.cameraGroupZOfs;
		}
		num_subjects++;
	}

	if ( !num_subjects )	// Bad cameragroup 
	{
#ifndef FINAL_BUILD
		G_Printf(S_COLOR_RED"ERROR: Camera Focus unable to locate cameragroup: %s\n", client_camera.cameraGroup);
#endif
		return;
	}

	//Now average all points
	VectorCopy( focus[0], center );
	for( i = 1; i < num_subjects; i++ )
	{
		VectorAdd( focus[i], center, center );
	}
	VectorScale( center, 1.0f/((float)num_subjects), center );

	/*
	if ( client_camera.cameraGroup && client_camera.cameraGroup[0] )
	{
		//Stay centered in my cameraGroup, if I have one
		while( NULL != (from = G_Find(from, FOFS(cameraGroup), client_camera.cameraGroup)))
		{
			/*
			if ( from->s.number == client_camera.aimEntNum )
			{//This is the misc_camera_focus, we'll be removing this ent altogether eventually
				continue;
			}
			*/
			/*
			if ( num_subjects >= MAX_CAMERA_GROUP_SUBJECTS )
			{
				gi.Printf(S_COLOR_RED"ERROR: Too many subjects in shot composition %s", client_camera.cameraGroup);
				break;
			}

			fromCent = &cg_entities[from->s.number];
			if ( !fromCent )
			{
				continue;
			}

			focused = qfalse;
			if ( from->client && client_camera.cameraGroupTag && client_camera.cameraGroupTag[0] && fromCent->gent->ghoul2.size() )
			{
				int newBolt = gi.G2API_AddBolt( &fromCent->gent->ghoul2[from->playerModel], client_camera.cameraGroupTag );
				if ( newBolt != -1 )
				{
					mdxaBone_t	boltMatrix;
					vec3_t	fromAngles = {0,from->client->ps.legsYaw,0};

					gi.G2API_GetBoltMatrix( fromCent->gent->ghoul2, from->playerModel, newBolt, &boltMatrix, fromAngles, fromCent->lerpOrigin, cg.time, cgs.model_draw, fromCent->currentState.modelScale );
					gi.G2API_GiveMeVectorFromMatrix( boltMatrix, ORIGIN, focus[num_subjects] );

					focused = qtrue;
				}
			}
			if ( !focused )
			{
				if ( from->s.pos.trType != TR_STATIONARY )
//				if ( from->s.pos.trType == TR_INTERPOLATE )
				{//use interpolated origin?
					if ( !VectorCompare( vec3_origin, fromCent->lerpOrigin ) )
					{//hunh?  Somehow we've never seen this gentity on the client, so there is no lerpOrigin, so cheat over to the game and use the currentOrigin
						VectorCopy( from->currentOrigin, focus[num_subjects] );
					}
					else
					{
						VectorCopy( fromCent->lerpOrigin, focus[num_subjects] );
					}
				}
				else
				{
					VectorCopy(from->currentOrigin, focus[num_subjects]);
				} 
				//FIXME: make a list here of their s.numbers instead so we can do other stuff with the list below
				if ( from->client )
				{//Track to their eyes - FIXME: maybe go off a tag?
					//FIXME: 
					//Based on FOV and distance to subject from camera, pick the point that
					//keeps eyes 3/4 up from bottom of screen... what about bars?
					focus[num_subjects][2] += from->client->ps.viewheight;
				}
			}
			if ( client_camera.cameraGroupZOfs )
			{
				focus[num_subjects][2] += client_camera.cameraGroupZOfs;
			}
			num_subjects++;
		}

		if ( !num_subjects )	// Bad cameragroup 
		{
#ifndef FINAL_BUILD
			gi.Printf(S_COLOR_RED"ERROR: Camera Focus unable to locate cameragroup: %s\n", client_camera.cameraGroup);
#endif
			return;
		}

		//Now average all points
		VectorCopy( focus[0], center );
		for( i = 1; i < num_subjects; i++ )
		{
			VectorAdd( focus[i], center, center );
		}
		VectorScale( center, 1.0f/((float)num_subjects), center );
	}
	else
	{
		return;
	}
	*/

	//Need to set a speed to keep a distance from
	//the subject- fixme: only do this if have a distance
	//set
	VectorSubtract( client_camera.subjectPos, center, vec );
	client_camera.subjectSpeed = VectorLengthSquared( vec ) * 100.0f / g_TimeSinceLastFrame;

	/*
	if ( !cg_skippingcin.integer )
	{
		Com_Printf( S_COLOR_RED"org: %s\n", vtos(center) );
	}
	*/
	VectorCopy( center, client_camera.subjectPos );

	VectorSubtract( center, camerapos, dir );//can't use client_camera.origin because it's not updated until the end of the move.

	//Get desired angle
	vectoangles(dir, cameraAngles);
	
	if ( client_camera.followInitLerp )
	{//Lerping
		float frac = g_TimeSinceLastFrame/100.0f * client_camera.followSpeed/100.f;
		for( i = 0; i < 3; i++ )
		{
			cameraAngles[i] = AngleNormalize180( cameraAngles[i] );
			cameraAngles[i] = AngleNormalize180( client_camera.angles[i] + frac * AngleNormalize180(cameraAngles[i] - client_camera.angles[i]) );
			cameraAngles[i] = AngleNormalize180( cameraAngles[i] );
		}
/*#if 0
		Com_Printf( "%s\n", vtos(cameraAngles) );
#endif*/
	}
	else
	{//Snapping, should do this first time if follow_lerp_to_start_duration is zero
		//will lerp from this point on
		client_camera.followInitLerp = qtrue;
		for( i = 0; i < 3; i++ )
		{//normalize so that when we start lerping, it doesn't freak out
			cameraAngles[i] = AngleNormalize180( cameraAngles[i] );
		}
		//So tracker doesn't move right away thinking the first angle change
		//is the subject moving... FIXME: shouldn't set this until lerp done OR snapped?
		client_camera.subjectSpeed = 0;
	}

	//Point camera to lerp angles
	/*
	if ( !cg_skippingcin.integer )
	{
		Com_Printf( "ang: %s\n", vtos(cameraAngles) );
	}
	*/
	VectorCopy( cameraAngles, client_camera.angles );
}


void GCam_FollowDisable( void )
{
	int i;
	client_camera.info_state &= ~CAMERA_FOLLOWING;
	//client_camera.cameraGroup[0] = 0;
	for(i = 0; i < MAX_CAMERA_GROUP_SUBJECTS; i++)
	{
		client_camera.cameraGroup[i] = -1;
	}
	client_camera.cameraGroupZOfs = 0;
	client_camera.cameraGroupTag[0] = 0;
}

void GCam_TrackDisable( void )
{
	client_camera.info_state &= ~CAMERA_TRACKING;
	client_camera.trackEntNum = ENTITYNUM_WORLD;
}


void GCam_DistanceDisable( void )
{
	client_camera.distance = 0;
}


void GCam_SetPosition( vec3_t org )
{
	VectorCopy( org, client_camera.origin );
	VectorCopy( client_camera.origin, camerapos );
}



void GCam_Move( vec3_t dest, float duration )
{
	if ( client_camera.info_state & CAMERA_ROFFING )
	{
		client_camera.info_state &= ~CAMERA_ROFFING;
	}

	GCam_TrackDisable();
	GCam_DistanceDisable();

	if ( !duration )
	{
		client_camera.info_state &= ~CAMERA_MOVING;
		GCam_SetPosition( dest );
		return;
	}

	client_camera.info_state |= CAMERA_MOVING;

	VectorCopy( dest, client_camera.origin2 );
	
	client_camera.move_duration = duration;
	client_camera.move_time = level.time;
}


void GCam_Follow( int cameraGroup[MAX_CAMERA_GROUP_SUBJECTS], float speed, float initLerp )
{
	int len;

	//Clear any previous
	GCam_FollowDisable();

	if(cameraGroup[0] == -2)
	{//only wanted to disable follow mode
		return;
	}

	/*
	if(!cameraGroup || !cameraGroup[0])
	{
		return;
	}

	if ( Q_stricmp("none", (char *)cameraGroup) == 0 )
	{//Turn off all aiming
		return;
	}
	
	if ( Q_stricmp("NULL", (char *)cameraGroup) == 0 )
	{//Turn off all aiming
		return;
	}
	*/

	//NOTE: if this interrupts a pan before it's done, need to copy the cg.refdef.viewAngles to the camera.angles!
	client_camera.info_state |= CAMERA_FOLLOWING;
	client_camera.info_state &= ~CAMERA_PANNING;

	//len = strlen(cameraGroup);
	//strncpy( client_camera.cameraGroup, cameraGroup, sizeof(client_camera.cameraGroup) );
	//NULL terminate last char in case they type a name too long
	//client_camera.cameraGroup[len] = 0;

	for( len = 0; len < MAX_CAMERA_GROUP_SUBJECTS; len++)
	{
		client_camera.cameraGroup[len] = cameraGroup[len];
	}

	if ( speed )
	{
		client_camera.followSpeed = speed;
	}
	else
	{
		client_camera.followSpeed = 100.0f;
	}

	if ( initLerp )
	{
		client_camera.followInitLerp = qtrue;
	}
	else
	{
		client_camera.followInitLerp = qfalse;
	}
}


void GCam_Enable( void )
{
	client_camera.bar_alpha = 0.0f;
	client_camera.bar_time = level.time;

	client_camera.bar_alpha_source = 0.0f;
	client_camera.bar_alpha_dest = 1.0f;
	
	client_camera.bar_height_source = 0.0f;
	client_camera.bar_height_dest = 480/10;
	client_camera.bar_height = 0.0f;

	client_camera.info_state |= CAMERA_BAR_FADING;

	client_camera.FOV	= CAMERA_DEFAULT_FOV;
	client_camera.FOV2	= CAMERA_DEFAULT_FOV;

	in_camera = qtrue;

	client_camera.next_roff_time = 0;
}


void GCam_Disable( void )
{//disable the server side part of the camera stuff.

	in_camera = qfalse;

	client_camera.bar_alpha = 1.0f;
	client_camera.bar_time = level.time;

	client_camera.bar_alpha_source = 1.0f;
	client_camera.bar_alpha_dest = 0.0f;
	
	client_camera.bar_height_source = 480/10;
	client_camera.bar_height_dest = 0.0f;

	client_camera.info_state |= CAMERA_BAR_FADING;

	DisablePlayerCameraPos();	
}


void GCam_SetAngles( vec3_t ang )
{
	VectorCopy( ang, client_camera.angles );
	VectorCopy(client_camera.angles, cameraang );
}


void GCam_Pan( vec3_t dest, vec3_t panDirection, float duration )
{
	//vec3_t	panDirection = {0, 0, 0};
	int		i;
	float	delta1 , delta2;

	GCam_FollowDisable();
	GCam_DistanceDisable();

	if ( !duration )
	{
		GCam_SetAngles( dest );
		client_camera.info_state &= ~CAMERA_PANNING;
		return;
	}

	//FIXME: make the dest an absolute value, and pass in a
	//panDirection as well.  If a panDirection's axis value is
	//zero, find the shortest difference for that axis.
	//Store the delta in client_camera.angles2. 
	for( i = 0; i < 3; i++ )
	{
		dest[i] = AngleNormalize360( dest[i] );
		delta1 = dest[i] - AngleNormalize360( client_camera.angles[i] );
		if ( delta1 < 0 )
		{
			delta2 = delta1 + 360;
		}
		else
		{
			delta2 = delta1 - 360;
		}
		if ( !panDirection[i] )
		{//Didn't specify a direction, pick shortest
			if( Q_fabs(delta1) < Q_fabs(delta2) )
			{
				client_camera.angles2[i] = delta1;
			}
			else
			{
				client_camera.angles2[i] = delta2;
			}
		}
		else if ( panDirection[i] < 0 )
		{
			if( delta1 < 0 )
			{
				client_camera.angles2[i] = delta1;
			}
			else if( delta1 > 0 )
			{
				client_camera.angles2[i] = delta2;
			}
			else
			{//exact
				client_camera.angles2[i] = 0;
			}
		}
		else if ( panDirection[i] > 0 )
		{
			if( delta1 > 0 )
			{
				client_camera.angles2[i] = delta1;
			}
			else if( delta1 < 0 )
			{
				client_camera.angles2[i] = delta2;
			}
			else
			{//exact
				client_camera.angles2[i] = 0;
			}
		}
	}
	//VectorCopy( dest, client_camera.angles2 );

	client_camera.info_state |= CAMERA_PANNING;
	
	client_camera.pan_duration = duration;
	client_camera.pan_time = level.time;
}


/*==================================
Player Positional Stuff
====================================*/

typedef struct 
{
	vec3_t		origin;
	vec3_t		viewangles;
	qboolean	inuse;
} playerPos_t;

//stores the original positions of all the players so the game can move them around for cutscenes.
playerPos_t playerCamPos[MAX_CLIENTS]; 

//while in a cutscene have all the player origins/angles on the camera's.  we do this to
//make the game render the scenes for the camera correctly.
void UpdatePlayerCameraPos(void)
{
	int i;
	gentity_t *player; 

	if(!in_camera)
	{
		return;
	}

	GCam_Update();
	for(i = 0; i < MAX_CLIENTS; i++)
	{
		player = &g_entities[i];
		if(!player || !player->client || !player->inuse 
			|| player->client->pers.connected != CON_CONNECTED)
		{//player not ingame
			continue;
		}

		if(!playerCamPos[i].inuse)
		{//player hasn't been snapped to camera yet
			EnablePlayerCameraPos(player);
		}

		//move the player origin/angles to the camera's
		VectorCopy(camerapos, player->client->ps.origin);
		VectorCopy(cameraang, player->client->ps.viewangles);


	}
}


extern void SaberUpdateSelf(gentity_t *ent);
extern void WP_SaberRemoveG2Model( gentity_t *saberent );
void EnablePlayerCameraPos(gentity_t *player)
{//set this player it be in camera mode
	int x;

	if(!player || !player->client || !player->inuse)
	{//bad player entity
		return;
	}

	//turn off zoomMode
	player->client->ps.zoomMode = 0;

	//holster sabers
	player->client->ps.saberHolstered = 2;

	if ( player->client->ps.saberInFlight && player->client->ps.saberEntityNum )
	{//saber is out
		gentity_t *saberent = &g_entities[player->client->ps.saberEntityNum];
		if ( saberent )
		{//force the weapon back to our hand.
			saberent->genericValue5 = 0;
			saberent->think = SaberUpdateSelf;
			saberent->nextthink = level.time;
			WP_SaberRemoveG2Model( saberent );
			
			player->client->ps.saberInFlight = qfalse;
			player->client->ps.saberEntityState = 0;
			player->client->ps.saberThrowDelay = level.time + 500;
			player->client->ps.saberCanThrow = qfalse;
		}
	}

	for ( x = 0; x < NUM_FORCE_POWERS; x++ )
	{//deactivate any active force powers
		player->client->ps.fd.forcePowerDuration[x] = 0;
		if ( player->client->ps.fd.forcePowerDuration[x] || (player->client->ps.fd.forcePowersActive&( 1 << x )) )
		{
			WP_ForcePowerStop( player, (forcePowers_t)x );
		}
	}

	VectorClear( player->client->ps.velocity );
	
	//make invisible
	player->s.eFlags |= EF_NODRAW;
	player->client->ps.eFlags |= EF_NODRAW;

	//go noclip
	player->client->noclip = qtrue;

	//save our current position to the array
	VectorCopy(player->client->ps.origin, playerCamPos[player->s.number].origin);
	VectorCopy(player->client->ps.viewangles, playerCamPos[player->s.number].viewangles);
	playerCamPos[player->s.number].inuse = qtrue;
}


extern qboolean SPSpawnpointCheck( vec3_t spawnloc );
void DisablePlayerCameraPos(void)
{//disable the player camera position code.
	int i;
	gentity_t *player; 

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		int flags;

		if(!playerCamPos[i].inuse)
		{//player's camera position was never set, just move on.
			continue;
		}

		//since we're going to be done with the player positional data no matter what after this, 
		//say that we're done with it.
		playerCamPos[i].inuse = qfalse;

		player = &g_entities[i];
		if(!player || !player->client || !player->inuse 
			|| player->client->pers.connected != CON_CONNECTED)
		{//player not ingame

			continue;
		}

		//make solid and make visible
		player->client->noclip = qfalse;
		player->s.eFlags &= ~EF_NODRAW;
		player->client->ps.eFlags &= ~EF_NODRAW;

		//check our respawn point
		if(!SPSpawnpointCheck(playerCamPos[i].origin))
		{//couldn't spawn in our original area, kill them.
			G_Damage ( player, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
			continue;
		}

		//flip the teleport flag so this dude doesn't client lerp
		flags = player->client->ps.eFlags & (EF_TELEPORT_BIT );
		flags ^= EF_TELEPORT_BIT;
		player->client->ps.eFlags = flags;

		//restore view angle
		SetClientViewAngle(player, playerCamPos[i].viewangles);

		//found good spot, move them there
		G_SetOrigin(player, playerCamPos[i].origin);
		VectorCopy(playerCamPos[i].origin, player->client->ps.origin);
	

		trap_LinkEntity(player);

	}
}


void UpdatePlayerCameraOrigin(gentity_t * ent, vec3_t newPos)
{//this alters the position at which the player will return to after the camera commands are over.
	if(!in_camera || ent->s.number >= MAX_CLIENTS || !playerCamPos[ent->s.number].inuse)
	{//not using camera or not able to use camera
		return;
	}

	VectorCopy(newPos, playerCamPos[ent->s.number].origin);

}


void UpdatePlayerCameraAngle(gentity_t * ent, vec3_t newAngle)
{//this function alters the angle at which the player will return to after the camera commands are over.
	if(!in_camera || ent->s.number >= MAX_CLIENTS || !playerCamPos[ent->s.number].inuse)
	{//not using camera or not able to use camera
		return;
	}

	VectorCopy(newAngle, playerCamPos[ent->s.number].viewangles);

}

