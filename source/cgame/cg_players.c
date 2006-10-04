// Copyright (C) 1999-2000 Id Software, Inc.
//
// cg_players.c -- handle the media and animation for player entities
#include "cg_local.h"
//[Mac]
#if MAC_PORT
#include "../ghoul2/g2.h"
#else
#include "..\ghoul2\g2.h"
#endif
//[/Mac]
#include "bg_saga.h"

//[TrueView]
//True View Camera Position Check Function
extern void CheckCameraLocation( vec3_t OldeyeOrigin );
//[/TrueView]

extern vmCvar_t	cg_thirdPersonAlpha;

extern int			cgSiegeTeam1PlShader;
extern int			cgSiegeTeam2PlShader;

extern void CG_AddRadarEnt(centity_t *cent);	//cg_ents.c
extern void CG_AddBracketedEnt(centity_t *cent);	//cg_ents.c
extern qboolean CG_InFighter( void );
extern qboolean WP_SaberBladeUseSecondBladeStyle( saberInfo_t *saber, int bladeNum );


//for g2 surface routines
#define TURN_ON				0x00000000
#define TURN_OFF			0x00000100

extern stringID_table_t animTable [MAX_ANIMATIONS+1];

char	*cg_customSoundNames[MAX_CUSTOM_SOUNDS] = {
	"*death1",
	"*death2",
	"*death3",
	"*jump1",
	"*pain25",
	"*pain50",
	"*pain75",
	"*pain100",
	"*falling1",
	"*choke1",
	"*choke2",
	"*choke3",
	"*gasp",
	"*land1",
	"*taunt",
	NULL
};

//NPC sounds:
//Used as a supplement to the basic set for enemies and hazard team
// (keep numbers in ascending order in order for variant-capping to work)
const char	*cg_customCombatSoundNames[MAX_CUSTOM_COMBAT_SOUNDS] = 
{
	"*anger1",	//Say when acquire an enemy when didn't have one before
	"*anger2",
	"*anger3",
	"*victory1",	//Say when killed an enemy
	"*victory2",
	"*victory3",
	"*confuse1",	//Say when confused
	"*confuse2",
	"*confuse3",
	"*pushed1",	//Say when force-pushed
	"*pushed2",
	"*pushed3",
	"*choke1",
	"*choke2",
	"*choke3",
	"*ffwarn",
	"*ffturn",
	NULL
};

//Used as a supplement to the basic set for stormtroopers
// (keep numbers in ascending order in order for variant-capping to work)
const char	*cg_customExtraSoundNames[MAX_CUSTOM_EXTRA_SOUNDS] = 
{
	"*chase1",
	"*chase2",
	"*chase3",
	"*cover1",
	"*cover2",
	"*cover3",
	"*cover4",
	"*cover5",
	"*detected1",
	"*detected2",
	"*detected3",
	"*detected4",
	"*detected5",
	"*lost1",
	"*outflank1",
	"*outflank2",
	"*escaping1",
	"*escaping2",
	"*escaping3",
	"*giveup1",
	"*giveup2",
	"*giveup3",
	"*giveup4",
	"*look1",
	"*look2",
	"*sight1",
	"*sight2",
	"*sight3",
	"*sound1",
	"*sound2",
	"*sound3",
	"*suspicious1",
	"*suspicious2",
	"*suspicious3",
	"*suspicious4",
	"*suspicious5",
	NULL
};

//Used as a supplement to the basic set for jedi
// (keep numbers in ascending order in order for variant-capping to work)
const char	*cg_customJediSoundNames[MAX_CUSTOM_JEDI_SOUNDS] = 
{
	"*combat1",
	"*combat2",
	"*combat3",
	"*jdetected1",
	"*jdetected2",
	"*jdetected3",
	"*taunt1",
	"*taunt2",
	"*taunt3",
	"*jchase1",
	"*jchase2",
	"*jchase3",
	"*jlost1",
	"*jlost2",
	"*jlost3",
	"*deflect1",
	"*deflect2",
	"*deflect3",
	"*gloat1",
	"*gloat2",
	"*gloat3",
	"*pushfail",
	NULL
};

//Used for DUEL taunts
const char	*cg_customDuelSoundNames[MAX_CUSTOM_DUEL_SOUNDS] = 
{
	"*anger1",	//Say when acquire an enemy when didn't have one before
	"*anger2",
	"*anger3",
	"*victory1",	//Say when killed an enemy
	"*victory2",
	"*victory3",
	"*taunt1",
	"*taunt2",
	"*taunt3",
	"*deflect1",
	"*deflect2",
	"*deflect3",
	"*gloat1",
	"*gloat2",
	"*gloat3",
	NULL
};


//[ANIMEVENTS]
//Moved all this stuff from bg_panimate.c and bg_public.h to make things easier.  
//Please note that there have been a lot of changes from the original functions.

extern stringID_table_t animEventTypeTable[MAX_ANIM_EVENTS+1];
extern stringID_table_t footstepTypeTable[NUM_FOOTSTEP_TYPES+1];

//size of Anim eventData array...
#define MAX_RANDOM_ANIM_SOUNDS		4
#define	AED_ARRAY_SIZE				(MAX_RANDOM_ANIM_SOUNDS+3)
//indices for AEV_SOUND data
#define	AED_SOUNDINDEX_START		0
#define	AED_SOUNDINDEX_END			(MAX_RANDOM_ANIM_SOUNDS-1)
#define	AED_SOUND_NUMRANDOMSNDS		(MAX_RANDOM_ANIM_SOUNDS)
#define	AED_SOUND_PROBABILITY		(MAX_RANDOM_ANIM_SOUNDS+1)
//indices for AEV_SOUNDCHAN data
#define	AED_SOUNDCHANNEL			(MAX_RANDOM_ANIM_SOUNDS+2)
//indices for AEV_FOOTSTEP data
#define	AED_FOOTSTEP_TYPE			0
#define	AED_FOOTSTEP_PROBABILITY	1
//indices for AEV_EFFECT data
#define	AED_EFFECTINDEX				0
#define	AED_BOLTINDEX				1
#define	AED_EFFECT_PROBABILITY		2
#define	AED_MODELINDEX				3
//indices for AEV_FIRE data
#define	AED_FIRE_ALT				0
#define	AED_FIRE_PROBABILITY		1
//indices for AEV_MOVE data
#define	AED_MOVE_FWD				0
#define	AED_MOVE_RT					1
#define	AED_MOVE_UP					2
//indices for AEV_SABER_SWING data
#define	AED_SABER_SWING_SABERNUM	0
#define	AED_SABER_SWING_TYPE		1
#define	AED_SABER_SWING_PROBABILITY	2
//indices for AEV_SABER_SPIN data
#define	AED_SABER_SPIN_SABERNUM		0
#define	AED_SABER_SPIN_TYPE			1	//0 = saberspinoff, 1 = saberspin, 2-4 = saberspin1-saberspin3
#define	AED_SABER_SPIN_PROBABILITY	2	

//Handling dynamic pointered sounds
//AED_SOUNDINDEX_START must = 0 (This is the dynamic pointered sound toggle)
#define AED_CSOUND_RANDSTART		1

typedef enum
{//NOTENOTE:  Be sure to update animEventTypeTable and ParseAnimationEvtBlock(...) if you change this enum list!
	AEV_NONE,
	AEV_SOUND,		//# animID AEV_SOUND framenum soundpath randomlow randomhi chancetoplay
	AEV_FOOTSTEP,	//# animID AEV_FOOTSTEP framenum footstepType chancetoplay
	AEV_EFFECT,		//# animID AEV_EFFECT framenum effectpath boltName chancetoplay
	AEV_FIRE,		//# animID AEV_FIRE framenum altfire chancetofire
	AEV_MOVE,		//# animID AEV_MOVE framenum forwardpush rightpush uppush
	AEV_SOUNDCHAN,  //# animID AEV_SOUNDCHAN framenum CHANNEL soundpath randomlow randomhi chancetoplay 
	AEV_SABER_SWING,  //# animID AEV_SABER_SWING framenum CHANNEL randomlow randomhi chancetoplay 
	AEV_SABER_SPIN,  //# animID AEV_SABER_SPIN framenum CHANNEL chancetoplay 
	//[AMBIENTEV]
	AEV_AMBIENT,
	//[AMBIENTEV]
	AEV_NUM_AEV
} animEventType_t;

typedef struct animevent_s 
{
	animEventType_t	eventType;
	unsigned short	keyFrame;			//Frame to play event on
	signed short	eventData[AED_ARRAY_SIZE];	//Unique IDs, can be soundIndex of sound file to play OR effect index or footstep type, etc.
	char			*stringData;		//we allow storage of one string, temporarily (in case we have to look up an index later, then make sure to set stringData to NULL so we only do the look-up once)
	//[AMBIENTEV]
	//I wanted to make sure the new ambient timers were big enough so I made them seperate
	//ints
	//Ambient Interval Time
	int				ambtime;
	//Ambient Random Factor
	int				ambrandom;
} animevent_t;

typedef struct
{
	char			filename[MAX_QPATH];
	animation_t		*anims;
//	animsounds_t	torsoAnimSnds[MAX_ANIM_SOUNDS];
//	animsounds_t	legsAnimSnds[MAX_ANIM_SOUNDS];
//	qboolean		soundsCached;
} cgLoadedAnim_t;

typedef struct
{
	char			filename[MAX_QPATH];
	animevent_t		torsoAnimEvents[MAX_ANIM_EVENTS];
	animevent_t		legsAnimEvents[MAX_ANIM_EVENTS];
	qboolean		eventsParsed;
} cgLoadedEvents_t;

stringID_table_t animEventTypeTable[MAX_ANIM_EVENTS+1] = 
{
	ENUM2STRING(AEV_SOUND),			//# animID AEV_SOUND framenum soundpath randomlow randomhi chancetoplay
	ENUM2STRING(AEV_FOOTSTEP),		//# animID AEV_FOOTSTEP framenum footstepType
	ENUM2STRING(AEV_EFFECT),		//# animID AEV_EFFECT framenum effectpath boltName
	ENUM2STRING(AEV_FIRE),			//# animID AEV_FIRE framenum altfire chancetofire
	ENUM2STRING(AEV_MOVE),			//# animID AEV_MOVE framenum forwardpush rightpush uppush
	ENUM2STRING(AEV_SOUNDCHAN),		//# animID AEV_SOUNDCHAN framenum CHANNEL soundpath randomlow randomhi chancetoplay 
	ENUM2STRING(AEV_SABER_SWING),	//# animID AEV_SABER_SWING framenum CHANNEL randomlow randomhi chancetoplay 
	ENUM2STRING(AEV_SABER_SPIN),	//# animID AEV_SABER_SPIN framenum CHANNEL chancetoplay 
	//[AMBIENTEV]
	ENUM2STRING(AEV_AMBIENT),
	//must be terminated
	NULL,-1
};

stringID_table_t footstepTypeTable[NUM_FOOTSTEP_TYPES+1] = 
{
	ENUM2STRING(FOOTSTEP_R),
	ENUM2STRING(FOOTSTEP_L),
	ENUM2STRING(FOOTSTEP_HEAVY_R),
	ENUM2STRING(FOOTSTEP_HEAVY_L),
	//must be terminated
	NULL,-1
};

int CheckAnimFrameForEventType( animevent_t *animEvents, int keyFrame, animEventType_t eventType )
{
	int i;

	for ( i = 0; i < MAX_ANIM_EVENTS; i++ )
	{
		//[AMBIENTEV]
		if( eventType == AEV_AMBIENT )
		{//find the ambient animevent
			if ( animEvents[i].eventType == AEV_AMBIENT )
				return i;
		}
		else if ( animEvents[i].keyFrame == keyFrame )
		//if ( animEvents[i].keyFrame == keyFrame )
		//[AMBIENTEV]
		{//there is an animevent on this frame already
			if ( animEvents[i].eventType == eventType )
			{//and it is of the same type
				return i;
			}
		}
	}
	//nope
	return -1;
}

void ParseAnimationEvtBlock(const char *aeb_filename, animevent_t *animEvents, animation_t *animations, int *i,const char **text_p) 
{
	const char		*token;
	int				num, n, animNum, keyFrame, lowestVal, highestVal, curAnimEvent, lastAnimEvent = 0;
	animEventType_t	eventType;
	char			stringData[MAX_QPATH];

	// get past starting bracket
	while(1) 
	{
		token = COM_Parse( text_p );
		if ( !Q_stricmp( token, "{" ) ) 
		{
			break;
		}
	}

	//NOTE: instead of a blind increment, increase the index
	//			this way if we have an event on an anim that already
	//			has an event of that type, it stomps it

	// read information for each frame
	while ( 1 ) 
	{
		if ( lastAnimEvent >= MAX_ANIM_EVENTS )
		{
			Com_Error( ERR_DROP, "ParseAnimationEvtBlock: number events in animEvent file %s > MAX_ANIM_EVENTS(%i)", aeb_filename, MAX_ANIM_EVENTS );
			return;
		}
		// Get base frame of sequence
		token = COM_Parse( text_p );
		if ( !token || !token[0]) 
		{
			break;
		}

		if ( !Q_stricmp( token, "}" ) )		// At end of block 
		{
			break;
		}

		//[AMBIENTEV]
		//Parse ambient sound
		if ( !Q_stricmp( token, "AEV_AMBIENT" ) )
		{//It's an ambient sound, do your thing.
			//AEV_AMBIENT intervals randomfactor soundpath randomlow randomhi chancetoplay
			//see if this frame already has an event of this type on it, if so, overwrite it
			curAnimEvent = CheckAnimFrameForEventType( animEvents, 0, AEV_AMBIENT);
			if ( curAnimEvent == -1 )
			{//this anim frame doesn't already have an event of this type on it
				curAnimEvent = lastAnimEvent;
			}
			animEvents[curAnimEvent].eventType = AEV_AMBIENT;

			//Lets grab the time intervals
			token = COM_Parse( text_p );
			if ( !token ) 
			{
				break;
			}
			animEvents[curAnimEvent].ambtime = atoi( token );

			//Lets grab the random factor
			token = COM_Parse( text_p );
			if ( !token ) 
			{
				break;
			}
			animEvents[curAnimEvent].ambrandom = atoi( token );

			//set sound channel
			token = COM_Parse( text_p );
			if ( !token ) 
			{
				break;
			}
			if ( stricmp( token, "CHAN_VOICE_ATTEN" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE_ATTEN;
			}
			else if ( stricmp( token, "CHAN_VOICE_GLOBAL" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE_GLOBAL;
			}
			else if ( stricmp( token, "CHAN_ANNOUNCER" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_ANNOUNCER;
			}
			else if ( stricmp( token, "CHAN_BODY" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_BODY;
			}
			else if ( stricmp( token, "CHAN_WEAPON" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_WEAPON;
			}
			else if ( stricmp( token, "CHAN_VOICE" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE;
			} 
			else
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_AUTO;
			}

			//get soundstring
			token = COM_Parse( text_p );
			if ( !token ) 
			{
				break;
			}		
			strcpy(stringData, token);
			//get lowest value
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			lowestVal = atoi( token );
			//get highest value
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			highestVal = atoi( token );
			//Now precache all the sounds
			if(lowestVal && highestVal)
			{
				//assert(highestVal - lowestVal < MAX_RANDOM_ANIM_SOUNDS);
				if ((highestVal-lowestVal) >= MAX_RANDOM_ANIM_SOUNDS)
				{
					highestVal = lowestVal + (MAX_RANDOM_ANIM_SOUNDS-1);
				}
				for ( n = lowestVal, num = AED_SOUNDINDEX_START; n <= highestVal && num <= AED_SOUNDINDEX_END; n++, num++ )
				{
					if (stringData[0] == '*' && n == lowestVal)
					{//You only need to do this once
						animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] = 0;
						animEvents[curAnimEvent].eventData[AED_CSOUND_RANDSTART] = lowestVal;
						if (!animEvents[curAnimEvent].stringData)
						{
							animEvents[curAnimEvent].stringData = (char *) BG_Alloc(MAX_QPATH);
						}
						strcpy( animEvents[curAnimEvent].stringData, stringData );
					}
					else if ( stringData[0] != '*' )
					//else
					{
						animEvents[curAnimEvent].eventData[num] = trap_S_RegisterSound( va( stringData, n ) );
					}
				}
				animEvents[curAnimEvent].eventData[AED_SOUND_NUMRANDOMSNDS] = num - 1;
			}
			else
			{
				if (stringData[0] == '*')
				{
					animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] = 0;
					animEvents[curAnimEvent].eventData[AED_CSOUND_RANDSTART] = 0;
					if (!animEvents[curAnimEvent].stringData)
					{
						animEvents[curAnimEvent].stringData = (char *) BG_Alloc(MAX_QPATH);
					}
					strcpy( animEvents[curAnimEvent].stringData, stringData );
				}
				else
				{
					animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] = trap_S_RegisterSound( stringData );
				}
			}

			//get probability
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_SOUND_PROBABILITY] = atoi( token );

			//CG_Printf( "AEV_AMBIENT %i %i %i\n", animEvents[curAnimEvent].ambtime, animEvents[curAnimEvent].ambrandom, animEvents[curAnimEvent].eventData[AED_SOUND_NUMRANDOMSNDS]);

			//bump the AnimEvent counter if this took the last number.
			if ( curAnimEvent == lastAnimEvent )
			{
				lastAnimEvent++;
			}	
			continue;
		}


		//Compare to same table as animations used 
		//	so we don't have to use actual numbers for animation first frames,
		//	just need offsets.
		//This way when animation numbers change, this table won't have to be updated,
		//	at least not much.
		animNum = GetIDForString(animTable, token);
		if(animNum == -1)
		{//Unrecognized ANIM ENUM name, or we're skipping this line, keep going till you get a good one
			Com_Printf(S_COLOR_YELLOW"WARNING: Unknown token %s in animEvent file %s\n", token, aeb_filename );
			while (token[0])
			{
				token = COM_ParseExt( text_p, qfalse );	//returns empty string when next token is EOL
			}
			continue;
		}

		if ( animations[animNum].numFrames == 0 )
		{//we don't use this anim
			Com_Printf(S_COLOR_YELLOW"WARNING: %s animevents.cfg: anim %s not used by this model\n", aeb_filename, token);
			//skip this entry
			SkipRestOfLine( text_p );
			continue;
		}

		token = COM_Parse( text_p );
		eventType = (animEventType_t)GetIDForString(animEventTypeTable, token);
		if ( eventType == AEV_NONE || eventType == -1 )
		{//Unrecognized ANIM EVENT TYOE, or we're skipping this line, keep going till you get a good one
			//Com_Printf(S_COLOR_YELLOW"WARNING: Unknown token %s in animEvent file %s\n", token, aeb_filename );
			continue;
		}

		//set our start frame
		keyFrame = animations[animNum].firstFrame;
		// Get offset to frame within sequence
		token = COM_Parse( text_p );
		if ( !token ) 
		{
			break;
		}
		keyFrame += atoi( token );

		//see if this frame already has an event of this type on it, if so, overwrite it
		curAnimEvent = CheckAnimFrameForEventType( animEvents, keyFrame, eventType );
		if ( curAnimEvent == -1 )
		{//this anim frame doesn't already have an event of this type on it
			curAnimEvent = lastAnimEvent;
		}

		//now that we know which event index we're going to plug the data into, start doing it
		animEvents[curAnimEvent].eventType = eventType;
		animEvents[curAnimEvent].keyFrame = keyFrame;

		//now read out the proper data based on the type
		switch ( animEvents[curAnimEvent].eventType )
		{
		case AEV_SOUNDCHAN:		//# animID AEV_SOUNDCHAN framenum CHANNEL soundpath randomlow randomhi chancetoplay
			token = COM_Parse( text_p );
			if ( !token ) 
			{
				break;
			}
			if ( stricmp( token, "CHAN_VOICE_ATTEN" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE_ATTEN;
			}
			else if ( stricmp( token, "CHAN_VOICE_GLOBAL" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE_GLOBAL;
			}
			else if ( stricmp( token, "CHAN_ANNOUNCER" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_ANNOUNCER;
			}
			else if ( stricmp( token, "CHAN_BODY" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_BODY;
			}
			else if ( stricmp( token, "CHAN_WEAPON" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_WEAPON;
			}
			else if ( stricmp( token, "CHAN_VOICE" ) == 0 )
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_VOICE;
			} 
			else
			{
				animEvents[curAnimEvent].eventData[AED_SOUNDCHANNEL] = CHAN_AUTO;
			}
			//fall through to normal sound
		case AEV_SOUND:			//# animID AEV_SOUND framenum soundpath randomlow randomhi chancetoplay
			//get soundstring
			token = COM_Parse( text_p );
			if ( !token ) 
			{
				break;
			}		
			strcpy(stringData, token);
			//get lowest value
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			lowestVal = atoi( token );
			//get highest value
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			highestVal = atoi( token );
			//Now precache all the sounds
			//NOTE: If we can be assured sequential handles, we can store sound indices
			//		instead of strings, unfortunately, if these sounds were previously
			//		registered, we cannot be guaranteed sequential indices.  Thus an array
			if(lowestVal && highestVal)
			{
				//assert(highestVal - lowestVal < MAX_RANDOM_ANIM_SOUNDS);
				if ((highestVal-lowestVal) >= MAX_RANDOM_ANIM_SOUNDS)
				{
					highestVal = lowestVal + (MAX_RANDOM_ANIM_SOUNDS-1);
				}
				for ( n = lowestVal, num = AED_SOUNDINDEX_START; n <= highestVal && num <= AED_SOUNDINDEX_END; n++, num++ )
				{
					if (stringData[0] == '*' && n == lowestVal)
					{//You only need to do this once
						animEvents[curAnimEvent].eventData[num] = 0;
						animEvents[curAnimEvent].eventData[AED_CSOUND_RANDSTART] = lowestVal;
						if (!animEvents[curAnimEvent].stringData)
						{ //eh, whatever. no dynamic stuff, so this will do.
							animEvents[curAnimEvent].stringData = (char *) BG_Alloc(MAX_QPATH);
						}
						strcpy( animEvents[curAnimEvent].stringData, stringData );
					}
					else if ( stringData[0] != '*' )
					//else
					{
						animEvents[curAnimEvent].eventData[num] = trap_S_RegisterSound( va( stringData, n ) );
					}
				}
				animEvents[curAnimEvent].eventData[AED_SOUND_NUMRANDOMSNDS] = num - 1;
			}
			else
			{
				if (stringData[0] == '*')
				{
					animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] = 0;
					animEvents[curAnimEvent].eventData[AED_CSOUND_RANDSTART] = 0;
					if (!animEvents[curAnimEvent].stringData)
					{ //eh, whatever. no dynamic stuff, so this will do.
						animEvents[curAnimEvent].stringData = (char *) BG_Alloc(MAX_QPATH);
					}
					strcpy( animEvents[curAnimEvent].stringData, stringData );
				}
				else
				{
					animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] = trap_S_RegisterSound( stringData );
				}
#ifndef FINAL_BUILD
				if ( !animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] && stringData[0] != '*')
				{//couldn't register it - file not found
					Com_Printf( S_COLOR_RED "ParseAnimationEvtBlock: sound %s does not exist (animevents.cfg %s)!\n", stringData, aeb_filename );
					Com_Printf( S_COLOR_RED "%s!\n", animEvents[curAnimEvent].eventData[AED_SOUNDINDEX_START] );
				}
#endif
				animEvents[curAnimEvent].eventData[AED_SOUND_NUMRANDOMSNDS] = 0;
			}
			//get probability
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_SOUND_PROBABILITY] = atoi( token );

			//last part - cheat and check and see if it's a special overridable saber sound we know of...
			if ( !Q_stricmpn( "sound/weapons/saber/saberhup", stringData, 28 ) )
			{//a saber swing
				animEvents[curAnimEvent].eventType = AEV_SABER_SWING;
				animEvents[curAnimEvent].eventData[AED_SABER_SWING_SABERNUM] = 0;//since we don't know which one they meant if we're hacking this, always use first saber
				animEvents[curAnimEvent].eventData[AED_SABER_SWING_PROBABILITY] = animEvents[curAnimEvent].eventData[AED_SOUND_PROBABILITY];
				if ( lowestVal < 4 )
				{//fast swing
					animEvents[curAnimEvent].eventData[AED_SABER_SWING_TYPE] = 0;//SWING_FAST;
				}
				else if ( lowestVal < 7 )
				{//medium swing
					animEvents[curAnimEvent].eventData[AED_SABER_SWING_TYPE] = 1;//SWING_MEDIUM;
				}
				else
				{//strong swing
					animEvents[curAnimEvent].eventData[AED_SABER_SWING_TYPE] = 2;//SWING_STRONG;
				}
			}
			else if ( !Q_stricmpn( "sound/weapons/saber/saberspin", stringData, 29 ) )
			{//a saber spin
				animEvents[curAnimEvent].eventType = AEV_SABER_SPIN;
				animEvents[curAnimEvent].eventData[AED_SABER_SPIN_SABERNUM] = 0;//since we don't know which one they meant if we're hacking this, always use first saber
				animEvents[curAnimEvent].eventData[AED_SABER_SPIN_PROBABILITY] = animEvents[curAnimEvent].eventData[AED_SOUND_PROBABILITY];
				if ( stringData[29] == 'o' )
				{//saberspinoff
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 0;
				}
				else if ( stringData[29] == '1' )
				{//saberspin1
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 2;
				}
				else if ( stringData[29] == '2' )
				{//saberspin2
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 3;
				}
				else if ( stringData[29] == '3' )
				{//saberspin3
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 4;
				}
				else if ( stringData[29] == '%' )
				{//saberspin%d
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 5;
				}
				else
				{//just plain saberspin
					animEvents[curAnimEvent].eventData[AED_SABER_SPIN_TYPE] = 1;
				}
			}
			break;
		case AEV_FOOTSTEP:		//# animID AEV_FOOTSTEP framenum footstepType
			//get footstep type
			token = COM_Parse( text_p );
			if ( !token ) 
			{
				break;
			}		
			animEvents[curAnimEvent].eventData[AED_FOOTSTEP_TYPE] = GetIDForString(footstepTypeTable, token);
			//get probability
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_FOOTSTEP_PROBABILITY] = atoi( token );
			break;
		case AEV_EFFECT:		//# animID AEV_EFFECT framenum effectpath boltName
			//get effect index
			token = COM_Parse( text_p );
			if ( !token ) 
			{
				break;
			}
			animEvents[curAnimEvent].eventData[AED_EFFECTINDEX] = trap_FX_RegisterEffect( token );
			//get bolt index
			token = COM_Parse( text_p );
			if ( !token ) 
			{
				break;
			}		
			if ( Q_stricmp( "none", token ) != 0 && Q_stricmp( "NULL", token ) != 0 )
			{//actually are specifying a bolt to use
				if (!animEvents[curAnimEvent].stringData)
				{ //eh, whatever. no dynamic stuff, so this will do.
					animEvents[curAnimEvent].stringData = (char *) BG_Alloc(2048);
				}
				strcpy(animEvents[curAnimEvent].stringData, token);
			}
			//NOTE: this string will later be used to add a bolt and store the index, as below:
			//animEvent->eventData[AED_BOLTINDEX] = gi.G2API_AddBolt( &cent->gent->ghoul2[cent->gent->playerModel], animEvent->stringData );
			//get probability
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_EFFECT_PROBABILITY] = atoi( token );
			break;
		case AEV_FIRE:			//# animID AEV_FIRE framenum altfire chancetofire
			//get altfire
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_FIRE_ALT] = atoi( token );
			//get probability
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_FIRE_PROBABILITY] = atoi( token );
			break;
		case AEV_MOVE:			//# animID AEV_MOVE framenum forwardpush rightpush uppush
			//get forward push
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_MOVE_FWD] = atoi( token );
			//get right push
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_MOVE_RT] = atoi( token );
			//get upwards push
			token = COM_Parse( text_p );
			if ( !token ) 
			{//WARNING!  BAD TABLE!
				break;
			}
			animEvents[curAnimEvent].eventData[AED_MOVE_UP] = atoi( token );
			break;
		default:				//unknown?
			SkipRestOfLine( text_p );
			continue;
			break;
		}

		if ( curAnimEvent == lastAnimEvent )
		{
			lastAnimEvent++;
		}
	}	
}

/*
======================
CG_ParseAnimationEvtFile

Read a configuration file containing animation events
models/players/kyle/animevents.cfg, etc

This file's presence is not required

======================
*/
cgLoadedEvents_t cgAllEvents[MAX_ANIM_FILES];
int cgNumAnimEvents = 1;
static int cg_animParseIncluding = 0;

int CG_ParseAnimationEvtFile( char *as_filename, int animFileIndex, int eventFileIndex ) 
{
	const char	*text_p;
	int			len;
	const char	*token;
	char		text[80000];
	char		sfilename[MAX_QPATH];
	fileHandle_t	f;
	int			i, j, upper_i, lower_i;
	int				usedIndex = -1;
	animevent_t	*legsAnimEvents;
	animevent_t	*torsoAnimEvents;
	animation_t		*animations;
	int				forcedIndex;
	qboolean		triedbaseskelevents = qfalse;
	
	assert(animFileIndex < MAX_ANIM_FILES);
	assert(eventFileIndex < MAX_ANIM_FILES);

	if ( animFileIndex < 0 || animFileIndex >= MAX_ANIM_FILES || eventFileIndex >= MAX_ANIM_FILES)
	{
		return 0;
	}

	if (eventFileIndex == -1)
	{
		forcedIndex = 0;
	}
	else
	{
		forcedIndex = eventFileIndex;
	}

	if (cg_animParseIncluding <= 0)
	{ //if we should be parsing an included file, skip this part
		if ( cgAllEvents[forcedIndex].eventsParsed )
		{//already cached this one
			return forcedIndex;
		}
	}

trybaseskel:

	legsAnimEvents = cgAllEvents[forcedIndex].legsAnimEvents;
	torsoAnimEvents = cgAllEvents[forcedIndex].torsoAnimEvents;
	animations = bgAllAnims[animFileIndex].anims;

	if (cg_animParseIncluding <= 0)
	{ //if we should be parsing an included file, skip this part
		//Go through and see if this filename is already in the table.
		i = 0;
		while (i < cgNumAnimEvents && forcedIndex != 0)
		{
			if (!Q_stricmp(as_filename, cgAllEvents[i].filename))
			{ //looks like we have it already.
				return i;
			}
			i++;
		}
	}

	// Load and parse animevents.cfg file
	Com_sprintf( sfilename, sizeof( sfilename ), "%sanimevents.cfg", as_filename );
	
	

	if (cg_animParseIncluding <= 0)
	{ //should already be done if we're including
		//initialize anim event array
		for( i = 0; i < MAX_ANIM_EVENTS; i++ )
		{
			//Type of event
			torsoAnimEvents[i].eventType = AEV_NONE;
			legsAnimEvents[i].eventType = AEV_NONE;
			//Frame to play event on
			torsoAnimEvents[i].keyFrame = -1;	
			legsAnimEvents[i].keyFrame = -1;
			//we allow storage of one string, temporarily (in case we have to look up an index later, then make sure to set stringData to NULL so we only do the look-up once)
			torsoAnimEvents[i].stringData = NULL;
			legsAnimEvents[i].stringData = NULL;
			//Unique IDs, can be soundIndex of sound file to play OR effect index or footstep type, etc.
			for ( j = 0; j < AED_ARRAY_SIZE; j++ )
			{
				torsoAnimEvents[i].eventData[j] = -1;
				legsAnimEvents[i].eventData[j] = -1;
			}
			//[AMBIENTEV]
			torsoAnimEvents[i].ambtime= 0;
			legsAnimEvents[i].ambtime = 0;


			torsoAnimEvents[i].ambrandom = 0;
			legsAnimEvents[i].ambrandom = 0;
			//[/AMBIENTEV]
		}
	}

	// load the file
	len = trap_FS_FOpenFile( sfilename, &f, FS_READ );
	if ( len <= 0 ) 
	{//no file
		if( !triedbaseskelevents )
		{//ok let's try using the base animevents for this model's skeleton.
			char	SkelName[MAX_QPATH];
			char	*slash = NULL;

			Q_strncpyz( SkelName, bgAllAnims[animFileIndex].filename, sizeof( SkelName ) );
			//Remove the animation.cfg section of the filename
			slash = strrchr( SkelName, '/' );
			if ( slash )
			{//move forward one character and then cut off the char string
				slash++;
				*slash = 0;
			}
			strcpy(as_filename, SkelName );
			
			triedbaseskelevents = qtrue;
			goto trybaseskel;
		}
		else
		{//ok, that didn't work either
			goto fin;
		}
	}
	if ( len >= sizeof( text ) - 1 ) 
	{
		trap_FS_FCloseFile(f);
#ifndef FINAL_BUILD
		Com_Error(ERR_DROP, "File %s too long\n", sfilename );
#else
		Com_Printf( "File %s too long\n", sfilename );
#endif
		goto fin;
	}


	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;
	upper_i =0;
	lower_i =0;

	// read information for batches of sounds (UPPER or LOWER)
	while ( 1 ) 
	{
		// Get base frame of sequence
		token = COM_Parse( &text_p );
		if ( !token || !token[0] ) 
		{
			break;
		}

		if ( !Q_stricmp(token,"include") )	// grab from another animevents.cfg
		{//NOTE: you REALLY should NOT do this after the main block of UPPERSOUNDS and LOWERSOUNDS
			const char	*include_filename = COM_Parse( &text_p );
			if ( include_filename != NULL )
			{
				char fullIPath[MAX_QPATH];
				strcpy(fullIPath, va("models/players/%s/", include_filename));
				cg_animParseIncluding++;
				CG_ParseAnimationEvtFile( fullIPath, animFileIndex, forcedIndex );
				cg_animParseIncluding--;
			}
		}

		if ( !Q_stricmp(token,"UPPEREVENTS") )	// A batch of upper sounds
		{
			ParseAnimationEvtBlock( as_filename, torsoAnimEvents, animations, &upper_i, &text_p ); 
		}

		else if ( !Q_stricmp(token,"LOWEREVENTS") )	// A batch of lower sounds
		{
			ParseAnimationEvtBlock( as_filename, legsAnimEvents, animations, &lower_i, &text_p ); 
		}
	}

	usedIndex = forcedIndex;
fin:
	//Mark this anim set so that we know we tried to load he sounds, don't care if the load failed
	if (cg_animParseIncluding <= 0)
	{ //if we should be parsing an included file, skip this part
		cgAllEvents[forcedIndex].eventsParsed = qtrue;
		strcpy(cgAllEvents[forcedIndex].filename, as_filename);
		if (forcedIndex)
		{
			cgNumAnimEvents++;
		}
	}

	return usedIndex;
}
//[/ANIMEVENTS]  End moving of animevent bg functions

void CG_Disintegration(centity_t *cent, refEntity_t *ent);

/*
================
CG_CustomSound

================
*/
sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName ) {
	clientInfo_t *ci;
	int			i;
	int			numCSounds = 0;
	int			numCComSounds = 0;
	int			numCExSounds = 0;
	int			numCJediSounds = 0;
	int			numCSiegeSounds = 0;
	int			numCDuelSounds = 0;
	char		lSoundName[MAX_QPATH];

	if ( soundName[0] != '*' ) {
		return trap_S_RegisterSound( soundName );
	}

	COM_StripExtension(soundName, lSoundName);

	if ( clientNum < 0 )
	{
		clientNum = 0;
	}

	if (clientNum >= MAX_CLIENTS)
	{
		ci = cg_entities[clientNum].npcClient;
	}
	else
	{
		ci = &cgs.clientinfo[ clientNum ];
	}

	if (!ci)
	{
		return 0;
	}

	for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
	{
		if (!cg_customSoundNames[i])
		{
			numCSounds = i;
			break;
		}
	}

	if (clientNum >= MAX_CLIENTS)
	{ //these are only for npc's
		for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
		{
			if (!cg_customCombatSoundNames[i])
			{
				numCComSounds = i;
				break;
			}
		}

		for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
		{
			if (!cg_customExtraSoundNames[i])
			{
				numCExSounds = i;
				break;
			}
		}

		for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
		{
			if (!cg_customJediSoundNames[i])
			{
				numCJediSounds = i;
				break;
			}
		}
	}

    if (cgs.gametype >= GT_TEAM || cg_buildScript.integer)
	{ //siege only
		for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
		{
			if (!bg_customSiegeSoundNames[i])
			{
				numCSiegeSounds = i;
				break;
			}
		}
	}

	//[AllTAUNTS]
	//Since duel taunts are now usable in any mode, the duel sounds must be used as well.
    /*
	if (cgs.gametype == GT_DUEL
		|| cgs.gametype == GT_POWERDUEL
		|| cg_buildScript.integer)
	*/
		//[/ALLTAUNTS]
	{ //Duel only
		for (i = 0; i < MAX_CUSTOM_SOUNDS; i++)
		{
			if (!cg_customDuelSoundNames[i])
			{
				numCDuelSounds = i;
				break;
			}
		}
	}

	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS ; i++ )
	{
		if ( i < numCSounds && !strcmp( lSoundName, cg_customSoundNames[i] ) )
		{
			return ci->sounds[i];
		}
		else if ( (cgs.gametype >= GT_TEAM || cg_buildScript.integer) && i < numCSiegeSounds && !strcmp( lSoundName, bg_customSiegeSoundNames[i] ) )
		{ //siege only
			return ci->siegeSounds[i];
		}
		//[ALLTAUNTS]
		//Since duel taunts are now usable in any mode, the duel sounds must be used as well.
		//else if ( (cgs.gametype == GT_DUEL || cgs.gametype == GT_POWERDUEL || cg_buildScript.integer) && i < numCDuelSounds && !strcmp( lSoundName, cg_customDuelSoundNames[i] ) )
		else if ( i < numCDuelSounds && !strcmp( lSoundName, cg_customDuelSoundNames[i] ) )
		//[/ALLTAUNTS]
		{ //siege only
			return ci->duelSounds[i];
		}
		else if ( clientNum >= MAX_CLIENTS && i < numCComSounds && !strcmp( lSoundName, cg_customCombatSoundNames[i] ) )
		{ //npc only
			return ci->combatSounds[i];
		}
		else if ( clientNum >= MAX_CLIENTS && i < numCExSounds && !strcmp( lSoundName, cg_customExtraSoundNames[i] ) )
		{ //npc only
			return ci->extraSounds[i];
		}
		else if ( clientNum >= MAX_CLIENTS && i < numCJediSounds && !strcmp( lSoundName, cg_customJediSoundNames[i] ) )
		{ //npc only
			return ci->jediSounds[i];
		}
	}

	//CG_Error( "Unknown custom sound: %s", lSoundName );
#ifndef FINAL_BUILD
	//[RACC] - This error will occur whenever there's there's a player sound type mismatch. (IE a Player trying to play a NPC sound.)
	//This does happen occasionally with animevent sounds but should not happen otherwise.
	Com_Printf( "Unknown custom sound: %s\n", lSoundName );
	//[RACC]
#endif
	return 0;
}

/*
=============================================================================

CLIENT INFO

=============================================================================
*/
#define MAX_SURF_LIST_SIZE	1024
qboolean CG_ParseSurfsFile( const char *modelName, const char *skinName, char *surfOff, char *surfOn ) 
{
	const char	*text_p;
	int			len;
	const char	*token;
	const char	*value;
	char		text[20000];
	char		sfilename[MAX_QPATH];
	fileHandle_t	f;
	int			i = 0;

	while (skinName && skinName[i])
	{
		if (skinName[i] == '|')
		{ //this is a multi-part skin, said skins do not support .surf files
			return qfalse;
		}

		i++;
	}


	// Load and parse .surf file
	Com_sprintf( sfilename, sizeof( sfilename ), "models/players/%s/model_%s.surf", modelName, skinName );

	// load the file
	len = trap_FS_FOpenFile( sfilename, &f, FS_READ );
	if ( len <= 0 ) 
	{//no file
		return qfalse;
	}
	if ( len >= sizeof( text ) - 1 ) 
	{
		Com_Printf( "File %s too long\n", sfilename );
		return qfalse;
	}

	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;

	memset( (char *)surfOff, 0, sizeof(surfOff) );
	memset( (char *)surfOn, 0, sizeof(surfOn) );

	// read information for surfOff and surfOn
	while ( 1 ) 
	{
		token = COM_ParseExt( &text_p, qtrue );
		if ( !token || !token[0] ) 
		{
			break;
		}

		// surfOff
		if ( !Q_stricmp( token, "surfOff" ) ) 
		{
			if ( COM_ParseString( &text_p, &value ) ) 
			{
				continue;
			}
			if ( surfOff && surfOff[0] )
			{
				Q_strcat( surfOff, MAX_SURF_LIST_SIZE, "," );
				Q_strcat( surfOff, MAX_SURF_LIST_SIZE, value );
			}
			else
			{
				Q_strncpyz( surfOff, value, MAX_SURF_LIST_SIZE );
			}
			continue;
		}
		
		// surfOn
		if ( !Q_stricmp( token, "surfOn" ) ) 
		{
			if ( COM_ParseString( &text_p, &value ) ) 
			{
				continue;
			}
			if ( surfOn && surfOn[0] )
			{
				Q_strcat( surfOn, MAX_SURF_LIST_SIZE, ",");
				Q_strcat( surfOn, MAX_SURF_LIST_SIZE, value );
			}
			else
			{
				Q_strncpyz( surfOn, value, MAX_SURF_LIST_SIZE );
			}
			continue;
		}
	}
	return qtrue;
}


//[TrueView]
//Warning flag for models that are incompatible with True View
qboolean	trueviewwarning = qfalse;
//[/TrueView]


/*
==========================
CG_RegisterClientModelname
==========================
*/
#include "../namespace_begin.h"
qboolean BG_IsValidCharacterModel(const char *modelName, const char *skinName);
qboolean BG_ValidateSkinForTeam( const char *modelName, char *skinName, int team, float *colors );
#include "../namespace_end.h"

static qboolean CG_RegisterClientModelname( clientInfo_t *ci, const char *modelName, const char *skinName, const char *teamName, int clientNum ) {
	int handle;
	char		afilename[MAX_QPATH];
	char		/**GLAName,*/ *slash;
	char		GLAName[MAX_QPATH];
	vec3_t	tempVec = {0,0,0};
	qboolean badModel = qfalse;
	char	surfOff[MAX_SURF_LIST_SIZE];
	char	surfOn[MAX_SURF_LIST_SIZE];
	int		checkSkin;
	char	*useSkinName;

	//[TrueView]
	//Warning flag for models that are incompatible with True View
	trueviewwarning = qfalse;
	//[/TrueView]

retryModel:
	if (badModel)
	{
		if (modelName && modelName[0])
		{
			Com_Printf("WARNING: Attempted to load an unsupported multiplayer model %s! (bad or missing bone, or missing animation sequence)\n", modelName);
		}

		modelName = "kyle";
		skinName = "default";

		badModel = qfalse;
	}

	// First things first.  If this is a ghoul2 model, then let's make sure we demolish this first.
	if (ci->ghoul2Model && trap_G2_HaveWeGhoul2Models(ci->ghoul2Model))
	{
		trap_G2API_CleanGhoul2Models(&(ci->ghoul2Model));
	}

	if (!BG_IsValidCharacterModel(modelName, skinName))
	{
		modelName = "kyle";
		skinName = "default";
	}

	if ( cgs.gametype >= GT_TEAM && !cgs.jediVmerc && cgs.gametype != GT_SIEGE )
	{ //We won't force colors for siege.
		BG_ValidateSkinForTeam( ci->modelName, ci->skinName, ci->team, ci->colorOverride );
		skinName = ci->skinName;
	}
	else
	{
		ci->colorOverride[0] = ci->colorOverride[1] = ci->colorOverride[2] = 0.0f;
	}

	if (strchr(skinName, '|'))
	{//three part skin
		useSkinName = va("models/players/%s/|%s", modelName, skinName);
	}
	else
	{
		useSkinName = va("models/players/%s/model_%s.skin", modelName, skinName);
	}

	checkSkin = trap_R_RegisterSkin(useSkinName);

	if (checkSkin)
	{
		ci->torsoSkin = checkSkin;
	}
	else
	{ //fallback to the default skin
		ci->torsoSkin = trap_R_RegisterSkin(va("models/players/%s/model_default.skin", modelName, skinName));
	}
	Com_sprintf( afilename, sizeof( afilename ), "models/players/%s/model.glm", modelName );
	handle = trap_G2API_InitGhoul2Model(&ci->ghoul2Model, afilename, 0, ci->torsoSkin, 0, 0, 0);

	if (handle<0)
	{
		return qfalse;
	}

	// The model is now loaded.

	trap_G2API_SetSkin(ci->ghoul2Model, 0, ci->torsoSkin, ci->torsoSkin);

	GLAName[0] = 0;

	trap_G2API_GetGLAName( ci->ghoul2Model, 0, GLAName);
	if (GLAName[0] != 0)
	{
		if (!strstr(GLAName, "players/_humanoid/") /*&&
			(!strstr(GLAName, "players/rockettrooper/") || cgs.gametype != GT_SIEGE)*/) //only allow rockettrooper in siege
		{ //Bad!
			badModel = qtrue;
			goto retryModel;
		}
	}

	if (!BGPAFtextLoaded)
	{
		if (GLAName[0] == 0/*GLAName == NULL*/)
		{
			badModel = qtrue;
			goto retryModel;
		}
		Q_strncpyz( afilename, GLAName, sizeof( afilename ));
		slash = Q_strrchr( afilename, '/' );
		if ( slash )
		{
			strcpy(slash, "/animation.cfg");
		}	// Now afilename holds just the path to the animation.cfg
		else 
		{	// Didn't find any slashes, this is a raw filename right in base (whish isn't a good thing)
			return qfalse;
		}

		//rww - All player models must use humanoid, no matter what.
		if (Q_stricmp(afilename, "models/players/_humanoid/animation.cfg") /*&&
			Q_stricmp(afilename, "models/players/rockettrooper/animation.cfg")*/)
		{
			Com_Printf( "Model does not use supported animation config.\n");
			return qfalse;
		}
		else if (BG_ParseAnimationFile("models/players/_humanoid/animation.cfg", bgHumanoidAnimations, qtrue) == -1)
		{
			Com_Printf( "Failed to load animation file models/players/_humanoid/animation.cfg\n" );
			return qfalse;
		}

		//[ANIMEVENTS]
		CG_ParseAnimationEvtFile( "models/players/_humanoid/", 0, -1 ); //get the sounds for the humanoid anims
		//BG_ParseAnimationEvtFile( "models/players/_humanoid/", 0, -1 ); //get the sounds for the humanoid anims
		//[/ANIMEVENTS]
//		if (cgs.gametype == GT_SIEGE)
//		{
//			BG_ParseAnimationEvtFile( "models/players/rockettrooper/", 1, 1 ); //parse rockettrooper too
//		}
		//For the time being, we're going to have all real players use the generic humanoid soundset and that's it.
		//Only npc's will use model-specific soundsets.

	//	BG_ParseAnimationSndFile(va("models/players/%s/", modelName), 0, -1);
	}
	//[ANIMEVENTS]
	else if (!cgAllEvents[0].eventsParsed)
	//else if (!bgAllEvents[0].eventsParsed)
	{ //make sure the player anim sounds are loaded even if the anims already are
		CG_ParseAnimationEvtFile( "models/players/_humanoid/", 0, -1 );
		//BG_ParseAnimationEvtFile( "models/players/_humanoid/", 0, -1 );
	//[/ANIMEVENTS]
//		if (cgs.gametype == GT_SIEGE)
//		{
//			BG_ParseAnimationEvtFile( "models/players/rockettrooper/", 1, 1 );
//		}
	}

	if ( CG_ParseSurfsFile( modelName, skinName, surfOff, surfOn ) )
	{//turn on/off any surfs
		const char	*token;
		const char	*p;

		//Now turn on/off any surfaces
		if ( surfOff && surfOff[0] )
		{
			p = surfOff;
			while ( 1 ) 
			{
				token = COM_ParseExt( &p, qtrue );
				if ( !token[0] ) 
				{//reached end of list
					break;
				}
				//turn off this surf
				trap_G2API_SetSurfaceOnOff( ci->ghoul2Model, token, 0x00000002/*G2SURFACEFLAG_OFF*/ );
			}
		}
		if ( surfOn && surfOn[0] )
		{
			p = surfOn;
			while ( 1 )
			{
				token = COM_ParseExt( &p, qtrue );
				if ( !token[0] ) 
				{//reached end of list
					break;
				}
				//turn on this surf
				trap_G2API_SetSurfaceOnOff( ci->ghoul2Model, token, 0 );
			}
		}
	}


	ci->bolt_rhand = trap_G2API_AddBolt(ci->ghoul2Model, 0, "*r_hand");
	
	if (!trap_G2API_SetBoneAnim(ci->ghoul2Model, 0, "model_root", 0, 12, BONE_ANIM_OVERRIDE_LOOP, 1.0f, cg.time, -1, -1))
	{
		badModel = qtrue;
	}
	
	if (!trap_G2API_SetBoneAngles(ci->ghoul2Model, 0, "upper_lumbar", tempVec, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, cg.time))
	{
		badModel = qtrue;
	}

	if (!trap_G2API_SetBoneAngles(ci->ghoul2Model, 0, "cranium", tempVec, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_Y, POSITIVE_X, NULL, 0, cg.time))
	{
		badModel = qtrue;
	}

	ci->bolt_lhand = trap_G2API_AddBolt(ci->ghoul2Model, 0, "*l_hand");

	//rhand must always be first bolt. lhand always second. Whichever you want the
	//jetpack bolted to must always be third.
	trap_G2API_AddBolt(ci->ghoul2Model, 0, "*chestg");

	//claw bolts
	trap_G2API_AddBolt(ci->ghoul2Model, 0, "*r_hand_cap_r_arm");
	trap_G2API_AddBolt(ci->ghoul2Model, 0, "*l_hand_cap_l_arm");

	ci->bolt_head = trap_G2API_AddBolt(ci->ghoul2Model, 0, "*head_top");
	if (ci->bolt_head == -1)
	{
		ci->bolt_head = trap_G2API_AddBolt(ci->ghoul2Model, 0, "ceyebrow");
	}

	ci->bolt_motion = trap_G2API_AddBolt(ci->ghoul2Model, 0, "Motion");

	//We need a lower lumbar bolt for footsteps
	ci->bolt_llumbar = trap_G2API_AddBolt(ci->ghoul2Model, 0, "lower_lumbar");


	//[VisualWeapons]
	//Initialize the holster bolts
	ci->holster_saber = trap_G2API_AddBolt(ci->ghoul2Model, 0, "*holster_saber");
	ci->saber_holstered = qfalse;

	ci->holster_saber2 = trap_G2API_AddBolt(ci->ghoul2Model, 0, "*holster_saber2");
	ci->saber2_holstered = qfalse;

	ci->holster_staff = trap_G2API_AddBolt(ci->ghoul2Model, 0, "*holster_staff");
	ci->staff_holstered = qfalse;

	ci->holster_blaster = trap_G2API_AddBolt(ci->ghoul2Model, 0, "*holster_blaster");
	ci->blaster_holstered = 0;

	ci->holster_blaster2 = trap_G2API_AddBolt(ci->ghoul2Model, 0, "*holster_blaster2");
	ci->blaster2_holstered = 0;

	ci->holster_golan = trap_G2API_AddBolt(ci->ghoul2Model, 0, "*holster_golan");
	ci->golan_holstered = qfalse;

	ci->holster_launcher = trap_G2API_AddBolt(ci->ghoul2Model, 0, "*holster_launcher");
	ci->launcher_holstered = 0;

	//offset holster bolts
	ci->bolt_rfemurYZ = trap_G2API_AddBolt(ci->ghoul2Model, 0, "rfemurYZ");
	ci->bolt_lfemurYZ = trap_G2API_AddBolt(ci->ghoul2Model, 0, "lfemurYZ");
	//[/VisualWeapons]

	if (ci->bolt_rhand == -1 || ci->bolt_lhand == -1 || ci->bolt_head == -1 || ci->bolt_motion == -1 || ci->bolt_llumbar == -1)
	{
		badModel = qtrue;
	}

	if (badModel)
	{
		goto retryModel;
	}

	if (!Q_stricmp(modelName, "boba_fett"))
	{ //special case, turn off the jetpack surfs
		trap_G2API_SetSurfaceOnOff(ci->ghoul2Model, "torso_rjet", TURN_OFF);
		trap_G2API_SetSurfaceOnOff(ci->ghoul2Model, "torso_cjet", TURN_OFF);
		trap_G2API_SetSurfaceOnOff(ci->ghoul2Model, "torso_ljet", TURN_OFF);
	}

//	ent->s.radius = 90;

	if (clientNum != -1)
	{
		/*
		if (cg_entities[clientNum].ghoul2 && trap_G2_HaveWeGhoul2Models(cg_entities[clientNum].ghoul2))
		{
			trap_G2API_CleanGhoul2Models(&(cg_entities[clientNum].ghoul2));
		}
		trap_G2API_DuplicateGhoul2Instance(ci->ghoul2Model, &cg_entities[clientNum].ghoul2);	
		*/

		cg_entities[clientNum].ghoul2weapon = NULL;
	}

	Q_strncpyz (ci->teamName, teamName, sizeof(ci->teamName));

	// Model icon for drawing the portrait on screen
	ci->modelIcon = trap_R_RegisterShaderNoMip ( va ( "models/players/%s/icon_%s", modelName, skinName ) );
	if (!ci->modelIcon)
	{
        int i = 0;
		int j;
		char iconName[1024];
		strcpy(iconName, "icon_");
		j = strlen(iconName);
		while (skinName[i] && skinName[i] != '|' && j < 1024)
		{
            iconName[j] = skinName[i];
			j++;
			i++;
		}
		iconName[j] = 0;
		if (skinName[i] == '|')
		{ //looks like it actually may be a custom model skin, let's try getting the icon...
		//[RACC] - This appears to be relevent to the custom model icons
			ci->modelIcon = trap_R_RegisterShaderNoMip ( va ( "models/players/%s/%s", modelName, iconName ) );
		}
	}

	return qtrue;
}

/*
====================
CG_ColorFromString
====================
*/
static void CG_ColorFromString( const char *v, vec3_t color ) {
	int val;

	VectorClear( color );

	val = atoi( v );

	if ( val < 1 || val > 7 ) {
		VectorSet( color, 1, 1, 1 );
		return;
	}

	if ( val & 1 ) {
		color[2] = 1.0f;
	}
	if ( val & 2 ) {
		color[1] = 1.0f;
	}
	if ( val & 4 ) {
		color[0] = 1.0f;
	}
}

/*
====================
CG_ColorFromInt
====================
*/
static void CG_ColorFromInt( int val, vec3_t color ) {
	VectorClear( color );

	if ( val < 1 || val > 7 ) {
		VectorSet( color, 1, 1, 1 );
		return;
	}

	if ( val & 1 ) {
		color[2] = 1.0f;
	}
	if ( val & 2 ) {
		color[1] = 1.0f;
	}
	if ( val & 4 ) {
		color[0] = 1.0f;
	}
}

//load anim info
int CG_G2SkelForModel(void *g2)
{
	int animIndex = -1;
	char GLAName[MAX_QPATH];
	char *slash;

	GLAName[0] = 0;
	trap_G2API_GetGLAName(g2, 0, GLAName);

	slash = Q_strrchr( GLAName, '/' );
	if ( slash )
	{
		strcpy(slash, "/animation.cfg");

		animIndex = BG_ParseAnimationFile(GLAName, NULL, qfalse);
	}

	return animIndex;
}

//[ANIMEVENTS]
//This function is no longer nessicary.  CG_ParseAnimationEvtFile automatically does 
//this if it can't find the initally suggested animevent.cfg

/*
//get the appropriate anim events file index
int CG_G2EvIndexForModel(void *g2, int animIndex)
{
	int evtIndex = -1;
	char GLAName[MAX_QPATH];
	char *slash;

	if (animIndex == -1)
	{
		assert(!"shouldn't happen, bad animIndex");
		return -1;
	}

	GLAName[0] = 0;
	trap_G2API_GetGLAName(g2, 0, GLAName);

	slash = Q_strrchr( GLAName, '/' );
	if ( slash )
	{
		slash++;
		*slash = 0;

		evtIndex = BG_ParseAnimationEvtFile(GLAName, animIndex, bgNumAnimEvents);
	}

	return evtIndex;
}
*/
//[/ANIMEVENTS]


#define DEFAULT_FEMALE_SOUNDPATH "chars/mp_generic_female/misc"//"chars/tavion/misc"
#define DEFAULT_MALE_SOUNDPATH "chars/mp_generic_male/misc"//"chars/kyle/misc"
void CG_LoadCISounds(clientInfo_t *ci, qboolean modelloaded)
{
	fileHandle_t f;
	qboolean	isFemale = qfalse;
	int			i = 0;
	int			fLen = 0;
	const char	*dir;
	char		soundpath[MAX_QPATH];
	char		soundName[1024];
	const char	*s;

	dir = ci->modelName;

	if ( !ci->skinName || !Q_stricmp( "default", ci->skinName ) )
	{//try default sounds.cfg first
		fLen = trap_FS_FOpenFile(va("models/players/%s/sounds.cfg", dir), &f, FS_READ);
		if ( !f ) 
		{//no?  Look for _default sounds.cfg
			fLen = trap_FS_FOpenFile(va("models/players/%s/sounds_default.cfg", dir), &f, FS_READ);
		}
	}
	else
	{//use the .skin associated with this skin
		fLen = trap_FS_FOpenFile(va("models/players/%s/sounds_%s.cfg", dir, ci->skinName), &f, FS_READ);
		if ( !f ) 
		{//fall back to default sounds
			fLen = trap_FS_FOpenFile(va("models/players/%s/sounds.cfg", dir), &f, FS_READ);
		}
	}

	soundpath[0] = 0;

	if (f)
	{
		trap_FS_Read(soundpath, fLen, f);
		soundpath[fLen] = 0;

		i = fLen;

		while (i >= 0 && soundpath[i] != '\n')
		{
			if (soundpath[i] == 'f')
			{
				isFemale = qtrue;
				soundpath[i] = 0;
			}

			i--;
		}

		i = 0;

		while (soundpath[i] && soundpath[i] != '\r' && soundpath[i] != '\n')
		{
			i++;
		}
		soundpath[i] = 0;

		trap_FS_FCloseFile(f);
	}

	if (isFemale)
	{
		ci->gender = GENDER_FEMALE;
	}
	else
	{
		ci->gender = GENDER_MALE;
	}

	trap_S_ShutUp(qtrue);

	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS ; i++ )
	{
		s = cg_customSoundNames[i];
		if ( !s ) {
			break;
		}

		Com_sprintf(soundName, sizeof(soundName), "%s", s+1);
		COM_StripExtension(soundName, soundName);
		//strip the extension because we might want .mp3's

		ci->sounds[i] = 0;
		// if the model didn't load use the sounds of the default model
		if (soundpath[0])
		{
			ci->sounds[i] = trap_S_RegisterSound( va("sound/chars/%s/misc/%s", soundpath, soundName) );
		}
		else
		{
			if (modelloaded)
			{
				ci->sounds[i] = trap_S_RegisterSound( va("sound/chars/%s/misc/%s", dir, soundName) );
			}
		}

		if (!ci->sounds[i])
		{ //failed the load, try one out of the generic path
			if (isFemale)
			{
				ci->sounds[i] = trap_S_RegisterSound( va("sound/%s/%s", DEFAULT_FEMALE_SOUNDPATH, soundName) );
			}
			else
			{
				ci->sounds[i] = trap_S_RegisterSound( va("sound/%s/%s", DEFAULT_MALE_SOUNDPATH, soundName) );
			}
		}
	}

	if (cgs.gametype >= GT_TEAM || cg_buildScript.integer)
	{ //load the siege sounds then
		for ( i = 0 ; i < MAX_CUSTOM_SIEGE_SOUNDS; i++ )
		{
			s = bg_customSiegeSoundNames[i];
			if ( !s )
			{
				break;
			}

			Com_sprintf(soundName, sizeof(soundName), "%s", s+1);
			COM_StripExtension(soundName, soundName);
			//strip the extension because we might want .mp3's

			ci->siegeSounds[i] = 0;
			// if the model didn't load use the sounds of the default model
			if (soundpath[0])
			{
				ci->siegeSounds[i] = trap_S_RegisterSound( va("sound/%s/%s", soundpath, soundName) );
			}
			else
			{
				if (modelloaded)
				{
					ci->siegeSounds[i] = trap_S_RegisterSound( va("sound/chars/%s/misc/%s", dir, soundName) );
				}
			}

			if (!ci->siegeSounds[i])
			{ //failed the load, try one out of the generic path
				if (isFemale)
				{
					ci->siegeSounds[i] = trap_S_RegisterSound( va("sound/%s/%s", DEFAULT_FEMALE_SOUNDPATH, soundName) );
				}
				else
				{
					ci->siegeSounds[i] = trap_S_RegisterSound( va("sound/%s/%s", DEFAULT_MALE_SOUNDPATH, soundName) );
				}
			}
		}
	}

	//[ALLTAUNT]
	//Since duel taunts are now usable in any mode, the sounds must be loaded as well.
	/*
	if (cgs.gametype == GT_DUEL
		||cgs.gametype == GT_POWERDUEL
		|| cg_buildScript.integer)
	*/
	//[/ALLTAUNT]
	{ //load the Duel sounds then
		for ( i = 0 ; i < MAX_CUSTOM_DUEL_SOUNDS; i++ )
		{
			s = cg_customDuelSoundNames[i];
			if ( !s )
			{
				break;
			}

			Com_sprintf(soundName, sizeof(soundName), "%s", s+1);
			COM_StripExtension(soundName, soundName);
			//strip the extension because we might want .mp3's

			ci->duelSounds[i] = 0;
			// if the model didn't load use the sounds of the default model
			if (soundpath[0])
			{
				ci->duelSounds[i] = trap_S_RegisterSound( va("sound/chars/%s/misc/%s", soundpath, soundName) );
			}
			else
			{
				if (modelloaded)
				{
					ci->duelSounds[i] = trap_S_RegisterSound( va("sound/chars/%s/misc/%s", dir, soundName) );
				}
			}

			if (!ci->duelSounds[i])
			{ //failed the load, try one out of the generic path
				if (isFemale)
				{
					ci->duelSounds[i] = trap_S_RegisterSound( va("sound/%s/%s", DEFAULT_FEMALE_SOUNDPATH, soundName) );
				}
				else
				{
					ci->duelSounds[i] = trap_S_RegisterSound( va("sound/%s/%s", DEFAULT_MALE_SOUNDPATH, soundName) );
				}
			}
		}
	}

	trap_S_ShutUp(qfalse);
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
//[VisualWeapons]
void CG_LoadHolsterData (clientInfo_t *ci);
//[/VisualWeapons]
void CG_LoadClientInfo( clientInfo_t *ci ) {
	qboolean	modelloaded;
	int			clientNum;
	int			i;
	char		teamname[MAX_QPATH];

	clientNum = ci - cgs.clientinfo;

	if (clientNum < 0 || clientNum >= MAX_CLIENTS)
	{
		clientNum = -1;
	}

	ci->deferred = qfalse;

	/*
	if (ci->team == TEAM_SPECTATOR)
	{
		// reset any existing players and bodies, because they might be in bad
		// frames for this new model
		clientNum = ci - cgs.clientinfo;
		for ( i = 0 ; i < MAX_GENTITIES ; i++ ) {
			if ( cg_entities[i].currentState.clientNum == clientNum
				&& cg_entities[i].currentState.eType == ET_PLAYER ) {
				CG_ResetPlayerEntity( &cg_entities[i] );
			}
		}

		if (ci->ghoul2Model && trap_G2_HaveWeGhoul2Models(ci->ghoul2Model))
		{
			trap_G2API_CleanGhoul2Models(&ci->ghoul2Model);
		}

		return;
	}
	*/

	teamname[0] = 0;
	if( cgs.gametype >= GT_TEAM) {
		if( ci->team == TEAM_BLUE ) {
			Q_strncpyz(teamname, DEFAULT_BLUETEAM_NAME/*cg_blueTeamName.string*/, sizeof(teamname) );
		} else {
			Q_strncpyz(teamname, DEFAULT_REDTEAM_NAME/*cg_redTeamName.string*/, sizeof(teamname) );
		}
	}
	if( teamname[0] ) {
		strcat( teamname, "/" );
	}
	modelloaded = qtrue;
	if (cgs.gametype == GT_SIEGE &&
		(ci->team == TEAM_SPECTATOR || ci->siegeIndex == -1))
	{ //yeah.. kind of a hack I guess. Don't care until they are actually ingame with a valid class.
		if ( !CG_RegisterClientModelname( ci, DEFAULT_MODEL, "default", teamname, -1 ) )
		{
			CG_Error( "DEFAULT_MODEL (%s) failed to register", DEFAULT_MODEL );
		}
	}
	else
	{
		if ( !CG_RegisterClientModelname( ci, ci->modelName, ci->skinName, teamname, clientNum ) ) {
			//CG_Error( "CG_RegisterClientModelname( %s, %s, %s, %s %s ) failed", ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, teamname );
			//rww - DO NOT error out here! Someone could just type in a nonsense model name and crash everyone's client.
			//Give it a chance to load default model for this client instead.

			// fall back to default team name
			if( cgs.gametype >= GT_TEAM) {
				// keep skin name
				if( ci->team == TEAM_BLUE ) {
					Q_strncpyz(teamname, DEFAULT_BLUETEAM_NAME, sizeof(teamname) );
				} else {
					Q_strncpyz(teamname, DEFAULT_REDTEAM_NAME, sizeof(teamname) );
				}
				if ( !CG_RegisterClientModelname( ci, DEFAULT_MODEL, ci->skinName, teamname, -1 ) ) {
					CG_Error( "DEFAULT_MODEL / skin (%s/%s) failed to register", DEFAULT_MODEL, ci->skinName );
				}
			} else {
				if ( !CG_RegisterClientModelname( ci, DEFAULT_MODEL, "default", teamname, -1 ) ) {
					CG_Error( "DEFAULT_MODEL (%s) failed to register", DEFAULT_MODEL );
				}
			}
			modelloaded = qfalse;
		}
	}

	if (clientNum != -1)
	{
		trap_G2API_ClearAttachedInstance(clientNum);
	}

	if (clientNum != -1 && ci->ghoul2Model && trap_G2_HaveWeGhoul2Models(ci->ghoul2Model))
	{
		if (cg_entities[clientNum].ghoul2 && trap_G2_HaveWeGhoul2Models(cg_entities[clientNum].ghoul2))
		{
			trap_G2API_CleanGhoul2Models(&cg_entities[clientNum].ghoul2);
		}
		trap_G2API_DuplicateGhoul2Instance(ci->ghoul2Model, &cg_entities[clientNum].ghoul2);

		//Attach the instance to this entity num so we can make use of client-server
		//shared operations if possible.
		trap_G2API_AttachInstanceToEntNum(cg_entities[clientNum].ghoul2, clientNum, qfalse);


		if (trap_G2API_AddBolt(cg_entities[clientNum].ghoul2, 0, "face") == -1)
		{ //check now to see if we have this bone for setting anims and such
			cg_entities[clientNum].noFace = qtrue;
		}

		cg_entities[clientNum].localAnimIndex = CG_G2SkelForModel(cg_entities[clientNum].ghoul2);
		//[ANIMEVENTS]
		cg_entities[clientNum].eventAnimIndex = CG_ParseAnimationEvtFile( va("models/players/%s/", ci->modelName), cg_entities[clientNum].localAnimIndex, cgNumAnimEvents );
		//cg_entities[clientNum].eventAnimIndex = CG_G2EvIndexForModel(cg_entities[clientNum].ghoul2, cg_entities[clientNum].localAnimIndex);
		//[/ANIMEVENTS]
	}

	ci->newAnims = qfalse;
	if ( ci->torsoModel ) {
		orientation_t tag;
		// if the torso model has the "tag_flag"
		if ( trap_R_LerpTag( &tag, ci->torsoModel, 0, 0, 1, "tag_flag" ) ) {
			ci->newAnims = qtrue;
		}
	}

	// sounds
	if (cgs.gametype == GT_SIEGE &&
		(ci->team == TEAM_SPECTATOR || ci->siegeIndex == -1))
	{ //don't need to load sounds
	}
	else
	{
		CG_LoadCISounds(ci, modelloaded);
		//[VisualWeapons]
		CG_LoadHolsterData(ci); //initialize our manual holster offset data.
		//[/VisualWeapons]
	}

	ci->deferred = qfalse;

	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	clientNum = ci - cgs.clientinfo;
	for ( i = 0 ; i < MAX_GENTITIES ; i++ ) {
		if ( cg_entities[i].currentState.clientNum == clientNum
			&& cg_entities[i].currentState.eType == ET_PLAYER ) {
			CG_ResetPlayerEntity( &cg_entities[i] );
		}
	}
}


//Take care of initializing all the ghoul2 saber stuff based on clientinfo data. -rww
static void CG_InitG2SaberData(int saberNum, clientInfo_t *ci)
{
	trap_G2API_InitGhoul2Model(&ci->ghoul2Weapons[saberNum], ci->saber[saberNum].model, 0, ci->saber[saberNum].skin, 0, 0, 0);

	if (ci->ghoul2Weapons[saberNum])
	{
		int k = 0;
		int tagBolt;
		char *tagName;

		if (ci->saber[saberNum].skin)
		{
			trap_G2API_SetSkin(ci->ghoul2Weapons[saberNum], 0, ci->saber[saberNum].skin, ci->saber[saberNum].skin);
		}

		if (ci->saber[saberNum].saberFlags & SFL_BOLT_TO_WRIST)
		{
			trap_G2API_SetBoltInfo(ci->ghoul2Weapons[saberNum], 0, 3+saberNum);
		}
		else
		{
			trap_G2API_SetBoltInfo(ci->ghoul2Weapons[saberNum], 0, saberNum);
		}

		while (k < ci->saber[saberNum].numBlades)
		{
			tagName = va("*blade%i", k+1);
			tagBolt = trap_G2API_AddBolt(ci->ghoul2Weapons[saberNum], 0, tagName);

			if (tagBolt == -1)
			{
				if (k == 0)
				{ //guess this is an 0ldsk3wl saber
					tagBolt = trap_G2API_AddBolt(ci->ghoul2Weapons[saberNum], 0, "*flash");

					if (tagBolt == -1)
					{
						assert(0);
					}
					break;
				}

				if (tagBolt == -1)
				{
					assert(0);
					break;
				}
			}

			k++;
		}
	}

	//[VisualWeapons]
	//init the holster model at the same time
	trap_G2API_InitGhoul2Model(&ci->ghoul2HolsterWeapons[saberNum], 
		ci->saber[saberNum].model, 0, ci->saber[saberNum].skin, 0, 0, 0);

	if (ci->ghoul2HolsterWeapons[saberNum])
	{
		if (ci->saber[saberNum].skin)
		{
			trap_G2API_SetSkin(ci->ghoul2HolsterWeapons[saberNum], 0, ci->saber[saberNum].skin, 
				ci->saber[saberNum].skin);
		}
	}

	//[/VisualWeapons]
}


/*
======================
CG_CopyClientInfoModel
======================
*/
static void CG_CopyClientInfoModel( clientInfo_t *from, clientInfo_t *to )
{
	VectorCopy( from->headOffset, to->headOffset );
//	to->footsteps = from->footsteps;
	to->gender = from->gender;

	to->legsModel = from->legsModel;
	to->legsSkin = from->legsSkin;
	to->torsoModel = from->torsoModel;
	to->torsoSkin = from->torsoSkin;
	//to->headModel = from->headModel;
	//to->headSkin = from->headSkin;
	to->modelIcon = from->modelIcon;

	to->newAnims = from->newAnims;

	//to->ghoul2Model = from->ghoul2Model;
	//rww - Trying to use the same ghoul2 pointer for two seperate clients == DISASTER
	assert(to->ghoul2Model != from->ghoul2Model);

	if (to->ghoul2Model && trap_G2_HaveWeGhoul2Models(to->ghoul2Model))
	{
		trap_G2API_CleanGhoul2Models(&to->ghoul2Model);
	}
	if (from->ghoul2Model && trap_G2_HaveWeGhoul2Models(from->ghoul2Model))
	{
		trap_G2API_DuplicateGhoul2Instance(from->ghoul2Model, &to->ghoul2Model);
	}

	//Don't do this, I guess. Just leave the saber info in the original, so it will be
	//properly initialized.
	/*
	strcpy(to->saberName, from->saberName);
	strcpy(to->saber2Name, from->saber2Name);

	while (i < MAX_SABERS)
	{
		if (to->ghoul2Weapons[i] && trap_G2_HaveWeGhoul2Models(to->ghoul2Weapons[i]))
		{
			trap_G2API_CleanGhoul2Models(&to->ghoul2Weapons[i]);
		}

		WP_SetSaber(to->saber, 0, to->saberName);
		WP_SetSaber(to->saber, 1, to->saber2Name);

		j = 0;

		while (j < MAX_SABERS)
		{
			if (to->saber[j].model[0])
			{
				CG_InitG2SaberData(j, to);
			}
			j++;
		}
		i++;
	}
	*/

	to->bolt_head = from->bolt_head;
	to->bolt_lhand = from->bolt_lhand;
	to->bolt_rhand = from->bolt_rhand;
	to->bolt_motion = from->bolt_motion;
	to->bolt_llumbar = from->bolt_llumbar;

	to->siegeIndex = from->siegeIndex;

	//[VisualWeapons]
	to->holster_saber = from->holster_saber;
	to->saber_holstered = from->saber_holstered;

	to->holster_saber2 = from->holster_saber2;
	to->saber2_holstered = from->saber2_holstered;

	to->holster_staff = from->holster_staff;
	to->staff_holstered = from->staff_holstered;

	to->holster_blaster = from->holster_blaster;
	to->blaster_holstered = from->blaster_holstered;

	to->holster_blaster2 = from->holster_blaster2;
	to->blaster2_holstered = from->blaster2_holstered;

	to->holster_golan = from->holster_golan;
	to->golan_holstered = from->golan_holstered;

	to->holster_launcher = from->holster_launcher;
	to->launcher_holstered = from->launcher_holstered;

	//offset bolts
	to->bolt_rfemurYZ = from->bolt_rfemurYZ;
	to->bolt_lfemurYZ = from->bolt_lfemurYZ;

	memcpy( to->holsterData, from->holsterData, sizeof( to->holsterData ) );
	//[/VisualWeapons]

	memcpy( to->sounds, from->sounds, sizeof( to->sounds ) );
	memcpy( to->siegeSounds, from->siegeSounds, sizeof( to->siegeSounds ) );
	memcpy( to->duelSounds, from->duelSounds, sizeof( to->duelSounds ) );
}

/*
======================
CG_ScanForExistingClientInfo
======================
*/
static qboolean CG_ScanForExistingClientInfo( clientInfo_t *ci, int clientNum ) {
	int		i;
	clientInfo_t	*match;

	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}
		if ( match->deferred ) {
			continue;
		}
		if ( !Q_stricmp( ci->modelName, match->modelName )
			&& !Q_stricmp( ci->skinName, match->skinName )
			&& !Q_stricmp( ci->saberName, match->saberName)
			&& !Q_stricmp( ci->saber2Name, match->saber2Name)
//			&& !Q_stricmp( ci->headModelName, match->headModelName )
//			&& !Q_stricmp( ci->headSkinName, match->headSkinName ) 
//			&& !Q_stricmp( ci->blueTeam, match->blueTeam ) 
//			&& !Q_stricmp( ci->redTeam, match->redTeam )
			&& (cgs.gametype < GT_TEAM || ci->team == match->team) 
			&& ci->siegeIndex == match->siegeIndex
			&& match->ghoul2Model
			&& match->bolt_head) //if the bolts haven't been initialized, this "match" is useless to us
		{
			// this clientinfo is identical, so use it's handles

			ci->deferred = qfalse;

			//rww - Filthy hack. If this is actually the info already belonging to us, just reassign the pointer.
			//Switching instances when not necessary produces small animation glitches.
			//Actually, before, were we even freeing the instance attached to the old clientinfo before copying
			//this new clientinfo over it? Could be a nasty leak possibility. (though this should remedy it in theory)
			if (clientNum == i)
			{
				if (match->ghoul2Model && trap_G2_HaveWeGhoul2Models(match->ghoul2Model))
				{ //The match has a valid instance (if it didn't, we'd probably already be fudged (^_^) at this state)
					if (ci->ghoul2Model && trap_G2_HaveWeGhoul2Models(ci->ghoul2Model))
					{ //First kill the copy we have if we have one. (but it should be null)
						trap_G2API_CleanGhoul2Models(&ci->ghoul2Model);
					}

					VectorCopy( match->headOffset, ci->headOffset );
//					ci->footsteps = match->footsteps;
					ci->gender = match->gender;

					ci->legsModel = match->legsModel;
					ci->legsSkin = match->legsSkin;
					ci->torsoModel = match->torsoModel;
					ci->torsoSkin = match->torsoSkin;
					ci->modelIcon = match->modelIcon;

					ci->newAnims = match->newAnims;

					ci->bolt_head = match->bolt_head;
					ci->bolt_lhand = match->bolt_lhand;
					ci->bolt_rhand = match->bolt_rhand;
					ci->bolt_motion = match->bolt_motion;
					ci->bolt_llumbar = match->bolt_llumbar;
					ci->siegeIndex = match->siegeIndex;

					//[VisualWeapons]
					ci->holster_saber = match->holster_saber;
					ci->saber_holstered = match->saber_holstered;

					ci->holster_saber2 = match->holster_saber2;
					ci->saber2_holstered = match->saber2_holstered;

					ci->holster_staff = match->holster_staff;
					ci->staff_holstered = match->staff_holstered;

					ci->holster_blaster = match->holster_blaster;
					ci->blaster_holstered = match->blaster_holstered;

					ci->holster_blaster2 = match->holster_blaster2;
					ci->blaster2_holstered = match->blaster2_holstered;

					ci->holster_golan = match->holster_golan;
					ci->golan_holstered = match->golan_holstered;

					ci->holster_launcher = match->holster_launcher;
					ci->launcher_holstered = match->launcher_holstered;

					ci->bolt_rfemurYZ = match->bolt_rfemurYZ;
					ci->bolt_lfemurYZ = match->bolt_lfemurYZ;

					memcpy( ci->holsterData, match->holsterData, sizeof( ci->holsterData ) );
					//[/VisualWeapons]
					
					memcpy( ci->sounds, match->sounds, sizeof( ci->sounds ) );
					memcpy( ci->siegeSounds, match->siegeSounds, sizeof( ci->siegeSounds ) );
					memcpy( ci->duelSounds, match->duelSounds, sizeof( ci->duelSounds ) );

					//We can share this pointer, because it already belongs to this client.
					//The pointer itself and the ghoul2 instance is never actually changed, just passed between
					//clientinfo structures.
					ci->ghoul2Model = match->ghoul2Model;

					//Don't need to do this I guess, whenever this function is called the saber stuff should
					//already be taken care of in the new info.
					/*
					while (k < MAX_SABERS)
					{
						if (match->ghoul2Weapons[k] && match->ghoul2Weapons[k] != ci->ghoul2Weapons[k])
						{
							if (ci->ghoul2Weapons[k])
							{
								trap_G2API_CleanGhoul2Models(&ci->ghoul2Weapons[k]);
							}
                            ci->ghoul2Weapons[k] = match->ghoul2Weapons[k];
						}
						k++;
					}
					*/
				}
			}
			else
			{
				CG_CopyClientInfoModel( match, ci );
			}

			return qtrue;
		}
	}

	// nothing matches, so defer the load
	return qfalse;
}

/*
======================
CG_SetDeferredClientInfo

We aren't going to load it now, so grab some other
client's info to use until we have some spare time.
======================
*/
static void CG_SetDeferredClientInfo( clientInfo_t *ci ) {
	int		i;
	clientInfo_t	*match;

	// if someone else is already the same models and skins we
	// can just load the client info
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid || match->deferred ) {
			continue;
		}
		if ( Q_stricmp( ci->skinName, match->skinName ) ||
			 Q_stricmp( ci->modelName, match->modelName ) ||
//			 Q_stricmp( ci->headModelName, match->headModelName ) ||
//			 Q_stricmp( ci->headSkinName, match->headSkinName ) ||
			 (cgs.gametype >= GT_TEAM && ci->team != match->team && ci->team != TEAM_SPECTATOR) ) {
			continue;
		}

		 /*
		if (Q_stricmp(ci->saberName, match->saberName) ||
			Q_stricmp(ci->saber2Name, match->saber2Name))
		{
			continue;
		}
		*/

		// just load the real info cause it uses the same models and skins
		CG_LoadClientInfo( ci );
		return;
	}

	// if we are in teamplay, only grab a model if the skin is correct
	if ( cgs.gametype >= GT_TEAM ) {
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			match = &cgs.clientinfo[ i ];
			if ( !match->infoValid || match->deferred ) {
				continue;
			}
			if ( ci->team != TEAM_SPECTATOR &&
				(Q_stricmp( ci->skinName, match->skinName ) ||
				 (cgs.gametype >= GT_TEAM && ci->team != match->team)) ) {
				continue;
			}

			/*
			if (Q_stricmp(ci->saberName, match->saberName) ||
				Q_stricmp(ci->saber2Name, match->saber2Name))
			{
				continue;
			}
			*/

			ci->deferred = qtrue;
			CG_CopyClientInfoModel( match, ci );
			return;
		}
		// load the full model, because we don't ever want to show
		// an improper team skin.  This will cause a hitch for the first
		// player, when the second enters.  Combat shouldn't be going on
		// yet, so it shouldn't matter
		CG_LoadClientInfo( ci );
		return;
	}

	// find the first valid clientinfo and grab its stuff
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}

		/*
		if (Q_stricmp(ci->saberName, match->saberName) ||
			Q_stricmp(ci->saber2Name, match->saber2Name))
		{
			continue;
		}
		*/

		if (match->deferred)
		{ //no deferring off of deferred info. Because I said so.
			continue;
		}

		ci->deferred = qtrue;
		CG_CopyClientInfoModel( match, ci );
		return;
	}

	// we should never get here...
	//CG_Printf( "CG_SetDeferredClientInfo: no valid clients!\n" );
	//Actually it is possible now because of the unique sabers.

	CG_LoadClientInfo( ci );
}

/*
======================
CG_NewClientInfo
======================
*/
//[RGBSabers]
void ParseRGBSaber(char * str, vec3_t c);
void CG_ParseScriptedSaber(char *script, clientInfo_t *ci, int snum);
//[/RGBSabers]

#include "../namespace_begin.h"
void WP_SetSaber( int entNum, saberInfo_t *sabers, int saberNum, const char *saberName );
#include "../namespace_end.h"

void CG_NewClientInfo( int clientNum, qboolean entitiesInitialized ) {
	clientInfo_t *ci;
	clientInfo_t newInfo;
	const char	*configstring;
	const char	*v;
	char		*slash;
//[RGBSabers]
	char		*yo,*yo2;
//[/RGBSabers]
	void *oldGhoul2;
	void *oldG2Weapons[MAX_SABERS];
	//[VisualWeapons]
	void *oldG2HolsteredWeapons[MAX_SABERS];
	//[/VisualWeapons]
	int i = 0;
	int k = 0;
	qboolean saberUpdate[MAX_SABERS];

	ci = &cgs.clientinfo[clientNum];

	oldGhoul2 = ci->ghoul2Model;

	//racc - copy all the our old saber instances into a backup area.
	while (k < MAX_SABERS)
	{
		oldG2Weapons[k] = ci->ghoul2Weapons[k];
		//[VisualWeapons]
		oldG2HolsteredWeapons[k] = ci->ghoul2HolsterWeapons[k];
		//[/VisualWeapons]
		k++;
	}

	configstring = CG_ConfigString( clientNum + CS_PLAYERS );
	if ( !configstring[0] ) 
	{//racc - bad configstring, the player probably left the server? kill our ghoul2 instances
		if (ci->ghoul2Model && trap_G2_HaveWeGhoul2Models(ci->ghoul2Model))
		{ //clean this stuff up first
			trap_G2API_CleanGhoul2Models(&ci->ghoul2Model);
		}
		k = 0;
		while (k < MAX_SABERS)
		{
			if (ci->ghoul2Weapons[k] && trap_G2_HaveWeGhoul2Models(ci->ghoul2Weapons[k]))
			{
				trap_G2API_CleanGhoul2Models(&ci->ghoul2Weapons[k]);
			}

			//[VisualWeapons]
			//racc - kill holster saber ghoul 2 instances
			if (ci->ghoul2HolsterWeapons[k] && trap_G2_HaveWeGhoul2Models(ci->ghoul2HolsterWeapons[k]))
			{
				trap_G2API_CleanGhoul2Models(&ci->ghoul2HolsterWeapons[k]);
			}
			//[/VisualWeapons]
			k++;
		}

		memset( ci, 0, sizeof( *ci ) );
		return;		// player just left
	}

	// build into a temp buffer so the defer checks can use
	// the old value
	memset( &newInfo, 0, sizeof( newInfo ) );

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz( newInfo.name, v, sizeof( newInfo.name ) );

	// colors
	v = Info_ValueForKey( configstring, "c1" );
	CG_ColorFromString( v, newInfo.color1 );

	newInfo.icolor1 = atoi(v);

	v = Info_ValueForKey( configstring, "c2" );
	CG_ColorFromString( v, newInfo.color2 );

	newInfo.icolor2 = atoi(v);

	// bot skill
	v = Info_ValueForKey( configstring, "skill" );
	newInfo.botSkill = atoi( v );

	// handicap
	v = Info_ValueForKey( configstring, "hc" );
	newInfo.handicap = atoi( v );

	// wins
	v = Info_ValueForKey( configstring, "w" );
	newInfo.wins = atoi( v );

	// losses
	v = Info_ValueForKey( configstring, "l" );
	newInfo.losses = atoi( v );

	// team
	v = Info_ValueForKey( configstring, "t" );
	newInfo.team = atoi( v );

//copy team info out to menu
	if ( clientNum == cg.clientNum)	//this is me
	{
		trap_Cvar_Set("ui_team", v);
	}

//[RGBSabers]
	yo = Info_ValueForKey(configstring,"tc1");
	ParseRGBSaber(yo,newInfo.rgb1);

	yo = Info_ValueForKey(configstring,"tc2");
	ParseRGBSaber(yo,newInfo.rgb2);

	yo = Info_ValueForKey(configstring,"ss1");
//	Com_Printf("saber1 : %s\n",yo);
	if(yo[0] != ':')
	{
		CG_ParseScriptedSaber(":255,0,255:500:0,0,255:500",&newInfo,0);
	}
	else
		CG_ParseScriptedSaber(yo,&newInfo,0);

	yo2 = Info_ValueForKey(configstring,"ss2");
//	Com_Printf("saber1 : %s\n",yo2);
	if(yo2[0] != ':')
	{
		CG_ParseScriptedSaber(":0,255,255:500:0,0,255:500",&newInfo,1);
	}
	else
		CG_ParseScriptedSaber(yo2,&newInfo,1);
//[/RGBSabers]

	// team task
	v = Info_ValueForKey( configstring, "tt" );
	newInfo.teamTask = atoi(v);

	// team leader
	v = Info_ValueForKey( configstring, "tl" );
	newInfo.teamLeader = atoi(v);

//	v = Info_ValueForKey( configstring, "g_redteam" );
//	Q_strncpyz(newInfo.redTeam, v, MAX_TEAMNAME);

//	v = Info_ValueForKey( configstring, "g_blueteam" );
//	Q_strncpyz(newInfo.blueTeam, v, MAX_TEAMNAME);

	// model
	v = Info_ValueForKey( configstring, "model" );
	if ( cg_forceModel.integer ) {
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];
		char *skin;

		trap_Cvar_VariableStringBuffer( "model", modelStr, sizeof( modelStr ) );
		if ( ( skin = strchr( modelStr, '/' ) ) == NULL) {
			skin = "default";
		} else {
			*skin++ = 0;
		}
		Q_strncpyz( newInfo.skinName, skin, sizeof( newInfo.skinName ) );
		Q_strncpyz( newInfo.modelName, modelStr, sizeof( newInfo.modelName ) );

		if ( cgs.gametype >= GT_TEAM ) {
			// keep skin name
			slash = strchr( v, '/' );
			if ( slash ) {
				Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			}
		}
	} else {
		Q_strncpyz( newInfo.modelName, v, sizeof( newInfo.modelName ) );

		slash = strchr( newInfo.modelName, '/' );
		if ( !slash ) {
			// modelName didn not include a skin name
			Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
		} else {
			Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			// truncate modelName
			*slash = 0;
		}
	}

	if (cgs.gametype == GT_SIEGE)
	{ //entries only sent in siege mode
		//siege desired team
		v = Info_ValueForKey( configstring, "sdt" );
		if (v && v[0])
		{
            newInfo.siegeDesiredTeam = atoi(v);
		}
		else
		{
			newInfo.siegeDesiredTeam = 0;
		}

		//siege classname
		v = Info_ValueForKey( configstring, "siegeclass" );
		newInfo.siegeIndex = -1;

		if (v)
		{
			siegeClass_t *siegeClass = BG_SiegeFindClassByName(v);

			if (siegeClass)
			{ //See if this class forces a model, if so, then use it. Same for skin.
				newInfo.siegeIndex = BG_SiegeFindClassIndexByName(v);

				if (siegeClass->forcedModel[0])
				{
					Q_strncpyz( newInfo.modelName, siegeClass->forcedModel, sizeof( newInfo.modelName ) );
				}

				if (siegeClass->forcedSkin[0])
				{
					Q_strncpyz( newInfo.skinName, siegeClass->forcedSkin, sizeof( newInfo.skinName ) );
				}

				if (siegeClass->hasForcedSaberColor)
				{
					newInfo.icolor1 = siegeClass->forcedSaberColor;

					CG_ColorFromInt( newInfo.icolor1, newInfo.color1 );
				}
				if (siegeClass->hasForcedSaber2Color)
				{
					newInfo.icolor2 = siegeClass->forcedSaber2Color;

					CG_ColorFromInt( newInfo.icolor2, newInfo.color2 );
				}
			}
		}
	}

	saberUpdate[0] = qfalse;
	saberUpdate[1] = qfalse;

	//saber being used
	v = Info_ValueForKey( configstring, "st" );

	if (v && Q_stricmp(v, ci->saberName))
	{
		Q_strncpyz( newInfo.saberName, v, 64 );
		WP_SetSaber(clientNum, newInfo.saber, 0, newInfo.saberName);
		saberUpdate[0] = qtrue;
	}
	else
	{
		Q_strncpyz( newInfo.saberName, ci->saberName, 64 );
		memcpy(&newInfo.saber[0], &ci->saber[0], sizeof(newInfo.saber[0]));
		newInfo.ghoul2Weapons[0] = ci->ghoul2Weapons[0];

		//[VisualWeapons]
		//copy our first ghoul2 holstered saber to the new info file.
		newInfo.ghoul2HolsterWeapons[0] = ci->ghoul2HolsterWeapons[0];
		//[/VisualWeapons]
	}

	v = Info_ValueForKey( configstring, "st2" );

	if (v && Q_stricmp(v, ci->saber2Name))
	{
		Q_strncpyz( newInfo.saber2Name, v, 64 );
		WP_SetSaber(clientNum, newInfo.saber, 1, newInfo.saber2Name);
		saberUpdate[1] = qtrue;
	}
	else
	{
		Q_strncpyz( newInfo.saber2Name, ci->saber2Name, 64 );
		memcpy(&newInfo.saber[1], &ci->saber[1], sizeof(newInfo.saber[1]));
		newInfo.ghoul2Weapons[1] = ci->ghoul2Weapons[1];

		//[VisualWeapons]
		//copy our first ghoul2 holstered saber to the new info file.
		newInfo.ghoul2HolsterWeapons[1] = ci->ghoul2HolsterWeapons[1];
		//[/VisualWeapons]
	}

	if (saberUpdate[0] || saberUpdate[1])
	{
		int j = 0;

		while (j < MAX_SABERS)
		{
			if (saberUpdate[j])
			{
				if (newInfo.saber[j].model[0])
				{
					if (oldG2Weapons[j])
					{ //free the old instance(s)
						trap_G2API_CleanGhoul2Models(&oldG2Weapons[j]);
						oldG2Weapons[j] = 0;
					}

					//[VisualWeapons]
					//racc - delete the old holstered saber instance since we have a new saber to load.
					if (oldG2HolsteredWeapons[j])
					{ //free the old instance(s)
						trap_G2API_CleanGhoul2Models(&oldG2HolsteredWeapons[j]);
						oldG2HolsteredWeapons[j] = 0;
					}
					//[/VisualWeapons]

					CG_InitG2SaberData(j, &newInfo);
				}
				else
				{
					if (oldG2Weapons[j])
					{ //free the old instance(s)
						trap_G2API_CleanGhoul2Models(&oldG2Weapons[j]);
						oldG2Weapons[j] = 0;
					}

					//[VisualWeapons]
					//racc - delete the old holstered saber instance since we don't have this saber anymore.
					if (oldG2HolsteredWeapons[j])
					{ //free the old instance(s)
						trap_G2API_CleanGhoul2Models(&oldG2HolsteredWeapons[j]);
						oldG2HolsteredWeapons[j] = 0;
					}
					//[/VisualWeapons]
				}

				cg_entities[clientNum].weapon = 0;
				cg_entities[clientNum].ghoul2weapon = NULL; //force a refresh
			}
			j++;
		}
	}

	//Check for any sabers that didn't get set again, if they didn't, then reassign the pointers for the new ci
	k = 0;
	while (k < MAX_SABERS)
	{
		if (oldG2Weapons[k])
		{
			newInfo.ghoul2Weapons[k] = oldG2Weapons[k];
			//[VisualWeapons]
			newInfo.ghoul2HolsterWeapons[k] = oldG2HolsteredWeapons[k];
			//[/VisualWeapons]
		}
		k++;
	}

	//duel team
	v = Info_ValueForKey( configstring, "dt" );

	if (v)
	{
		newInfo.duelTeam = atoi(v);
	}
	else
	{
		newInfo.duelTeam = 0;
	}

	// force powers
	v = Info_ValueForKey( configstring, "forcepowers" );
	Q_strncpyz( newInfo.forcePowers, v, sizeof( newInfo.forcePowers ) );

	if (cgs.gametype >= GT_TEAM	&& !cgs.jediVmerc && cgs.gametype != GT_SIEGE )
	{ //We won't force colors for siege.
		BG_ValidateSkinForTeam( newInfo.modelName, newInfo.skinName, newInfo.team, newInfo.colorOverride );
	}
	else
	{
		newInfo.colorOverride[0] = newInfo.colorOverride[1] = newInfo.colorOverride[2] = 0.0f;
	}

	// scan for an existing clientinfo that matches this modelname
	// so we can avoid loading checks if possible
	if ( !CG_ScanForExistingClientInfo( &newInfo, clientNum ) ) {
		// if we are defering loads, just have it pick the first valid
		if (cg.snap && cg.snap->ps.clientNum == clientNum)
		{ //rww - don't defer your own client info ever
			CG_LoadClientInfo( &newInfo );
		}
		else if (  cg_deferPlayers.integer && cgs.gametype != GT_SIEGE && !cg_buildScript.integer && !cg.loading ) {
			// keep whatever they had if it won't violate team skins
			CG_SetDeferredClientInfo( &newInfo );
		} else {
			CG_LoadClientInfo( &newInfo );
		}
	}

	//[TrueView]
	if(cg.snap && cg.snap->ps.clientNum == clientNum)
	{//adjust our True View position for this new model since this model is for this client

		//Set the eye position based on the trueviewmodel.cfg if it has a position for this model.
		CG_AdjustEyePos ( newInfo.modelName );
	}
	//[/TrueView]

	// replace whatever was there with the new one
	newInfo.infoValid = qtrue;
	if (ci->ghoul2Model &&
		ci->ghoul2Model != newInfo.ghoul2Model &&
		trap_G2_HaveWeGhoul2Models(ci->ghoul2Model))
	{ //We must kill this instance before we remove our only pointer to it from the cgame.
	  //Otherwise we will end up with extra instances all over the place, I think.
		trap_G2API_CleanGhoul2Models(&ci->ghoul2Model);
	}
	*ci = newInfo;

	//force a weapon change anyway, for all clients being rendered to the current client
	while (i < MAX_CLIENTS)
	{
		cg_entities[i].ghoul2weapon = NULL;
		i++;
	}

	if (clientNum != -1)
	{ //don't want it using an invalid pointer to share
		trap_G2API_ClearAttachedInstance(clientNum);
	}

	// Check if the ghoul2 model changed in any way.  This is safer than assuming we have a legal cent shile loading info.
	if (entitiesInitialized && ci->ghoul2Model && (oldGhoul2 != ci->ghoul2Model))
	{	// Copy the new ghoul2 model to the centity.
		animation_t *anim;
		centity_t *cent = &cg_entities[clientNum];
		
		anim = &bgHumanoidAnimations[ (cg_entities[clientNum].currentState.legsAnim) ];

		if (anim)
		{
			int flags = BONE_ANIM_OVERRIDE_FREEZE;
			int firstFrame = anim->firstFrame;
			int setFrame = -1;
			float animSpeed = 50.0f / anim->frameLerp;

			if (anim->loopFrames != -1)
			{
				flags = BONE_ANIM_OVERRIDE_LOOP;
			}

			if (cent->pe.legs.frame >= anim->firstFrame && cent->pe.legs.frame <= (anim->firstFrame + anim->numFrames))
			{
				setFrame = cent->pe.legs.frame;
			}

			//rww - Set the animation again because it just got reset due to the model change
			trap_G2API_SetBoneAnim(ci->ghoul2Model, 0, "model_root", firstFrame, anim->firstFrame + anim->numFrames, flags, animSpeed, cg.time, setFrame, 150);

			cg_entities[clientNum].currentState.legsAnim = 0;
		}

		anim = &bgHumanoidAnimations[ (cg_entities[clientNum].currentState.torsoAnim) ];

		if (anim)
		{
			int flags = BONE_ANIM_OVERRIDE_FREEZE;
			int firstFrame = anim->firstFrame;
			int setFrame = -1;
			float animSpeed = 50.0f / anim->frameLerp;

			if (anim->loopFrames != -1)
			{
				flags = BONE_ANIM_OVERRIDE_LOOP;
			}

			if (cent->pe.torso.frame >= anim->firstFrame && cent->pe.torso.frame <= (anim->firstFrame + anim->numFrames))
			{
				setFrame = cent->pe.torso.frame;
			}

			//rww - Set the animation again because it just got reset due to the model change
			trap_G2API_SetBoneAnim(ci->ghoul2Model, 0, "lower_lumbar", firstFrame, anim->firstFrame + anim->numFrames, flags, animSpeed, cg.time, setFrame, 150);

			cg_entities[clientNum].currentState.torsoAnim = 0;
		}

		if (cg_entities[clientNum].ghoul2 && trap_G2_HaveWeGhoul2Models(cg_entities[clientNum].ghoul2))
		{
			trap_G2API_CleanGhoul2Models(&cg_entities[clientNum].ghoul2);
		}
		trap_G2API_DuplicateGhoul2Instance(ci->ghoul2Model, &cg_entities[clientNum].ghoul2);

		if (clientNum != -1)
		{
			//Attach the instance to this entity num so we can make use of client-server
			//shared operations if possible.
			trap_G2API_AttachInstanceToEntNum(cg_entities[clientNum].ghoul2, clientNum, qfalse);
		}

		if (trap_G2API_AddBolt(cg_entities[clientNum].ghoul2, 0, "face") == -1)
		{ //check now to see if we have this bone for setting anims and such
			cg_entities[clientNum].noFace = qtrue;
		}

		cg_entities[clientNum].localAnimIndex = CG_G2SkelForModel(cg_entities[clientNum].ghoul2);
		//[ANIMEVENTS]
		//This might cause slow down whenever someone with a new animevent.cfg joins the game.  Oh well.
		cg_entities[clientNum].eventAnimIndex = CG_ParseAnimationEvtFile( va("models/players/%s/", ci->modelName), cg_entities[clientNum].localAnimIndex, cgNumAnimEvents );
		//cg_entities[clientNum].eventAnimIndex = CG_G2EvIndexForModel(cg_entities[clientNum].ghoul2, cg_entities[clientNum].localAnimIndex);
		//[/ANIMEVENTS]

		if (cg_entities[clientNum].currentState.number != cg.predictedPlayerState.clientNum &&
			cg_entities[clientNum].currentState.weapon == WP_SABER)
		{
			cg_entities[clientNum].weapon = cg_entities[clientNum].currentState.weapon;
			if (cg_entities[clientNum].ghoul2 && ci->ghoul2Model)
			{
				CG_CopyG2WeaponInstance(&cg_entities[clientNum], cg_entities[clientNum].currentState.weapon, cg_entities[clientNum].ghoul2);
				cg_entities[clientNum].ghoul2weapon = CG_G2WeaponInstance(&cg_entities[clientNum], cg_entities[clientNum].currentState.weapon);
			}
			if (!cg_entities[clientNum].currentState.saberHolstered)
			{ //if not holstered set length and desired length for both blades to full right now.
				int j;
				BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
				BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);

				i = 0;
				while (i < MAX_SABERS)
				{
					j = 0;
					while (j < ci->saber[i].numBlades)
					{
						ci->saber[i].blade[j].length = ci->saber[i].blade[j].lengthMax;
						j++;
					}
					i++;
				}
			}
		}
	}
}


qboolean cgQueueLoad = qfalse;
/*
======================
CG_ActualLoadDeferredPlayers

Called at the beginning of CG_Player if cgQueueLoad is set.
======================
*/
void CG_ActualLoadDeferredPlayers( void )
{
	int		i;
	clientInfo_t	*ci;

	// scan for a deferred player to load
	for ( i = 0, ci = cgs.clientinfo ; i < cgs.maxclients ; i++, ci++ ) {
		if ( ci->infoValid && ci->deferred ) {
			CG_LoadClientInfo( ci );
//			break;
		}
	}
}

/*
======================
CG_LoadDeferredPlayers

Called each frame when a player is dead
and the scoreboard is up
so deferred players can be loaded
======================
*/
void CG_LoadDeferredPlayers( void ) {
	cgQueueLoad = qtrue;
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/

#define	FOOTSTEP_DISTANCE	32
static void _PlayerFootStep( const vec3_t origin, 
								const float orientation, 
								const float radius, 
								centity_t *const cent, footstepType_t footStepType ) 
{
	vec3_t		end, mins = {-7, -7, 0}, maxs = {7, 7, 2};
	trace_t		trace;
	footstep_t	soundType = FOOTSTEP_TOTAL;
	qboolean	bMark = qfalse;
	qhandle_t footMarkShader;
	int			effectID = -1;
	//float		alpha;

	// send a trace down from the player to the ground
	VectorCopy( origin, end );
	end[2] -= FOOTSTEP_DISTANCE;

	trap_CM_BoxTrace( &trace, origin, end, mins, maxs, 0, MASK_PLAYERSOLID );

	// no shadow if too high
	if ( trace.fraction >= 1.0f ) 
	{
		return;
	}

	//check for foot-steppable surface flag
	switch( trace.surfaceFlags & MATERIAL_MASK )
	{
		case MATERIAL_MUD:
			bMark = qtrue;
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_MUDRUN;
			} else {
				soundType = FOOTSTEP_MUDWALK;
			}
			effectID = cgs.effects.footstepMud;
			break;
		case MATERIAL_DIRT:			
			bMark = qtrue;
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_DIRTRUN;
			} else {
				soundType = FOOTSTEP_DIRTWALK;
			}
			effectID = cgs.effects.footstepSand;
			break;
		case MATERIAL_SAND:			
			bMark = qtrue;
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_SANDRUN;
			} else {
				soundType = FOOTSTEP_SANDWALK;
			}
			effectID = cgs.effects.footstepSand;
			break;
		case MATERIAL_SNOW:			
			bMark = qtrue;
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_SNOWRUN;
			} else {
				soundType = FOOTSTEP_SNOWWALK;
			}
			effectID = cgs.effects.footstepSnow;
			break;
		case MATERIAL_SHORTGRASS:		
		case MATERIAL_LONGGRASS:		
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_GRASSRUN;
			} else {
				soundType = FOOTSTEP_GRASSWALK;
			}
			break;
		case MATERIAL_SOLIDMETAL:		
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_METALRUN;
			} else {
				soundType = FOOTSTEP_METALWALK;
			}
			break;
		case MATERIAL_HOLLOWMETAL:	
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_PIPERUN;
			} else {
				soundType = FOOTSTEP_PIPEWALK;
			}
			break;
		case MATERIAL_GRAVEL:
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_GRAVELRUN;
			} else {
				soundType = FOOTSTEP_GRAVELWALK;
			}
			effectID = cgs.effects.footstepGravel;
			break;
		case MATERIAL_CARPET:
		case MATERIAL_FABRIC:
		case MATERIAL_CANVAS:
		case MATERIAL_RUBBER:
		case MATERIAL_PLASTIC:
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_RUGRUN;
			} else {
				soundType = FOOTSTEP_RUGWALK;
			}
			break;
		case MATERIAL_SOLIDWOOD:
		case MATERIAL_HOLLOWWOOD:
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_WOODRUN;
			} else {
				soundType = FOOTSTEP_WOODWALK;
			}
			break;

		default:
		//fall through
		case MATERIAL_GLASS:
		case MATERIAL_WATER:
		case MATERIAL_FLESH:
		case MATERIAL_BPGLASS:
		case MATERIAL_DRYLEAVES:
		case MATERIAL_GREENLEAVES:
		case MATERIAL_TILES:		
		case MATERIAL_PLASTER:
		case MATERIAL_SHATTERGLASS:
		case MATERIAL_ARMOR:
		case MATERIAL_COMPUTER:

		case MATERIAL_CONCRETE:		
		case MATERIAL_ROCK:			
		case MATERIAL_ICE:			
		case MATERIAL_MARBLE:			
			if ( footStepType == FOOTSTEP_HEAVY_R || footStepType == FOOTSTEP_HEAVY_L) {
				soundType = FOOTSTEP_STONERUN;
			} else {
				soundType = FOOTSTEP_STONEWALK;
			}
			break;
	}
	if (soundType < FOOTSTEP_TOTAL) 
	{
	 	trap_S_StartSound( NULL, cent->currentState.clientNum, CHAN_BODY, cgs.media.footsteps[soundType][rand()&3] );
	}

	if ( cg_footsteps.integer < 4 )
	{//debugging - 4 always does footstep effect
		if ( cg_footsteps.integer < 2 )	//1 for sounds, 2 for effects, 3 for marks
		{
			return;
		}
	}

	if ( effectID != -1 )
	{
		trap_FX_PlayEffectID( effectID, trace.endpos, trace.plane.normal, -1, -1 );
	}

	if ( cg_footsteps.integer < 4 )
	{//debugging - 4 always does footstep effect
		if ( !bMark || cg_footsteps.integer < 3 )	//1 for sounds, 2 for effects, 3 for marks
		{
			return;
		}
	}

	switch ( footStepType )
	{
	case FOOTSTEP_HEAVY_R:
		footMarkShader = cgs.media.fshrMarkShader;
		break;
	case FOOTSTEP_HEAVY_L:
		footMarkShader = cgs.media.fshlMarkShader;
		break;
	case FOOTSTEP_R:
		footMarkShader = cgs.media.fsrMarkShader;
		break;
	default:
	case FOOTSTEP_L:
		footMarkShader = cgs.media.fslMarkShader;
		break;
	}

	// fade the shadow out with height
//	alpha = 1.0 - trace.fraction;

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	if (trace.plane.normal[0] || trace.plane.normal[1] || trace.plane.normal[2])
	{
		CG_ImpactMark( footMarkShader, trace.endpos, trace.plane.normal, 
			orientation, 1,1,1, 1.0f, qfalse, radius, qfalse );
	}
}

static void CG_PlayerFootsteps( centity_t *cent, footstepType_t footStepType )
{
	if ( !cg_footsteps.integer ) 
	{
		return;
	}

	//FIXME: make this a feature of NPCs in the NPCs.cfg? Specify a footstep shader, if any?
	if ( cent->currentState.NPC_class != CLASS_ATST
		&& cent->currentState.NPC_class != CLASS_CLAW
		&& cent->currentState.NPC_class != CLASS_FISH
		&& cent->currentState.NPC_class != CLASS_FLIER2
		&& cent->currentState.NPC_class != CLASS_GLIDER
		&& cent->currentState.NPC_class != CLASS_INTERROGATOR
		&& cent->currentState.NPC_class != CLASS_MURJJ
		&& cent->currentState.NPC_class != CLASS_PROBE
		&& cent->currentState.NPC_class != CLASS_R2D2
		&& cent->currentState.NPC_class != CLASS_R5D2
		&& cent->currentState.NPC_class != CLASS_REMOTE
		&& cent->currentState.NPC_class != CLASS_SEEKER
		&& cent->currentState.NPC_class != CLASS_SENTRY
		&& cent->currentState.NPC_class != CLASS_SWAMP )
	{
		mdxaBone_t	boltMatrix;
		vec3_t tempAngles, sideOrigin;
		int footBolt = -1;

		tempAngles[PITCH]	= 0;
		tempAngles[YAW]		= cent->pe.legs.yawAngle;
		tempAngles[ROLL]	= 0;

		switch ( footStepType )
		{
		case FOOTSTEP_R:
		case FOOTSTEP_HEAVY_R:
			footBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*r_leg_foot");//cent->gent->footRBolt;
			break;
		case FOOTSTEP_L:
		case FOOTSTEP_HEAVY_L:
		default:
			footBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*l_leg_foot");//cent->gent->footLBolt;
			break;
		}


		//FIXME: get yaw orientation of the foot and use on decal
		trap_G2API_GetBoltMatrix( cent->ghoul2, 0, footBolt, &boltMatrix, tempAngles, cent->lerpOrigin, 
				cg.time, cgs.gameModels, cent->modelScale);
		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, sideOrigin );
		sideOrigin[2] += 15;	//fudge up a bit for coplanar
		_PlayerFootStep( sideOrigin, cent->pe.legs.yawAngle, 6, cent, footStepType );
	}
}

void CG_PlayerAnimEventDo( centity_t *cent, animevent_t *animEvent )
{
	soundChannel_t channel = CHAN_AUTO;
	clientInfo_t *client = NULL;
	qhandle_t swingSound = 0;
	qhandle_t spinSound = 0;

	if ( !cent || !animEvent )
	{
		return;
	}

	switch ( animEvent->eventType )
	{
	case AEV_SOUNDCHAN:
	//[AMBIENTEV]
	case AEV_AMBIENT:
		channel = (soundChannel_t)animEvent->eventData[AED_SOUNDCHANNEL];
	case AEV_SOUND:
		{	// are there variations on the sound?
			if ( animEvent->eventData[AED_SOUNDINDEX_START] == 0)
			{
				const int n = Q_irand( animEvent->eventData[AED_CSOUND_RANDSTART], (animEvent->eventData[AED_CSOUND_RANDSTART]+animEvent->eventData[AED_SOUND_NUMRANDOMSNDS]) );
				trap_S_StartSound( NULL, cent->currentState.number, channel, CG_CustomSound(cent->currentState.number, va(animEvent->stringData, n)) );
			}
			else
			{
				const int holdSnd = animEvent->eventData[ AED_SOUNDINDEX_START+Q_irand( 0, animEvent->eventData[AED_SOUND_NUMRANDOMSNDS] ) ];
				if ( holdSnd > 0 )
				{
					trap_S_StartSound( NULL, cent->currentState.number, channel, holdSnd );
				}
	//[/AMBIENTEV]
			}
		}
		break;
	case AEV_SABER_SWING:
		if (cent->currentState.eType == ET_NPC)
		{
			client = cent->npcClient;
			assert(client);
		}
		else
		{
			client = &cgs.clientinfo[cent->currentState.clientNum];
		}
		if ( client && client->infoValid && client->saber[animEvent->eventData[AED_SABER_SWING_SABERNUM]].swingSound[0] )
		{//custom swing sound
			swingSound = client->saber[0].swingSound[Q_irand(0,2)];
		}
		else
		{
			int		randomSwing = 1;
			switch ( animEvent->eventData[AED_SABER_SWING_TYPE] )
			{
			default:
			case 0://SWING_FAST
				randomSwing = Q_irand( 1, 3 );
				break;
			case 1://SWING_MEDIUM
				randomSwing = Q_irand( 4, 6 );
				break;
			case 2://SWING_STRONG
				randomSwing = Q_irand( 7, 9 );
				break;
			}
			swingSound = trap_S_RegisterSound(va("sound/weapons/saber/saberhup%i.wav", randomSwing));
		}
		trap_S_StartSound(cent->currentState.pos.trBase, cent->currentState.number, CHAN_AUTO, swingSound );
		break;
	case AEV_SABER_SPIN:
		if (cent->currentState.eType == ET_NPC)
		{
			client = cent->npcClient;
			assert(client);
		}
		else
		{
			client = &cgs.clientinfo[cent->currentState.clientNum];
		}
		if ( client 
			&& client->infoValid 
			&& client->saber[AED_SABER_SPIN_SABERNUM].spinSound )
		{//use override
			spinSound = client->saber[AED_SABER_SPIN_SABERNUM].spinSound;
		}
		else
		{
			switch ( animEvent->eventData[AED_SABER_SPIN_TYPE] )
			{
			case 0://saberspinoff
				spinSound = trap_S_RegisterSound( "sound/weapons/saber/saberspinoff.wav" );
				break;
			case 1://saberspin
				spinSound = trap_S_RegisterSound( "sound/weapons/saber/saberspin.wav" );
				break;
			case 2://saberspin1
				spinSound = trap_S_RegisterSound( "sound/weapons/saber/saberspin1.wav" );
				break;
			case 3://saberspin2
				spinSound = trap_S_RegisterSound( "sound/weapons/saber/saberspin2.wav" );
				break;
			case 4://saberspin3
				spinSound = trap_S_RegisterSound( "sound/weapons/saber/saberspin3.wav" );
				break;
			default://random saberspin1-3
				spinSound = trap_S_RegisterSound( va( "sound/weapons/saber/saberspin%d.wav", Q_irand(1,3)) );
				break;
			}
		}
		if ( spinSound )
		{
			trap_S_StartSound( NULL, cent->currentState.clientNum, CHAN_AUTO, spinSound );
		}
		break;
	case AEV_FOOTSTEP:
		CG_PlayerFootsteps( cent, (footstepType_t)animEvent->eventData[AED_FOOTSTEP_TYPE] );
		break;
	case AEV_EFFECT:
#if 0 //SP method
		//add bolt, play effect
		if ( animEvent->stringData != NULL && cent && cent->gent && cent->gent->ghoul2.size() )
		{//have a bolt name we want to use
			animEvent->eventData[AED_BOLTINDEX] = gi.G2API_AddBolt( &cent->gent->ghoul2[cent->gent->playerModel], animEvent->stringData );
			animEvent->stringData = NULL;//so we don't try to do this again
		}
		if ( animEvent->eventData[AED_BOLTINDEX] != -1 )
		{//have a bolt we want to play the effect on
			G_PlayEffect( animEvent->eventData[AED_EFFECTINDEX], cent->gent->playerModel, animEvent->eventData[AED_BOLTINDEX], cent->currentState.clientNum );
		}
		else
		{//play at origin?  FIXME: maybe allow a fwd/rt/up offset?
			theFxScheduler.PlayEffect( animEvent->eventData[AED_EFFECTINDEX], cent->lerpOrigin, qfalse );
		}
#else //my method
		if (animEvent->stringData && animEvent->stringData[0] && cent && cent->ghoul2)
		{
			animEvent->eventData[AED_MODELINDEX] = 0;
			if ( ( Q_stricmpn( "*blade", animEvent->stringData, 6 ) == 0 
					|| Q_stricmp( "*flash", animEvent->stringData ) == 0 ) )
			{//must be a weapon, try weapon 0?
				animEvent->eventData[AED_BOLTINDEX] = trap_G2API_AddBolt( cent->ghoul2, 1, animEvent->stringData );
				if ( animEvent->eventData[AED_BOLTINDEX] != -1 )
				{//found it!
					animEvent->eventData[AED_MODELINDEX] = 1;
				}
				else
				{//hmm, just try on the player model, then?
					animEvent->eventData[AED_BOLTINDEX] = trap_G2API_AddBolt( cent->ghoul2, 0, animEvent->stringData );
				}
			}
			else
			{
				animEvent->eventData[AED_BOLTINDEX] = trap_G2API_AddBolt( cent->ghoul2, 0, animEvent->stringData );
			}
			animEvent->stringData[0] = 0;
		}
		if ( animEvent->eventData[AED_BOLTINDEX] != -1 )
		{
			vec3_t lAngles;
			vec3_t bPoint, bAngle;
			mdxaBone_t matrix;

			VectorSet(lAngles, 0, cent->lerpAngles[YAW], 0);

			trap_G2API_GetBoltMatrix(cent->ghoul2, animEvent->eventData[AED_MODELINDEX], animEvent->eventData[AED_BOLTINDEX], &matrix, lAngles,
				cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
			BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, bPoint);
			VectorSet(bAngle, 0, 1, 0);

			trap_FX_PlayEffectID(animEvent->eventData[AED_EFFECTINDEX], bPoint, bAngle, -1, -1);
		}
		else
		{
			vec3_t bAngle;

			VectorSet(bAngle, 0, 1, 0);
			trap_FX_PlayEffectID(animEvent->eventData[AED_EFFECTINDEX], cent->lerpOrigin, bAngle, -1, -1);
		}
#endif
		break;
	//Would have to keep track of this on server to for these, it's not worth it.
	case AEV_FIRE:
	case AEV_MOVE:
		break;
		/*
	case AEV_FIRE:
		//add fire event
		if ( animEvent->eventData[AED_FIRE_ALT] )
		{
			G_AddEvent( cent->gent, EV_ALT_FIRE, 0 );
		}
		else
		{
			G_AddEvent( cent->gent, EV_FIRE_WEAPON, 0 );
		}
		break;
	case AEV_MOVE:
		//make him jump
		if ( cent && cent->gent && cent->gent->client )
		{
			if ( cent->gent->client->ps.groundEntityNum != ENTITYNUM_NONE )
			{//on something
				vec3_t	fwd, rt, up, angles = {0, cent->gent->client->ps.viewangles[YAW], 0};
				AngleVectors( angles, fwd, rt, up );
				//FIXME: set or add to velocity?
				VectorScale( fwd, animEvent->eventData[AED_MOVE_FWD], cent->gent->client->ps.velocity );
				VectorMA( cent->gent->client->ps.velocity, animEvent->eventData[AED_MOVE_RT], rt, cent->gent->client->ps.velocity );
				VectorMA( cent->gent->client->ps.velocity, animEvent->eventData[AED_MOVE_UP], up, cent->gent->client->ps.velocity );
				
				if ( animEvent->eventData[AED_MOVE_UP] > 0 )
				{//a jump
					cent->gent->client->ps.pm_flags |= PMF_JUMPING;

					G_AddEvent( cent->gent, EV_JUMP, 0 );
					//FIXME: if have force jump, do this?  or specify sound in the event data?
					//cent->gent->client->ps.forceJumpZStart = cent->gent->client->ps.origin[2];//so we don't take damage if we land at same height
					//G_SoundOnEnt( cent->gent, CHAN_BODY, "sound/weapons/force/jump.wav" );
				}
			}
		}
		break;
		*/
	default:
		return;
		break;
	}
}

/*
void CG_PlayerAnimEvents( int animFileIndex, int eventFileIndex, qboolean torso, int oldFrame, int frame, const vec3_t org, int entNum )

play any keyframed sounds - only when start a new frame
This func is called once for legs and once for torso
*/
void CG_PlayerAnimEvents( int animFileIndex, int eventFileIndex, qboolean torso, int oldFrame, int frame, int entNum )
{
	int		i;
	int		firstFrame = 0, lastFrame = 0;
	qboolean	doEvent = qfalse, inSameAnim = qfalse, loopAnim = qfalse, match = qfalse, animBackward = qfalse;
	animevent_t *animEvents = NULL;
	
	if ( torso )
	{
		//[ANIMEVENTS]
		animEvents = cgAllEvents[eventFileIndex].torsoAnimEvents;
		//animEvents = bgAllEvents[eventFileIndex].torsoAnimEvents;
		//[/ANIMEVENTS]
	}
	else
	{
		//[ANIMEVENTS]
		animEvents = cgAllEvents[eventFileIndex].legsAnimEvents;
		//animEvents = bgAllEvents[eventFileIndex].legsAnimEvents;
		//[/ANIMEVENTS]
	}
	if ( fabs((float)(oldFrame-frame)) > 1 )
	{//given a range, see if keyFrame falls in that range
		int oldAnim, anim;
		if ( torso )
		{
			/*
			if ( cg_reliableAnimSounds.integer > 1 )
			{//more precise, slower
				oldAnim = PM_TorsoAnimForFrame( &g_entities[entNum], oldFrame );
				anim = PM_TorsoAnimForFrame( &g_entities[entNum], frame );
			}
			else
			*/
			{//less precise, but faster
				oldAnim = cg_entities[entNum].currentState.torsoAnim;
				anim = cg_entities[entNum].nextState.torsoAnim;
			}
		}
		else
		{
			/*
			if ( cg_reliableAnimSounds.integer > 1 )
			{//more precise, slower
				oldAnim = PM_LegsAnimForFrame( &g_entities[entNum], oldFrame );
				anim = PM_TorsoAnimForFrame( &g_entities[entNum], frame );
			}
			else
			*/
			{//less precise, but faster
				oldAnim = cg_entities[entNum].currentState.legsAnim;
				anim = cg_entities[entNum].nextState.legsAnim;
			}
		}
		if ( anim != oldAnim )
		{//not in same anim
			inSameAnim = qfalse;
			//FIXME: we *could* see if the oldFrame was *just about* to play the keyframed sound...
		}
		else
		{//still in same anim, check for looping anim
			animation_t *animation;

			inSameAnim = qtrue;
			animation = &bgAllAnims[animFileIndex].anims[anim];
			animBackward = (animation->frameLerp<0);
			if ( animation->loopFrames != -1 )
			{//a looping anim!
				loopAnim = qtrue;
				firstFrame = animation->firstFrame;
				lastFrame = animation->firstFrame+animation->numFrames;
			}
		}
	}

	// Check for anim sound
	for (i=0;i<MAX_ANIM_EVENTS;++i)
	{
		if ( animEvents[i].eventType == AEV_NONE )	// No event, end of list
		{
			break;
		}

		match = qfalse;
		if ( animEvents[i].keyFrame == frame )
		{//exact match
			match = qtrue;
		}
		else if ( fabs((float)(oldFrame-frame)) > 1 /*&& cg_reliableAnimSounds.integer*/ )
		{//given a range, see if keyFrame falls in that range
			if ( inSameAnim )
			{//if changed anims altogether, sorry, the sound is lost
				if ( fabs((float)(oldFrame-animEvents[i].keyFrame)) <= 3
					 || fabs((float)(frame-animEvents[i].keyFrame)) <= 3 )
				{//must be at least close to the keyframe
					if ( animBackward )
					{//animation plays backwards
						if ( oldFrame > animEvents[i].keyFrame && frame < animEvents[i].keyFrame )
						{//old to new passed through keyframe
							match = qtrue;
						}
						else if ( loopAnim )
						{//hmm, didn't pass through it linearally, see if we looped
							if ( animEvents[i].keyFrame >= firstFrame && animEvents[i].keyFrame < lastFrame )
							{//keyframe is in this anim
								if ( oldFrame > animEvents[i].keyFrame 
									&& frame > oldFrame )
								{//old to new passed through keyframe
									match = qtrue;
								}
							}
						}
					}
					else
					{//anim plays forwards
						if ( oldFrame < animEvents[i].keyFrame && frame > animEvents[i].keyFrame )
						{//old to new passed through keyframe
							match = qtrue;
						}
						else if ( loopAnim )
						{//hmm, didn't pass through it linearally, see if we looped
							if ( animEvents[i].keyFrame >= firstFrame && animEvents[i].keyFrame < lastFrame )
							{//keyframe is in this anim
								if ( oldFrame < animEvents[i].keyFrame 
									&& frame < oldFrame )
								{//old to new passed through keyframe
									match = qtrue;
								}
							}
						}
					}
				}
			}
		}
		if ( match )
		{
			switch ( animEvents[i].eventType )
			{
			case AEV_SOUND:
			case AEV_SOUNDCHAN:
				// Determine probability of playing sound
				if (!animEvents[i].eventData[AED_SOUND_PROBABILITY])	// 100% 
				{
					doEvent = qtrue;
				}
				else if (animEvents[i].eventData[AED_SOUND_PROBABILITY] > Q_irand(0, 99) )
				{
					doEvent = qtrue;
				}
				break;
			case AEV_SABER_SWING:
				// Determine probability of playing sound
				if (!animEvents[i].eventData[AED_SABER_SWING_PROBABILITY])	// 100% 
				{
					doEvent = qtrue;
				}
				else if (animEvents[i].eventData[AED_SABER_SWING_PROBABILITY] > Q_irand(0, 99) )
				{
					doEvent = qtrue;
				}
				break;
			case AEV_SABER_SPIN:
				// Determine probability of playing sound
				if (!animEvents[i].eventData[AED_SABER_SPIN_PROBABILITY])	// 100% 
				{
					doEvent = qtrue;
				}
				else if (animEvents[i].eventData[AED_SABER_SPIN_PROBABILITY] > Q_irand(0, 99) )
				{
					doEvent = qtrue;
				}
				break;
			case AEV_FOOTSTEP:
				// Determine probability of playing sound
				if (!animEvents[i].eventData[AED_FOOTSTEP_PROBABILITY])	// 100% 
				{
					doEvent = qtrue;
				}
				else if (animEvents[i].eventData[AED_FOOTSTEP_PROBABILITY] > Q_irand(0, 99) )
				{
					doEvent = qtrue;
				}
				break;
			case AEV_EFFECT:
				// Determine probability of playing sound
				if (!animEvents[i].eventData[AED_EFFECT_PROBABILITY])	// 100% 
				{
					doEvent = qtrue;
				}
				else if (animEvents[i].eventData[AED_EFFECT_PROBABILITY] > Q_irand(0, 99) )
				{
					doEvent = qtrue;
				}
				break;
			case AEV_FIRE:
				// Determine probability of playing sound
				if (!animEvents[i].eventData[AED_FIRE_PROBABILITY])	// 100% 
				{
					doEvent = qtrue;
				}
				else if (animEvents[i].eventData[AED_FIRE_PROBABILITY] > Q_irand(0, 99) )
				{
					doEvent = qtrue;
				}
				break;
			case AEV_MOVE:
				doEvent = qtrue;
				break;
			//[AMBIENTEV]
			case AEV_AMBIENT:
				CG_Printf( "ERROR: CG_PlayerAnimEvents() tried to play a AEV_AMBIENT as a keyframed sound.\n" );
				break;
			//[/AMBIENTEV]
			default:
				//[ANIMEVENTS]
				CG_Printf( "ERROR: No eventType for animevent in CG_PlayerAnimEvents().\n" );
				//[/ANIMEVENTS]
				//doEvent = qfalse;//implicit
				break;
			}
			// do event
			if ( doEvent )
			{
				CG_PlayerAnimEventDo( &cg_entities[entNum], &animEvents[i] );
			}
		}
	}
}

//[AMBIENTEV]
//Checks for and plays Ambient model sounds
void CG_PlayerAmbientEvents ( centity_t *cent )
{
	animevent_t *animEvents = NULL;
	int EventNum;
	
	if(cent->currentState.eFlags & EF_DEAD || (cent->currentState.eType == ET_PLAYER &&
		(cg.predictedPlayerState.pm_type == PM_INTERMISSION || cgs.clientinfo[cent->currentState.clientNum].team == TEAM_SPECTATOR)) )
	{//Shouldn't play ambient sounds when dead; in intermission; or a spectator.
		return;
	}

	//Torso Ambient
	if( cg.time > cent->AmbTorsoTimer  )
	{
		animEvents = cgAllEvents[cent->eventAnimIndex].torsoAnimEvents;
		EventNum = CheckAnimFrameForEventType ( animEvents, 0, AEV_AMBIENT );
		if( EventNum != -1 )
		{//Find the Ambient event.
			if( !animEvents[EventNum].eventData[AED_SOUND_PROBABILITY] || animEvents[EventNum].eventData[AED_SOUND_PROBABILITY] > Q_irand(0, 99) )
			{ 
				CG_PlayerAnimEventDo( cent, &animEvents[EventNum] );
			}

			//Ok, now reset the timer.
			cent->AmbTorsoTimer = cg.time + (animEvents[EventNum].ambtime - animEvents[EventNum].ambrandom) + Q_irand( 0, (animEvents[EventNum].ambrandom + animEvents[EventNum].ambrandom));
		}
	}
	if( cg.time > cent->AmbLegsTimer )
	{
		animEvents = cgAllEvents[cent->eventAnimIndex].legsAnimEvents;
		EventNum = CheckAnimFrameForEventType ( animEvents, 0, AEV_AMBIENT );
		if( EventNum != -1 )
		{//Find the Ambient event.
			if(  !animEvents[EventNum].eventData[AED_SOUND_PROBABILITY] || animEvents[EventNum].eventData[AED_SOUND_PROBABILITY] > Q_irand(0, 99) )
			{ 
				CG_PlayerAnimEventDo( cent, &animEvents[EventNum] );
			}
			//Ok, now reset the timer.
			cent->AmbLegsTimer = cg.time + (animEvents[EventNum].ambtime - animEvents[EventNum].ambrandom) + Q_irand( 0, (animEvents[EventNum].ambrandom + animEvents[EventNum].ambrandom));
		}
	}
}
//[/AMBIENTEV]

void CG_TriggerAnimSounds( centity_t *cent )
{ //this also sets the lerp frames, so I suggest you keep calling it regardless of if you want anim sounds.
	int		curFrame = 0;
	float	currentFrame = 0;
	int		sFileIndex;

	assert(cent->localAnimIndex >= 0);

	sFileIndex = cent->eventAnimIndex;

	if (trap_G2API_GetBoneFrame(cent->ghoul2, "model_root", cg.time, &currentFrame, cgs.gameModels, 0))
	{
		// the above may have failed, not sure what to do about it, current frame will be zero in that case
		curFrame = floor( currentFrame );
	}
	if ( curFrame != cent->pe.legs.frame )
	{
		CG_PlayerAnimEvents( cent->localAnimIndex, sFileIndex, qfalse, cent->pe.legs.frame, curFrame, cent->currentState.number );
	}
	cent->pe.legs.oldFrame = cent->pe.torso.frame;
	cent->pe.legs.frame = curFrame;

	if (cent->noLumbar)
	{ //probably a droid or something.
		cent->pe.torso.oldFrame = cent->pe.legs.oldFrame;
		cent->pe.torso.frame = cent->pe.legs.frame;
		return;
	}

	if (trap_G2API_GetBoneFrame(cent->ghoul2, "lower_lumbar", cg.time, &currentFrame, cgs.gameModels, 0))
	{
		curFrame = floor( currentFrame );
	}
	if ( curFrame != cent->pe.torso.frame )
	{
		CG_PlayerAnimEvents( cent->localAnimIndex, sFileIndex, qtrue, cent->pe.torso.frame, curFrame, cent->currentState.number );
	}
	cent->pe.torso.oldFrame = cent->pe.torso.frame;
	cent->pe.torso.frame = curFrame;	
	cent->pe.torso.backlerp = 1.0f - (currentFrame - (float)curFrame);	

	//[AMBIENTEV]
	//Check/play ambient player sounds
	CG_PlayerAmbientEvents ( cent );
	//[/AMBIENTEV]
}


static qboolean CG_FirstAnimFrame(lerpFrame_t *lf, qboolean torsoOnly, float speedScale);

qboolean CG_InRoll( centity_t *cent )
{
	switch ( (cent->currentState.legsAnim) )
	{
	case BOTH_GETUP_BROLL_B:
	case BOTH_GETUP_BROLL_F:
	case BOTH_GETUP_BROLL_L:
	case BOTH_GETUP_BROLL_R:
	case BOTH_GETUP_FROLL_B:
	case BOTH_GETUP_FROLL_F:
	case BOTH_GETUP_FROLL_L:
	case BOTH_GETUP_FROLL_R:
	case BOTH_ROLL_F:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		if ( cent->pe.legs.animationTime > cg.time )
		{
			return qtrue;
		}
		break;
	}
	return qfalse;
}

qboolean CG_InRollAnim( centity_t *cent )
{
	switch ( (cent->currentState.legsAnim) )
	{
	case BOTH_ROLL_F:
	case BOTH_ROLL_B:
	case BOTH_ROLL_R:
	case BOTH_ROLL_L:
		return qtrue;
	}
	return qfalse;
}

/*
===============
CG_SetLerpFrameAnimation
===============
*/
#include "../namespace_begin.h"
qboolean BG_SaberStanceAnim( int anim );
qboolean PM_RunningAnim( int anim );
#include "../namespace_end.h"
static void CG_SetLerpFrameAnimation( centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float animSpeedMult, qboolean torsoOnly, qboolean flipState) {
	animation_t	*anim;
	float animSpeed;
	int	  flags=BONE_ANIM_OVERRIDE_FREEZE;
	int oldAnim = -1;
	int blendTime = 100;
	float oldSpeed = lf->animationSpeed;

	if (cent->localAnimIndex > 0)
	{ //rockettroopers can't have broken arms, nor can anything else but humanoids
		ci->brokenLimbs = cent->currentState.brokenLimbs;
	}

	oldAnim = lf->animationNumber;

	lf->animationNumber = newAnimation;

	if ( newAnimation < 0 || newAnimation >= MAX_TOTALANIMATIONS ) {
		CG_Error( "Bad animation number: %i", newAnimation );
	}

	anim = &bgAllAnims[cent->localAnimIndex].anims[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + abs(anim->frameLerp);

	if (cent->localAnimIndex > 1 &&
		anim->firstFrame == 0 &&
		anim->numFrames == 0)
	{ //We'll allow this for non-humanoids.
		return;
	}

	if ( cg_debugAnim.integer && (cg_debugAnim.integer < 0 || cg_debugAnim.integer == cent->currentState.clientNum) ) {
		if (lf == &cent->pe.legs)
		{
			CG_Printf( "%d: %d TORSO Anim: %i, '%s'\n", cg.time, cent->currentState.clientNum, newAnimation, GetStringForID(animTable, newAnimation));
		}
		else
		{
			CG_Printf( "%d: %d LEGS Anim: %i, '%s'\n", cg.time, cent->currentState.clientNum, newAnimation, GetStringForID(animTable, newAnimation));
		}
	}

	if (cent->ghoul2)
	{
		qboolean resumeFrame = qfalse;
		int beginFrame = -1;
		int firstFrame;
		int lastFrame;
#if 0 //disabled for now
		float unused;
#endif

		animSpeed = 50.0f / anim->frameLerp;
		if (lf->animation->loopFrames != -1)
		{
			flags = BONE_ANIM_OVERRIDE_LOOP;
		}

		if (animSpeed < 0)
		{
			lastFrame = anim->firstFrame;
			firstFrame = anim->firstFrame + anim->numFrames;
		}
		else
		{
			firstFrame = anim->firstFrame;
			lastFrame = anim->firstFrame + anim->numFrames;
		}

		if (cg_animBlend.integer)
		{
			flags |= BONE_ANIM_BLEND;
		}

		if (BG_InDeathAnim(newAnimation))
		{
			flags &= ~BONE_ANIM_BLEND;
		}
		else if ( oldAnim != -1 &&
			BG_InDeathAnim(oldAnim))
		{
			flags &= ~BONE_ANIM_BLEND;
		}

		if (flags & BONE_ANIM_BLEND)
		{
			if (BG_FlippingAnim(newAnimation))
			{
				blendTime = 200;
			}
			else if ( oldAnim != -1 &&
				(BG_FlippingAnim(oldAnim)) )
			{
				blendTime = 200;
			}
		}

		animSpeed *= animSpeedMult;

		BG_SaberStartTransAnim(cent->currentState.number, cent->currentState.fireflag, cent->currentState.weapon, newAnimation, &animSpeed, cent->currentState.brokenLimbs);

		if (torsoOnly)
		{
			if (lf->animationTorsoSpeed != animSpeedMult && newAnimation == oldAnim &&
				flipState == lf->lastFlip)
			{ //same animation, but changing speed, so we will want to resume off the frame we're on.
				resumeFrame = qtrue;
			}
			lf->animationTorsoSpeed = animSpeedMult;
		}
		else
		{
			if (lf->animationSpeed != animSpeedMult && newAnimation == oldAnim &&
				flipState == lf->lastFlip)
			{ //same animation, but changing speed, so we will want to resume off the frame we're on.
				resumeFrame = qtrue;
			}
			lf->animationSpeed = animSpeedMult;
		}

		//vehicles may have torso etc but we only want to animate the root bone
		if ( cent->currentState.NPC_class == CLASS_VEHICLE )
		{
			trap_G2API_SetBoneAnim(cent->ghoul2, 0, "model_root", firstFrame, lastFrame, flags, animSpeed,cg.time, beginFrame, blendTime);
			return;
		}

		if (torsoOnly && !cent->noLumbar)
		{ //rww - The guesswork based on the lerp frame figures is usually BS, so I've resorted to a call to get the frame of the bone directly.
			float GBAcFrame = 0;
			if (resumeFrame)
			{ //we already checked, and this is the same anim, same flip state, but different speed, so we want to resume with the new speed off of the same frame.
				trap_G2API_GetBoneFrame(cent->ghoul2, "lower_lumbar", cg.time, &GBAcFrame, NULL, 0);
				beginFrame = GBAcFrame;
			}

			//even if resuming, also be sure to check if we are running the same frame on the legs. If so, we want to use their frame no matter what.
			trap_G2API_GetBoneFrame(cent->ghoul2, "model_root", cg.time, &GBAcFrame, NULL, 0);

			if ((cent->currentState.torsoAnim) == (cent->currentState.legsAnim) && GBAcFrame >= anim->firstFrame && GBAcFrame <= (anim->firstFrame + anim->numFrames))
			{ //if the legs are already running this anim, pick up on the exact same frame to avoid the "wobbly spine" problem.
				beginFrame = GBAcFrame;
			}

			if (firstFrame > lastFrame || ci->torsoAnim == newAnimation)
			{ //don't resume on backwards playing animations.. I guess.
				beginFrame = -1;
			}

			trap_G2API_SetBoneAnim(cent->ghoul2, 0, "lower_lumbar", firstFrame, lastFrame, flags, animSpeed,cg.time, beginFrame, blendTime);

			// Update the torso frame with the new animation
			cent->pe.torso.frame = firstFrame;
			
			if (ci)
			{
				ci->torsoAnim = newAnimation;
			}
		}
		else
		{
			if (resumeFrame)
			{ //we already checked, and this is the same anim, same flip state, but different speed, so we want to resume with the new speed off of the same frame.
				float GBAcFrame = 0;
				trap_G2API_GetBoneFrame(cent->ghoul2, "model_root", cg.time, &GBAcFrame, NULL, 0);
				beginFrame = GBAcFrame;
			}

			if ((beginFrame < firstFrame) || (beginFrame > lastFrame))
			{ //out of range, don't use it then.
				beginFrame = -1;
			}

			if (cent->currentState.torsoAnim == cent->currentState.legsAnim &&
				(ci->legsAnim != newAnimation || oldSpeed != animSpeed))
			{ //alright, we are starting an anim on the legs, and that same anim is already playing on the toro, so pick up the frame.
				float GBAcFrame = 0;
				int oldBeginFrame = beginFrame;

				trap_G2API_GetBoneFrame(cent->ghoul2, "lower_lumbar", cg.time, &GBAcFrame, NULL, 0);
				beginFrame = GBAcFrame;
				if ((beginFrame < firstFrame) || (beginFrame > lastFrame))
				{ //out of range, don't use it then.
					beginFrame = oldBeginFrame;
				}
			}

			trap_G2API_SetBoneAnim(cent->ghoul2, 0, "model_root", firstFrame, lastFrame, flags, animSpeed, cg.time, beginFrame, blendTime);

			if (ci)
			{
				ci->legsAnim = newAnimation;
			}
		}

		if (cent->localAnimIndex <= 1 && (cent->currentState.torsoAnim) == newAnimation && !cent->noLumbar)
		{ //make sure we're humanoid before we access the motion bone
			trap_G2API_SetBoneAnim(cent->ghoul2, 0, "Motion", firstFrame, lastFrame, flags, animSpeed, cg.time, beginFrame, blendTime);
		}

#if 0 //disabled for now
		if (cent->localAnimIndex <= 1 && cent->currentState.brokenLimbs &&
			(cent->currentState.brokenLimbs & (1 << BROKENLIMB_LARM)))
		{ //broken left arm
			char *brokenBone = "lhumerus";
			animation_t *armAnim;
			int armFirstFrame;
			int armLastFrame;
			int armFlags = 0;
			float armAnimSpeed;

			armAnim = &bgAllAnims[cent->localAnimIndex].anims[ BOTH_DEAD21 ];
			ci->brokenLimbs = cent->currentState.brokenLimbs;

			armFirstFrame = armAnim->firstFrame;
			armLastFrame = armAnim->firstFrame+armAnim->numFrames;
			armAnimSpeed = 50.0f / armAnim->frameLerp;
			armFlags = BONE_ANIM_OVERRIDE_LOOP;

			if (cg_animBlend.integer)
			{
				armFlags |= BONE_ANIM_BLEND;
			}

			trap_G2API_SetBoneAnim(cent->ghoul2, 0, brokenBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, cg.time, -1, blendTime);
		}
		else if (cent->localAnimIndex <= 1 && cent->currentState.brokenLimbs &&
			(cent->currentState.brokenLimbs & (1 << BROKENLIMB_RARM)))
		{ //broken right arm
			char *brokenBone = "rhumerus";
			char *supportBone = "lhumerus";

			ci->brokenLimbs = cent->currentState.brokenLimbs;

			//Only put the arm in a broken pose if the anim is such that we
			//want to allow it.
			if ((//cent->currentState.weapon == WP_MELEE ||
				cent->currentState.weapon != WP_SABER ||
				BG_SaberStanceAnim(newAnimation) ||
				PM_RunningAnim(newAnimation)) &&
				cent->currentState.torsoAnim == newAnimation &&
				(!ci->saber[1].model[0] || cent->currentState.weapon != WP_SABER))
			{
				int armFirstFrame;
				int armLastFrame;
				int armFlags = 0;
				float armAnimSpeed;
				animation_t *armAnim;

				if (cent->currentState.weapon == WP_MELEE ||
					cent->currentState.weapon == WP_SABER ||
					cent->currentState.weapon == WP_BRYAR_PISTOL)
				{ //don't affect this arm if holding a gun, just make the other arm support it
					armAnim = &bgAllAnims[cent->localAnimIndex].anims[ BOTH_ATTACK2 ];

					//armFirstFrame = armAnim->firstFrame;
					armFirstFrame = armAnim->firstFrame+armAnim->numFrames;
					armLastFrame = armAnim->firstFrame+armAnim->numFrames;
					armAnimSpeed = 50.0f / armAnim->frameLerp;
					armFlags = BONE_ANIM_OVERRIDE_LOOP;

					/*
					if (cg_animBlend.integer)
					{
						armFlags |= BONE_ANIM_BLEND;
					}
					*/
					//No blend on the broken arm

					trap_G2API_SetBoneAnim(cent->ghoul2, 0, brokenBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, cg.time, -1, 0);
				}
				else
				{ //we want to keep the broken bone updated for some cases
					trap_G2API_SetBoneAnim(cent->ghoul2, 0, brokenBone, firstFrame, lastFrame, flags, animSpeed, cg.time, beginFrame, blendTime);
				}

				if (newAnimation != BOTH_MELEE1 &&
					newAnimation != BOTH_MELEE2 &&
					(newAnimation == TORSO_WEAPONREADY2 || newAnimation == BOTH_ATTACK2 || cent->currentState.weapon < WP_BRYAR_PISTOL))
				{
					//Now set the left arm to "support" the right one
					armAnim = &bgAllAnims[cent->localAnimIndex].anims[ BOTH_STAND2 ];
					armFirstFrame = armAnim->firstFrame;
					armLastFrame = armAnim->firstFrame+armAnim->numFrames;
					armAnimSpeed = 50.0f / armAnim->frameLerp;
					armFlags = BONE_ANIM_OVERRIDE_LOOP;

					if (cg_animBlend.integer)
					{
						armFlags |= BONE_ANIM_BLEND;
					}

					trap_G2API_SetBoneAnim(cent->ghoul2, 0, supportBone, armFirstFrame, armLastFrame, armFlags, armAnimSpeed, cg.time, -1, 150);
				}
				else
				{ //we want to keep the support bone updated for some cases
					trap_G2API_SetBoneAnim(cent->ghoul2, 0, supportBone, firstFrame, lastFrame, flags, animSpeed, cg.time, beginFrame, blendTime);
				}
			}
			else if (cent->currentState.torsoAnim == newAnimation)
			{ //otherwise, keep it set to the same as the torso
				trap_G2API_SetBoneAnim(cent->ghoul2, 0, brokenBone, firstFrame, lastFrame, flags, animSpeed, cg.time, beginFrame, blendTime);
				trap_G2API_SetBoneAnim(cent->ghoul2, 0, supportBone, firstFrame, lastFrame, flags, animSpeed, cg.time, beginFrame, blendTime);
			}
		}
		else if (ci &&
			(ci->brokenLimbs ||
			trap_G2API_GetBoneFrame(cent->ghoul2, "lhumerus", cg.time, &unused, cgs.gameModels, 0) ||
			trap_G2API_GetBoneFrame(cent->ghoul2, "rhumerus", cg.time, &unused, cgs.gameModels, 0)))
			//rwwFIXMEFIXME: brokenLimbs gets stomped sometimes, but it shouldn't.
		{ //remove the bone now so it can be set again
			char *brokenBone = NULL;
			int broken = 0;

			//Warning: Don't remove bones that you've added as bolts unless you want to invalidate your bolt index
			//(well, in theory, I haven't actually run into the problem)
			if (ci->brokenLimbs & (1<<BROKENLIMB_LARM))
			{
				brokenBone = "lhumerus";
				broken |= (1<<BROKENLIMB_LARM);
			}
			else if (ci->brokenLimbs & (1<<BROKENLIMB_RARM))
			{ //can only have one arm broken at once.
				brokenBone = "rhumerus";
				broken |= (1<<BROKENLIMB_RARM);

				//want to remove the support bone too then
				trap_G2API_SetBoneAnim(cent->ghoul2, 0, "lhumerus", 0, 1, 0, 0, cg.time, -1, 0);
				if (!trap_G2API_RemoveBone(cent->ghoul2, "lhumerus", 0))
				{
					assert(0);
					Com_Printf("WARNING: Failed to remove lhumerus\n");
				}
			}

			if (!brokenBone)
			{
				trap_G2API_SetBoneAnim(cent->ghoul2, 0, "lhumerus", 0, 1, 0, 0, cg.time, -1, 0);
				trap_G2API_SetBoneAnim(cent->ghoul2, 0, "rhumerus", 0, 1, 0, 0, cg.time, -1, 0);
				trap_G2API_RemoveBone(cent->ghoul2, "lhumerus", 0);
				trap_G2API_RemoveBone(cent->ghoul2, "rhumerus", 0);
				ci->brokenLimbs = 0;
			}
			else
			{
				//Set the flags and stuff to 0, so that the remove will succeed
				trap_G2API_SetBoneAnim(cent->ghoul2, 0, brokenBone, 0, 1, 0, 0, cg.time, -1, 0);

				//Now remove it
				if (!trap_G2API_RemoveBone(cent->ghoul2, brokenBone, 0))
				{
					assert(0);
					Com_Printf("WARNING: Failed to remove %s\n", brokenBone);
				}
				ci->brokenLimbs &= ~broken;
			}
		}
#endif
	}
}


/*
===============
CG_FirstAnimFrame

Returns true if the lerpframe is on its first frame of animation.
Otherwise false.

This is used to scale an animation into higher-speed without restarting
the animation before it completes at normal speed, in the case of a looping
animation (such as the leg running anim).
===============
*/
static qboolean CG_FirstAnimFrame(lerpFrame_t *lf, qboolean torsoOnly, float speedScale)
{
	if (torsoOnly)
	{
		if (lf->animationTorsoSpeed == speedScale)
		{
			return qfalse;
		}
	}
	else
	{
		if (lf->animationSpeed == speedScale)
		{
			return qfalse;
		}
	}

	//I don't care where it is in the anim now, I am going to pick up from the same bone frame.
/*
	if (lf->animation->numFrames < 2)
	{
		return qtrue;
	}

	if (lf->animation->firstFrame == lf->frame)
	{
		return qtrue;
	}
*/

	return qtrue;
}

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static void CG_RunLerpFrame( centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, qboolean flipState, int newAnimation, float speedScale, qboolean torsoOnly) 
{
	// debugging tool to get no animations
	if ( cg_animSpeed.integer == 0 ) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if (cent->currentState.forceFrame)
	{
		if (lf->lastForcedFrame != cent->currentState.forceFrame)
		{
			int flags = BONE_ANIM_OVERRIDE_FREEZE|BONE_ANIM_BLEND;
			float animSpeed = 1.0f;
			trap_G2API_SetBoneAnim(cent->ghoul2, 0, "lower_lumbar", cent->currentState.forceFrame, cent->currentState.forceFrame+1, flags, animSpeed, cg.time, -1, 150);
			trap_G2API_SetBoneAnim(cent->ghoul2, 0, "model_root", cent->currentState.forceFrame, cent->currentState.forceFrame+1, flags, animSpeed, cg.time, -1, 150);
			trap_G2API_SetBoneAnim(cent->ghoul2, 0, "Motion", cent->currentState.forceFrame, cent->currentState.forceFrame+1, flags, animSpeed, cg.time, -1, 150);
		}

		lf->lastForcedFrame = cent->currentState.forceFrame;

		lf->animationNumber = 0;
	}
	else
	{
		lf->lastForcedFrame = -1;

		if ( (newAnimation != lf->animationNumber || cent->currentState.brokenLimbs != ci->brokenLimbs || lf->lastFlip != flipState || !lf->animation) || (CG_FirstAnimFrame(lf, torsoOnly, speedScale)) ) 
		{
			CG_SetLerpFrameAnimation( cent, ci, lf, newAnimation, speedScale, torsoOnly, flipState);
		}
	}

	lf->lastFlip = flipState;

	if ( lf->frameTime > cg.time + 200 ) {
		lf->frameTime = cg.time;
	}

	if ( lf->oldFrameTime > cg.time ) {
		lf->oldFrameTime = cg.time;
	}
	
	// calculate current lerp value
	if ( lf->frameTime == lf->oldFrameTime ) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)( cg.time - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
}


/*
===============
CG_ClearLerpFrame
===============
*/
static void CG_ClearLerpFrame( centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, int animationNumber, qboolean torsoOnly) {
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation( cent, ci, lf, animationNumber, 1, torsoOnly, qfalse );

	if ( lf->animation->frameLerp < 0 )
	{//Plays backwards
		lf->oldFrame = lf->frame = (lf->animation->firstFrame + lf->animation->numFrames);
	}
	else
	{
		lf->oldFrame = lf->frame = lf->animation->firstFrame;
	}
}


/*
===============
CG_PlayerAnimation
===============
*/
#include "../namespace_begin.h"
qboolean PM_WalkingAnim( int anim );
#include "../namespace_end.h"

static void CG_PlayerAnimation( centity_t *cent, int *legsOld, int *legs, float *legsBackLerp,
						int *torsoOld, int *torso, float *torsoBackLerp ) {
	clientInfo_t	*ci;
	int				clientNum;
	float			speedScale;

	clientNum = cent->currentState.clientNum;

	if ( cg_noPlayerAnims.integer ) {
		*legsOld = *legs = *torsoOld = *torso = 0;
		return;
	}

	if (!PM_RunningAnim(cent->currentState.legsAnim) &&
		!PM_WalkingAnim(cent->currentState.legsAnim))
	{ //if legs are not in a walking/running anim then just animate at standard speed
		speedScale = 1.0f;
	}
	else if (cent->currentState.forcePowersActive & (1 << FP_RAGE))
	{
		speedScale = 1.3f;
	}
	else if (cent->currentState.forcePowersActive & (1 << FP_SPEED))
	{
		speedScale = 1.7f;
	}
	else
	{
		speedScale = 1.0f;
	}

	if (cent->currentState.eType == ET_NPC)
	{
		ci = cent->npcClient;
		assert(ci);
	}
	else
	{
		ci = &cgs.clientinfo[ clientNum ];
	}

	CG_RunLerpFrame( cent, ci, &cent->pe.legs, cent->currentState.legsFlip, cent->currentState.legsAnim, speedScale, qfalse);

	if (!(cent->currentState.forcePowersActive & (1 << FP_RAGE)))
	{ //don't affect torso anim speed unless raged
		speedScale = 1.0f;
	}
	else
	{
		speedScale = 1.7f;
	}

	*legsOld = cent->pe.legs.oldFrame;
	*legs = cent->pe.legs.frame;
	*legsBackLerp = cent->pe.legs.backlerp;

	// If this is not a vehicle, you may lerm the frame (since vehicles never have a torso anim). -AReis
	if ( cent->currentState.NPC_class != CLASS_VEHICLE )
	{	
		CG_RunLerpFrame( cent, ci, &cent->pe.torso, cent->currentState.torsoFlip, cent->currentState.torsoAnim, speedScale, qtrue );

		*torsoOld = cent->pe.torso.oldFrame;
		*torso = cent->pe.torso.frame;
		*torsoBackLerp = cent->pe.torso.backlerp;
	}
}




/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

#if 0
typedef struct boneAngleParms_s {
	void *ghoul2;
	int modelIndex;
	char *boneName;
	vec3_t angles;
	int flags;
	int up;
	int right;
	int forward;
	qhandle_t *modelList;
	int blendTime;
	int currentTime;

	qboolean refreshSet;
} boneAngleParms_t;

boneAngleParms_t cgBoneAnglePostSet;
#endif

void CG_G2SetBoneAngles(void *ghoul2, int modelIndex, const char *boneName, const vec3_t angles, const int flags,
								const int up, const int right, const int forward, qhandle_t *modelList,
								int blendTime , int currentTime )
{ //we want to hold off on setting the bone angles until the end of the frame, because every time we set
  //them the entire skeleton has to be reconstructed.
#if 0
	//This function should ONLY be called from CG_Player() or a function that is called only within CG_Player().
	//At the end of the frame we will check to use this information to call SetBoneAngles
	memset(&cgBoneAnglePostSet, 0, sizeof(cgBoneAnglePostSet));
	cgBoneAnglePostSet.ghoul2 = ghoul2;
	cgBoneAnglePostSet.modelIndex = modelIndex;
	cgBoneAnglePostSet.boneName = (char *)boneName;

	cgBoneAnglePostSet.angles[0] = angles[0];
	cgBoneAnglePostSet.angles[1] = angles[1];
	cgBoneAnglePostSet.angles[2] = angles[2];

	cgBoneAnglePostSet.flags = flags;
	cgBoneAnglePostSet.up = up;
	cgBoneAnglePostSet.right = right;
	cgBoneAnglePostSet.forward = forward;
	cgBoneAnglePostSet.modelList = modelList;
	cgBoneAnglePostSet.blendTime = blendTime;
	cgBoneAnglePostSet.currentTime = currentTime;

	cgBoneAnglePostSet.refreshSet = qtrue;
#endif
	//We don't want to go with the delayed approach, we want out bolt points and everything to be updated in realtime.
	//We'll just take the reconstructs and live with them.
	trap_G2API_SetBoneAngles(ghoul2, modelIndex, boneName, angles, flags, up, right, forward, modelList,
		blendTime, currentTime);
}

/*
================
CG_Rag_Trace

Variant on CG_Trace. Doesn't trace for ents because ragdoll engine trace code has no entity
trace access. Maybe correct this sometime, so bmodel col. at least works with ragdoll.
But I don't want to slow it down..
================
*/
void	CG_Rag_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, 
					 int skipNumber, int mask ) {
	trap_CM_BoxTrace ( result, start, end, mins, maxs, 0, mask);
	result->entityNum = result->fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
}

//#define _RAG_BOLT_TESTING

#ifdef _RAG_BOLT_TESTING
void CG_TempTestFunction(centity_t *cent, vec3_t forcedAngles)
{
	mdxaBone_t boltMatrix;
	vec3_t tAngles;
	vec3_t bOrg;
	vec3_t bDir;
	vec3_t uOrg;

	VectorSet(tAngles, 0, cent->lerpAngles[YAW], 0);

	trap_G2API_GetBoltMatrix(cent->ghoul2, 1, 0, &boltMatrix, tAngles, cent->lerpOrigin,
		cg.time, cgs.gameModels, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, bOrg);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, bDir);

	VectorMA(bOrg, 40, bDir, uOrg);

	CG_TestLine(bOrg, uOrg, 50, 0x0000ff, 1);

	cent->turAngles[YAW] = forcedAngles[YAW];
}
#endif

//list of valid ragdoll effectors
static const char *cg_effectorStringTable[] =
{ //commented out the ones I don't want dragging to affect
//	"thoracic",
//	"rhand",
	"lhand",
	"rtibia",
	"ltibia",
	"rtalus",
	"ltalus",
//	"rradiusX",
	"lradiusX",
	"rfemurX",
	"lfemurX",
//	"ceyebrow",
	NULL //always terminate
};

//we want to see which way the pelvis is facing to get a relatively oriented base settling frame
//this is to avoid the arms stretching in opposite directions on the body trying to reach the base
//pose if the pelvis is flipped opposite of the base pose or something -rww
static int CG_RagAnimForPositioning(centity_t *cent)
{
	int bolt;
	vec3_t dir;
	mdxaBone_t matrix;

	assert(cent->ghoul2);
	bolt = trap_G2API_AddBolt(cent->ghoul2, 0, "pelvis");
	assert(bolt > -1);

	trap_G2API_GetBoltMatrix(cent->ghoul2, 0, bolt, &matrix, cent->turAngles, cent->lerpOrigin,
		cg.time, cgs.gameModels, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&matrix, NEGATIVE_Z, dir);

	if (dir[2] > 0.0f)
	{ //facing up
		return BOTH_DEADFLOP2;
	}
	else
	{ //facing down
		return BOTH_DEADFLOP1;
	}
}

//rww - cgame interface for the ragdoll stuff.
//Returns qtrue if the entity is now in a ragdoll state, otherwise qfalse.
qboolean CG_RagDoll(centity_t *cent, vec3_t forcedAngles)
{
	vec3_t usedOrg;
	qboolean inSomething = qfalse;
	int ragAnim;//BOTH_DEAD1; //BOTH_DEATH1;

	if (!cg_ragDoll.integer)
	{
		return qfalse;
	}

	if (cent->localAnimIndex)
	{ //don't rag non-humanoids
		return qfalse;
	}

	VectorCopy(cent->lerpOrigin, usedOrg);

	if (!cent->isRagging)
	{ //If we're not in a ragdoll state, perform the checks.
		if (cent->currentState.eFlags & EF_RAG)
		{ //want to go into it no matter what then
			inSomething = qtrue;
		}
		else if (cent->currentState.groundEntityNum == ENTITYNUM_NONE)
		{//racc - in the air.
			vec3_t cVel;

			VectorCopy(cent->currentState.pos.trDelta, cVel);

			if (VectorNormalize(cVel) > 400)
			{ //if he's flying through the air at a good enough speed, switch into ragdoll
				inSomething = qtrue;
			}
		}

		if (cent->currentState.eType == ET_BODY)
		{ //just rag bodies immediately if their own was ragging on respawn
			if (cent->ownerRagging)
			{
				cent->isRagging = qtrue;
				return qfalse;
			}
		}

		if (cg_ragDoll.integer > 1)
		{
			inSomething = qtrue;
		}

		if (!inSomething)
		{
			int anim = (cent->currentState.legsAnim);
			int dur = (bgAllAnims[cent->localAnimIndex].anims[anim].numFrames-1) * fabs((float)(bgAllAnims[cent->localAnimIndex].anims[anim].frameLerp));
			int i = 0;
			int boltChecks[5];
			vec3_t boltPoints[5];
			vec3_t trStart, trEnd;
			vec3_t tAng;
			qboolean deathDone = qfalse;
			trace_t tr;
			mdxaBone_t boltMatrix;

			VectorSet( tAng, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL] );

			if (cent->pe.legs.animationTime > 50 && (cg.time - cent->pe.legs.animationTime) > dur)
			{ //Looks like the death anim is done playing
				deathDone = qtrue;
			}

			if (deathDone)
			{ //only trace from the hands if the death anim is already done.
				boltChecks[0] = trap_G2API_AddBolt(cent->ghoul2, 0, "rhand");
				boltChecks[1] = trap_G2API_AddBolt(cent->ghoul2, 0, "lhand");
			}
			else
			{ //otherwise start the trace loop at the cranium.
				i = 2;
			}
			boltChecks[2] = trap_G2API_AddBolt(cent->ghoul2, 0, "cranium");
			//boltChecks[3] = trap_G2API_AddBolt(cent->ghoul2, 0, "rtarsal");
			//boltChecks[4] = trap_G2API_AddBolt(cent->ghoul2, 0, "ltarsal");
			boltChecks[3] = trap_G2API_AddBolt(cent->ghoul2, 0, "rtalus");
			boltChecks[4] = trap_G2API_AddBolt(cent->ghoul2, 0, "ltalus");

			//This may seem bad, but since we have a bone cache now it should manage to not be too disgustingly slow.
			//Do the head first, because the hands reference it anyway.
			trap_G2API_GetBoltMatrix(cent->ghoul2, 0, boltChecks[2], &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltPoints[2]);

			while (i < 5)
			{
				if (i < 2)
				{ //when doing hands, trace to the head instead of origin
					trap_G2API_GetBoltMatrix(cent->ghoul2, 0, boltChecks[i], &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
					BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltPoints[i]);
					VectorCopy(boltPoints[i], trStart);
					VectorCopy(boltPoints[2], trEnd);
				}
				else
				{
					if (i > 2)
					{ //2 is the head, which already has the bolt point.
						trap_G2API_GetBoltMatrix(cent->ghoul2, 0, boltChecks[i], &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
						BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltPoints[i]);
					}
					VectorCopy(boltPoints[i], trStart);
					VectorCopy(cent->lerpOrigin, trEnd);
				}

				//Now that we have all that sorted out, trace between the two points we desire.
				CG_Rag_Trace(&tr, trStart, NULL, NULL, trEnd, cent->currentState.number, MASK_SOLID);

				if (tr.fraction != 1.0 || tr.startsolid || tr.allsolid)
				{ //Hit something or start in solid, so flag it and break.
					//This is a slight hack, but if we aren't done with the death anim, we don't really want to
					//go into ragdoll unless our body has a relatively "flat" pitch.
#if 0
					vec3_t vSub;

					//Check the pitch from the head to the right foot (should be reasonable)
					VectorSubtract(boltPoints[2], boltPoints[3], vSub);
					VectorNormalize(vSub);
					vectoangles(vSub, vSub);

					if (deathDone || (vSub[PITCH] < 50 && vSub[PITCH] > -50))
					{
						inSomething = qtrue;
					}
#else
					inSomething = qtrue;
#endif
					break;
				}

				i++;
			}
		}

		if (inSomething)
		{
			cent->isRagging = qtrue;
#if 0
			VectorClear(cent->lerpOriginOffset);
#endif
		}
	}

	if (cent->isRagging)
	{ //We're in a ragdoll state, so make the call to keep our positions updated and whatnot.
		sharedRagDollParams_t tParms;
		sharedRagDollUpdateParams_t tuParms;

		ragAnim = CG_RagAnimForPositioning(cent);

		if (cent->ikStatus)
		{ //ik must be reset before ragdoll is started, or you'll get some interesting results.
			trap_G2API_SetBoneIKState(cent->ghoul2, cg.time, NULL, IKS_NONE, NULL);
			cent->ikStatus = qfalse;
		}

		//these will be used as "base" frames for the ragoll settling.
		tParms.startFrame = bgAllAnims[cent->localAnimIndex].anims[ragAnim].firstFrame;// + bgAllAnims[cent->localAnimIndex].anims[ragAnim].numFrames;
		tParms.endFrame = bgAllAnims[cent->localAnimIndex].anims[ragAnim].firstFrame + bgAllAnims[cent->localAnimIndex].anims[ragAnim].numFrames;
#if 0
		{
			float animSpeed = 0;
			int blendTime = 600;
			int flags = 0;//BONE_ANIM_OVERRIDE_FREEZE;

			if (bgAllAnims[cent->localAnimIndex].anims[ragAnim].loopFrames != -1)
			{
				flags = BONE_ANIM_OVERRIDE_LOOP;
			}

			/*
			if (cg_animBlend.integer)
			{
				flags |= BONE_ANIM_BLEND;
			}
			*/

			animSpeed = 50.0f / bgAllAnims[cent->localAnimIndex].anims[ragAnim].frameLerp;
			trap_G2API_SetBoneAnim(cent->ghoul2, 0, "lower_lumbar", tParms.startFrame, tParms.endFrame, flags, animSpeed,cg.time, -1, blendTime);
			trap_G2API_SetBoneAnim(cent->ghoul2, 0, "Motion", tParms.startFrame, tParms.endFrame, flags, animSpeed, cg.time, -1, blendTime);
			trap_G2API_SetBoneAnim(cent->ghoul2, 0, "model_root", tParms.startFrame, tParms.endFrame, flags, animSpeed, cg.time, -1, blendTime);
		}
#elif 1 //with my new method of doing things I want it to continue the anim
		{
			float currentFrame;
			int startFrame, endFrame;
			int flags;
			float animSpeed;

			if (trap_G2API_GetBoneAnim(cent->ghoul2, "model_root", cg.time, &currentFrame, &startFrame, &endFrame, &flags, &animSpeed, cgs.gameModels, 0))
			{ //lock the anim on the current frame.
				int blendTime = 500;
				animation_t *curAnim = &bgAllAnims[cent->localAnimIndex].anims[cent->currentState.legsAnim];

				if (currentFrame >= (curAnim->firstFrame + curAnim->numFrames-1))
				{ //this is sort of silly but it works for now.
					currentFrame = (curAnim->firstFrame + curAnim->numFrames-2);
				}

				trap_G2API_SetBoneAnim(cent->ghoul2, 0, "lower_lumbar", currentFrame, currentFrame+1, flags, animSpeed,cg.time, currentFrame, blendTime);
				trap_G2API_SetBoneAnim(cent->ghoul2, 0, "model_root", currentFrame, currentFrame+1, flags, animSpeed, cg.time, currentFrame, blendTime);
				trap_G2API_SetBoneAnim(cent->ghoul2, 0, "Motion", currentFrame, currentFrame+1, flags, animSpeed, cg.time, currentFrame, blendTime);
			}
		}
#endif
		CG_G2SetBoneAngles(cent->ghoul2, 0, "upper_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.gameModels, 0, cg.time);
		CG_G2SetBoneAngles(cent->ghoul2, 0, "lower_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.gameModels, 0, cg.time);
		CG_G2SetBoneAngles(cent->ghoul2, 0, "thoracic", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.gameModels, 0, cg.time);
		CG_G2SetBoneAngles(cent->ghoul2, 0, "cervical", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, cgs.gameModels, 0, cg.time);

		VectorCopy(forcedAngles, tParms.angles);
		VectorCopy(usedOrg, tParms.position);
		VectorCopy(cent->modelScale, tParms.scale);
		tParms.me = cent->currentState.number;

		tParms.collisionType = 1;
		tParms.RagPhase = RP_DEATH_COLLISION;
		tParms.fShotStrength = 4;

		trap_G2API_SetRagDoll(cent->ghoul2, &tParms);

		VectorCopy(forcedAngles, tuParms.angles);
		VectorCopy(usedOrg, tuParms.position);
		VectorCopy(cent->modelScale, tuParms.scale);
		tuParms.me = cent->currentState.number;
		tuParms.settleFrame = tParms.endFrame-1;

		if (cent->currentState.groundEntityNum != ENTITYNUM_NONE)
		{
			VectorClear(tuParms.velocity);
		}
		else
		{
			VectorScale(cent->currentState.pos.trDelta, 2.0f, tuParms.velocity);
		}

		trap_G2API_AnimateG2Models(cent->ghoul2, cg.time, &tuParms);

		//So if we try to get a bolt point it's still correct
		cent->turAngles[YAW] = 
		cent->lerpAngles[YAW] = 
		cent->pe.torso.yawAngle = 
		cent->pe.legs.yawAngle = forcedAngles[YAW];

		if (cent->currentState.ragAttach &&
			(cent->currentState.eType != ET_NPC || cent->currentState.NPC_class != CLASS_VEHICLE))
		{
			centity_t *grabEnt;

			if (cent->currentState.ragAttach == ENTITYNUM_NONE)
			{ //switch cl 0 and entitynum_none, so we can operate on the "if non-0" concept
				grabEnt = &cg_entities[0];
			}
			else
			{
				grabEnt = &cg_entities[cent->currentState.ragAttach];
			}

			if (grabEnt->ghoul2)
			{
				mdxaBone_t matrix;
				vec3_t bOrg;
				vec3_t thisHand;
				vec3_t hands;
				vec3_t pcjMin, pcjMax;
				vec3_t pDif;
				vec3_t thorPoint;
				float difLen;
				int thorBolt;

				//Get the person who is holding our hand's hand location
				trap_G2API_GetBoltMatrix(grabEnt->ghoul2, 0, 0, &matrix, grabEnt->turAngles, grabEnt->lerpOrigin,
					cg.time, cgs.gameModels, grabEnt->modelScale);
				BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, bOrg);

				//Get our hand's location
				trap_G2API_GetBoltMatrix(cent->ghoul2, 0, 0, &matrix, cent->turAngles, cent->lerpOrigin,
					cg.time, cgs.gameModels, cent->modelScale);
				BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, thisHand);

				//Get the position of the thoracic bone for hinting its velocity later on
				thorBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "thoracic");
				trap_G2API_GetBoltMatrix(cent->ghoul2, 0, thorBolt, &matrix, cent->turAngles, cent->lerpOrigin,
					cg.time, cgs.gameModels, cent->modelScale);
				BG_GiveMeVectorFromMatrix(&matrix, ORIGIN, thorPoint);

				VectorSubtract(bOrg, thisHand, hands);

				if (VectorLength(hands) < 3.0f)
				{
					trap_G2API_RagForceSolve(cent->ghoul2, qfalse);
				}
				else
				{
					trap_G2API_RagForceSolve(cent->ghoul2, qtrue);
				}

				//got the hand pos of him, now we want to make our hand go to it
				trap_G2API_RagEffectorGoal(cent->ghoul2, "rhand", bOrg);
				trap_G2API_RagEffectorGoal(cent->ghoul2, "rradius", bOrg);
				trap_G2API_RagEffectorGoal(cent->ghoul2, "rradiusX", bOrg);
				trap_G2API_RagEffectorGoal(cent->ghoul2, "rhumerusX", bOrg);
				trap_G2API_RagEffectorGoal(cent->ghoul2, "rhumerus", bOrg);

				//Make these two solve quickly so we can update decently
				trap_G2API_RagPCJGradientSpeed(cent->ghoul2, "rhumerus", 1.5f);
				trap_G2API_RagPCJGradientSpeed(cent->ghoul2, "rradius", 1.5f);

				//Break the constraints on them I suppose
				VectorSet(pcjMin, -999, -999, -999);
				VectorSet(pcjMax, 999, 999, 999);
				trap_G2API_RagPCJConstraint(cent->ghoul2, "rhumerus", pcjMin, pcjMax);
				trap_G2API_RagPCJConstraint(cent->ghoul2, "rradius", pcjMin, pcjMax);

				cent->overridingBones = cg.time + 2000;

				//hit the thoracic velocity to the hand point
				VectorSubtract(bOrg, thorPoint, hands);
				VectorNormalize(hands);
				VectorScale(hands, 2048.0f, hands);
				trap_G2API_RagEffectorKick(cent->ghoul2, "thoracic", hands);
				trap_G2API_RagEffectorKick(cent->ghoul2, "ceyebrow", hands);

				VectorSubtract(cent->ragLastOrigin, cent->lerpOrigin, pDif);
				VectorCopy(cent->lerpOrigin, cent->ragLastOrigin);

				if (cent->ragLastOriginTime >= cg.time && cent->currentState.groundEntityNum != ENTITYNUM_NONE)
				{ //make sure it's reasonably updated
					difLen = VectorLength(pDif);
					if (difLen > 0.0f)
					{ //if we're being dragged, then kick all the bones around a bit
						vec3_t dVel;
						vec3_t rVel;
						int i = 0;

						if (difLen < 12.0f)
						{
							VectorScale(pDif, 12.0f/difLen, pDif);
							difLen = 12.0f;
						}

						while (cg_effectorStringTable[i])
						{
							VectorCopy(pDif, dVel);
							dVel[2] = 0;

							//Factor in a random velocity
							VectorSet(rVel, flrand(-0.1f, 0.1f), flrand(-0.1f, 0.1f), flrand(0.1f, 0.5));
							VectorScale(rVel, 8.0f, rVel);

							VectorAdd(dVel, rVel, dVel);
							VectorScale(dVel, 10.0f, dVel);

							trap_G2API_RagEffectorKick(cent->ghoul2, cg_effectorStringTable[i], dVel);

#if 0
							{
								mdxaBone_t bm;
								vec3_t borg;
								vec3_t vorg;
								int b = trap_G2API_AddBolt(cent->ghoul2, 0, cg_effectorStringTable[i]);

								trap_G2API_GetBoltMatrix(cent->ghoul2, 0, b, &bm, cent->turAngles, cent->lerpOrigin, cg.time,
									cgs.gameModels, cent->modelScale);
								BG_GiveMeVectorFromMatrix(&bm, ORIGIN, borg);

								VectorMA(borg, 1.0f, dVel, vorg);

								CG_TestLine(borg, vorg, 50, 0x0000ff, 1);
							}
#endif

							i++;
						}
					}
				}
				cent->ragLastOriginTime = cg.time + 1000;
			}
		}
		else if (cent->overridingBones)
		{ //reset things to their normal rag state
			vec3_t pcjMin, pcjMax;
			vec3_t dVel;

			//got the hand pos of him, now we want to make our hand go to it
			trap_G2API_RagEffectorGoal(cent->ghoul2, "rhand", NULL);
			trap_G2API_RagEffectorGoal(cent->ghoul2, "rradius", NULL);
			trap_G2API_RagEffectorGoal(cent->ghoul2, "rradiusX", NULL);
			trap_G2API_RagEffectorGoal(cent->ghoul2, "rhumerusX", NULL);
			trap_G2API_RagEffectorGoal(cent->ghoul2, "rhumerus", NULL);

			VectorSet(dVel, 0.0f, 0.0f, -64.0f);
			trap_G2API_RagEffectorKick(cent->ghoul2, "rhand", dVel);

			trap_G2API_RagPCJGradientSpeed(cent->ghoul2, "rhumerus", 0.0f);
			trap_G2API_RagPCJGradientSpeed(cent->ghoul2, "rradius", 0.0f);

			VectorSet(pcjMin,-100.0f,-40.0f,-15.0f);
			VectorSet(pcjMax,-15.0f,80.0f,15.0f);
			trap_G2API_RagPCJConstraint(cent->ghoul2, "rhumerus", pcjMin, pcjMax);

			VectorSet(pcjMin,-25.0f,-20.0f,-20.0f);
			VectorSet(pcjMax,90.0f,20.0f,-20.0f);
			trap_G2API_RagPCJConstraint(cent->ghoul2, "rradius", pcjMin, pcjMax);

			if (cent->overridingBones < cg.time)
			{
				trap_G2API_RagForceSolve(cent->ghoul2, qfalse);
				cent->overridingBones = 0;
			}
			else
			{
				trap_G2API_RagForceSolve(cent->ghoul2, qtrue);
			}
		}

		return qtrue;
	}

	return qfalse;
}

//set the bone angles of this client entity based on data from the server -rww
void CG_G2ServerBoneAngles(centity_t *cent)
{
	int i = 0;
	int bone = cent->currentState.boneIndex1;
	int flags, up, right, forward;
	vec3_t boneAngles;

	VectorCopy(cent->currentState.boneAngles1, boneAngles);

	while (i < 4)
	{ //cycle through the 4 bone index values on the entstate
		if (bone)
		{ //if it's non-0 then it could have something in it.
			const char *boneName = CG_ConfigString(CS_G2BONES+bone);

			if (boneName && boneName[0])
			{ //got the bone, now set the angles from the corresponding entitystate boneangles value.
				flags = BONE_ANGLES_POSTMULT;

				//get the orientation out of our bit field
				forward = (cent->currentState.boneOrient)&7; //3 bits from bit 0
				right = (cent->currentState.boneOrient>>3)&7; //3 bits from bit 3
				up = (cent->currentState.boneOrient>>6)&7; //3 bits from bit 6

				trap_G2API_SetBoneAngles(cent->ghoul2, 0, boneName, boneAngles, flags, up, right, forward, cgs.gameModels, 100, cg.time);
			}
		}

		switch (i)
		{
		case 0:
			bone = cent->currentState.boneIndex2;
			VectorCopy(cent->currentState.boneAngles2, boneAngles);
			break;
		case 1:
			bone = cent->currentState.boneIndex3;
			VectorCopy(cent->currentState.boneAngles3, boneAngles);
			break;
		case 2:
			bone = cent->currentState.boneIndex4;
			VectorCopy(cent->currentState.boneAngles4, boneAngles);
			break;
		default:
			break;
		}

		i++;
	}
}

/*
-------------------------
CG_G2SetHeadBlink
-------------------------
*/
static void CG_G2SetHeadBlink( centity_t *cent, qboolean bStart )
{
	vec3_t	desiredAngles;
	int blendTime = 80;
	qboolean bWink = qfalse;
	const int hReye = trap_G2API_AddBolt( cent->ghoul2, 0, "reye" );
	const int hLeye = trap_G2API_AddBolt( cent->ghoul2, 0, "leye" );

	if (hLeye == -1)
	{
		return;
	}

	VectorClear(desiredAngles);

	if (bStart)
	{
		desiredAngles[YAW] = -50;
		if ( random() > 0.95f )
		{
			bWink = qtrue;
			blendTime /=3;
		}
	}
	trap_G2API_SetBoneAngles( cent->ghoul2, 0, "leye", desiredAngles,
		BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, blendTime, cg.time ); 

	if (hReye == -1)
	{
		return;
	}
	
	if (!bWink)
	{
		trap_G2API_SetBoneAngles( cent->ghoul2, 0, "reye", desiredAngles,
			BONE_ANGLES_POSTMULT, POSITIVE_Y, POSITIVE_Z, POSITIVE_X, NULL, blendTime, cg.time ); 
	}
}

/*
-------------------------
CG_G2SetHeadAnims
-------------------------
*/
static void CG_G2SetHeadAnim( centity_t *cent, int anim )
{
	const int blendTime = 50;
	const animation_t *animations = bgAllAnims[cent->localAnimIndex].anims;
	int	animFlags = BONE_ANIM_OVERRIDE ;//| BONE_ANIM_BLEND;
	// animSpeed is 1.0 if the frameLerp (ms/frame) is 50 (20 fps).
//	float		timeScaleMod = (cg_timescale.value&&gent&&gent->s.clientNum==0&&!player_locked&&!MatrixMode&&gent->client->ps.forcePowersActive&(1<<FP_SPEED))?(1.0/cg_timescale.value):1.0;
	const float		timeScaleMod = (cg_timescale.value)?(1.0/cg_timescale.value):1.0;
	float animSpeed = 50.0f / animations[anim].frameLerp * timeScaleMod;
	int	firstFrame;
	int	lastFrame;

	if (animations[anim].numFrames <= 0)
	{
		return;
	}
	if (anim == FACE_DEAD)
	{
		animFlags |= BONE_ANIM_OVERRIDE_FREEZE;
	}
	// animSpeed is 1.0 if the frameLerp (ms/frame) is 50 (20 fps).
	if ( animSpeed < 0 )
	{//play anim backwards
		
		lastFrame = animations[anim].firstFrame -1;
		firstFrame = (animations[anim].numFrames -1) + animations[anim].firstFrame ;
	}
	else
	{
		firstFrame = animations[anim].firstFrame;
		lastFrame = (animations[anim].numFrames) + animations[anim].firstFrame;
	}

	// first decide if we are doing an animation on the head already
//	int startFrame, endFrame;
//	const qboolean animatingHead =  gi.G2API_GetAnimRangeIndex(&gent->ghoul2[gent->playerModel], cent->gent->faceBone, &startFrame, &endFrame);
	
//	if (!animatingHead || ( animations[anim].firstFrame != startFrame ) )// only set the anim if we aren't going to do the same animation again
	{
	//	gi.G2API_SetBoneAnimIndex(&gent->ghoul2[gent->playerModel], cent->gent->faceBone,
	//		firstFrame, lastFrame, animFlags, animSpeed, cg.time, -1, blendTime);
		trap_G2API_SetBoneAnim(cent->ghoul2, 0, "face", firstFrame, lastFrame, animFlags, animSpeed,
			cg.time, -1, blendTime);
	}
}

qboolean CG_G2PlayerHeadAnims( centity_t *cent )
{
	clientInfo_t *ci = NULL;
	int anim = -1;
	int voiceVolume = 0;

	if(cent->localAnimIndex > 1)
	{ //only do this for humanoids
		return qfalse;
	}
	
	if (cent->noFace)
	{	// i don't have a face
		return qfalse;
	}

	if (cent->currentState.number < MAX_CLIENTS)
	{
		ci = &cgs.clientinfo[cent->currentState.number];
	}
	else
	{
		ci = cent->npcClient;
	}

	if (!ci)
	{
		return qfalse;
	}

	if ( cent->currentState.eFlags & EF_DEAD )
	{//Dead people close their eyes and don't make faces!
		anim = FACE_DEAD;
		ci->facial_blink = -1;
	}
	else 
	{
		if (!ci->facial_blink)
		{	// set the timers
			ci->facial_blink = cg.time + flrand(4000.0, 8000.0);
			ci->facial_frown = cg.time + flrand(6000.0, 10000.0);
			ci->facial_aux = cg.time + flrand(6000.0, 10000.0);
		}
		
		//are we blinking?
		if (ci->facial_blink < 0)
		{	// yes, check if we are we done blinking ?
			if (-(ci->facial_blink) < cg.time)
			{	// yes, so reset blink timer
				ci->facial_blink = cg.time + flrand(4000.0, 8000.0);
				CG_G2SetHeadBlink( cent, qfalse );	//stop the blink
			}
		}
		else // no we aren't blinking 
		{	
			if (ci->facial_blink < cg.time)// but should we start ?
			{
				CG_G2SetHeadBlink( cent, qtrue );
				if (ci->facial_blink == 1)
				{//requested to stay shut by SET_FACEEYESCLOSED
					ci->facial_blink = -(cg.time + 99999999.0f);// set blink timer
				}
				else
				{
					ci->facial_blink = -(cg.time + 300.0f);// set blink timer
				}
			} 
		}
		
		voiceVolume = trap_S_GetVoiceVolume(cent->currentState.number);

		if (voiceVolume > 0)	// if we aren't talking, then it will be 0, -1 for talking but paused
		{
			anim = FACE_TALK1 + voiceVolume -1;
		}
		else if (voiceVolume == 0)	//don't do aux if in a slient part of speech
		{//not talking
			if (ci->facial_aux < 0)	// are we auxing ?
			{	//yes
				if (-(ci->facial_aux) < cg.time)// are we done auxing ?
				{	// yes, reset aux timer
					ci->facial_aux = cg.time + flrand(7000.0, 10000.0);
				}
				else
				{	// not yet, so choose aux
					anim = FACE_ALERT;
				}
			}
			else // no we aren't auxing 
			{	// but should we start ?
				if (ci->facial_aux < cg.time)
				{//yes
					anim = FACE_ALERT;
					// set aux timer
					ci->facial_aux = -(cg.time + 2000.0);
				} 
			}	
			
			if (anim != -1)	//we we are auxing, see if we should override with a frown
			{	
				if (ci->facial_frown < 0)// are we frowning ?
				{	// yes, 
					if (-(ci->facial_frown) < cg.time)//are we done frowning ?
					{	// yes, reset frown timer
						ci->facial_frown = cg.time + flrand(7000.0, 10000.0);
					}
					else 
					{	// not yet, so choose frown
						anim = FACE_FROWN;
					}
				}
				else// no we aren't frowning 
				{	// but should we start ?
					if (ci->facial_frown < cg.time)
					{
						anim = FACE_FROWN;
						// set frown timer
						ci->facial_frown = -(cg.time + 2000.0);
					}
				}
			}

		}//talking
	}//dead
	if (anim != -1)
	{
		CG_G2SetHeadAnim( cent, anim );
		return qtrue;
	}
	return qfalse;
}


static void CG_G2PlayerAngles( centity_t *cent, vec3_t legs[3], vec3_t legsAngles)
{
	clientInfo_t *ci;

	//rww - now do ragdoll stuff
	if ((cent->currentState.eFlags & EF_DEAD) || (cent->currentState.eFlags & EF_RAG))
	{
		vec3_t forcedAngles;

		VectorClear(forcedAngles);
		forcedAngles[YAW] = cent->lerpAngles[YAW];

		if (CG_RagDoll(cent, forcedAngles))
		{ //if we managed to go into the rag state, give our ent axis the forced angles and return.
			AnglesToAxis( forcedAngles, legs );
			VectorCopy(forcedAngles, legsAngles);
			return;
		}
	}
	else if (cent->isRagging)
	{//racc - disable rag doll.
		cent->isRagging = qfalse;
		trap_G2API_SetRagDoll(cent->ghoul2, NULL); //calling with null parms resets to no ragdoll.
	}

	if (cent->currentState.eType == ET_NPC)
	{
		ci = cent->npcClient;
		assert(ci);
	}
	else
	{
		ci = &cgs.clientinfo[cent->currentState.number];
	}

	//rww - Quite possibly the most arguments for a function ever.
	if (cent->localAnimIndex <= 1)
	{ //don't do these things on non-humanoids
		vec3_t lookAngles;
		entityState_t *emplaced = NULL;

		if (cent->currentState.hasLookTarget)
		{
			VectorSubtract(cg_entities[cent->currentState.lookTarget].lerpOrigin, cent->lerpOrigin, lookAngles);
			vectoangles(lookAngles, lookAngles);
			ci->lookTime = cg.time + 1000;
		}
		else
		{
			VectorCopy(cent->lerpAngles, lookAngles);
		}
		lookAngles[PITCH] = 0;

		if (cent->currentState.otherEntityNum2)
		{
			emplaced = &cg_entities[cent->currentState.otherEntityNum2].currentState;
		}

		BG_G2PlayerAngles(cent->ghoul2, ci->bolt_motion, &cent->currentState, cg.time,
			cent->lerpOrigin, cent->lerpAngles, legs, legsAngles, &cent->pe.torso.yawing, &cent->pe.torso.pitching,
			&cent->pe.legs.yawing, &cent->pe.torso.yawAngle, &cent->pe.torso.pitchAngle, &cent->pe.legs.yawAngle,
			cg.frametime, cent->turAngles, cent->modelScale, ci->legsAnim, ci->torsoAnim, &ci->corrTime,
			lookAngles, ci->lastHeadAngles, ci->lookTime, emplaced, &ci->superSmoothTime);

		if (cent->currentState.heldByClient && cent->currentState.heldByClient <= MAX_CLIENTS)
		{ //then put our arm in this client's hand
			//is index+1 because index 0 is valid.
			int heldByIndex = cent->currentState.heldByClient-1;
			centity_t *other = &cg_entities[heldByIndex];

			if (other && other->ghoul2 && ci->bolt_lhand)
			{
				mdxaBone_t boltMatrix;
				vec3_t boltOrg;

				trap_G2API_GetBoltMatrix(other->ghoul2, 0, ci->bolt_lhand, &boltMatrix, other->turAngles, other->lerpOrigin, cg.time, cgs.gameModels, other->modelScale);
				BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltOrg);

				BG_IK_MoveArm(cent->ghoul2, ci->bolt_lhand, cg.time, &cent->currentState,
					cent->currentState.torsoAnim/*BOTH_DEAD1*/, boltOrg, &cent->ikStatus, cent->lerpOrigin, cent->lerpAngles, cent->modelScale, 500, qfalse);
			}
		}
		else if (cent->ikStatus)
		{ //make sure we aren't IKing if we don't have anyone to hold onto us.
			BG_IK_MoveArm(cent->ghoul2, ci->bolt_lhand, cg.time, &cent->currentState,
				cent->currentState.torsoAnim/*BOTH_DEAD1*/, vec3_origin, &cent->ikStatus, cent->lerpOrigin, cent->lerpAngles, cent->modelScale, 500, qtrue);
		}
	}
	else if ( cent->m_pVehicle && cent->m_pVehicle->m_pVehicleInfo->type == VH_WALKER )
	{
		vec3_t lookAngles;

		VectorCopy(cent->lerpAngles, legsAngles);
		legsAngles[PITCH] = 0;
		AnglesToAxis( legsAngles, legs );

		VectorCopy(cent->lerpAngles, lookAngles);
		lookAngles[YAW] = lookAngles[ROLL] = 0;

		BG_G2ATSTAngles( cent->ghoul2, cg.time, lookAngles );
	}
	else
	{
		if (cent->currentState.eType == ET_NPC &&
			cent->currentState.NPC_class == CLASS_VEHICLE &&
			cent->m_pVehicle &&
			cent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
		{ //fighters actually want to take pitch and roll into account for the axial angles
			VectorCopy(cent->lerpAngles, legsAngles);
			AnglesToAxis( legsAngles, legs );
		}
		else if (cent->currentState.eType == ET_NPC &&
			cent->currentState.m_iVehicleNum &&
			cent->currentState.NPC_class != CLASS_VEHICLE )
		{ //an NPC bolted to a vehicle should use the full angles
			VectorCopy(cent->lerpAngles, legsAngles);
			AnglesToAxis( legsAngles, legs );
		}
		else
		{
			vec3_t nhAngles;

			if (cent->currentState.eType == ET_NPC &&
				cent->currentState.NPC_class == CLASS_VEHICLE &&
				cent->m_pVehicle &&
				cent->m_pVehicle->m_pVehicleInfo->type == VH_SPEEDER)
			{ //yeah, a hack, sorry.
				VectorSet(nhAngles, 0, cent->lerpAngles[YAW], cent->lerpAngles[ROLL]);
			}
			else
			{
				VectorSet(nhAngles, 0, cent->lerpAngles[YAW], 0);
			}
			AnglesToAxis( nhAngles, legs );
		}
	}

	//See if we have any bone angles sent from the server
	CG_G2ServerBoneAngles(cent);
}
//==========================================================================

/*
===============
CG_TrailItem
===============
*/
#if 0
static void CG_TrailItem( centity_t *cent, qhandle_t hModel ) {
	refEntity_t		ent;
	vec3_t			angles;
	vec3_t			axis[3];

	VectorCopy( cent->lerpAngles, angles );
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AnglesToAxis( angles, axis );

	memset( &ent, 0, sizeof( ent ) );
	VectorMA( cent->lerpOrigin, -16, axis[0], ent.origin );
	ent.origin[2] += 16;
	angles[YAW] += 90;
	AnglesToAxis( angles, ent.axis );

	ent.hModel = hModel;
	trap_R_AddRefEntityToScene( &ent );
}
#endif


/*
===============
CG_PlayerFlag
===============
*/
static void CG_PlayerFlag( centity_t *cent, qhandle_t hModel ) {
	refEntity_t		ent;
	vec3_t			angles;
	vec3_t			axis[3];
	vec3_t			boltOrg, tAng, getAng, right;
	mdxaBone_t		boltMatrix;
	clientInfo_t	*ci;

	//[TrueView]
	if (cent->currentState.number == cg.snap->ps.clientNum 
		&& !cg.renderingThirdPerson && !cg_trueguns.integer 
		&& cg.snap->ps.weapon != WP_SABER)
	/*
	if (cent->currentState.number == cg.snap->ps.clientNum &&
		!cg.renderingThirdPerson)
	*/
	//[/TrueView]
	{
		return;
	}

	if (!cent->ghoul2)
	{
		return;
	}

	if (cent->currentState.eType == ET_NPC)
	{
		ci = cent->npcClient;
		assert(ci);
	}
	else
	{
		ci = &cgs.clientinfo[cent->currentState.number];
	}

	VectorSet( tAng, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL] );

	trap_G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_llumbar, &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltOrg);

	BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_X, tAng);
	vectoangles(tAng, tAng);

	VectorCopy(cent->lerpAngles, angles);

	boltOrg[2] -= 12;
	VectorSet(getAng, 0, cent->lerpAngles[1], 0);
	AngleVectors(getAng, 0, right, 0);
	boltOrg[0] += right[0]*8;
	boltOrg[1] += right[1]*8;
	boltOrg[2] += right[2]*8;

	angles[PITCH] = -cent->lerpAngles[PITCH]/2-30;
	angles[YAW] = tAng[YAW]+270;

	AnglesToAxis(angles, axis);

	memset( &ent, 0, sizeof( ent ) );
	VectorMA( boltOrg, 24, axis[0], ent.origin );

	angles[ROLL] += 20;
	AnglesToAxis( angles, ent.axis );

	ent.hModel = hModel;

	ent.modelScale[0] = 0.5;
	ent.modelScale[1] = 0.5;
	ent.modelScale[2] = 0.5;
	ScaleModelAxis(&ent);

	/*
	if (cent->currentState.number == cg.snap->ps.clientNum)
	{ //If we're the current client (in third person), render the flag on our back transparently
		ent.renderfx |= RF_FORCE_ENT_ALPHA;
		ent.shaderRGBA[3] = 100;
	}
	*/
	//FIXME: Not doing this at the moment because sorting totally messes up

	trap_R_AddRefEntityToScene( &ent );
}


/*
===============
CG_PlayerPowerups
===============
*/
static void CG_PlayerPowerups( centity_t *cent, refEntity_t *torso ) {
	int		powerups;
	clientInfo_t	*ci;

	powerups = cent->currentState.powerups;
	if ( !powerups ) {
		return;
	}

	// quad gives a dlight
	if ( powerups & ( 1 << PW_QUAD ) ) {
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2f, 0.2f, 1 );
	}

	if (cent->currentState.eType == ET_NPC)
	{
		ci = cent->npcClient;
		assert(ci);
	}
	else
	{
		ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	}
	// redflag
	if ( powerups & ( 1 << PW_REDFLAG ) ) {
		CG_PlayerFlag( cent, cgs.media.redFlagModel );
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0, 0.2f, 0.2f );
	}

	// blueflag
	if ( powerups & ( 1 << PW_BLUEFLAG ) ) {
		CG_PlayerFlag( cent, cgs.media.blueFlagModel );
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2f, 0.2f, 1.0 );
	}

	// neutralflag
	if ( powerups & ( 1 << PW_NEUTRALFLAG ) ) {
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0, 1.0, 1.0 );
	}

	// haste leaves smoke trails
	/*
	if ( powerups & ( 1 << PW_HASTE ) ) {
		CG_HasteTrail( cent );
	}
	*/
}


/*
===============
CG_PlayerFloatSprite

Float a sprite over the player's head
===============
*/
static void CG_PlayerFloatSprite( centity_t *cent, qhandle_t shader ) {
	int				rf;
	refEntity_t		ent;

	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson ) {
		rf = RF_THIRD_PERSON;		// only show in mirrors
	} else {
		rf = 0;
	}

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene( &ent );
}



/*
===============
CG_PlayerFloatSprite

Same as above but allows custom RGBA values
===============
*/
#if 0
static void CG_PlayerFloatSpriteRGBA( centity_t *cent, qhandle_t shader, vec4_t rgba ) {
	int				rf;
	refEntity_t		ent;

	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson ) {
		rf = RF_THIRD_PERSON;		// only show in mirrors
	} else {
		rf = 0;
	}

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = rgba[0];
	ent.shaderRGBA[1] = rgba[1];
	ent.shaderRGBA[2] = rgba[2];
	ent.shaderRGBA[3] = rgba[3];
	trap_R_AddRefEntityToScene( &ent );
}
#endif


/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites( centity_t *cent ) {
//	int		team;

	if (cg.snap &&
		CG_IsMindTricked(cent->currentState.trickedentindex,
		cent->currentState.trickedentindex2,
		cent->currentState.trickedentindex3,
		cent->currentState.trickedentindex4,
		cg.snap->ps.clientNum))
	{
		return; //this entity is mind-tricking the current client, so don't render it
	}

	if ( cent->currentState.eFlags & EF_CONNECTION ) {
		CG_PlayerFloatSprite( cent, cgs.media.connectionShader );
		return;
	}

	if (cent->vChatTime > cg.time)
	{
		CG_PlayerFloatSprite( cent, cgs.media.vchatShader );
	}
	else if ( cent->currentState.eType != ET_NPC && //don't draw talk balloons on NPCs
		(cent->currentState.eFlags & EF_TALK) )
	{
		CG_PlayerFloatSprite( cent, cgs.media.balloonShader );
		return;
	}
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define	SHADOW_DISTANCE		128
static qboolean CG_PlayerShadow( centity_t *cent, float *shadowPlane ) {
	vec3_t		end, mins = {-15, -15, 0}, maxs = {15, 15, 2};
	trace_t		trace;
	float		alpha;
	float		radius = 24.0f;

	*shadowPlane = 0;

	if ( cg_shadows.integer == 0 ) {
		return qfalse;
	}

	// no shadows when cloaked
	if ( cent->currentState.powerups & ( 1 << PW_CLOAKED )) 
	{
		return qfalse;
	}

	if (cent->currentState.eFlags & EF_DEAD)
	{
		return qfalse;
	}

	if (CG_IsMindTricked(cent->currentState.trickedentindex,
		cent->currentState.trickedentindex2,
		cent->currentState.trickedentindex3,
		cent->currentState.trickedentindex4,
		cg.snap->ps.clientNum))
	{
		return qfalse; //this entity is mind-tricking the current client, so don't render it
	}

	//[NPCSandCreature]
	if ( cent->currentState.NPC_class == CLASS_SAND_CREATURE )
	//wahoo  - disable shadows for el suckers, toerhwise a big blob of shadow shows up in the middle of nowhere
    {//sand creatures have no shadow 
        return qfalse;
    }
	//[/NPCSandCreature]

	if ( cg_shadows.integer == 1 )
	{//dropshadow
		if (cent->currentState.m_iVehicleNum &&
			cent->currentState.NPC_class != CLASS_VEHICLE )
		{//riding a vehicle, no dropshadow
			return qfalse;
		}
	}
	// send a trace down from the player to the ground
	VectorCopy( cent->lerpOrigin, end );
	if (cg_shadows.integer == 2)
	{ //stencil
		end[2] -= 4096.0f;

		trap_CM_BoxTrace( &trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID );

		if ( trace.fraction == 1.0 || trace.startsolid || trace.allsolid )
		{
			trace.endpos[2] = cent->lerpOrigin[2]-25.0f;
		}
	}
	else
	{
		end[2] -= SHADOW_DISTANCE;

		trap_CM_BoxTrace( &trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID );

		// no shadow if too high
		if ( trace.fraction == 1.0 || trace.startsolid || trace.allsolid ) {
			return qfalse;
		}
	}

	if (cg_shadows.integer == 2)
	{ //stencil shadows need plane to be on ground
		*shadowPlane = trace.endpos[2];
	}
	else
	{
		*shadowPlane = trace.endpos[2] + 1;
	}

	if ( cg_shadows.integer != 1 ) {	// no mark for stencil or projection shadows
		return qtrue;
	}

	// fade the shadow out with height
	alpha = 1.0 - trace.fraction;

	// bk0101022 - hack / FPE - bogus planes?
	//assert( DotProduct( trace.plane.normal, trace.plane.normal ) != 0.0f ) 

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	if ( cent->currentState.NPC_class == CLASS_REMOTE
		|| cent->currentState.NPC_class == CLASS_SEEKER )
	{
		radius = 8.0f;
	}
	CG_ImpactMark( cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal, 
		cent->pe.legs.yawAngle, alpha,alpha,alpha,1, qfalse, radius, qtrue );

	return qtrue;
}


/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
static void CG_PlayerSplash( centity_t *cent ) {
	vec3_t		start, end;
	trace_t		trace;
	int			contents;
	polyVert_t	verts[4];

	if ( !cg_shadows.integer ) {
		return;
	}

	VectorCopy( cent->lerpOrigin, end );
	end[2] -= 24;

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = trap_CM_PointContents( end, 0 );
	if ( !( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) ) {
		return;
	}

	VectorCopy( cent->lerpOrigin, start );
	start[2] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = trap_CM_PointContents( start, 0 );
	if ( contents & ( CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		return;
	}

	// trace down to find the surface
	trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) );

	if ( trace.fraction == 1.0 ) {
		return;
	}

	// create a mark polygon
	VectorCopy( trace.endpos, verts[0].xyz );
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[1].xyz );
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[2].xyz );
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[3].xyz );
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.wakeMarkShader, 4, verts );
}

#define REFRACT_EFFECT_DURATION		500
static void CG_ForcePushBlur( vec3_t org, centity_t *cent )
{
	if (!cent || !cg_renderToTextureFX.integer)
	{
		localEntity_t	*ex;

		ex = CG_AllocLocalEntity();
		ex->leType = LE_PUFF;
		ex->refEntity.reType = RT_SPRITE;
		ex->radius = 2.0f;
		ex->startTime = cg.time;
		ex->endTime = ex->startTime + 120;
		VectorCopy( org, ex->pos.trBase );
		ex->pos.trTime = cg.time;
		ex->pos.trType = TR_LINEAR;
		VectorScale( cg.refdef.viewaxis[1], 55, ex->pos.trDelta );
			
		ex->color[0] = 24;
		ex->color[1] = 32;
		ex->color[2] = 40;
		ex->refEntity.customShader = trap_R_RegisterShader( "gfx/effects/forcePush" );

		ex = CG_AllocLocalEntity();
		ex->leType = LE_PUFF;
		ex->refEntity.reType = RT_SPRITE;
		ex->refEntity.rotation = 180.0f;
		ex->radius = 2.0f;
		ex->startTime = cg.time;
		ex->endTime = ex->startTime + 120;
		VectorCopy( org, ex->pos.trBase );
		ex->pos.trTime = cg.time;
		ex->pos.trType = TR_LINEAR;
		VectorScale( cg.refdef.viewaxis[1], -55, ex->pos.trDelta );
			
		ex->color[0] = 24;
		ex->color[1] = 32;
		ex->color[2] = 40;
		ex->refEntity.customShader = trap_R_RegisterShader( "gfx/effects/forcePush" );
	}
	else
	{ //superkewl "refraction" (well sort of) effect -rww
		refEntity_t ent;
		vec3_t ang;
		float scale;
		float vLen;
		float alpha;
		int tDif;

		if (!cent->bodyFadeTime)
		{ //the duration for the expansion and fade
			cent->bodyFadeTime = cg.time + REFRACT_EFFECT_DURATION;
		}

		//closer tDif is to 0, the closer we are to
		//being "done"
		tDif = (cent->bodyFadeTime - cg.time);

		if ((REFRACT_EFFECT_DURATION-tDif) < 200)
		{ //stop following the hand after a little and stay in a fixed spot
			//save the initial spot of the effect
			VectorCopy(org, cent->pushEffectOrigin);
		}

		//scale from 1.0f to 0.1f then hold at 0.1 for the rest of the duration
		if (cent->currentState.powerups & (1 << PW_PULL))
		{
			scale = (float)(REFRACT_EFFECT_DURATION-tDif)*0.003f;
		}
		else
		{
			scale = (float)(tDif)*0.003f;
		}

		if (scale > 1.0f)
		{
			scale = 1.0f;
		}
		else if (scale < 0.2f)
		{
			scale = 0.2f;
		}

		//start alpha at 244, fade to 10
		alpha = (float)tDif*0.488f;

		if (alpha > 244.0f)
		{
			alpha = 244.0f;
		}
		else if (alpha < 10.0f)
		{
			alpha = 10.0f;
		}

		memset( &ent, 0, sizeof( ent ) );
		ent.shaderTime = (cent->bodyFadeTime-REFRACT_EFFECT_DURATION) / 1000.0f;

		VectorCopy( cent->pushEffectOrigin, ent.origin );

		VectorSubtract(ent.origin, cg.refdef.vieworg, ent.axis[0]);
		vLen = VectorLength(ent.axis[0]);
		if (vLen <= 0.1f)
		{	// Entity is right on vieworg.  quit.
			return;
		}

		vectoangles(ent.axis[0], ang);
		ang[ROLL] += 180.0f;
		AnglesToAxis(ang, ent.axis);

		//radius must be a power of 2, and is the actual captured texture size
		if (vLen < 128)
		{
			ent.radius = 256;
		}
		else if (vLen < 256)
		{
			ent.radius = 128;
		}
		else if (vLen < 512)
		{
			ent.radius = 64;
		}
		else
		{
			ent.radius = 32;
		}

		VectorScale(ent.axis[0], scale, ent.axis[0]);
		VectorScale(ent.axis[1], scale, ent.axis[1]);
		VectorScale(ent.axis[2], scale, ent.axis[2]);

		ent.hModel = cgs.media.halfShieldModel;
		ent.customShader = cgs.media.refractionShader; //cgs.media.cloakedShader;
		ent.nonNormalizedAxes = qtrue;

		//make it partially transparent so it blends with the background
		ent.renderfx = (RF_DISTORTION|RF_FORCE_ENT_ALPHA);
		ent.shaderRGBA[0] = 255.0f;
		ent.shaderRGBA[1] = 255.0f;
		ent.shaderRGBA[2] = 255.0f;
		ent.shaderRGBA[3] = alpha;

		trap_R_AddRefEntityToScene( &ent );
	}
}

static const char *cg_pushBoneNames[] =
{
	"cranium",
	"lower_lumbar",
	"rhand",
	"lhand",
	"ltibia",
	"rtibia",
	"lradius",
	"rradius",
	NULL
};

static void CG_ForcePushBodyBlur( centity_t *cent )
{
	vec3_t fxOrg;
	mdxaBone_t	boltMatrix;
	int bolt;
	int i;

	if (cent->localAnimIndex > 1)
	{ //Sorry, the humanoid IS IN ANOTHER CASTLE.
		return;
	}

	if (cg.snap &&
		CG_IsMindTricked(cent->currentState.trickedentindex,
		cent->currentState.trickedentindex2,
		cent->currentState.trickedentindex3,
		cent->currentState.trickedentindex4,
		cg.snap->ps.clientNum))
	{
		return; //this entity is mind-tricking the current client, so don't render it
	}

	assert(cent->ghoul2);

	for (i = 0; cg_pushBoneNames[i]; i++)
	{ //go through all the bones we want to put a blur effect on
		bolt = trap_G2API_AddBolt(cent->ghoul2, 0, cg_pushBoneNames[i]);

		if (bolt == -1)
		{
			assert(!"You've got an invalid bone/bolt name in cg_pushBoneNames");
			continue;
		}

		trap_G2API_GetBoltMatrix(cent->ghoul2, 0, bolt, &boltMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, fxOrg);

		//standard effect, don't be refractive (for now)
		CG_ForcePushBlur(fxOrg, NULL);
	}
}

static void CG_ForceGripEffect( vec3_t org )
{
	localEntity_t	*ex;
	float wv = sin( cg.time * 0.004f ) * 0.08f + 0.1f;

	ex = CG_AllocLocalEntity();
	ex->leType = LE_PUFF;
	ex->refEntity.reType = RT_SPRITE;
	ex->radius = 2.0f;
	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 120;
	VectorCopy( org, ex->pos.trBase );
	ex->pos.trTime = cg.time;
	ex->pos.trType = TR_LINEAR;
	VectorScale( cg.refdef.viewaxis[1], 55, ex->pos.trDelta );
		
	ex->color[0] = 200+((wv*255));
	if (ex->color[0] > 255)
	{
		ex->color[0] = 255;
	}
	ex->color[1] = 0;
	ex->color[2] = 0;
	ex->refEntity.customShader = trap_R_RegisterShader( "gfx/effects/forcePush" );

	ex = CG_AllocLocalEntity();
	ex->leType = LE_PUFF;
	ex->refEntity.reType = RT_SPRITE;
	ex->refEntity.rotation = 180.0f;
	ex->radius = 2.0f;
	ex->startTime = cg.time;
	ex->endTime = ex->startTime + 120;
	VectorCopy( org, ex->pos.trBase );
	ex->pos.trTime = cg.time;
	ex->pos.trType = TR_LINEAR;
	VectorScale( cg.refdef.viewaxis[1], -55, ex->pos.trDelta );

	/*
	ex->color[0] = 200+((wv*255));
	if (ex->color[0] > 255)
	{
		ex->color[0] = 255;
	}
	*/
	ex->color[0] = 255;
	ex->color[1] = 255;
	ex->color[2] = 255;
	ex->refEntity.customShader = cgs.media.redSaberGlowShader;//trap_R_RegisterShader( "gfx/effects/forcePush" );
}


/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
===============
*/
void CG_AddRefEntityWithPowerups( refEntity_t *ent, entityState_t *state, int team ) {

	if (CG_IsMindTricked(state->trickedentindex,
		state->trickedentindex2,
		state->trickedentindex3,
		state->trickedentindex4,
		cg.snap->ps.clientNum))
	{
		return; //this entity is mind-tricking the current client, so don't render it
	}

	trap_R_AddRefEntityToScene( ent );
}

#define MAX_SHIELD_TIME	2000.0
#define MIN_SHIELD_TIME	2000.0


void CG_PlayerShieldHit(int entitynum, vec3_t dir, int amount)
{
	centity_t *cent;
	int	time;

	if (entitynum<0 || entitynum >= MAX_ENTITIES)
	{
		return;
	}

	cent = &cg_entities[entitynum];

	if (amount > 100)
	{
		time = cg.time + MAX_SHIELD_TIME;		// 2 sec.
	}
	else
	{
		time = cg.time + 500 + amount*15;
	}

	if (time > cent->damageTime)
	{
		cent->damageTime = time;
		VectorScale(dir, -1, dir);
		vectoangles(dir, cent->damageAngles);
	}
}


void CG_DrawPlayerShield(centity_t *cent, vec3_t origin)
{
	refEntity_t ent;
	int			alpha;
	float		scale;
	
	// Don't draw the shield when the player is dead.
	if (cent->currentState.eFlags & EF_DEAD)
	{
		return;
	}

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( origin, ent.origin );
	ent.origin[2] += 10.0;
	AnglesToAxis( cent->damageAngles, ent.axis );

	alpha = 255.0 * ((cent->damageTime - cg.time) / MIN_SHIELD_TIME) + random()*16;
	if (alpha>255)
		alpha=255;

	// Make it bigger, but tighter if more solid
	scale = 1.4 - ((float)alpha*(0.4/255.0));		// Range from 1.0 to 1.4
	VectorScale( ent.axis[0], scale, ent.axis[0] );
	VectorScale( ent.axis[1], scale, ent.axis[1] );
	VectorScale( ent.axis[2], scale, ent.axis[2] );

	ent.hModel = cgs.media.halfShieldModel;
	ent.customShader = cgs.media.halfShieldShader;
	ent.shaderRGBA[0] = alpha;
	ent.shaderRGBA[1] = alpha;
	ent.shaderRGBA[2] = alpha;
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene( &ent );
}


void CG_PlayerHitFX(centity_t *cent)
{
	// only do the below fx if the cent in question is...uh...me, and it's first person.
	if (cent->currentState.clientNum != cg.predictedPlayerState.clientNum || cg.renderingThirdPerson)
	{
		if (cent->damageTime > cg.time
			&& cent->currentState.NPC_class != CLASS_VEHICLE )
		{
			CG_DrawPlayerShield(cent, cent->lerpOrigin);
		}

		return;
	}
}



/*
=================
CG_LightVerts
=================
*/
int CG_LightVerts( vec3_t normal, int numVerts, polyVert_t *verts )
{
	int				i, j;
	float			incoming;
	vec3_t			ambientLight;
	vec3_t			lightDir;
	vec3_t			directedLight;

	trap_R_LightForPoint( verts[0].xyz, ambientLight, directedLight, lightDir );

	for (i = 0; i < numVerts; i++) {
		incoming = DotProduct (normal, lightDir);
		if ( incoming <= 0 ) {
			verts[i].modulate[0] = ambientLight[0];
			verts[i].modulate[1] = ambientLight[1];
			verts[i].modulate[2] = ambientLight[2];
			verts[i].modulate[3] = 255;
			continue;
		} 
		j = ( ambientLight[0] + incoming * directedLight[0] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[0] = j;

		j = ( ambientLight[1] + incoming * directedLight[1] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[1] = j;

		j = ( ambientLight[2] + incoming * directedLight[2] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[2] = j;

		verts[i].modulate[3] = 255;
	}
	return qtrue;
}


//[RGBSabers]
void RGB_LerpColor(vec3_t from, vec3_t to, float frac, vec3_t out)
{
	vec3_t diff;
	int i;

	VectorSubtract(to,from,diff);

	VectorCopy(from,out);

	for(i=0;i<3;i++)
		out[i] += diff[i] * frac;

}

int getint(char **buf)
{
	double temp;
	temp = strtod(*buf,buf);
	return (int)temp;
}

void ParseRGBSaber(char * str, vec3_t c)
{
	char *p = str;
	int i;

	for(i=0;i<3;i++)
	{
		c[i] = getint(&p);
		p++;
	}
}

void CG_ParseScriptedSaber(char *script, clientInfo_t *ci, int snum)
{
	int n =0,l;
	char *p = script;

	l = strlen(p);
	p++; //skip the 1st ':'

	while(p[0] && p-script < l && n<10)
	{
		ParseRGBSaber(p,ci->ScriptedColors[n][snum]);
		while(p[0]!=':')
			p++;
		p++;            //skipped 1st point 

		ci->ScriptedTimes[n][snum] = getint(&p);
		
		p++;
		n++;
	}
	ci->ScriptedNum[snum] = n;

}

void RGB_AdjustSciptedSaberColor(clientInfo_t *ci, vec3_t color, int n)
{
	int actual;
	float frac;

	if(!ci->ScriptedStartTime[n])
	{
		ci->ScriptedActualNum[n] = 0;
		ci->ScriptedStartTime[n] = cg.time;
		ci->ScriptedEndTime[n] = cg.time + ci->ScriptedTimes[0][n];
	}
	else if(ci->ScriptedEndTime[n] < cg.time)
	{
		ci->ScriptedActualNum[n] = (ci->ScriptedActualNum[n] + 1) % ci->ScriptedNum[n];
		actual = ci->ScriptedActualNum[n];
		ci->ScriptedStartTime[n] = cg.time;
		ci->ScriptedEndTime[n] = cg.time + ci->ScriptedTimes[actual][n];
	}

	actual = ci->ScriptedActualNum[n];

	frac = (float)(cg.time - ci->ScriptedStartTime[n]) / (float)(ci->ScriptedEndTime[n] - ci->ScriptedStartTime[n]);


	if(actual+1 != ci->ScriptedNum[n])
		RGB_LerpColor(ci->ScriptedColors[actual][n], ci->ScriptedColors[actual+1][n], frac, color);
	else
		RGB_LerpColor(ci->ScriptedColors[actual][n], ci->ScriptedColors[0][n], frac, color);

}



#define PIMP_MIN_INTESITY 120

void RGB_RandomRGB(vec3_t c)
{
	int i;
	for(i=0;i<3;i++)
		c[i]=0;

	while(c[0]+c[1]+c[2] < PIMP_MIN_INTESITY)
		for(i=0;i<3;i++)
			c[i] = rand() % 255;

}

void RGB_AdjustPimpSaberColor(clientInfo_t *ci, vec3_t color, int n)
{
	int time;
	float frac;

	if(!ci->PimpStartTime[n])
	{
		ci->PimpStartTime[n] = cg.time;
		RGB_RandomRGB(ci->PimpColorFrom[n]);
		RGB_RandomRGB(ci->PimpColorTo[n]);
		time = 250 + rand()%250;
//		time = 500 + rand()%250;
		ci->PimpEndTime[n] = cg.time + time;
	}
	else if(ci->PimpEndTime[n] < cg.time)
	{
		VectorCopy(ci->PimpColorTo[n], ci->PimpColorFrom[n]);
		RGB_RandomRGB(ci->PimpColorTo[n]);
		time = 250 + rand()%250;
		ci->PimpStartTime[n] = cg.time;
		ci->PimpEndTime[n] = cg.time + time;
	}

	frac = (float)(cg.time - ci->PimpStartTime[n]) / (float)(ci->PimpEndTime[n] - ci->PimpStartTime[n]);

//	Com_Printf("frac : %f\n",frac);

	RGB_LerpColor(ci->PimpColorFrom[n], ci->PimpColorTo[n], frac, color);

}


static void CG_RGBForSaberColor( saber_colors_t color, vec3_t rgb, int cnum, int bnum )
//[/RGBSabers]
{
	switch( color )
	{
		case SABER_RED:
			VectorSet( rgb, 1.0f, 0.2f, 0.2f );
			break;
		case SABER_ORANGE:
			VectorSet( rgb, 1.0f, 0.5f, 0.1f );
			break;
		case SABER_YELLOW:
			VectorSet( rgb, 1.0f, 1.0f, 0.2f );
			break;
		case SABER_GREEN:
			VectorSet( rgb, 0.2f, 1.0f, 0.2f );
			break;
		case SABER_BLUE:
			VectorSet( rgb, 0.2f, 0.4f, 1.0f );
			break;
		case SABER_PURPLE:
			VectorSet( rgb, 0.9f, 0.2f, 1.0f );
			break;
//[RGBSabers]
		case SABER_RGB:
			{
				if(cnum < MAX_CLIENTS)
				{
					int i;
					clientInfo_t *ci = &cgs.clientinfo[cnum];

					if(bnum == 0)
						VectorCopy(ci->rgb1, rgb);
					else
						VectorCopy(ci->rgb2, rgb);
					for(i=0;i<3;i++)
						rgb[i]/=255;
				}
				else
					VectorSet( rgb, 0.2f, 0.4f, 1.0f );
			}
			break;
		case SABER_PIMP:
			{
				if(cnum < MAX_CLIENTS)
				{
					int i;
					clientInfo_t *ci = &cgs.clientinfo[cnum];

					RGB_AdjustPimpSaberColor(ci, rgb,bnum);

					for(i=0;i<3;i++)
						rgb[i]/=255;
				}
				else
					VectorSet( rgb, 0.2f, 0.4f, 1.0f );
			}
			break;
		case SABER_SCRIPTED:
			{
				if(cnum < MAX_CLIENTS)
				{
					int i;
					clientInfo_t *ci = &cgs.clientinfo[cnum];

					RGB_AdjustSciptedSaberColor(ci, rgb,bnum);

					for(i=0;i<3;i++)
						rgb[i]/=255;
				}
				else
					VectorSet( rgb, 0.2f, 0.4f, 1.0f );
			}

			break;
		case SABER_BLACK:
			VectorSet( rgb, 0.0f, 0.0f, 0.0f );
			break;
		case SABER_WHITE:
			VectorSet( rgb, 1.0f, 1.0f, 1.0f );
			break;
	}
//	Com_Printf("sabercolor %i %i %i ^1%i %i\n",(int)rgb[0],(int)rgb[1],(int)rgb[2],cnum,bnum);
}

static void CG_DoSaberLight( saberInfo_t *saber , int cnum, int bnum)
//[/RGBSabers]
{
	vec3_t		positions[MAX_BLADES*2], mid={0}, rgbs[MAX_BLADES*2], rgb={0};
	float		lengths[MAX_BLADES*2]={0}, totallength = 0, numpositions = 0, dist, diameter = 0;
	int			i, j;

	//RGB combine all the colors of the sabers you're using into one averaged color!
	if ( !saber )
	{ 
		return;
	}
	
	if ( (saber->saberFlags2&SFL2_NO_DLIGHT) )
	{//no dlight!
		return;
	}

	for ( i = 0; i < saber->numBlades; i++ )
	{
		if ( saber->blade[i].length >= 0.5f )
		{
			//FIXME: make RGB sabers
			//[RGBSabers]
			CG_RGBForSaberColor( saber->blade[i].color, rgbs[i] , cnum, bnum);
			//[/RGBSabers]
			lengths[i] = saber->blade[i].length;
			if ( saber->blade[i].length*2.0f > diameter )
			{
				diameter = saber->blade[i].length*2.0f;
			}
			totallength += saber->blade[i].length;
			VectorMA( saber->blade[i].muzzlePoint, saber->blade[i].length, saber->blade[i].muzzleDir, positions[i] );
			if ( !numpositions )
			{//first blade, store middle of that as midpoint
				VectorMA( saber->blade[i].muzzlePoint, saber->blade[i].length*0.5, saber->blade[i].muzzleDir, mid );
				VectorCopy( rgbs[i], rgb );
			}
			numpositions++;
		}
	}

	if ( totallength )
	{//actually have something to do	
		if ( numpositions == 1 )
		{//only 1 blade, midpoint is already set (halfway between the start and end of that blade), rgb is already set, so it diameter
		}
		else
		{//multiple blades, calc averages
			VectorClear( mid );
			VectorClear( rgb );
			//now go through all the data and get the average RGB and middle position and the radius
			for ( i = 0; i < MAX_BLADES*2; i++ )
			{
				if ( lengths[i] )
				{
					VectorMA( rgb, lengths[i], rgbs[i], rgb );
					VectorAdd( mid, positions[i], mid );
				}
			}

			//get middle rgb
			VectorScale( rgb, 1/totallength, rgb );//get the average, normalized RGB
			//get mid position
			VectorScale( mid, 1/numpositions, mid );
			//find the farthest distance between the blade tips, this will be our diameter
			for ( i = 0; i < MAX_BLADES*2; i++ )
			{
				if ( lengths[i] )
				{
					for ( j = 0; j < MAX_BLADES*2; j++ )
					{
						if ( lengths[j] )
						{
							dist = Distance( positions[i], positions[j] );
							if ( dist > diameter )
							{
								diameter = dist;
							}
						}
					}
				}
			}
		}

	//[RGBSabers]
		trap_R_AddLightToScene( mid, (diameter + random()*8.0f), rgb[0], rgb[1], rgb[2] );
	}
}


void CG_DoSaber( vec3_t origin, vec3_t dir, float length, float lengthMax, float radius, saber_colors_t color, int rfx, qboolean doLight, int cnum, int bnum )
{
	vec3_t		mid;
	qhandle_t	blade = 0, glow = 0;
	refEntity_t saber,sbak;
	float radiusmult;
	float radiusRange;
	float radiusStart;
	vec3_t rgb={1,1,1};
	int i;
	//[/RGBSabers]

	if ( length < 0.5f )
	{
		// if the thing is so short, just forget even adding me.
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA( origin, length * 0.5f, dir, mid );

	switch( color )
	{
		case SABER_RED:
			glow = cgs.media.redSaberGlowShader;
			blade = cgs.media.redSaberCoreShader;
			break;
		case SABER_ORANGE:
			glow = cgs.media.orangeSaberGlowShader;
			blade = cgs.media.orangeSaberCoreShader;
			break;
		case SABER_YELLOW:
			glow = cgs.media.yellowSaberGlowShader;
			blade = cgs.media.yellowSaberCoreShader;
			break;
		case SABER_GREEN:
			glow = cgs.media.greenSaberGlowShader;
			blade = cgs.media.greenSaberCoreShader;
			break;
		case SABER_BLUE:
			glow = cgs.media.blueSaberGlowShader;
			blade = cgs.media.blueSaberCoreShader;
			break;
		case SABER_PURPLE:
			glow = cgs.media.purpleSaberGlowShader;
			blade = cgs.media.purpleSaberCoreShader;
			break;
		//[RGBSabers]
		case SABER_SCRIPTED:
		case SABER_WHITE:
		case SABER_PIMP:
		case SABER_RGB:
			glow = cgs.media.rgbSaberGlowShader;
			blade = cgs.media.rgbSaberCoreShader;
			break;
		case SABER_BLACK:
			glow = cgs.media.blackSaberGlowShader;
			blade = cgs.media.rgbSaberCoreShader;
			break;
		//[/RGBSabers]
		default:
			glow = cgs.media.blueSaberGlowShader;
			blade = cgs.media.blueSaberCoreShader;
			break;
	}

	if (doLight)
	{	// always add a light because sabers cast a nice glow before they slice you in half!!  or something...
		//[RGBSabers]
		float light = length*1.4f + random()*3.0f;
		CG_RGBForSaberColor( color, rgb , cnum, bnum);
		trap_R_AddLightToScene( mid, light, rgb[0], rgb[1], rgb[2] );
		//[/RGBSabers]
	}

	memset( &saber, 0, sizeof( refEntity_t ));

	// Saber glow is it's own ref type because it uses a ton of sprites, otherwise it would eat up too many
	//	refEnts to do each glow blob individually
	saber.saberLength = length;

	// Jeff, I did this because I foolishly wished to have a bright halo as the saber is unleashed.  
	// It's not quite what I'd hoped tho.  If you have any ideas, go for it!  --Pat
	if (length < lengthMax)
	{
		radiusmult = 1.0 + (2.0 / length);		// Note this creates a curve, and length cannot be < 0.5.
	}
	else
	{
		radiusmult = 1.0;
	}

	if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
	{ //draw the blade as a post-render so it doesn't get in the cap...
		rfx |= RF_FORCEPOST;
	}

	//[RGBSabers]
	for(i=0;i<3;i++)
		rgb[i] *= 255;
	//[/RGBSabers]
	radiusRange = radius * 0.075f;
	radiusStart = radius-radiusRange;

	saber.radius = (radiusStart + crandom() * radiusRange)*radiusmult;
	//saber.radius = (2.8f + crandom() * 0.2f)*radiusmult;

	VectorCopy( origin, saber.origin );
	VectorCopy( dir, saber.axis[0] );
	saber.reType = RT_SABER_GLOW;
	saber.customShader = glow;
	//[RGBSabers]
	if(color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
	else
	{
		for(i=0;i<3;i++)
			saber.shaderRGBA[i] = rgb[i];
		saber.shaderRGBA[3] = 255;
	}
	//[/RGBSabers]

	saber.renderfx = rfx;

	trap_R_AddRefEntityToScene( &saber );

	// Do the hot core
	VectorMA( origin, length, dir, saber.origin );
	VectorMA( origin, -1, dir, saber.oldorigin );


//	CG_TestLine(saber.origin, saber.oldorigin, 50, 0x000000ff, 3);
	saber.customShader = blade;
	saber.reType = RT_LINE;
	radiusStart = radius/3.0f;
	saber.radius = (radiusStart + crandom() * radiusRange)*radiusmult;
//	saber.radius = (1.0 + crandom() * 0.2f)*radiusmult;

	saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
	//[RGBSabers]
	if(color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
	else
	{
		for(i=0;i<3;i++)
			saber.shaderRGBA[i] = rgb[i];
		saber.shaderRGBA[3] = 255;
	}

	sbak = saber;
	trap_R_AddRefEntityToScene( &saber );

	if(color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
		return;

	sbak.customShader = cgs.media.rgbSaberCore2Shader;
	saber.reType = RT_LINE;
	saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
	saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
	saber.radius = (radiusStart + crandom() * radiusRange)*radiusmult;
	trap_R_AddRefEntityToScene( &sbak );
	//[/RGBSabers]

}

//--------------------------------------------------------------
// CG_GetTagWorldPosition
//
// Can pass in NULL for the axis
//--------------------------------------------------------------
void CG_GetTagWorldPosition( refEntity_t *model, char *tag, vec3_t pos, vec3_t axis[3] )
{
	orientation_t	orientation;
	int i = 0;

	// Get the requested tag
	trap_R_LerpTag( &orientation, model->hModel, model->oldframe, model->frame,
		1.0f - model->backlerp, tag );

	VectorCopy( model->origin, pos );
	for ( i = 0 ; i < 3 ; i++ ) 
	{
		VectorMA( pos, orientation.origin[i], model->axis[i], pos );
	}

	if ( axis )
	{
		MatrixMultiply( orientation.axis, model->axis, axis );
	}
}

#define	MAX_MARK_FRAGMENTS	128
#define	MAX_MARK_POINTS		384
extern markPoly_t *CG_AllocMark();

void CG_CreateSaberMarks( vec3_t start, vec3_t end, vec3_t normal )
{
//	byte			colors[4];
	int				i, j;
	int				numFragments;
	vec3_t			axis[3], originalPoints[4], mid;
	vec3_t			markPoints[MAX_MARK_POINTS], projection;
	polyVert_t		*v, verts[MAX_VERTS_ON_POLY];
	markPoly_t		*mark;
	markFragment_t	markFragments[MAX_MARK_FRAGMENTS], *mf;

	float	radius = 0.65f;

	if ( !cg_addMarks.integer ) 
	{
		return;
	}

	VectorSubtract( end, start, axis[1] );
	VectorNormalize( axis[1] );

	// create the texture axis
	VectorCopy( normal, axis[0] );
	CrossProduct( axis[1], axis[0], axis[2] );

	// create the full polygon that we'll project
	for ( i = 0 ; i < 3 ; i++ ) 
	{	// stretch a bit more in the direction that we are traveling in...  debateable as to whether this makes things better or worse
		originalPoints[0][i] = start[i] - radius * axis[1][i] - radius * axis[2][i];
		originalPoints[1][i] = end[i] + radius * axis[1][i] - radius * axis[2][i];
		originalPoints[2][i] = end[i] + radius * axis[1][i] + radius * axis[2][i];
		originalPoints[3][i] = start[i] - radius * axis[1][i] + radius * axis[2][i];
	}

	VectorScale( normal, -1, projection );

	// get the fragments
	numFragments = trap_CM_MarkFragments( 4, (const float (*)[3])originalPoints,
					projection, MAX_MARK_POINTS, markPoints[0], MAX_MARK_FRAGMENTS, markFragments );

	for ( i = 0, mf = markFragments ; i < numFragments ; i++, mf++ ) 
	{
		// we have an upper limit on the complexity of polygons that we store persistantly
		if ( mf->numPoints > MAX_VERTS_ON_POLY ) 
		{
			mf->numPoints = MAX_VERTS_ON_POLY;
		}

		for ( j = 0, v = verts ; j < mf->numPoints ; j++, v++ ) 
		{
			vec3_t delta;

			// Set up our texture coords, this may need some work 
			VectorCopy( markPoints[mf->firstPoint + j], v->xyz );
			VectorAdd( end, start, mid );
			VectorScale( mid, 0.5f, mid );
			VectorSubtract( v->xyz, mid, delta );

			v->st[0] = 0.5 + DotProduct( delta, axis[1] ) * (0.05f + random() * 0.03f); 
			v->st[1] = 0.5 + DotProduct( delta, axis[2] ) * (0.15f + random() * 0.05f);	
		}

		if (cg_saberDynamicMarks.integer)
		{
			int i = 0;
			int i_2 = 0;
			addpolyArgStruct_t apArgs;
			vec3_t x;

			memset (&apArgs, 0, sizeof(apArgs));

			while (i < 4)
			{
				while (i_2 < 3)
				{
					apArgs.p[i][i_2] = verts[i].xyz[i_2];

					i_2++;
				}

				i_2 = 0;
				i++;
			}

			i = 0;
			i_2 = 0;

			while (i < 4)
			{
				while (i_2 < 2)
				{
					apArgs.ev[i][i_2] = verts[i].st[i_2];

					i_2++;
				}

				i_2 = 0;
				i++;
			}

			//When using addpoly, having a situation like this tends to cause bad results.
			//(I assume it doesn't like trying to draw a polygon over two planes and extends
			//the vertex out to some odd value)
			VectorSubtract(apArgs.p[0], apArgs.p[3], x);
			if (VectorLength(x) > 3.0f)
			{
				return;
			}

			apArgs.numVerts = mf->numPoints;
			VectorCopy(vec3_origin, apArgs.vel);
			VectorCopy(vec3_origin, apArgs.accel);

			apArgs.alpha1 = 1.0f;
			apArgs.alpha2 = 0.0f;
			apArgs.alphaParm = 255.0f;

			VectorSet(apArgs.rgb1, 0.0f, 0.0f, 0.0f);
			VectorSet(apArgs.rgb2, 0.0f, 0.0f, 0.0f);

			apArgs.rgbParm = 0.0f;

			apArgs.bounce = 0;
			apArgs.motionDelay = 0;
			apArgs.killTime = cg_saberDynamicMarkTime.integer;
			apArgs.shader = cgs.media.rivetMarkShader;
			apArgs.flags = 0x08000000|0x00000004;

			trap_FX_AddPoly(&apArgs);

			apArgs.shader = cgs.media.mSaberDamageGlow;
			apArgs.rgb1[0] = 215 + random() * 40.0f;
			apArgs.rgb1[1] = 96 + random() * 32.0f;
			apArgs.rgb1[2] = apArgs.alphaParm = random()*15.0f;

			apArgs.rgb1[0] /= 255;
			apArgs.rgb1[1] /= 255;
			apArgs.rgb1[2] /= 255;
			VectorCopy(apArgs.rgb1, apArgs.rgb2);

			apArgs.killTime = 100;

			trap_FX_AddPoly(&apArgs);
		}
		else
		{
			// save it persistantly, do burn first
			mark = CG_AllocMark();
			mark->time = cg.time;
			mark->alphaFade = qtrue;
			mark->markShader = cgs.media.rivetMarkShader;
			mark->poly.numVerts = mf->numPoints;
			mark->color[0] = mark->color[1] = mark->color[2] = mark->color[3] = 255;
			memcpy( mark->verts, verts, mf->numPoints * sizeof( verts[0] ) );

			// And now do a glow pass
			// by moving the start time back, we can hack it to fade out way before the burn does
			mark = CG_AllocMark();
			mark->time = cg.time - 8500;
			mark->alphaFade = qfalse;
			mark->markShader = cgs.media.mSaberDamageGlow;
			mark->poly.numVerts = mf->numPoints;
			mark->color[0] = 215 + random() * 40.0f;
			mark->color[1] = 96 + random() * 32.0f;
			mark->color[2] = mark->color[3] = random()*15.0f;
			memcpy( mark->verts, verts, mf->numPoints * sizeof( verts[0] ) );
		}
	}
}

qboolean CG_G2TraceCollide(trace_t *tr, vec3_t const mins, vec3_t const maxs, const vec3_t lastValidStart, const vec3_t lastValidEnd)
{
	G2Trace_t		G2Trace;
	centity_t		*g2Hit;
	vec3_t			angles;
	int				tN = 0;
	float			fRadius = 0.0f;

	if (mins && maxs &&
		(mins[0] || maxs[0]))
	{
		fRadius=(maxs[0]-mins[0])/2.0f;
	}

	memset (&G2Trace, 0, sizeof(G2Trace));

	while (tN < MAX_G2_COLLISIONS)
	{
		G2Trace[tN].mEntityNum = -1;
		tN++;
	}
	g2Hit = &cg_entities[tr->entityNum];

	if (g2Hit && g2Hit->ghoul2)
	{
		angles[ROLL] = angles[PITCH] = 0;
		angles[YAW] = g2Hit->lerpAngles[YAW];

		if (cg_optvehtrace.integer &&
			g2Hit->currentState.eType == ET_NPC &&
			g2Hit->currentState.NPC_class == CLASS_VEHICLE &&
			g2Hit->m_pVehicle)
		{
			trap_G2API_CollisionDetectCache ( G2Trace, g2Hit->ghoul2, angles, g2Hit->lerpOrigin, cg.time, g2Hit->currentState.number, lastValidStart, lastValidEnd, g2Hit->modelScale, 0, cg_g2TraceLod.integer, fRadius );
		}
		else
		{
			trap_G2API_CollisionDetect ( G2Trace, g2Hit->ghoul2, angles, g2Hit->lerpOrigin, cg.time, g2Hit->currentState.number, lastValidStart, lastValidEnd, g2Hit->modelScale, 0, cg_g2TraceLod.integer, fRadius );
		}

		if (G2Trace[0].mEntityNum != g2Hit->currentState.number)
		{
			tr->fraction = 1.0f;
			tr->entityNum = ENTITYNUM_NONE;
			tr->startsolid = 0;
			tr->allsolid = 0;
			return qfalse;
		}
		else
		{ //Yay!
			VectorCopy(G2Trace[0].mCollisionPosition, tr->endpos);
			VectorCopy(G2Trace[0].mCollisionNormal, tr->plane.normal);
			return qtrue;
		}
	}

	return qfalse;
}

void CG_G2SaberEffects(vec3_t start, vec3_t end, centity_t *owner)
{
	trace_t trace;
	vec3_t startTr;
	vec3_t endTr;
	qboolean backWards = qfalse;
	qboolean doneWithTraces = qfalse;

	while (!doneWithTraces)
	{
		if (!backWards)
		{
			VectorCopy(start, startTr);
			VectorCopy(end, endTr);
		}
		else
		{
			VectorCopy(end, startTr);
			VectorCopy(start, endTr);
		}

		CG_Trace( &trace, startTr, NULL, NULL, endTr, owner->currentState.number, MASK_PLAYERSOLID );

		if (trace.entityNum < MAX_CLIENTS)
		{ //hit a client..
			CG_G2TraceCollide(&trace, NULL, NULL, startTr, endTr);

			if (trace.entityNum != ENTITYNUM_NONE)
			{ //it succeeded with the ghoul2 trace
				trap_FX_PlayEffectID( cgs.effects.mSaberBloodSparks, trace.endpos, trace.plane.normal, -1, -1 );
				trap_S_StartSound(trace.endpos, trace.entityNum, CHAN_AUTO, trap_S_RegisterSound(va("sound/weapons/saber/saberhit%i.wav", Q_irand(1, 3))));
			}
		}

		if (!backWards)
		{
			backWards = qtrue;
		}
		else
		{
			doneWithTraces = qtrue;
		}
	}
}

#define CG_MAX_SABER_COMP_TIME 400 //last registered saber entity hit must match within this many ms for the client effect to take place.

void CG_AddGhoul2Mark(int shader, float size, vec3_t start, vec3_t end, int entnum,
					  vec3_t entposition, float entangle, void *ghoul2, vec3_t scale, int lifeTime)
{
	SSkinGoreData goreSkin;

	assert(ghoul2);

	memset ( &goreSkin, 0, sizeof(goreSkin) );

	if (trap_G2API_GetNumGoreMarks(ghoul2, 0) >= cg_ghoul2Marks.integer)
	{ //you've got too many marks already
		return;
	}

	goreSkin.growDuration = -1; // default expandy time
	goreSkin.goreScaleStartFraction = 1.0; // default start scale
	goreSkin.frontFaces = qtrue;
	goreSkin.backFaces = qtrue;
	goreSkin.lifeTime = lifeTime; //last randomly 10-20 seconds
	/*
	if (lifeTime)
	{
		goreSkin.fadeOutTime = lifeTime*0.1; //default fade duration is relative to lifetime.
	}
	goreSkin.fadeRGB = qtrue; //fade on RGB instead of alpha (this depends on the shader really, modify if needed)
	*/
	//rwwFIXMEFIXME: fade has sorting issues with other non-fading decals, disabled until fixed

	goreSkin.baseModelOnly = qfalse;
	
	goreSkin.currentTime = cg.time;
	goreSkin.entNum      = entnum;
	goreSkin.SSize		 = size;
	goreSkin.TSize		 = size;
	goreSkin.theta		 = flrand(0.0f,6.28f);
	goreSkin.shader		 = shader;

	if (!scale[0] && !scale[1] && !scale[2])
	{
		VectorSet(goreSkin.scale, 1.0f, 1.0f, 1.0f);
	}
	else
	{
		VectorCopy(goreSkin.scale, scale);
	}

	VectorCopy (start, goreSkin.hitLocation);

	VectorSubtract(end, start, goreSkin.rayDirection);
	if (VectorNormalize(goreSkin.rayDirection)<.1f)
	{
		return;
	}

	VectorCopy ( entposition, goreSkin.position );
	goreSkin.angles[YAW] = entangle;

	trap_G2API_AddSkinGore(ghoul2, &goreSkin);
}

void CG_SaberCompWork(vec3_t start, vec3_t end, centity_t *owner, int saberNum, int bladeNum)
{
	trace_t trace;
	vec3_t startTr;
	vec3_t endTr;
	qboolean backWards = qfalse;
	qboolean doneWithTraces = qfalse;
	qboolean doEffect = qfalse;
	clientInfo_t *client = NULL;

	if ((cg.time - owner->serverSaberHitTime) > CG_MAX_SABER_COMP_TIME)
	{
		return;
	}

	if (cg.time == owner->serverSaberHitTime)
	{ //don't want to do it the same frame as the server hit, to avoid burst effect concentrations every x ms.
		return;
	}

	while (!doneWithTraces)
	{
		if (!backWards)
		{
			VectorCopy(start, startTr);
			VectorCopy(end, endTr);
		}
		else
		{
			VectorCopy(end, startTr);
			VectorCopy(start, endTr);
		}

		CG_Trace( &trace, startTr, NULL, NULL, endTr, owner->currentState.number, MASK_PLAYERSOLID );

		if (trace.entityNum == owner->serverSaberHitIndex)
		{ //this is the guy the server says we last hit, so continue.
			if (cg_entities[trace.entityNum].ghoul2)
			{ //If it has a g2 instance, do the proper ghoul2 checks
				CG_G2TraceCollide(&trace, NULL, NULL, startTr, endTr);

				if (trace.entityNum != ENTITYNUM_NONE)
				{ //it succeeded with the ghoul2 trace
					doEffect = qtrue;

					if (cg_ghoul2Marks.integer)
					{
						vec3_t ePos;
						centity_t *trEnt = &cg_entities[trace.entityNum];

						if (trEnt->ghoul2)
						{
							if (trEnt->currentState.eType != ET_NPC ||
								trEnt->currentState.NPC_class != CLASS_VEHICLE ||
								!trEnt->m_pVehicle ||
								trEnt->m_pVehicle->m_pVehicleInfo->type != VH_FIGHTER)
							{ //don't do on fighters cause they have crazy full axial angles
								int weaponMarkShader = 0, markShader = cgs.media.bdecal_saberglow;

								VectorSubtract(endTr, trace.endpos, ePos);
								VectorNormalize(ePos);
								VectorMA(trace.endpos, 4.0f, ePos, ePos);

								if (owner->currentState.eType == ET_NPC)
								{
									client = owner->npcClient;
								}
								else
								{
									client = &cgs.clientinfo[owner->currentState.clientNum];
								}
								if ( client 
									&& client->infoValid )
								{
									if ( WP_SaberBladeUseSecondBladeStyle( &client->saber[saberNum], bladeNum ) )
									{
										if ( client->saber[saberNum].g2MarksShader2 )
										{//we have a shader to use instead of the standard mark shader
											markShader = client->saber[saberNum].g2MarksShader2;
										}
										if ( client->saber[saberNum].g2WeaponMarkShader2 )
										{//we have a shader to use as a splashback onto the weapon model
											weaponMarkShader = client->saber[saberNum].g2WeaponMarkShader2;
										}
									}
									else
									{
										if ( client->saber[saberNum].g2MarksShader )
										{//we have a shader to use instead of the standard mark shader
											markShader = client->saber[saberNum].g2MarksShader;
										}
										if ( client->saber[saberNum].g2WeaponMarkShader )
										{//we have a shader to use as a splashback onto the weapon model
											weaponMarkShader = client->saber[saberNum].g2WeaponMarkShader;
										}
									}
								}
								//ROP VEHICLE_IMP START
								//Cannot be marked if we are a vehicle which - well - can't be marked ;)
								if(!(trEnt->currentState.NPC_class == CLASS_VEHICLE 
									&& trEnt->m_pVehicle && trEnt->m_pVehicle->m_pVehicleInfo->ResistsMarking))
								{
									//[RGBSabers]
									CG_AddGhoul2Mark(markShader, flrand(3.0f, 4.0f),
										trace.endpos, ePos, trace.entityNum, trEnt->lerpOrigin, trEnt->lerpAngles[YAW],
										trEnt->ghoul2, trEnt->modelScale, Q_irand(5000, 10000));
									if ( weaponMarkShader )
									{
										vec3_t splashBackDir;
										VectorScale( ePos, -1 , splashBackDir );
										CG_AddGhoul2Mark(weaponMarkShader, flrand(0.5f, 2.0f),
											trace.endpos, splashBackDir, owner->currentState.clientNum, owner->lerpOrigin, owner->lerpAngles[YAW],
											owner->ghoul2, owner->modelScale, Q_irand(5000, 10000));
									}
									//[/RGBSabers]
								}
								//ROP VEHICLE_IMP END
							}
						}
					}
				}
			}
			else
			{ //otherwise, we're all set.
				doEffect = qtrue;
			}

			if (doEffect)
			{
				int hitPersonFxID = cgs.effects.mSaberBloodSparks;
				int hitOtherFxID = cgs.effects.mSaberCut;

				if (owner->currentState.eType == ET_NPC)
				{
					client = owner->npcClient;
				}
				else
				{
					client = &cgs.clientinfo[owner->currentState.clientNum];
				}
				if ( client && client->infoValid )
				{
					if ( WP_SaberBladeUseSecondBladeStyle( &client->saber[saberNum], bladeNum ) )
					{//use second blade style values
						if ( client->saber[saberNum].hitPersonEffect2 )
						{
							hitPersonFxID = client->saber[saberNum].hitPersonEffect2;
						}
						if ( client->saber[saberNum].hitOtherEffect2 )
						{//custom hit other effect
							hitOtherFxID = client->saber[saberNum].hitOtherEffect2;
						}
					}
					else
					{//use first blade style values
						if ( client->saber[saberNum].hitPersonEffect )
						{
							hitPersonFxID = client->saber[saberNum].hitPersonEffect;
						}
						if ( client->saber[saberNum].hitOtherEffect )
						{//custom hit other effect
							hitOtherFxID = client->saber[saberNum].hitOtherEffect;
						}
					}
				}
				if (!trace.plane.normal[0] && !trace.plane.normal[1] && !trace.plane.normal[2])
				{ //who cares, just shoot it somewhere.
					trace.plane.normal[1] = 1;
				}

				if (owner->serverSaberFleshImpact)
				{ //do standard player/live ent hit sparks
					trap_FX_PlayEffectID( hitPersonFxID, trace.endpos, trace.plane.normal, -1, -1 );
					//trap_S_StartSound(trace.endpos, trace.entityNum, CHAN_AUTO, trap_S_RegisterSound(va("sound/weapons/saber/saberhit%i.wav", Q_irand(1, 3))));
				}
				else
				{ //do the cut effect
					trap_FX_PlayEffectID( hitOtherFxID, trace.endpos, trace.plane.normal, -1, -1 );
				}
				doEffect = qfalse;
			}
		}

		/*
		if (!backWards)
		{
			backWards = qtrue;
		}
		else
		{
			doneWithTraces = qtrue;
		}
		*/
		doneWithTraces = qtrue; //disabling backwards tr for now, sometimes it just makes too many effects.
	}
}


#define SABER_TRAIL_TIME	40.0f
#define FX_USE_ALPHA		0x08000000

#include "../namespace_begin.h"
qboolean BG_SuperBreakWinAnim( int anim );
#include "../namespace_end.h"

void CG_AddSaberBlade( centity_t *cent, centity_t *scent, refEntity_t *saber, int renderfx, int modelIndex, int saberNum, int bladeNum, vec3_t origin, vec3_t angles, qboolean fromSaber, qboolean dontDraw)
{
	vec3_t	org_, end, v,
			axis_[3] = {0,0,0, 0,0,0, 0,0,0};	// shut the compiler up
	trace_t	trace;
	int i = 0;
	int trailDur;
	float saberLen;
	float diff;
	clientInfo_t *client;
	centity_t *saberEnt;
	saberTrail_t *saberTrail;
	mdxaBone_t	boltMatrix;
	vec3_t futureAngles;
	effectTrailArgStruct_t fx;
	int scolor = 0;
	int	useModelIndex = 0;

	if (cent->currentState.eType == ET_NPC)
	{
		client = cent->npcClient;
		assert(client);
	}
	else
	{
		client = &cgs.clientinfo[cent->currentState.number];
	}

	saberEnt = &cg_entities[cent->currentState.saberEntityNum];
	saberLen = client->saber[saberNum].blade[bladeNum].length;

	if (saberLen <= 0 && !dontDraw)
	{ //don't bother then.
		return;
	}

	futureAngles[YAW] = angles[YAW];
	futureAngles[PITCH] = angles[PITCH];
	futureAngles[ROLL] = angles[ROLL];


	if ( fromSaber )
	{
		useModelIndex = 0;
	}
	else
	{
		useModelIndex = saberNum+1;
	}
	//Assume bladeNum is equal to the bolt index because bolts should be added in order of the blades.
	//if there is an effect on this blade, play it
	if (  !WP_SaberBladeUseSecondBladeStyle( &client->saber[saberNum], bladeNum )
			&& client->saber[saberNum].bladeEffect )
	{
		trap_FX_PlayBoltedEffectID(client->saber[saberNum].bladeEffect, scent->lerpOrigin, 
			scent->ghoul2, bladeNum, scent->currentState.number, useModelIndex, -1, qfalse);
	}
	else if ( WP_SaberBladeUseSecondBladeStyle( &client->saber[saberNum], bladeNum )
			&& client->saber[saberNum].bladeEffect2 )
	{
		trap_FX_PlayBoltedEffectID(client->saber[saberNum].bladeEffect2, scent->lerpOrigin, 
			scent->ghoul2, bladeNum, scent->currentState.number, useModelIndex, -1, qfalse);
	}
	//get the boltMatrix
	trap_G2API_GetBoltMatrix(scent->ghoul2, useModelIndex, bladeNum, &boltMatrix, futureAngles, origin, cg.time, cgs.gameModels, scent->modelScale);

	// work the matrix axis stuff into the original axis and origins used.
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, org_);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, axis_[0]);

	if (!fromSaber && saberEnt && !cent->currentState.saberInFlight)
	{
		VectorCopy(org_, saberEnt->currentState.pos.trBase);

		VectorCopy(axis_[0], saberEnt->currentState.apos.trBase);
	}

	VectorMA( org_, saberLen, axis_[0], end );
	
	VectorAdd( end, axis_[0], end );

	if (cent->currentState.eType == ET_NPC)
	{
		scolor = client->saber[saberNum].blade[bladeNum].color;
	}
	else
	{
		if (saberNum == 0)
		{
			scolor = client->icolor1;
		}
		else
		{
			scolor = client->icolor2;
		}
	}

	//[RGBSabers]
	if(((ojp_teamrgbsabers.integer < 1) 
		|| (ojp_teamrgbsabers.integer == 1 && cg.snap && cg.snap->ps.clientNum != cent->currentState.number)) &&
		cgs.gametype >= GT_TEAM &&
		cgs.gametype != GT_SIEGE &&
		!cgs.jediVmerc &&
		cent->currentState.eType != ET_NPC)
	/*
	if (cgs.gametype >= GT_TEAM &&
		cgs.gametype != GT_SIEGE &&
		!cgs.jediVmerc &&
		cent->currentState.eType != ET_NPC)
	*/
	//[/RGBSabers]
	{//racc - in a team game where we want to override the saber colors.
		if (client->team == TEAM_RED)
		{
			scolor = SABER_RED;
		}
		else if (client->team == TEAM_BLUE)
		{
			scolor = SABER_BLUE;
		}
	}


	if (!cg_saberContact.integer)
	{ //if we don't have saber contact enabled, just add the blade and don't care what it's touching
		goto CheckTrail;
	}

	if (!dontDraw)
	{
		if (cg_saberModelTraceEffect.integer)
		{
			CG_G2SaberEffects(org_, end, cent);
		}
		else if (cg_saberClientVisualCompensation.integer)
		{
			CG_Trace( &trace, org_, NULL, NULL, end, ENTITYNUM_NONE, MASK_SOLID );

			if (trace.fraction != 1)
			{ //nudge the endpos a very small amount from the beginning to the end, so the comp trace hits at the end.
			//I'm only bothering with this because I want to do a backwards trace too in the comp trace, so if the
			//blade is sticking through a player or something the standard trace doesn't it, it will make sparks
			//on each side.
				vec3_t seDif;

				VectorSubtract(trace.endpos, org_, seDif);
				VectorNormalize(seDif);
				trace.endpos[0] += seDif[0]*0.1f;
				trace.endpos[1] += seDif[1]*0.1f;
				trace.endpos[2] += seDif[2]*0.1f;
			}

			if (client->saber[saberNum].blade[bladeNum].storageTime < cg.time)
			{ //debounce it in case our framerate is absurdly high. Using storageTime since it's not used for anything else in the client.
				CG_SaberCompWork(org_, trace.endpos, cent, saberNum, bladeNum);

				client->saber[saberNum].blade[bladeNum].storageTime = cg.time + 5;
			}
		}

		for ( i = 0; i < 1; i++ )//was 2 because it would go through architecture and leave saber trails on either side of the brush - but still looks bad if we hit a corner, blade is still 8 longer than hit
		{
			if ( i )
			{//tracing from end to base
				CG_Trace( &trace, end, NULL, NULL, org_, ENTITYNUM_NONE, MASK_SOLID );
			}
			else
			{//tracing from base to end
				CG_Trace( &trace, org_, NULL, NULL, end, ENTITYNUM_NONE, MASK_SOLID );
			}
			
			if ( trace.fraction < 1.0f )
			{
				vec3_t trDir;
				VectorCopy(trace.plane.normal, trDir);
				if (!trDir[0] && !trDir[1] && !trDir[2])
				{
					trDir[1] = 1;
				}

				if ( (client->saber[saberNum].saberFlags2&SFL2_NO_WALL_MARKS) )
				{//don't actually draw the marks/impact effects
				}
				else
				{
					if (!(trace.surfaceFlags & SURF_NOIMPACT) ) // never spark on sky
					{
						trap_FX_PlayEffectID( cgs.effects.mSparks, trace.endpos, trDir, -1, -1 );
					}
				}

				//Stop saber? (it wouldn't look right if it was stuck through a thin wall and unable to hurt players on the other side)
				VectorSubtract(org_, trace.endpos, v);
				saberLen = VectorLength(v);

				VectorCopy(trace.endpos, end);

				if ( (client->saber[saberNum].saberFlags2&SFL2_NO_WALL_MARKS) )
				{//don't actually draw the marks
				}
				else
				{//draw marks if we hit a wall
					// All I need is a bool to mark whether I have a previous point to work with.
					//....come up with something better..
					if ( client->saber[saberNum].blade[bladeNum].trail.haveOldPos[i] )
					{
						if ( trace.entityNum == ENTITYNUM_WORLD || cg_entities[trace.entityNum].currentState.eType == ET_TERRAIN || (cg_entities[trace.entityNum].currentState.eFlags & EF_PERMANENT) )
						{//only put marks on architecture
							// Let's do some cool burn/glowing mark bits!!!
							CG_CreateSaberMarks( client->saber[saberNum].blade[bladeNum].trail.oldPos[i], trace.endpos, trace.plane.normal );
						
							//make a sound
							if ( cg.time - client->saber[saberNum].blade[bladeNum].hitWallDebounceTime >= 100 )
							{//ugh, need to have a real sound debouncer... or do this game-side
								client->saber[saberNum].blade[bladeNum].hitWallDebounceTime = cg.time;
								trap_S_StartSound ( trace.endpos, -1, CHAN_WEAPON, trap_S_RegisterSound( va("sound/weapons/saber/saberhitwall%i", Q_irand(1, 3)) ) );
							}
						}
					}
					else
					{
						// if we impact next frame, we'll mark a slash mark
						client->saber[saberNum].blade[bladeNum].trail.haveOldPos[i] = qtrue;
		//				CG_ImpactMark( cgs.media.rivetMarkShader, client->saber[saberNum].blade[bladeNum].trail.oldPos[i], client->saber[saberNum].blade[bladeNum].trail.oldNormal[i],
		//						0.0f, 1.0f, 1.0f, 1.0f, 1.0f, qfalse, 1.1f, qfalse );
					}
				}

				// stash point so we can connect-the-dots later
				VectorCopy( trace.endpos, client->saber[saberNum].blade[bladeNum].trail.oldPos[i] );
				VectorCopy( trace.plane.normal, client->saber[saberNum].blade[bladeNum].trail.oldNormal[i] );
			}
			else
			{
				if ( client->saber[saberNum].blade[bladeNum].trail.haveOldPos[i] )
				{
					// Hmmm, no impact this frame, but we have an old point
					// Let's put the mark there, we should use an endcap mark to close the line, but we 
					//	can probably just get away with a round mark
	//					CG_ImpactMark( cgs.media.rivetMarkShader, client->saber[saberNum].blade[bladeNum].trail.oldPos[i], client->saber[saberNum].blade[bladeNum].trail.oldNormal[i],
	//							0.0f, 1.0f, 1.0f, 1.0f, 1.0f, qfalse, 1.1f, qfalse );
				}

				// we aren't impacting, so turn off our mark tracking mechanism
				client->saber[saberNum].blade[bladeNum].trail.haveOldPos[i] = qfalse;
			}
		}
	}
CheckTrail:

	if (!cg_saberTrail.integer)
	{ //don't do the trail in this case
		goto JustDoIt;
	}

	if ( (!WP_SaberBladeUseSecondBladeStyle( &client->saber[saberNum], bladeNum ) && client->saber[saberNum].trailStyle > 1 )
		 || ( WP_SaberBladeUseSecondBladeStyle( &client->saber[saberNum], bladeNum ) && client->saber[saberNum].trailStyle2 > 1 ) )
	{//don't actually draw the trail at all
		goto JustDoIt;
	}

	//FIXME: if trailStyle is 1, use the motion blur instead

	saberTrail = &client->saber[saberNum].blade[bladeNum].trail;
	saberTrail->duration = saberMoveData[cent->currentState.saberMove].trailLength;

	trailDur = (saberTrail->duration/5.0f);
	if (!trailDur)
	{ //hmm.. ok, default
		if ( BG_SuperBreakWinAnim(cent->currentState.torsoAnim) )
		{
			trailDur = 150;
		}
		else
		{
			trailDur = SABER_TRAIL_TIME;
		}
	}

	// if we happen to be timescaled or running in a high framerate situation, we don't want to flood
	//	the system with very small trail slices...but perhaps doing it by distance would yield better results?
	if ( cg.time > saberTrail->lastTime + 2 || cg_saberTrail.integer == 2 ) // 2ms
	{
		if (!dontDraw)
		{
			if ( (BG_SuperBreakWinAnim(cent->currentState.torsoAnim) || saberMoveData[cent->currentState.saberMove].trailLength > 0 || ((cent->currentState.powerups & (1 << PW_SPEED) && cg_speedTrail.integer)) || (cent->currentState.saberInFlight && saberNum == 0)) && cg.time < saberTrail->lastTime + 2000 ) // if we have a stale segment, don't draw until we have a fresh one
			{
	#if 0
				if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
				{
					polyVert_t	verts[4];

					VectorCopy( org_, verts[0].xyz );
					VectorMA( end, 3.0f, axis_[0], verts[1].xyz );
					VectorCopy( saberTrail->tip, verts[2].xyz );
					VectorCopy( saberTrail->base, verts[3].xyz );

					//tc doesn't even matter since we're just gonna stencil an outline, but whatever.
					verts[0].st[0] = 0;	
					verts[0].st[1] = 0;	
					verts[0].modulate[0] = 255;	
					verts[0].modulate[1] = 255;	
					verts[0].modulate[2] = 255;	
					verts[0].modulate[3] = 255;

					verts[1].st[0] = 0;	
					verts[1].st[1] = 1;	
					verts[1].modulate[0] = 255;	
					verts[1].modulate[1] = 255;	
					verts[1].modulate[2] = 255;	
					verts[1].modulate[3] = 255;

					verts[2].st[0] = 1;	
					verts[2].st[1] = 1;	
					verts[2].modulate[0] = 255;	
					verts[2].modulate[1] = 255;	
					verts[2].modulate[2] = 255;	
					verts[2].modulate[3] = 255;

					verts[3].st[0] = 1;	
					verts[3].st[1] = 0;	
					verts[3].modulate[0] = 255;	
					verts[3].modulate[1] = 255;	
					verts[3].modulate[2] = 255;	
					verts[3].modulate[3] = 255;

					//don't capture postrender objects (now we'll postrender the saber so it doesn't get in the capture)
					trap_R_SetRefractProp(1.0f, 0.0f, qtrue, qtrue);

					//shader 2 is always the crazy refractive shader.
					trap_R_AddPolyToScene( 2, 4, verts );
				}
				else
	#endif
				{
					vec3_t	rgb1={255.0f,255.0f,255.0f};

					switch( scolor )
					{
						case SABER_RED:
							VectorSet( rgb1, 255.0f, 0.0f, 0.0f );
							break;
						case SABER_ORANGE:
							VectorSet( rgb1, 255.0f, 64.0f, 0.0f );
							break;
						case SABER_YELLOW:
							VectorSet( rgb1, 255.0f, 255.0f, 0.0f );
							break;
						case SABER_GREEN:
							VectorSet( rgb1, 0.0f, 255.0f, 0.0f );
							break;
						case SABER_BLUE:
							VectorSet( rgb1, 0.0f, 64.0f, 255.0f );
							break;
						case SABER_PURPLE:
							VectorSet( rgb1, 220.0f, 0.0f, 255.0f );
							break;
						//[RGBSabers]
						case SABER_RGB:
							{
								int cnum = cent->currentState.clientNum;
								if(cnum < MAX_CLIENTS)
								{
									clientInfo_t *ci = &cgs.clientinfo[cnum];

									if(saberNum == 0)
										VectorCopy(ci->rgb1, rgb1);
									else
										VectorCopy(ci->rgb2, rgb1);
								}
								else
								VectorSet( rgb1, 0.0f, 64.0f, 255.0f );
							}
							break;
						case SABER_PIMP:
							{
								int cnum = cent->currentState.clientNum;
								if(cnum < MAX_CLIENTS)
								{
									clientInfo_t *ci = &cgs.clientinfo[cnum];
									RGB_AdjustPimpSaberColor(ci,rgb1,saberNum);

								}
								else
								VectorSet( rgb1, 0.0f, 64.0f, 255.0f );

							}
							break;
						case SABER_SCRIPTED:
							{
								int cnum = cent->currentState.clientNum;
								if(cnum < MAX_CLIENTS)
								{
									clientInfo_t *ci = &cgs.clientinfo[cnum];
									RGB_AdjustSciptedSaberColor(ci,rgb1,saberNum);

								}
								else
								VectorSet( rgb1, 0.0f, 64.0f, 255.0f );

							}
							break;
						case SABER_BLACK:
							VectorSet( rgb1, 1.0f, 1.0f, 1.0f );
							break;
						case SABER_WHITE:
							VectorSet( rgb1, 1.0f, 1.0f, 1.0f );
							break;
						//[/RGBSabers]
						default:
							VectorSet( rgb1, 0.0f, 64.0f, 255.0f );
							break;
					}

					//Here we will use the happy process of filling a struct in with arguments and passing it to a trap function
					//so that we can take the struct and fill in an actual CTrail type using the data within it once we get it
					//into the effects area

					// Go from new muzzle to new end...then to old end...back down to old muzzle...finally
					//	connect back to the new muzzle...this is our trail quad
					VectorCopy( org_, fx.mVerts[0].origin );
					VectorMA( end, 3.0f, axis_[0], fx.mVerts[1].origin );

					VectorCopy( saberTrail->tip, fx.mVerts[2].origin );
					VectorCopy( saberTrail->base, fx.mVerts[3].origin );

					diff = cg.time - saberTrail->lastTime;

					// I'm not sure that clipping this is really the best idea
					//This prevents the trail from showing at all in low framerate situations.
					//if ( diff <= SABER_TRAIL_TIME * 2 )
					if ( diff <= 10000 )
					{ //don't draw it if the last time is way out of date
						float oldAlpha = 1.0f - ( diff / trailDur );

						if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
						{//does other stuff below
						}
						else
						{
							if ( (!WP_SaberBladeUseSecondBladeStyle( &client->saber[saberNum], bladeNum ) && client->saber[saberNum].trailStyle == 1 )
								|| ( WP_SaberBladeUseSecondBladeStyle( &client->saber[saberNum], bladeNum ) && client->saber[saberNum].trailStyle2 == 1 ) )
							{//motion trail
								fx.mShader = cgs.media.swordTrailShader;
								VectorSet( rgb1, 32.0f, 32.0f, 32.0f ); // make the sith sword trail pretty faint
								trailDur *= 2.0f; // stay around twice as long?
							} 
							else 
							{ 
								fx.mShader = cgs.media.saberBlurShader;
							}
							fx.mKillTime = trailDur;
							fx.mSetFlags = FX_USE_ALPHA;
						}

						// New muzzle
						VectorCopy( rgb1, fx.mVerts[0].rgb );
						fx.mVerts[0].alpha = 255.0f;

						fx.mVerts[0].ST[0] = 0.0f;
						fx.mVerts[0].ST[1] = 1.0f;
						fx.mVerts[0].destST[0] = 1.0f;
						fx.mVerts[0].destST[1] = 1.0f;

						// new tip
						VectorCopy( rgb1, fx.mVerts[1].rgb );
						fx.mVerts[1].alpha = 255.0f;
						
						fx.mVerts[1].ST[0] = 0.0f;
						fx.mVerts[1].ST[1] = 0.0f;
						fx.mVerts[1].destST[0] = 1.0f;
						fx.mVerts[1].destST[1] = 0.0f;

						// old tip
						VectorCopy( rgb1, fx.mVerts[2].rgb );
						fx.mVerts[2].alpha = 255.0f;

						fx.mVerts[2].ST[0] = 1.0f - oldAlpha; // NOTE: this just happens to contain the value I want
						fx.mVerts[2].ST[1] = 0.0f;
						fx.mVerts[2].destST[0] = 1.0f + fx.mVerts[2].ST[0];
						fx.mVerts[2].destST[1] = 0.0f;

						// old muzzle
						VectorCopy( rgb1, fx.mVerts[3].rgb );
						fx.mVerts[3].alpha = 255.0f;

						fx.mVerts[3].ST[0] = 1.0f - oldAlpha; // NOTE: this just happens to contain the value I want
						fx.mVerts[3].ST[1] = 1.0f;
						fx.mVerts[3].destST[0] = 1.0f + fx.mVerts[2].ST[0];
						fx.mVerts[3].destST[1] = 1.0f;

						if (cg_saberTrail.integer == 2 && cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4)
						{
							trap_R_SetRefractProp(1.0f, 0.0f, qtrue, qtrue); //don't need to do this every frame.. but..

							if (BG_SaberInAttack(cent->currentState.saberMove)
								||BG_SuperBreakWinAnim(cent->currentState.torsoAnim))
							{ //in attack, strong trail
								fx.mKillTime = 300;
							}
							else
							{ //faded trail
								fx.mKillTime = 40;
							}
							fx.mShader = 2; //2 is always refractive shader
							fx.mSetFlags = FX_USE_ALPHA;
						}
						/*
						else
						{
							fx.mShader = cgs.media.saberBlurShader;
							fx.mKillTime = trailDur;
							fx.mSetFlags = FX_USE_ALPHA;
						}
						*/
						//[RGBSabers]
						if(scolor == SABER_BLACK)
							fx.mShader = cgs.media.blackSaberTrail;
						//[/RGBSabers]

						trap_FX_AddPrimitive(&fx);
					}
				}
			}
		}

		// we must always do this, even if we aren't active..otherwise we won't know where to pick up from
		VectorCopy( org_, saberTrail->base );
		VectorMA( end, 3.0f, axis_[0], saberTrail->tip );
		saberTrail->lastTime = cg.time;
	}

JustDoIt:

	if (dontDraw)
	{
		return;
	}

	if ( (client->saber[saberNum].saberFlags2&SFL2_NO_BLADE) )
	{//don't actually draw the blade at all
		if ( client->saber[saberNum].numBlades < 3
			&& !(client->saber[saberNum].saberFlags2&SFL2_NO_DLIGHT) )
		{//hmm, but still add the dlight
			//[RGBSabers]
			CG_DoSaberLight( &client->saber[saberNum] ,cent->currentState.clientNum, saberNum);
			//[/RGBSabers]
		}
		return;
	}
	// Pass in the renderfx flags attached to the saber weapon model...this is done so that saber glows
	//	will get rendered properly in a mirror...not sure if this is necessary??
	//CG_DoSaber( org_, axis_[0], saberLen, client->saber[saberNum].blade[bladeNum].lengthMax, client->saber[saberNum].blade[bladeNum].radius,
	//	scolor, renderfx, (qboolean)(saberNum==0&&bladeNum==0) );
	CG_DoSaber( org_, axis_[0], saberLen, client->saber[saberNum].blade[bladeNum].lengthMax, client->saber[saberNum].blade[bladeNum].radius,
		//[RGBSabers]
		scolor, renderfx, (qboolean)(client->saber[saberNum].numBlades < 3 && !(client->saber[saberNum].saberFlags2&SFL2_NO_DLIGHT)),
		cent->currentState.clientNum, saberNum);
		//[/RGBSabers]
}

int CG_IsMindTricked(int trickIndex1, int trickIndex2, int trickIndex3, int trickIndex4, int client)
{
	int checkIn;
	int sub = 0;

	if (cg_entities[client].currentState.forcePowersActive & (1 << FP_SEE))
	{
		return 0;
	}

	if (client > 47)
	{
		checkIn = trickIndex4;
		sub = 48;
	}
	else if (client > 31)
	{
		checkIn = trickIndex3;
		sub = 32;
	}
	else if (client > 15)
	{
		checkIn = trickIndex2;
		sub = 16;
	}
	else
	{
		checkIn = trickIndex1;
	}

	if (checkIn & (1 << (client-sub)))
	{
		return 1;
	}
	
	return 0;
}

#define SPEED_TRAIL_DISTANCE 6

void CG_DrawPlayerSphere(centity_t *cent, vec3_t origin, float scale, int shader)
{
	refEntity_t ent;
	vec3_t ang;
	float vLen;
	vec3_t viewDir;
	
	// Don't draw the shield when the player is dead.
	if (cent->currentState.eFlags & EF_DEAD)
	{
		return;
	}

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( origin, ent.origin );
	ent.origin[2] += 9.0;

	VectorSubtract(ent.origin, cg.refdef.vieworg, ent.axis[0]);
	vLen = VectorLength(ent.axis[0]);
	if (vLen <= 0.1f)
	{	// Entity is right on vieworg.  quit.
		return;
	}

	VectorCopy(ent.axis[0], viewDir);
	VectorInverse(viewDir);
	VectorNormalize(viewDir);

	vectoangles(ent.axis[0], ang);
	ang[ROLL] += 180.0f;
	ang[PITCH] += 180.0f;
	AnglesToAxis(ang, ent.axis);

	VectorScale(ent.axis[0], scale, ent.axis[0]);
	VectorScale(ent.axis[1], scale, ent.axis[1]);
	VectorScale(ent.axis[2], scale, ent.axis[2]);

	ent.nonNormalizedAxes = qtrue;

	ent.hModel = cgs.media.halfShieldModel;
	ent.customShader = shader;	

	trap_R_AddRefEntityToScene( &ent );

	if (!cg.renderingThirdPerson && cent->currentState.number == cg.predictedPlayerState.clientNum)
	{ //don't do the rest then
		return;
	}
	if (!cg_renderToTextureFX.integer)
	{
		return;
	}

	ang[PITCH] -= 180.0f;
	AnglesToAxis(ang, ent.axis);

	VectorScale(ent.axis[0], scale*0.5f, ent.axis[0]);
	VectorScale(ent.axis[1], scale*0.5f, ent.axis[1]);
	VectorScale(ent.axis[2], scale*0.5f, ent.axis[2]);

	ent.renderfx = (RF_DISTORTION|RF_FORCE_ENT_ALPHA);
	if (shader == cgs.media.invulnerabilityShader)
	{ //ok, ok, this is a little hacky. sorry!
		ent.shaderRGBA[0] = 0;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 0;
		ent.shaderRGBA[3] = 100;
	}
	else if (shader == cgs.media.ysalimariShader)
	{
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 0;
		ent.shaderRGBA[3] = 100;
	}
	else if (shader == cgs.media.endarkenmentShader)
	{
		ent.shaderRGBA[0] = 100;
		ent.shaderRGBA[1] = 0;
		ent.shaderRGBA[2] = 0;
		ent.shaderRGBA[3] = 20;
	}
	else if (shader == cgs.media.enlightenmentShader)
	{
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 20;
	}
	else
	{ //ysal red/blue, boon
		ent.shaderRGBA[0] = 255.0f;
		ent.shaderRGBA[1] = 255.0f;
		ent.shaderRGBA[2] = 255.0f;
		ent.shaderRGBA[3] = 20;
	}

	ent.radius = 256;

	VectorMA(ent.origin, 40.0f, viewDir, ent.origin);

	ent.customShader = trap_R_RegisterShader("effects/refract_2");
	trap_R_AddRefEntityToScene( &ent );
}

void CG_AddLightningBeam(vec3_t start, vec3_t end)
{
	vec3_t	dir, chaos,
			c1, c2,
			v1, v2;
	float	len,
			s1, s2, s3;

	addbezierArgStruct_t b;

	VectorCopy(start, b.start);
	VectorCopy(end, b.end);

	VectorSubtract( b.end, b.start, dir );
	len = VectorNormalize( dir );

	// Get the base control points, we'll work from there
	VectorMA( b.start, 0.3333f * len, dir, c1 );
	VectorMA( b.start, 0.6666f * len, dir, c2 );

	// get some chaos values that really aren't very chaotic :)
	s1 = sin( cg.time * 0.005f ) * 2 + crandom() * 0.2f;
	s2 = sin( cg.time * 0.001f );
	s3 = sin( cg.time * 0.011f );

	VectorSet( chaos, len * 0.01f * s1,
						len * 0.02f * s2,
						len * 0.04f * (s1 + s2 + s3));

	VectorAdd( c1, chaos, c1 );
	VectorScale( chaos, 4.0f, v1 );

	VectorSet( chaos, -len * 0.02f * s3,
						len * 0.01f * (s1 * s2),
						-len * 0.02f * (s1 + s2 * s3));

	VectorAdd( c2, chaos, c2 );
	VectorScale( chaos, 2.0f, v2 );

	VectorSet( chaos, 1.0f, 1.0f, 1.0f );

	VectorCopy(c1, b.control1);
	VectorCopy(vec3_origin, b.control1Vel);
	VectorCopy(c2, b.control2);
	VectorCopy(vec3_origin, b.control2Vel);

	b.size1 = 6.0f;
	b.size2 = 6.0f;
	b.sizeParm = 0.0f;
	b.alpha1 = 0.0f;
	b.alpha2 = 0.2f;
	b.alphaParm = 0.5f;
	
	/*
	VectorCopy(WHITE, b.sRGB);
	VectorCopy(WHITE, b.eRGB);
	*/

	b.sRGB[0] = 255;
	b.sRGB[1] = 255;
	b.sRGB[2] = 255;
	VectorCopy(b.sRGB, b.eRGB);

	b.rgbParm = 0.0f;
	b.killTime = 50;
	b.shader = trap_R_RegisterShader( "gfx/misc/electric2" );
	b.flags = 0x00000001; //FX_ALPHA_LINEAR

	trap_FX_AddBezier(&b);
}

void CG_AddRandomLightning(vec3_t start, vec3_t end)
{
	vec3_t inOrg, outOrg;

	VectorCopy(start, inOrg);
	VectorCopy(end, outOrg);

	if ( rand() & 1 )
	{
		outOrg[0] += Q_irand(0, 24);
		inOrg[0] += Q_irand(0, 8);
	}
	else
	{
		outOrg[0] -= Q_irand(0, 24);
		inOrg[0] -= Q_irand(0, 8);
	}

	if ( rand() & 1 )
	{
		outOrg[1] += Q_irand(0, 24);
		inOrg[1] += Q_irand(0, 8);
	}
	else
	{
		outOrg[1] -= Q_irand(0, 24);
		inOrg[1] -= Q_irand(0, 8);
	}

	if ( rand() & 1 )
	{
		outOrg[2] += Q_irand(0, 50);
		inOrg[2] += Q_irand(0, 40);
	}
	else
	{
		outOrg[2] -= Q_irand(0, 64);
		inOrg[2] -= Q_irand(0, 40);
	}

	CG_AddLightningBeam(inOrg, outOrg);
}

extern char *forceHolocronModels[];

qboolean CG_ThereIsAMaster(void)
{
	int i = 0;
	centity_t *cent;

	while (i < MAX_CLIENTS)
	{
		cent = &cg_entities[i];

		if (cent && cent->currentState.isJediMaster)
		{
			return qtrue;
		}

		i++;
	}

	return qfalse;
}

#if 0
void CG_DrawNoForceSphere(centity_t *cent, vec3_t origin, float scale, int shader)
{
	refEntity_t ent;
	
	// Don't draw the shield when the player is dead.
	if (cent->currentState.eFlags & EF_DEAD)
	{
		return;
	}

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( origin, ent.origin );
	ent.origin[2] += 9.0;

	VectorSubtract(cg.refdef.vieworg, ent.origin, ent.axis[0]);
	if (VectorNormalize(ent.axis[0]) <= 0.1f)
	{	// Entity is right on vieworg.  quit.
		return;
	}

	VectorCopy(cg.refdef.viewaxis[2], ent.axis[2]);
	CrossProduct(ent.axis[0], ent.axis[2], ent.axis[1]);

	VectorScale(ent.axis[0], scale, ent.axis[0]);
	VectorScale(ent.axis[1], scale, ent.axis[1]);
	VectorScale(ent.axis[2], -scale, ent.axis[2]);

	ent.shaderRGBA[3] = (cent->currentState.genericenemyindex - cg.time)/8;
	ent.renderfx |= RF_RGB_TINT;
	if (ent.shaderRGBA[3] > 200)
	{
		ent.shaderRGBA[3] = 200;
	}
	if (ent.shaderRGBA[3] < 1)
	{
		ent.shaderRGBA[3] = 1;
	}

	ent.shaderRGBA[2] = 0;
	ent.shaderRGBA[0] = ent.shaderRGBA[1] = ent.shaderRGBA[3];

	ent.hModel = cgs.media.halfShieldModel;
	ent.customShader = shader;	

	trap_R_AddRefEntityToScene( &ent );
}
#endif

//Checks to see if the model string has a * appended with a custom skin name after.
//If so, it terminates the model string correctly, parses the skin name out, and returns
//the handle of the registered skin.
int CG_HandleAppendedSkin(char *modelName)
{
	char skinName[MAX_QPATH];
	char *p;
	qhandle_t skinID = 0;
	int i = 0;

	//see if it has a skin name
	p = Q_strrchr(modelName, '*');

	if (p)
	{ //found a *, we should have a model name before it and a skin name after it.
		*p = 0; //terminate the modelName string at this point, then go ahead and parse to the next 0 for the skin.
		p++;

		while (p && *p)
		{
			skinName[i] = *p;
			i++;
			p++;
		}
		skinName[i] = 0;

		if (skinName[0]) 
		{ //got it, register the skin under the model path.
			char baseFolder[MAX_QPATH];

			strcpy(baseFolder, modelName);
			p = Q_strrchr(baseFolder, '/'); //go back to the first /, should be the path point

			if (p)
			{ //got it.. terminate at the slash and register.
				char *useSkinName;

				*p = 0;

				if (strchr(skinName, '|'))
				{//three part skin
					useSkinName = va("%s/|%s", baseFolder, skinName);
				}
				else
				{
					useSkinName = va("%s/model_%s.skin", baseFolder, skinName);
				}

				skinID = trap_R_RegisterSkin(useSkinName);
			}
		}
	}

	return skinID;
}

//Create a temporary ghoul2 instance and get the gla name so we can try loading animation data and sounds.
#include "../namespace_begin.h"
void BG_GetVehicleModelName(char *modelname);
void BG_GetVehicleSkinName(char *skinname);
#include "../namespace_end.h"

void CG_CacheG2AnimInfo(char *modelName)
{
	void *g2 = NULL;
	char *slash;
	char useModel[MAX_QPATH];
	char useSkin[MAX_QPATH];
	int animIndex;

	strcpy(useModel, modelName);
	strcpy(useSkin, modelName);

	if (modelName[0] == '$')
	{ //it's a vehicle name actually, let's precache the whole vehicle
		BG_GetVehicleModelName(useModel);
		BG_GetVehicleSkinName(useSkin);
		if ( useSkin[0] )
		{ //use a custom skin
			trap_R_RegisterSkin(va("models/players/%s/model_%s.skin", useModel, useSkin));
		}
		else
		{
			trap_R_RegisterSkin(va("models/players/%s/model_default.skin", useModel));
		}
		strcpy(useModel, va("models/players/%s/model.glm", useModel));
	}

	trap_G2API_InitGhoul2Model(&g2, useModel, 0, 0, 0, 0, 0);

	if (g2)
	{
		char GLAName[MAX_QPATH];
		char originalModelName[MAX_QPATH];

		animIndex = -1;

		GLAName[0] = 0;
		trap_G2API_GetGLAName(g2, 0, GLAName);

		strcpy(originalModelName, useModel);
			
		slash = Q_strrchr( GLAName, '/' );
		if ( slash )
		{
			strcpy(slash, "/animation.cfg");

			animIndex = BG_ParseAnimationFile(GLAName, NULL, qfalse);
		}

		//[RACC] This is true even if the animation set for this model was already set.
		//It's ok thou, the parsing functions are only slow the first time they are run for each set.
		if (animIndex != -1)
		{
			slash = Q_strrchr( originalModelName, '/' );
			if ( slash )
			{
				slash++;
				*slash = 0;
			}

			//[ANIMEVENTS]
			CG_ParseAnimationEvtFile(originalModelName, animIndex, cgNumAnimEvents);
			//BG_ParseAnimationEvtFile(originalModelName, animIndex, cgNumAnimEvents);
			//[/ANIMEVENTS]
		}

		//Now free the temp instance
		trap_G2API_CleanGhoul2Models(&g2);
	}
}

static void CG_RegisterVehicleAssets( Vehicle_t *pVeh )
{
	/*
	if ( pVeh->m_pVehicleInfo->exhaustFX )
	{
		pVeh->m_pVehicleInfo->iExhaustFX = trap_FX_RegisterEffect( pVeh->m_pVehicleInfo->exhaustFX );
	}
	if ( pVeh->m_pVehicleInfo->trailFX )
	{
		pVeh->m_pVehicleInfo->iTrailFX = trap_FX_RegisterEffect( pVeh->m_pVehicleInfo->trailFX );
	}
	if ( pVeh->m_pVehicleInfo->impactFX )
	{
		pVeh->m_pVehicleInfo->iImpactFX = trap_FX_RegisterEffect( pVeh->m_pVehicleInfo->impactFX );
	}
	if ( pVeh->m_pVehicleInfo->explodeFX )
	{
		pVeh->m_pVehicleInfo->iExplodeFX = trap_FX_RegisterEffect( pVeh->m_pVehicleInfo->explodeFX );
	}
	if ( pVeh->m_pVehicleInfo->wakeFX )
	{
		pVeh->m_pVehicleInfo->iWakeFX = trap_FX_RegisterEffect( pVeh->m_pVehicleInfo->wakeFX );
	}

	if ( pVeh->m_pVehicleInfo->dmgFX )
	{
		pVeh->m_pVehicleInfo->iDmgFX = trap_FX_RegisterEffect( pVeh->m_pVehicleInfo->dmgFX );
	}
	if ( pVeh->m_pVehicleInfo->wpn1FX )
	{
		pVeh->m_pVehicleInfo->iWpn1FX = trap_FX_RegisterEffect( pVeh->m_pVehicleInfo->wpn1FX );
	}
	if ( pVeh->m_pVehicleInfo->wpn2FX )
	{
		pVeh->m_pVehicleInfo->iWpn2FX = trap_FX_RegisterEffect( pVeh->m_pVehicleInfo->wpn2FX );
	}
	if ( pVeh->m_pVehicleInfo->wpn1FireFX )
	{
		pVeh->m_pVehicleInfo->iWpn1FireFX = trap_FX_RegisterEffect( pVeh->m_pVehicleInfo->wpn1FireFX );
	}
	if ( pVeh->m_pVehicleInfo->wpn2FireFX )
	{
		pVeh->m_pVehicleInfo->iWpn2FireFX = trap_FX_RegisterEffect( pVeh->m_pVehicleInfo->wpn2FireFX );
	}
	*/
}

extern void CG_HandleNPCSounds(centity_t *cent);

#include "../namespace_begin.h"
extern void G_CreateAnimalNPC( Vehicle_t **pVeh, const char *strAnimalType );
extern void G_CreateSpeederNPC( Vehicle_t **pVeh, const char *strType );
extern void G_CreateWalkerNPC( Vehicle_t **pVeh, const char *strAnimalType );
extern void G_CreateFighterNPC( Vehicle_t **pVeh, const char *strType );
#include "../namespace_end.h"

extern playerState_t *cgSendPS[MAX_GENTITIES];
void CG_G2AnimEntModelLoad(centity_t *cent)
{
	const char *cModelName = CG_ConfigString( CS_MODELS+cent->currentState.modelindex );

	if (!cent->npcClient)
	{ //have not init'd client yet
		return;
	}

	if (cModelName && cModelName[0])
	{
		char modelName[MAX_QPATH];
		int skinID;
		char *slash;

		strcpy(modelName, cModelName);

		if (cent->currentState.NPC_class == CLASS_VEHICLE && modelName[0] == '$')
		{ //vehicles pass their veh names over as model names, then we get the model name from the veh type
			//create a vehicle object clientside for this type
			char *vehType = &modelName[1];
			int iVehIndex = BG_VehicleGetIndex( vehType );
			
			switch( g_vehicleInfo[iVehIndex].type )
			{
				case VH_ANIMAL:
					// Create the animal (making sure all it's data is initialized).
					G_CreateAnimalNPC( &cent->m_pVehicle, vehType );
					break;
				case VH_SPEEDER:
					// Create the speeder (making sure all it's data is initialized).
					G_CreateSpeederNPC( &cent->m_pVehicle, vehType );
					break;
				case VH_FIGHTER:
					// Create the fighter (making sure all it's data is initialized).
					G_CreateFighterNPC( &cent->m_pVehicle, vehType );
					break;
				case VH_WALKER:
					// Create the walker (making sure all it's data is initialized).
					G_CreateWalkerNPC( &cent->m_pVehicle, vehType );
					break;

				default:
					assert(!"vehicle with an unknown type - couldn't create vehicle_t");
					break;
			}
			
			//set up my happy prediction hack
			cent->m_pVehicle->m_vOrientation = &cgSendPS[cent->currentState.number]->vehOrientation[0];

			cent->m_pVehicle->m_pParentEntity = (bgEntity_t *)cent;

			//attach the handles for fx cgame-side
			CG_RegisterVehicleAssets(cent->m_pVehicle);

			BG_GetVehicleModelName(modelName);
			if (cent->m_pVehicle->m_pVehicleInfo->skin &&
				cent->m_pVehicle->m_pVehicleInfo->skin[0])
			{ //use a custom skin
				skinID = trap_R_RegisterSkin(va("models/players/%s/model_%s.skin", modelName, cent->m_pVehicle->m_pVehicleInfo->skin));
			}
			else
			{
				skinID = trap_R_RegisterSkin(va("models/players/%s/model_default.skin", modelName));
			}
			strcpy(modelName, va("models/players/%s/model.glm", modelName));

			//this sound is *only* used for vehicles now
			cgs.media.noAmmoSound = trap_S_RegisterSound( "sound/weapons/noammo.wav" );
		}
		else
		{
			skinID = CG_HandleAppendedSkin(modelName); //get the skin if there is one.
		}

		if (cent->ghoul2)
		{ //clean it first!
            trap_G2API_CleanGhoul2Models(&cent->ghoul2);
		}

		trap_G2API_InitGhoul2Model(&cent->ghoul2, modelName, 0, skinID, 0, 0, 0);

		if (cent->ghoul2)
		{
			char GLAName[MAX_QPATH];
			char originalModelName[MAX_QPATH];
			char *saber;
			int j = 0;

			if (cent->currentState.NPC_class == CLASS_VEHICLE &&
				cent->m_pVehicle)
			{ //do special vehicle stuff
				char strTemp[128];
				int i;

				// Setup the default first bolt
				i = trap_G2API_AddBolt( cent->ghoul2, 0, "model_root" );

				// Setup the droid unit.
				cent->m_pVehicle->m_iDroidUnitTag = trap_G2API_AddBolt( cent->ghoul2, 0, "*droidunit" );

				// Setup the Exhausts.
				for ( i = 0; i < MAX_VEHICLE_EXHAUSTS; i++ )
				{
					Com_sprintf( strTemp, 128, "*exhaust%i", i + 1 );
					cent->m_pVehicle->m_iExhaustTag[i] = trap_G2API_AddBolt( cent->ghoul2, 0, strTemp );
				}

				// Setup the Muzzles.
				for ( i = 0; i < MAX_VEHICLE_MUZZLES; i++ )
				{
					Com_sprintf( strTemp, 128, "*muzzle%i", i + 1 );
					cent->m_pVehicle->m_iMuzzleTag[i] = trap_G2API_AddBolt( cent->ghoul2, 0, strTemp );
					if ( cent->m_pVehicle->m_iMuzzleTag[i] == -1 )
					{//ergh, try *flash?
						Com_sprintf( strTemp, 128, "*flash%i", i + 1 );
						cent->m_pVehicle->m_iMuzzleTag[i] = trap_G2API_AddBolt( cent->ghoul2, 0, strTemp );
					}
				}

				// Setup the Turrets.
				for ( i = 0; i < MAX_VEHICLE_TURRETS; i++ )
				{
					if ( cent->m_pVehicle->m_pVehicleInfo->turret[i].gunnerViewTag )
					{
						cent->m_pVehicle->m_iGunnerViewTag[i] = trap_G2API_AddBolt( cent->ghoul2, 0, cent->m_pVehicle->m_pVehicleInfo->turret[i].gunnerViewTag );
					}
					else
					{
						cent->m_pVehicle->m_iGunnerViewTag[i] = -1;
					}
				}
			}

			if (cent->currentState.npcSaber1)
			{
				saber = (char *)CG_ConfigString(CS_MODELS+cent->currentState.npcSaber1);
				assert(!saber || !saber[0] || saber[0] == '@');
				//valid saber names should always start with '@' for NPCs

				if (saber && saber[0])
				{
					saber++; //skip over the @
					WP_SetSaber(cent->currentState.number, cent->npcClient->saber, 0, saber);
				}
			}
			if (cent->currentState.npcSaber2)
			{
				saber = (char *)CG_ConfigString(CS_MODELS+cent->currentState.npcSaber2);
				assert(!saber || !saber[0] || saber[0] == '@');
				//valid saber names should always start with '@' for NPCs

				if (saber && saber[0])
				{
					saber++; //skip over the @
					WP_SetSaber(cent->currentState.number, cent->npcClient->saber, 1, saber);
				}
			}

			// If this is a not vehicle, give it saber stuff...
			if ( cent->currentState.NPC_class != CLASS_VEHICLE )
			{
				while (j < MAX_SABERS)
				{
					if (cent->npcClient->saber[j].model[0])
					{
						if (cent->npcClient->ghoul2Weapons[j])
						{ //free the old instance(s)
							trap_G2API_CleanGhoul2Models(&cent->npcClient->ghoul2Weapons[j]);
							cent->npcClient->ghoul2Weapons[j] = 0;
						}
						//[VisualWeapons]
						//racc - delete the current ghoul2holsterWeapons so we can load new ones
						if (cent->npcClient->ghoul2HolsterWeapons[j])
						{ //free the old instance(s)
							trap_G2API_CleanGhoul2Models(&cent->npcClient->ghoul2HolsterWeapons[j]);
							cent->npcClient->ghoul2HolsterWeapons[j] = 0;
						}
						//[/VisualWeapons]

						CG_InitG2SaberData(j, cent->npcClient);
					}
					j++;
				}
			}

			trap_G2API_SetSkin(cent->ghoul2, 0, skinID, skinID);

			cent->localAnimIndex = -1;

			GLAName[0] = 0;
			trap_G2API_GetGLAName(cent->ghoul2, 0, GLAName);

			strcpy(originalModelName, modelName);

			if (GLAName[0] &&
				!strstr(GLAName, "players/_humanoid/") /*&&
				!strstr(GLAName, "players/rockettrooper/")*/)
			{ //it doesn't use humanoid anims.				
				slash = Q_strrchr( GLAName, '/' );
				if ( slash )
				{
					strcpy(slash, "/animation.cfg");

					cent->localAnimIndex = BG_ParseAnimationFile(GLAName, NULL, qfalse);
				}
			}
			else
			{ //humanoid index.
				trap_G2API_AddBolt(cent->ghoul2, 0, "*r_hand");
				trap_G2API_AddBolt(cent->ghoul2, 0, "*l_hand");

				//rhand must always be first bolt. lhand always second. Whichever you want the
				//jetpack bolted to must always be third.
				trap_G2API_AddBolt(cent->ghoul2, 0, "*chestg");

				//claw bolts
				trap_G2API_AddBolt(cent->ghoul2, 0, "*r_hand_cap_r_arm");
				trap_G2API_AddBolt(cent->ghoul2, 0, "*l_hand_cap_l_arm");

				if (strstr(GLAName, "players/rockettrooper/"))
				{
					cent->localAnimIndex = 1;
				}
				else
				{
					cent->localAnimIndex = 0;
				}

				if (trap_G2API_AddBolt(cent->ghoul2, 0, "*head_top") == -1)
				{
					trap_G2API_AddBolt(cent->ghoul2, 0, "ceyebrow");
				}
				trap_G2API_AddBolt(cent->ghoul2, 0, "Motion");
			}

			// If this is a not vehicle...
			if ( cent->currentState.NPC_class != CLASS_VEHICLE )
			{
				if (trap_G2API_AddBolt(cent->ghoul2, 0, "lower_lumbar") == -1)
				{ //check now to see if we have this bone for setting anims and such
					cent->noLumbar = qtrue;
				}

				if (trap_G2API_AddBolt(cent->ghoul2, 0, "face") == -1)
				{ //check now to see if we have this bone for setting anims and such
					cent->noFace = qtrue;
				}
			}
			else
			{
				cent->noLumbar = qtrue;
				cent->noFace = qtrue;
			}

			if (cent->localAnimIndex != -1)
			{
				slash = Q_strrchr( originalModelName, '/' );
				if ( slash )
				{
					slash++;
					*slash = 0;
				}

				//[ANIMEVENTS]
				cent->eventAnimIndex = CG_ParseAnimationEvtFile(originalModelName, cent->localAnimIndex, cgNumAnimEvents);
				//cent->eventAnimIndex = BG_ParseAnimationEvtFile(originalModelName, cent->localAnimIndex, bgNumAnimEvents);
				//[/ANIMEVENTS]
			}
		}
	}

	trap_S_ShutUp(qtrue);
	CG_HandleNPCSounds(cent); //handle sound loading here as well.
	trap_S_ShutUp(qfalse);
}

//for now this is just gonna create a big explosion on the area of the surface,
//because I am lazy.
static void CG_CreateSurfaceDebris(centity_t *cent, int surfNum, int fxID, qboolean throwPart)
{
	int lostPartFX = 0;
	//[Asteroids]
	int b = -1;
	//int b;
	vec3_t v, d;
	mdxaBone_t boltMatrix;
	const char *surfName = NULL;	
	//const char *surfName = bgToggleableSurfaces[surfNum];
	if ( surfNum > 0 )
	{
		surfName = bgToggleableSurfaces[surfNum];
	}
	//[/Asteroids]

	if (!cent->ghoul2)
	{ //oh no
		return;
	}

	//let's add the surface as a bolt so we can get the base point of it
	if (bgToggleableSurfaceDebris[surfNum] == 3)
	{ //right wing flame
		b = trap_G2API_AddBolt(cent->ghoul2, 0, "*r_wingdamage");
		if ( throwPart
			&& cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo )
		{
			lostPartFX = cent->m_pVehicle->m_pVehicleInfo->iRWingFX;
		}
	}
	else if (bgToggleableSurfaceDebris[surfNum] == 4)
	{ //left wing flame
		b = trap_G2API_AddBolt(cent->ghoul2, 0, "*l_wingdamage");
		if ( throwPart
			&& cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo )
		{
			lostPartFX = cent->m_pVehicle->m_pVehicleInfo->iLWingFX;
		}
	}
	else if (bgToggleableSurfaceDebris[surfNum] == 5)
	{ //right wing flame 2
		b = trap_G2API_AddBolt(cent->ghoul2, 0, "*r_wingdamage");
		if ( throwPart
			&& cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo )
		{
			lostPartFX = cent->m_pVehicle->m_pVehicleInfo->iRWingFX;
		}
	}
	else if (bgToggleableSurfaceDebris[surfNum] == 6)
	{ //left wing flame 2
		b = trap_G2API_AddBolt(cent->ghoul2, 0, "*l_wingdamage");
		if ( throwPart
			&& cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo )
		{
			lostPartFX = cent->m_pVehicle->m_pVehicleInfo->iLWingFX;
		}
	}
	else if (bgToggleableSurfaceDebris[surfNum] == 7)
	{ //nose flame
		b = trap_G2API_AddBolt(cent->ghoul2, 0, "*nosedamage");
		if ( cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo )
		{
			lostPartFX = cent->m_pVehicle->m_pVehicleInfo->iNoseFX;
		}
	}
	//[Asteroids]
	else if ( surfName )
	//else
	{
		b = trap_G2API_AddBolt(cent->ghoul2, 0, surfName);
	}

	/*
	if (b == -1)
	{ //couldn't find this surface apparently
		return;
	*/
	if (b == -1 || surfNum == -1)
	{ //couldn't find this surface apparently, so play on origin?
		VectorCopy( cent->lerpOrigin, v );
		AngleVectors( cent->lerpAngles, d, NULL, NULL );
		VectorNormalize( d );
	}
	else
	{
		//now let's get the position and direction of this surface and make a big explosion
		trap_G2API_GetBoltMatrix(cent->ghoul2, 0, b, &boltMatrix, cent->lerpAngles, cent->lerpOrigin, cg.time,
			cgs.gameModels, cent->modelScale);
		BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, v);
		BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_Z, d);
	}

	/*
	//now let's get the position and direction of this surface and make a big explosion
	trap_G2API_GetBoltMatrix(cent->ghoul2, 0, b, &boltMatrix, cent->lerpAngles, cent->lerpOrigin, cg.time,
		cgs.gameModels, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, v);
	BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_Z, d);
	*/
	//[/Asteroids]

	trap_FX_PlayEffectID(fxID, v, d, -1, -1);
	if ( throwPart && lostPartFX )
	{//throw off a ship part, too
		vec3_t	fxFwd;
		AngleVectors( cent->lerpAngles, fxFwd, NULL, NULL );
		trap_FX_PlayEffectID(lostPartFX, v, fxFwd, -1, -1);
	}
}

//for now this is just gonna create a big explosion on the area of the surface,
//because I am lazy.
static void CG_CreateSurfaceSmoke(centity_t *cent, int shipSurf, int fxID)
{
	int b = -1;
	vec3_t v, d;
	mdxaBone_t boltMatrix;
	const char *surfName = NULL;

	if (!cent->ghoul2)
	{ //oh no
		return;
	}

	//let's add the surface as a bolt so we can get the base point of it
	if ( shipSurf == SHIPSURF_FRONT )
	{ //front flame/smoke
		surfName = "*nosedamage";
	}
	else if (shipSurf == SHIPSURF_BACK )
	{ //back flame/smoke
		surfName = "*exhaust1";//FIXME: random?  Some point in-between?
	}
	else if (shipSurf == SHIPSURF_RIGHT )
	{ //right wing flame/smoke
		surfName = "*r_wingdamage";
	}
	else if (shipSurf == SHIPSURF_LEFT )
	{ //left wing flame/smoke
		surfName = "*l_wingdamage";
	}
	else
	{//unknown surf!
		return;
	}
	b = trap_G2API_AddBolt(cent->ghoul2, 0, surfName);
	if (b == -1)
	{ //couldn't find this surface apparently
		return;
	}

	//now let's get the position and direction of this surface and make a big explosion
	trap_G2API_GetBoltMatrix(cent->ghoul2, 0, b, &boltMatrix, cent->lerpAngles, cent->lerpOrigin, cg.time,
		cgs.gameModels, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, v);
	BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_Z, d);

	trap_FX_PlayEffectID(fxID, v, d, -1, -1);
}

#define SMOOTH_G2ANIM_LERPANGLES

qboolean CG_VehicleShouldDrawShields( centity_t *vehCent )
{
	if ( vehCent->damageTime > cg.time //ship shields currently taking damage
		&& vehCent->currentState.NPC_class == CLASS_VEHICLE 
		&& vehCent->m_pVehicle
		&& vehCent->m_pVehicle->m_pVehicleInfo )
	{
		return qtrue;
	}
	return qfalse;
}

/*
extern	vmCvar_t		cg_showVehBounds;
extern void BG_VehicleAdjustBBoxForOrientation( Vehicle_t *veh, vec3_t origin, vec3_t mins, vec3_t maxs,
										int clientNum, int tracemask,
										void (*localTrace)(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask)); // bg_pmove.c
*/
qboolean CG_VehicleAttachDroidUnit( centity_t *droidCent, refEntity_t *legs )
{
	if ( droidCent
		&& droidCent->currentState.owner 
		&& droidCent->currentState.clientNum >= MAX_CLIENTS )
	{//the only NPCs that can ride a vehicle are droids...???
		centity_t *vehCent = &cg_entities[droidCent->currentState.owner];
		if ( vehCent 
			&& vehCent->m_pVehicle 
			&& vehCent->ghoul2
			&& vehCent->m_pVehicle->m_iDroidUnitTag != -1 )
		{
			mdxaBone_t boltMatrix;
			vec3_t	fwd, rt, tempAng;

			trap_G2API_GetBoltMatrix(vehCent->ghoul2, 0, vehCent->m_pVehicle->m_iDroidUnitTag, &boltMatrix, vehCent->lerpAngles, vehCent->lerpOrigin, cg.time,
				cgs.gameModels, vehCent->modelScale);
			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, droidCent->lerpOrigin);
			BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_X, fwd);//WTF???
			BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, rt);//WTF???
			vectoangles( fwd, droidCent->lerpAngles );
			vectoangles( rt, tempAng );
			droidCent->lerpAngles[ROLL] = tempAng[PITCH];
			
			return qtrue;
		}
	}
	return qfalse;
}

void CG_G2Animated( centity_t *cent )
{
#ifdef SMOOTH_G2ANIM_LERPANGLES
	float angSmoothFactor = 0.7f;
#endif


	if (!cent->ghoul2)
	{ //Initialize this g2 anim ent, then return (will start rendering next frame)
		CG_G2AnimEntModelLoad(cent);
		cent->npcLocalSurfOff = 0;
		cent->npcLocalSurfOn = 0;
		return;
	}

	if (cent->npcLocalSurfOff != cent->currentState.surfacesOff ||
		cent->npcLocalSurfOn != cent->currentState.surfacesOn)
	{ //looks like it's time for an update.
		int i = 0;

		while (i < BG_NUM_TOGGLEABLE_SURFACES && bgToggleableSurfaces[i])
		{
			if (!(cent->npcLocalSurfOff & (1 << i)) &&
				(cent->currentState.surfacesOff & (1 << i)))
			{ //it wasn't off before but it's off now, so reflect this change in the g2 instance.
				if (bgToggleableSurfaceDebris[i] > 0)
				{ //make some local debris of this thing?
					//FIXME: throw off the proper model effect, too
					CG_CreateSurfaceDebris(cent, i, cgs.effects.mShipDestDestroyed, qtrue);
				}

				trap_G2API_SetSurfaceOnOff(cent->ghoul2, bgToggleableSurfaces[i], TURN_OFF);
			}

			if (!(cent->npcLocalSurfOn & (1 << i)) &&
				(cent->currentState.surfacesOn & (1 << i)))
			{ //same as above, but on instead of off.
				trap_G2API_SetSurfaceOnOff(cent->ghoul2, bgToggleableSurfaces[i], TURN_ON);
			}

			i++;
		}

		cent->npcLocalSurfOff = cent->currentState.surfacesOff;
		cent->npcLocalSurfOn = cent->currentState.surfacesOn;
	}


	/*
	if (cent->currentState.weapon &&
		!trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 1) &&
		!(cent->currentState.eFlags & EF_DEAD))
	{ //if the server says we have a weapon and we haven't copied one onto ourselves yet, then do so.
		trap_G2API_CopySpecificGhoul2Model(g2WeaponInstances[cent->currentState.weapon], 0, cent->ghoul2, 1);

		if (cent->currentState.weapon == WP_SABER)
		{
			trap_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, trap_S_RegisterSound( "sound/weapons/saber/saberon.wav" ));
		}
	}
	*/

	if (cent->torsoBolt && !(cent->currentState.eFlags & EF_DEAD))
	{ //he's alive and has a limb missing still, reattach it and reset the weapon
		CG_ReattachLimb(cent);
	}

	if (((cent->currentState.eFlags & EF_DEAD) || (cent->currentState.eFlags & EF_RAG)) && !cent->localAnimIndex)
	{//racc - try rag dolling.
		vec3_t forcedAngles;

		VectorClear(forcedAngles);
		forcedAngles[YAW] = cent->lerpAngles[YAW];

		CG_RagDoll(cent, forcedAngles);
	}

#ifdef SMOOTH_G2ANIM_LERPANGLES
	if ((cent->lerpAngles[YAW] > 0 && cent->smoothYaw < 0) ||
		(cent->lerpAngles[YAW] < 0 && cent->smoothYaw > 0))
	{ //keep it from snapping around on the threshold
		cent->smoothYaw = -cent->smoothYaw;
	}
	cent->lerpAngles[YAW] = cent->smoothYaw+(cent->lerpAngles[YAW]-cent->smoothYaw)*angSmoothFactor;
	cent->smoothYaw = cent->lerpAngles[YAW];
#endif

	//now just render as a player
	CG_Player(cent);

	/*
	if ( cg_showVehBounds.integer )
	{//show vehicle bboxes
		if ( cent->currentState.clientNum >= MAX_CLIENTS
			&& cent->currentState.NPC_class == CLASS_VEHICLE 
			&& cent->m_pVehicle
			&& cent->m_pVehicle->m_pVehicleInfo 
			&& cent->currentState.clientNum != cg.predictedVehicleState.clientNum )
		{//not the predicted vehicle
			vec3_t NPCDEBUG_RED = {1.0, 0.0, 0.0};
			vec3_t absmin, absmax;
			vec3_t bmins, bmaxs;
			float *old = cent->m_pVehicle->m_vOrientation;
			cent->m_pVehicle->m_vOrientation = &cent->lerpAngles[0];

			BG_VehicleAdjustBBoxForOrientation( cent->m_pVehicle, cent->lerpOrigin, bmins, bmaxs,
										cent->currentState.number, MASK_PLAYERSOLID, NULL );
			cent->m_pVehicle->m_vOrientation = old;

			VectorAdd( cent->lerpOrigin, bmins, absmin );
			VectorAdd( cent->lerpOrigin, bmaxs, absmax );
			CG_Cube( absmin, absmax, NPCDEBUG_RED, 0.25 );
		}
	}
	*/
}
//rww - here ends the majority of my g2animent stuff.

//Disabled for now, I'm too lazy to keep it working with all the stuff changing around.
#if 0
int cgFPLSState = 0;

void CG_ForceFPLSPlayerModel(centity_t *cent, clientInfo_t *ci)
{
	animation_t *anim;

	if (cg_fpls.integer && !cg.renderingThirdPerson)
	{
		int				skinHandle;

		skinHandle = trap_R_RegisterSkin("models/players/kyle/model_fpls2.skin");

		trap_G2API_CleanGhoul2Models(&(ci->ghoul2Model));

		ci->torsoSkin = skinHandle;
		trap_G2API_InitGhoul2Model(&ci->ghoul2Model, "models/players/kyle/model.glm", 0, ci->torsoSkin, 0, 0, 0);

		ci->bolt_rhand = trap_G2API_AddBolt(ci->ghoul2Model, 0, "*r_hand");
		
		trap_G2API_SetBoneAnim(ci->ghoul2Model, 0, "model_root", 0, 12, BONE_ANIM_OVERRIDE_LOOP, 1.0f, cg.time, -1, -1);
		trap_G2API_SetBoneAngles(ci->ghoul2Model, 0, "upper_lumbar", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_X, NEGATIVE_Y, NEGATIVE_Z, NULL, 0, cg.time);
		trap_G2API_SetBoneAngles(ci->ghoul2Model, 0, "cranium", vec3_origin, BONE_ANGLES_POSTMULT, POSITIVE_Z, NEGATIVE_Y, POSITIVE_X, NULL, 0, cg.time);

		ci->bolt_lhand = trap_G2API_AddBolt(ci->ghoul2Model, 0, "*l_hand");

		//rhand must always be first bolt. lhand always second. Whichever you want the
		//jetpack bolted to must always be third.
		trap_G2API_AddBolt(ci->ghoul2Model, 0, "*chestg");

		//claw bolts
		trap_G2API_AddBolt(ci->ghoul2Model, 0, "*r_hand_cap_r_arm");
		trap_G2API_AddBolt(ci->ghoul2Model, 0, "*l_hand_cap_l_arm");

		ci->bolt_head = trap_G2API_AddBolt(ci->ghoul2Model, 0, "*head_top");
		if (ci->bolt_head == -1)
		{
			ci->bolt_head = trap_G2API_AddBolt(ci->ghoul2Model, 0, "ceyebrow");
		}

		ci->bolt_motion = trap_G2API_AddBolt(ci->ghoul2Model, 0, "Motion");

		//We need a lower lumbar bolt for footsteps
		ci->bolt_llumbar = trap_G2API_AddBolt(ci->ghoul2Model, 0, "lower_lumbar");

		CG_CopyG2WeaponInstance(cent, cent->currentState.weapon, ci->ghoul2Model);
	}
	else
	{
		CG_RegisterClientModelname(ci, ci->modelName, ci->skinName, ci->teamName, cent->currentState.number);
	}

	anim = &bgAllAnims[cent->localAnimIndex].anims[ cent->currentState.legsAnim ];

	if (anim)
	{
		int flags = BONE_ANIM_OVERRIDE_FREEZE;
		int firstFrame = anim->firstFrame;
		int setFrame = -1;
		float animSpeed = 50.0f / anim->frameLerp;

		if (anim->loopFrames != -1)
		{
			flags = BONE_ANIM_OVERRIDE_LOOP;
		}

		if (cent->pe.legs.frame >= anim->firstFrame && cent->pe.legs.frame <= (anim->firstFrame + anim->numFrames))
		{
			setFrame = cent->pe.legs.frame;
		}

		trap_G2API_SetBoneAnim(ci->ghoul2Model, 0, "model_root", firstFrame, anim->firstFrame + anim->numFrames, flags, animSpeed, cg.time, setFrame, 150);

		cent->currentState.legsAnim = 0;
	}

	anim = &bgAllAnims[cent->localAnimIndex].anims[ cent->currentState.torsoAnim ];

	if (anim)
	{
		int flags = BONE_ANIM_OVERRIDE_FREEZE;
		int firstFrame = anim->firstFrame;
		int setFrame = -1;
		float animSpeed = 50.0f / anim->frameLerp;

		if (anim->loopFrames != -1)
		{
			flags = BONE_ANIM_OVERRIDE_LOOP;
		}

		if (cent->pe.torso.frame >= anim->firstFrame && cent->pe.torso.frame <= (anim->firstFrame + anim->numFrames))
		{
			setFrame = cent->pe.torso.frame;
		}

		trap_G2API_SetBoneAnim(ci->ghoul2Model, 0, "lower_lumbar", firstFrame, anim->firstFrame + anim->numFrames, flags, animSpeed, cg.time, setFrame, 150);

		cent->currentState.torsoAnim = 0;
	}

	trap_G2API_CleanGhoul2Models(&(cent->ghoul2));
	trap_G2API_DuplicateGhoul2Instance(ci->ghoul2Model, &cent->ghoul2);

	//Attach the instance to this entity num so we can make use of client-server
	//shared operations if possible.
	trap_G2API_AttachInstanceToEntNum(cent->ghoul2, cent->currentState.number, qfalse);
}
#endif


//[CoOp]
//find the gender for a given modelname.  This is used to set up the NPCClient's gender correct
//to make jaden's dialog play in the correct gender
//based off the approprate sections of CG_LoadCISounds
int FindGender( const char *modelPath, centity_t *cent )
{
	fileHandle_t f;
	qboolean	isFemale = qfalse;
	int			i = 0;
	int			fLen = 0;
	char		soundpath[MAX_QPATH];
	char		modelName[MAX_QPATH];
	char		*temp;

	if(cent->currentState.NPC_class != CLASS_PLAYER)
	{//we don't care about the gender of models that aren't going to be doing Jaden
		//voice scripting.
		return GENDER_MALE;
	}

	strcpy(modelName, modelPath);

	temp = Q_strrchr( modelName, '/' );

	if(!temp)
	{//bad player model
		CG_Printf("Bad modelPath passed to FindGender.\n");
		return GENDER_MALE;
	}

	*temp = '\0';


	fLen = trap_FS_FOpenFile(va("%s/sounds.cfg", modelName), &f, FS_READ);

	if (f)
	{
		trap_FS_Read(soundpath, fLen, f);
		soundpath[fLen] = 0;

		i = fLen;

		while (i >= 0 && soundpath[i] != '\n')
		{
			if (soundpath[i] == 'f')
			{
				isFemale = qtrue;
				soundpath[i] = 0;
			}

			i--;
		}

		trap_FS_FCloseFile(f);
	}
	else
	{
		CG_Printf("FindGender Error: Couldnt find sounds.cfg for %s\n", modelName);
	}

	if (isFemale)
	{
		return GENDER_FEMALE;
	}
	else
	{
		return GENDER_MALE;
	}
}
//[/CoOp]


//for allocating and freeing npc clientinfo structures.
//Remember to free this before game shutdown no matter what
//and don't stomp over it, as it is dynamic memory from the
//exe.
void CG_CreateNPCClient(clientInfo_t **ci)
{
	//trap_TrueMalloc((void **)ci, sizeof(clientInfo_t));
	*ci = (clientInfo_t *) BG_Alloc(sizeof(clientInfo_t));
}

void CG_DestroyNPCClient(clientInfo_t **ci)
{
	memset(*ci, 0, sizeof(clientInfo_t));
	//trap_TrueFree((void **)ci);
}

static void CG_ForceElectrocution( centity_t *cent, const vec3_t origin, vec3_t tempAngles, qhandle_t shader, qboolean alwaysDo )
{
	// Undoing for now, at least this code should compile if I ( or anyone else ) decides to work on this effect
	qboolean	found = qfalse;
	vec3_t		fxOrg, fxOrg2, dir;
	vec3_t		rgb;
	mdxaBone_t	boltMatrix;
	trace_t		tr;
	int bolt=-1;
	int iter=0;
	int torsoBolt = -1;
	int crotchBolt = -1;
	int elbowLBolt = -1;
	int elbowRBolt = -1;
	int handLBolt = -1;
	int handRBolt = -1;
	int kneeLBolt = -1;
	int kneeRBolt = -1;
	int footLBolt = -1;
	int footRBolt = -1;

	VectorSet(rgb, 1, 1, 1);

	if (cent->localAnimIndex <= 1)
	{ //humanoid
		torsoBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "lower_lumbar");
		crotchBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "pelvis");
		elbowLBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*l_arm_elbow");
		elbowRBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*r_arm_elbow");
		handLBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*l_hand");
		handRBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*r_hand");
		kneeLBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*hips_l_knee");
		kneeRBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*hips_r_knee");
		footLBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*l_leg_foot");
		footRBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*r_leg_foot");
	}
	else if (cent->currentState.NPC_class == CLASS_PROTOCOL)
	{ //any others that can use these bolts too?
		torsoBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "lower_lumbar");
		crotchBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "pelvis");
		elbowLBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*bicep_lg");
		elbowRBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*bicep_rg");
		handLBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*hand_l");
		handRBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*weapon");
		kneeLBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*thigh_lg");
		kneeRBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*thigh_rg");
		footLBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*foot_lg");
		footRBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*foot_rg");
	}

	// Pick a random start point
	while (bolt<0)
	{
		int test;
		if (iter>5)
		{
			test=iter-5;
		}
		else
		{
			test=Q_irand(0,6);
		}
		switch(test)
		{
		case 0:
			// Right Elbow
			bolt=elbowRBolt;
			break;
		case 1:
			// Left Hand
			bolt=handLBolt;
			break;
		case 2:
			// Right hand
			bolt=handRBolt;
			break;
		case 3:
			// Left Foot
			bolt=footLBolt;
			break;
		case 4:
			// Right foot
			bolt=footRBolt;
			break;
		case 5:
			// Torso
			bolt=torsoBolt;
			break;
		case 6:
		default:
			// Left Elbow
			bolt=elbowLBolt;
			break;
		}
		if (++iter==20)
			break;
	}
	if (bolt>=0)
	{
		found = trap_G2API_GetBoltMatrix( cent->ghoul2, 0, bolt, 
				&boltMatrix, tempAngles, origin, cg.time, 
				cgs.gameModels, cent->modelScale);
	}

	// Make sure that it's safe to even try and get these values out of the Matrix, otherwise the values could be garbage
	if ( found )
	{
		BG_GiveMeVectorFromMatrix( &boltMatrix, ORIGIN, fxOrg );
		if ( random() > 0.5f )
		{
			BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_X, dir );
		}
		else
		{
			BG_GiveMeVectorFromMatrix( &boltMatrix, NEGATIVE_Y, dir );
		}

		// Add some fudge, makes us not normalized, but that isn't really important
		dir[0] += crandom() * 0.4f;
		dir[1] += crandom() * 0.4f;
		dir[2] += crandom() * 0.4f;
	}
	else
	{
		// Just use the lerp Origin and a random direction
		VectorCopy( cent->lerpOrigin, fxOrg );
		VectorSet( dir, crandom(), crandom(), crandom() ); // Not normalized, but who cares.
		switch ( cent->currentState.NPC_class )
		{
		case CLASS_PROBE:
			fxOrg[2] += 50;
			break;
		case CLASS_MARK1:
			fxOrg[2] += 50;
			break;
		case CLASS_ATST:
			fxOrg[2] += 120;
			break;
		default:
			break;
		}
	}

	VectorMA( fxOrg, random() * 40 + 40, dir, fxOrg2 );

	CG_Trace( &tr, fxOrg, NULL, NULL, fxOrg2, -1, CONTENTS_SOLID );

	if ( tr.fraction < 1.0f || random() > 0.94f || alwaysDo )
	{
		addElectricityArgStruct_t p;

		VectorCopy(fxOrg, p.start);
		VectorCopy(tr.endpos, p.end);
		p.size1 = 1.5f;
		p.size2 = 4.0f;
		p.sizeParm = 0.0f;
		p.alpha1 = 1.0f;
		p.alpha2 = 0.5f;
		p.alphaParm = 0.0f;
		VectorCopy(rgb, p.sRGB);
		VectorCopy(rgb, p.eRGB);
		p.rgbParm = 0.0f;
		p.chaos = 5.0f;
		p.killTime = (random() * 50 + 100);
		p.shader = shader;
		p.flags = (0x00000001 | 0x00000100 | 0x02000000 | 0x04000000 | 0x01000000);

		trap_FX_AddElectricity(&p);

		//In other words:
		/*
		FX_AddElectricity( fxOrg, tr.endpos,
			1.5f, 4.0f, 0.0f, 
			1.0f, 0.5f, 0.0f,
			rgb, rgb, 0.0f,
			5.5f, random() * 50 + 100, shader, FX_ALPHA_LINEAR | FX_SIZE_LINEAR | FX_BRANCH | FX_GROW | FX_TAPER );
		*/
	}
}

void *cg_g2JetpackInstance = NULL;

#define JETPACK_MODEL "models/weapons2/jetpack/model.glm"

void CG_InitJetpackGhoul2(void)
{
	if (cg_g2JetpackInstance)
	{
		assert(!"Tried to init jetpack inst, already init'd");
		return;
	}

	trap_G2API_InitGhoul2Model(&cg_g2JetpackInstance, JETPACK_MODEL, 0, 0, 0, 0, 0);

	assert(cg_g2JetpackInstance);

	//Indicate which bolt on the player we will be attached to
	//In this case bolt 0 is rhand, 1 is lhand, and 2 is the bolt
	//for the jetpack (*chestg)
	trap_G2API_SetBoltInfo(cg_g2JetpackInstance, 0, 2);

	//Add the bolts jet effects will be played from
	trap_G2API_AddBolt(cg_g2JetpackInstance, 0, "torso_ljet");
	trap_G2API_AddBolt(cg_g2JetpackInstance, 0, "torso_rjet");
}

void CG_CleanJetpackGhoul2(void)
{
	if (cg_g2JetpackInstance)
	{
		trap_G2API_CleanGhoul2Models(&cg_g2JetpackInstance);
		cg_g2JetpackInstance = NULL;
	}
}

#define RARMBIT			(1 << (G2_MODELPART_RARM-10))
#define RHANDBIT		(1 << (G2_MODELPART_RHAND-10))
#define WAISTBIT		(1 << (G2_MODELPART_WAIST-10))

#if 0
static void CG_VehicleHeatEffect( vec3_t org, centity_t *cent )
{
	refEntity_t ent;
	vec3_t ang;
	float scale;
	float vLen;
	float alpha;

	if (!cg_renderToTextureFX.integer)
	{
		return;
	}
	scale = 0.1f;

	alpha = 200.0f;

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( org, ent.origin );

	VectorSubtract(ent.origin, cg.refdef.vieworg, ent.axis[0]);
	vLen = VectorLength(ent.axis[0]);
	if (VectorNormalize(ent.axis[0]) <= 0.1f)
	{	// Entity is right on vieworg.  quit.
		return;
	}

	vectoangles(ent.axis[0], ang);
	AnglesToAxis(ang, ent.axis);

	//radius must be a power of 2, and is the actual captured texture size
	ent.radius = 32;

	VectorScale(ent.axis[0], scale, ent.axis[0]);
	VectorScale(ent.axis[1], scale, ent.axis[1]);
	VectorScale(ent.axis[2], -scale, ent.axis[2]);

	ent.hModel = cgs.media.halfShieldModel;
	ent.customShader = cgs.media.cloakedShader;

	//make it partially transparent so it blends with the background
	ent.renderfx = (RF_DISTORTION|RF_FORCE_ENT_ALPHA);
	ent.shaderRGBA[0] = 255.0f;
	ent.shaderRGBA[1] = 255.0f;
	ent.shaderRGBA[2] = 255.0f;
	ent.shaderRGBA[3] = alpha;

	trap_R_AddRefEntityToScene( &ent );
}
#endif

static int lastFlyBySound[MAX_GENTITIES] = {0};
#define	FLYBYSOUNDTIME 2000
int	cg_lastHyperSpaceEffectTime = 0;
static CGAME_INLINE void CG_VehicleEffects(centity_t *cent)
{
	Vehicle_t *pVehNPC;

	if (cent->currentState.eType != ET_NPC ||
		cent->currentState.NPC_class != CLASS_VEHICLE ||
		!cent->m_pVehicle)
	{
		return;
	}

	pVehNPC = cent->m_pVehicle;

	if ( cent->currentState.clientNum == cg.predictedPlayerState.m_iVehicleNum//my vehicle
		&& (cent->currentState.eFlags2&EF2_HYPERSPACE) )//hyperspacing
	{//in hyperspace!
		if ( cg.predictedVehicleState.hyperSpaceTime
			&& (cg.time-cg.predictedVehicleState.hyperSpaceTime) < HYPERSPACE_TIME )
		{
			if ( !cg_lastHyperSpaceEffectTime
				|| (cg.time - cg_lastHyperSpaceEffectTime) > HYPERSPACE_TIME+500 )
			{//can't be from the last time we were in hyperspace, so play the effect!
				trap_FX_PlayBoltedEffectID( cgs.effects.mHyperspaceStars, cent->lerpOrigin, cent->ghoul2, 0, 
											cent->currentState.number, 0, 0, qtrue );
				cg_lastHyperSpaceEffectTime = cg.time;
			}
		}
	}

	//FLYBY sound
	if ( cent->currentState.clientNum != cg.predictedPlayerState.m_iVehicleNum
		&& (pVehNPC->m_pVehicleInfo->soundFlyBy||pVehNPC->m_pVehicleInfo->soundFlyBy2) )
	{//not my vehicle
		if ( cent->currentState.speed && cg.predictedPlayerState.speed+cent->currentState.speed > 500 )
		{//he's moving and between the two of us, we're moving fast
			vec3_t diff;
			VectorSubtract( cent->lerpOrigin, cg.predictedPlayerState.origin, diff );
			if ( VectorLength( diff ) < 2048 )
			{//close
				vec3_t	myFwd, theirFwd;
				AngleVectors( cg.predictedPlayerState.viewangles, myFwd, NULL, NULL );
				VectorScale( myFwd, cg.predictedPlayerState.speed, myFwd );
				AngleVectors( cent->lerpAngles, theirFwd, NULL, NULL );
				VectorScale( theirFwd, cent->currentState.speed, theirFwd );
				if ( lastFlyBySound[cent->currentState.clientNum]+FLYBYSOUNDTIME < cg.time )
				{//okay to do a flyby sound on this vehicle
					if ( DotProduct( myFwd, theirFwd ) < 500 )
					{
						int flyBySound = 0;
						if ( pVehNPC->m_pVehicleInfo->soundFlyBy && pVehNPC->m_pVehicleInfo->soundFlyBy2 )
						{
							flyBySound = Q_irand(0,1)?pVehNPC->m_pVehicleInfo->soundFlyBy:pVehNPC->m_pVehicleInfo->soundFlyBy2;
						}
						else if ( pVehNPC->m_pVehicleInfo->soundFlyBy  )
						{
							flyBySound = pVehNPC->m_pVehicleInfo->soundFlyBy;
						}
						else //if ( pVehNPC->m_pVehicleInfo->soundFlyBy2 )
						{
							flyBySound = pVehNPC->m_pVehicleInfo->soundFlyBy2;
						}
						trap_S_StartSound(NULL, cent->currentState.clientNum, CHAN_LESS_ATTEN, flyBySound );
						lastFlyBySound[cent->currentState.clientNum] = cg.time;
					}
				}
			}
		}
	}

	if ( !cent->currentState.speed//was stopped
		&& cent->nextState.speed > 0//now moving forward
		&& cent->m_pVehicle->m_pVehicleInfo->soundEngineStart )
	{//engines rev up for the first time
		trap_S_StartSound(NULL, cent->currentState.clientNum, CHAN_LESS_ATTEN, cent->m_pVehicle->m_pVehicleInfo->soundEngineStart );
	}
	// Animals don't exude any effects...
	if ( pVehNPC->m_pVehicleInfo->type != VH_ANIMAL )
	{
		//[Asteroids]
		qboolean didFireTrail = qfalse;
		//[/Asteroids]
		if (pVehNPC->m_pVehicleInfo->surfDestruction && cent->ghoul2)
		{ //see if anything has been blown off
			int i = 0;
			qboolean surfDmg = qfalse;

			while (i < BG_NUM_TOGGLEABLE_SURFACES)
			{
				if (bgToggleableSurfaceDebris[i] > 1)
				{ //this is decidedly a destroyable surface, let's check its status
					int surfTest = trap_G2API_GetSurfaceRenderStatus(cent->ghoul2, 0, bgToggleableSurfaces[i]);

					if ( surfTest != -1
						&& (surfTest&TURN_OFF) )
					{ //it exists, but it's off...
						surfDmg = qtrue;

						//create some flames
                        CG_CreateSurfaceDebris(cent, i, cgs.effects.mShipDestBurning, qfalse);
						//[Asteroids]
						didFireTrail = qtrue;
						//[/Asteroids]
					}
				}

				i++;
			}

			if (surfDmg)
			{ //if any surface are damaged, neglect exhaust etc effects (so we don't have exhaust trails coming out of invisible surfaces)
				return;
			}
		}
		//[Asteroids]
		if ( !didFireTrail && (cent->currentState.eFlags&EF_DEAD) )
		{//spiralling out of control anyway
            CG_CreateSurfaceDebris(cent, -1, cgs.effects.mShipDestBurning, qfalse);
		}
		//[/Asteroids]

		if ( pVehNPC->m_iLastFXTime <= cg.time )
		{//until we attach it, we need to debounce this
			vec3_t	fwd, rt, up;
			vec3_t	flat;
			float nextFXDelay = 50;
			VectorSet(flat, 0, cent->lerpAngles[1], cent->lerpAngles[2]);
			AngleVectors( flat, fwd, rt, up );
			if ( cent->currentState.speed > 0 )
			{//FIXME: only do this when accelerator is being pressed! (must have a driver?)
				vec3_t	org;
				qboolean doExhaust = qfalse;
				VectorMA( cent->lerpOrigin, -16, up, org );
				VectorMA( org, -42, fwd, org );
				// Play damage effects.
				//if ( pVehNPC->m_iArmor <= 75 )
				if (0)
				{//hurt
					trap_FX_PlayEffectID( cgs.effects.mBlackSmoke, org, fwd, -1, -1 );
				}
				else if ( pVehNPC->m_pVehicleInfo->iTrailFX )
				{//okay, do normal trail
					trap_FX_PlayEffectID( pVehNPC->m_pVehicleInfo->iTrailFX, org, fwd, -1, -1 );
				}
				//=====================================================================
				//EXHAUST FX
				//=====================================================================
				//do exhaust
				if ( (cent->currentState.eFlags&EF_JETPACK_ACTIVE) )
				{//cheap way of telling us the vehicle is in "turbo" mode
					doExhaust = (pVehNPC->m_pVehicleInfo->iTurboFX!=0);
				}
				else
				{
					doExhaust = (pVehNPC->m_pVehicleInfo->iExhaustFX!=0);
				}
				//[CloakingVehicles]
				//don't draw engine effects if the ship is cloaked
				if ( doExhaust && cent->ghoul2 && !(cent->currentState.powerups & (1 <<PW_CLOAKED)) )
				//if ( doExhaust && cent->ghoul2 )
				//[/CloakingVehicles]
				{
					int i;
					int fx;

					for ( i = 0; i < MAX_VEHICLE_EXHAUSTS; i++ )
					{
						// We hit an invalid tag, we quit (they should be created in order so tough luck if not).
						if ( pVehNPC->m_iExhaustTag[i] == -1 )
						{
							break;
						}

						if ( (cent->currentState.brokenLimbs&(1<<SHIPSURF_DAMAGE_BACK_HEAVY)) )
						{//engine has taken heavy damage
							if ( !Q_irand( 0, 1 ) )
							{//50% chance of not drawing this engine glow this frame
								continue;
							}
						}
						else if ( (cent->currentState.brokenLimbs&(1<<SHIPSURF_DAMAGE_BACK_LIGHT)) )
						{//engine has taken light damage
							if ( !Q_irand( 0, 4 ) )
							{//20% chance of not drawing this engine glow this frame
								continue;
							}
						}

						if ( (cent->currentState.eFlags&EF_JETPACK_ACTIVE) //cheap way of telling us the vehicle is in "turbo" mode
							&& pVehNPC->m_pVehicleInfo->iTurboFX )//they have a valid turbo exhaust effect to play
						{
							fx = pVehNPC->m_pVehicleInfo->iTurboFX;
						}
						else
						{//play the normal one
							fx = pVehNPC->m_pVehicleInfo->iExhaustFX;
						}

						if (pVehNPC->m_pVehicleInfo->type == VH_FIGHTER)
						{
							trap_FX_PlayBoltedEffectID(fx, cent->lerpOrigin, cent->ghoul2, pVehNPC->m_iExhaustTag[i], 
														cent->currentState.number, 0, 0, qtrue);
						}
						else
						{ //fixme: bolt these too
							mdxaBone_t boltMatrix;
							vec3_t boltOrg, boltDir;

							trap_G2API_GetBoltMatrix(cent->ghoul2, 0, pVehNPC->m_iExhaustTag[i], &boltMatrix, flat,
								cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);

							BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltOrg);
							VectorCopy(fwd, boltDir); //fixme?

							trap_FX_PlayEffectID( fx, boltOrg, boltDir, -1, -1 );
						}
					}
				}
				//=====================================================================
				//WING TRAIL FX
				//=====================================================================
				//do trail
				//FIXME: not in space!!!
				if ( pVehNPC->m_pVehicleInfo->iTrailFX != 0 && cent->ghoul2 )
				{
					int i;
					vec3_t boltOrg, boltDir;
					mdxaBone_t boltMatrix;
					vec3_t getBoltAngles;

					VectorCopy(cent->lerpAngles, getBoltAngles);
					if (pVehNPC->m_pVehicleInfo->type != VH_FIGHTER)
					{ //only fighters use pitch/roll in refent axis
                        getBoltAngles[PITCH] = getBoltAngles[ROLL] = 0.0f;
					}

					for ( i = 1; i < 5; i++ )
					{
						int trailBolt = trap_G2API_AddBolt(cent->ghoul2, 0, va("*trail%d",i) );
						// We hit an invalid tag, we quit (they should be created in order so tough luck if not).
						if ( trailBolt == -1 )
						{
							break;
						}

						trap_G2API_GetBoltMatrix(cent->ghoul2, 0, trailBolt, &boltMatrix, getBoltAngles,
							cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);

						BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltOrg);
						VectorCopy(fwd, boltDir); //fixme?

						trap_FX_PlayEffectID( pVehNPC->m_pVehicleInfo->iTrailFX, boltOrg, boltDir, -1, -1 );
					}
				}
			}
			//FIXME armor needs to be sent over network
			{
				if ( (cent->currentState.eFlags&EF_DEAD) )
				{//just plain dead, use flames 
					vec3_t	up ={0,0,1};
					vec3_t boltOrg;

					//if ( pVehNPC->m_iDriverTag == -1 )
					{//doh!  no tag
						VectorCopy( cent->lerpOrigin, boltOrg );
					}
					//else
					//{
					//	mdxaBone_t boltMatrix;
					//	vec3_t getBoltAngles;

					//	VectorCopy(cent->lerpAngles, getBoltAngles);
					//	if (pVehNPC->m_pVehicleInfo->type != VH_FIGHTER)
					//	{ //only fighters use pitch/roll in refent axis
					//		getBoltAngles[PITCH] = getBoltAngles[ROLL] = 0.0f;
					//	}

					//	trap_G2API_GetBoltMatrix(cent->ghoul2, 0, pVehNPC->m_iDriverTag, &boltMatrix, getBoltAngles,
					//		cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);

					//	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltOrg);
					//}
					trap_FX_PlayEffectID( cgs.effects.mShipDestBurning, boltOrg, up, -1, -1 );
				}
			}
			if ( cent->currentState.brokenLimbs )
			{
				int i;
				if ( !Q_irand( 0, 5 ) )
				{
					for ( i = SHIPSURF_FRONT; i <= SHIPSURF_LEFT; i++ )
					{
						if ( (cent->currentState.brokenLimbs&(1<<((i-SHIPSURF_FRONT)+SHIPSURF_DAMAGE_FRONT_HEAVY))) )
						{//heavy damage, do both effects
							if ( pVehNPC->m_pVehicleInfo->iInjureFX )
							{
								CG_CreateSurfaceSmoke( cent, i, pVehNPC->m_pVehicleInfo->iInjureFX );
							}
							if ( pVehNPC->m_pVehicleInfo->iDmgFX )
							{
								CG_CreateSurfaceSmoke( cent, i, pVehNPC->m_pVehicleInfo->iDmgFX );
							}
						}
						else if ( (cent->currentState.brokenLimbs&(1<<((i-SHIPSURF_FRONT)+SHIPSURF_DAMAGE_FRONT_LIGHT))) )
						{//only light damage
							if ( pVehNPC->m_pVehicleInfo->iInjureFX )
							{
								CG_CreateSurfaceSmoke( cent, i, pVehNPC->m_pVehicleInfo->iInjureFX );
							}
						}
					}
				}
			}
			/*
			if ( pVehNPC->m_iArmor <= 50 )
			{//FIXME: use as a proportion of max armor?
				VectorMA( cent->lerpOrigin, 64, fwd, org );
				VectorScale( fwd, -1, fwd );
				
				trap_FX_PlayEffectID( cgs.effects.mBlackSmoke, org, fwd, -1, -1 );
			}
			if ( pVehNPC->m_iArmor <= 0 )
			{//FIXME: should use something attached.. but want it to build up over time, so...
				if ( flrand( 0, cg.time - pVehNPC->m_iDieTime ) < 1000 )
				{//flaming!
					VectorMA( cent->lerpOrigin, flrand(-64, 64), fwd, org );
					VectorScale( fwd, -1, fwd );
					trap_FX_PlayEffectID( trap_FX_RegisterEffect("ships/fire"), org, fwd, -1, -1 );
					nextFXDelay = 50;
				}
			}
			*/
			pVehNPC->m_iLastFXTime = cg.time + nextFXDelay;
		}
	}
}

/*
===============
CG_Player
===============
*/
#include "../namespace_begin.h"
int BG_EmplacedView(vec3_t baseAngles, vec3_t angles, float *newYaw, float constraint);
#include "../namespace_end.h"

float CG_RadiusForCent( centity_t *cent )
{
	if ( cent->currentState.eType == ET_NPC )
	{
		if (cent->currentState.NPC_class == CLASS_VEHICLE &&
			cent->m_pVehicle &&
			cent->m_pVehicle->m_pVehicleInfo->g2radius)
		{ //has override
			return cent->m_pVehicle->m_pVehicleInfo->g2radius;
		}
		else if ( cent->currentState.g2radius )
		{
			return cent->currentState.g2radius;
		}
	}
	else if ( cent->currentState.g2radius )
	{
		return cent->currentState.g2radius;
	}
	return 64.0f;
}

static float cg_vehThirdPersonAlpha = 1.0f;
extern vec3_t	cg_crosshairPos;
extern vec3_t	cameraCurLoc;
void CG_CheckThirdPersonAlpha( centity_t *cent, refEntity_t *legs )
{
	float alpha = 1.0f;		
	int	setFlags = 0;

	if ( cent->m_pVehicle )
	{//a vehicle 
		if ( cg.predictedPlayerState.m_iVehicleNum != cent->currentState.clientNum//not mine
			&& cent->m_pVehicle->m_pVehicleInfo
			&& cent->m_pVehicle->m_pVehicleInfo->cameraOverride
			&& cent->m_pVehicle->m_pVehicleInfo->cameraAlpha )//it has alpha
		{//make sure it's not using any alpha
			legs->renderfx |= RF_FORCE_ENT_ALPHA;
			legs->shaderRGBA[3] = 255;
			return;
		}
	}

	if ( !cg.renderingThirdPerson )
	{
		return;
	}

	if ( cg.predictedPlayerState.m_iVehicleNum )
	{//in a vehicle
		if ( cg.predictedPlayerState.m_iVehicleNum == cent->currentState.clientNum )
		{//this is my vehicle
			if ( cent->m_pVehicle
				&& cent->m_pVehicle->m_pVehicleInfo
				&& cent->m_pVehicle->m_pVehicleInfo->cameraOverride
				&& cent->m_pVehicle->m_pVehicleInfo->cameraAlpha )
			{//vehicle has auto third-person alpha on
				trace_t trace;
				vec3_t	dir2Crosshair, end;
				VectorSubtract( cg_crosshairPos, cameraCurLoc, dir2Crosshair );
				VectorNormalize( dir2Crosshair );
				VectorMA( cameraCurLoc, cent->m_pVehicle->m_pVehicleInfo->cameraRange*2.0f, dir2Crosshair, end );
				CG_G2Trace( &trace, cameraCurLoc, vec3_origin, vec3_origin, end, ENTITYNUM_NONE, CONTENTS_BODY );
				if ( trace.entityNum == cent->currentState.clientNum 
					|| trace.entityNum == cg.predictedPlayerState.clientNum)
				{//hit me or the vehicle I'm in
					cg_vehThirdPersonAlpha -= 0.1f*cg.frametime/50.0f;
					if ( cg_vehThirdPersonAlpha < cent->m_pVehicle->m_pVehicleInfo->cameraAlpha )
					{
						cg_vehThirdPersonAlpha = cent->m_pVehicle->m_pVehicleInfo->cameraAlpha;
					}
				}
				else
				{
					cg_vehThirdPersonAlpha += 0.1f*cg.frametime/50.0f;
					if ( cg_vehThirdPersonAlpha > 1.0f )
					{
						cg_vehThirdPersonAlpha = 1.0f;
					}
				}
				alpha = cg_vehThirdPersonAlpha;
			}
			else
			{//use the cvar
				//reset this
				cg_vehThirdPersonAlpha = 1.0f;
				//use the cvar
				alpha = cg_thirdPersonAlpha.value;
			}
		}
	}
	else if ( cg.predictedPlayerState.clientNum == cent->currentState.clientNum )
	{//it's me
		//reset this
		cg_vehThirdPersonAlpha = 1.0f;
		//use the cvar
		setFlags = RF_FORCE_ENT_ALPHA;
		alpha = cg_thirdPersonAlpha.value;
	}
	
	if ( alpha < 1.0f )
	{
		legs->renderfx |= setFlags;
		legs->shaderRGBA[3] = (unsigned char)(alpha * 255.0f);
	}
}


//[TrueView]
/*
================
GetSelfLegAnimPoint

  Based On:  G_GetAnimPoint
================
*/
//Get the point in the leg animation and return a percentage of the current point in the anim between 0 and the total anim length (0.0f - 1.0f)
float GetSelfLegAnimPoint(void)
{
	//[BugFix2]
	return BG_GetLegsAnimPoint(&cg.predictedPlayerState, cg_entities[cg.predictedPlayerState.clientNum].localAnimIndex);
	
	/*
	int animindex = cg_entities[cg.predictedPlayerState.clientNum].localAnimIndex;
	float attackAnimLength = bgAllAnims[animindex].anims[cg.predictedPlayerState.legsAnim].numFrames * fabs(bgAllAnims[animindex].anims[cg.predictedPlayerState.legsAnim].frameLerp);
	float currentPoint = 0;
	//float animSpeedFactor = 1.0f;
	float animPercentage = 0;

	//currentPoint = cg.snap->ps.legsTimer;
	currentPoint = cg.predictedPlayerState.legsTimer;

	animPercentage = currentPoint/attackAnimLength;

	//Com_Printf("Leg Animation Float Percentage: %f\n", animPercentage);

	return animPercentage;
	*/
	//[/BugFix2]
}


/*
================
GetSelfTorsoAnimPoint

  Based On:  G_GetAnimPoint
================
*/
//Get the point in the torso animation and return a percentage of the current point in the anim between 0 and the total anim length (0.0f - 1.0f)
float GetSelfTorsoAnimPoint(void)
{
	//[BugFix2]
	return BG_GetTorsoAnimPoint(&cg.predictedPlayerState, cg_entities[cg.predictedPlayerState.clientNum].localAnimIndex);
	
	/*
	int animindex = cg_entities[cg.predictedPlayerState.clientNum].localAnimIndex;
	int speedDif = 0;
	float attackAnimLength = bgAllAnims[animindex].anims[cg.predictedPlayerState.torsoAnim].numFrames * fabs(bgAllAnims[animindex].anims[cg.predictedPlayerState.torsoAnim].frameLerp);
	float currentPoint = 0;
	float animSpeedFactor = 1.0f;
	float animPercentage = 0;

	//Be sure to scale by the proper anim speed just as if we were going to play the animation
	BG_SaberStartTransAnim(cg.predictedPlayerState.clientNum, cg.predictedPlayerState.fd.saberAnimLevel, cg.predictedPlayerState.weapon, cg.predictedPlayerState.torsoAnim, &animSpeedFactor, cg.predictedPlayerState.brokenLimbs);
	speedDif = attackAnimLength - (attackAnimLength * animSpeedFactor);
	attackAnimLength += speedDif;

	currentPoint = cg.predictedPlayerState.torsoTimer;

	animPercentage = currentPoint/attackAnimLength;

	//Com_Printf("Torso Animation Float Percentage: %f\n", animPercentage);

	return animPercentage;
	*/
	//[/BugFix2]
}


/*
===============
SmoothTrueView

  Purpose:  Uses the currently setup model-based First Person View to calculation the final viewangles.  Features the 
				following:
				1.  Simulates allowable eye movement by makes a deadzone around the inputed viewangles vs the desired
					viewangles of cg.refdef.viewangles
				2.  Prevents the sudden view flipping during moves where your camera is suppose to flip 360 on the pitch (x)
					pitch (x) axis. 
===============
*/

void SmoothTrueView(vec3_t eyeAngles)
{
	float LegAnimPoint = GetSelfLegAnimPoint();
	float TorsoAnimPoint = GetSelfTorsoAnimPoint();
	
	//counter
	int		i;

	//cg.refdef.viewangles in relation to eyeAngles
	float	AngDiff;

	qboolean	eyeRange = qtrue;
	qboolean	UseRefDef = qfalse;
	qboolean	DidSpecial = qfalse;

	//Debug messages
	//Com_Printf("eyeAngles: %f, %f, %f\n", eyeAngles[0], eyeAngles[1], eyeAngles[2]);
	//Com_Printf("cg.refdef.viewangles: %f, %f, %f\n", cg.refdef.viewangles[0], cg.refdef.viewangles[1], cg.refdef.viewangles[2]);
	


	//RAFIXME: See if I can find a link this to the prediction stuff.  I think the snap is of just the last gamestate snap 

	//Rolls
	if ( cg_trueroll.integer )
	{
		if ( ( (cg.predictedPlayerState.legsAnim) == BOTH_WALL_RUN_LEFT )
			|| ( (cg.predictedPlayerState.legsAnim) == BOTH_WALL_RUN_RIGHT )
			|| ( (cg.predictedPlayerState.legsAnim) == BOTH_WALL_RUN_LEFT_STOP )
			|| ( (cg.predictedPlayerState.legsAnim) == BOTH_WALL_RUN_RIGHT_STOP )
			|| ( (cg.predictedPlayerState.legsAnim) == BOTH_WALL_RUN_LEFT_FLIP )
			|| ( (cg.predictedPlayerState.legsAnim) == BOTH_WALL_RUN_RIGHT_FLIP )
			|| ( (cg.predictedPlayerState.legsAnim) == BOTH_WALL_FLIP_LEFT )
			|| ( (cg.predictedPlayerState.legsAnim) == BOTH_WALL_FLIP_RIGHT ) )
		{//Roll moves that look good with eye range
			eyeRange = qtrue;
			DidSpecial = qtrue;
		}
		else if ( cg_trueroll.integer == 1 )
		{//Use simple roll for the more complicated rolls
			if ( ( (cg.predictedPlayerState.legsAnim) == BOTH_FLIP_L ) 
				|| ( (cg.predictedPlayerState.legsAnim) == BOTH_ROLL_L ) ) 
			{//Left rolls
				VectorCopy( cg.refdef.viewangles, eyeAngles );
				eyeAngles[2] += AngleNormalize180( (360 * LegAnimPoint) );
				AngleNormalize180( eyeAngles[2] );
				eyeRange = qfalse;
				DidSpecial = qtrue;
			}
			else if( ((cg.predictedPlayerState.legsAnim) == BOTH_FLIP_R)
				|| ((cg.predictedPlayerState.legsAnim) == BOTH_ROLL_R) )
			{//Right rolls
				VectorCopy( cg.refdef.viewangles, eyeAngles );
				eyeAngles[2] += AngleNormalize180( ( 360 - (360 * LegAnimPoint) ) );
				AngleNormalize180( eyeAngles[2] );
				eyeRange = qfalse;
				DidSpecial = qtrue;
			}
		}
		else
		{//You're here because you're using cg_trueroll.integer == 2
			if ( ((cg.predictedPlayerState.legsAnim) == BOTH_FLIP_L)
				|| ((cg.predictedPlayerState.legsAnim) == BOTH_ROLL_L)
				|| ((cg.predictedPlayerState.legsAnim) == BOTH_FLIP_R)
				|| ((cg.predictedPlayerState.legsAnim) == BOTH_ROLL_R) )
			{//Roll animation, lock the eyemovement
				eyeRange = qfalse;
				DidSpecial = qtrue;
			}
		}
	}
	else if ( ((cg.predictedPlayerState.legsAnim) == BOTH_WALL_RUN_LEFT)
			|| ((cg.predictedPlayerState.legsAnim) == BOTH_WALL_RUN_RIGHT)
			|| ((cg.predictedPlayerState.legsAnim) == BOTH_WALL_RUN_LEFT_STOP)
			|| ((cg.predictedPlayerState.legsAnim) ==	BOTH_WALL_RUN_RIGHT_STOP)
			|| ((cg.predictedPlayerState.legsAnim) == BOTH_WALL_RUN_LEFT_FLIP)
			|| ((cg.predictedPlayerState.legsAnim) == BOTH_WALL_RUN_RIGHT_FLIP)
			|| ((cg.predictedPlayerState.legsAnim) == BOTH_WALL_FLIP_LEFT)
			|| ((cg.predictedPlayerState.legsAnim) == BOTH_WALL_FLIP_RIGHT) 
			|| ((cg.predictedPlayerState.legsAnim) == BOTH_FLIP_L)
			|| ((cg.predictedPlayerState.legsAnim) == BOTH_ROLL_L)
			|| ((cg.predictedPlayerState.legsAnim) == BOTH_FLIP_R)
			|| ((cg.predictedPlayerState.legsAnim) == BOTH_ROLL_R) )
	{//you don't want rolling so use cg.refdef.viewangles as the view
		UseRefDef = qtrue;
	}

	//Flips
	if( cg_trueflip.integer )
	{
		if( ((cg.predictedPlayerState.legsAnim) == BOTH_WALL_FLIP_BACK1) )
		{//Flip moves that look good with the eyemovement locked
			eyeRange = qfalse;
			DidSpecial = qtrue;
		}
		else if ( cg_trueflip.integer == 1 )
		{//Use simple flip for the more complicated flips
			if ( ((cg.predictedPlayerState.legsAnim) == BOTH_FLIP_F)
				|| ((cg.predictedPlayerState.legsAnim) == BOTH_ROLL_F) )
			{//forward flips
				VectorCopy( cg.refdef.viewangles, eyeAngles );
				eyeAngles[0] += AngleNormalize180( 360 - (360 * LegAnimPoint) );
				AngleNormalize180( eyeAngles[0] );
				eyeRange = qfalse;
				DidSpecial = qtrue;
			}
			else if ( ((cg.predictedPlayerState.legsAnim) == BOTH_FLIP_B)
				|| ((cg.predictedPlayerState.legsAnim) == BOTH_ROLL_B)
				|| ((cg.predictedPlayerState.legsAnim) == BOTH_FLIP_BACK1) )
			{//back flips
				VectorCopy( cg.refdef.viewangles, eyeAngles );
				eyeAngles[0] += AngleNormalize180( (360 * LegAnimPoint) );
				AngleNormalize180( eyeAngles[0] );
				eyeRange = qfalse;
				DidSpecial = qtrue;
			}
		}
		else 
		{//You're here because you're using cg_trueflip.integer = 2
			if ( ( (cg.predictedPlayerState.legsAnim) == BOTH_FLIP_F )
				|| ( (cg.predictedPlayerState.legsAnim) == BOTH_ROLL_F )
				|| ( (cg.predictedPlayerState.legsAnim) == BOTH_FLIP_B )
				|| ( (cg.predictedPlayerState.legsAnim) == BOTH_ROLL_B )
				|| ( (cg.predictedPlayerState.legsAnim) == BOTH_FLIP_BACK1 ) )
			{//Flip animation and using cg_trueflip.integer = 2, lock the eyemovement
				eyeRange = qfalse;
				DidSpecial = qtrue;
			}
		}
	}
	else if ( ((cg.predictedPlayerState.legsAnim) == BOTH_WALL_FLIP_BACK1)
			|| ((cg.predictedPlayerState.legsAnim) == BOTH_FLIP_F)
			|| ((cg.predictedPlayerState.legsAnim) == BOTH_ROLL_F)
			|| ((cg.predictedPlayerState.legsAnim) == BOTH_FLIP_B)
			|| ((cg.predictedPlayerState.legsAnim) == BOTH_ROLL_B)
			|| ((cg.predictedPlayerState.legsAnim) == BOTH_FLIP_BACK1) )		
	{//you don't want flipping so use cg.refdef.viewangles as the view
		UseRefDef = qtrue;
	}



	if ( cg_truespin.integer )
	{
		if ( cg_truespin.integer == 1 )
		{//Do a simulated Spin for the more complicated spins
			if ( ((cg.predictedPlayerState.torsoAnim) == BOTH_T1_TL_BR)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T1__L_BR)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T1__L__R)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T1_BL_BR)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T1_BL__R)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T1_BL_TR)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T2__L_BR)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T2_BL_BR)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T2_BL__R)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T3__L_BR)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T3_BL_BR)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T3_BL__R)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T4__L_BR)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T4_BL_BR)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T4_BL__R)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T5_TL_BR)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T5__L_BR)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T5__L__R)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T5_BL_BR)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T5_BL__R)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T5_BL_TR)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_ATTACK_BACK)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_CROUCHATTACKBACK1)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_BUTTERFLY_LEFT)
				//This technically has 2 spins and seems to have been labeled wrong
				|| ((cg.predictedPlayerState.legsAnim) == BOTH_FJSS_TR_BL) )
			{//Left Spins
				VectorCopy( cg.refdef.viewangles, eyeAngles );
				eyeAngles[1] += AngleNormalize180( (360 - (360 * TorsoAnimPoint)) );
				AngleNormalize180( eyeAngles[1] );
				eyeRange = qfalse;
				DidSpecial = qtrue;
			}
			else if ( ((cg.predictedPlayerState.torsoAnim) == BOTH_T1_BR_BL)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T1__R__L)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T1__R_BL)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T1_TR_BL)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T1_BR_TL)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T1_BR__L)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T2_BR__L)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T2_BR_BL)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T2__R_BL)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T3_BR__L)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T3_BR_BL)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T3__R_BL)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T4_BR__L)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T4_BR_BL)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T4__R_BL)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T5_BR_BL)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T5__R__L)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T5__R_BL)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T5_TR_BL)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T5_BR_TL)
				|| ((cg.predictedPlayerState.torsoAnim) == BOTH_T5_BR__L)
				//This technically has 2 spins
				|| ((cg.predictedPlayerState.legsAnim) == BOTH_BUTTERFLY_RIGHT)
				//This technically has 2 spins and seems to have been labeled wrong
				|| ((cg.predictedPlayerState.legsAnim) == BOTH_FJSS_TL_BR) )
			{//Right Spins
				VectorCopy( cg.refdef.viewangles, eyeAngles );
				eyeAngles[1] += AngleNormalize180( (360 * TorsoAnimPoint) );
				AngleNormalize180( eyeAngles[1] );
				eyeRange = qfalse;
				DidSpecial = qtrue;
			}
		}
		else
		{//You're here because you're using cg_truespin.integer == 2 
			if ( BG_SpinningSaberAnim( (cg.predictedPlayerState.torsoAnim) )
				&& ((cg.predictedPlayerState.torsoAnim)  != BOTH_JUMPFLIPSLASHDOWN1)
				&& ((cg.predictedPlayerState.torsoAnim)  != BOTH_JUMPFLIPSTABDOWN) )
			{//Flip animation and using cg_trueflip.integer = 2, lock the eyemovement
				eyeRange = qfalse;
				DidSpecial = qtrue;
			}
		}
	}
	else if ( BG_SpinningSaberAnim( (cg.predictedPlayerState.torsoAnim) )
				&& ((cg.predictedPlayerState.torsoAnim)  != BOTH_JUMPFLIPSLASHDOWN1)
				&& ((cg.predictedPlayerState.torsoAnim)  != BOTH_JUMPFLIPSTABDOWN) )
	{//you don't want spinning so use cg.refdef.viewangles as the view
		UseRefDef = qtrue;
	}
	else if ( (cg.predictedPlayerState.legsAnim) == BOTH_JUMPATTACK6)
	{
		UseRefDef = qtrue;
	}

	//Prevent camera flicker while landing.
	if ( ((cg.predictedPlayerState.legsAnim)  == BOTH_LAND1)
		|| ((cg.predictedPlayerState.legsAnim)  == BOTH_LAND2)
		|| ((cg.predictedPlayerState.legsAnim)  == BOTH_LANDBACK1)
		|| ((cg.predictedPlayerState.legsAnim)  == BOTH_LANDLEFT1)
		|| ((cg.predictedPlayerState.legsAnim)  == BOTH_LANDRIGHT1) )
	{
		UseRefDef = qtrue;
	}

	//Prevent the camera flicker while switching to the saber.
	if ( ( (cg.predictedPlayerState.torsoAnim) ==	BOTH_STAND2TO1 )
		|| ( (cg.predictedPlayerState.torsoAnim) == BOTH_STAND1TO2 ) )
	{
		UseRefDef = qtrue;
	}

	//special camera view for blue backstab
	if ( (cg.predictedPlayerState.torsoAnim) == BOTH_A2_STABBACK1)
	{
		eyeRange = qfalse;
		DidSpecial = qtrue;
	}

	if ( ( (cg.predictedPlayerState.torsoAnim) == BOTH_JUMPFLIPSLASHDOWN1)
		|| ( (cg.predictedPlayerState.torsoAnim) == BOTH_JUMPFLIPSLASHDOWN1) )
	{
		eyeRange = qfalse;
		DidSpecial = qtrue;
	}


	if ( UseRefDef )
	{
		VectorCopy( cg.refdef.viewangles, eyeAngles );
	}
	else
	{
		//Movement Roll dampener
		if ( !DidSpecial )
		{
			if ( !cg_truemoveroll.integer )
			{
				eyeAngles[2] = cg.refdef.viewangles[2];
			}
			else if ( cg_truemoveroll.integer == 1 )
			{//dampen the movement leaning
				eyeAngles[2] *= .5;
			}
		}
	
		//eye movement
		if ( eyeRange )
		{//allow eye motion
			for (i = 0; i < 2; i++ )
			{
				int fov;
				if ( cg_truefov.integer )
				{
					fov = cg_truefov.value;
				}
				else
				{
					fov = cg_fov.value;
				}

				AngDiff = eyeAngles[i] - cg.refdef.viewangles[i];
	
				AngDiff = AngleNormalize180( AngDiff );
				if ( fabs( AngDiff ) > fov )
				{
					if ( AngDiff < 0 )
					{
						eyeAngles[i] += fov;
					}
					else
					{
						eyeAngles[i] -= fov;
					}
				}
				else
				{ 
					eyeAngles[i] = cg.refdef.viewangles[i];
				}
				AngleNormalize180( eyeAngles[i] );
			}
		}
	}
}
//[/TrueView]


//[VisualWeapons]
extern void *g2HolsterWeaponInstances[MAX_WEAPONS];
void *CG_G2HolsterWeaponInstance(centity_t *cent, int weapon, qboolean secondSaber);
void ApplyAxisRotation( vec3_t axis[3], int rotType, float value )
{//apply matrix rotation to this axis.
	//rotType = type of rotation (PITCH, YAW, ROLL)
	//value = size of rotation in degrees, no action if == 0
	vec3_t result[3];  //The resulting axis
	vec3_t rotation[3];  //rotation matrix
	int i, j; //multiplication counters

	if(value == 0)
	{//no rotation, just return.
		return;
	}

	//init rotation matrix
	switch(rotType)
	{
		case ROLL: //R_X
			rotation[0][0] = 1;
			rotation[0][1] = 0;
			rotation[0][2] = 0;

			rotation[1][0] = 0;
			rotation[1][1] = cos(value / 360 * (2 * M_PI));
			rotation[1][2] = -sin(value / 360 * (2 * M_PI));

			rotation[2][0] = 0;
			rotation[2][1] = sin(value / 360 * (2 * M_PI));
			rotation[2][2] = cos(value / 360 * (2 * M_PI));
			break;

		case PITCH: //R_Y
			rotation[0][0] = cos(value / 360 * (2 * M_PI));
			rotation[0][1] = 0;
			rotation[0][2] = sin(value / 360 * (2 * M_PI));

			rotation[1][0] = 0;
			rotation[1][1] = 1;
			rotation[1][2] = 0;

			rotation[2][0] = -sin(value / 360 * (2 * M_PI));
			rotation[2][1] = 0;
			rotation[2][2] = cos(value / 360 * (2 * M_PI));
			break;

		case YAW: //R_Z
			rotation[0][0] = cos(value / 360 * (2 * M_PI));
			rotation[0][1] = -sin(value / 360 * (2 * M_PI));
			rotation[0][2] = 0;

			rotation[1][0] = sin(value / 360 * (2 * M_PI));
			rotation[1][1] = cos(value / 360 * (2 * M_PI));
			rotation[1][2] = 0;

			rotation[2][0] = 0;
			rotation[2][1] = 0;
			rotation[2][2] = 1;
			break;

		default:
			CG_Printf("Error:  Bad rotType %i given to ApplyAxisRotation\n", rotType);
			break;
	};

	//apply rotation
	for( i=0; i < 3; i++ )
	{
		for ( j=0; j < 3; j++ )
		{
			result[i][j] = rotation[i][0]*axis[0][j] + rotation[i][1]*axis[1][j]
									 + rotation[i][2]*axis[2][j];
			/* post apply method
			result[i][j] = axis[i][0]*rotation[0][j] + axis[i][1]*rotation[1][j]
									 + axis[i][2]*rotation[2][j];
			*/
		}
	}

	//copy result
	AxisCopy(result, axis);

}


extern stringID_table_t holsterTypeTable[];
void CG_HolsteredWeaponRender( centity_t *cent, clientInfo_t *ci, int holsterType ) 
{
	refEntity_t		ent;
	vec3_t			axis[3];
	vec3_t			boltOrg;
	mdxaBone_t		boltMatrix;
	vec3_t			PosOffset;
	vec3_t			AngOffset;
	qhandle_t		offsetBolt;
	int				weaponType;
	qboolean		secondWeap = qfalse;
	int				boneIndex;

	if (cent->currentState.number == cg.snap->ps.clientNum &&
		!cg.renderingThirdPerson && !cg_trueguns.integer && cg.snap->ps.weapon != WP_SABER)
	{//don't render when in first person and not in True View.
		return;
	}

	if (!cent->ghoul2)
	{//no ghoul2 instance on player
		return;
	}

	//debugger override for editting the holster positions
	if(ojp_holsterdebug.integer == holsterType)
	{//debug has been set for this holsterType, use the debug overrides
		boneIndex = ojp_holsterdebug_boneindex.integer;
		sscanf(ojp_holsterdebug_posoffset.string, "%f %f %f", &PosOffset[0], &PosOffset[1], &PosOffset[2]);
		sscanf(ojp_holsterdebug_angoffset.string, "%f %f %f", &AngOffset[0], &AngOffset[1], &AngOffset[2]);
	}
	else
	{//use the model's loaded data for this holsterType
		boneIndex = ci->holsterData[holsterType].boneIndex;
		VectorCopy(ci->holsterData[holsterType].posOffset, PosOffset);
		VectorCopy(ci->holsterData[holsterType].angOffset, AngOffset);
	}

	if(boneIndex == HOLSTER_NONE)
	{//this weapon isn't set up to be rendered on this player model.
		return;
	}

	switch(holsterType)
	{
		case HLR_SINGLESABER_1:
			weaponType = WP_SABER;
			break;
		case HLR_SINGLESABER_2:
			weaponType = WP_SABER;
			secondWeap = qtrue;
			break;
		case HLR_STAFFSABER:
			weaponType = WP_SABER;
			break;
		case HLR_PISTOL_L:
		case HLR_PISTOL_R:
			weaponType = WP_BRYAR_PISTOL;
			break;
		case HLR_BLASTER_L:
		case HLR_BLASTER_R:
			weaponType = WP_BLASTER;
			break;
		case HLR_BRYARPISTOL_L:
		case HLR_BRYARPISTOL_R:
			weaponType = WP_BRYAR_OLD;
			break;
		case HLR_BOWCASTER:
			weaponType = WP_BOWCASTER;
			break;
		case HLR_ROCKET_LAUNCHER:
			weaponType = WP_ROCKET_LAUNCHER;
			break;
		case HLR_DEMP2:
			weaponType = WP_DEMP2;
			break;
		case HLR_CONCUSSION:
			weaponType = WP_CONCUSSION;
			break;
		case HLR_REPEATER:
			weaponType = WP_REPEATER;
			break;
		case HLR_FLECHETTE:
			weaponType = WP_FLECHETTE;
			break;
		case HLR_DISRUPTOR:
			weaponType = WP_DISRUPTOR;
			break;
		default:
			CG_Printf("Unknown weaponType for holsterType %i in CG_HolsteredWeaponRender.\n", holsterType);
			return;
			break;
	};

	//set offsetBolt
	switch(boneIndex)
	{
		case HOLSTER_UPPERBACK:	
			offsetBolt = 2;  //2 = jetpack tag position
			break;
		case HOLSTER_LOWERBACK:
		offsetBolt = ci->bolt_llumbar; //use lower lumbar bone
			break;
		case HOLSTER_LEFTHIP:
		offsetBolt = ci->bolt_lfemurYZ;  //use left hip bone
			break;
		case HOLSTER_RIGHTHIP:
			offsetBolt = ci->bolt_rfemurYZ; //use right hip bone
			break;
		default:
			CG_Printf("Unknown offsetBolt for boneIndex %i in CG_HolsteredWeaponRender.\n", boneIndex);
			return;
			break;
	};

	trap_G2API_GetBoltMatrix(cent->ghoul2, 0, offsetBolt, &boltMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, boltOrg);

	//find bolt rotation unit vectors
	BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_X, axis[0]);  //left/right	
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Z, axis[1]);  //fwd/back
	BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_Y, axis[2]);  //up/down?!
	
	memset( &ent, 0, sizeof( ent ) );

	//positional transitions
	VectorMA(boltOrg, PosOffset[0], axis[0], boltOrg);
	VectorMA(boltOrg, PosOffset[1], axis[1], boltOrg);
	VectorMA(boltOrg, PosOffset[2], axis[2], boltOrg);

	//rotational transitions
	//configure the initial rotational axis
	VectorCopy(axis[1], ent.axis[0]);
	VectorScale(axis[0], -1, ent.axis[1]); //reversed since this is a right hand rule system.
	VectorCopy(axis[2], ent.axis[2]);

	//debug/config rotation statement.

	ApplyAxisRotation(ent.axis, PITCH, AngOffset[PITCH]);
	ApplyAxisRotation(ent.axis, YAW, AngOffset[YAW]);
	ApplyAxisRotation(ent.axis, ROLL, AngOffset[ROLL]);

	VectorCopy(boltOrg, ent.origin);
	//AnglesToAxis( angles, ent.axis );

	//attach the ghoul2 weapon instance to the render entity.
	ent.ghoul2 = CG_G2HolsterWeaponInstance(cent, weaponType, secondWeap);

	trap_R_AddRefEntityToScene( &ent );
}


//Defines for the ghoul2 model indexes
#define		G2MODEL_SABER_HOLSTERED			4
#define		G2MODEL_SABER2_HOLSTERED		5
#define		G2MODEL_STAFF_HOLSTERED			6
#define		G2MODEL_BLASTER_HOLSTERED		7
#define		G2MODEL_BLASTER2_HOLSTERED		8
#define		G2MODEL_LAUNCHER_HOLSTERED		9
#define		G2MODEL_GOLAN_HOLSTERED			10


void CG_VisualWeaponsUpdate( centity_t *cent, clientInfo_t *ci )
{//renders holstered weapons on players.
	//flag to indicate that 
	//used to make sure that 
	qboolean backInUse = (cent->currentState.eFlags & EF_JETPACK) ? qtrue : qfalse;  
	int rightHipInUse = 0; //the weapon currently holstered on the right hip.
	int leftHipInUse = 0;
	int weapInv;

	if(cg.snap && cg.snap->ps.clientNum == cent->currentState.number)
	{//this cent is the client, use playerstate data
		if(ojp_holsteredweapons.integer < 1)
		{//no holstered weapon rendering
			return;
		}

		weapInv = cg.snap->ps.stats[STAT_WEAPONS];
	}
	else
	{//use event generated data.  Note that this data won't be updated if not all of the clients have the OJP client plugin.
		if(ojp_holsteredweapons.integer < 2)
		{//no rendering holstered weapons on other players.
			return;
		}
		else if(CG_IsMindTricked(cent->currentState.trickedentindex,
			cent->currentState.trickedentindex2,
			cent->currentState.trickedentindex3,
			cent->currentState.trickedentindex4,
			cg.predictedPlayerState.clientNum))
		{//this player has mind tricked the client, don't render weapons on this dude.
			return;
		}

		weapInv = cent->weapons;
	}

	if (cent->ghoul2 && 
	(cent->currentState.eType != ET_NPC 
	|| (cent->currentState.NPC_class != CLASS_VEHICLE 
	&& cent->currentState.NPC_class != CLASS_REMOTE
	&& cent->currentState.NPC_class != CLASS_SEEKER)) //don't add weapon models to NPCs that have no bolt for them!
		&& !(cent->currentState.eFlags & EF_DEAD)	//dead players don't have holstered weapons.
		&& !cent->torsoBolt 
		&& cg.snap )
	{//this player can have holstered weapons
		//sabers
		if(weapInv & (1 << WP_SABER))
		{//have saber in inventory
			if(cent->currentState.weapon != WP_SABER)
			{//saber is holstered. render as nessicary.
				if(!(ci->saber[0].saberFlags & SFL_TWO_HANDED))
				{//using single saber(s)
					if(cent->currentState.saberEntityNum && !cent->currentState.saberInFlight)
					{//only have holstered saber if we current have our saber in our posession.
						if( ci->holster_saber != -1 )
						{//have specialized holster surface tag for single sabers
							if(!ci->saber_holstered)
							{//we haven't bolted the saber to the model yet. Do it now.
								trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_SABER), 0, 
									cent->ghoul2, G2MODEL_SABER_HOLSTERED);
								trap_G2API_SetBoltInfo(cent->ghoul2, G2MODEL_SABER_HOLSTERED, ci->holster_saber);
								ci->saber_holstered = qtrue;
							}
						}
						else
						{//use offset method
							CG_HolsteredWeaponRender(cent, ci, HLR_SINGLESABER_1);
						}
					}

					//secondary saber
					if(ci->saber[1].model && ci->ghoul2Weapons[1])
					{//have a second saber
						if( ci->holster_saber2 != -1 )
						{//use specialized bolt.
							trap_G2API_CopySpecificGhoul2Model(ci->ghoul2Weapons[1], 0, cent->ghoul2, 
								G2MODEL_SABER2_HOLSTERED);
							trap_G2API_SetBoltInfo(cent->ghoul2, G2MODEL_SABER2_HOLSTERED, ci->holster_saber2);
							ci->saber2_holstered = qtrue;
						}
						else
						{//use offset method
							CG_HolsteredWeaponRender(cent, ci, HLR_SINGLESABER_2);
						}
					}
				}
				else
				{//using staff saber
					if(cent->currentState.saberEntityNum && !cent->currentState.saberInFlight)
					{//only have holstered saber if we current have our saber in our posession.
						if(!backInUse)
						{//only use if you're not wearing a jetpack.
							if( ci->holster_staff != -1 )
							{//have specialized holster surface tag for single sabers
								if(!ci->saber_holstered)
								{//we haven't bolted the saber to the model yet. Do it now.
									trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_SABER), 0, 
										cent->ghoul2, G2MODEL_STAFF_HOLSTERED);
									trap_G2API_SetBoltInfo(cent->ghoul2, G2MODEL_STAFF_HOLSTERED, ci->holster_staff);
									ci->staff_holstered = qtrue;
								}
							}
							else
							{//use offset method
								CG_HolsteredWeaponRender(cent, ci, HLR_STAFFSABER);
							}
							//let the system know that we're using the back holster position at the moment.
							backInUse = qtrue;
						}
					}
				}
			}
			else
			{//sabers in use, remove models from bolts if being used.
				if(ci->holster_saber)
				{//single saber in use and is bolted, remove
					if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_SABER_HOLSTERED))
					{
						trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_SABER_HOLSTERED);
					}
					ci->saber_holstered = qfalse;
				}

				if(ci->staff_holstered)
				{//staff saber in use and is bolted, remove
					if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_STAFF_HOLSTERED))
					{
						trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_STAFF_HOLSTERED);
					}
					ci->staff_holstered = qfalse;
				}

				if( ci->saber2_holstered )
				{//second saber in use and is bolted, remove.
					if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_SABER2_HOLSTERED))
					{
						trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_SABER2_HOLSTERED);
					}
					ci->saber2_holstered = qfalse;
				}
			}
		}
		else 
		{//don't have any sabers, remove them from bolts if we have them.
			if(ci->holster_saber)
			{//single saber in use and is bolted, remove
				if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_SABER_HOLSTERED))
				{
					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_SABER_HOLSTERED);
				}
				ci->saber_holstered = qfalse;
			}

			if(ci->staff_holstered)
			{//staff saber in use and is bolted, remove
				if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_STAFF_HOLSTERED))
				{
					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_STAFF_HOLSTERED);
				}
				ci->staff_holstered = qfalse;
			}

			if( ci->saber2_holstered )
			{//second saber in use and is bolted, remove.
				if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_SABER2_HOLSTERED))
				{
					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_SABER2_HOLSTERED);
				}
				ci->saber2_holstered = qfalse;
			}
		}

		//Handle Blaster Holster on right hip
		if( !(weapInv & (1 << WP_BLASTER))  //don't have blaster 
			|| cent->currentState.weapon == WP_BLASTER) //or are currently using blaster
		{//don't need holstered blaster rendered
			if(ci->holster_blaster != -1 && ci->blaster_holstered == WP_BLASTER)
			{
				if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_BLASTER_HOLSTERED))
				{
					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_BLASTER_HOLSTERED);
				}
				ci->blaster_holstered = 0;
			}
		}
		else
		{//need holstered blaster to be rendered
			if(ci->holster_blaster != -1)
			{//have specialized bolt
				if(ci->blaster_holstered != WP_BLASTER)
				{//don't already have the blaster bolted.
					if(ci->blaster_holstered != 0)
					{//we have something else bolted there, remove it first.
						if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_BLASTER_HOLSTERED))
						{
							trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_BLASTER_HOLSTERED);
						}
						ci->blaster_holstered = 0;
					}

					//now add the blaster
					trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_BLASTER), 0, cent->ghoul2, G2MODEL_BLASTER_HOLSTERED);
					trap_G2API_SetBoltInfo(cent->ghoul2, G2MODEL_BLASTER_HOLSTERED, ci->holster_blaster);
					ci->blaster_holstered = WP_BLASTER;
				}
			}
			else
			{//manually render the blaster
				CG_HolsteredWeaponRender(cent, ci, HLR_BLASTER_R);
			}
			rightHipInUse = WP_BLASTER;
		}

		//handle rendering WP_BRYAR_PISTOL on right hip holster
		if(rightHipInUse //hip in use already
			|| !(weapInv & (1 << WP_BRYAR_PISTOL)) //don't have the WP_BRYAR_PISTOL
			|| cent->currentState.weapon == WP_BRYAR_PISTOL) //currently using WP_BRYAR_PISTOL
		{//don't render WP_BRYAR_PISTOL on right hip.
			if(ci->holster_blaster != -1 && ci->blaster_holstered == WP_BRYAR_PISTOL)
			{
				if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_BLASTER_HOLSTERED))
				{
					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_BLASTER_HOLSTERED);
				}
				ci->blaster_holstered = 0;
			}
		}
		else
		{//render WP_BRYAR_PISTOL on right hip
			if(ci->holster_blaster != -1)
			{//have specialized bolt
				if(ci->blaster_holstered != WP_BRYAR_PISTOL)
				{//don't already have the WP_BRYAR_PISTOL bolted.
					if(ci->blaster_holstered != 0)
					{//we have something else bolted there, remove it first.
						if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_BLASTER_HOLSTERED))
						{
							trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_BLASTER_HOLSTERED);
						}
						ci->blaster_holstered = 0;
					}

					//now add the WP_BRYAR_PISTOL
					trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_BRYAR_PISTOL), 0, cent->ghoul2, 
						G2MODEL_BLASTER_HOLSTERED);
					trap_G2API_SetBoltInfo(cent->ghoul2, G2MODEL_BLASTER_HOLSTERED, ci->holster_blaster);
					ci->blaster_holstered = WP_BRYAR_PISTOL;
				}
			}
			else
			{//manually render the pistol
				CG_HolsteredWeaponRender(cent, ci, HLR_PISTOL_R);
			}
			rightHipInUse = WP_BRYAR_PISTOL;
		}

		//handle old school pistol
		if(rightHipInUse //hip in use already
			|| !(weapInv & (1 << WP_BRYAR_OLD)) //don't have the WP_BRYAR_OLD
			|| cent->currentState.weapon == WP_BRYAR_OLD) //currently using WP_BRYAR_OLD
		{//don't render WP_BRYAR_OLD on right hip.
			if(ci->holster_blaster != -1 && ci->blaster_holstered == WP_BRYAR_OLD)
			{
				if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_BLASTER_HOLSTERED))
				{
					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_BLASTER_HOLSTERED);
				}
				ci->blaster_holstered = 0;
			}
		}
		else
		{//render WP_BRYAR_OLD on right hip
			if(ci->holster_blaster != -1)
			{//have specialized bolt
				if(ci->blaster_holstered != WP_BRYAR_OLD)
				{//don't already have the WP_BRYAR_OLD bolted.
					if(ci->blaster_holstered != 0)
					{//we have something else bolted there, remove it first.
						if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_BLASTER_HOLSTERED))
						{
							trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_BLASTER_HOLSTERED);
						}
						ci->blaster_holstered = 0;
					}

					//now add the WP_BRYAR_OLD
					trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_BRYAR_OLD), 0, cent->ghoul2, 
						G2MODEL_BLASTER_HOLSTERED);
					trap_G2API_SetBoltInfo(cent->ghoul2, G2MODEL_BLASTER_HOLSTERED, ci->holster_blaster);
					ci->blaster_holstered = WP_BRYAR_OLD;
				}
			}
			else
			{//manually render the pistol
				CG_HolsteredWeaponRender(cent, ci, HLR_BRYARPISTOL_R);
			}
			rightHipInUse = WP_BRYAR_OLD;
		}


		/*============================
		 * Start Left Hip Holster code
		 *============================
		 */
		//Handle Blaster Holster on left hip
		if( !(weapInv & (1 << WP_BLASTER))  //don't have blaster 
			|| cent->currentState.weapon == WP_BLASTER  //or are currently using blaster
			|| rightHipInUse == WP_BLASTER)  //or the blaster is already on the right hip. 

		{//don't need holstered blaster on left hip rendered
			if(ci->holster_blaster2 != -1 && ci->blaster2_holstered == WP_BLASTER)
			{//remove bolted holster instance.
				if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_BLASTER2_HOLSTERED))
				{
					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_BLASTER2_HOLSTERED);
				}
				ci->blaster2_holstered = 0;
			}
		}
		else
		{//need holstered blaster to be rendered
			if(ci->holster_blaster2 != -1)
			{//have specialized bolt
				if(ci->blaster2_holstered != WP_BLASTER)
				{//don't already have the blaster bolted.
					if(ci->blaster2_holstered != 0)
					{//we have something else bolted there, remove it first.
						if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_BLASTER2_HOLSTERED))
						{
							trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_BLASTER2_HOLSTERED);
						}
						ci->blaster2_holstered = 0;
					}

					//now add the blaster
					trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_BLASTER), 0, cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED);
					trap_G2API_SetBoltInfo(cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED, ci->holster_blaster2);
					ci->blaster2_holstered = WP_BLASTER;
				}
			}
			else
			{//manually render the blaster
				CG_HolsteredWeaponRender(cent, ci, HLR_BLASTER_L);
			}
			leftHipInUse = WP_BLASTER;
		}

		//Handle pistol Holster on left hip
		if( leftHipInUse
			|| !(weapInv & (1 << WP_BRYAR_PISTOL))  //don't have pistol 
			|| cent->currentState.weapon == WP_BRYAR_PISTOL  //or are currently using pistol
			|| rightHipInUse == WP_BRYAR_PISTOL)  //or the pistol is already on the right hip. 
		{//don't need holstered pistol on left hip rendered
			if(ci->holster_blaster2 != -1 && ci->blaster2_holstered == WP_BRYAR_PISTOL)
			{//remove bolted holster instance.
				if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_BLASTER2_HOLSTERED))
				{
					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_BLASTER2_HOLSTERED);
				}
				ci->blaster2_holstered = 0;
			}
		}
		else
		{//need holstered pistol to be rendered
			if(ci->holster_blaster2 != -1)
			{//have specialized bolt
				if(ci->blaster2_holstered != WP_BRYAR_PISTOL)
				{//don't already have the pistol bolted.
					if(ci->blaster2_holstered != 0)
					{//we have something else bolted there, remove it first.
						if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_BLASTER2_HOLSTERED))
						{
							trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_BLASTER2_HOLSTERED);
						}
						ci->blaster2_holstered = 0;
					}

					//now add the blaster
					trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_BRYAR_PISTOL), 0, cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED);
					trap_G2API_SetBoltInfo(cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED, ci->holster_blaster2);
					ci->blaster2_holstered = WP_BRYAR_PISTOL;
				}
			}
			else
			{//manually render the pistol
				CG_HolsteredWeaponRender(cent, ci, HLR_PISTOL_L);
			}
			leftHipInUse = WP_BRYAR_PISTOL;
		}

		//Handle pistol Holster on left hip
		if( leftHipInUse
			|| !(weapInv & (1 << WP_BRYAR_OLD))  //don't have pistol 
			|| cent->currentState.weapon == WP_BRYAR_OLD  //or are currently using pistol
			|| rightHipInUse == WP_BRYAR_OLD)  //or the pistol is already on the right hip. 
		{//don't need holstered pistol on left hip rendered
			if(ci->holster_blaster2 != -1 && ci->blaster2_holstered == WP_BRYAR_OLD)
			{//remove bolted holster instance.
				if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_BLASTER2_HOLSTERED))
				{
					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_BLASTER2_HOLSTERED);
				}
				ci->blaster2_holstered = 0;
			}
		}
		else
		{//need holstered pistol to be rendered
			if(ci->holster_blaster2 != -1)
			{//have specialized bolt
				if(ci->blaster2_holstered != WP_BRYAR_OLD)
				{//don't already have the pistol bolted.
					if(ci->blaster2_holstered != 0)
					{//we have something else bolted there, remove it first.
						if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_BLASTER2_HOLSTERED))
						{
							trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_BLASTER2_HOLSTERED);
						}
						ci->blaster2_holstered = 0;
					}

					//now add the blaster
					trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_BRYAR_OLD), 0, cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED);
					trap_G2API_SetBoltInfo(cent->ghoul2, G2MODEL_BLASTER2_HOLSTERED, ci->holster_blaster2);
					ci->blaster2_holstered = WP_BRYAR_OLD;
				}
			}
			else
			{//manually render the pistol
				CG_HolsteredWeaponRender(cent, ci, HLR_BRYARPISTOL_L);
			}
			leftHipInUse = WP_BRYAR_OLD;
		}
		
		/*============================
		 * End Left Hip Holster code
		 *============================
		 */

		/*============================
		 * Start Back Gun Holster code
		 *============================
		 */

		//Golan Rocket Launcher
		if( weapInv & (1 << WP_ROCKET_LAUNCHER) )
		{//have launcher
			if(cent->currentState.weapon != WP_ROCKET_LAUNCHER && !backInUse )
			{//need to render holstered rocket launcher
				if( ci->holster_golan != -1 && !ci->golan_holstered)
				{//we have a valid holster bolt for this weapon and we haven't bolted
					trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_ROCKET_LAUNCHER), 0, cent->ghoul2, G2MODEL_GOLAN_HOLSTERED);
					trap_G2API_SetBoltInfo(cent->ghoul2, G2MODEL_GOLAN_HOLSTERED, ci->holster_golan);
					ci->golan_holstered = qtrue;
				}
				else
				{//manual offset method
					CG_HolsteredWeaponRender(cent, ci, HLR_ROCKET_LAUNCHER);
				}
				backInUse = qtrue;
			}
			else if(ci->golan_holstered)
			{//remove holstered instace if exists.
				if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_GOLAN_HOLSTERED))
				{
					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_GOLAN_HOLSTERED);
				}
				ci->golan_holstered = qfalse;
			}
		}
		else if(ci->golan_holstered)
		{//remove holstered instace if exists.
			if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_GOLAN_HOLSTERED))
			{
				trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_GOLAN_HOLSTERED);
			}
			ci->golan_holstered = qfalse;
		}

		//handle concussion on back
		if(backInUse //back in use already
			|| !(weapInv & (1 << WP_CONCUSSION)) //don't have the concussion
			|| cent->currentState.weapon == WP_CONCUSSION) //currently using concussion
		{//don't render weapon on back
			if(ci->holster_launcher != -1 && ci->launcher_holstered == WP_CONCUSSION)
			{
				if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED))
				{
					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED);
				}
				ci->launcher_holstered = 0;
			}
		}
		else
		{//render concussion on right hip
			if(ci->holster_launcher != -1)
			{//have specialized bolt
				if(ci->launcher_holstered != WP_CONCUSSION)
				{//don't already have the concussion bolted.
					if(ci->launcher_holstered != 0)
					{//we have something else bolted there, remove it first.
						if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED))
						{
							trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED);
						}
						ci->launcher_holstered = 0;
					}

					//now add the concussion
					trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_CONCUSSION), 0, cent->ghoul2, 
						G2MODEL_LAUNCHER_HOLSTERED);
					trap_G2API_SetBoltInfo(cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED, ci->holster_launcher);
					ci->launcher_holstered = WP_CONCUSSION;
				}
			}
			else
			{//manually render the blaster
				CG_HolsteredWeaponRender(cent, ci, HLR_CONCUSSION);
			}
			backInUse = qtrue;
		}

		//handle repeater on back
		if(backInUse //back in use already
			|| !(weapInv & (1 << WP_REPEATER)) //don't have weapon
			|| cent->currentState.weapon == WP_REPEATER) //currently using weapon
		{//don't render weapon on back
			if(ci->holster_launcher != -1 && ci->launcher_holstered == WP_REPEATER)
			{
				if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED))
				{
					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED);
				}
				ci->launcher_holstered = 0;
			}
		}
		else
		{//render weapon on back
			if(ci->holster_launcher != -1)
			{//have specialized bolt
				if(ci->launcher_holstered != WP_REPEATER)
				{//don't already have the concussion bolted.
					if(ci->launcher_holstered != 0)
					{//we have something else bolted there, remove it first.
						if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED))
						{
							trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED);
						}
						ci->launcher_holstered = 0;
					}

					//now bolt the weapon
					trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_REPEATER), 0, cent->ghoul2, 
						G2MODEL_LAUNCHER_HOLSTERED);
					trap_G2API_SetBoltInfo(cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED, ci->holster_launcher);
					ci->launcher_holstered = WP_REPEATER;
				}
			}
			else
			{//manually render the weapon
				CG_HolsteredWeaponRender(cent, ci, HLR_REPEATER);
			}
			backInUse = qtrue;
		}

		//handle flechette on back
		if(backInUse //back in use already
			|| !(weapInv & (1 << WP_FLECHETTE)) //don't have weapon
			|| cent->currentState.weapon == WP_FLECHETTE) //currently using weapon
		{//don't render weapon on back
			if(ci->holster_launcher != -1 && ci->launcher_holstered == WP_FLECHETTE)
			{
				if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED))
				{
					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED);
				}
				ci->launcher_holstered = 0;
			}
		}
		else
		{//render weapon on back
			if(ci->holster_launcher != -1)
			{//have specialized bolt
				if(ci->launcher_holstered != WP_FLECHETTE)
				{//don't already have the weapon bolted.
					if(ci->launcher_holstered != 0)
					{//we have something else bolted there, remove it first.
						if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED))
						{
							trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED);
						}
						ci->launcher_holstered = 0;
					}

					//now bolt the weapon
					trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_FLECHETTE), 0, cent->ghoul2, 
						G2MODEL_LAUNCHER_HOLSTERED);
					trap_G2API_SetBoltInfo(cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED, ci->holster_launcher);
					ci->launcher_holstered = WP_FLECHETTE;
				}
			}
			else
			{//manually render the weapon
				CG_HolsteredWeaponRender(cent, ci, HLR_FLECHETTE);
			}
			backInUse = qtrue;
		}

		//handle disruptor on back
		if(backInUse //back in use already
			|| !(weapInv & (1 << WP_DISRUPTOR)) //don't have weapon
			|| cent->currentState.weapon == WP_DISRUPTOR) //currently using weapon
		{//don't render weapon on back
			if(ci->holster_launcher != -1 && ci->launcher_holstered == WP_DISRUPTOR)
			{
				if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED))
				{
					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED);
				}
				ci->launcher_holstered = 0;
			}
		}
		else
		{//render weapon on back
			if(ci->holster_launcher != -1)
			{//have specialized bolt
				if(ci->launcher_holstered != WP_DISRUPTOR)
				{//don't already have the weapon bolted.
					if(ci->launcher_holstered != 0)
					{//we have something else bolted there, remove it first.
						if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED))
						{
							trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED);
						}
						ci->launcher_holstered = 0;
					}

					//now bolt the weapon
					trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_DISRUPTOR), 0, cent->ghoul2, 
						G2MODEL_LAUNCHER_HOLSTERED);
					trap_G2API_SetBoltInfo(cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED, ci->holster_launcher);
					ci->launcher_holstered = WP_DISRUPTOR;
				}
			}
			else
			{//manually render the weapon
				CG_HolsteredWeaponRender(cent, ci, HLR_DISRUPTOR);
			}
			backInUse = qtrue;
		}

		//handle bowcaster on back
		if(backInUse //back in use already
			|| !(weapInv & (1 << WP_BOWCASTER)) //don't have weapon
			|| cent->currentState.weapon == WP_BOWCASTER) //currently using weapon
		{//don't render weapon on back
			if(ci->holster_launcher != -1 && ci->launcher_holstered == WP_BOWCASTER)
			{
				if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED))
				{
					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED);
				}
				ci->launcher_holstered = 0;
			}
		}
		else
		{//render weapon on back
			if(ci->holster_launcher != -1)
			{//have specialized bolt
				if(ci->launcher_holstered != WP_BOWCASTER)
				{//don't already have the weapon bolted.
					if(ci->launcher_holstered != 0)
					{//we have something else bolted there, remove it first.
						if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED))
						{
							trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED);
						}
						ci->launcher_holstered = 0;
					}

					//now bolt the weapon
					trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_BOWCASTER), 0, cent->ghoul2, 
						G2MODEL_LAUNCHER_HOLSTERED);
					trap_G2API_SetBoltInfo(cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED, ci->holster_launcher);
					ci->launcher_holstered = WP_BOWCASTER;
				}
			}
			else
			{//manually render the weapon
				CG_HolsteredWeaponRender(cent, ci, HLR_BOWCASTER);
			}
			backInUse = qtrue;
		}

		//handle demp 2 on back
		if(backInUse //back in use already
			|| !(weapInv & (1 << WP_DEMP2)) //don't have the demp2
			|| cent->currentState.weapon == WP_DEMP2) //currently using Demp2
		{//don't render Demp2 on right hip.
			if(ci->holster_launcher != -1 && ci->launcher_holstered == WP_DEMP2)
			{
				if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED))
				{
					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED);
				}
				ci->launcher_holstered = 0;
			}
		}
		else
		{//render demp 2 on right hip
			if(ci->holster_launcher != -1)
			{//have specialized bolt
				if(ci->launcher_holstered != WP_DEMP2)
				{//don't already have the demp2 bolted.
					if(ci->launcher_holstered != 0)
					{//we have something else bolted there, remove it first.
						if(trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED))
						{
							trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), G2MODEL_LAUNCHER_HOLSTERED);
						}
						ci->launcher_holstered = 0;
					}

					//now add the demp2
					trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_DEMP2), 0, cent->ghoul2, 
						G2MODEL_LAUNCHER_HOLSTERED);
					trap_G2API_SetBoltInfo(cent->ghoul2, G2MODEL_LAUNCHER_HOLSTERED, ci->holster_launcher);
					ci->launcher_holstered = WP_DEMP2;
				}
			}
			else
			{//manually render the blaster
				CG_HolsteredWeaponRender(cent, ci, HLR_DEMP2);
			}
			backInUse = qtrue;
		}

		/*============================
		 * End Back Gun Holster code
		 *============================
		 */

	}
}
//[/VisualWeapons]


void CG_Player( centity_t *cent ) {
	clientInfo_t	*ci;
	refEntity_t		legs;
	refEntity_t		torso;
	int				clientNum;
	int				renderfx;
	qboolean		shadow = qfalse;
	float			shadowPlane = 0;
	qboolean		dead = qfalse;
	vec3_t			rootAngles;
	float			angle;
	vec3_t			angles, dir, elevated, enang, seekorg;
	int				iwantout = 0, successchange = 0;
	int				team;
	mdxaBone_t 		boltMatrix, lHandMatrix;
	int				doAlpha = 0;
	qboolean		gotLHandMatrix = qfalse;
	qboolean		g2HasWeapon = qfalse;
	qboolean		drawPlayerSaber = qfalse;
	qboolean		checkDroidShields = qfalse;

	//first if we are not an npc and we are using an emplaced gun then make sure our
	//angles are visually capped to the constraints (otherwise it's possible to lerp
	//a little outside and look kind of twitchy)
	if (cent->currentState.weapon == WP_EMPLACED_GUN &&
		cent->currentState.otherEntityNum2)
	{
		float empYaw;

		if (BG_EmplacedView(cent->lerpAngles, cg_entities[cent->currentState.otherEntityNum2].currentState.angles, &empYaw, cg_entities[cent->currentState.otherEntityNum2].currentState.origin2[0]))
		{
			cent->lerpAngles[YAW] = empYaw;
		}
	}

	if (cent->currentState.iModelScale)
	{ //if the server says we have a custom scale then set it now.
		cent->modelScale[0] = cent->modelScale[1] = cent->modelScale[2] = cent->currentState.iModelScale/100.0f;
		if ( cent->currentState.NPC_class != CLASS_VEHICLE )
		{
			if (cent->modelScale[2] && cent->modelScale[2] != 1.0f)
			{
				cent->lerpOrigin[2] += 24 * (cent->modelScale[2] - 1);
			}
		}
	}
	else
	{
		VectorClear(cent->modelScale);
	}

	if ((cg_smoothClients.integer || cent->currentState.heldByClient) && (cent->currentState.groundEntityNum >= ENTITYNUM_WORLD || cent->currentState.eType == ET_TERRAIN) &&
		!(cent->currentState.eFlags2 & EF2_HYPERSPACE) && cg.predictedPlayerState.m_iVehicleNum != cent->currentState.number)
	{ //always smooth when being thrown
		vec3_t			posDif;
		float			smoothFactor;
		int				k = 0;
		float			fTolerance = 20000.0f;

		if (cent->currentState.heldByClient)
		{ //smooth the origin more when in this state, because movement is origin-based on server.
			smoothFactor = 0.2f;
		}
		else if ( (cent->currentState.powerups & (1 << PW_SPEED)) ||
			(cent->currentState.forcePowersActive & (1 << FP_RAGE)) )
		{ //we're moving fast so don't smooth as much
			smoothFactor = 0.6f;
		}
		else if (cent->currentState.eType == ET_NPC && cent->currentState.NPC_class == CLASS_VEHICLE &&
			cent->m_pVehicle && cent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER)
		{ //greater smoothing for flying vehicles, since they move so fast
			fTolerance = 6000000.0f;//500000.0f; //yeah, this is so wrong..but..
			smoothFactor = 0.5f;
		}
		else
		{
			smoothFactor = 0.5f;
		}
		
		if (DistanceSquared(cent->beamEnd,cent->lerpOrigin) > smoothFactor*fTolerance) //10000
		{
			VectorCopy(cent->lerpOrigin, cent->beamEnd);
		}

		VectorSubtract(cent->lerpOrigin, cent->beamEnd, posDif);
		
		for (k=0;k<3;k++)
		{
			cent->beamEnd[k]=(cent->beamEnd[k]+posDif[k]*smoothFactor);
			cent->lerpOrigin[k]=cent->beamEnd[k];
		}
	}
	else
	{
		VectorCopy(cent->lerpOrigin, cent->beamEnd);
	}

	if (cent->currentState.m_iVehicleNum &&
		cent->currentState.NPC_class != CLASS_VEHICLE)
	{ //this player is riding a vehicle
		centity_t *veh = &cg_entities[cent->currentState.m_iVehicleNum];

		cent->lerpAngles[YAW] = veh->lerpAngles[YAW];

		//Attach ourself to the vehicle
        if (veh->m_pVehicle &&
			cent->playerState &&
			veh->playerState &&
			cent->ghoul2 &&
			veh->ghoul2 )
		{
			if ( veh->currentState.owner != cent->currentState.clientNum )
			{//FIXME: what about visible passengers?
				if ( CG_VehicleAttachDroidUnit( cent, &legs ) )
				{
					checkDroidShields = qtrue;
				}
			}
			else if ( veh->currentState.owner != ENTITYNUM_NONE)
			{//has a pilot...???
				vec3_t oldPSOrg;

				//make sure it has its pilot and parent set
				veh->m_pVehicle->m_pPilot = (bgEntity_t *)&cg_entities[veh->currentState.owner];
				veh->m_pVehicle->m_pParentEntity = (bgEntity_t *)veh;
	            
				VectorCopy(veh->playerState->origin, oldPSOrg);

				//update the veh's playerstate org for getting the bolt
				VectorCopy(veh->lerpOrigin, veh->playerState->origin);
				VectorCopy(cent->lerpOrigin, cent->playerState->origin);

				//Now do the attach
				VectorCopy(veh->lerpAngles, veh->playerState->viewangles);
				veh->m_pVehicle->m_pVehicleInfo->AttachRiders(veh->m_pVehicle);

				//copy the "playerstate origin" to the lerpOrigin since that's what we use to display
				VectorCopy(cent->playerState->origin, cent->lerpOrigin);

				VectorCopy(oldPSOrg, veh->playerState->origin);
			}
		}
	}

	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	if (cent->currentState.eType != ET_NPC)
	{
		clientNum = cent->currentState.clientNum;
		if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
			CG_Error( "Bad clientNum on player entity");
		}
		ci = &cgs.clientinfo[ clientNum ];
	}
	else
	{
		if (!cent->npcClient)
		{
			CG_CreateNPCClient(&cent->npcClient); //allocate memory for it

			if (!cent->npcClient)
			{
				assert(0);
				return;
			}

			memset(cent->npcClient, 0, sizeof(clientInfo_t));
			cent->npcClient->ghoul2Model = NULL;
			//[CoOp]
			//figure out the gender for this model
			cent->npcClient->gender = FindGender(CG_ConfigString( CS_MODELS+cent->currentState.modelindex ), cent);
			//[/CoOp]
		}

		assert(cent->npcClient);

		if (cent->npcClient->ghoul2Model != cent->ghoul2 && cent->ghoul2)
		{
			cent->npcClient->ghoul2Model = cent->ghoul2;
			if (cent->localAnimIndex <= 1)
			{
				cent->npcClient->bolt_rhand = trap_G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*r_hand");
				cent->npcClient->bolt_lhand = trap_G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*l_hand");

				//rhand must always be first bolt. lhand always second. Whichever you want the
				//jetpack bolted to must always be third.
				trap_G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*chestg");

				//claw bolts
				trap_G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*r_hand_cap_r_arm");
				trap_G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*l_hand_cap_l_arm");

				cent->npcClient->bolt_head = trap_G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "*head_top");
				if (cent->npcClient->bolt_head == -1)
				{
					cent->npcClient->bolt_head = trap_G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "ceyebrow");
				}
				cent->npcClient->bolt_motion = trap_G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "Motion");
				cent->npcClient->bolt_llumbar = trap_G2API_AddBolt(cent->npcClient->ghoul2Model, 0, "lower_lumbar");
			}
			else
			{
				cent->npcClient->bolt_rhand = -1;
				cent->npcClient->bolt_lhand = -1;
				cent->npcClient->bolt_head = -1;
				cent->npcClient->bolt_motion = -1;
				cent->npcClient->bolt_llumbar = -1;
			}
			cent->npcClient->team = TEAM_FREE;
			cent->npcClient->infoValid = qtrue;
		}
		ci = cent->npcClient;
	}

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !ci->infoValid ) {
		return;
	}

	// Add the player to the radar if on the same team and its a team game
	if (cgs.gametype >= GT_TEAM)
	{
		if ( cent->currentState.eType != ET_NPC &&
			cg.snap->ps.clientNum != cent->currentState.number &&
			ci->team == cg.snap->ps.persistant[PERS_TEAM] )
		{
			CG_AddRadarEnt(cent);
		}
	}

	if (cent->currentState.eType == ET_NPC &&
		cent->currentState.NPC_class == CLASS_VEHICLE)
	{ //add vehicles
		CG_AddRadarEnt(cent);
		if ( CG_InFighter() )
		{//this is a vehicle, bracket it
			if ( cg.predictedPlayerState.m_iVehicleNum != cent->currentState.clientNum )
			{//don't add the vehicle I'm in... :)
				CG_AddBracketedEnt(cent);
			}
		}

	}

	if (!cent->ghoul2)
	{ //not ready yet?
#ifdef _DEBUG
		Com_Printf("WARNING: Client %i has a null ghoul2 instance\n", cent->currentState.number);
#endif
		trap_G2API_ClearAttachedInstance(cent->currentState.number);

		if (ci->ghoul2Model &&
			trap_G2_HaveWeGhoul2Models(ci->ghoul2Model))
		{
#ifdef _DEBUG
			Com_Printf("Clientinfo instance was valid, duplicating for cent\n");
#endif
			trap_G2API_DuplicateGhoul2Instance(ci->ghoul2Model, &cent->ghoul2);

			//Attach the instance to this entity num so we can make use of client-server
			//shared operations if possible.
			trap_G2API_AttachInstanceToEntNum(cent->ghoul2, cent->currentState.number, qfalse);

			if (trap_G2API_AddBolt(cent->ghoul2, 0, "face") == -1)
			{ //check now to see if we have this bone for setting anims and such
				cent->noFace = qtrue;
			}

			cent->localAnimIndex = CG_G2SkelForModel(cent->ghoul2);
			//[ANIMEVENTS]
			//This will probably cause NPCs that use this code path to use their 
			//skeleton's animevent instead of their actual models.  Fortunately, I don't
			//think NPCs ever go thru here anyway.
			cent->eventAnimIndex = CG_ParseAnimationEvtFile( va("models/players/%s/", ci->modelName), cent->localAnimIndex, cgNumAnimEvents );
			//cent->eventAnimIndex = CG_G2EvIndexForModel(cent->ghoul2, cent->localAnimIndex);
			//[/ANIMEVENTS]
		}
		return;
	}

	if (ci->superSmoothTime)
	{ //do crazy smoothing
		if (ci->superSmoothTime > cg.time)
		{ //do it
			trap_G2API_AbsurdSmoothing(cent->ghoul2, qtrue);
		}
		else
		{ //turn it off
			ci->superSmoothTime = 0;
			trap_G2API_AbsurdSmoothing(cent->ghoul2, qfalse);
		}
	}

	if (cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{ //don't show all this shit during intermission
		if ( cent->currentState.eType == ET_NPC
			&& cent->currentState.NPC_class != CLASS_VEHICLE )
		{//NPC in intermission
		}
		else
		{//don't render players or vehicles in intermissions, allow other NPCs for scripts
			return;
		}
	}

	CG_VehicleEffects(cent);

	if ((cent->currentState.eFlags & EF_JETPACK) && !(cent->currentState.eFlags & EF_DEAD) &&
		cg_g2JetpackInstance)
	{ //should have a jetpack attached
		//1 is rhand weap, 2 is lhand weap (akimbo sabs), 3 is jetpack
		if (!trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 3))
		{
			trap_G2API_CopySpecificGhoul2Model(cg_g2JetpackInstance, 0, cent->ghoul2, 3); 
		}

		if (cent->currentState.eFlags & EF_JETPACK_ACTIVE)
		{
			mdxaBone_t mat;
			vec3_t flamePos, flameDir;
			int n = 0;

			while (n < 2)
			{
				//Get the position/dir of the flame bolt on the jetpack model bolted to the player
				trap_G2API_GetBoltMatrix(cent->ghoul2, 3, n, &mat, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
				BG_GiveMeVectorFromMatrix(&mat, ORIGIN, flamePos);

				if (n == 0)
				{
					BG_GiveMeVectorFromMatrix(&mat, NEGATIVE_Y, flameDir);
					VectorMA(flamePos, -9.5f, flameDir, flamePos);
					BG_GiveMeVectorFromMatrix(&mat, POSITIVE_X, flameDir);
					VectorMA(flamePos, -13.5f, flameDir, flamePos);
				}
				else
				{
					BG_GiveMeVectorFromMatrix(&mat, POSITIVE_X, flameDir);
					VectorMA(flamePos, -9.5f, flameDir, flamePos);
					BG_GiveMeVectorFromMatrix(&mat, NEGATIVE_Y, flameDir);
					VectorMA(flamePos, -13.5f, flameDir, flamePos);
				}

				if (cent->currentState.eFlags & EF_JETPACK_FLAMING)
				{ //create effects
					//FIXME: Just one big effect
					//Play the effect
					trap_FX_PlayEffectID(cgs.effects.mBobaJet, flamePos, flameDir, -1, -1);
					trap_FX_PlayEffectID(cgs.effects.mBobaJet, flamePos, flameDir, -1, -1);

					//Keep the jet fire sound looping
					trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, 
						trap_S_RegisterSound( "sound/effects/fire_lp" ) );
				}
				else
				{ //just idling
					//FIXME: Different smaller effect for idle
					//Play the effect
					trap_FX_PlayEffectID(cgs.effects.mBobaJet, flamePos, flameDir, -1, -1);
				}

				n++;
			}

			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, 
				trap_S_RegisterSound( "sound/boba/JETHOVER" ) );
		}
	}
	else if (trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 3))
	{ //fixme: would be good if this could be done not every frame
		trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), 3);
	}

	g2HasWeapon = trap_G2API_HasGhoul2ModelOnIndex(&(cent->ghoul2), 1);

	if (!g2HasWeapon)
	{ //force a redup of the weapon instance onto the client instance
		cent->ghoul2weapon = NULL;
		cent->weapon = 0;
	}

	if (cent->torsoBolt && !(cent->currentState.eFlags & EF_DEAD))
	{ //he's alive and has a limb missing still, reattach it and reset the weapon
		CG_ReattachLimb(cent);
	}

	if (cent->isRagging && !(cent->currentState.eFlags & EF_DEAD) && !(cent->currentState.eFlags & EF_RAG))
	{ //make sure we don't ragdoll ever while alive unless directly told to with eFlags
		cent->isRagging = qfalse;
		trap_G2API_SetRagDoll(cent->ghoul2, NULL); //calling with null parms resets to no ragdoll.
	}

	if (cent->ghoul2 && cent->torsoBolt && ((cent->torsoBolt & RARMBIT) || (cent->torsoBolt & RHANDBIT) || (cent->torsoBolt & WAISTBIT)) && g2HasWeapon)
	{ //kill the weapon if the limb holding it is no longer on the model
		trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), 1);
		g2HasWeapon = qfalse;
	}

	if (!cent->trickAlphaTime || (cg.time - cent->trickAlphaTime) > 1000)
	{ //things got out of sync, perhaps a new client is trying to fill in this slot
		cent->trickAlpha = 255;
		cent->trickAlphaTime = cg.time;
	}

	if (cent->currentState.eFlags & EF_NODRAW)
	{ //If nodraw, return here
		return;
	}
	else if (cent->currentState.eFlags2 & EF2_SHIP_DEATH)
	{ //died in ship, don't draw, we were "obliterated"
		return;
	}

	//If this client has tricked you.
	if (CG_IsMindTricked(cent->currentState.trickedentindex,
		cent->currentState.trickedentindex2,
		cent->currentState.trickedentindex3,
		cent->currentState.trickedentindex4,
		cg.snap->ps.clientNum))
	{
		if (cent->trickAlpha > 1)
		{
			cent->trickAlpha -= (cg.time - cent->trickAlphaTime)*0.5;
			cent->trickAlphaTime = cg.time;

			if (cent->trickAlpha < 0)
			{
				cent->trickAlpha = 0;
			}

			doAlpha = 1;
		}
		else
		{
			doAlpha = 1;
			cent->trickAlpha = 1;
			cent->trickAlphaTime = cg.time;
			iwantout = 1;
		}
	}
	else
	{
		if (cent->trickAlpha < 255)
		{
			cent->trickAlpha += (cg.time - cent->trickAlphaTime);
			cent->trickAlphaTime = cg.time;

			if (cent->trickAlpha > 255)
			{
				cent->trickAlpha = 255;
			}

			doAlpha = 1;
		}
		else
		{
			cent->trickAlpha = 255;
			cent->trickAlphaTime = cg.time;
		}
	}

	// get the player model information
	renderfx = 0;
	if ( cent->currentState.number == cg.snap->ps.clientNum) 
	{
		if (!cg.renderingThirdPerson) 
		{
			//[TrueView]
			if ( ( !cg_trueguns.integer && cg.predictedPlayerState.weapon != WP_SABER 
				&& cg.predictedPlayerState.weapon != WP_MELEE) 
				|| ( cg.predictedPlayerState.weapon == WP_SABER && cg_truesaberonly.integer )
				|| cg.predictedPlayerState.zoomMode)
			/*
#if 0
			if (!cg_fpls.integer || cent->currentState.weapon != WP_SABER)
#else
			if (cent->currentState.weapon != WP_SABER)
#endif
			*/
			{
				renderfx = RF_THIRD_PERSON;			// only draw in mirrors
			}
		} else 
		{
			if (cg_cameraMode.integer) 
			{
				iwantout = 1;

				
				// goto minimal_add;
				
				// NOTENOTE Temporary
				return;
			}
		}
	}

	// Update the player's client entity information regarding weapons.
	// Explanation:  The entitystate has a weapond defined on it.  The cliententity does as well.
	// The cliententity's weapon tells us what the ghoul2 instance on the cliententity has bolted to it.
	// If the entitystate and cliententity weapons differ, then the state's needs to be copied to the client.
	// Save the old weapon, to verify that it is or is not the same as the new weapon.
	// rww - Make sure weapons don't get set BEFORE cent->ghoul2 is initialized or else we'll have no
	// weapon bolted on
	if (cent->currentState.saberInFlight)
	{
		cent->ghoul2weapon = CG_G2WeaponInstance(cent, WP_SABER);
	}

	if (cent->ghoul2 && 
		(cent->currentState.eType != ET_NPC || (cent->currentState.NPC_class != CLASS_VEHICLE&&cent->currentState.NPC_class != CLASS_REMOTE&&cent->currentState.NPC_class != CLASS_SEEKER)) && //don't add weapon models to NPCs that have no bolt for them!
		cent->ghoul2weapon != CG_G2WeaponInstance(cent, cent->currentState.weapon) &&
		!(cent->currentState.eFlags & EF_DEAD) && !cent->torsoBolt &&
		cg.snap && (cent->currentState.number != cg.snap->ps.clientNum || (cg.snap->ps.pm_flags & PMF_FOLLOW)))
	{
		if (ci->team == TEAM_SPECTATOR)
		{
			cent->ghoul2weapon = NULL;
			cent->weapon = 0;
		}
		else
		{
			CG_CopyG2WeaponInstance(cent, cent->currentState.weapon, cent->ghoul2);

			if (cent->currentState.eType != ET_NPC)
			{
				if (cent->weapon == WP_SABER 
					&& cent->weapon != cent->currentState.weapon 
					&& !cent->currentState.saberHolstered)
				{ //switching away from the saber
					//trap_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, trap_S_RegisterSound( "sound/weapons/saber/saberoffquick.wav" ));
					if (ci->saber[0].soundOff 
						&& !cent->currentState.saberHolstered)
					{
						trap_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, ci->saber[0].soundOff);
					}

					if (ci->saber[1].soundOff &&
						ci->saber[1].model[0] &&
						!cent->currentState.saberHolstered)
					{
						trap_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, ci->saber[1].soundOff);
					}

				}
				else if (cent->currentState.weapon == WP_SABER
					&& cent->weapon != cent->currentState.weapon 
					&& !cent->saberWasInFlight)
				{ //switching to the saber
					//trap_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, trap_S_RegisterSound( "sound/weapons/saber/saberon.wav" ));
					if (ci->saber[0].soundOn)
					{
						trap_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, ci->saber[0].soundOn);
					}

					if (ci->saber[1].soundOn)
					{
						trap_S_StartSound(cent->lerpOrigin, cent->currentState.number, CHAN_AUTO, ci->saber[1].soundOn);
					}

					BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
					BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);
				}
			}

			cent->weapon = cent->currentState.weapon;
			cent->ghoul2weapon = CG_G2WeaponInstance(cent, cent->currentState.weapon);
		}
	}
	else if ((cent->currentState.eFlags & EF_DEAD) || cent->torsoBolt)
	{
		cent->ghoul2weapon = NULL; //be sure to update after respawning/getting limb regrown
	}

	//[VisualWeapons]
	//Update the visual weapons to make sure everything is place
	CG_VisualWeaponsUpdate(cent, ci);
	//[/VisualWeapons]
	
	if (cent->saberWasInFlight && g2HasWeapon)
	{
		 cent->saberWasInFlight = qfalse;
	}

	memset (&legs, 0, sizeof(legs));

	CG_SetGhoul2Info(&legs, cent);

	VectorCopy(cent->modelScale, legs.modelScale);
	legs.radius = CG_RadiusForCent( cent );
	VectorClear(legs.angles);

	if (ci->colorOverride[0] != 0.0f ||
		ci->colorOverride[1] != 0.0f ||
		ci->colorOverride[2] != 0.0f)
	{
		legs.shaderRGBA[0] = ci->colorOverride[0]*255.0f;
		legs.shaderRGBA[1] = ci->colorOverride[1]*255.0f;
		legs.shaderRGBA[2] = ci->colorOverride[2]*255.0f;
		legs.shaderRGBA[3] = cent->currentState.customRGBA[3];
	}
	else
	{
		legs.shaderRGBA[0] = cent->currentState.customRGBA[0];
		legs.shaderRGBA[1] = cent->currentState.customRGBA[1];
		legs.shaderRGBA[2] = cent->currentState.customRGBA[2];
		legs.shaderRGBA[3] = cent->currentState.customRGBA[3];
	}

// minimal_add:

	team = ci->team;

	if (cgs.gametype >= GT_TEAM && cg_drawFriend.integer &&
		cent->currentState.number != cg.snap->ps.clientNum &&
		cent->currentState.eType != ET_NPC)
	{	// If the view is either a spectator or on the same team as this character, show a symbol above their head.
		if ((cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.snap->ps.persistant[PERS_TEAM] == team) &&
			!(cent->currentState.eFlags & EF_DEAD))
		{
			if (cgs.gametype == GT_SIEGE)
			{ //check for per-map team shaders
				if (team == SIEGETEAM_TEAM1)
				{
					if (cgSiegeTeam1PlShader)
					{
						CG_PlayerFloatSprite( cent, cgSiegeTeam1PlShader);
					}
					else
					{ //if there isn't one fallback to default
						CG_PlayerFloatSprite( cent, cgs.media.teamRedShader);
					}
				}
				else
				{
					if (cgSiegeTeam2PlShader)
					{
						CG_PlayerFloatSprite( cent, cgSiegeTeam2PlShader);
					}
					else
					{ //if there isn't one fallback to default
						CG_PlayerFloatSprite( cent, cgs.media.teamBlueShader);
					}
				}
			}
			else
			{ //generic teamplay
				if (team == TEAM_RED)
				{
					CG_PlayerFloatSprite( cent, cgs.media.teamRedShader);
				}
				else	// if (team == TEAM_BLUE)
				{
					CG_PlayerFloatSprite( cent, cgs.media.teamBlueShader);
				}
			}
		}
	}
	else if (cgs.gametype == GT_POWERDUEL && cg_drawFriend.integer &&
		cent->currentState.number != cg.snap->ps.clientNum)
	{
		if (cg.predictedPlayerState.persistant[PERS_TEAM] != TEAM_SPECTATOR &&
			cent->currentState.number < MAX_CLIENTS &&
			!(cent->currentState.eFlags & EF_DEAD) &&
			ci &&
			cgs.clientinfo[cg.snap->ps.clientNum].duelTeam == ci->duelTeam)
		{ //ally in powerduel, so draw the icon
			CG_PlayerFloatSprite( cent, cgs.media.powerDuelAllyShader);
		}
		else if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_SPECTATOR &&
			cent->currentState.number < MAX_CLIENTS &&
			!(cent->currentState.eFlags & EF_DEAD) &&
			ci->duelTeam == DUELTEAM_DOUBLE)
		{
			CG_PlayerFloatSprite( cent, cgs.media.powerDuelAllyShader);
		}
	}

	if (cgs.gametype == GT_JEDIMASTER && cg_drawFriend.integer &&
		cent->currentState.number != cg.snap->ps.clientNum)			// Don't show a sprite above a player's own head in 3rd person.
	{	// If the view is either a spectator or on the same team as this character, show a symbol above their head.
		if ((cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR || cg.snap->ps.persistant[PERS_TEAM] == team) &&
			!(cent->currentState.eFlags & EF_DEAD))
		{
			if (CG_ThereIsAMaster())
			{
				if (!cg.snap->ps.isJediMaster)
				{
					if (!cent->currentState.isJediMaster)
					{
						CG_PlayerFloatSprite( cent, cgs.media.teamRedShader);
					}
				}
			}
		}
	}

	// add the shadow
	shadow = CG_PlayerShadow( cent, &shadowPlane );

	if ( ((cent->currentState.eFlags & EF_SEEKERDRONE) || cent->currentState.genericenemyindex != -1) && cent->currentState.eType != ET_NPC )
	{
		refEntity_t		seeker;

		memset( &seeker, 0, sizeof(seeker) );

		VectorCopy(cent->lerpOrigin, elevated);
		elevated[2] += 40;

		VectorCopy( elevated, seeker.lightingOrigin );
		seeker.shadowPlane = shadowPlane;
		seeker.renderfx = 0; //renderfx;
							 //don't show in first person?

		angle = ((cg.time / 12) & 255) * (M_PI * 2) / 255;
		dir[0] = cos(angle) * 20;
		dir[1] = sin(angle) * 20;
		dir[2] = cos(angle) * 5;
		VectorAdd(elevated, dir, seeker.origin);

		VectorCopy(seeker.origin, seekorg);

		if (cent->currentState.genericenemyindex > MAX_GENTITIES)
		{
			float prefig = (cent->currentState.genericenemyindex-cg.time)/80;

			if (prefig > 55)
			{
				prefig = 55;
			}
			else if (prefig < 1)
			{
				prefig = 1;
			}

			elevated[2] -= 55-prefig;

			angle = ((cg.time / 12) & 255) * (M_PI * 2) / 255;
			dir[0] = cos(angle) * 20;
			dir[1] = sin(angle) * 20;
			dir[2] = cos(angle) * 5;
			VectorAdd(elevated, dir, seeker.origin);
		}
		else if (cent->currentState.genericenemyindex != ENTITYNUM_NONE && cent->currentState.genericenemyindex != -1)
		{
			centity_t *enent = &cg_entities[cent->currentState.genericenemyindex];

			if (enent)
			{
				VectorSubtract(enent->lerpOrigin, seekorg, enang);
				VectorNormalize(enang);
				vectoangles(enang, angles);
				successchange = 1;
			}
		}

		if (!successchange)
		{
			angles[0] = sin(angle) * 30;
			angles[1] = (angle * 180 / M_PI) + 90;
			if (angles[1] > 360)
				angles[1] -= 360;
			angles[2] = 0;
		}

		AnglesToAxis( angles, seeker.axis );

		seeker.hModel = trap_R_RegisterModel("models/items/remote.md3");
		trap_R_AddRefEntityToScene( &seeker );
	}

	// add a water splash if partially in and out of water
	CG_PlayerSplash( cent );

	if ( (cg_shadows.integer == 3 || cg_shadows.integer == 2) && shadow ) {
		renderfx |= RF_SHADOW_PLANE;
	}
	renderfx |= RF_LIGHTING_ORIGIN;			// use the same origin for all

	// if we've been hit, display proper fullscreen fx
	CG_PlayerHitFX(cent);

	VectorCopy( cent->lerpOrigin, legs.origin );

	VectorCopy( cent->lerpOrigin, legs.lightingOrigin );
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	if (cg_shadows.integer == 2 && (renderfx & RF_THIRD_PERSON))
	{ //can see own shadow
		legs.renderfx |= RF_SHADOW_ONLY;
	}
	VectorCopy (legs.origin, legs.oldorigin);	// don't positionally lerp at all

	CG_G2PlayerAngles( cent, legs.axis, rootAngles );
	CG_G2PlayerHeadAnims( cent );

	if ( (cent->currentState.eFlags2&EF2_HELD_BY_MONSTER) 
		&& cent->currentState.hasLookTarget )//NOTE: lookTarget is an entity number, so this presumes that client 0 is NOT a Rancor...
	{
		centity_t	*rancor = &cg_entities[cent->currentState.lookTarget];
		if ( rancor )
		{
			BG_AttachToRancor( rancor->ghoul2, //ghoul2 info
				rancor->lerpAngles[YAW],
				rancor->lerpOrigin,
				cg.time,
				cgs.gameModels,
				rancor->modelScale,
				(rancor->currentState.eFlags2&EF2_GENERIC_NPC_FLAG),
				legs.origin,
				legs.angles,
				NULL );

			if ( cent->isRagging )
			{//hack, ragdoll has you way at bottom of bounding box
				VectorMA( legs.origin, 32, legs.axis[2], legs.origin );
			}
			VectorCopy( legs.origin, legs.oldorigin );
			VectorCopy( legs.origin, legs.lightingOrigin );

			VectorCopy( legs.angles, cent->lerpAngles );
			VectorCopy( cent->lerpAngles, rootAngles );//??? tempAngles );//tempAngles is needed a lot below
			VectorCopy( cent->lerpAngles, cent->turAngles );
			VectorCopy( legs.origin, cent->lerpOrigin );
		}
	}
	//This call is mainly just to reconstruct the skeleton. But we'll get the left hand matrix while we're at it.
	//If we don't reconstruct the skeleton after setting the bone angles, we will get bad bolt points on the model
	//(e.g. the weapon model bolt will look "lagged") if there's no other GetBoltMatrix call for the rest of the
	//frame. Yes, this is stupid and needs to be fixed properly.
	//The current solution is to force it not to reconstruct the skeleton for the first GBM call in G2PlayerAngles.
	//It works and we end up only reconstructing it once, but it doesn't seem like the best solution.
	trap_G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &lHandMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
	gotLHandMatrix = qtrue;

	//[TrueView]
	//Restrict True View Model changes to the player and do the True View camera view work.
	if (cg.snap && cent->currentState.number == cg.snap->ps.clientNum)
	{
		if ( !cg.renderingThirdPerson && (cg_trueguns.integer || cent->currentState.weapon == WP_SABER 
			|| cent->currentState.weapon == WP_MELEE) && !cg.predictedPlayerState.zoomMode)	
		{
			//<True View varibles
			mdxaBone_t 		eyeMatrix;
			vec3_t			eyeAngles;	
			vec3_t			EyeAxis[3];
			vec3_t			OldeyeOrigin;
			qhandle_t		eyesBolt;
			qboolean		boneBased = qfalse;
				
			//make the player's be based on the ghoul2 model
				
			//grab the location data for the "*head_eyes" tag surface
			eyesBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*head_eyes");
			if( !trap_G2API_GetBoltMatrix(cent->ghoul2, 0, eyesBolt, &eyeMatrix, cent->turAngles, cent->lerpOrigin, 
				cg.time, cgs.gameModels, cent->modelScale) )
			{//Something prevented you from getting the "*head_eyes" information.  The model probably doesn't have a 
				//*head_eyes tag surface.  Try using *head_front instead

				eyesBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "*head_front");
				if( !trap_G2API_GetBoltMatrix(cent->ghoul2, 0, eyesBolt, &eyeMatrix, cent->turAngles, cent->lerpOrigin, 
					cg.time, cgs.gameModels, cent->modelScale) )
				{
					eyesBolt = trap_G2API_AddBolt(cent->ghoul2, 0, "reye");
					boneBased = qtrue;
					if( !trap_G2API_GetBoltMatrix(cent->ghoul2, 0, eyesBolt, &eyeMatrix, cent->turAngles, cent->lerpOrigin, 
					cg.time, cgs.gameModels, cent->modelScale) )
					{
						if( !trueviewwarning )
						{//first failure.  Do a single warning then turn the warnings off.
							CG_Printf("WARNING:  This Model seems to have missing the *head_eyes and *head_front tag surfaces.  True View Disabled.\n");
							trueviewwarning = qtrue;
						}

						goto SkipTrueView;
					}
				}
			}
					
			//Set the original eye Origin
			VectorCopy( cg.refdef.vieworg, OldeyeOrigin);

			//set the player's view origin
			BG_GiveMeVectorFromMatrix(&eyeMatrix, ORIGIN, cg.refdef.vieworg);

			//Find the orientation of the eye tag surface
			//I based this on coordsys.h that I found at http://www.xs4all.nl/~hkuiper/cwmtx/html/coordsys_8h-source.html
			//According to the file, Harry Kuiper, Will DeVore deserve credit for making that file that I based this on.

			if(boneBased)
			{//the eye bone has different default axis orientation than the tag surfaces.
				EyeAxis[0][0] = eyeMatrix.matrix[0][1];
				EyeAxis[1][0] = eyeMatrix.matrix[1][1];
				EyeAxis[2][0] = eyeMatrix.matrix[2][1];

				EyeAxis[0][1] = eyeMatrix.matrix[0][0];
				EyeAxis[1][1] = eyeMatrix.matrix[1][0];
				EyeAxis[2][1] = eyeMatrix.matrix[2][0];

				EyeAxis[0][2] = -eyeMatrix.matrix[0][2];
				EyeAxis[1][2] = -eyeMatrix.matrix[1][2];
				EyeAxis[2][2] = -eyeMatrix.matrix[2][2];
			}
			else
			{
				EyeAxis[0][0] = eyeMatrix.matrix[0][0];
				EyeAxis[1][0] = eyeMatrix.matrix[1][0];
				EyeAxis[2][0] = eyeMatrix.matrix[2][0];

				EyeAxis[0][1] = eyeMatrix.matrix[0][1];
				EyeAxis[1][1] = eyeMatrix.matrix[1][1];
				EyeAxis[2][1] = eyeMatrix.matrix[2][1];

				EyeAxis[0][2] = eyeMatrix.matrix[0][2];
				EyeAxis[1][2] = eyeMatrix.matrix[1][2];
				EyeAxis[2][2] = eyeMatrix.matrix[2][2];
			}

			eyeAngles[YAW] = ( atan2(EyeAxis[1][0], EyeAxis[0][0]) * 180 / M_PI );

			//I want asin but it's not setup in the libraries so I'm useing the statement asin x = (M_PI / 2) - acos x
			eyeAngles[PITCH] = ( ( (M_PI / 2) - acos (-EyeAxis[2][0]) ) * 180 / M_PI );
			eyeAngles[ROLL] = ( atan2(EyeAxis[2][1], EyeAxis[2][2]) * 180 / M_PI );

			//END Find the orientation of the eye tag surface

			//Shift the camera origin by cg_trueeyeposition
			AngleVectors( eyeAngles, EyeAxis[0], NULL, NULL );
			VectorMA( cg.refdef.vieworg, cg_trueeyeposition.value, EyeAxis[0], cg.refdef.vieworg );

			//Trace to see if the bolt eye origin is ok to move to.  If it's not, place it at the last safe position.
			CheckCameraLocation( OldeyeOrigin );
			
			//Do all the Eye "movement" and simplified moves here.
			SmoothTrueView(eyeAngles);

			//set the player view angles
			VectorCopy(eyeAngles, cg.refdef.viewangles); 

			//set the player view axis
			AnglesToAxis( cg.refdef.viewangles, cg.refdef.viewaxis );

		}
	}

SkipTrueView:

	/*
#if 0
	if (cg.renderingThirdPerson)
	{
		if (cgFPLSState != 0)
		{
			CG_ForceFPLSPlayerModel(cent, ci);
			cgFPLSState = 0;
			return;
		}
	}
	else if (ci->team == TEAM_SPECTATOR || (cg.snap && (cg.snap->ps.pm_flags & PMF_FOLLOW)))
	{ //don't allow this when spectating
		if (cgFPLSState != 0)
		{
			trap_Cvar_Set("cg_fpls", "0");
			cg_fpls.integer = 0;

			CG_ForceFPLSPlayerModel(cent, ci);
			cgFPLSState = 0;
			return;
		}

		if (cg_fpls.integer)
		{
			trap_Cvar_Set("cg_fpls", "0");
		}
	}
	else
	{
		if (cg_fpls.integer && cent->currentState.weapon == WP_SABER && cg.snap && cent->currentState.number == cg.snap->ps.clientNum)
		{

			if (cgFPLSState != cg_fpls.integer)
			{
				CG_ForceFPLSPlayerModel(cent, ci);
				cgFPLSState = cg_fpls.integer;
				return;
			}

			/*
			mdxaBone_t 		headMatrix;
			trap_G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_head, &headMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
			BG_GiveMeVectorFromMatrix(&headMatrix, ORIGIN, cg.refdef.vieworg);
			*/
			/*
		}
		else if (!cg_fpls.integer && cgFPLSState)
		{
			if (cgFPLSState != cg_fpls.integer)
			{
				CG_ForceFPLSPlayerModel(cent, ci);
				cgFPLSState = cg_fpls.integer;
				return;
			}
		}
	}
#endif
	*/
	//[/TrueView]

	if (cent->currentState.eFlags & EF_DEAD)
	{
		dead = qtrue;
		//rww - since our angles are fixed when we're dead this shouldn't be an issue anyway
		//we need to render the dying/dead player because we are now spawning the body on respawn instead of death
		//return;
	}

	ScaleModelAxis(&legs);

	memset( &torso, 0, sizeof(torso) );

	//rww - force speed "trail" effect
	if (!(cent->currentState.powerups & (1 << PW_SPEED)) || doAlpha || !cg_speedTrail.integer)
	{
		cent->frame_minus1_refreshed = 0;
		cent->frame_minus2_refreshed = 0;
	}

	if (cent->frame_minus1_refreshed ||
		cent->frame_minus2_refreshed)
	{
		vec3_t			tDir;
		int				distVelBase;

		VectorCopy(cent->currentState.pos.trDelta, tDir);
		distVelBase = SPEED_TRAIL_DISTANCE*(VectorNormalize(tDir)*0.004);

		if (cent->frame_minus1_refreshed)
		{
			refEntity_t reframe_minus1 = legs;
			reframe_minus1.renderfx |= RF_FORCE_ENT_ALPHA;
			reframe_minus1.shaderRGBA[0] = legs.shaderRGBA[0];
			reframe_minus1.shaderRGBA[1] = legs.shaderRGBA[1];
			reframe_minus1.shaderRGBA[2] = legs.shaderRGBA[2];
			reframe_minus1.shaderRGBA[3] = 100;

			//rww - if the client gets a bad framerate we will only receive frame positions
			//once per frame anyway, so we might end up with speed trails very spread out.
			//in order to avoid that, we'll get the direction of the last trail from the player
			//and place the trail refent a set distance from the player location this frame
			VectorSubtract(cent->frame_minus1, legs.origin, tDir);
			VectorNormalize(tDir);

			cent->frame_minus1[0] = legs.origin[0]+tDir[0]*distVelBase;
			cent->frame_minus1[1] = legs.origin[1]+tDir[1]*distVelBase;
			cent->frame_minus1[2] = legs.origin[2]+tDir[2]*distVelBase;

			VectorCopy(cent->frame_minus1, reframe_minus1.origin);

			//reframe_minus1.customShader = 2;

			trap_R_AddRefEntityToScene(&reframe_minus1);
		}

		if (cent->frame_minus2_refreshed)
		{
			refEntity_t reframe_minus2 = legs;

			reframe_minus2.renderfx |= RF_FORCE_ENT_ALPHA;
			reframe_minus2.shaderRGBA[0] = legs.shaderRGBA[0];
			reframe_minus2.shaderRGBA[1] = legs.shaderRGBA[1];
			reframe_minus2.shaderRGBA[2] = legs.shaderRGBA[2];
			reframe_minus2.shaderRGBA[3] = 50;

			//Same as above but do it between trail points instead of the player and first trail entry
			VectorSubtract(cent->frame_minus2, cent->frame_minus1, tDir);
			VectorNormalize(tDir);

			cent->frame_minus2[0] = cent->frame_minus1[0]+tDir[0]*distVelBase;
			cent->frame_minus2[1] = cent->frame_minus1[1]+tDir[1]*distVelBase;
			cent->frame_minus2[2] = cent->frame_minus1[2]+tDir[2]*distVelBase;

			VectorCopy(cent->frame_minus2, reframe_minus2.origin);

			//reframe_minus2.customShader = 2;

			trap_R_AddRefEntityToScene(&reframe_minus2);
		}
	}

	//trigger animation-based sounds, done before next lerp frame.
	CG_TriggerAnimSounds(cent);

	// get the animation state (after rotation, to allow feet shuffle)
	CG_PlayerAnimation( cent, &legs.oldframe, &legs.frame, &legs.backlerp,
		 &torso.oldframe, &torso.frame, &torso.backlerp );

	// add the talk baloon or disconnect icon
	CG_PlayerSprites( cent );

	if (cent->currentState.eFlags & EF_DEAD)
	{ //keep track of death anim frame for when we copy off the bodyqueue
		ci->frame = cent->pe.torso.frame;
	}

	if (cent->currentState.activeForcePass > FORCE_LEVEL_3
		&& cent->currentState.NPC_class != CLASS_VEHICLE)
	{
		vec3_t axis[3];
		vec3_t tAng, fAng, fxDir;
		vec3_t efOrg;

		int realForceLev = (cent->currentState.activeForcePass - FORCE_LEVEL_3);

		VectorSet( tAng, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL] );

		VectorSet( fAng, cent->pe.torso.pitchAngle, cent->pe.torso.yawAngle, 0 );

		AngleVectors( fAng, fxDir, NULL, NULL );

		if ( cent->currentState.torsoAnim == BOTH_FORCE_2HANDEDLIGHTNING_HOLD 
			&& Q_irand( 0, 1 ) )
		{//alternate back and forth between left and right
			mdxaBone_t 	rHandMatrix;
			trap_G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_rhand, &rHandMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
			efOrg[0] = rHandMatrix.matrix[0][3];
			efOrg[1] = rHandMatrix.matrix[1][3];
			efOrg[2] = rHandMatrix.matrix[2][3];
		}
		else
		{
			//trap_G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
			if (!gotLHandMatrix)
			{
				trap_G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &lHandMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
				gotLHandMatrix = qtrue;
			}
			efOrg[0] = lHandMatrix.matrix[0][3];
			efOrg[1] = lHandMatrix.matrix[1][3];
			efOrg[2] = lHandMatrix.matrix[2][3];
		}

		AnglesToAxis( fAng, axis );
	
		if ( realForceLev > FORCE_LEVEL_2 )
		{//arc
			//trap_FX_PlayEffectID( cgs.effects.forceLightningWide, efOrg, fxDir );
			//trap_FX_PlayEntityEffectID(cgs.effects.forceDrainWide, efOrg, axis, cent->boltInfo, cent->currentState.number, -1, -1);
			trap_FX_PlayEntityEffectID(cgs.effects.forceDrainWide, efOrg, axis, -1, -1, -1, -1);
		}
		else
		{//line
			//trap_FX_PlayEffectID( cgs.effects.forceLightning, efOrg, fxDir );
			//trap_FX_PlayEntityEffectID(cgs.effects.forceDrain, efOrg, axis, cent->boltInfo, cent->currentState.number, -1, -1);
			trap_FX_PlayEntityEffectID(cgs.effects.forceDrain, efOrg, axis, -1, -1, -1, -1);
		}

		/*
		if (cent->bolt4 < cg.time)
		{
			cent->bolt4 = cg.time + 100;
			trap_S_StartSound(NULL, cent->currentState.number, CHAN_AUTO, trap_S_RegisterSound("sound/weapons/force/drain.wav") );
		}
		*/
	}
	else if ( cent->currentState.activeForcePass 
		&& cent->currentState.NPC_class != CLASS_VEHICLE)
	{//doing the electrocuting
		vec3_t axis[3];
		vec3_t tAng, fAng, fxDir;
		vec3_t efOrg;

		VectorSet( tAng, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL] );

		VectorSet( fAng, cent->pe.torso.pitchAngle, cent->pe.torso.yawAngle, 0 );

		AngleVectors( fAng, fxDir, NULL, NULL );

		//trap_G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
		if (!gotLHandMatrix)
		{
			trap_G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &lHandMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
			gotLHandMatrix = qtrue;
		}

		efOrg[0] = lHandMatrix.matrix[0][3];
		efOrg[1] = lHandMatrix.matrix[1][3];
		efOrg[2] = lHandMatrix.matrix[2][3];

		AnglesToAxis( fAng, axis );
	
		if ( cent->currentState.activeForcePass > FORCE_LEVEL_2 )
		{//arc
			//trap_FX_PlayEffectID( cgs.effects.forceLightningWide, efOrg, fxDir );
			//trap_FX_PlayEntityEffectID(cgs.effects.forceLightningWide, efOrg, axis, cent->boltInfo, cent->currentState.number, -1, -1);
			trap_FX_PlayEntityEffectID(cgs.effects.forceLightningWide, efOrg, axis, -1, -1, -1, -1);
		}
		else
		{//line
			//trap_FX_PlayEffectID( cgs.effects.forceLightning, efOrg, fxDir );
			//trap_FX_PlayEntityEffectID(cgs.effects.forceLightning, efOrg, axis, cent->boltInfo, cent->currentState.number, -1, -1);
			trap_FX_PlayEntityEffectID(cgs.effects.forceLightning, efOrg, axis, -1, -1, -1, -1);
		}

		/*
		if (cent->bolt4 < cg.time)
		{
			cent->bolt4 = cg.time + 100;
			trap_S_StartSound(NULL, cent->currentState.number, CHAN_AUTO, trap_S_RegisterSound("sound/weapons/force/lightning.wav") );
		}
		*/
	}

	//fullbody push effect
	if (cent->currentState.eFlags & EF_BODYPUSH)
	{
		CG_ForcePushBodyBlur(cent);
	}

	if ( cent->currentState.powerups & (1 << PW_DISINT_4) )
	{
		vec3_t tAng;
		vec3_t efOrg;

		//VectorSet( tAng, 0, cent->pe.torso.yawAngle, 0 );
		VectorSet( tAng, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL] );

		//trap_G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
		if (!gotLHandMatrix)
		{
			trap_G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_lhand, &lHandMatrix, cent->turAngles, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);
			gotLHandMatrix = qtrue;
		}

		efOrg[0] = lHandMatrix.matrix[0][3];
		efOrg[1] = lHandMatrix.matrix[1][3];
		efOrg[2] = lHandMatrix.matrix[2][3];

		//[True View]
		//Do the Grip visual when you're using true view.
		if ( (cent->currentState.forcePowersActive & (1 << FP_GRIP)) &&
			(cg.renderingThirdPerson || cent->currentState.number != cg.snap->ps.clientNum 
			|| cg_trueguns.integer  || cg.predictedPlayerState.weapon == WP_SABER
			|| cg.predictedPlayerState.weapon == WP_MELEE) )
		/*
		if ( (cent->currentState.forcePowersActive & (1 << FP_GRIP)) &&
			(cg.renderingThirdPerson || cent->currentState.number != cg.snap->ps.clientNum) )
		*/
		{
			vec3_t boltDir;
			vec3_t origBolt;
			VectorCopy(efOrg, origBolt);
			BG_GiveMeVectorFromMatrix( &lHandMatrix, NEGATIVE_Y, boltDir );

			CG_ForceGripEffect( efOrg );
			CG_ForceGripEffect( efOrg );

			/*
			//Render a scaled version of the model's hand with a n337 looking shader
			{
				const char *rotateBone;
				char *limbName;
				char *limbCapName;
				vec3_t armAng;
				refEntity_t regrip_arm;
				float wv = sin( cg.time * 0.003f ) * 0.08f + 0.1f;

				//rotateBone = "lradius";
				rotateBone = "lradiusX";
				limbName = "l_arm";
				limbCapName = "l_arm_cap_torso";

				if (cent->grip_arm && trap_G2_HaveWeGhoul2Models(cent->grip_arm))
				{
					trap_G2API_CleanGhoul2Models(&(cent->grip_arm));
				}

				memset( &regrip_arm, 0, sizeof(regrip_arm) );

				VectorCopy(origBolt, efOrg);


				//efOrg[2] += 8;
				efOrg[2] -= 4;

				VectorCopy(efOrg, regrip_arm.origin);
				VectorCopy(regrip_arm.origin, regrip_arm.lightingOrigin);

				//VectorCopy(cent->lerpAngles, armAng);
				VectorAdd(vec3_origin, rootAngles, armAng);
				//armAng[ROLL] = -90;
				armAng[ROLL] = 0;
				armAng[PITCH] = 0;
				AnglesToAxis(armAng, regrip_arm.axis);
				
				trap_G2API_DuplicateGhoul2Instance(cent->ghoul2, &cent->grip_arm);

				//remove all other models
				if (trap_G2API_HasGhoul2ModelOnIndex(&(cent->grip_arm), 1))
				{ //weapon right
					trap_G2API_RemoveGhoul2Model(&(cent->grip_arm), 1);
				}
				if (trap_G2API_HasGhoul2ModelOnIndex(&(cent->grip_arm), 2))
				{ //weapon left
					trap_G2API_RemoveGhoul2Model(&(cent->grip_arm), 2);
				}
				if (trap_G2API_HasGhoul2ModelOnIndex(&(cent->grip_arm), 3))
				{ //jetpack
					trap_G2API_RemoveGhoul2Model(&(cent->grip_arm), 3);
				}

				trap_G2API_SetRootSurface(cent->grip_arm, 0, limbName);
				trap_G2API_SetNewOrigin(cent->grip_arm, trap_G2API_AddBolt(cent->grip_arm, 0, rotateBone));
				trap_G2API_SetSurfaceOnOff(cent->grip_arm, limbCapName, 0);

				regrip_arm.modelScale[0] = 1;//+(wv*6);
				regrip_arm.modelScale[1] = 1;//+(wv*6);
				regrip_arm.modelScale[2] = 1;//+(wv*6);
				ScaleModelAxis(&regrip_arm);

				regrip_arm.radius = 64;

				regrip_arm.customShader = trap_R_RegisterShader( "gfx/misc/red_portashield" );
				
				regrip_arm.renderfx |= RF_RGB_TINT;
				regrip_arm.shaderRGBA[0] = 255 - (wv*900);
				if (regrip_arm.shaderRGBA[0] < 30)
				{
					regrip_arm.shaderRGBA[0] = 30;
				}
				if (regrip_arm.shaderRGBA[0] > 255)
				{
					regrip_arm.shaderRGBA[0] = 255;
				}
				regrip_arm.shaderRGBA[1] = regrip_arm.shaderRGBA[2] = regrip_arm.shaderRGBA[0];
				
				regrip_arm.ghoul2 = cent->grip_arm;
				trap_R_AddRefEntityToScene( &regrip_arm );
			}
			*/
		}
		else if (!(cent->currentState.forcePowersActive & (1 << FP_GRIP)))
		{
			//use refractive effect
			CG_ForcePushBlur( efOrg, cent );
		}
	}
	else if (cent->bodyFadeTime)
	{ //reset the counter for keeping track of push refraction effect state
		cent->bodyFadeTime = 0;
	}

	if (cent->currentState.weapon == WP_STUN_BATON && cent->currentState.number == cg.snap->ps.clientNum)
	{
		trap_S_AddLoopingSound( cent->currentState.number, cg.refdef.vieworg, vec3_origin, 
			trap_S_RegisterSound( "sound/weapons/baton/idle.wav" ) );
	}

	//NOTE: All effects that should be visible during mindtrick should go above here

	if (iwantout)
	{
		goto stillDoSaber;
		//return;
	}
	else if (doAlpha)
	{
		legs.renderfx |= RF_FORCE_ENT_ALPHA;
		legs.shaderRGBA[3] = cent->trickAlpha;

		if (legs.shaderRGBA[3] < 1)
		{ //don't cancel it out even if it's < 1
			legs.shaderRGBA[3] = 1;
		}
	}

	if (cent->teamPowerEffectTime > cg.time)
	{
		if (cent->teamPowerType == 3)
		{ //absorb is a somewhat different effect entirely
			//Guess I'll take care of it where it's always been, just checking these values instead.
		}
		else
		{
			vec4_t preCol;
			int preRFX;

			preRFX = legs.renderfx;

			legs.renderfx |= RF_RGB_TINT;
			legs.renderfx |= RF_FORCE_ENT_ALPHA;

			preCol[0] = legs.shaderRGBA[0];
			preCol[1] = legs.shaderRGBA[1];
			preCol[2] = legs.shaderRGBA[2];
			preCol[3] = legs.shaderRGBA[3];

			if (cent->teamPowerType == 1)
			{ //heal
				legs.shaderRGBA[0] = 0;
				legs.shaderRGBA[1] = 255;
				legs.shaderRGBA[2] = 0;
			}
			else if (cent->teamPowerType == 0)
			{ //regen
				legs.shaderRGBA[0] = 0;
				legs.shaderRGBA[1] = 0;
				legs.shaderRGBA[2] = 255;
			}
			else
			{ //drain
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = 0;
				legs.shaderRGBA[2] = 0;
			}

			legs.shaderRGBA[3] = ((cent->teamPowerEffectTime - cg.time)/8);

			legs.customShader = trap_R_RegisterShader( "powerups/ysalimarishell" );
			trap_R_AddRefEntityToScene(&legs);

			legs.customShader = 0;
			legs.renderfx = preRFX;
			legs.shaderRGBA[0] = preCol[0];
			legs.shaderRGBA[1] = preCol[1];
			legs.shaderRGBA[2] = preCol[2];
			legs.shaderRGBA[3] = preCol[3];
		}
	}

	//If you've tricked this client.
	if (CG_IsMindTricked(cg.snap->ps.fd.forceMindtrickTargetIndex,
		cg.snap->ps.fd.forceMindtrickTargetIndex2,
		cg.snap->ps.fd.forceMindtrickTargetIndex3,
		cg.snap->ps.fd.forceMindtrickTargetIndex4,
		cent->currentState.number))
	{
		if (cent->ghoul2)
		{
			vec3_t efOrg;
			vec3_t tAng, fxAng;
			vec3_t axis[3];

			//VectorSet( tAng, 0, cent->pe.torso.yawAngle, 0 );
			VectorSet( tAng, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL] );

			trap_G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_head, &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);

			BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, efOrg);
			BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, fxAng);

 			axis[0][0] = boltMatrix.matrix[0][0];
 			axis[0][1] = boltMatrix.matrix[1][0];
		 	axis[0][2] = boltMatrix.matrix[2][0];

 			axis[1][0] = boltMatrix.matrix[0][1];
 			axis[1][1] = boltMatrix.matrix[1][1];
		 	axis[1][2] = boltMatrix.matrix[2][1];

 			axis[2][0] = boltMatrix.matrix[0][2];
 			axis[2][1] = boltMatrix.matrix[1][2];
		 	axis[2][2] = boltMatrix.matrix[2][2];

			//trap_FX_PlayEntityEffectID(trap_FX_RegisterEffect("force/confusion.efx"), efOrg, axis, cent->boltInfo, cent->currentState.number);
			trap_FX_PlayEntityEffectID(cgs.effects.mForceConfustionOld, efOrg, axis, -1, -1, -1, -1);
		}
	}

	//[TrueView]
		if (cgs.gametype == GT_HOLOCRON && cent->currentState.time2 && 
		((cg.renderingThirdPerson || cg_trueguns.integer || cg.predictedPlayerState.weapon == WP_SABER || cg.predictedPlayerState.weapon == WP_MELEE) 
		|| cg.snap->ps.clientNum != cent->currentState.number))
	//if (cgs.gametype == GT_HOLOCRON && cent->currentState.time2 && (cg.renderingThirdPerson || cg.snap->ps.clientNum != cent->currentState.number))
	//[/TrueView]
	{
		int i = 0;
		int renderedHolos = 0;
		refEntity_t		holoRef;

		while (i < NUM_FORCE_POWERS && renderedHolos < 3)
		{
			if (cent->currentState.time2 & (1 << i))
			{
				memset( &holoRef, 0, sizeof(holoRef) );

				VectorCopy(cent->lerpOrigin, elevated);
				elevated[2] += 8;

				VectorCopy( elevated, holoRef.lightingOrigin );
				holoRef.shadowPlane = shadowPlane;
				holoRef.renderfx = 0;//RF_THIRD_PERSON;

				if (renderedHolos == 0)
				{
					angle = ((cg.time / 8) & 255) * (M_PI * 2) / 255;
					dir[0] = cos(angle) * 20;
					dir[1] = sin(angle) * 20;
					dir[2] = cos(angle) * 20;
					VectorAdd(elevated, dir, holoRef.origin);

					angles[0] = sin(angle) * 30;
					angles[1] = (angle * 180 / M_PI) + 90;
					if (angles[1] > 360)
						angles[1] -= 360;
					angles[2] = 0;
					AnglesToAxis( angles, holoRef.axis );
				}
				else if (renderedHolos == 1)
				{
					angle = ((cg.time / 8) & 255) * (M_PI * 2) / 255 + M_PI;
					if (angle > M_PI * 2)
						angle -= (float)M_PI * 2;
					dir[0] = sin(angle) * 20;
					dir[1] = cos(angle) * 20;
					dir[2] = cos(angle) * 20;
					VectorAdd(elevated, dir, holoRef.origin);

					angles[0] = cos(angle - 0.5 * M_PI) * 30;
					angles[1] = 360 - (angle * 180 / M_PI);
					if (angles[1] > 360)
						angles[1] -= 360;
					angles[2] = 0;
					AnglesToAxis( angles, holoRef.axis );
				}
				else
				{
					angle = ((cg.time / 6) & 255) * (M_PI * 2) / 255 + 0.5 * M_PI;
					if (angle > M_PI * 2)
						angle -= (float)M_PI * 2;
					dir[0] = sin(angle) * 20;
					dir[1] = cos(angle) * 20;
					dir[2] = 0;
					VectorAdd(elevated, dir, holoRef.origin);
			
					VectorCopy(dir, holoRef.axis[1]);
					VectorNormalize(holoRef.axis[1]);
					VectorSet(holoRef.axis[2], 0, 0, 1);
					CrossProduct(holoRef.axis[1], holoRef.axis[2], holoRef.axis[0]);
				}

				holoRef.modelScale[0] = 0.5;
				holoRef.modelScale[1] = 0.5;
				holoRef.modelScale[2] = 0.5;
				ScaleModelAxis(&holoRef);

				{
					float wv;
					addspriteArgStruct_t fxSArgs;
					vec3_t holoCenter;

					holoCenter[0] = holoRef.origin[0] + holoRef.axis[2][0]*18;
					holoCenter[1] = holoRef.origin[1] + holoRef.axis[2][1]*18;
					holoCenter[2] = holoRef.origin[2] + holoRef.axis[2][2]*18;

					wv = sin( cg.time * 0.004f ) * 0.08f + 0.1f;

					VectorCopy(holoCenter, fxSArgs.origin);
					VectorClear(fxSArgs.vel);
					VectorClear(fxSArgs.accel);
					fxSArgs.scale = wv*60;
					fxSArgs.dscale = wv*60;
					fxSArgs.sAlpha = wv*12;
					fxSArgs.eAlpha = wv*12;
					fxSArgs.rotation = 0.0f;
					fxSArgs.bounce = 0.0f;
					fxSArgs.life = 1.0f;

					fxSArgs.flags = 0x08000000|0x00000001;

					if (forcePowerDarkLight[i] == FORCE_DARKSIDE)
					{ //dark
						fxSArgs.sAlpha *= 3;
						fxSArgs.eAlpha *= 3;
						fxSArgs.shader = cgs.media.redSaberGlowShader;
						trap_FX_AddSprite(&fxSArgs);
					}
					else if (forcePowerDarkLight[i] == FORCE_LIGHTSIDE)
					{ //light
						fxSArgs.sAlpha *= 1.5;
						fxSArgs.eAlpha *= 1.5;
						fxSArgs.shader = cgs.media.redSaberGlowShader;
						trap_FX_AddSprite(&fxSArgs);
						fxSArgs.shader = cgs.media.greenSaberGlowShader;
						trap_FX_AddSprite(&fxSArgs);
						fxSArgs.shader = cgs.media.blueSaberGlowShader;
						trap_FX_AddSprite(&fxSArgs);
					}
					else
					{ //neutral
						if (i == FP_SABER_OFFENSE ||
							i == FP_SABER_DEFENSE ||
							i == FP_SABERTHROW)
						{ //saber power
							fxSArgs.sAlpha *= 1.5;
							fxSArgs.eAlpha *= 1.5;
							fxSArgs.shader = cgs.media.greenSaberGlowShader;
							trap_FX_AddSprite(&fxSArgs);
						}
						else
						{
							fxSArgs.sAlpha *= 0.5;
							fxSArgs.eAlpha *= 0.5;
							fxSArgs.shader = cgs.media.greenSaberGlowShader;
							trap_FX_AddSprite(&fxSArgs);
							fxSArgs.shader = cgs.media.blueSaberGlowShader;
							trap_FX_AddSprite(&fxSArgs);
						}
					}
				}

				holoRef.hModel = trap_R_RegisterModel(forceHolocronModels[i]);
				trap_R_AddRefEntityToScene( &holoRef );

				renderedHolos++;
			}
			i++;
		}
	}

	if ((cent->currentState.powerups & (1 << PW_YSALAMIRI)) ||
		(cgs.gametype == GT_CTY && ((cent->currentState.powerups & (1 << PW_REDFLAG)) || (cent->currentState.powerups & (1 << PW_BLUEFLAG)))) )
	{
		if (cgs.gametype == GT_CTY && (cent->currentState.powerups & (1 << PW_REDFLAG)))
		{
			CG_DrawPlayerSphere(cent, cent->lerpOrigin, 1.4f, cgs.media.ysaliredShader );
		}
		else if (cgs.gametype == GT_CTY && (cent->currentState.powerups & (1 << PW_BLUEFLAG)))
		{
			CG_DrawPlayerSphere(cent, cent->lerpOrigin, 1.4f, cgs.media.ysaliblueShader );
		}
		else
		{
			CG_DrawPlayerSphere(cent, cent->lerpOrigin, 1.4f, cgs.media.ysalimariShader );
		}
	}
	
	if (cent->currentState.powerups & (1 << PW_FORCE_BOON))
	{
		CG_DrawPlayerSphere(cent, cent->lerpOrigin, 2.0f, cgs.media.boonShader );
	}

	if (cent->currentState.powerups & (1 << PW_FORCE_ENLIGHTENED_DARK))
	{
		CG_DrawPlayerSphere(cent, cent->lerpOrigin, 2.0f, cgs.media.endarkenmentShader );
	}
	else if (cent->currentState.powerups & (1 << PW_FORCE_ENLIGHTENED_LIGHT))
	{
		CG_DrawPlayerSphere(cent, cent->lerpOrigin, 2.0f, cgs.media.enlightenmentShader );
	}

	if (cent->currentState.eFlags & EF_INVULNERABLE)
	{
		CG_DrawPlayerSphere(cent, cent->lerpOrigin, 1.0f, cgs.media.invulnerabilityShader );
	}
stillDoSaber:
	if ((cent->currentState.eFlags & EF_DEAD) && cent->currentState.weapon == WP_SABER)
	{//racc - turn off saber if player died.
		//cent->saberLength = 0;
		BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
		BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);

		drawPlayerSaber = qtrue;
	}
	else if (cent->currentState.weapon == WP_SABER 
		&& cent->currentState.saberHolstered < 2 )
	{
		if ( (!cent->currentState.saberInFlight //saber not in flight
				|| ci->saber[1].soundLoop) //???
			&& !(cent->currentState.eFlags & EF_DEAD))//still alive
		{//racc - do the saber blade hum.
			vec3_t soundSpot;
			qboolean didFirstSound = qfalse;

			if (cg.snap->ps.clientNum == cent->currentState.number)
			{//racc - render the blade on top of ourself if it's our blade.
				//trap_S_AddLoopingSound( cent->currentState.number, cg.refdef.vieworg, vec3_origin, 
				//	trap_S_RegisterSound( "sound/weapons/saber/saberhum1.wav" ) );
				VectorCopy(cg.refdef.vieworg, soundSpot);
			}
			else
			{//racc - otherwise, generate the noise at the player's lerpOrigin
				//trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, 
				//	trap_S_RegisterSound( "sound/weapons/saber/saberhum1.wav" ) );
				VectorCopy(cent->lerpOrigin, soundSpot);
			}

			if (ci->saber[0].model[0] 
				&& ci->saber[0].soundLoop 
				&& !cent->currentState.saberInFlight)
			{//racc - if our primary saber isn't in flight, use it's hum noise.
				int i = 0;
				qboolean hasLen = qfalse;

				while (i < ci->saber[0].numBlades)
				{
					if (ci->saber[0].blade[i].length)
					{
						hasLen = qtrue;
						break;
					}
					i++;
				}

				if (hasLen)
				{
					trap_S_AddLoopingSound( cent->currentState.number, soundSpot, vec3_origin, 
						ci->saber[0].soundLoop );
					didFirstSound = qtrue;
				}
			}
			if (ci->saber[1].model[0] 
				&& ci->saber[1].soundLoop 
					&& (!didFirstSound || ci->saber[0].soundLoop != ci->saber[1].soundLoop))
			{//racc - do second saber's hum if we need to.
				int i = 0;
				qboolean hasLen = qfalse;

				while (i < ci->saber[1].numBlades)
				{
					if (ci->saber[1].blade[i].length)
					{
						hasLen = qtrue;
						break;
					}
					i++;
				}

				if (hasLen)
				{
					trap_S_AddLoopingSound( cent->currentState.number, soundSpot, vec3_origin, 
						ci->saber[1].soundLoop );
				}
			}
		}

		if (iwantout 
			&& !cent->currentState.saberInFlight)
		{//racc - we're doing minimal rendering and our saber isn't out.  IE, don't render the saber blade.
			if (cent->currentState.eFlags & EF_DEAD)
			{
				if (cent->ghoul2 
					&& cent->currentState.saberInFlight 
					&& g2HasWeapon)
				{ //special case, kill the saber on a freshly dead player if another source says to.
					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), 1);
					g2HasWeapon = qfalse;
				}
			}
			return;
			//goto endOfCall;
		}

		if (g2HasWeapon 
			&& cent->currentState.saberInFlight)
		{ //keep this set, so we don't re-unholster the thing when we get it back, even if it's knocked away.
			cent->saberWasInFlight = qtrue;
		}

		if (cent->currentState.saberInFlight 
			&& cent->currentState.saberEntityNum)
		{//racc - saber in active saber throw.
			centity_t *saberEnt;

			saberEnt = &cg_entities[cent->currentState.saberEntityNum];

			if (/*!cent->bolt4 &&*/ g2HasWeapon || !cent->bolt3 ||
				saberEnt->serverSaberHitIndex != saberEnt->currentState.modelindex/*|| !cent->saberLength*/)
			{ //saber is in flight, do not have it as a standard weapon model
				qboolean addBolts = qfalse;
				mdxaBone_t boltMat;

				if (g2HasWeapon)
				{
					//ah well, just stick it over the right hand right now.
					trap_G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_rhand, &boltMat, cent->turAngles, cent->lerpOrigin,
						cg.time, cgs.gameModels, cent->modelScale);
					BG_GiveMeVectorFromMatrix(&boltMat, ORIGIN, saberEnt->currentState.pos.trBase);

					trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), 1);
					g2HasWeapon = qfalse;
				}

				//cent->bolt4 = 1;

				saberEnt->currentState.pos.trTime = cg.time;
				saberEnt->currentState.apos.trTime = cg.time;

				VectorCopy(saberEnt->currentState.pos.trBase, saberEnt->lerpOrigin);
				VectorCopy(saberEnt->currentState.apos.trBase, saberEnt->lerpAngles);

				cent->bolt3 = saberEnt->currentState.apos.trBase[0];
				if (!cent->bolt3)
				{
					cent->bolt3 = 1;
				}
				cent->bolt2 = 0;

				saberEnt->currentState.bolt2 = 123;

				if (saberEnt->ghoul2 &&
					saberEnt->serverSaberHitIndex == saberEnt->currentState.modelindex)
				{
					// now set up the gun bolt on it
					addBolts = qtrue;
				}
				else
				{
					const char *saberModel = CG_ConfigString( CS_MODELS+saberEnt->currentState.modelindex );

					saberEnt->serverSaberHitIndex = saberEnt->currentState.modelindex;

					if (saberEnt->ghoul2)
					{ //clean if we already have one (because server changed model string index)
						trap_G2API_CleanGhoul2Models(&(saberEnt->ghoul2));
						saberEnt->ghoul2 = 0;
					}

					if (saberModel && saberModel[0])
					{
						trap_G2API_InitGhoul2Model(&saberEnt->ghoul2, saberModel, 0, 0, 0, 0, 0);
					}
					else if (ci->saber[0].model[0])
					{
						trap_G2API_InitGhoul2Model(&saberEnt->ghoul2, ci->saber[0].model, 0, 0, 0, 0, 0);
					}
					else
					{
						trap_G2API_InitGhoul2Model(&saberEnt->ghoul2, "models/weapons2/saber/saber_w.glm", 0, 0, 0, 0, 0);
					}
					//trap_G2API_DuplicateGhoul2Instance(cent->ghoul2, &saberEnt->ghoul2);

					if (saberEnt->ghoul2)
					{
						addBolts = qtrue;
						//cent->bolt4 = 2;
						
						VectorCopy(saberEnt->currentState.pos.trBase, saberEnt->lerpOrigin);
						VectorCopy(saberEnt->currentState.apos.trBase, saberEnt->lerpAngles);
						saberEnt->currentState.pos.trTime = cg.time;
						saberEnt->currentState.apos.trTime = cg.time;
					}
				}

				if (addBolts)
				{
					int m = 0;
					int tagBolt;
					char *tagName;

					while (m < ci->saber[0].numBlades)
					{
						tagName = va("*blade%i", m+1);
						tagBolt = trap_G2API_AddBolt(saberEnt->ghoul2, 0, tagName);

						if (tagBolt == -1)
						{
							if (m == 0)
							{ //guess this is an 0ldsk3wl saber
								tagBolt = trap_G2API_AddBolt(saberEnt->ghoul2, 0, "*flash");

								if (tagBolt == -1)
								{
									assert(0);
								}
								break;
							}

							if (tagBolt == -1)
							{
								assert(0);
								break;
							}
						}

						m++;
					}
				}
			}
			/*else if (cent->bolt4 != 2)
			{
				if (saberEnt->ghoul2)
				{
					trap_G2API_AddBolt(saberEnt->ghoul2, 0, "*flash");
					cent->bolt4 = 2;
				}
			}*/

			if (saberEnt && saberEnt->ghoul2 /*&& cent->bolt4 == 2*/)
			{
				vec3_t bladeAngles;
				vec3_t tAng;
				vec3_t efOrg;
				float wv;
				int k = 0;
				int l = 0;
				addspriteArgStruct_t fxSArgs;

				if (!cent->bolt2)
				{
					cent->bolt2 = cg.time;
				}

				if (cent->bolt3 != 90)
				{
					if (cent->bolt3 < 90)
					{
						cent->bolt3 += (cg.time - cent->bolt2)*0.5;

						if (cent->bolt3 > 90)
						{
							cent->bolt3 = 90;
						}
					}
					else if (cent->bolt3 > 90)
					{
						cent->bolt3 -= (cg.time - cent->bolt2)*0.5;

						if (cent->bolt3 < 90)
						{
							cent->bolt3 = 90;
						}
					}
				}

				cent->bolt2 = cg.time;

				saberEnt->currentState.apos.trBase[0] = cent->bolt3;
				saberEnt->lerpAngles[0] = cent->bolt3;

				if (!saberEnt->currentState.saberInFlight && saberEnt->currentState.bolt2 != 123)
				{ //owner is pulling is back
					if ( !(ci->saber[0].saberFlags&SFL_RETURN_DAMAGE)
						|| cent->currentState.saberHolstered )
					{
						vec3_t owndir;

						VectorSubtract(saberEnt->lerpOrigin, cent->lerpOrigin, owndir);
						VectorNormalize(owndir);

						vectoangles(owndir, owndir);

						owndir[0] += 90;

						VectorCopy(owndir, saberEnt->currentState.apos.trBase);
						VectorCopy(owndir, saberEnt->lerpAngles);
						VectorClear(saberEnt->currentState.apos.trDelta);
					}
				}

				//We don't actually want to rely entirely on server updates to render the position of the saber, because we actually know generally where
				//it's going to be before the first position update even gets here, and it needs to start getting rendered the instant the saber model is
				//removed from the player hand. So we'll just render it manually and let normal rendering for the entity be ignored.
				if (!saberEnt->currentState.saberInFlight && saberEnt->currentState.bolt2 != 123)
				{ //tell it that we're a saber and to render the glow around our handle because we're being pulled back
					saberEnt->bolt3 = 999;
				}

				saberEnt->currentState.modelGhoul2 = 1;
				CG_ManualEntityRender(saberEnt);
				saberEnt->bolt3 = 0;
				saberEnt->currentState.modelGhoul2 = 127;

				VectorCopy(saberEnt->lerpAngles, bladeAngles);
				bladeAngles[ROLL] = 0;

				if ( ci->saber[0].numBlades > 1//staff
					&& cent->currentState.saberHolstered == 1 )//extra blades off
				{//only first blade should be on
					BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
					BG_SI_SetDesiredLength(&ci->saber[0], -1, 0);
				}
				else
				{
					BG_SI_SetDesiredLength(&ci->saber[0], -1, -1);
				}
				if ( ci->saber[1].model	//dual sabers
					&& cent->currentState.saberHolstered == 1 )//second one off
				{
					BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);
				}
				else
				{
					BG_SI_SetDesiredLength(&ci->saber[1], -1, -1);
				}

				//while (l < MAX_SABERS)
				//Only want to do for the first saber actually, it's the one in flight.
				while (l < 1)
				{
					if (!ci->saber[l].model[0])
					{
						break;
					}

					k = 0;
					while (k < ci->saber[l].numBlades)
					{
						if ( //cent->currentState.fireflag == SS_STAFF&& //in saberstaff style
							l == 0//first saber
							&& cent->currentState.saberHolstered == 1 //extra blades should be off
							&& k > 0 )//this is an extra blade
						{//extra blades off
							//don't draw them
							CG_AddSaberBlade(cent, saberEnt, NULL, 0, 0, l, k, saberEnt->lerpOrigin, bladeAngles, qtrue, qtrue);
						}
						else
						{
							CG_AddSaberBlade(cent, saberEnt, NULL, 0, 0, l, k, saberEnt->lerpOrigin, bladeAngles, qtrue, qfalse);
						}

						k++;
					}
					if ( ci->saber[l].numBlades > 2 )
					{//add a single glow for the saber based on all the blade colors combined
						//[RGBSabers]
						CG_DoSaberLight( &ci->saber[l], cent->currentState.clientNum, l );
						//[/RGBSabers]
					}

					l++;
				}

				//Make the player's hand glow while guiding the saber
				VectorSet( tAng, cent->turAngles[PITCH], cent->turAngles[YAW], cent->turAngles[ROLL] );

				trap_G2API_GetBoltMatrix(cent->ghoul2, 0, ci->bolt_rhand, &boltMatrix, tAng, cent->lerpOrigin, cg.time, cgs.gameModels, cent->modelScale);

				efOrg[0] = boltMatrix.matrix[0][3];
				efOrg[1] = boltMatrix.matrix[1][3];
				efOrg[2] = boltMatrix.matrix[2][3];

				wv = sin( cg.time * 0.003f ) * 0.08f + 0.1f;

				//trap_FX_AddSprite( NULL, efOrg, NULL, NULL, 8.0f, 8.0f, wv, wv, 0.0f, 0.0f, 1.0f, cgs.media.yellowSaberGlowShader, 0x08000000 );
				VectorCopy(efOrg, fxSArgs.origin);
				VectorClear(fxSArgs.vel);
				VectorClear(fxSArgs.accel);
				fxSArgs.scale = 8.0f;
				fxSArgs.dscale = 8.0f;
				fxSArgs.sAlpha = wv;
				fxSArgs.eAlpha = wv;
				fxSArgs.rotation = 0.0f;
				fxSArgs.bounce = 0.0f;
				fxSArgs.life = 1.0f;
				fxSArgs.shader = cgs.media.yellowDroppedSaberShader;
				fxSArgs.flags = 0x08000000;
				trap_FX_AddSprite(&fxSArgs);
			}
		}
		else
		{//saber is not in a throw.
			if ( ci->saber[0].numBlades > 1//staff
				&& cent->currentState.saberHolstered == 1 )//extra blades off
			{//only first blade should be on
				BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
				BG_SI_SetDesiredLength(&ci->saber[0], -1, 0);
			}
			else
			{
				BG_SI_SetDesiredLength(&ci->saber[0], -1, -1);
			}
			if ( ci->saber[1].model	//dual sabers
				&& cent->currentState.saberHolstered == 1 )//second one off
			{
				BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);
			}
			else
			{
				BG_SI_SetDesiredLength(&ci->saber[1], -1, -1);
			}
		}

		//If the arm the saber is in is broken, turn it off.
		/*
		if (cent->currentState.brokenLimbs & (1 << BROKENLIMB_RARM))
		{
			BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
		}
		*/
		//Leaving right arm on, at least for now.
		if (cent->currentState.brokenLimbs & (1 << BROKENLIMB_LARM))
		{
			BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);
		}

		if (!cent->currentState.saberEntityNum)
		{
			BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
			//BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);
		}
		/*
		else
		{
			BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
			BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);
		}
		*/
		drawPlayerSaber = qtrue;
	}
	else if (cent->currentState.weapon == WP_SABER)
	{
		//cent->saberLength = 0;
		BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
		BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);

		drawPlayerSaber = qtrue;
	}
	else
	{
		//cent->saberLength = 0;
		BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
		BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);

		BG_SI_SetLength(&ci->saber[0], 0);
		BG_SI_SetLength(&ci->saber[1], 0);
	}

#ifdef _RAG_BOLT_TESTING
	if (cent->currentState.eFlags & EF_RAG)
	{
		CG_TempTestFunction(cent, cent->turAngles);
	}
#endif

	if (cent->currentState.weapon == WP_SABER)
	{
		BG_SI_SetLengthGradual(&ci->saber[0], cg.time);
		BG_SI_SetLengthGradual(&ci->saber[1], cg.time);
	}

	if (drawPlayerSaber)
	{
		centity_t *saberEnt;
		int k = 0;
		int l = 0;

		if (!cent->currentState.saberEntityNum)
		{
			l = 1; //The "primary" saber is missing or in flight or something, so only try to draw in the second one
		}
		else if (!cent->currentState.saberInFlight)
		{
			saberEnt = &cg_entities[cent->currentState.saberEntityNum];

			if (/*cent->bolt4 && */!g2HasWeapon)
			{
				trap_G2API_CopySpecificGhoul2Model(CG_G2WeaponInstance(cent, WP_SABER), 0, cent->ghoul2, 1);

				if (saberEnt && saberEnt->ghoul2)
				{
					trap_G2API_CleanGhoul2Models(&(saberEnt->ghoul2));
				}

				saberEnt->currentState.modelindex = 0;
				saberEnt->ghoul2 = NULL;
				VectorClear(saberEnt->currentState.pos.trBase);
			}

			cent->bolt3 = 0;
			cent->bolt2 = 0;
		}
		else
		{
			l = 1; //The "primary" saber is missing or in flight or something, so only try to draw in the second one
		}

		while (l < MAX_SABERS)
		{
			k = 0;

			if (!ci->saber[l].model[0])
			{
				break;
			}

			if (cent->currentState.eFlags2&EF2_HELD_BY_MONSTER)
			{
				//vectoangles(legs.axis[0], rootAngles);
#if 0
				if ( cent->currentState.hasLookTarget )//NOTE: lookTarget is an entity number, so this presumes that client 0 is NOT a Rancor...
				{
					centity_t	*rancor = &cg_entities[cent->currentState.lookTarget];
					if ( rancor && rancor->ghoul2 )
					{
						BG_AttachToRancor( rancor->ghoul2, //ghoul2 info
							rancor->lerpAngles[YAW],
							rancor->lerpOrigin,
							cg.time,
							cgs.gameModels,
							rancor->modelScale,
							(rancor->currentState.eFlags2&EF2_GENERIC_NPC_FLAG),
							legs.origin,
							rootAngles,
							NULL );
					}
				}
#else
				vectoangles(legs.axis[0], rootAngles);
#endif
			}

			while (k < ci->saber[l].numBlades)
			{
				if ( //cent->currentState.fireflag == SS_STAFF&& //in saberstaff style
					cent->currentState.saberHolstered == 1 //extra blades should be off
					&& k > 0 //this is an extra blade
					&& ci->saber[l].blade[k].length <= 0 )//it's completely off
				{//extra blades off
					//don't draw them
					CG_AddSaberBlade( cent, cent, NULL, 0, 0, l, k, legs.origin, rootAngles, qfalse, qtrue);
				}
				else if ( ci->saber[1].model[0]//we have a second saber
					&& cent->currentState.saberHolstered == 1 //it should be off
					&& l > 0//and this is the second one
					&& ci->saber[l].blade[k].length <= 0 )//it's completely off
				{//second saber is turned off and this blade is done with turning off
					CG_AddSaberBlade( cent, cent, NULL, 0, 0, l, k, legs.origin, rootAngles, qfalse, qtrue);
				}
				else
				{
					CG_AddSaberBlade( cent, cent, NULL, 0, 0, l, k, legs.origin, rootAngles, qfalse, qfalse);
				}

				k++;
			}
			if ( ci->saber[l].numBlades > 2 )
			{//add a single glow for the saber based on all the blade colors combined
				//[RGBSabers]
				CG_DoSaberLight( &ci->saber[l], cent->currentState.clientNum, l );
				//[/RGBSabers]
			}

			l++;
		}
	}

	if (cent->currentState.saberInFlight && !cent->currentState.saberEntityNum)
	{ //reset the length if the saber is knocked away
		BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
		BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);

		if (g2HasWeapon)
		{ //and remember to kill the bolton model in case we didn't get a thrown saber update first
			trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), 1);
			g2HasWeapon = qfalse;
		}
		cent->bolt3 = 0;
		cent->bolt2 = 0;
	}

	if (cent->currentState.eFlags & EF_DEAD)
	{
		if (cent->ghoul2 && cent->currentState.saberInFlight && g2HasWeapon)
		{ //special case, kill the saber on a freshly dead player if another source says to.
			trap_G2API_RemoveGhoul2Model(&(cent->ghoul2), 1);
			g2HasWeapon = qfalse;
		}
	}

	if (iwantout)
	{
		return;
		//goto endOfCall;
	}

	if ((cg.snap->ps.fd.forcePowersActive & (1 << FP_SEE)) && cg.snap->ps.clientNum != cent->currentState.number)
	{
		legs.shaderRGBA[0] = 255;
		legs.shaderRGBA[1] = 255;
		legs.shaderRGBA[2] = 0;
		legs.renderfx |= RF_MINLIGHT;
	}

	if (cg.snap->ps.duelInProgress /*&& cent->currentState.number != cg.snap->ps.clientNum*/)
	{ //I guess go ahead and glow your own client too in a duel
		if (cent->currentState.number != cg.snap->ps.duelIndex &&
			cent->currentState.number != cg.snap->ps.clientNum)
		{ //everyone not involved in the duel is drawn very dark
			legs.shaderRGBA[0] /= 5.0f;
			legs.shaderRGBA[1] /= 5.0f;
			legs.shaderRGBA[2] /= 5.0f;
			legs.renderfx |= RF_RGB_TINT;
		}
		else
		{ //adjust the glow by how far away you are from your dueling partner
			centity_t *duelEnt;

			duelEnt = &cg_entities[cg.snap->ps.duelIndex];
			
			if (duelEnt)
			{
				vec3_t vecSub;
				float subLen = 0;

				VectorSubtract(duelEnt->lerpOrigin, cg.snap->ps.origin, vecSub);
				subLen = VectorLength(vecSub);

				if (subLen < 1)
				{
					subLen = 1;
				}

				if (subLen > 1020)
				{
					subLen = 1020;
				}

				{
				//[MiscCodeTweaks]
				//fixed compiler issue.
				unsigned char savRGBA[3];
				savRGBA[0] = legs.shaderRGBA[0];
				savRGBA[1] = legs.shaderRGBA[1];
				savRGBA[2] = legs.shaderRGBA[2];

				//const unsigned char savRGBA[3] = {legs.shaderRGBA[0],legs.shaderRGBA[1],legs.shaderRGBA[2]};
				//[/MiscCodeTweaks]
				legs.shaderRGBA[0] = max(255-subLen/4,1);
				legs.shaderRGBA[1] = max(255-subLen/4,1);
				legs.shaderRGBA[2] = max(255-subLen/4,1);

				legs.renderfx &= ~RF_RGB_TINT;
				legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
				legs.customShader = cgs.media.forceShell;
		
				trap_R_AddRefEntityToScene( &legs );	//draw the shell

				legs.customShader = 0;	//reset to player model

				legs.shaderRGBA[0] = max(savRGBA[0]-subLen/8,1);
				legs.shaderRGBA[1] = max(savRGBA[1]-subLen/8,1);
				legs.shaderRGBA[2] = max(savRGBA[2]-subLen/8,1);
				}

				if (subLen <= 1024)
				{
					legs.renderfx |= RF_RGB_TINT;
				}
			}
		}
	}
	else
	{
		if (cent->currentState.bolt1 && !(cent->currentState.eFlags & EF_DEAD) && cent->currentState.number != cg.snap->ps.clientNum && (!cg.snap->ps.duelInProgress || cg.snap->ps.duelIndex != cent->currentState.number))
		{
			legs.shaderRGBA[0] = 50;
			legs.shaderRGBA[1] = 50;
			legs.shaderRGBA[2] = 50;
			legs.renderfx |= RF_RGB_TINT;
		}
	}

	if (cent->currentState.eFlags & EF_DISINTEGRATION)
	{
		if (!cent->dustTrailTime)
		{
			cent->dustTrailTime = cg.time;
			cent->miscTime = legs.frame;
		}

		if ((cg.time - cent->dustTrailTime) > 1500)
		{ //avoid rendering the entity after disintegration has finished anyway
			//goto endOfCall;
			return;
		}

		trap_G2API_SetBoneAnim(legs.ghoul2, 0, "model_root", cent->miscTime, cent->miscTime, BONE_ANIM_OVERRIDE_FREEZE, 1.0f, cg.time, cent->miscTime, -1);

		if (!cent->noLumbar)
		{
			trap_G2API_SetBoneAnim(legs.ghoul2, 0, "lower_lumbar", cent->miscTime, cent->miscTime, BONE_ANIM_OVERRIDE_FREEZE, 1.0f, cg.time, cent->miscTime, -1);

			if (cent->localAnimIndex <= 1)
			{
				trap_G2API_SetBoneAnim(legs.ghoul2, 0, "Motion", cent->miscTime, cent->miscTime, BONE_ANIM_OVERRIDE_FREEZE, 1.0f, cg.time, cent->miscTime, -1);
			}
		}

		CG_Disintegration(cent, &legs);

		//goto endOfCall;
		return;
	}
	else
	{
		cent->dustTrailTime = 0;
		cent->miscTime = 0;
	}

	if (cent->currentState.powerups & (1 << PW_CLOAKED))
	{
		if (!cent->cloaked)
		{
			cent->cloaked = qtrue;
			cent->uncloaking = cg.time + 2000;
		}
	}
	else if (cent->cloaked)
	{
		cent->cloaked = qfalse;
		cent->uncloaking = cg.time + 2000;
	}

	if (cent->uncloaking > cg.time)
	{//in the middle of cloaking
		if ((cg.snap->ps.fd.forcePowersActive & (1 << FP_SEE)) 
			&& cg.snap->ps.clientNum != cent->currentState.number)
		{//just draw him
			trap_R_AddRefEntityToScene( &legs );
		}
		else
		{
			float perc = (float)(cent->uncloaking - cg.time) / 2000.0f;
			if (( cent->currentState.powerups & ( 1 << PW_CLOAKED ))) 
			{//actually cloaking, so reverse it
				perc = 1.0f - perc;
			}

			if ( perc >= 0.0f && perc <= 1.0f )
			{
				legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
				legs.renderfx |= RF_RGB_TINT;
				legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = 255.0f * perc;
				legs.shaderRGBA[3] = 0;
				legs.customShader = cgs.media.cloakedShader;
				trap_R_AddRefEntityToScene( &legs );

				legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = 255;
				legs.shaderRGBA[3] = 255 * (1.0f - perc); // let model alpha in
				legs.customShader = 0; // use regular skin
				legs.renderfx &= ~RF_RGB_TINT;
				legs.renderfx |= RF_FORCE_ENT_ALPHA;
				trap_R_AddRefEntityToScene( &legs );
			}
		}
	}
	else if (( cent->currentState.powerups & ( 1 << PW_CLOAKED ))) 
	{//fully cloaked
		if ((cg.snap->ps.fd.forcePowersActive & (1 << FP_SEE)) 
			&& cg.snap->ps.clientNum != cent->currentState.number)
		{//just draw him
			trap_R_AddRefEntityToScene( &legs );
		}
		else
		{
			if (cg.renderingThirdPerson || cent->currentState.number != cg.predictedPlayerState.clientNum)
			{
				/*
				legs.renderfx = 0;//&= ~(RF_RGB_TINT|RF_ALPHA_FADE);
				legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = legs.shaderRGBA[3] = 255;
				legs.customShader = cgs.media.cloakedShader;

				legs.nonNormalizedAxes = qtrue;

				legs.modelScale[0] = 1.02f;
				legs.modelScale[1] = 1.02f;
				legs.modelScale[2] = 1.02f;
				VectorScale( legs.axis[0], legs.modelScale[0], legs.axis[0] );
				VectorScale( legs.axis[1], legs.modelScale[1], legs.axis[1] );
				VectorScale( legs.axis[2], legs.modelScale[2], legs.axis[2] );

				ScaleModelAxis(&legs);

				trap_R_AddRefEntityToScene( &legs );
				
				legs.modelScale[0] = 0.98f;
				legs.modelScale[1] = 0.98f;
				legs.modelScale[2] = 0.98f;
				VectorScale( legs.axis[0], legs.modelScale[0], legs.axis[0] );
				VectorScale( legs.axis[1], legs.modelScale[1], legs.axis[1] );
				VectorScale( legs.axis[2], legs.modelScale[2], legs.axis[2] );

				ScaleModelAxis(&legs);
				*/

				if (cg_shadows.integer != 2 && cgs.glconfig.stencilBits >= 4 && cg_renderToTextureFX.integer)
				{
					trap_R_SetRefractProp(1.0f, 0.0f, qfalse, qfalse); //don't need to do this every frame.. but..
					legs.customShader = 2; //crazy "refractive" shader
					trap_R_AddRefEntityToScene( &legs );
					legs.customShader = 0;
				}
				else
				{ //stencil buffer's in use, sorry
					legs.renderfx = 0;//&= ~(RF_RGB_TINT|RF_ALPHA_FADE);
					legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = legs.shaderRGBA[3] = 255;
					legs.customShader = cgs.media.cloakedShader;
					trap_R_AddRefEntityToScene( &legs );
					legs.customShader = 0;
				}
			}
		}
	}

	if (!(cent->currentState.powerups & (1 << PW_CLOAKED)))
	{ //don't add the normal model if cloaked
		CG_CheckThirdPersonAlpha( cent, &legs );
		trap_R_AddRefEntityToScene(&legs);
	}

	//cent->frame_minus2 = cent->frame_minus1;
	VectorCopy(cent->frame_minus1, cent->frame_minus2);
	
	if (cent->frame_minus1_refreshed)
	{
		cent->frame_minus2_refreshed = 1;
	}

	//cent->frame_minus1 = legs;
	VectorCopy(legs.origin, cent->frame_minus1);

	cent->frame_minus1_refreshed = 1;

	if (!cent->frame_hold_refreshed && (cent->currentState.powerups & (1 << PW_SPEEDBURST)))
	{
		cent->frame_hold_time = cg.time + 254;
	}

	if (cent->frame_hold_time >= cg.time)
	{
		refEntity_t reframe_hold;

		if (!cent->frame_hold_refreshed)
		{ //We're taking the ghoul2 instance from the original refent and duplicating it onto our refent alias so that we can then freeze the frame and fade it for the effect
			if (cent->frame_hold && trap_G2_HaveWeGhoul2Models(cent->frame_hold) &&
				cent->frame_hold != cent->ghoul2)
			{
				trap_G2API_CleanGhoul2Models(&(cent->frame_hold));
			}
			reframe_hold = legs;
			cent->frame_hold_refreshed = 1;
			reframe_hold.ghoul2 = NULL;
	
			trap_G2API_DuplicateGhoul2Instance(cent->ghoul2, &cent->frame_hold);

			//Set the animation to the current frame and freeze on end
			//trap_G2API_SetBoneAnim(cent->frame_hold.ghoul2, 0, "model_root", cent->frame_hold.frame, cent->frame_hold.frame, BONE_ANIM_OVERRIDE_FREEZE, 1.0f, cg.time, cent->frame_hold.frame, -1);
			trap_G2API_SetBoneAnim(cent->frame_hold, 0, "model_root", legs.frame, legs.frame, 0, 1.0f, cg.time, legs.frame, -1);
		}
		else
		{
			reframe_hold = legs;
			reframe_hold.ghoul2 = cent->frame_hold;
		}

		reframe_hold.renderfx |= RF_FORCE_ENT_ALPHA;
		reframe_hold.shaderRGBA[3] = (cent->frame_hold_time - cg.time);
		if (reframe_hold.shaderRGBA[3] > 254)
		{
			reframe_hold.shaderRGBA[3] = 254;
		}
		if (reframe_hold.shaderRGBA[3] < 1)
		{
			reframe_hold.shaderRGBA[3] = 1;
		}

		reframe_hold.ghoul2 = cent->frame_hold;
		trap_R_AddRefEntityToScene(&reframe_hold);
	}
	else
	{
		cent->frame_hold_refreshed = 0;
	}

	//
	// add the gun / barrel / flash
	//
	if (cent->currentState.weapon != WP_EMPLACED_GUN)
	{
		CG_AddPlayerWeapon( &legs, NULL, cent, ci->team, rootAngles, qtrue );
	}
	// add powerups floating behind the player
	CG_PlayerPowerups( cent, &legs );

	//[TrueView]
	if ((cent->currentState.forcePowersActive & (1 << FP_RAGE)) &&
		(cg.renderingThirdPerson || cent->currentState.number != cg.snap->ps.clientNum
		|| cg_trueguns.integer || cg.predictedPlayerState.weapon == WP_SABER
		|| cg.predictedPlayerState.weapon == WP_MELEE))
	//if ((cent->currentState.forcePowersActive & (1 << FP_RAGE)) &&
	//	(cg.renderingThirdPerson || cent->currentState.number != cg.snap->ps.clientNum))
	//[/TrueView]
	{
		//legs.customShader = cgs.media.rageShader;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.renderfx &= ~RF_MINLIGHT;

		legs.renderfx |= RF_RGB_TINT;
		legs.shaderRGBA[0] = 255;
		legs.shaderRGBA[1] = legs.shaderRGBA[2] = 0;
		legs.shaderRGBA[3] = 255;

		if ( rand() & 1 )
		{
			legs.customShader = cgs.media.electricBodyShader;	
		}
		else
		{
			legs.customShader = cgs.media.electricBody2Shader;
		}

		trap_R_AddRefEntityToScene(&legs);
	}

	if (!cg.snap->ps.duelInProgress && cent->currentState.bolt1 && !(cent->currentState.eFlags & EF_DEAD) && cent->currentState.number != cg.snap->ps.clientNum && (!cg.snap->ps.duelInProgress || cg.snap->ps.duelIndex != cent->currentState.number))
	{
		legs.shaderRGBA[0] = 50;
		legs.shaderRGBA[1] = 50;
		legs.shaderRGBA[2] = 255;

		legs.renderfx &= ~RF_RGB_TINT;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.customShader = cgs.media.forceSightBubble;
		
		trap_R_AddRefEntityToScene( &legs );
	}

	if ( CG_VehicleShouldDrawShields( cent ) //vehicle
		|| (checkDroidShields && CG_VehicleShouldDrawShields( &cg_entities[cent->currentState.m_iVehicleNum] )) )//droid in vehicle
	{//Vehicles have form-fitting shields
		Vehicle_t *pVeh = cent->m_pVehicle;
		if ( checkDroidShields )
		{
			pVeh = cg_entities[cent->currentState.m_iVehicleNum].m_pVehicle;
		}
		legs.shaderRGBA[0] = 255;
		legs.shaderRGBA[1] = 255;
		legs.shaderRGBA[2] = 255;
		legs.shaderRGBA[3] = 10.0f+(sin((float)(cg.time/4))*128.0f);//112.0 * ((cent->damageTime - cg.time) / MIN_SHIELD_TIME) + random()*16;

		legs.renderfx &= ~RF_RGB_TINT;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;

		if ( pVeh 
			&& pVeh->m_pVehicleInfo
			&& pVeh->m_pVehicleInfo->shieldShaderHandle )
		{//use the vehicle-specific shader
			legs.customShader = pVeh->m_pVehicleInfo->shieldShaderHandle;
		}
		else
		{
			legs.customShader = cgs.media.playerShieldDamage;
		}
		
		trap_R_AddRefEntityToScene( &legs );
	}
	//For now, these two are using the old shield shader. This is just so that you
	//can tell it apart from the JM/duel shaders, but it's still very obvious.
	if (cent->currentState.forcePowersActive & (1 << FP_PROTECT))
	{ //aborb is represented by green..
		refEntity_t prot;
		
		memcpy(&prot, &legs, sizeof(prot));

		prot.shaderRGBA[0] = 0;
		prot.shaderRGBA[1] = 128;
		prot.shaderRGBA[2] = 0;
		prot.shaderRGBA[3] = 254;

		prot.renderfx &= ~RF_RGB_TINT;
		prot.renderfx &= ~RF_FORCE_ENT_ALPHA;
		prot.customShader = cgs.media.protectShader;

		/*
		if (!prot.modelScale[0] && !prot.modelScale[1] && !prot.modelScale[2])
		{
			prot.modelScale[0] = prot.modelScale[1] = prot.modelScale[2] = 1.0f;
		}
		VectorScale(prot.modelScale, 1.1f, prot.modelScale);
		prot.origin[2] -= 2.0f;
		ScaleModelAxis(&prot);
		*/

		trap_R_AddRefEntityToScene( &prot );
	}
	//if (cent->currentState.forcePowersActive & (1 << FP_ABSORB))
	//Showing only when the power has been active (absorbed something) recently now, instead of always.
	//AND
	//always show if it is you with the absorb on
	if ((cent->currentState.number == cg.predictedPlayerState.clientNum && (cg.predictedPlayerState.fd.forcePowersActive & (1<<FP_ABSORB))) ||
		(cent->teamPowerEffectTime > cg.time && cent->teamPowerType == 3))
	{ //aborb is represented by blue..
		legs.shaderRGBA[0] = 0;
		legs.shaderRGBA[1] = 0;
		legs.shaderRGBA[2] = 255;
		legs.shaderRGBA[3] = 254;

		legs.renderfx &= ~RF_RGB_TINT;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.customShader = cgs.media.playerShieldDamage;
		
		trap_R_AddRefEntityToScene( &legs );
	}

	if (cent->currentState.isJediMaster && cg.snap->ps.clientNum != cent->currentState.number)
	{
		legs.shaderRGBA[0] = 100;
		legs.shaderRGBA[1] = 100;
		legs.shaderRGBA[2] = 255;

		legs.renderfx &= ~RF_RGB_TINT;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.renderfx |= RF_NODEPTH;
		legs.customShader = cgs.media.forceShell;
		
		trap_R_AddRefEntityToScene( &legs );

		legs.renderfx &= ~RF_NODEPTH;
	}

	if ((cg.snap->ps.fd.forcePowersActive & (1 << FP_SEE)) && cg.snap->ps.clientNum != cent->currentState.number && cg_auraShell.integer)
	{
		if (cgs.gametype == GT_SIEGE)
		{	// A team game
			if ( ci->team == TEAM_SPECTATOR || ci->team == TEAM_FREE )
			{//yellow
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = 255;
				legs.shaderRGBA[2] = 0;
			}
			else if ( ci->team != cgs.clientinfo[cg.snap->ps.clientNum].team )
			{//red
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = 50;
				legs.shaderRGBA[2] = 50;
			}
			else
			{//green
				legs.shaderRGBA[0] = 50;
				legs.shaderRGBA[1] = 255;
				legs.shaderRGBA[2] = 50;
			}
		}
		else if (cgs.gametype >= GT_TEAM)
		{	// A team game
			switch(ci->team)
			{
			case TEAM_RED:
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = 50;
				legs.shaderRGBA[2] = 50;
				break;
			case TEAM_BLUE:
				legs.shaderRGBA[0] = 75;
				legs.shaderRGBA[1] = 75;
				legs.shaderRGBA[2] = 255;
				break;

			default:
				legs.shaderRGBA[0] = 255;
				legs.shaderRGBA[1] = 255;
				legs.shaderRGBA[2] = 0;
				break;
			}
		}
		else
		{	// Not a team game
			legs.shaderRGBA[0] = 255;
			legs.shaderRGBA[1] = 255;
			legs.shaderRGBA[2] = 0;
		}

/*		if (cg.snap->ps.fd.forcePowerLevel[FP_SEE] <= FORCE_LEVEL_1)
		{
			legs.renderfx |= RF_MINLIGHT;
		}
		else
*/		{	// See through walls.
			legs.renderfx |= RF_MINLIGHT | RF_NODEPTH;

			if (cg.snap->ps.fd.forcePowerLevel[FP_SEE] < FORCE_LEVEL_2)
			{ //only level 2+ can see players through walls
				legs.renderfx &= ~RF_NODEPTH;
			}
		}

		legs.renderfx &= ~RF_RGB_TINT;
		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.customShader = cgs.media.sightShell;
		
		trap_R_AddRefEntityToScene( &legs );
	}

	// Electricity
	//------------------------------------------------
	if ( cent->currentState.emplacedOwner > cg.time ) 
	{
		int	dif = cent->currentState.emplacedOwner - cg.time;
		vec3_t tempAngles;

		if ( dif > 0 && random() > 0.4f )
		{
			// fade out over the last 500 ms
			int brightness = 255;
			
			if ( dif < 500 )
			{
				brightness = floor((dif - 500.0f) / 500.0f * 255.0f );
			}

			legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
			legs.renderfx &= ~RF_MINLIGHT;

			legs.renderfx |= RF_RGB_TINT;
			legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = brightness;
			legs.shaderRGBA[3] = 255;

			if ( rand() & 1 )
			{
				legs.customShader = cgs.media.electricBodyShader;	
			}
			else
			{
				legs.customShader = cgs.media.electricBody2Shader;
			}

			trap_R_AddRefEntityToScene( &legs );

			if ( random() > 0.9f )
				trap_S_StartSound ( NULL, cent->currentState.number, CHAN_AUTO, cgs.media.crackleSound );
		}

		VectorSet(tempAngles, 0, cent->lerpAngles[YAW], 0);
		CG_ForceElectrocution( cent, legs.origin, tempAngles, cgs.media.boltShader, qfalse );
	} 

	if (cent->currentState.powerups & (1 << PW_SHIELDHIT))
	{
		/*
		legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = 255.0f * 0.5f;//t;
		legs.shaderRGBA[3] = 255;
		legs.renderfx &= ~RF_ALPHA_FADE;
		legs.renderfx |= RF_RGB_TINT;
		*/

		legs.shaderRGBA[0] = legs.shaderRGBA[1] = legs.shaderRGBA[2] = Q_irand(1, 255);

		legs.renderfx &= ~RF_FORCE_ENT_ALPHA;
		legs.renderfx &= ~RF_MINLIGHT;
		legs.renderfx &= ~RF_RGB_TINT;
		legs.customShader = cgs.media.playerShieldDamage;
		
		trap_R_AddRefEntityToScene( &legs );
	}
#if 0
endOfCall:
	
	if (cgBoneAnglePostSet.refreshSet)
	{
		trap_G2API_SetBoneAngles(cgBoneAnglePostSet.ghoul2, cgBoneAnglePostSet.modelIndex, cgBoneAnglePostSet.boneName,
			cgBoneAnglePostSet.angles, cgBoneAnglePostSet.flags, cgBoneAnglePostSet.up, cgBoneAnglePostSet.right,
			cgBoneAnglePostSet.forward, cgBoneAnglePostSet.modelList, cgBoneAnglePostSet.blendTime, cgBoneAnglePostSet.currentTime);

		cgBoneAnglePostSet.refreshSet = qfalse;
	}
#endif
}


//=====================================================================

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity( centity_t *cent ) 
{
	clientInfo_t *ci;
	int i = 0;
	int j = 0;

//	cent->errorTime = -99999;		// guarantee no error decay added
//	cent->extrapolated = qfalse;	

	if (cent->currentState.eType == ET_NPC)
	{
		if (cent->currentState.NPC_class == CLASS_VEHICLE &&
			cent->m_pVehicle &&
			cent->m_pVehicle->m_pVehicleInfo->type == VH_FIGHTER &&
			cg.predictedPlayerState.m_iVehicleNum &&
			cent->currentState.number == cg.predictedPlayerState.m_iVehicleNum)
		{ //holy hackery, batman!
			//I don't think this will break anything. But really, do I ever?
			return;
		}

		if (!cent->npcClient)
		{
			CG_CreateNPCClient(&cent->npcClient); //allocate memory for it

			if (!cent->npcClient)
			{
				assert(0);
				return;
			}

			memset(cent->npcClient, 0, sizeof(clientInfo_t));
			cent->npcClient->ghoul2Model = NULL;
			//[CoOp]
			//figure out the gender for this model
			cent->npcClient->gender = FindGender(CG_ConfigString( CS_MODELS+cent->currentState.modelindex ), cent);
			//[/CoOp]
		}

		ci = cent->npcClient;

		assert(ci);

		//just force these guys to be set again, it won't hurt anything if they're
		//already set.
		cent->npcLocalSurfOff = 0;
		cent->npcLocalSurfOn = 0;
	}
	else
	{
		ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	}

	while (i < MAX_SABERS)
	{
		j = 0;
		while (j < ci->saber[i].numBlades)
		{
			ci->saber[i].blade[j].trail.lastTime = -20000;
			j++;
		}
		i++;
	}

	ci->facial_blink = -1;
	ci->facial_frown = 0;
	ci->facial_aux = 0;
	ci->superSmoothTime = 0;

	//reset lerp origin smooth point
	VectorCopy(cent->lerpOrigin, cent->beamEnd);

	if (cent->currentState.eType != ET_NPC ||
		!(cent->currentState.eFlags & EF_DEAD))
	{
		CG_ClearLerpFrame( cent, ci, &cent->pe.legs, cent->currentState.legsAnim, qfalse);
		CG_ClearLerpFrame( cent, ci, &cent->pe.torso, cent->currentState.torsoAnim, qtrue);

		BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, cent->lerpOrigin );
		BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );

//		VectorCopy( cent->lerpOrigin, cent->rawOrigin );
		VectorCopy( cent->lerpAngles, cent->rawAngles );

		memset( &cent->pe.legs, 0, sizeof( cent->pe.legs ) );
		cent->pe.legs.yawAngle = cent->rawAngles[YAW];
		cent->pe.legs.yawing = qfalse;
		cent->pe.legs.pitchAngle = 0;
		cent->pe.legs.pitching = qfalse;

		memset( &cent->pe.torso, 0, sizeof( cent->pe.legs ) );
		cent->pe.torso.yawAngle = cent->rawAngles[YAW];
		cent->pe.torso.yawing = qfalse;
		cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
		cent->pe.torso.pitching = qfalse;

		if (cent->currentState.eType == ET_NPC)
		{ //just start them off at 0 pitch
			cent->pe.torso.pitchAngle = 0;
		}

		if ((cent->ghoul2 == NULL) && ci->ghoul2Model && trap_G2_HaveWeGhoul2Models(ci->ghoul2Model))
		{
			trap_G2API_DuplicateGhoul2Instance(ci->ghoul2Model, &cent->ghoul2);
			cent->weapon = 0;
			cent->ghoul2weapon = NULL;

			//Attach the instance to this entity num so we can make use of client-server
			//shared operations if possible.
			trap_G2API_AttachInstanceToEntNum(cent->ghoul2, cent->currentState.number, qfalse);

			if (trap_G2API_AddBolt(cent->ghoul2, 0, "face") == -1)
			{ //check now to see if we have this bone for setting anims and such
				cent->noFace = qtrue;
			}

			cent->localAnimIndex = CG_G2SkelForModel(cent->ghoul2);
			//[ANIMEVENTS]
			//This will probably cause NPCs that use this code path to use their 
			//skeleton's animevent instead of their actual models.  Fortunately, I don't
			//think NPCs ever go thru here anyway.
			cent->eventAnimIndex = CG_ParseAnimationEvtFile( va("models/players/%s/", ci->modelName), cent->localAnimIndex, cgNumAnimEvents );
			//cent->eventAnimIndex = CG_G2EvIndexForModel(cent->ghoul2, cent->localAnimIndex);
			//[/ANIMEVENTS]

			//CG_CopyG2WeaponInstance(cent->currentState.weapon, ci->ghoul2Model);
			//cent->weapon = cent->currentState.weapon;
		}
	}

	//do this to prevent us from making a saber unholster sound the first time we enter the pvs
	if (cent->currentState.number != cg.predictedPlayerState.clientNum &&
		cent->currentState.weapon == WP_SABER &&
		cent->weapon != cent->currentState.weapon)
	{
		cent->weapon = cent->currentState.weapon;
		if (cent->ghoul2 && ci->ghoul2Model)
		{
			CG_CopyG2WeaponInstance(cent, cent->currentState.weapon, cent->ghoul2);
			cent->ghoul2weapon = CG_G2WeaponInstance(cent, cent->currentState.weapon);
		}
		if (!cent->currentState.saberHolstered)
		{ //if not holstered set length and desired length for both blades to full right now.
			BG_SI_SetDesiredLength(&ci->saber[0], 0, -1);
			BG_SI_SetDesiredLength(&ci->saber[1], 0, -1);

			i = 0;
			while (i < MAX_SABERS)
			{
				j = 0;
				while (j < ci->saber[i].numBlades)
				{
					ci->saber[i].blade[j].length = ci->saber[i].blade[j].lengthMax;
					j++;
				}
				i++;
			}
		}
	}


	if ( cg_debugPosition.integer ) {
		CG_Printf("%i ResetPlayerEntity yaw=%i\n", cent->currentState.number, cent->pe.torso.yawAngle );
	}
}

