//This file contains functions relate to the saber impact behavior of OJP Enhanced's saber system.
#ifndef _SABERBEH_H
#define _SABERBEH_H

#include "q_shared.h"

//This is the attack parry rate for the bots since bots don't block intelligently
//This is multipled by the bot's skill level (which can be 1-5) and is actually a percentage of the total parries as
//set by BOT_PARRYRATE.
#define BOT_ATTACKPARRYRATE			20
#define MPCOST_PARRIED				3		//base MP cost of getting parried.	
#define MPCOST_PARRIED_ATTACKFAKE	6		//base MP cost of an attack fake getting parried.
#define MPCOST_PARRYING				-3		//MP you deplete by parrying in general
#define MPCOST_PARRYING_ATTACKFAKE	-4		//MP you deplete by parrying an attackfake
//[SaberSys]
//This struct holds all the relivent saber mechanics data
struct sabmech_s {
	//Do Knockaway animation
	qboolean doStun;

	//Do knockdown animation
	qboolean doKnockdown;

	//Do butterFingers (disarment)
	qboolean doButterFingers;

	//did a parry (do knockaway animation)
	qboolean doParry;

	qboolean doSlowBounce;

	qboolean doHeavySlowBounce;

#ifdef _DEBUG
	int behaveMode;
#endif
};

typedef struct sabmech_s sabmech_t;

#ifdef _DEBUG
typedef enum
{//used to debug saber behavior.  
//Shows (with behaveMode) which mishap probability set was used for saber behavior calculations.
	SABBEHAVE_NONE = 0,
	SABBEHAVE_ATTACK,
	SABBEHAVE_ATTACKPARRIED,
	SABBEHAVE_ATTACKBLOCKED,
	SABBEHAVE_BLOCK,
	SABBEHAVE_BLOCKFAKED,
	SABBEHAVE_PARRY,
} sabBehave_t;
#endif



void SabBeh_RunSaberBehavior( gentity_t *self, sabmech_t *mechSelf, 
								gentity_t *otherOwner, sabmech_t *mechOther, vec3_t hitLoc, 
								qboolean *didHit, qboolean otherHitSaberBlade );
void SabBeh_AnimateSlowBounce(gentity_t* self, gentity_t *inflictor);
void SabBeh_AnimateHeavySlowBounce(gentity_t*, gentity_t *inflictor);
#endif

