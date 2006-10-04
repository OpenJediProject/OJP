#include "bg_saga.h"

#define DEFAULT_FORCEPOWERS		"5-1-000000000000000000"

//#define FORCEJUMP_INSTANTMETHOD 1

#ifdef _XBOX	// No bot has more than 150 bytes of chat right now
#define MAX_CHAT_BUFFER_SIZE 256
#else
#define MAX_CHAT_BUFFER_SIZE 8192
#endif
#define MAX_CHAT_LINE_SIZE 128

#define TABLE_BRANCH_DISTANCE 32
#define MAX_NODETABLE_SIZE 16384

#define MAX_LOVED_ONES 4
#define MAX_ATTACHMENT_NAME 64

#define MAX_FORCE_INFO_SIZE 2048

#define WPFLAG_JUMP					0x00000010 //jump when we hit this
#define WPFLAG_DUCK					0x00000020 //duck while moving around here
#define WPFLAG_NOVIS				0x00000400 //go here for a bit even with no visibility
#define WPFLAG_SNIPEORCAMPSTAND		0x00000800 //a good position to snipe or camp - stand
#define WPFLAG_WAITFORFUNC			0x00001000 //wait for a func brushent under this point before moving here
#define WPFLAG_SNIPEORCAMP			0x00002000 //a good position to snipe or camp - crouch
#define WPFLAG_ONEWAY_FWD			0x00004000 //can only go forward on the trial from here (e.g. went over a ledge)
#define WPFLAG_ONEWAY_BACK			0x00008000 //can only go backward on the trail from here
#define WPFLAG_GOALPOINT			0x00010000 //make it a goal to get here.. goal points will be decided by setting "weight" values
#define WPFLAG_RED_FLAG				0x00020000 //red flag
#define WPFLAG_BLUE_FLAG			0x00040000 //blue flag
#define WPFLAG_SIEGE_REBELOBJ		0x00080000 //rebel siege objective
#define WPFLAG_SIEGE_IMPERIALOBJ	0x00100000 //imperial siege objective
#define WPFLAG_NOMOVEFUNC			0x00200000 //don't move over if a func is under

#define WPFLAG_CALCULATED			0x00400000 //don't calculate it again
#define WPFLAG_NEVERONEWAY			0x00800000 //never flag it as one-way

//[TABBot]
#define WPFLAG_DESTROY_FUNCBREAK	0x01000000 //destroy all the func_breakables in the area
												//before moving to this waypoint
#define WPFLAG_REDONLY				0x02000000 //only bots on the red team will be able to
												//use this waypoint
#define WPFLAG_BLUEONLY				0x04000000 //only bots on the blue team will be able to
												//use this waypoint
#define WPFLAG_FORCEPUSH			0x08000000 //force push all the active func_doors in the
												//area before moving to this waypoint.
#define WPFLAG_FORCEPULL			0x10000000 //force pull all the active func_doors in the
												//area before moving to this waypoint.			
//[/TABBot]

#define LEVELFLAG_NOPOINTPREDICTION			1 //don't take waypoint beyond current into account when adjusting path view angles
#define LEVELFLAG_IGNOREINFALLBACK			2 //ignore enemies when in a fallback navigation routine
#define LEVELFLAG_IMUSTNTRUNAWAY			4 //don't be scared

#define WP_KEEP_FLAG_DIST			128

#define BWEAPONRANGE_MELEE			1
#define BWEAPONRANGE_MID			2
#define BWEAPONRANGE_LONG			3
#define BWEAPONRANGE_SABER			4

#define MELEE_ATTACK_RANGE			256
#define SABER_ATTACK_RANGE			128
#define MAX_CHICKENWUSS_TIME		10000 //wait 10 secs between checking which run-away path to take

#define BOT_RUN_HEALTH				40
#define BOT_WPTOUCH_DISTANCE		32

//[TABBot]
//[BotDefines]
//Distance at which a bot knows it touched the weapon/spawnpoint it was traveling to
#define	BOT_WEAPTOUCH_DISTANCE		10

//MiscBotFlags Defines - Used by tabbots 
#define	BOTFLAG_REACHEDCAPTUREPOINT		0x00000001
//indicates that the bot has visited the capture point.  This is used by the bot AI to
//know when to engage the "waiting for flag to return" behavior.  This flag is set when the
//bot initially reaches the capture point and it's team flag isn't there.

#define BOTFLAG_KNEELINGBEFOREZOD		0x00000002
//flag to indicate that we're kneeling for the "kneel before zod" command.

#define BOTFLAG_SABERCHALLENGED			0x00000004
//flag to indicate we're issued a saber challenge as part of the BOTORDER_SABERDUELCHALLENGE tactic.

//[/BotDefines]
//[/TABBot]

#define ENEMY_FORGET_MS				10000
//if our enemy isn't visible within 10000ms (aprx 10sec) then "forget" about him and treat him like every other threat, but still look for
//more immediate threats while main enemy is not visible

#define BOT_PLANT_DISTANCE			256 //plant if within this radius from the last spotted enemy position
#define BOT_PLANT_INTERVAL			15000 //only plant once per 15 seconds at max
#define BOT_PLANT_BLOW_DISTANCE		256 //blow det packs if enemy is within this radius and I am further away than the enemy

#define BOT_MAX_WEAPON_GATHER_TIME	1000 //spend a max of 1 second after spawn issuing orders to gather weapons before attacking enemy base
#define BOT_MAX_WEAPON_CHASE_TIME	15000 //time to spend gathering the weapon before persuing the enemy base (in case it takes longer than expected)

#define BOT_MAX_WEAPON_CHASE_CTF	5000 //time to spend gathering the weapon before persuing the enemy base (in case it takes longer than expected) [ctf-only]

#define BOT_MIN_SIEGE_GOAL_SHOOT		1024
#define BOT_MIN_SIEGE_GOAL_TRAVEL	128

#define BASE_GUARD_DISTANCE			256 //guarding the flag
#define BASE_FLAGWAIT_DISTANCE		256 //has the enemy flag and is waiting in his own base for his flag to be returned
#define BASE_GETENEMYFLAG_DISTANCE	256 //waiting around to get the enemy's flag

#define BOT_FLAG_GET_DISTANCE		256

#define BOT_SABER_THROW_RANGE		800

//[TABBot]
//moved to ai_main.h so we could seperate out the tab code into ai_tab.c
#define BOT_THINK_TIME	1000/bot_fps.integer

typedef int bot_route_t[MAX_WPARRAY_SIZE];
//[/TABBot]

typedef enum
{
	CTFSTATE_NONE,
	CTFSTATE_ATTACKER,
	CTFSTATE_DEFENDER,
	CTFSTATE_RETRIEVAL,
	CTFSTATE_GUARDCARRIER,
	CTFSTATE_GETFLAGHOME,
	CTFSTATE_MAXCTFSTATES
} bot_ctf_state_t;

typedef enum
{
	SIEGESTATE_NONE,
	SIEGESTATE_ATTACKER,
	SIEGESTATE_DEFENDER,
	SIEGESTATE_MAXSIEGESTATES
} bot_siege_state_t;

typedef enum
{
	TEAMPLAYSTATE_NONE,
	TEAMPLAYSTATE_FOLLOWING,
	TEAMPLAYSTATE_ASSISTING,
	TEAMPLAYSTATE_REGROUP,
	TEAMPLAYSTATE_MAXTPSTATES
} bot_teamplay_state_t;

typedef struct botattachment_s
{
	int level;
	char name[MAX_ATTACHMENT_NAME];
} botattachment_t;

typedef struct nodeobject_s
{
	vec3_t origin;
//	int index;
	float weight;
	int flags;
#ifdef _XBOX
	short	neighbornum;
	short	inuse;
#else
	int neighbornum;
	int inuse;
#endif
} nodeobject_t;

typedef struct boteventtracker_s
{
	int			eventSequence;
	int			events[MAX_PS_EVENTS];
	float		eventTime;
} boteventtracker_t;

typedef struct botskills_s
{
	int					reflex;
	float				accuracy;
	float				turnspeed;
	float				turnspeed_combat;
	float				maxturn;
	int					perfectaim;
} botskills_t;

//bot state
typedef struct bot_state_s
{
	int inuse;										//true if this state is used by a bot client
	int botthink_residual;							//residual for the bot thinks
	int client;										//client number of the bot
	int entitynum;									//entity number of the bot
	playerState_t cur_ps;							//current player state
	usercmd_t lastucmd;								//usercmd from last frame
	bot_settings_t settings;						//several bot settings
	float thinktime;								//time the bot thinks this frame
	vec3_t origin;									//origin of the bot
	vec3_t velocity;								//velocity of the bot
	vec3_t eye;										//eye coordinates of the bot
	int setupcount;									//true when the bot has just been setup
	float ltime;									//local bot time
	float entergame_time;							//time the bot entered the game
	int ms;											//move state of the bot
	int gs;											//goal state of the bot
	int ws;											//weapon state of the bot
	vec3_t viewangles;								//current view angles
	vec3_t ideal_viewangles;						//ideal view angles
	vec3_t viewanglespeed;

	//rww - new AI values
	gentity_t			*currentEnemy;
	gentity_t			*revengeEnemy;

	gentity_t			*squadLeader;

	gentity_t			*lastHurt;
	gentity_t			*lastAttacked;

	gentity_t			*wantFlag;

	gentity_t			*touchGoal;
	gentity_t			*shootGoal;

	//RACC - closest nasty object that can take damage probably an explosive or something.
	gentity_t			*dangerousObject;

	vec3_t				staticFlagSpot;

	int					revengeHateLevel;
	int					isSquadLeader;

	int					squadRegroupInterval;
	int					squadCannotLead;

	int					lastDeadTime;

	wpobject_t			*wpCurrent;
	wpobject_t			*wpDestination;

	//RACC - storage location for your next way point while you're evading a bad thing.
	wpobject_t			*wpStoreDest;
	vec3_t				goalAngles;
	vec3_t				goalMovedir;
	vec3_t				goalPosition;

	//[TABBot]
	//viewangles of enemy when you last saw him.
	vec3_t				lastEnemyAngles;

	int					MiscBotFlags;		//misc flags used for TABBot behavior.
	int					miscBotFlagsTimer;  //this timer is used for a variety of tactic based
											//debouncers, it's usage is based on the currentTactic
											//and which miscFlagsTimers set.
											//Kneel before zod uses this for debouncing the kneel
											//The saber duel challenge behavior uses it to
											//debounce the saber challenges.
	//[/TABBot]

	//RACC - location of current enemy
	vec3_t				lastEnemySpotted;
	//RACC - Where bot was standing when lastEnemySpotted was done
	vec3_t				hereWhenSpotted;
	//RACC - This is the EntityNum of the last enemy you did a successful visual check on
	//This is normally the same as current enemy.
	int					lastVisibleEnemyIndex;
	//RACC - Not used anymore.
	int					hitSpotted;

	int					wpDirection;

	float				destinationGrabTime;
	//RACC - Clock time at which point we will give up on trying to get to the next waypoint.
	//This is normally constantly refreshed unless you loss sight of the waypoint.
	float				wpSeenTime;
	//RACC - Clock time of the maximum time we're willing to spend traveling to this wp.
	float				wpTravelTime;
	float				wpDestSwitchTime;
	float				wpSwitchTime;
	float				wpDestIgnoreTime;

	//RACC - Time to react to a newly found enemy.
	float				timeToReact;

	float				enemySeenTime;

	float				chickenWussCalculationTime;

	//RACC - Stand still until this time
	float				beStill;
	//RACC - Duck until this clock time.
	float				duckTime;
	//RACC - Jumping Clock!  = jumpHoldTime if it's valid.
	float				jumpTime;
	//RACC - Move forward while jumping.  Don't move forward if the wpcurrent is still too
	//high.
	float				jumpHoldTime;
	float				jumpPrep;
	//RACC - Force Jumping to waypoint
	float				forceJumping;
	float				jDelay;

	float				aimOffsetTime;
	float				aimOffsetAmtYaw;
	float				aimOffsetAmtPitch;

	//RACC - Distance to the waypoint you're currently travelling to.
	float				frame_Waypoint_Len;
	//RACC - We can visually see the next waypoint.  Note, players and such don't count.
	int					frame_Waypoint_Vis;
	//RACC - Distance to your current enemy.
	float				frame_Enemy_Len;
	int					frame_Enemy_Vis;

	int					isCamper;
	float				isCamping;

	//RACC - the wp you're currently trying to camp.
	wpobject_t			*wpCamping;
	wpobject_t			*wpCampingTo;
	qboolean			campStanding;

	int					randomNavTime;
	int					randomNav;

	int					saberSpecialist;

	int					canChat;
	int					chatFrequency;
	char				currentChat[MAX_CHAT_LINE_SIZE];
	float				chatTime;
	float				chatTime_stored;
	int					doChat;
	int					chatTeam;
	gentity_t			*chatObject;
	gentity_t			*chatAltObject;

	float				meleeStrafeTime;
	int					meleeStrafeDir;
	//RACC - Clock debounce for the disabling of the combat strafing.
	float				meleeStrafeDisable;

	int					altChargeTime;

	float				escapeDirTime;

	//RACC - timer (clock time) to prevent you from charging back into a dangerous area 
	//after you evaded a bad thing.
	float				dontGoBack;

	int					doAttack;
	int					doAltAttack;

	int					forceWeaponSelect;
	//RACC - in the process of switching to this weapon.
	int					virtualWeapon;

	//RACC - Clock debounce for planting explosives
	int					plantTime;
	//RACC - Try to plant explosives for this amount of clock time.  This is cleared when the 
	//explosive is set.
	int					plantDecided;
	//RACC - Time that the last explosive was set (plus 500 ms).  
	//Also used as a debounce for holding down the 
	//attack button to make sure the explosive is planted AND
	//cancelling out forceweaponselect.
	int					plantContinue;
	//RACC - Clock timer to try to blow your detpack.  Orders bot to switch to detpack weapon and hold
	//alt fire.  Lasts until the clock timer is over.
	int					plantKillEmAll;

	int					runningLikeASissy;
	int					runningToEscapeThreat;

	//char				chatBuffer[MAX_CHAT_BUFFER_SIZE];
	//Since we're once again not allocating bot structs dynamically,
	//shoving a 64k chat buffer into one is a bad thing.

	botskills_t			skills;

	botattachment_t		loved[MAX_LOVED_ONES];
	int					lovednum;

	int					loved_death_thresh;

	int					deathActivitiesDone;

	float				botWeaponWeights[WP_NUM_WEAPONS];

	int					ctfState;

	int					siegeState;

	int					teamplayState;

	int					jmState;

	int					state_Forced; //set by player ordering menu

	//RACC - Defence mode toggle (toggles on/off randomly).  All it really does is 
	//have the bot randomly not attack when close to an enemy and who is using a blockable
	//weapon.
	int					saberDefending;
	//RACC - Time between saberDefending toggling. 
	int					saberDefendDecideTime;
	int					saberBFTime;
	int					saberBTime;
	int					saberSTime;
	int					saberThrowTime;

	qboolean			saberPower;
	int					saberPowerTime;

	//RACC - Bot trying to do a Saber Challenge during this Clock time. 
	int					botChallengingTime;

	char				forceinfo[MAX_FORCE_INFO_SIZE];

#ifndef FORCEJUMP_INSTANTMETHOD
	int					forceJumpChargeTime;
#endif

	int					doForcePush;

	int					noUseTime;
	qboolean			doingFallback;

	int					iHaveNoIdeaWhereIAmGoing;
	vec3_t				lastSignificantAreaChange;
	int					lastSignificantChangeTime;

	int					forceMove_Forward;
	int					forceMove_Right;
	int					forceMove_Up;
	//end rww

	//[TABBot]
	//bot's wp route path
	bot_route_t			botRoute;

	//Order level stuff
	//bot's current order/behavior
	int					botOrder;
	//bot orderer's clientNum
	int					ordererNum;
	//order's relivent entity
	gentity_t			*orderEntity;
	//order siege objective
	int					orderObjective;
	

	//Tactical Level
	int					currentTactic;
	gentity_t			*tacticEntity;
	//objective number
	int					tacticObjective;
	//objective type
	int					objectiveType;

	//Visual scan behavior
	qboolean			doVisualScan;
	int					VisualScanTime;
	vec3_t				VisualScanDir;

	//current bot behavior
	int					botBehave;

	//evade direction
	qboolean			evadeTime;
	int					evadeDir;

	//Walk flag
	qboolean			doWalk;

	vec3_t				DestPosition;

	//Used to prevent a whole much of destination checks when moving around
	vec3_t				lastDestPosition;

	//performing some sort of special action (destroying breakables, pushing switches, etc)
	//Don't try to override this when this is occurring.
	qboolean			wpSpecial;

	//we've touched our final destination waypoint and am now moving towards our destposition.
	qboolean			wpTouchedDest;

	//Do Jump Flag for the TAB Bot
	qboolean			doJump;

	//do Force Pull for this amount of time
	int					doForcePull;

	//position we were at when we first decided to go to this waypoint
	vec3_t				wpCurrentLoc;

	//This debounces the push pull to prevent the bots from push/pulling stuff for navigation
	//purposes
	int					DontSpamPushPull;

	//debouncer for button presses, since this doesn't reset with wp point changes, be 
	//careful not to set this too high
	int					DontSpamButton;

	//have you checked for an alternate route?
	qboolean			AltRouteCheck;

	//entity number you ignore for move traces.
	int					DestIgnore;

	//hold down the Use Button.
	int					useTime;

	//Debouncer for vchats to prevent the bots from spamming the hell out of them.
	int					vchatTime;
	//[/TABBot]

	//[BotTweaks]
	//debouncer for the saberlock button presses.  So you can boost the bot fps without
	//problems.
	int					saberLockDebounce;
	//[/BotTweaks]

	//[AotCTCAI]
	int					walkTime;		//timer for walking
	int					full_thinktime;	//debouncer for full AI processing.
	int					BOTjumpState;	//bot jump state
	//[AotCTCAI]

	//[TABBots]
	int					PathFindDebounce; //debouncer for A* path finding checks.

	//debouncer for the more CPU intensive higher level thinking.  No point in checking to see if
	//we want more weapons/ammo every 1/20 of a second
	int					highThinkTime; 
	//[/TABBots]

} bot_state_t;

void *B_TempAlloc(int size);
void B_TempFree(int size);

void *B_Alloc(int size);
void B_Free(void *ptr);

//resets the whole bot state
void BotResetState(bot_state_t *bs);
//returns the number of bots in the game
int NumBots(void);

void BotUtilizePersonality(bot_state_t *bs);
int BotDoChat(bot_state_t *bs, char *section, int always);
void StandardBotAI(bot_state_t *bs, float thinktime);
void BotWaypointRender(void);
int OrgVisibleBox(vec3_t org1, vec3_t mins, vec3_t maxs, vec3_t org2, int ignore);
int BotIsAChickenWuss(bot_state_t *bs);
int GetNearestVisibleWP(vec3_t org, int ignore);
int GetBestIdleGoal(bot_state_t *bs);

//[TABBot]
//ai_tab.c
void TAB_StandardBotAI(bot_state_t *bs, float thinktime);
//[/TABBot]

char *ConcatArgs( int start );

extern vmCvar_t bot_forcepowers;
extern vmCvar_t bot_forgimmick;
extern vmCvar_t bot_honorableduelacceptance;
#ifdef _DEBUG
extern vmCvar_t bot_nogoals;
extern vmCvar_t bot_debugmessages;
#endif

extern vmCvar_t bot_attachments;
extern vmCvar_t bot_camp;

extern vmCvar_t bot_wp_info;
extern vmCvar_t bot_wp_edit;
extern vmCvar_t bot_wp_clearweight;
extern vmCvar_t bot_wp_distconnect;
extern vmCvar_t bot_wp_visconnect;

extern wpobject_t *flagRed;
extern wpobject_t *oFlagRed;
extern wpobject_t *flagBlue;
extern wpobject_t *oFlagBlue;

extern gentity_t *eFlagRed;
extern gentity_t *eFlagBlue;

extern char gBotChatBuffer[MAX_CLIENTS][MAX_CHAT_BUFFER_SIZE];
extern float gWPRenderTime;
extern float gDeactivated;
extern float gBotEdit;
extern int gWPRenderedFrame;

#include "../namespace_begin.h"
extern wpobject_t *gWPArray[MAX_WPARRAY_SIZE];
extern int gWPNum;
#include "../namespace_end.h"

extern int gLastPrintedIndex;
#ifndef _XBOX
extern nodeobject_t nodetable[MAX_NODETABLE_SIZE];
#endif
extern int nodenum;

extern int gLevelFlags;

extern float floattime;
#define FloatTime() floattime


//[TABBots]
//TAB bot Behaviors
//[Linux]
#ifndef __linux__
typedef enum
#else
enum
#endif
//[/Linux]
{
	BBEHAVE_NONE,  //use this if you don't want a behavior function to be run
	BBEHAVE_STILL, //bot stands still
	BBEHAVE_MOVETO, //Move to the current inputted goalPosition;
	BBEHAVE_ATTACK,  //Attack given entity
	BBEHAVE_VISUALSCAN	//visually scanning around
};
//[/TABBots]



