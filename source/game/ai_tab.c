//[TABBot]
//includes
#include "g_local.h"
#include "ai_main.h"

#include "bg_local.h" // needed for MIN_WALK_NORMAL define

//externs
extern bot_state_t	*botstates[MAX_CLIENTS];

extern vmCvar_t bot_fps;

extern int BotSelectChoiceWeapon(bot_state_t *bs, int weapon, int doselection);
extern int CheckForFunc(vec3_t org, int ignore);
extern float forceJumpStrength[NUM_FORCE_POWER_LEVELS];
extern int BotMindTricked(int botClient, int enemyClient);
extern int BotCanHear(bot_state_t *bs, gentity_t *en, float endist);
extern qboolean BotPVSCheck( const vec3_t p1, const vec3_t p2 );
extern void BotDeathNotify(bot_state_t *bs);
extern void BotReplyGreetings(bot_state_t *bs);
extern void MoveTowardIdealAngles(bot_state_t *bs);
extern int forcePowerNeeded[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS];
extern int ObjectiveDependancy[MAX_OBJECTIVES][MAX_OBJECTIVEDEPENDANCY];
extern qboolean gSiegeRoundEnded;
qboolean G_PointInBounds( vec3_t point, vec3_t mins, vec3_t maxs );
qboolean G_NameInTriggerClassList(char *list, char *str);
qboolean G_HeavyMelee( gentity_t *attacker );
extern float BotWeaponCanLead(bot_state_t *bs);
extern gentity_t *FindClosestHumanPlayer(vec3_t position, int enemyTeam);
extern qboolean PM_SaberInBounce( int move );
extern qboolean PM_SaberInReturn( int move );
extern gitem_t	*BG_FindItemForAmmo( ammo_t ammo );
extern siegeClass_t *BG_GetClassOnBaseClass(const int team, const short classIndex, const short cntIndex);
extern qboolean InFOV2( vec3_t origin, gentity_t *from, int hFOV, int vFOV );

//inits
int FavoriteWeapon(bot_state_t *bs, gentity_t *target, qboolean haveCheck, 
				   qboolean ammoCheck, int ignoreWeaps);
void ResetWPTimers(bot_state_t *bs);
void TAB_AdjustforStrafe(bot_state_t *bs, vec3_t moveDir);
void TAB_BotObjective(bot_state_t *bs);
void FindAngles(gentity_t *ent, vec3_t angles);
void FindOrigin(gentity_t *ent, vec3_t origin);
gentity_t *GetObjectThatTargets(gentity_t *ent);
void TAB_BotBehave_Attack(bot_state_t *bs);
void TAB_BotBehave_AttackBasic(bot_state_t *bs, gentity_t* target);
float TargetDistance(bot_state_t *bs, gentity_t* target, vec3_t targetorigin);
int TAB_GetNearestVisibleWP(bot_state_t *bs, vec3_t org, int ignore, int badwp);
qboolean AttackLocalBreakable(bot_state_t *bs, vec3_t origin);
void TAB_BotBehave_DefendBasic(bot_state_t *bs, vec3_t defpoint);
void ShouldSwitchSiegeClasses(bot_state_t *bs, qboolean saber);
int NumberofSiegeBasicClass(int team, int BaseClass);
void RequestSiegeAssistance(bot_state_t *bs, int BaseClass);
qboolean SwitchSiegeIdealClass(bot_state_t *bs, char *idealclass);
int NumberofSiegeSpecificClass(int team, const char *classname);
int BotWeapon_Detpack(bot_state_t *bs, gentity_t *target);
qboolean DontBlockAllies(bot_state_t *bs);
void TAB_BotAimLeading(bot_state_t *bs, vec3_t headlevel, float leadAmount);
qboolean TAB_ForcePowersThink(bot_state_t *bs);
qboolean CarryingCapObjective(bot_state_t *bs);
void TAB_BotJediMaster(bot_state_t *bs);
void TAB_BotSaberDuel(bot_state_t *bs);
qboolean TryMoveAroundObsticle( bot_state_t *bs, vec3_t moveDir, int targetNum, 
							   vec3_t hitNormal, int tryNum, qboolean CheckBothWays );
void TAB_BotResupply( bot_state_t *bs, gentity_t* tacticEnt );
gentity_t* TAB_WantWeapon(bot_state_t *bs, qboolean setOrder, int numOfChecks);
gentity_t* TAB_WantAmmo(bot_state_t *bs, qboolean setOrder, int numOfChecks);
int OJP_PassStandardEnemyChecks(bot_state_t *bs, gentity_t *en);

//[TABBotDefines]
//Maximum distance allowed between nodes for them to count as seqencial wp.
#define MAX_NODE_DIS 1000

//distance at which the bots consider themselves to be touching their destination point
//This is used by the basic move code to prevent them from spazing while trying to move
//to a location.  This is measured in horizontal distance so the bots won't spaz while
//waiting for an elevator to go up.
#define TAB_NAVTOUCH_DISTANCE 10

//Sets the distance at which we're sure to have captured an objective object. 
//If we haven't captured at this point, it means that our team's flag isn't at our base.
//activate our waiting for flag to return behavior.
#define BOTAI_CAPTUREDISTANCE 10

//distance at which the bot will move out of the way of an ally.
#define BLOCKALLIESDISTANCE 50

//targets must be inside this distance for the bot to attempt to blow up det packs.
#define DETPACK_DETDISTANCE 500

//bots will stay within this distance to their defend targets while fighting enemies
#define	DEFEND_MAXDISTANCE	500

//if noone enemies are in the area, the bot will stay this close to their defend target
#define DEFEND_MINDISTANCE	200


int MinimumAttackDistance[WP_NUM_WEAPONS] = 
{
	0, //WP_NONE,
	0, //WP_STUN_BATON,
	30, //WP_MELEE,
	60, //WP_SABER,
	200, //WP_BRYAR_PISTOL,
	0, //WP_BLASTER,
	0, //WP_DISRUPTOR,
	0, //WP_BOWCASTER,
	0, //WP_REPEATER,
	0, //WP_DEMP2,
	0, //WP_FLECHETTE,
	100, //WP_ROCKET_LAUNCHER,
	100, //WP_THERMAL,
	100, //WP_TRIP_MINE,
	0, //WP_DET_PACK,
	0, //WP_CONCUSSION,
	0, //WP_BRYAR_OLD,
	0, //WP_EMPLACED_GUN,
	0 //WP_TURRET,
	//WP_NUM_WEAPONS
};


int MaximumAttackDistance[WP_NUM_WEAPONS] = 
{
	9999, //WP_NONE,
	0, //WP_STUN_BATON,
	100, //WP_MELEE,
	160, //WP_SABER,
	9999, //WP_BRYAR_PISTOL,
	9999, //WP_BLASTER,
	9999, //WP_DISRUPTOR,
	9999, //WP_BOWCASTER,
	9999, //WP_REPEATER,
	9999, //WP_DEMP2,
	9999, //WP_FLECHETTE,
	9999, //WP_ROCKET_LAUNCHER,
	9999, //WP_THERMAL,
	9999, //WP_TRIP_MINE,
	9999, //WP_DET_PACK,
	9999, //WP_CONCUSSION,
	9999, //WP_BRYAR_OLD,
	9999, //WP_EMPLACED_GUN,
	9999 //WP_TURRET,
	//WP_NUM_WEAPONS
};


int IdealAttackDistance[WP_NUM_WEAPONS] = 
{
	0, //WP_NONE,
	0, //WP_STUN_BATON,
	40, //WP_MELEE,
	80, //WP_SABER,
	350, //WP_BRYAR_PISTOL,
	1000, //WP_BLASTER,
	1000, //WP_DISRUPTOR,
	1000, //WP_BOWCASTER,
	1000, //WP_REPEATER,
	1000, //WP_DEMP2,
	1000, //WP_FLECHETTE,
	1000, //WP_ROCKET_LAUNCHER,
	1000, //WP_THERMAL,
	1000, //WP_TRIP_MINE,
	1000, //WP_DET_PACK,
	1000, //WP_CONCUSSION,
	1000, //WP_BRYAR_OLD,
	1000, //WP_EMPLACED_GUN,
	1000 //WP_TURRET,
	//WP_NUM_WEAPONS
};

//used by track attac to prevent the bot from thinking you've disappeared the second it loses
//visual/hearing on you (like if you hid behind it).  It will continue to know where you 
//are during this time.
#define BOT_VISUALLOSETRACKTIME	2000

//the amount of Force Power you'll need for a given Force Jump level
int ForcePowerforJump[NUM_FORCE_POWER_LEVELS] =
{	
	0,
	30,
	30,
	30
};

#define TAB_BOTMOVETRACE_NORMALDIST	20  //the distance of the movement traces (for object avoidance) for bots.
#define TAB_BOTMOVETRACE_FALLDIST	45	//The distance ahead of the player where the fall checks are done.
#define TAB_BOTMOVETRACE_HOPHEIGHT	20  //the height above the players where the hop traces are done.

//RAFIXME - this should probably be handled by a new bot attribute.
#define DESIREDAMMOLEVEL .5

//Defines the top list of weapons that we care about getting or having ammo for.
//IE the top 5, the top 10, etc...
#define TAB_FAVWEAPCARELEVEL_MAX 5	


//Defines the top list of weapons that we care about getting or having ammo for
//while not immediately caring about it. 
//IE, the point at which we'll abandon going after our kill target, the flag, etc.
//IE the top 5, the top 10, etc...
#define TAB_FAVWEAPCARELEVEL_INTERRUPT 1

//The debounce time for most of the higher level thinking (like the desire to get weapons/ammo).
#define TAB_HIGHTHINKDEBOUNCE	5000

//[/TABBotDefines]

//Returns the Force level needed to make this jump
//FORCE_LEVEL_4 (4) = Jump too high!
int ForceJumpNeeded(vec3_t startvect, vec3_t endvect)
{
	float heightdif, lengthdif; 
	vec3_t tempstart, tempend;

	heightdif = endvect[2] - startvect[2];
	
	VectorCopy(startvect, tempstart);
	VectorCopy(endvect, tempend);

	tempend[2] = tempstart[2] = 0;
	lengthdif = Distance(tempstart, tempend);

	if (heightdif < 128 && lengthdif < 128)
	{ //no force needed
		return FORCE_LEVEL_0;
	}

	if (heightdif > 512)
	{ //too high
		return FORCE_LEVEL_4;
	}
	if (heightdif > 350 || lengthdif > 700)
	{
		return FORCE_LEVEL_3;
	}
	else if (heightdif > 256 || lengthdif > 512)
	{
		return FORCE_LEVEL_2;
	}
	else
	{
		return FORCE_LEVEL_1;
	}
}
	


//just like GetNearestVisibleWP except without visiblity checks
int GetNearestWP(bot_state_t *bs, vec3_t org, int badwp)
{
	int i;
	float bestdist;
	float flLen;
	int bestindex;
	vec3_t a;

	if (g_RMG.integer)
	{
		bestdist = 300;
	}
	else
	{
		//We're not doing traces!
		bestdist = 99999;

	}
	bestindex = -1;

	for(i=0; i < gWPNum; i++)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && i != badwp)
		{
			if(bs)
			{//check to make sure that this bot's team can use this waypoint
				if(gWPArray[i]->flags & WPFLAG_REDONLY 
					&& g_entities[bs->client].client->sess.sessionTeam != TEAM_RED)
				{//red only wp, can't use
					continue;
				}

				if(gWPArray[i]->flags & WPFLAG_BLUEONLY 
					&& g_entities[bs->client].client->sess.sessionTeam != TEAM_BLUE)
				{//blue only wp, can't use
					continue;
				}
			}

			VectorSubtract(org, gWPArray[i]->origin, a);
			flLen = VectorLength(a);

			if(gWPArray[i]->flags & WPFLAG_WAITFORFUNC 
				|| (gWPArray[i]->flags & WPFLAG_NOMOVEFUNC)
				|| (gWPArray[i]->flags & WPFLAG_DESTROY_FUNCBREAK)
				|| (gWPArray[i]->flags & WPFLAG_FORCEPUSH)
				|| (gWPArray[i]->flags & WPFLAG_FORCEPULL) )
			{//boost the distance for these waypoints so that we will try to avoid using them
				//if at all possible
				flLen = flLen + 500;
			}

			if (flLen < bestdist)
			{
				bestdist = flLen;
				bestindex = i;
			}
		}
	}

	return bestindex;
}


/*
=========================
START A* Pathfinding Code
=========================
*/



typedef struct nodeWaypoint_s 
{
	int wpNum;
	float f;
	float g;
	float h;
	int pNum;
} nodeWaypoint_t;


//Don't use the 0 slot of this array.  It's a binary heap and it's based on 1 being the first slot.
nodeWaypoint_t OpenList[MAX_WPARRAY_SIZE+1];

nodeWaypoint_t CloseList[MAX_WPARRAY_SIZE];



qboolean OpenListEmpty(void)
{
	//since we're using a binary heap, in theory, if the first slot is empty, the heap
	//is empty.
	if(OpenList[1].wpNum != -1)
	{
		return qfalse;
	}

	return qtrue;
	/*
	int i;
	for( i = 1; i < MAX_WPARRAY_SIZE+1; i++ )
	{
		if( OpenList[i].wpNum != -1 )
		{
			return qfalse;
		}
	}
	return qtrue;
	*/
}


//Scans for the given wp on the Open List and returns it's OpenList position.  
//Returns -1 if not found.
int FindOpenList(int wpNum)
{
	int i;
	for( i = 1; i < MAX_WPARRAY_SIZE+1 && OpenList[i].wpNum != -1; i++ )
	{
		if( OpenList[i].wpNum == wpNum )
		{
			return i;
		}
	}
	return -1;
}


//Scans for the given wp on the Close List and returns it's CloseList position.  
//Returns -1 if not found.
int FindCloseList(int wpNum)
{
	int i;
	for( i = 0; i < MAX_WPARRAY_SIZE && CloseList[i].wpNum != -1; i++ )
	{
		if( CloseList[i].wpNum == wpNum )
		{
			return i;
		}
	}
	return -1;
}


extern float RandFloat(float min, float max);
GAME_INLINE float RouteRandomize(bot_state_t *bs, float DestDist)
{//this function randomizes the h value (distance to target location) to make the 
	//bots take a random path instead of always taking the shortest route.
	//This should vary based on situation to prevent the bots from taking weird routes
	//for inapproprate situations. 
	if(bs->currentTactic == BOTORDER_OBJECTIVE 
		&& bs->objectiveType == OT_CAPTURE
		&& !CarryingCapObjective(bs))
	{//trying to capture something.  Fairly random paths to mix up the defending team.
		return DestDist * RandFloat(.5, 1.5);
	}

	//return shortest distance.
	return DestDist;
}


//Add this wpNum to the open list.
void AddOpenList(bot_state_t *bs, int wpNum, int parent, int end)
{
	int i;
	float f;
	float g;
	float h;

	if(gWPArray[wpNum]->flags & WPFLAG_REDONLY 
		&& g_entities[bs->client].client->sess.sessionTeam != TEAM_RED)
	{//red only wp, can't use
		return;
	}

	if(gWPArray[wpNum]->flags & WPFLAG_BLUEONLY 
		&& g_entities[bs->client].client->sess.sessionTeam != TEAM_BLUE)
	{//blue only wp, can't use
		return;
	}

	if(parent != -1 && gWPArray[wpNum]->flags & WPFLAG_JUMP)
	{
		if(ForceJumpNeeded(gWPArray[parent]->origin, gWPArray[wpNum]->origin) > bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION])
		{//can't make this jump with our level of Force Jump
			return;
		}
	}
		

	if(parent == -1)
	{//This is the start point, just use zero for g
		g = 0;
	}
	else if(wpNum == parent + 1)
	{
		if(gWPArray[wpNum]->flags & WPFLAG_ONEWAY_BACK)
		{//can't go down this one way
			return;
		}
		i = FindCloseList(parent);
		if( i != -1)
		{
			g = CloseList[i].g + gWPArray[parent]->disttonext;
		}
		else
		{
			G_Printf("Couldn't find parent on CloseList in AddOpenList().  This shouldn't happen.\n");
			return;
		}
	}
	else if(wpNum == parent - 1)
	{
		if(gWPArray[wpNum]->flags & WPFLAG_ONEWAY_FWD)
		{//can't go down this one way
			return;
		}
		i = FindCloseList(parent);
		if( i != -1)
		{
			g = CloseList[i].g + gWPArray[wpNum]->disttonext;
		}
		else
		{
			G_Printf("Couldn't find parent on CloseList in AddOpenList().  This shouldn't happen.\n");
			return;
		}
	}
	else
	{//nonsequencal parent/wpNum
		//don't try to go thru oneways when you're doing neighbor moves
		if((gWPArray[wpNum]->flags & WPFLAG_ONEWAY_FWD) || (gWPArray[wpNum]->flags & WPFLAG_ONEWAY_BACK))
		{
			return;
		}

		i = FindCloseList(parent);
		if( i != -1)
		{
			g = OpenList[i].g + Distance( gWPArray[wpNum]->origin, gWPArray[parent]->origin );
		}
		else
		{
			G_Printf("Couldn't find parent on CloseList in AddOpenList().  This shouldn't happen.\n");
			return;
		}
	}

	//RAFIXME - Do some fancy force jump checking here...
	
	//Find first open slot or the slot that this wpNum already occupies.
	for( i = 1; i < MAX_WPARRAY_SIZE+1 && OpenList[i].wpNum != -1; i++ )
	{
		if(OpenList[i].wpNum == wpNum)
		{
			break;
		}
	}

	h = Distance(gWPArray[wpNum]->origin, gWPArray[end]->origin);

	//add a bit of a random factor to h to make the bots take a variety of paths.
	h = RouteRandomize(bs, h);

	f = g + h;
	OpenList[i].f = f;
	OpenList[i].g = g;
	OpenList[i].h = h;
	OpenList[i].pNum = parent;
	OpenList[i].wpNum = wpNum;

	while( OpenList[i].f <= OpenList[i/2].f && i != 1)
	{//binary parent has higher f than child
		float ftemp = OpenList[i/2].f;
		float gtemp = OpenList[i/2].g;
		float htemp = OpenList[i/2].h;
		int pNumtemp = OpenList[i/2].pNum;
		int wptemp = OpenList[i/2].wpNum;

		OpenList[i/2].f = OpenList[i].f;
		OpenList[i/2].g = OpenList[i].g;
		OpenList[i/2].h = OpenList[i].h;
		OpenList[i/2].pNum = OpenList[i].pNum;
		OpenList[i/2].wpNum = OpenList[i].wpNum;

		OpenList[i].f = ftemp;
		OpenList[i].g = gtemp;
		OpenList[i].h = htemp;
		OpenList[i].pNum = pNumtemp;
		OpenList[i].wpNum = wptemp;

		i = i/2;
	}
	return;
}


//Remove the first element from the OpenList.
void RemoveFirstOpenList( void )
{
	int i = 0;
	for( i = 1; i < MAX_WPARRAY_SIZE+1 && OpenList[i].wpNum != -1; i++ )
	{
	}

	i = i--;
	if(OpenList[i].wpNum == -1)
	{
		G_Printf("Crap!  The last entry scanner in RemoveFirstOpenList() totally failed.\n");
	}

	if(OpenList[1].wpNum == OpenList[i].wpNum)
	{//the first slot is the only thing on the list. blank it.
		OpenList[1].f = -1;
		OpenList[1].g = -1;
		OpenList[1].h = -1;
		OpenList[1].pNum = -1;
		OpenList[1].wpNum = -1;
		return;
	}

	//shift last entry to start
	OpenList[1].f = OpenList[i].f;
	OpenList[1].g = OpenList[i].g;
	OpenList[1].h = OpenList[i].h;
	OpenList[1].pNum = OpenList[i].pNum;
	OpenList[1].wpNum = OpenList[i].wpNum;

	OpenList[i].f = -1;
	OpenList[i].g = -1;
	OpenList[i].h = -1;
	OpenList[i].pNum = -1;
	OpenList[i].wpNum = -1;

	while( (OpenList[i].f >= OpenList[i*2].f && OpenList[i*2].wpNum != -1)
		|| (OpenList[i].f >= OpenList[i*2+1].f && OpenList[i*2+1].wpNum != -1) )
	{
		if((OpenList[i*2].f < OpenList[i*2+1].f) || OpenList[i*2+1].wpNum == -1 )
		{
			float ftemp = OpenList[i*2].f;
			float gtemp = OpenList[i*2].g;
			float htemp = OpenList[i*2].h;
			int pNumtemp = OpenList[i*2].pNum;
			int wptemp = OpenList[i*2].wpNum;

			OpenList[i*2].f = OpenList[i].f;
			OpenList[i*2].g = OpenList[i].g;
			OpenList[i*2].h = OpenList[i].h;
			OpenList[i*2].pNum = OpenList[i].pNum;
			OpenList[i*2].wpNum = OpenList[i].wpNum;

			OpenList[i].f = ftemp;
			OpenList[i].g = gtemp;
			OpenList[i].h = htemp;
			OpenList[i].pNum = pNumtemp;
			OpenList[i].wpNum = wptemp;

			i = i*2;
		}
		else if( OpenList[i*2+1].wpNum != -1)
		{
			float ftemp = OpenList[i*2+1].f;
			float gtemp = OpenList[i*2+1].g;
			float htemp = OpenList[i*2+1].h;
			int pNumtemp = OpenList[i*2+1].pNum;
			int wptemp = OpenList[i*2+1].wpNum;

			OpenList[i*2+1].f = OpenList[i].f;
			OpenList[i*2+1].g = OpenList[i].g;
			OpenList[i*2+1].h = OpenList[i].h;
			OpenList[i*2+1].pNum = OpenList[i].pNum;
			OpenList[i*2+1].wpNum = OpenList[i].wpNum;

			OpenList[i].f = ftemp;
			OpenList[i].g = gtemp;
			OpenList[i].h = htemp;
			OpenList[i].pNum = pNumtemp;
			OpenList[i].wpNum = wptemp;

			i = i*2+1;
		}
		else
		{
			G_Printf("Something went wrong in RemoveFirstOpenList().\n");
			return;
		}
	}
		//sorting complete
		return;
}


//Adds a given OpenList wp to the closed list
void AddCloseList( int openListpos )
{
	int i;
	if(OpenList[openListpos].wpNum != -1)
	{
		for(i = 0; i < MAX_WPARRAY_SIZE; i++)
		{
			if( CloseList[i].wpNum == -1)
			{//open slot, fill it.  heheh.
				CloseList[i].f = OpenList[openListpos].f;
				CloseList[i].g = OpenList[openListpos].g;
				CloseList[i].h = OpenList[openListpos].h;
				CloseList[i].pNum = OpenList[openListpos].pNum;
				CloseList[i].wpNum = OpenList[openListpos].wpNum;
				return;
			}
		}
		G_Printf("CloseList was full in AddCloseList().  This shouldn't happen.\n");
		return;
	}
	
	G_Printf("Bad openListpos given to AddCloseList().\n");
	return;
}


//Clear out the Route
void ClearRoute( int Route[MAX_WPARRAY_SIZE] )
{
	int i;
	for( i=0; i < MAX_WPARRAY_SIZE; i++ )
	{
		Route[i] = -1;
	}
}


void AddtoRoute( int wpNum, int Route[MAX_WPARRAY_SIZE] )
{
	int i;
	for( i=0; i < MAX_WPARRAY_SIZE && Route[i] != -1; i++ )
	{
	}

	if( Route[i] == -1 && i < MAX_WPARRAY_SIZE )
	{//found the first empty slot
		while( i > 0 )
		{
			Route[i] = Route[i-1];
			i--;
		}
	}
	else
	{
		G_Printf("Empty Route slot couldn't be found in AddtoRoute.  This shouldn't happen.\n");
		return;
	}
	if( i == 0 )
	{
		Route[0] = wpNum;
	}
}


//find a given wpNum on the given route and return it's address.  return -1 if not on route.
//use wpNum = -1 to find the last wp on route.
int FindOnRoute( int wpNum, int Route[MAX_WPARRAY_SIZE] )
{
	int i;
	for( i=0; i < MAX_WPARRAY_SIZE && Route[i] != wpNum; i++ )
	{
	}

	//Special find end route command stuff
	if(wpNum == -1)
	{
		i = i--;
		if(Route[i] != -1)
		{//found it
			return i;
		}

		//otherwise, this is a empty route list
		return -1;
	}

	if( wpNum == Route[i] )
	{//Success!
		return i;
	}

	//Couldn't find it
	return -1;
}


//Copy Route
void CopyRoute(bot_route_t routesource, bot_route_t routedest)
{
	int i;
	for( i=0; i < MAX_WPARRAY_SIZE; i++ )
	{
		routedest[i] = routesource[i];
	}
}


//Find the ideal (shortest) route between the start wp and the end wp
//badwp is for situations where you need to recalc a path when you dynamically discover
//that a wp is bad (door locked, blocked, etc).
//doRoute = actually set botRoute
float FindIdealPathtoWP(bot_state_t *bs, int start, int end, int badwp, bot_route_t Route)
{
	int i;
	float dist = -1;

	if(bs->PathFindDebounce > level.time)
	{//currently debouncing the path finding to prevent a massive overload of AI thinking
		//in weird situations, like when the target near a waypoint where the bot can't
		//get to (like in an area where you need a specific force power to get to).
		return -1;
	}

	if(start == end)
	{
		ClearRoute(Route);
		AddtoRoute(end, Route);
		return 0;
	}

	//reset node lists
	for(i = 0; i < MAX_WPARRAY_SIZE; i++)
	{
		OpenList[i].wpNum = -1;
		OpenList[i].f = -1;
		OpenList[i].g = -1;
		OpenList[i].h = -1;
		OpenList[i].pNum = -1;

	}

	for(i = 0; i < MAX_WPARRAY_SIZE; i++)
	{
		CloseList[i].wpNum = -1;
		CloseList[i].f = -1;
		CloseList[i].g = -1;
		CloseList[i].h = -1;
		CloseList[i].pNum = -1;
	}

	AddOpenList(bs, start, -1, end);

	while(!OpenListEmpty() && FindOpenList(end) == -1)
	{//open list not empty
	
		//we're using a binary pile so the first slot always has the lowest f score
		AddCloseList(1);
		i = OpenList[1].wpNum;
		RemoveFirstOpenList();

		//Add surrounding nodes
		if(gWPArray[i+1] && gWPArray[i+1]->inuse)
		{
			if(gWPArray[i]->disttonext < MAX_NODE_DIS 
				&& FindCloseList(i+1) == -1 && i+1 != badwp)
			{//Add next sequencial node
				AddOpenList(bs, i+1, i, end);
			}
		}
		
		if( i > 0)
		{
			if(gWPArray[i-1]->disttonext < MAX_NODE_DIS && gWPArray[i-1]->inuse 
				&& FindCloseList(i-1) == -1 && i-1 != badwp)
			{//Add previous sequencial node
				AddOpenList(bs, i-1, i, end);
			}
		}

		if(gWPArray[i]->neighbornum)
		{//add all the wp's neighbors to the list
			int x;
			for( x = 0; x < gWPArray[i]->neighbornum; x++ )
			{
				if(x != badwp && FindCloseList(gWPArray[i]->neighbors[x].num) == -1)
				{
					AddOpenList(bs, gWPArray[i]->neighbors[x].num, i, end);
				}
			}
		}
	}

	i = FindOpenList(end);

	if(i != -1)
	{//we have a valid route to the end point
		ClearRoute(Route);
		AddtoRoute(end, Route);
		dist = OpenList[i].g;
		i = OpenList[i].pNum;
		i = FindCloseList(i);
		while(i != -1)
		{
			AddtoRoute(CloseList[i].wpNum, Route);
			i = FindCloseList(CloseList[i].pNum);
		}
		//only have the debouncer when we fail to find a route.
		bs->PathFindDebounce = level.time;	
		return dist;
	}

	if(bot_wp_edit.integer)
	{//print error message if in edit mode.
		G_Printf("Bot waypoint %i can't get to point %i with bad waypoint %i set.\n", 
			start, end, badwp);
	}
	bs->PathFindDebounce = level.time + 3000;  //try again in 3 seconds.

	return -1;
}

/*
=========================
END A* Pathfinding Code
=========================
*/

char *OrderNames[BOTORDER_MAX] = 
{
	"none",						//BOTORDER_NONE
	"kneel before",				//BOTORDER_KNEELBEFOREZOD
	"attack",					//BOTORDER_ATTACK
	"compete objectives",		//BOTORDER_OBJECTIVE
	"play JediMaster",			//BOTORDER_JEDIMASTER
	"enter a saber duel with",	//BOTORDER_SABERDUELCHALLENGE
	/*
	"become infantry",		//BOTORDER_SIEGECLASS_INFANTRY
	"become scout",			//BOTORDER_SIEGECLASS_VANGUARD
	"become tech",			//BOTORDER_SIEGECLASS_SUPPORT
	"become Jedi",			//BOTORDER_SIEGECLASS_JEDI
	"become demolitionist",	//BOTORDER_SIEGECLASS_DEMOLITIONIST
	"become heavy weapons"	//BOTORDER_SIEGECLASS_HEAVY_WEAPONS
	*/
	//BOTORDER_MAX
};


void TAB_BotOrder( gentity_t *orderer, gentity_t *orderee, int order, gentity_t *objective)
{
	bot_state_t *bs;
	if(!orderer || !orderee || !orderer->client || !orderee->client || !(orderee->r.svFlags & SVF_BOT))
	{//Sanity check
		return;
	}

	bs = botstates[orderee->client->ps.clientNum];

	//orders with objective entities
	if(order == BOTORDER_KNEELBEFOREZOD 
		|| order == BOTORDER_SABERDUELCHALLENGE
		|| (order == BOTORDER_SEARCHANDDESTROY && objective) )
	{
		if(objective)
		{
			bs->botOrder = order;
			bs->orderEntity = objective;
			bs->ordererNum = orderer->client->ps.clientNum;
			//orders for a client objective 
			if(objective->client)
			{
				G_Printf("%s ordered %s to %s %s\n", orderer->client->pers.netname, orderee->client->pers.netname, OrderNames[order], objective->client->pers.netname);
			}
		}
		else
		{//no objective!  bad!
			return;
		}
	}
	else if ( order == BOTORDER_SEARCHANDDESTROY)
	{
		bs->botOrder = order;
		bs->orderEntity = NULL;
		bs->ordererNum = orderer->client->ps.clientNum;
		G_Printf("%s ordered %s to %s\n", orderer->client->pers.netname, orderee->client->pers.netname, OrderNames[order]);

	}
	else
	{//bad order
		return;
	}

	BotDoChat(bs, "OrderAccepted", 1);
}


//[TABBotDefines]

//BOTORDER_KNEELBEFOREZOD Defines
//Distance at which point you stop and kneel before kneel target
#define	KNEELZODDISTANCE	100

//Distance at which point you quit running towards kneel target
#define	WALKZODDISTANCE		200

//The Distance at which you can challenge someone to a saber duel.
#define SABERDUELCHALLENGEDIST 256


//Bot Order implimentation for BOTORDER_KNEELBEFOREZOD
void TAB_BotKneelBeforeZod(bot_state_t *bs)
{
	vec_t dist;

	//Sanity checks
	if(!bs->tacticEntity || !bs->tacticEntity->client)
	{//bad!
		G_Printf("Bad tacticEntity sent to TAB_BotKneelBeforeZod.\n");
		return;
	}

	dist = Distance(bs->origin, bs->tacticEntity->client->ps.origin);

	if( dist < KNEELZODDISTANCE || (bs->MiscBotFlags & BOTFLAG_KNEELINGBEFOREZOD))
	{//Close enough to hold and kneel
		vec3_t ang;
		vec3_t viewDir;

		bs->duckTime = level.time + BOT_THINK_TIME;
		bs->botBehave = BBEHAVE_STILL;	

		VectorSubtract(bs->tacticEntity->client->ps.origin, bs->eye, viewDir);
		vectoangles(viewDir, ang);
		VectorCopy(ang, bs->goalAngles);

		if(bs->cur_ps.weapon != WP_SABER && bs->virtualWeapon != WP_SABER )
		{//don't have saber selected, try switching to it
			BotSelectChoiceWeapon(bs, WP_SABER, 1);
		}
		else if(bs->cur_ps.weapon == WP_SABER)
		{
				if (!bs->cur_ps.saberHolstered)
				{
					Cmd_ToggleSaber_f(&g_entities[bs->client]);
				}
		}

		if((bs->MiscBotFlags & BOTFLAG_KNEELINGBEFOREZOD))
		{//already started kneel.
			if(bs->miscBotFlagsTimer < level.time)
			{//objective complete, clear tactic and order
				bs->currentTactic = BOTORDER_NONE;
				bs->MiscBotFlags &= ~BOTFLAG_KNEELINGBEFOREZOD;
				if(bs->botOrder == BOTORDER_KNEELBEFOREZOD)
				{
					bs->botOrder = BOTORDER_NONE;
					bs->orderEntity = NULL;
				}
			}
		}
		else
		{
			bs->MiscBotFlags |= BOTFLAG_KNEELINGBEFOREZOD;
			bs->miscBotFlagsTimer = level.time + Q_irand(3000, 8000);
		}

	}
	else
	{//Still need to move to kneel target
		if( dist < WALKZODDISTANCE )
		{//close enough to quit running like an idiot.
			bs->doWalk = qtrue;
		}

		bs->botBehave = BBEHAVE_MOVETO;
		VectorCopy(bs->tacticEntity->client->ps.origin, bs->DestPosition);
		bs->DestIgnore = bs->tacticEntity->s.number;
	}

}


//Just stand still
void TAB_BotBeStill(bot_state_t *bs)
{
	VectorCopy(bs->origin, bs->goalPosition);
	bs->beStill = level.time + BOT_THINK_TIME;
	VectorClear(bs->DestPosition);
	bs->DestIgnore = -1;
	bs->wpCurrent = NULL;
}


//TAB version of BotTrace_Jump()
//This isn't used anymore.  Use TAB_TraceJumpCrouchFall
//1 = can/should jump to clear
//0 = don't jump
int TAB_TraceJump(bot_state_t *bs, vec3_t traceto)
{
	vec3_t mins, maxs, fwd, traceto_mod, tracefrom_mod;
	trace_t tr;
	int orTr;

	VectorSubtract(traceto, bs->origin, fwd);
	

	traceto_mod[0] = bs->origin[0] + fwd[0]*4;
	traceto_mod[1] = bs->origin[1] + fwd[1]*4;
	traceto_mod[2] = bs->origin[2] + fwd[2]*4;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -18;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 32;

	trap_Trace(&tr, bs->origin, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID);

	if (tr.fraction == 1)
	{
		return 0;
	}

	orTr = tr.entityNum;

	VectorCopy(bs->origin, tracefrom_mod);

	tracefrom_mod[2] += 41;
	traceto_mod[2] += 41;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = 0;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 8;

	trap_Trace(&tr, tracefrom_mod, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID);

	if (tr.fraction == 1)
	{
		if (orTr >= 0 && orTr < MAX_CLIENTS && botstates[orTr] && botstates[orTr]->jumpTime > level.time)
		{
			return 0; //so bots don't try to jump over each other at the same time
		}

		return 1;
	}

	return 0;
}


//Performs the obsticle jump/crouch/fall traces
//moveDir must be normalized.
//0 = no obstruction/action needed
//1 = Force Jump
//2 = crouch
//-1 = cliff or no way to get there from here.
//hitNormal = the normal vector of the object blocking the path if one exists.
int TAB_TraceJumpCrouchFall(bot_state_t *bs, vec3_t moveDir, int targetNum, vec3_t hitNormal)
{
	vec3_t mins, maxs, traceto_mod, tracefrom_mod, saveNormal;
	trace_t tr;
	int contents;
	int moveCommand = -1;

	VectorClear(saveNormal);

	//set the mins/maxs for the standard obstruction checks.
	VectorCopy(g_entities[bs->client].r.maxs, maxs);
	VectorCopy(g_entities[bs->client].r.mins, mins);

	//boost up the trace box as much as we can normally step up
	mins[2] += STEPSIZE;

	//Ok, check for obstructions then.
	traceto_mod[0] = bs->origin[0] + moveDir[0]*TAB_BOTMOVETRACE_NORMALDIST;
	traceto_mod[1] = bs->origin[1] + moveDir[1]*TAB_BOTMOVETRACE_NORMALDIST;
	traceto_mod[2] = bs->origin[2] + moveDir[2]*TAB_BOTMOVETRACE_NORMALDIST;

	//obstruction trace
	trap_Trace(&tr, bs->origin, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID);

	if (tr.fraction == 1 //trace is clear
		|| tr.entityNum == targetNum //is our ignore target
		|| (bs->currentEnemy && bs->currentEnemy->s.number == tr.entityNum)) //is our current enemy
	{//nothing blocking our path
		moveCommand = 0;
	}
	else if(tr.entityNum == ENTITYNUM_WORLD)
	{//world object, check to see if we can walk on it.
		if(tr.plane.normal[2] >= MIN_WALK_NORMAL)
		{//you're probably moving up a steep ledge that's still walkable
			moveCommand = 0;
		}
	}
	//check to see if this is another player.  If so, we should be able to jump over them easily.
	//RAFIXME - add force power/force jump skill check?
	else if(tr.entityNum < MAX_CLIENTS
		//not a bot or a bot that isn't jumping.
		&& (!botstates[tr.entityNum] || !botstates[tr.entityNum]->inuse 
		|| botstates[tr.entityNum]->jumpTime < level.time)
		&& bs->cur_ps.fd.forcePowerLevel[FP_LEVITATION] >= FORCE_LEVEL_1)
	{//another player who isn't our objective and isn't our current enemy.  Hop over them.  Don't hop
		//over bots who are already hopping.
		moveCommand = 1;
	}

	if(moveCommand == -1)
	{//our initial path is blocked. Try other methods to get around it.
		//Save the normal of the object so we can move around it if we can't jump/duck around it.
		VectorCopy(tr.plane.normal, saveNormal);

		//Check to see if we can successfully hop over this obsticle.
		VectorCopy(bs->origin, tracefrom_mod);

		//RAFIXME - make this based on Force jump skill level/force power availible.
		tracefrom_mod[2] += TAB_BOTMOVETRACE_HOPHEIGHT;
		traceto_mod[2] += TAB_BOTMOVETRACE_HOPHEIGHT;

		trap_Trace(&tr, tracefrom_mod, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID);

		if (tr.fraction == 1 //trace is clear
		|| tr.entityNum == targetNum //is our ignore target
		|| (bs->currentEnemy && bs->currentEnemy->s.number == tr.entityNum)) //is our current enemy
		{//the hop check was clean.
			moveCommand = 1;
		}
		//check the slope of the thing blocking us
		else if(tr.entityNum == ENTITYNUM_WORLD)
		{//world object
			if(tr.plane.normal[2] >= MIN_WALK_NORMAL)
			{//you could hop to this, which is a steep ledge that's still walkable
				moveCommand = 1;
			}
		}

		if(moveCommand == -1)
		{//our hop would be blocked by something.  let's try crawling under obsticle.

			//just move the traceto_mod down from the hop trace position.  This is faster
			//than redoing it from scratch.
			traceto_mod[2] -= TAB_BOTMOVETRACE_HOPHEIGHT;

			maxs[2] = CROUCH_MAXS_2; //set our trace box to be the size of a crouched player.

			trap_Trace(&tr, bs->origin, mins, maxs, traceto_mod, bs->client, MASK_PLAYERSOLID);

			if (tr.fraction == 1 //trace is clear
				|| tr.entityNum == targetNum //is our ignore target
				|| (bs->currentEnemy && bs->currentEnemy->s.number == tr.entityNum)) //is our current enemy
			{//we can duck under this object.
				moveCommand = 2;
			}
			//check the slope of the thing blocking us
			else if(tr.entityNum == ENTITYNUM_WORLD)
			{//world object
				if(tr.plane.normal[2] >= MIN_WALK_NORMAL)
				{//you could hop to this, which is a steep ledge that's still walkable
					moveCommand = 2;
				}
			}
		}
	}

	if(moveCommand != -1)
	{//we found a way around our current obsticle, check to make sure we're not going to go off a cliff.

		traceto_mod[0] = bs->origin[0] + moveDir[0]*TAB_BOTMOVETRACE_FALLDIST;
		traceto_mod[1] = bs->origin[1] + moveDir[1]*TAB_BOTMOVETRACE_FALLDIST;
		traceto_mod[2] = bs->origin[2] + moveDir[2]*TAB_BOTMOVETRACE_FALLDIST;

		VectorCopy(traceto_mod, tracefrom_mod);

		//check for 50+ feet drops
		traceto_mod[2] -= 532;

		trap_Trace(&tr, tracefrom_mod, mins, maxs, traceto_mod, bs->client, MASK_SOLID);
		if ((tr.fraction == 1 && !tr.startsolid))
		{//CLIFF!
			moveCommand = -1;
		}

		//RAFIXME - This might cause bots to freeze after the apex of a jump
		//over lava and such.  Keep an eye out for this possible behavior.
		if(bs->jumpTime < level.time)
		{//we're not actively jumping, so check to make sure we're not going to move
		//into slime/lava/fall to death areas.

			contents = trap_PointContents( tr.endpos, -1 );
			if(contents & (CONTENTS_SLIME|CONTENTS_LAVA))
			{//the fall point is inside something we don't want to move into
				moveCommand = -1;
			}
		}
	}

	if(moveCommand == -1)
	{//couldn't find a way to move in this direction.  Save the normal vector so we can use it to move around
		//this object.  Note, we even do this when we can hop/crawl around something but the fall check fails
		//so we can follow the railing or whatever this is.
		VectorCopy(saveNormal, hitNormal);
	}
	return moveCommand;
}


//701
//6 2
//543
//Find the movement quad you're trying to move into
int FindMovementQuad( playerState_t *ps, vec3_t moveDir )
{
	vec3_t viewfwd, viewright;
	vec3_t move;
	float x;
	float y;

	AngleVectors(ps->viewangles, viewfwd, viewright, NULL);

	VectorCopy(moveDir, move);

	viewfwd[2] = 0;
	viewright[2] = 0;
	move[2] = 0;

	VectorNormalize(viewfwd);
	VectorNormalize(viewright);
	VectorNormalize(move);

	x = DotProduct(viewright, move);
	y = DotProduct(viewfwd, move);

	if( x > .8 )
	{//right
		return 2;
	}
	else if( x < -0.8 )
	{//left
		return 6;
	}
	else if( x > .2 )
	{//right forward/back
		if( y < 0 )
		{//right back
			return 3;
		}
		else
		{//right forward
			return 1;
		}
	}
	else if ( x < -0.2 )
	{//left forward/back
		if( y < 0 )
		{//left back
			return 5;
		}
		else
		{//left forward
			return 7;
		}
	}
	else
	{//forward/back
		if( y < 0 )
		{//back
			return 4;
		}
		else
		{//forward
			return 0;
		}
	}
}


//701
//6 2
//543
//Adjusts moveDir based on a given move Quad.
//moveDir is suppose to be VectorNormalized
void TAB_AdjustMoveDirection( bot_state_t *bs, vec3_t moveDir, int Quad )
{
	vec3_t fwd, right;
	vec3_t addvect;

	AngleVectors(bs->cur_ps.viewangles, fwd, right, NULL);
	fwd[2] = 0;
	right[2] = 0;

	VectorNormalize(fwd);
	VectorNormalize(right);

	switch(Quad)
	{
		case 0:
			VectorCopy(fwd, addvect);
			break;
		case 1:
			VectorAdd(fwd, right, addvect);
			VectorNormalize(addvect);
			break;
		case 2:
			VectorCopy(right, addvect);
			break;
		case 3:
			VectorScale(fwd, -1, fwd);
			VectorAdd(fwd, right, addvect);
			VectorNormalize(addvect);
			break;
		case 4:
			VectorScale(fwd, -1, addvect);
			break;
		case 5:
			VectorScale(fwd, -1, fwd);
			VectorScale(right, -1, right);
			VectorAdd(fwd, right, addvect);
			VectorNormalize(addvect);
			break;
		case 6:
			VectorScale(right, -1, addvect);
			break;
		case 7:
			VectorScale(right, -1, right);
			VectorAdd(fwd, right, addvect);
			VectorNormalize(addvect);
			break;
		default:
			G_Printf("Bad Quad in TAB_AdjustMoveDirection.\n");
			return;
	}

	//VectorAdd(addvect, moveDir, moveDir);
	VectorCopy(addvect, moveDir);
	VectorNormalize(moveDir);
}


//process the return value from TAB_TraceJumpCrouchFall to do whatever you need to do to 
//get where.
void TAB_MovementCommand(bot_state_t *bs, int command, vec3_t moveDir)
{
	if(!command)
	{//don't need to do anything
		return;
	}
	else if(command == 1)
	{//Force Jump
		bs->jumpTime = level.time + 100;
		return;
	}
	else if(command == 2)
	{//crouch
		bs->duckTime = level.time + 100;
		return;
	}
	else
	{//can't move!
		G_Printf("Error: Bad movecommand in TAB_MovementCommand!\n");
		VectorCopy(vec3_origin, moveDir);
	}
}


//701
//6 2
//543
//shift the Quad to make sure it's aways valid
int TAB_AdjustQuad(int Quad)
{
	int Dir = Quad;
	while(Dir > 7)
	{//shift
		Dir -= 8;
	}
	while(Dir < 0)
	{//shift	
		Dir += 8;
	}

	return Dir;
}


//do all the nessicary calculations/traces for movement, automatically uses obstical evasion
//targetNum is the moveto target.  you're suppose to trace hit that eventually
void TAB_TraceMove(bot_state_t *bs, vec3_t moveDir, int targetNum)
{	
	vec3_t Dir;
	vec3_t hitNormal;
	int movecom;
	int fwdstrafe;
	int i = 7;
	int Quad;
	VectorClear(hitNormal);
	movecom = TAB_TraceJumpCrouchFall(bs, moveDir, targetNum, hitNormal);

	VectorCopy(moveDir, Dir);

	if(movecom != -1)
	{
		TAB_MovementCommand(bs, movecom, moveDir);
		
		/*	
		if(bs->evadeTime > level.time)
		{//ok, clear path, if we were using evade time, turn off the time and toggle
			//the evadeDir for the sake of the TryMoveAroundObsticle system so we
			//don't get caught on large objects
			if(bs->evadeDir > 3)
			{
				bs->evadeDir = 0;
			}
			else
			{
				bs->evadeDir = 7;
			}
			bs->evadeTime = level.time;
		}
		*/
		return;
	}

	//try moving around the edge of the blocking object if that's the problem.
	if(TryMoveAroundObsticle(bs, Dir, targetNum, hitNormal, 0, qtrue))
	{//found a way.
		VectorCopy(Dir, moveDir);
		return;
	}

	//restore the original moveDir incase our obsticle code choked.
	VectorCopy( moveDir, Dir );
	
	if(bs->evadeTime > level.time)
	{//try the current evade direction to prevent spazing
		TAB_AdjustMoveDirection(bs, Dir, bs->evadeDir);
		movecom = TAB_TraceJumpCrouchFall(bs, Dir, targetNum, hitNormal);
		if(movecom != -1)
		{
			TAB_MovementCommand(bs, movecom, Dir);
			VectorCopy(Dir, moveDir);
			bs->evadeTime = level.time + 500;
			return;
		}
		i--;
	}

	//Since our default direction didn't work we need to switch melee strafe directions if 
	//we are melee strafing.
	//0 = no strafe
	//1 = strafe right
	//2 = strafe left
	if( bs->meleeStrafeTime > level.time )
	{
		bs->meleeStrafeDir = Q_irand(0,2);
		bs->meleeStrafeTime = level.time + Q_irand(500, 1800);
	}

	fwdstrafe = FindMovementQuad(&bs->cur_ps, moveDir);
	
	if(Q_irand(0, 1))
	{//try strafing left 
		Quad = fwdstrafe - 2; 
	}
	else
	{
		Quad = fwdstrafe + 2;
	}

	Quad = TAB_AdjustQuad(Quad);
	
	//reset Dir to original moveDir
	VectorCopy(moveDir, Dir);

	//shift movedir for quad
	TAB_AdjustMoveDirection(bs, Dir, Quad);

	movecom = TAB_TraceJumpCrouchFall(bs, Dir, targetNum, hitNormal);

	if(movecom != -1)
	{
		TAB_MovementCommand(bs, movecom, Dir);
		VectorCopy(Dir, moveDir);
		bs->evadeDir = Quad;
		bs->evadeTime = level.time + 100;
		return;
	}
	i--;

	//no luck, try the other full strafe direction
	Quad += 4;
	Quad = TAB_AdjustQuad(Quad);

	//reset Dir to original moveDir
	VectorCopy(moveDir, Dir);

	//shift movedir for quad
	TAB_AdjustMoveDirection(bs, Dir, Quad);

	movecom = TAB_TraceJumpCrouchFall(bs, Dir, targetNum, hitNormal);

	if(movecom != -1)
	{
		TAB_MovementCommand(bs, movecom, Dir);
		VectorCopy(Dir, moveDir);
		bs->evadeDir = Quad;
		bs->evadeTime = level.time + 100;
		return;
	}
	i--;

	//still no dice
	for(; i > 0; i--)
	{
		Quad++;
		Quad = TAB_AdjustQuad(Quad);

		if(Quad == fwdstrafe || Quad == TAB_AdjustQuad(fwdstrafe - 2) || Quad == TAB_AdjustQuad(fwdstrafe + 2) 
			|| (bs->evadeTime > level.time && Quad == bs->evadeDir) )
		{//Already did those directions
			continue;
		}

		VectorCopy(moveDir, Dir);

		//shift movedir for quad
		TAB_AdjustMoveDirection(bs, Dir, Quad);
		movecom = TAB_TraceJumpCrouchFall(bs, Dir, targetNum, hitNormal);
		if(movecom != -1)
		{//find a good direction
			TAB_MovementCommand(bs, movecom, Dir);
			VectorCopy(Dir, moveDir);
			bs->evadeDir = Quad;
			bs->evadeTime = level.time + 100;
			return;
		}

	}

	//still no dice, just move as normal....and pray.
	//This probably shouldn't happen very often
	//but can happen when we're in free fall.  Don't worry about it.
}


//does this current inventory have a heavy weapon?
qboolean HaveHeavyWeapon(int weapons)
{
	if( (weapons & (1 << WP_SABER))
		|| (weapons & (1 << WP_ROCKET_LAUNCHER))
		|| (weapons & (1 << WP_THERMAL))
		|| (weapons & (1 << WP_TRIP_MINE))
		|| (weapons & (1 << WP_DET_PACK))
		|| (weapons & (1 << WP_CONCUSSION)))
	{
		return qtrue;
	}
	return qfalse;
}


//checks to see if this weapon will damage FL_DMG_BY_HEAVY_WEAP_ONLY targets
qboolean IsHeavyWeapon(bot_state_t *bs, int weapon)
{//right now we only show positive for weapons that can do this in primary fire mode
	switch (weapon)
	{
	case WP_MELEE:
		if(G_HeavyMelee(&g_entities[bs->client]))
		{
			return qtrue;
		}
		break;

	case WP_SABER:
	case WP_ROCKET_LAUNCHER:
	case WP_THERMAL:
	case WP_DET_PACK:
	case WP_CONCUSSION:
		return qtrue;
		break;
	};

	return qfalse;
}

//use Force push or pull on local func_doors
qboolean UseForceonLocal(bot_state_t *bs, vec3_t origin, qboolean pull)
{
	gentity_t *test = NULL;
	vec3_t center, pos1, pos2, size;
	qboolean SkipPushPull = qfalse;


	if(bs->DontSpamPushPull > level.time)
	{//pushed/pulled for this waypoint recently
		SkipPushPull = qtrue;
	}

	if(!SkipPushPull)
	{
		//Force Push/Pull
		while ( (test = G_Find( test, FOFS( classname ), "func_door" )) != NULL )
		{
			if(!(test->spawnflags & 2/*MOVER_FORCE_ACTIVATE*/))
			{//can't use the Force to move this door, ignore
				continue;
			}

			if(test->wait < 0 && test->moverState == MOVER_POS2)
			{//locked in position already, ignore.
				continue;
			}

			//find center, pos1, pos2
			if ( VectorCompare( vec3_origin, test->s.origin ) )
			{//does not have an origin brush, so pos1 & pos2 are relative to world origin, need to calc center
				VectorSubtract( test->r.absmax, test->r.absmin, size );
				VectorMA( test->r.absmin, 0.5, size, center );
				if ( (test->spawnflags&1) && test->moverState == MOVER_POS1 )
				{//if at pos1 and started open, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
					VectorSubtract( center, test->pos1, center );
				}
				else if ( !(test->spawnflags&1) && test->moverState == MOVER_POS2 )
				{//if at pos2, make sure we get the center where it *started* because we're going to add back in the relative values pos1 and pos2
					VectorSubtract( center, test->pos2, center );
				}
				VectorAdd( center, test->pos1, pos1 );
				VectorAdd( center, test->pos2, pos2 );
			}
			else
			{//actually has an origin, pos1 and pos2 are absolute
				VectorCopy( test->r.currentOrigin, center );
				VectorCopy( test->pos1, pos1 );
				VectorCopy( test->pos2, pos2 );
			}

			if(Distance(origin, center) > 400)
			{//too far away
				continue;
			}

			if ( Distance( pos1, bs->eye ) < Distance( pos2, bs->eye ) )
			{//pos1 is closer
				if ( test->moverState == MOVER_POS1 )
				{//at the closest pos
					if ( pull )
					{//trying to pull, but already at closest point, so screw it
						continue;
					}
				}
				else if ( test->moverState == MOVER_POS2 )
				{//at farthest pos
					if ( !pull )
					{//trying to push, but already at farthest point, so screw it
						continue;
					}
				}
			}
			else
			{//pos2 is closer
				if ( test->moverState == MOVER_POS1 )
				{//at the farthest pos
					if ( !pull )
					{//trying to push, but already at farthest point, so screw it
						continue;
					}
				}
				else if ( test->moverState == MOVER_POS2 )
				{//at closest pos
					if ( pull )
					{//trying to pull, but already at closest point, so screw it
						continue;
					}
				}
			}
			//we have a winner!
			break;
		}

		if(test)
		{//have a push/pull able door
			vec3_t viewDir;
			vec3_t ang;
			
			//doing special wp move
			bs->wpSpecial = qtrue;

			VectorSubtract(center, bs->eye, viewDir);
			vectoangles(viewDir, ang);
			VectorCopy(ang, bs->goalAngles);
			
			if(InFieldOfVision(bs->viewangles, 5, ang))
			{//use the force
				if(pull)
				{
					bs->doForcePull = level.time + 700;
				}
				else
				{
					bs->doForcePush = level.time + 700;
				}
				//Only press the button once every 15 seconds
				//this way the bots will be able to go up/down a lift weither the elevator
				//was up or down.
				//This debounce only applies to this waypoint.
				bs->DontSpamPushPull = level.time + 15000;
			}
			return qtrue;
		}
	}

	if(bs->DontSpamButton > level.time)
	{//pressed a button recently
		if(bs->useTime > level.time)
		{//holding down button
			//update the hacking buttons to account for lag about crap
			if(g_entities[bs->client].client->isHacking)
			{
				bs->useTime = bs->cur_ps.hackingTime + 100;
				bs->DontSpamButton = bs->useTime + 15000;
				bs->wpSpecial = qtrue;
				return qtrue;
			}

			//lost hack target.  clear things out
			bs->useTime = level.time;
			return qfalse;
		}
		else
		{
			return qfalse;
		}
	}

	//use button checking
	while ( (test = G_Find( test, FOFS( classname ), "trigger_multiple" )) != NULL )
	{
		if ( test->flags & FL_INACTIVE )
		{//set by target_deactivate
			continue;
		}

		if (!(test->spawnflags & 4) /*USE_BUTTON*/)
		{//can't use button this trigger
			continue;
		}

		if(test->alliedTeam)
		{
			if( g_entities[bs->client].client->sess.sessionTeam != test->alliedTeam )
			{//not useable by this team
				continue;
			}
		}

		FindOrigin(test, center);

		if(Distance(origin, center) > 200)
		{//too far away
			continue;
		}

		break;
	}

	if(!test)
	{//not luck so far
		while ( (test = G_Find( test, FOFS( classname ), "trigger_once" )) != NULL )
		{
			if ( test->flags & FL_INACTIVE )
			{//set by target_deactivate
				continue;
			}

			if(!test->use)
			{//trigger already fired
				continue;
			}

			if (!(test->spawnflags & 4) /*USE_BUTTON*/)
			{//can't use button this trigger
				continue;
			}

			if(test->alliedTeam)
			{
				if( g_entities[bs->client].client->sess.sessionTeam != test->alliedTeam )
				{//not useable by this team
					continue;
				}
			}
		
			FindOrigin(test, center);

			if(Distance(origin, center) > 200)
			{//too far away
				continue;
			}

			break;
		}
	}

	if(test)
	{//found a pressable/hackable button
		vec3_t ang, viewDir;
		//trace_t tr;

		if (g_gametype.integer == GT_SIEGE &&
		test->idealclass && test->idealclass[0])
		{
			if(!G_NameInTriggerClassList(bgSiegeClasses[g_entities[bs->client].client->siegeClass].name, test->idealclass))
			{ //not the right class to use this button
				if(!SwitchSiegeIdealClass(bs, test->idealclass))
				{//didn't want to switch to the class that hacks this trigger, call
					//for help and then see if there's a local breakable to just
					//smash it open.
					RequestSiegeAssistance(bs, SPC_SUPPORT);
					return AttackLocalBreakable(bs, origin);
				}
				else
				{//switched classes to be able to hack this target
					return qtrue;
				}
			}
		}

		//trap_Trace(&tr, bs->eye, NULL, NULL, center, bs->client, MASK_PLAYERSOLID);

		//if(tr.entityNum == test->s.number || tr.fraction == 1.0)
		{
			bs->wpSpecial = qtrue;
			
			//you can use use
			//set view angles.
			if(test->spawnflags & 2 /*FACING*/)
			{//you have to face in the direction of the trigger to have it work
				vectoangles(test->movedir, ang);
			}
			else
			{
				VectorSubtract( center, bs->eye, viewDir);
				vectoangles(viewDir, ang);
			}
			VectorCopy(ang, bs->goalAngles);

			if (G_PointInBounds( bs->origin, test->r.absmin, test->r.absmax ))
			{//inside trigger zone, press use.
				bs->useTime = level.time + test->genericValue7 + 100;
				bs->DontSpamButton = bs->useTime + 15000;
			}
			else
			{//need to move closer
				VectorSubtract(center, bs->origin, viewDir);

				viewDir[2] = 0;
				VectorNormalize(viewDir);

				trap_EA_Move(bs->client, viewDir, 5000);
			}
			return qtrue;
		}
	}
	return qfalse;
}


//scan for nearby breakables that we can destroy
qboolean AttackLocalBreakable(bot_state_t *bs, vec3_t origin)
{
	gentity_t *test = NULL;
	gentity_t *valid = NULL;
	qboolean defend = qfalse;
	vec3_t testorigin;

	while ( (test = G_Find( test, FOFS( classname ), "func_breakable" )) != NULL )
	{
		FindOrigin(test, testorigin);

		if( TargetDistance(bs, test, testorigin) < 300 )
		{
			if(test->teamnodmg && test->teamnodmg == g_entities[bs->client].client->sess.sessionTeam)
			{//on a team that can't damage this breakable, as such defend it from immediate harm
				defend = qtrue;
			}
		
			valid = test;
			break;
		}

		//reset for next check
		VectorClear(testorigin);
	}

	if(valid)
	{//due to crazy stack overflow issues, just attack wildly while moving towards the
		//breakable
		trace_t tr;
		int desiredweap = FavoriteWeapon(bs, valid, qtrue, qtrue, 0);	

		//visual check
		trap_Trace(&tr, bs->eye, NULL, NULL, testorigin, bs->client, MASK_PLAYERSOLID);

		if(tr.entityNum == test->s.number || tr.fraction == 1.0)
		{//we can see the breakable

			//doing special wp move
			bs->wpSpecial = qtrue;

			if(defend)
			{//defend this target since we can assume that the other team is going to try to
				//destroy this thingy
				//RAFIXME:  Add repair code.
				TAB_BotBehave_DefendBasic(bs, testorigin);
			}
			else if((test->flags & FL_DMG_BY_HEAVY_WEAP_ONLY) && !IsHeavyWeapon(bs, desiredweap))
			{//we currently don't have a heavy weap that we can use to destroy this target
				if(HaveHeavyWeapon(bs->cur_ps.stats[STAT_WEAPONS]))
				{//we have a weapon that could destroy this target but we don't have ammo
					//RAFIXME:  at this point we should have the bot go look for some ammo
					//but for now just defend this area.
					TAB_BotBehave_DefendBasic(bs, testorigin);
				}
				else if(g_gametype.integer == GT_SIEGE)
				{//ok, check to see if we should switch classes if noone else can blast this
					ShouldSwitchSiegeClasses(bs, qfalse);
					TAB_BotBehave_DefendBasic(bs, testorigin);
				}
				else
				{//go hunting for a weapon that can destroy this object
					//RAFIXME:  Add this code
					TAB_BotBehave_DefendBasic(bs, testorigin);
				}
			}
			else if((test->flags & FL_DMG_BY_SABER_ONLY) && !(bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_SABER)) )
			{//This is only damaged by sabers and we don't have a saber
				ShouldSwitchSiegeClasses(bs, qtrue);
				TAB_BotBehave_DefendBasic(bs, testorigin);
			}
			else
			{//ATTACK!
				//determine which weapon you want to use
				if(desiredweap != bs->virtualWeapon)
				{//need to switch to desired weapon
					BotSelectChoiceWeapon(bs, desiredweap, qtrue);
				}
				//set visible flag so we'll attack this.
				bs->frame_Enemy_Vis = 1;
				TAB_BotBehave_AttackBasic(bs, valid);
			}


			return qtrue;
		}
	}
	return qfalse;
}


qboolean CalculateJump(bot_state_t *bs, vec3_t origin, vec3_t dest)
{//should we try jumping to this location?
	vec3_t flatorigin, flatdest;
	float dist;
	float heightDif = dest[2] - origin[2];
	
	VectorCopy(origin, flatorigin);
	VectorCopy(dest, flatdest);

	dist = Distance(flatdest, flatorigin);

	if(heightDif > 30 && dist < 100)
	{
		return qtrue;
	}
	return qfalse;
}


//update the waypoint visibility debounce
void WPVisibleUpdate(bot_state_t *bs)
{
	if(OrgVisibleBox(bs->origin, NULL, NULL, bs->wpCurrent->origin, bs->client))
	{//see the waypoint hold the counter
		bs->wpSeenTime = level.time + 3000;
	}
}


//move to the given location.  This is for point to pointto point navigation only.  Use 
//TAB_BotMoveto if you want the bot to use waypoints instead of just blindly moving
//towards the target location.
//wptravel = traveling to waypoint?
//strafe = strafe while moving here?
//target = ignore number for trace move stuff, this should be for the person you're 
//attacking/moving to

void TAB_BotMove(bot_state_t *bs, vec3_t dest, qboolean wptravel, qboolean strafe)
{
	vec3_t moveDir;
	vec3_t viewDir;
	vec3_t ang;
	qboolean movetrace = qtrue;

	VectorSubtract(dest, bs->eye, viewDir);
	vectoangles(viewDir, ang);
	VectorCopy(ang, bs->goalAngles);

	if(wptravel)
	{//if we're traveling between waypoints, don't bob the view up and down.
		bs->goalAngles[PITCH] = 0;
	}

	VectorSubtract(dest, bs->origin, moveDir); 
	moveDir[2] = 0;
	VectorNormalize(moveDir);
	
	if (wptravel && !bs->wpCurrent)
	{
		return;
	}
	else if( wptravel )
	{
		//special wp moves
		if (bs->wpCurrent->flags & WPFLAG_DESTROY_FUNCBREAK)
		{//look for nearby func_breakable and break them if we can before we continue
			if(AttackLocalBreakable(bs, bs->wpCurrent->origin))
			{//found a breakable that we can destroy
				bs->wpSeenTime = level.time + 3000;
				//WPVisibleUpdate(bs);
				return;
			}
		}

		if (bs->wpCurrent->flags & WPFLAG_FORCEPUSH)
		{
			if(UseForceonLocal(bs, bs->wpCurrent->origin, qfalse))
			{//found something we can Force Push
				bs->wpSeenTime = level.time + 3000;
				//WPVisibleUpdate(bs);
				return;
			}
		}

		if (bs->wpCurrent->flags & WPFLAG_FORCEPULL)
		{
			if(UseForceonLocal(bs, bs->wpCurrent->origin, qtrue))
			{//found something we can Force Pull
				WPVisibleUpdate(bs);
				return;
			}
		}

		if (bs->wpCurrent->flags & WPFLAG_JUMP)
		{ //jump while travelling to this point
			vec3_t ang;
			vec3_t viewang;
			vec3_t velocity;
			vec3_t flatorigin, flatstart, flatend;
			float diststart;
			float distend;
			float horVelo;  //the horizontal velocity.

			VectorCopy(bs->origin, flatorigin);
			VectorCopy(bs->wpCurrentLoc, flatstart);
			VectorCopy(bs->wpCurrent->origin, flatend);

			flatorigin[2] = flatstart[2] = flatend[2] = 0;

			diststart = Distance(flatorigin, flatstart);
			distend = Distance(flatorigin, flatend);

			VectorSubtract(dest, bs->origin, viewang); 
			vectoangles(viewang, ang);

			//never strafe during when jumping somewhere
			strafe = qfalse;

			if(bs->cur_ps.groundEntityNum != ENTITYNUM_NONE &&
				(diststart < distend || bs->origin[2] < bs->wpCurrent->origin[2]) )
			{//before jump attempt
				if(ForcePowerforJump[ForceJumpNeeded(bs->origin, bs->wpCurrent->origin)] > bs->cur_ps.fd.forcePower)
				{//we don't have enough energy to make our jump.  wait here.
					bs->wpSpecial = qtrue;
					return;
				}
			}

			//velocity analysis
			viewang[2] = 0;
			VectorNormalize(viewang);
			VectorCopy(bs->cur_ps.velocity, velocity);
			velocity[2] = 0;
			horVelo = VectorNormalize(velocity);

			//make sure we're stopped or moving towards our goal before jumping
			if((diststart < distend && (VectorCompare(vec3_origin, velocity) || DotProduct(velocity, viewang) > .7))
				|| bs->cur_ps.groundEntityNum == ENTITYNUM_NONE)
			{//moving towards to our jump target or not moving at all or already on route and not already near the target.
				//hold down jump until we're pretty sure that we'll hit our target by just falling onto it.
				vec3_t toDestFlat;
				float estVert;
				float timeToEnd;
				float veloScaler; 
				qboolean holdJump = qtrue;

				VectorSubtract(flatend, flatorigin, toDestFlat);
				VectorNormalize(toDestFlat);

				veloScaler = DotProduct(toDestFlat, velocity);

				//figure out how long it will take make it to the target with our current horizontal velocity.
				if( horVelo )
				{//can't check when not moving
					timeToEnd = distend/(horVelo * veloScaler);  //assumes we're moving fully in the correct direction

					//calculate our estimated vectical position if we just let go of the jump now.
					estVert = bs->origin[2] + bs->cur_ps.velocity[2] * timeToEnd - g_gravity.value * timeToEnd * timeToEnd;
					
					if( estVert >= bs->wpCurrent->origin[2] )
					{//we're going to make it, let go of jump
						holdJump = qfalse;
					}

				}
	
				if(holdJump)
				{//jump
					bs->jumpTime = level.time + 100;
					bs->wpSpecial = qtrue;
					WPVisibleUpdate(bs);
					trap_EA_Move(bs->client, moveDir, 5000);
					return;
				}

			}

			//G_Printf("Not Holding jump during a waypoint jump move.\n");
		}

		//not doing a special wp move so clear that flag.
		bs->wpSpecial = qfalse;

		if (bs->wpCurrent->flags & WPFLAG_WAITFORFUNC)
		{
			if (!CheckForFunc(bs->wpCurrent->origin, bs->client))
			{
				WPVisibleUpdate(bs);
				if(!bs->AltRouteCheck && (bs->wpTravelTime - level.time) < 20000)
				{//been waiting for 10 seconds, try looking for alt route if we haven't 
					//already
					bot_route_t routeTest;
					int newwp = TAB_GetNearestVisibleWP(bs, bs->origin, bs->client, 
						bs->wpCurrent->index);
					bs->AltRouteCheck = qtrue;
					
					if(newwp == -1)
					{
						newwp = GetNearestWP(bs, bs->origin, bs->wpCurrent->index);
					}
					if(FindIdealPathtoWP(bs, newwp, bs->wpDestination->index, 
						bs->wpCurrent->index, routeTest) != -1)
					{//found a new route
						bs->wpCurrent = gWPArray[newwp];
						CopyRoute(routeTest, bs->botRoute);
						ResetWPTimers(bs);
					}
				}
				return;
			}
			
		}
		if (bs->wpCurrent->flags & WPFLAG_NOMOVEFUNC)
		{
			if (CheckForFunc(bs->wpCurrent->origin, bs->client))
			{
				WPVisibleUpdate(bs);
				if(!bs->AltRouteCheck && (bs->wpTravelTime - level.time) < 20000)
				{//been waiting for 10 seconds, try looking for alt route if we haven't 
					//already
					bot_route_t routeTest;
					int newwp = TAB_GetNearestVisibleWP(bs, bs->origin, bs->client, bs->wpCurrent->index);
					bs->AltRouteCheck = qtrue;
					
					if(newwp == -1)
					{
						newwp = GetNearestWP(bs, bs->origin, bs->wpCurrent->index);
					}
					if(FindIdealPathtoWP(bs, newwp, bs->wpDestination->index, 
						bs->wpCurrent->index, routeTest) != -1)
					{//found a new route
						bs->wpCurrent = gWPArray[newwp];
						CopyRoute(routeTest, bs->botRoute);
						ResetWPTimers(bs);
					}
				}
				return;
			}
		}

		if (bs->wpCurrent->flags & WPFLAG_DUCK)
		{ //duck while travelling to this point
			bs->duckTime = level.time + 100;
		}

		//visual check
		if(!(bs->wpCurrent->flags & WPFLAG_NOVIS))
		{//do visual check
			WPVisibleUpdate(bs);
		}
		else
		{
			movetrace = qfalse;
			strafe = qfalse;
		}
	}
	else
	{//jump to dest if we need to.
		if(CalculateJump(bs, bs->origin, dest))
		{
			bs->jumpTime = level.time + 100;
		}
	}

	//set strafing.
	if(strafe)
	{
		if(bs->meleeStrafeTime < level.time)
		{//select a new strafing direction, since we're actively navigating, switch strafe
			//directions more often
			//0 = no strafe
			//1 = strafe right
			//2 = strafe left
			bs->meleeStrafeDir = Q_irand(0,2);
			bs->meleeStrafeTime = level.time + Q_irand(500, 1000);
		}

		//adjust the moveDir to do strafing
		TAB_AdjustforStrafe(bs, moveDir);
	}

	if(movetrace)
	{
		TAB_TraceMove(bs, moveDir, bs->DestIgnore);
	}

	if(DistanceHorizontal(bs->origin, dest) > TAB_NAVTOUCH_DISTANCE)
	{//move if we're not in touch range.
		trap_EA_Move(bs->client, moveDir, 5000);
	}
}





void TAB_WPTouch(bot_state_t *bs)
{//Touched the target WP
	int i = FindOnRoute(bs->wpCurrent->index, bs->botRoute);

	if( i == -1 )
	{//This wp isn't on the route
		G_Printf("TAB_WPTouch %i: Somehow we've lost our route to our wpDestination.  Trying to reaquire it.\n", 
			bs->client);
		if(FindIdealPathtoWP(bs, bs->wpCurrent->index, bs->wpDestination->index, -2, bs->botRoute) == -1)
		{//couldn't find new path to destination!
			bs->wpCurrent = NULL;
			TAB_BotMove(bs, bs->DestPosition, qfalse, qfalse);
			G_Printf("TAB_WPTouch %i: No luck finding new path, going to move towards it manually.\n", 
				bs->client);
			return;
		}
		else
		{//set wp timers
			ResetWPTimers(bs);
		}
	}

	i = FindOnRoute(bs->wpCurrent->index, bs->botRoute);
	i++;

	if(i >= MAX_WPARRAY_SIZE || bs->botRoute[i] == -1)
	{//at destination wp
		bs->wpCurrent = bs->wpDestination;
		VectorCopy(bs->origin, bs->wpCurrentLoc);
		ResetWPTimers(bs);
		return;
	}
	else if(!gWPArray[i] || !gWPArray[i]->inuse  )
	{
		G_Printf("Error in TAB_WPTouch.\n");
		return;
	}

	bs->wpCurrent = gWPArray[bs->botRoute[i]];
	VectorCopy(bs->origin, bs->wpCurrentLoc);
	ResetWPTimers(bs);
}


void ResetWPTimers(bot_state_t *bs)
{
	if( (bs->wpCurrent->flags & WPFLAG_WAITFORFUNC) 
		|| (bs->wpCurrent->flags & WPFLAG_NOMOVEFUNC)
		|| (bs->wpCurrent->flags & WPFLAG_DESTROY_FUNCBREAK)
		|| (bs->wpCurrent->flags & WPFLAG_FORCEPUSH)
		|| (bs->wpCurrent->flags & WPFLAG_FORCEPULL) )
	{//it's an elevator or something waypoint time needs to be longer.
		bs->wpSeenTime = level.time + 30000;
		bs->wpTravelTime = level.time + 30000;	
	}
	else if(bs->wpCurrent->flags & WPFLAG_NOVIS)
	{
		//10 sec
		bs->wpSeenTime = level.time + 10000;
		bs->wpTravelTime = level.time + 10000;
	}
	else
	{
		//3 sec visual time
		bs->wpSeenTime = level.time + 3000;

		//10 sec move time
		bs->wpTravelTime = level.time + 10000;
	}

	if(bs->wpCurrent->index == bs->wpDestination->index)
	{//just touched our destination node
		bs->wpTouchedDest = qtrue;
	}
	else
	{//reset the final node touched flag
		bs->wpTouchedDest = qfalse;
	}

	bs->AltRouteCheck = qfalse;
	bs->DontSpamPushPull = 0;
}


//get the index to the nearest visible waypoint in the global trail
//just like GetNearestVisibleWP except with a bad waypoint input
int TAB_GetNearestVisibleWP(bot_state_t *bs, vec3_t org, int ignore, int badwp)
{
	int i;
	float bestdist;
	float flLen;
	int bestindex;
	vec3_t a, mins, maxs;

	if (g_RMG.integer)
	{
		bestdist = 300;
	}
	else
	{
		bestdist = 800;//99999;
				   //don't trace over 800 units away to avoid GIANT HORRIBLE SPEED HITS ^_^
	}
	bestindex = -1;

	mins[0] = -15;
	mins[1] = -15;
	mins[2] = -1;
	maxs[0] = 15;
	maxs[1] = 15;
	maxs[2] = 1;

	for (i=0; i < gWPNum; i++)
	{
		if (gWPArray[i] && gWPArray[i]->inuse && i != badwp)
		{
			if(bs)
			{//check to make sure that this bot's team can use this waypoint
				if(gWPArray[i]->flags & WPFLAG_REDONLY 
					&& g_entities[bs->client].client->sess.sessionTeam != TEAM_RED)
				{//red only wp, can't use
					continue;
				}

				if(gWPArray[i]->flags & WPFLAG_BLUEONLY 
					&& g_entities[bs->client].client->sess.sessionTeam != TEAM_BLUE)
				{//blue only wp, can't use
					continue;
				}
			}

			VectorSubtract(org, gWPArray[i]->origin, a);
			flLen = VectorLength(a);

			if(gWPArray[i]->flags & WPFLAG_WAITFORFUNC 
				|| (gWPArray[i]->flags & WPFLAG_NOMOVEFUNC)
				|| (gWPArray[i]->flags & WPFLAG_DESTROY_FUNCBREAK)
				|| (gWPArray[i]->flags & WPFLAG_FORCEPUSH)
				|| (gWPArray[i]->flags & WPFLAG_FORCEPULL) )
			{//boost the distance for these waypoints so that we will try to avoid using them
				//if at all possible
				flLen =+ 500;
			}

			if (flLen < bestdist && (g_RMG.integer || BotPVSCheck(org, gWPArray[i]->origin)) && OrgVisibleBox(org, mins, maxs, gWPArray[i]->origin, ignore))
			{
				bestdist = flLen;
				bestindex = i;
			}
		}
	}

	return bestindex;
}


//Behavior to move to the given DestPosition
//strafe = do some strafing while moving to this location
void TAB_BotMoveto(bot_state_t *bs, qboolean strafe)
{
	qboolean recalcroute = qfalse;
	qboolean findwp = qfalse;
	int badwp = -2;
	int destwp = -1;
	float distthen, distnow;

	if(!bs->wpCurrent)
	{////ok, we just did something other than wp navigation.  find the closest wp.
		findwp = qtrue;
	}
	else if( bs->wpSeenTime < level.time )
	{//lost track of the waypoint
		findwp = qtrue;
		badwp = bs->wpCurrent->index;
		bs->wpDestination = NULL;
		recalcroute = qtrue;
	}
	else if( bs->wpTravelTime < level.time )
	{//spent too much time traveling to this point or lost sight for too long.
		//recheck everything
		findwp = qtrue;
		badwp = bs->wpCurrent->index;
		bs->wpDestination = NULL;
		recalcroute = qtrue;
	}
	//Check to make sure we didn't get knocked way off course.
	else if( !bs->wpSpecial )
	{
		distthen = Distance(bs->wpCurrentLoc, bs->wpCurrent->origin);
		distnow = Distance(bs->wpCurrent->origin, bs->origin);
		if( 2 * distthen < distnow )
		{//we're pretty far off the path, check to make sure we didn't get knocked way off course.
				findwp = qtrue;
		}
	}

	if( !VectorCompare(bs->DestPosition, bs->lastDestPosition) || !bs->wpDestination)
	{//The goal position has moved from last frame.  make sure it's not closer to a different
		//destination WP
		destwp = TAB_GetNearestVisibleWP(bs, bs->DestPosition, bs->client, badwp);
	
		if( destwp == -1 )
		{//ok, don't have a wappoint that can see that point...ok, go to the closest wp and
			//and try from there.
			destwp = GetNearestWP(bs, bs->DestPosition, badwp);

			if( destwp == -1)
			{//crap, this map has no wps.  try just autonaving it then
				TAB_BotMove(bs, bs->DestPosition, qfalse, strafe);
				return;
			}
		}
		
		if (!bs->wpDestination || bs->wpDestination->index != destwp)
		{
			bs->wpDestination = gWPArray[destwp];
			recalcroute = qtrue;
		}
	}	

	if(findwp)
	{
		int wp = TAB_GetNearestVisibleWP(bs, bs->origin, bs->client, badwp);
		if (wp == -1)
		{//can't find a visible
			wp = GetNearestWP(bs, bs->origin, badwp);
			if ( wp == -1 )
			{//no waypoints
				TAB_BotMove(bs, bs->DestPosition, qfalse, strafe);
				return;
			}
		}
		
		//got a waypoint, lock on and move towards it
		bs->wpCurrent = gWPArray[wp];
		ResetWPTimers(bs);
		VectorCopy(bs->origin, bs->wpCurrentLoc);
		if(!recalcroute && FindOnRoute(bs->wpCurrent->index, bs->botRoute) == -1 )
		{//recalc route
			recalcroute = qtrue;
		}
	}

	if(recalcroute)
	{
		if(FindIdealPathtoWP(bs, bs->wpCurrent->index, bs->wpDestination->index, badwp, bs->botRoute) == -1)
		{//can't get to destination wp from current wp, wing it
			bs->wpCurrent = NULL;
			ClearRoute(bs->botRoute);
			TAB_BotMove(bs, bs->DestPosition, qfalse, strafe);
			return;
		}
		else
		{//set wp timers
			ResetWPTimers(bs);
		}
			
	}

	//travelling to a waypoint
	if((bs->wpCurrent->index != bs->wpDestination->index || !bs->wpTouchedDest) && Distance(bs->origin, bs->wpCurrent->origin) < BOT_WPTOUCH_DISTANCE
		&& !bs->wpSpecial)
	{
		TAB_WPTouch(bs);
	}


	//if you're closer to your bs->DestPosition than you are to your next waypoint, just 
	//move to your bs->DestPosition.  This is to prevent the bots from backstepping when 
	//very close to their target
	if(!bs->wpSpecial  //not doing something special
		&& (Distance(bs->origin, bs->wpCurrent->origin) > Distance(bs->origin, bs->DestPosition)) //closer to our destination than the next waypoint
		|| (bs->wpCurrent->index == bs->wpDestination->index && bs->wpTouchedDest) ) //We've touched our final waypoint and should head towards the destination
	{//move to DestPosition
		TAB_BotMove(bs, bs->DestPosition, qfalse, strafe);
	}
	else
	{//move to next waypoint
		TAB_BotMove(bs, bs->wpCurrent->origin, qtrue, strafe);
	}

	/*
	if(bs->wpCurrent->index != bs->wpDestination->index)
	{
		TAB_BotMove(bs, bs->wpCurrent->origin, qtrue);
	}
	else
	{
		TAB_BotMove(bs, bs->DestPosition, qfalse);
	}
	*/

}


//Adjusts the moveDir to account for strafing
void TAB_AdjustforStrafe(bot_state_t *bs, vec3_t moveDir)
{
	vec3_t right;

	if(!bs->meleeStrafeDir || bs->meleeStrafeTime < level.time )
	{//no strafe
		return;
	}

	AngleVectors(g_entities[bs->client].client->ps.viewangles, NULL, right, NULL);

	//flaten up/down
	right[2] = 0;

	if(bs->meleeStrafeDir == 2)
	{//strafing left
		VectorScale(right, -1, right);
	}

	//We assume that moveDir has been normalized before this function.
	VectorAdd(moveDir, right, moveDir);
	VectorNormalize(moveDir);
}


//swiped from the Unique's AotCTC code.
#define AI_LUNGE_DISTANCE 128

qboolean AI_CanLunge(bot_state_t *bs)
{
	gentity_t *ent;
	trace_t tr;
	vec3_t flatAng;
	vec3_t fwd, traceTo;
	vec3_t trmins = {-15, -15, -8};
	vec3_t trmaxs = {15, 15, 8};

	VectorCopy(bs->cur_ps.viewangles, flatAng);
	flatAng[PITCH] = 0;

	AngleVectors(flatAng, fwd, 0, 0);

	traceTo[0] = bs->origin[0] + fwd[0]*AI_LUNGE_DISTANCE;
	traceTo[1] = bs->origin[1] + fwd[1]*AI_LUNGE_DISTANCE;
	traceTo[2] = bs->origin[2] + fwd[2]*AI_LUNGE_DISTANCE;

	trap_Trace(&tr, bs->origin, trmins, trmaxs, traceTo, bs->client, MASK_PLAYERSOLID);

	ent = &g_entities[tr.entityNum];

	if ( tr.fraction != 1.0 && OJP_PassStandardEnemyChecks(bs, ent))
	{
		return qtrue;
	}

	return qfalse;
}


//Find the favorite weapon for this range.
int FindWeaponforRange(bot_state_t *bs, float range)
{
	int bestweap = -1;
	int bestfav = -1;
	int weapdist;
	int i;

	//try to find the fav weapon for this attack range
	for(i=0;i < WP_NUM_WEAPONS; i++)
	{
		if (bs->cur_ps.ammo[weaponData[i].ammoIndex] < weaponData[i].energyPerShot
			|| !(bs->cur_ps.stats[STAT_WEAPONS] & (1 << i)))
		{//check to see if we have this weapon or enough ammo for it.
			continue;
		}

		if(i == WP_SABER)
		{//hack to prevent the player from switching away from the saber when close to 
			//target
			weapdist = 300;
		}
		else
		{
			weapdist = MaximumAttackDistance[i];
			//weapdist = IdealAttackDistance[i] * 1.1;
		}

		if( range < weapdist && bs->botWeaponWeights[i] > bestfav )
		{
			bestweap = i;
			bestfav = bs->botWeaponWeights[i];
		}
	}

	return bestweap;
}


//attack/fire at currentEnemy while moving towards DestPosition
void TAB_BotBehave_AttackMove(bot_state_t *bs)
{
	vec3_t viewDir;
	vec3_t ang;
	vec3_t enemyOrigin;

	//switch to an approprate weapon
	int desiredweap;
	float range;

	float leadamount; //lead amount

	if(!bs->frame_Enemy_Vis && bs->enemySeenTime < level.time)
	{//lost track of enemy
		bs->currentEnemy = NULL;
		return;
	}

	FindOrigin(bs->currentEnemy, enemyOrigin);

	range = TargetDistance(bs, bs->currentEnemy, enemyOrigin);
	
	desiredweap = FindWeaponforRange(bs, range);

	if(desiredweap != bs->virtualWeapon && desiredweap != -1)
	{//need to switch to desired weapon otherwise stay with what you go
		BotSelectChoiceWeapon(bs, desiredweap, qtrue);
	}

	//move towards DestPosition
	TAB_BotMoveto(bs, qfalse);

	if(bs->wpSpecial)
	{//in special wp move, don't do interrupt it.
		return;
	}

	//adjust angle for target leading.
	leadamount = BotWeaponCanLead(bs);

	TAB_BotAimLeading(bs, enemyOrigin, leadamount);

	//set viewangle
	VectorSubtract(enemyOrigin, bs->eye, viewDir);

	vectoangles(viewDir, ang);
	VectorCopy(ang, bs->goalAngles);

	if(bs->frame_Enemy_Vis && bs->cur_ps.weapon == bs->virtualWeapon && range < MaximumAttackDistance[bs->virtualWeapon]
		&& range > MinimumAttackDistance[bs->virtualWeapon]
	//if(bs->cur_ps.weapon == bs->virtualWeapon && range <= IdealAttackDistance[bs->virtualWeapon] * 1.1
		&& (InFieldOfVision(bs->viewangles, 30, ang) 
			|| (bs->virtualWeapon == WP_SABER && InFieldOfVision(bs->viewangles, 100, ang))) )
	{//don't attack unless you're inside your AttackDistance band and actually pointing at your enemy.  
		//This is to prevent the bots from attackmoving with the saber @ 500 meters. :)
		trap_EA_Attack(bs->client);
	}
}


//special function to hack the distance readings for extremely tall entities like
//the rancor or func_breakable doors.
float TargetDistance(bot_state_t *bs, gentity_t* target, vec3_t targetorigin)
{
	vec3_t enemyOrigin;

	if(strcmp(target->classname, "misc_siege_item") == 0
		|| strcmp(target->classname, "func_breakable") == 0 
		|| target->client && target->client->NPC_class == CLASS_RANCOR) 
	{//flatten origin heights and measure
		VectorCopy(targetorigin, enemyOrigin);
		if(fabs(enemyOrigin[2] - bs->eye[2]) < 150)
		{//don't flatten unless you're on the same relative plane
			enemyOrigin[2] = bs->eye[2];
		}
		
		if(target->client && target->client->NPC_class == CLASS_RANCOR)
		{//Rancors are big and stuff
			return Distance(bs->eye, enemyOrigin) - 60;
		}
		else if(strcmp(target->classname, "misc_siege_item") == 0)
		{//assume this is a misc_siege_item.  These have absolute based mins/maxs.
			//Scale for the entity's bounding box
			float adjustor;
			float x = fabs(bs->eye[0] - enemyOrigin[0]);
			float y = fabs(bs->eye[1] - enemyOrigin[1]);
			float z = fabs(bs->eye[2] - enemyOrigin[2]);
			
			//find the general direction of the impact to determine which bbox length to
			//scale with
			if(x > y && x > z)
			{//x
				adjustor = target->r.maxs[0];
				//adjustor = target->r.maxs[0] - enemyOrigin[0];
			}
			else if( y > x && y > z )
			{//y
				adjustor = target->r.maxs[1];
				//adjustor = target->r.maxs[1] - enemyOrigin[1];
			}
			else
			{//z
				adjustor = target->r.maxs[2];
				//adjustor = target->r.maxs[2] - enemyOrigin[2];
			}

			return Distance(bs->eye, enemyOrigin) - adjustor + 15;
		}
		else if(strcmp(target->classname, "func_breakable") == 0)
		{
			//Scale for the entity's bounding box
			float adjustor;
			//float x = fabs(bs->eye[0] - enemyOrigin[0]);
			//float y = fabs(bs->eye[1] - enemyOrigin[1]);
			//float z = fabs(bs->eye[2] - enemyOrigin[2]);
			
			//find the smallest min/max and use that.
			if((target->r.absmax[0] - enemyOrigin[0]) < (target->r.absmax[1] - enemyOrigin[1]))
			{
				adjustor = target->r.absmax[0] - enemyOrigin[0];
			}
			else
			{
				adjustor = target->r.absmax[1] - enemyOrigin[1];
			}

			/*
			//find the general direction of the impact to determine which bbox length to
			//scale with
			if(x > y && x > z)
			{//x
				adjustor = target->r.absmax[0] - enemyOrigin[0];
			}
			else if( y > x && y > z )
			{//y
				adjustor = target->r.absmax[1] - enemyOrigin[1];
			}
			else
			{//z
				adjustor = target->r.absmax[2] - enemyOrigin[2];
			}
			*/

			return Distance(bs->eye, enemyOrigin) - adjustor + 15;
		}
		else
		{//func_breakable
			return Distance(bs->eye, enemyOrigin);
		}
	}
	else
	{//standard stuff
		return Distance(bs->eye, targetorigin);
	}
}


//Basic form of defend system.  Used for situation where you don't want to use the waypoint
//system
void TAB_BotBehave_DefendBasic(bot_state_t *bs, vec3_t defpoint)
{
	float dist;

	dist = Distance(bs->origin, defpoint);

	if(bs->currentEnemy)
	{//see an enemy
		TAB_BotBehave_AttackBasic(bs, bs->currentEnemy);
		if(dist > DEFEND_MAXDISTANCE)
		{//attack move back into the defend range
			//RAFIXME:  We need to have a basic version of the attackmove function
			TAB_BotMove(bs, defpoint, qfalse, qfalse);
		}
		else if(dist > DEFEND_MAXDISTANCE * .9)
		{//nearing max distance hold here and attack
			trap_EA_Move(bs->client, vec3_origin, 0);
		}
		else
		{//just attack them
		}
	}
	else
	{//don't see an enemy
		if(DontBlockAllies(bs))
		{
		}
		else if(dist < DEFEND_MINDISTANCE)
		{//just stand around and wait
			//RAFIXME:  visual scan stuff here?
		}
		else
		{//move closer to defend target
			TAB_BotMove(bs, defpoint, qfalse, qfalse);
		}
	}
}


void TAB_MoveforAttackQuad( bot_state_t *bs, vec3_t moveDir, int Quad )
{//set the moveDir to set our attack direction to be towards this Quad.
	vec3_t forward, right;

	AngleVectors(bs->viewangles, forward, right, NULL);

	switch(Quad)
	{	
	case Q_B: //down strike.
		VectorCopy(forward, moveDir);
		break;
	case Q_BR: //down right strike
		VectorAdd(forward, right, moveDir);
		VectorNormalize(moveDir);
		break;
	case Q_R: //right strike
		VectorCopy(right, moveDir);
		break;
	case Q_TR: //up right strike
		VectorScale(forward, -1, forward);
		VectorAdd(forward, right, moveDir);
		VectorNormalize(moveDir);
		break;
	case Q_T: //up strike
		VectorScale(forward, -1, forward);
		VectorCopy(forward, moveDir);
		break;
	case Q_TL: //up left strike
		VectorScale(forward, -1, forward);
		VectorScale(right, -1, right);
		VectorAdd(forward, right, moveDir);
		VectorNormalize(moveDir);
		break;
	case Q_L: //left strike
		VectorScale(right, -1, right);
		VectorCopy(right, moveDir);
		break;
	case Q_BL: //down left strike.
		VectorScale(right, -1, right);
		VectorAdd(forward, right, moveDir);
		VectorNormalize(moveDir);
		break;
	default:
		G_Printf("Bad Quad in TAB_MoveforAttackQuad.\n");
		break;
	};
}


//This is the basic, basic attack system, this doesn't use the waypoints system so you can
//link to it from inside the waypoint nav system without problems
void TAB_BotBehave_AttackBasic(bot_state_t *bs, gentity_t* target)
{
	vec3_t enemyOrigin, viewDir, ang, moveDir;
	float dist;
	float leadamount;

	FindOrigin(target, enemyOrigin);

	dist = TargetDistance(bs, target, enemyOrigin);

	//adjust angle for target leading.
	leadamount = BotWeaponCanLead(bs);

	TAB_BotAimLeading(bs, enemyOrigin, leadamount);

	//face enemy
	VectorSubtract(enemyOrigin, bs->eye, viewDir);
	vectoangles(viewDir, ang);
	VectorCopy(ang, bs->goalAngles);

	//check to see if there's a detpack in the immediate area of the target.
	if(bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_DET_PACK))
	{//only check if you got det packs.
		BotWeapon_Detpack(bs, target);
	}

	if(!BG_SaberInKata(bs->cur_ps.saberMove) && bs->cur_ps.fd.forcePower > 60 && 
		bs->cur_ps.weapon == WP_SABER && dist < 128 && InFieldOfVision(bs->viewangles, 90, ang))
	{//KATA!
		trap_EA_Attack(bs->client);
		trap_EA_Alt_Attack(bs->client);
		return;
	}

	if(bs->meleeStrafeTime < level.time)
	{//select a new strafing direction
		//0 = no strafe
		//1 = strafe right
		//2 = strafe left
		bs->meleeStrafeDir = Q_irand(0,2);
		bs->meleeStrafeTime = level.time + Q_irand(500, 1800);
	}

	VectorSubtract(enemyOrigin, bs->origin, moveDir); 

	if(dist < MinimumAttackDistance[bs->virtualWeapon])
	//if(dist < IdealAttackDistance[bs->virtualWeapon] * .7)
	{//move back
		VectorScale(moveDir, -1, moveDir);
	}
	else if( dist < IdealAttackDistance[bs->virtualWeapon] )
	{//we're close enough, quit moving closer
		VectorClear(moveDir);
	}
	/*
	else if( dist < IdealAttackDistance[bs->virtualWeapon] * 1.1)
	{//in ideal band, hold here
		VectorCopy(vec3_origin, moveDir);
	}
	*/
		
	moveDir[2] = 0;
	VectorNormalize(moveDir);

	//adjust the moveDir to do strafing
	TAB_AdjustforStrafe(bs, moveDir);

	if ( bs->cur_ps.weapon == bs->virtualWeapon 
		&& bs->virtualWeapon == WP_SABER && InFieldOfVision(bs->viewangles, 100, ang)
		&& (BG_SaberInIdle(bs->cur_ps.saberMove) 
		|| PM_SaberInBounce(bs->cur_ps.saberMove)
		|| PM_SaberInReturn(bs->cur_ps.saberMove)))
	{//we're using a lightsaber, we want to attack, 
		//and we need to choose a new attack swing, pick randomly.
		TAB_MoveforAttackQuad(bs, moveDir, Q_irand(Q_BR, Q_B));
	}

	if(!VectorCompare(vec3_origin, moveDir))
	{
		TAB_TraceMove(bs, moveDir, target->s.clientNum);
		trap_EA_Move(bs->client, moveDir, 5000);
	}

	if(bs->frame_Enemy_Vis && bs->cur_ps.weapon == bs->virtualWeapon
		&& (InFieldOfVision(bs->viewangles, 30, ang) 
		|| (bs->virtualWeapon == WP_SABER && InFieldOfVision(bs->viewangles, 100, ang))) )
	{//not switching weapons so attack
		trap_EA_Attack(bs->client);
	}
}


void TAB_BotBehave_Attack(bot_state_t *bs)
{
	int desiredweap = FavoriteWeapon(bs, bs->currentEnemy, qtrue, qtrue, 0);

	if( bs->frame_Enemy_Len > MaximumAttackDistance[desiredweap])
	//if( bs->frame_Enemy_Len > IdealAttackDistance[desiredweap] * 1.1)
	{//this should be an attack while moving function but for now we'll just use moveto
		vec3_t enemyOrigin;
		FindOrigin(bs->currentEnemy, enemyOrigin);
		VectorCopy(enemyOrigin, bs->DestPosition);
		bs->DestIgnore = bs->currentEnemy->s.number;
		TAB_BotBehave_AttackMove(bs);
		return;
	}

	//determine which weapon you want to use
	if(desiredweap != bs->virtualWeapon)
	{//need to switch to desired weapon
		BotSelectChoiceWeapon(bs, desiredweap, qtrue);
	}

	//we're going to go get in close so null out the wpCurrent so it will update when we're 
	//done.
	bs->wpCurrent = NULL;

	//use basic attack
	TAB_BotBehave_AttackBasic(bs, bs->currentEnemy);
}


//Attack an enemy or track after them if you loss them.
void TAB_BotTrackAttack(bot_state_t *bs)
{
	vec_t distance = Distance(bs->origin, bs->lastEnemySpotted);
	if(bs->frame_Enemy_Vis || bs->enemySeenTime > level.time )
	{//attack!
		bs->botBehave = BBEHAVE_ATTACK;
		VectorClear(bs->DestPosition);
		bs->DestIgnore = -1;
		return;
	}
	else if( distance < BOT_WEAPTOUCH_DISTANCE )
	{//do a visual scan
		if( bs->doVisualScan && bs->VisualScanTime < level.time )
		{//no dice.
			bs->doVisualScan = qfalse;
			bs->currentEnemy = NULL;
			bs->botBehave = BBEHAVE_STILL;
			return;
		}
		else 
		{//try looking around for 5 seconds
			VectorCopy(bs->lastEnemyAngles, bs->VisualScanDir);
			if(!bs->doVisualScan)
			{
				bs->doVisualScan = qtrue;
				bs->VisualScanTime = level.time + 5000;
			}
			bs->botBehave = BBEHAVE_VISUALSCAN;
			return;
		}
	}
	else
	{//lost him, go to the last seen location and see if we can find them
		bs->botBehave = BBEHAVE_MOVETO;
		VectorCopy(bs->lastEnemySpotted, bs->DestPosition);
		bs->DestIgnore = bs->currentEnemy->s.number;
		return;
	}
}


void TAB_BotSearchAndDestroy(bot_state_t *bs)
{
	if(!bs->currentEnemy && (VectorCompare(bs->DestPosition, vec3_origin) || DistanceHorizontal(bs->origin, bs->DestPosition) < BOT_WEAPTOUCH_DISTANCE) )
	{//hmmm, noone in the area and we're not already going somewhere
		//Check to see if we need some weapons or ammo
		gentity_t* desiredPickup = TAB_WantWeapon(bs, qfalse, TAB_FAVWEAPCARELEVEL_MAX);
		if(desiredPickup)
		{//want weapon, going for it.
			TAB_BotResupply(bs, desiredPickup);
			return;
		}

		desiredPickup = TAB_WantAmmo(bs, qfalse, TAB_FAVWEAPCARELEVEL_MAX);

		if(desiredPickup)
		{//want ammo, going for it. behavior is set in 
			TAB_BotResupply(bs, desiredPickup);
			return;
		}
		else
		{//let's just randomly go to a spawnpoint
			gentity_t *spawnpoint;
			vec3_t temp;
			spawnpoint = SelectSpawnPoint(vec3_origin, temp, temp, level.clients[bs->client].sess.sessionTeam);
			if(spawnpoint)
			{
				VectorCopy(spawnpoint->s.origin, bs->DestPosition);
				bs->DestIgnore = -1;
				bs->botBehave = BBEHAVE_MOVETO;
				return;
			}
			else
			{//that's not good
				bs->botBehave = BBEHAVE_STILL;
				VectorClear(bs->DestPosition);
				bs->DestIgnore = -1;
				return;
			}
		}
	}
	else if( !bs->currentEnemy && !VectorCompare(bs->DestPosition, vec3_origin) )
	{//moving towards a weapon or spawnpoint
		bs->botBehave = BBEHAVE_MOVETO;
		return;
	}
	else
	{//have an enemy and can see him
		if( bs->currentEnemy != bs->tacticEntity )
		{//attacking someone other that your target
			//This should probably be some sort of attack move or something
			TAB_BotTrackAttack(bs);
			return;
		}
		else
		{
			TAB_BotTrackAttack(bs);
			return;
		}
	}
}


//get out of the way of allies if they're close
qboolean DontBlockAllies(bot_state_t *bs)
{
	int i;
	for(i = 0; i < level.maxclients; i++)
	{
		if(i != bs->client //not the bot
			&& g_entities[i].inuse && g_entities[i].client //valid player
			&& g_entities[i].client->pers.connected == CON_CONNECTED  //who is connected
			&& !(g_entities[i].s.eFlags & EF_DEAD) //and isn't dead
			&& g_entities[i].client->sess.sessionTeam == g_entities[bs->client].client->sess.sessionTeam  //is on our team
			&& Distance(g_entities[i].client->ps.origin, bs->origin) < BLOCKALLIESDISTANCE)  //and we're too close to them.
		{//on your team and too close
			vec3_t moveDir, DestOrigin;
			VectorSubtract(bs->origin, g_entities[i].client->ps.origin, moveDir);
			VectorAdd(bs->origin, moveDir, DestOrigin);
			TAB_BotMove(bs, DestOrigin, qfalse, qfalse);
			return qtrue;
		}
	}
	return qfalse;
}


//defend given entity from attack
void TAB_BotDefend(bot_state_t *bs, gentity_t *defendEnt)
{
	vec3_t defendOrigin;
	float dist;

	FindOrigin(defendEnt, defendOrigin);

	if(strcmp(defendEnt->classname, "func_breakable") == 0 
		&& defendEnt->paintarget
		&& strcmp(defendEnt->paintarget, "shieldgen_underattack") == 0)
	{//dirty hack to get the bots to attack the shield generator on siege_hoth
		VectorSet(defendOrigin, -369, 858, -231);
	}

	dist = Distance(bs->origin, defendOrigin);

	if(bs->currentEnemy)
	{//see an enemy
		if(dist > DEFEND_MAXDISTANCE)
		{//attack move back into the defend range
			VectorCopy(defendOrigin, bs->DestPosition);
			bs->DestIgnore = defendEnt->s.number;
			TAB_BotBehave_AttackMove(bs);
		}
		else
		{//just attack them
			VectorClear(bs->DestPosition);
			bs->DestIgnore = -1;
			TAB_BotTrackAttack(bs);
		}
	}
	else
	{//don't see an enemy
		if(DontBlockAllies(bs))
		{
		}
		else if(dist < DEFEND_MINDISTANCE)
		{//just stand around and wait	
			VectorClear(bs->DestPosition);
			bs->DestIgnore = -1;
			bs->botBehave = BBEHAVE_STILL;
		}
		else
		{//move closer to defend target
			VectorCopy(defendOrigin, bs->DestPosition);
			bs->DestIgnore = defendEnt->s.number;
			bs->botBehave = BBEHAVE_MOVETO;
		}
	}
}


//update the currentEnemy visual data for the current enemy.
void TAB_EnemyVisualUpdate(bot_state_t *bs)
{
	vec3_t a;
	vec3_t enemyOrigin;
	vec3_t enemyAngles;
	trace_t tr;
	float dist;

	if(!bs->currentEnemy)
	{//bad!  This should never happen
		return;
	}

	FindOrigin(bs->currentEnemy, enemyOrigin);
	FindAngles(bs->currentEnemy, enemyAngles);

	VectorSubtract(enemyOrigin, bs->eye, a);
	dist = VectorLength(a);
	vectoangles(a, a);

	trap_Trace(&tr, bs->eye, NULL, NULL, enemyOrigin, bs->client, MASK_PLAYERSOLID);

	if ((tr.entityNum == bs->currentEnemy->s.number && InFieldOfVision(bs->viewangles, 90, a) && !BotMindTricked(bs->client, bs->currentEnemy->s.number)) 
		|| BotCanHear(bs, bs->currentEnemy, dist))
	{//spotted him
		bs->frame_Enemy_Len = TargetDistance(bs, bs->currentEnemy, enemyOrigin);
		bs->frame_Enemy_Vis = 1;
		VectorCopy(enemyOrigin, bs->lastEnemySpotted);
		VectorCopy(bs->currentEnemy->s.angles, bs->lastEnemyAngles);
		bs->enemySeenTime = level.time + BOT_VISUALLOSETRACKTIME;
	}
	else
	{//can't see him
		bs->frame_Enemy_Vis = 0;
	}
}


//the main scan regular scan for enemies for the TAB Bot
void TAB_ScanforEnemies(bot_state_t *bs)
{
	vec3_t	a, EnemyOrigin;
	int		closestEnemyNum = -1;
	float	closestDist = 99999;
	int		i = 0;
	float	distcheck;

	//check to see if our currentEnemy is still valid.
	if(bs->currentEnemy)
	{
		if(!OJP_PassStandardEnemyChecks(bs, bs->currentEnemy))
		{//target became invalid, move to next target
			bs->currentEnemy = NULL;
		}
	}

	if(bs->currentEnemy)
	{//we're already locked onto an enemy
		if(bs->currentEnemy->client && bs->currentEnemy->client->ps.isJediMaster)
		{//never lose lock on the JM and we always know where they are.
			TAB_EnemyVisualUpdate(bs);

			//override the last seen locations because we can always see them
			FindOrigin(bs->currentEnemy, bs->lastEnemySpotted);
			FindAngles(bs->currentEnemy, bs->lastEnemyAngles);
			return;
		}
		else if(bs->currentTactic == BOTORDER_SEARCHANDDESTROY 
			&& bs->tacticEntity)
		{//currently going after search and destroy target
			if(bs->tacticEntity->s.number == bs->currentEnemy->s.number)
			{
				TAB_EnemyVisualUpdate(bs);
				return;
			}
		}
		//If you're locked onto an objective, don't lose it.
		else if(bs->currentTactic == BOTORDER_OBJECTIVE
			&& bs->tacticEntity)
		{
			if(bs->tacticEntity->s.number == bs->currentEnemy->s.number)
			{
				TAB_EnemyVisualUpdate(bs);
				return;
			}
		}
	}


	for(i = 0; i <= level.num_entities; i++)
	{
		if (OJP_PassStandardEnemyChecks(bs, &g_entities[i]))
		{
			FindOrigin(&g_entities[i], EnemyOrigin);

			//RACC - PVS checks are precalced as part of the BSP process, should
			//be much faster than manual visual checks.
			if(!BotPVSCheck(EnemyOrigin, bs->eye))
			{//can't physically see this entity
				continue;
			}

			VectorSubtract(EnemyOrigin, bs->eye, a);
			distcheck = TargetDistance(bs, &g_entities[i], EnemyOrigin);
			vectoangles(a, a);

			if (g_entities[i].client && g_entities[i].client->ps.isJediMaster)
			{ //make us think the Jedi Master is close so we'll attack him above all
				distcheck = 1;
			}

			if (distcheck < closestDist //this enemy is closer than our currentEnemy.
					//we can physically see the other guy.
				&& ((InFieldOfVision(bs->viewangles, 90, a) && !BotMindTricked(bs->client, i) && OrgVisible(bs->eye, EnemyOrigin, -1))
					//or we can hear him.
					|| BotCanHear(bs, &g_entities[i], distcheck)) )
			{
				if (!bs->currentEnemy //don't have a current enemy 
					|| bs->currentEnemy->s.number == i //we ARE the current enemy
					|| distcheck < bs->frame_Enemy_Len - 100 ) //this enemy is much closer 
				{
					closestDist = distcheck;
					closestEnemyNum = i;
				}
			}
		}
	}

	if(closestEnemyNum == -1)
	{
		if(bs->currentEnemy)
		{//no enemies in the area but we were locked on previously.  Clear frame visual data so
		//we know that we should go go try to find them again.
			bs->frame_Enemy_Vis = 0;
		}
		else
		{//we're all alone.  No real need to update stuff in this case.
		}
	}
	else
	{//have a new target, update their data
		vec3_t EnemyAngles;
		FindOrigin(&g_entities[closestEnemyNum], EnemyOrigin);
		FindAngles(&g_entities[closestEnemyNum], EnemyAngles);

		bs->frame_Enemy_Len = closestDist;
		bs->frame_Enemy_Vis = 1;
		bs->currentEnemy = &g_entities[closestEnemyNum];
		VectorCopy(EnemyOrigin, bs->lastEnemySpotted);
		VectorCopy(EnemyAngles, bs->lastEnemyAngles);
		bs->enemySeenTime = level.time + BOT_VISUALLOSETRACKTIME;
	}
}


//visually scanning in the given direction.
void TAB_BotBehave_VisualScan(bot_state_t *bs)
{
	//RAFIXME - this could use more stuff
	VectorCopy(bs->VisualScanDir, bs->goalAngles);
	bs->wpCurrent = NULL;
}


//Find a class with a heavyweapon
//index = which one of the classes with heavy weapons do you return first?  the first one?
//second one?  etc.
//saber:	qtrue = look for class with saber
//			qfalse = look for class with general heavy weapon
//returns the basic class enum
int FindHeavyWeaponClass(int team, int index, qboolean saber)
{
	int i;
	int NumHeavyWeapClasses = 0;
	siegeTeam_t *stm;

	stm = BG_SiegeFindThemeForTeam(team);
	if (!stm)
	{
		G_Printf("Couldn't find team's theme in FindHeavyWeaponClass().\n");
		return -1;
	}

	// Loop through all the classes for this team
	for (i=0;i<stm->numClasses;i++)
	{
		if(!saber)
		{
			if(HaveHeavyWeapon(stm->classes[i]->weapons))
			{
				if(index == NumHeavyWeapClasses)
				{
					return stm->classes[i]->playerClass;
				}
				NumHeavyWeapClasses++;
			}
		}
		else
		{//look for saber
			if(stm->classes[i]->weapons & (1 << WP_SABER))
			{
				if(index == NumHeavyWeapClasses)
				{
					return stm->classes[i]->playerClass;
				}
				NumHeavyWeapClasses++;
			}
		}
	}

	//no heavy weapons/saber carrying units at this index
	return -1;
}


//use vchat to let people know you need a certain siege class at your current location
void RequestSiegeAssistance(bot_state_t *bs, int BaseClass)
{
	if(bs->vchatTime > level.time)
	{//recently did vchat, don't do it know.
		return;
	}
		
	//	voice_cmd
	switch(BaseClass)
	{
		case SPC_DEMOLITIONIST:
			trap_EA_Command(bs->client, "voice_cmd req_demo");
			bs->vchatTime = level.time + Q_irand(25000, 45000);
			break;
		case SPC_SUPPORT:
			trap_EA_Command(bs->client, "voice_cmd req_tech");
			bs->vchatTime = level.time + Q_irand(25000, 45000);
			break;

	};		
}


//check to see if the bot should switch player 
qboolean SwitchSiegeIdealClass(bot_state_t *bs, char *idealclass)
{
	char cmp[MAX_STRING_CHARS];
	int i = 0;
	int j;

	while (idealclass[i])
	{
        j = 0;
        while (idealclass[i] && idealclass[i] != '|')
		{
			cmp[j] = idealclass[i];
			i++;
			j++;
		}
		cmp[j] = 0;

		if(NumberofSiegeSpecificClass(g_entities[bs->client].client->sess.siegeDesiredTeam, cmp))
		{//found a player that can hack this trigger
			return qfalse;
		}
		
		if (idealclass[i] != '|')
		{ //reached the end, so, switch to a unit that can hack this trigger.
			siegeClass_t *holdClass = BG_SiegeFindClassByName(cmp);
			if(holdClass)
			{
				trap_EA_Command(bs->client, va("siegeclass \"%s\"\n", holdClass->name));
				return qtrue;
			}
			return qfalse;
		}
		i++;
	}

	return qfalse;
}


//Should we switch classes to destroy this breakable or just call for help?
//saber = saber only destroyable?
void ShouldSwitchSiegeClasses(bot_state_t *bs, qboolean saber)
{
	int i = 0;
	int x;
	int classNum;
	
	classNum = FindHeavyWeaponClass(g_entities[bs->client].client->sess.siegeDesiredTeam, i, saber);
	while(classNum != -1)
	{
		x = NumberofSiegeBasicClass(g_entities[bs->client].client->sess.siegeDesiredTeam, classNum);
		if(x)
		{//request assistance for this class since we already have someone
			//playing that class
			RequestSiegeAssistance(bs, SPC_DEMOLITIONIST);
			return;
		}
	
		//ok, noone is using that class check for the next 
		//indexed heavy weapon class
		i++;
		classNum = FindHeavyWeaponClass(g_entities[bs->client].client->sess.siegeDesiredTeam, i, saber);
	}

	//ok, noone else is using a siege class with a heavyweapon.  Switch to
	//one ourselves
	i--;
	classNum = FindHeavyWeaponClass(g_entities[bs->client].client->sess.siegeDesiredTeam, i, saber);

	if(classNum == -1)
	{//what the?!
		G_Printf("couldn't find a siege class with a heavy weapon in ShouldSwitchSiegeClasses().\n");
	}
	else
	{//switch to this class
		siegeClass_t *holdClass = BG_GetClassOnBaseClass( g_entities[bs->client].client->sess.siegeDesiredTeam, classNum, 0);
		trap_EA_Command(bs->client, va("siegeclass \"%s\"\n", holdClass->name));
	}
}


//find a numberof a specific class on a team.
int NumberofSiegeSpecificClass(int team, const char *classname)
{
	int i = 0;
	int NumPlayers = 0;

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		gentity_t *ent = &g_entities[i];
		if(ent && ent->client && ent->client->pers.connected == CON_CONNECTED
			&& ent->client->sess.siegeClass && ent->client->sess.siegeDesiredTeam == team)
		{
			if(strcmp(ent->client->sess.siegeClass, classname) == 0)
			{
				NumPlayers++;
			}
		}
	}
	return NumPlayers;
}


//Find the number of players useing this basic class on team.
int NumberofSiegeBasicClass(int team, int BaseClass)
{
	int i = 0;
	siegeClass_t *holdClass = BG_GetClassOnBaseClass( team, BaseClass, 0);
	int NumPlayers = 0;

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		gentity_t *ent = &g_entities[i];
		if(ent && ent->client && ent->client->pers.connected == CON_CONNECTED
			&& ent->client->sess.siegeClass && ent->client->sess.sessionTeam == team)
		{
			if(strcmp(ent->client->sess.siegeClass, holdClass->name) == 0)
			{
				NumPlayers++;
			}
		}
	}
	return NumPlayers;
}

//has the bot select the siege class with the fewest number of players on this team.
void SelectBestSiegeClass(int ClientNum, qboolean ForceJoin)
{
	int i;
	int bestNum = MAX_CLIENTS;
	int bestBaseClass = -1;
	int tempNum;

	if( ClientNum < 0 || ClientNum > MAX_CLIENTS )
	{//bad ClientNum
		return;
	}

	for(i=0; i < SPC_MAX; i++)
	{
		tempNum = NumberofSiegeBasicClass(g_entities[ClientNum].client->sess.siegeDesiredTeam, i);
		if( tempNum < bestNum )
		{//this class has fewer players.
			bestNum = tempNum;
			bestBaseClass = i;
		}
	}

	if(bestBaseClass != -1)
	{//found one, join that class
		siegeClass_t *holdClass = BG_GetClassOnBaseClass( g_entities[ClientNum].client->sess.siegeDesiredTeam, bestBaseClass, 0);
		if(Q_stricmp(g_entities[ClientNum].client->sess.siegeClass, holdClass->name) 
			|| ForceJoin)
		{//we're not already this class.
			trap_EA_Command(ClientNum, va("siegeclass \"%s\"\n", holdClass->name));
		}
	}
}


void DetermineCTFGoal(bot_state_t *bs)
{//has the bot decide what role it should take in a CTF fight
	int i;
	int NumOffence = 0;		//number of bots on offence
	int NumDefense = 0;		//number of bots on defence

	bot_state_t *tempbot;
	
	//clear out the current tactic
	bs->currentTactic = BOTORDER_OBJECTIVE;
	bs->tacticEntity = NULL;

	for(i=0; i < MAX_CLIENTS; i++)
	{
		tempbot = botstates[i];

		if(!tempbot || !tempbot->inuse || tempbot->client == bs->client)
		{//this bot isn't in use or this is the current bot
			continue;
		}

		if(g_entities[tempbot->client].client->sess.sessionTeam 
			!= g_entities[bs->client].client->sess.sessionTeam)
		{
			continue;
		}

		if(tempbot->currentTactic == BOTORDER_OBJECTIVE)
		{//this bot is going for/defending the flag
			if(tempbot->objectiveType == OT_CAPTURE)
			{
				NumOffence++;
			}
			else
			{//it's on defense
				NumDefense++;
			}
		}
	}

	if( NumDefense < NumOffence )
	{//we have less defenders than attackers.  Go on the defense.
			bs->objectiveType = OT_DEFENDCAPTURE;
	}
	else
	{//go on the attack
			bs->objectiveType = OT_CAPTURE;
	}
}


gentity_t *ClosestItemforAmmo( bot_state_t *bs, int ammo )
{//returns the closest gentity item (perminate) for a given ammo type.
	float		bestDist = 9999999;
	gentity_t	*bestItem = NULL;

	gitem_t		*currentItemType;
	gentity_t	*currentObject = NULL;
	float		currentDist;

	//look for the ammo type for this weapon
	currentItemType = BG_FindItemForAmmo((ammo_t) ammo);

	while ( (currentObject = G_Find( currentObject, FOFS( classname ), 
		currentItemType->classname )) != NULL )
	{//scan thru the map entities until we find an gentity of this ammo item

		if(!BG_CanItemBeGrabbed(g_gametype.integer, &currentObject->s, &bs->cur_ps))
		{//we can't pick up this item.
			continue;
		}

		currentDist = DistanceSquared(bs->origin, currentObject->r.currentOrigin);

		if(currentDist < bestDist)
		{//this item is closer than the current best ammo object
			if((currentObject->s.eFlags & EF_NODRAW) || (currentObject->flags & FL_DROPPED_ITEM)) 
			{//we're going to need a visibility check to see if we know about the status of this 
				//object
				if( BotPVSCheck(bs->eye, currentObject->r.currentOrigin) 
					&& InFOV2( currentObject->r.currentOrigin, &g_entities[bs->client], 100, 100 )
					&& OrgVisible(bs->eye, currentObject->r.currentOrigin, bs->client) )
				{//we can see the item
					if(currentObject->s.eFlags & EF_NODRAW)
					{//the ammo item isn't currently there, ignore.
						continue;
					}
				}
				else
				{//can't see the item
					if(currentObject->flags & FL_DROPPED_ITEM)
					{//If we can't see the item, we won't know about this dropped item.
						continue;
					}
				}
			}

			bestDist = currentDist;
			bestItem = currentObject;
		}
	}

	//try looking for those ammo despensor things
	currentObject = NULL;
	while ( (currentObject = G_Find( currentObject, FOFS( classname ), 
		"misc_ammo_floor_unit" )) != NULL )
	{//scan thru the map entities until we find an gentity of this ammo item
		currentDist = DistanceSquared(bs->origin, currentObject->r.currentOrigin);

		if(currentDist < bestDist)
		{//this item is closer than the current best ammo object
			bestDist = currentDist;
			bestItem = currentObject;
		}
	}
	
	if( bestItem )
	{//we've found an object for this ammo type, go for it.
		return bestItem;
	}
	else
	{
		return NULL;
	}
}

gentity_t *ClosestItemforWeapon( bot_state_t *bs, weapon_t weapon )
{//returns the closest gentity item (perminate) for a given ammo type.
	float		bestDist = 9999999;
	gentity_t	*bestItem = NULL;

	gitem_t		*currentItemType;
	gentity_t	*currentObject = NULL;
	float		currentDist;

	//look for the ammo type for this weapon
	currentItemType = BG_FindItemForWeapon(weapon);

	while ( (currentObject = G_Find( currentObject, FOFS( classname ), 
		currentItemType->classname )) != NULL )
	{//scan thru the map entities until we find an gentity of this ammo item
		if(!BG_CanItemBeGrabbed(g_gametype.integer, &currentObject->s, &bs->cur_ps))
		{//we can't pick up this item right now.
			continue;
		}

		currentDist = DistanceSquared(bs->origin, currentObject->r.currentOrigin);

		if(currentDist < bestDist)
		{//this item is closer than the current best ammo object

			if((currentObject->flags & FL_DROPPED_ITEM) 
				|| (currentObject->s.eFlags & EF_ITEMPLACEHOLDER))
			{//we're going to need a visibility check to see if we know about the status of this 
				//object
				if( BotPVSCheck(bs->eye, currentObject->r.currentOrigin) 
					&& InFOV2( currentObject->r.currentOrigin, &g_entities[bs->client], 100, 100 )
					&& OrgVisible(bs->eye, currentObject->r.currentOrigin, bs->client) )
				{//we can see the item
					if(currentObject->s.eFlags & EF_ITEMPLACEHOLDER)
					{//the weapon item isn't currently there, ignore.
						continue;
					}
				}
				else
				{//can't see the item
					if(currentObject->flags & FL_DROPPED_ITEM)
					{//If we can't see the item, we won't know about this dropped item.
						continue;
					}
				}
			}

			bestDist = currentDist;
			bestItem = currentObject;
		}
	}

	if( bestItem )
	{//we've found an object for this ammo type, go for it.
		return bestItem;
	}
	else
	{
		return NULL;
	}
}


gentity_t* TAB_WantWeapon(bot_state_t *bs, qboolean setOrder, int numOfChecks)
{//This function checks to see if the bot wishes to go  particular weapon that it doesn't
	//have.
	//Returns true if we've decided to go after ammo 
	//setOrder - sets wheither or not we should switch our current tactic if we want ammo.
	//numOfChecks - number of different types of weapons we want to check for.
	int favWeapon;
	int ignoreWeapons = 0;  //weapons we've already checked.
	int counter;

	if(g_gametype.integer == GT_SIEGE
		|| g_gametype.integer == GT_DUEL
		|| g_gametype.integer == GT_POWERDUEL)
	{//you can't pick up new weapons in these gametypes
		return NULL;
	}

	for(counter = 0; counter < numOfChecks; counter++)
	{//keep checking until we run out of fav weapons we want to check.
		//check to see if we have our favorite weapon.
		favWeapon = FavoriteWeapon(bs, bs->currentEnemy, qfalse, qfalse, ignoreWeapons);

		if(!(bs->cur_ps.stats[STAT_WEAPONS] & (1 << favWeapon)))
		{//We don't have the weapon we want.
			gentity_t *item = ClosestItemforWeapon(bs, favWeapon);
			if(item)
			{
				if(setOrder)
				{//we want to switch our current tactic.
					bs->currentTactic = BOTORDER_RESUPPLY;
					bs->tacticEntity = item;
				}
				return item;
			}
		}

		//no dice, add to ignore list and retry if we want to.
		ignoreWeapons |= (1 << favWeapon);
	}

	return NULL;
}


gentity_t* TAB_WantAmmo(bot_state_t *bs, qboolean setOrder, int numOfChecks)
{//This function checks to see if the bot wishes to go for ammo for a particular weapon.
	//Returns true if we've decided to go after ammo 
	//setOrder - sets wheither or not we should switch our current tactic if we want ammo.
	int ignoreWeapons = 0;  //weapons we've already checked.
	int counter;
	int favWeapon;
	int ammoMax;

	for(counter = 0; counter < numOfChecks; counter++)
	{//keep checking until we run out of fav weapons we want to check.
		//check our ammo on our current favorite weapon
		favWeapon = FavoriteWeapon(bs, bs->currentEnemy, qtrue, qfalse, ignoreWeapons);
		ammoMax = ammoData[weaponData[favWeapon].ammoIndex].max;

		if(weaponData[favWeapon].ammoIndex == AMMO_ROCKETS && g_gametype.integer == GT_SIEGE)
		{//hack for the lower rocket amount in Siege
			ammoMax = 10;
		}

		if(weaponData[favWeapon].ammoIndex != AMMO_NONE
			&& (bs->cur_ps.ammo[weaponData[favWeapon].ammoIndex] 
			< ammoMax * DESIREDAMMOLEVEL) )
		{//we have less ammo than we'd like, check to see if there's ammo for this weapon on
			//this map.
			gentity_t *item = ClosestItemforAmmo(bs, weaponData[favWeapon].ammoIndex);
			if(item)
			{
				if(setOrder)
				{//we want to switch our current tactic.
					bs->currentTactic = BOTORDER_RESUPPLY;
					bs->tacticEntity = item;
				}
				return item;
			}
		}

		//no dice, add to ignore list and retry if we want to.
		ignoreWeapons |= (1 << favWeapon);
	}

	return NULL;
}


void TAB_HigherBotAI(bot_state_t *bs)
{//This function handles the higher level thinking for the TABBots

	qboolean highLevelThink = (qboolean) (bs->highThinkTime < level.time);

	//determine which tactic we want to use.

	if(CarryingCapObjective(bs))
	{//we're carrying the objective, always go into capture mode.
		bs->currentTactic = BOTORDER_OBJECTIVE;
		bs->objectiveType = OT_CAPTURE;
	}
	else if(bs->currentTactic != BOTORDER_RESUPPLY 
		&& highLevelThink && TAB_WantWeapon(bs, qtrue, TAB_FAVWEAPCARELEVEL_INTERRUPT))
	//we want a particular weapon that we don't have.  Going for it. (Search for our two fav weapons only)
	{
		//G_Printf("Bot %i:  I've decided to go for a better gun.\n", bs->client);
		bs->highThinkTime = level.time + TAB_HIGHTHINKDEBOUNCE;
	}
	else if(bs->currentTactic != BOTORDER_RESUPPLY 
		&& highLevelThink && TAB_WantAmmo(bs, qtrue, TAB_FAVWEAPCARELEVEL_INTERRUPT))
	{//we want ammo for a weapon, going for it.  (Search for our two fav weapons' ammo only)
		//G_Printf("Bot %i:  I've decided to go for more ammo.\n", bs->client);
		bs->highThinkTime = level.time + TAB_HIGHTHINKDEBOUNCE;
	}
	else
	{//otherwise, just pick our tactic based on current situation.
		if(bs->botOrder == BOTORDER_NONE)
		{//we don't have a higher level order, use the default for the current situation
			if(bs->currentTactic)
			{//already have a tactic, use it.
			}
			else if(g_gametype.integer == GT_SIEGE)
			{//hack do objectives
				bs->currentTactic = BOTORDER_OBJECTIVE;
			}
			else if(g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTY)
			{
				DetermineCTFGoal(bs);
			}
			else if(g_gametype.integer == GT_SINGLE_PLAYER)
			{
				gentity_t *player = FindClosestHumanPlayer(bs->origin, NPCTEAM_PLAYER);
				if(player)
				{//a player on our team
					bs->currentTactic = BOTORDER_DEFEND;
					bs->tacticEntity = player;
				}
				else
				{//just run around and kill enemies
					bs->currentTactic = BOTORDER_SEARCHANDDESTROY;
					bs->tacticEntity = NULL;
				}

			}
			else if(g_gametype.integer == GT_JEDIMASTER)
			{
				bs->currentTactic = BOTORDER_JEDIMASTER;
			}
			else
			{
				//G_Printf("Bot %i:  I've decided to go into Search and Destroy mode.\n", bs->client);
				bs->currentTactic = BOTORDER_SEARCHANDDESTROY;
				bs->tacticEntity = NULL;
			}
		}
		else
		{
			bs->currentTactic = bs->botOrder;
			bs->tacticEntity = bs->orderEntity;
		}
	}
}


//Guts of the TAB Bot's AI
void TAB_StandardBotAI(bot_state_t *bs, float thinktime)
{
	qboolean UsetheForce = qfalse;

	//Reset the action states
	bs->doAttack = qfalse;
	bs->doAltAttack = qfalse;
	bs->doWalk = qfalse;
	bs->virtualWeapon = bs->cur_ps.weapon;
	bs->botBehave = BBEHAVE_NONE;

	if (gDeactivated || g_entities[bs->client].client->tempSpectate > level.time)
	{//disable ai during bot editting or while waiting for respawn in siege.
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		ClearRoute(bs->botRoute);
		VectorClear(bs->lastDestPosition);
		bs->wpSpecial = qfalse;

		//reset tactical stuff
		bs->currentTactic = BOTORDER_NONE;
		bs->tacticEntity = NULL;
		bs->objectiveType = 0;
		bs->MiscBotFlags = 0;
		return;

	}

	if(g_gametype.integer == GT_SIEGE && (level.time - level.startTime < 10000) )
	{//make sure that the bots aren't all on the same team after map changes.
		SelectBestSiegeClass(bs->client, qfalse);
	}

	if(bs->cur_ps.pm_type == PM_INTERMISSION 
		|| g_entities[bs->client].client->sess.sessionTeam == TEAM_SPECTATOR)
	{//in intermission
		//Mash the button to prevent the game from sticking on one level.
		if(g_gametype.integer == GT_SIEGE)
		{//hack to get the bots to spawn into seige games after the game has started
			if(g_entities[bs->client].client->sess.siegeDesiredTeam != SIEGETEAM_TEAM1
				&& g_entities[bs->client].client->sess.siegeDesiredTeam != SIEGETEAM_TEAM2)
			{//we're not on a team, go onto the best team available.
				g_entities[bs->client].client->sess.siegeDesiredTeam = PickTeam(bs->client, qtrue);
			}

			SelectBestSiegeClass(bs->client, qtrue);
			//siegeClass_t *holdClass = BG_GetClassOnBaseClass( g_entities[bs->client].client->sess.siegeDesiredTeam, irand(SPC_INFANTRY, SPC_HEAVY_WEAPONS), 0);
			//trap_EA_Command(bs->client, va("siegeclass \"%s\"\n", holdClass->name));
		}

		if(!(g_entities[bs->client].client->pers.cmd.buttons & BUTTON_ATTACK))
		{//only tap the button if it's not currently being pressed
			trap_EA_Attack(bs->client);
		}
		return;
	}

	//ripped the death stuff right from the standard AI.
	if (!bs->lastDeadTime)
	{ //just spawned in?
		bs->lastDeadTime = level.time;
		bs->MiscBotFlags = 0;
		bs->botOrder = BOTORDER_NONE;
		bs->orderEntity = NULL;
		bs->ordererNum = bs->client;
		VectorClear(bs->DestPosition);
		bs->DestIgnore = -1;
	}

	if (g_entities[bs->client].health < 1 || g_entities[bs->client].client->ps.pm_type == PM_DEAD)
	{
		bs->lastDeadTime = level.time;

		if (!bs->deathActivitiesDone && bs->lastHurt && bs->lastHurt->client && bs->lastHurt->s.number != bs->client)
		{//let other bots know that I was killed by someone.  This is for the emotional attachment stuff.  
			//However, TABBots don't care about emotional attachments so this is really for the other bottypes' sake.
			BotDeathNotify(bs);
			bs->deathActivitiesDone = 1;
		}
		
		//movement code
		bs->wpCurrent = NULL;
		bs->currentEnemy = NULL;
		bs->wpDestination = NULL;
		VectorClear(bs->lastDestPosition);
		ClearRoute(bs->botRoute);
		bs->wpSpecial = qfalse;
		

		//reset tactical stuff
		bs->currentTactic = BOTORDER_NONE;
		bs->tacticEntity = NULL;
		bs->objectiveType = 0;
		bs->MiscBotFlags = 0;

		//RACC - Try to respawn if you're done talking.
		if (rand()%10 < 5 &&
			(!bs->doChat || bs->chatTime < level.time))
		{
			trap_EA_Attack(bs->client);
		}

		return;
	}

	VectorCopy(bs->DestPosition, bs->lastDestPosition);


	//higher level thinking
	TAB_HigherBotAI(bs);

	//Scan for enemy targets and update the visual check data
	TAB_ScanforEnemies(bs);

	//doing current order
	if(bs->currentTactic == BOTORDER_KNEELBEFOREZOD)
	{
		TAB_BotKneelBeforeZod(bs);
	}
	else if(bs->currentTactic == BOTORDER_SEARCHANDDESTROY)
	{
		TAB_BotSearchAndDestroy(bs);
	}
	else if(bs->currentTactic == BOTORDER_OBJECTIVE)
	{
		TAB_BotObjective(bs);
	}
	else if(bs->currentTactic == BOTORDER_DEFEND)
	{
		TAB_BotDefend(bs, bs->tacticEntity);
	}
	else if(bs->currentTactic == BOTORDER_JEDIMASTER)
	{
		TAB_BotJediMaster(bs);
	}
	else if(bs->currentTactic == BOTORDER_SABERDUELCHALLENGE)
	{
		TAB_BotSaberDuel(bs);
	}
	else if(bs->currentTactic == BOTORDER_RESUPPLY)
	{
		TAB_BotResupply(bs, bs->tacticEntity);
	}
		
	//behavior implimentation
	if(bs->botBehave == BBEHAVE_MOVETO)
	{
		TAB_BotMoveto(bs, qfalse);
	}
	else if(bs->botBehave == BBEHAVE_ATTACK)
	{
		TAB_BotBehave_Attack(bs);
	}
	else if(bs->botBehave == BBEHAVE_VISUALSCAN)
	{
		TAB_BotBehave_VisualScan(bs);
	}
	else if(bs->botBehave == BBEHAVE_STILL)
	{
		TAB_BotBeStill(bs);
	}
	else
	{//BBEHAVE_NONE
	}

	//check for hard impacts
	//borrowed directly from impact code, so we might want to add in some fudge factor
	//at some point.  mmmmMM, fudge.
	if(bs->cur_ps.lastOnGround + 300 < level.time //haven't been on the ground for a while
		&& (!bs->cur_ps.fd.forceJumpZStart || bs->origin[2] < bs->cur_ps.fd.forceJumpZStart) ) //and not safely landing from a jump
	{//been off the ground for a little while
		float speed = VectorLength(bs->cur_ps.velocity);
		if( ( speed >= 100 + g_entities[bs->client].health && bs->virtualWeapon != WP_SABER ) || ( speed >= 700 ) )
		{//moving fast enough to get hurt get ready to crouch roll
			if(bs->virtualWeapon != WP_SABER)
			{//try switching to saber
				if(!BotSelectChoiceWeapon(bs, WP_SABER, 1))
				{//ok, try switching to melee
					BotSelectChoiceWeapon(bs, WP_MELEE, 1);
				}
			}

			if(bs->virtualWeapon == WP_MELEE || bs->virtualWeapon == WP_SABER)
			{//in or switching to a weapon that allows us to do roll landings
				bs->duckTime = level.time + 300;
				if(!bs->lastucmd.forwardmove && !bs->lastucmd.rightmove)
				{//not trying to move at all so we should at least attempt to move
					trap_EA_MoveForward(bs->client);
				}
			}
		}
	}


	//Chat Stuff
	if (bs->doChat && bs->chatTime <= level.time)
	{
		if (bs->chatTeam)
		{
			trap_EA_SayTeam(bs->client, bs->currentChat);
			bs->chatTeam = 0;
		}
		else
		{
			trap_EA_Say(bs->client, bs->currentChat);
		}
		if (bs->doChat == 2)
		{
			BotReplyGreetings(bs);
		}
		bs->doChat = 0;
	}
	
	if(bs->duckTime > level.time)
	{
		trap_EA_Crouch(bs->client);
	}

	if(bs->jumpTime > level.time)
	{
		trap_EA_Jump(bs->client);

		if (g_entities[bs->client].client->ps.groundEntityNum == ENTITYNUM_NONE)
		{
			g_entities[bs->client].client->ps.pm_flags |= PMF_JUMP_HELD;
		}
	}

	//use action
	if(bs->useTime >level.time)
	{
		trap_EA_Use(bs->client);
	}

	//attack actions
	if(bs->doAttack)
	{
		trap_EA_Attack(bs->client);
	}

	if(bs->doAltAttack)
	{
		trap_EA_Alt_Attack(bs->client);
	}

	UsetheForce = TAB_ForcePowersThink(bs);

	//Force powers are listed in terms of priority

	if (!UsetheForce && (bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH)) && (bs->doForcePush > level.time || bs->cur_ps.fd.forceGripBeingGripped > level.time) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH])
	{//use force push
		level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
		UsetheForce = qtrue;
	}

	if (!UsetheForce && (bs->cur_ps.fd.forcePowersKnown & (1 << FP_PULL)) && bs->doForcePull > level.time && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PULL]][FP_PULL])
	{//use force pull
		level.clients[bs->client].ps.fd.forcePowerSelected = FP_PULL;
		UsetheForce = qtrue;
	}


	if(UsetheForce)
	{
		if (bot_forcepowers.integer && !g_forcePowerDisable.integer)
		{
			trap_EA_ForcePower(bs->client);
		}
	}

	//Set view angles
	MoveTowardIdealAngles(bs);

}


//find the favorite weapon that this bot has for this target at the moment.  This doesn't do any sort of distance checking, it 
//just returns the favorite weapon.
int FavoriteWeapon(bot_state_t *bs, gentity_t *target, qboolean haveCheck, 
				   qboolean ammoCheck, int ignoreWeaps)
{
	int i;
	int bestweight = -1;
	int bestweapon = 0;

	for(i=0; i < WP_NUM_WEAPONS; i++)
	{
		if (haveCheck &&
			!(bs->cur_ps.stats[STAT_WEAPONS] & (1 << i)))
		{//check to see if we actually have this weapon.
			continue;
		}

		if (ammoCheck && 
			bs->cur_ps.ammo[weaponData[i].ammoIndex] < weaponData[i].energyPerShot)
		{//check to see if we have enough ammo for this weapon.
			continue;
		}

		if(ignoreWeaps & (1 << i))
		{//check to see if this weapon is on our ignored weapons list
			continue;
		}

		if(target && target->inuse)
		{//do additional weapon checks based on our target's attributes.
			if (target->flags & FL_DMG_BY_HEAVY_WEAP_ONLY)
			{//don't use weapons that can't damage this target
				if(!IsHeavyWeapon(bs, i))
				{
					continue;
				}
			}

			//try to use explosives on breakables if we can.
			if(strcmp(target->classname, "func_breakable") == 0)
			{
				if(i == WP_DET_PACK)
				{	
					bestweight = 100;
					bestweapon = i;
				}
				else if(i == WP_THERMAL)
				{
					bestweight = 99;
					bestweapon = i;
				}
				else if(i == WP_ROCKET_LAUNCHER)
				{
					bestweight = 99;
					bestweapon = i;
				}
				else if(bs->botWeaponWeights[i] > bestweight)
				{
					bestweight = bs->botWeaponWeights[i];
					bestweapon = i;
				}
			}
			else
			{//no special requirements for this target.
				if(bs->botWeaponWeights[i] > bestweight)
				{
					bestweight = bs->botWeaponWeights[i];
					bestweapon = i;
				}
			}
		}
		else
		{//no target
			if(bs->botWeaponWeights[i] > bestweight)
			{
				bestweight = bs->botWeaponWeights[i];
				bestweapon = i;
			}
		}
	}

	if (g_gametype.integer == GT_SIEGE 
		&& bestweapon == WP_NONE 
		&& target && (target->flags & FL_DMG_BY_HEAVY_WEAP_ONLY))
	{//we don't have the weapons to damage this thingy. call for help!
		RequestSiegeAssistance(bs, SPC_DEMOLITIONIST);
	}

	return bestweapon;
}

extern char gObjectiveCfgStr[1024];
qboolean ObjectiveStillActive(int objective)
{//Is the given Objective for the given team still active?
	int i = 0;
	int objectiveNum = 0;

	if(objective <= 0)
	{//bad objective number
		return qfalse;
	}

	while (gObjectiveCfgStr[i])
	{
		if (gObjectiveCfgStr[i] == '|')
		{ //switch over to team2, this is the next section
			objectiveNum = 0;
		}
		else if (gObjectiveCfgStr[i] == '-')
		{
			objectiveNum++;
			i++;
			if(gObjectiveCfgStr[i] == '0' && objectiveNum == objective)
			{//tactic still active.
				return qtrue;
			}
		}
		i++;
	}

	return qfalse;
}


int FindValidObjective(int objective)
{
	int x = 0;

	//since the game only ever does 6 objectives
	if(objective == -1)
	{ 
		objective = Q_irand(1,6);
	}

	//we assume that the round over check is done before this point
	while(!ObjectiveStillActive(objective))
	{
		objective--;
		if(objective < 1)
		{
			objective = 6;
		}
	}

	//depandancy checking
	for(x=0; x < MAX_OBJECTIVEDEPENDANCY; x++)
	{
		if(ObjectiveDependancy[objective-1][x])
		{//dependancy
			if(ObjectiveStillActive(ObjectiveDependancy[objective-1][x]))
			{//a prereq objective hasn't been completed, do that first
				return FindValidObjective(ObjectiveDependancy[objective-1][x]);
			}
		}
	}

	return objective;
}

/*
//Find the first active objective for this team
int FindFirstActiveObjective(int team)
{
	int i = 0;
	int	curteam = SIEGETEAM_TEAM1;
	int objectiveNum = 0;

	while (gObjectiveCfgStr[i])
	{
		if (gObjectiveCfgStr[i] == '|')
		{ //switch over to team2, this is the next section
            curteam = SIEGETEAM_TEAM2;
			objectiveNum = 0;
		}
		else if (gObjectiveCfgStr[i] == '-')
		{
			objectiveNum++;
			i++;
			if(gObjectiveCfgStr[i] == '0')
			{//completed
				return objectiveNum;
			}
		}
		i++;
	}
	return -1;
}
*/


//determines the trigger entity and type for a seige objective
//attacker = objective attacker or defender? This is used for the recursion stuff.
//use 0 to have it be determined by the side of the info_siege_objective
gentity_t * DetermineObjectiveType(int team, int objective, int *type, gentity_t *obj, int attacker)
{
	gentity_t *test = NULL;

	if(g_gametype.integer != GT_SIEGE)
	{//find the flag for this objective type
		char *c;
		if(*type == OT_CAPTURE)
		{
			if(team == TEAM_RED)
			{
				c = "team_CTF_blueflag";
			}
			else
			{
				c = "team_CTF_redflag";
			}
		}
		else if(*type == OT_DEFENDCAPTURE)
		{
			if(team == TEAM_RED)
			{
				c = "team_CTF_redflag";
			}
			else
			{
				c = "team_CTF_blueflag";
			}
		}
		else
		{
			G_Printf("DetermineObjectiveType() Error: Bad ObjectiveType Given for CTF flag find.\n");
			return NULL;
		}
		test = G_Find (test, FOFS(classname), c);
			return test;

	}

	if(!obj)
	{//don't already have obj entity to scan from, find it.
		while ( (obj = G_Find( obj, FOFS( classname ), "info_siege_objective" )) != NULL )
		{
			if( objective == obj->objective )
			{//found it
				if(!attacker)
				{//this should always be true
					if(obj->side == team)
					{//we get points from this objective, we're the attacker.
						attacker = 1;
					}
					else
					{//we're the defender
						attacker = 2;
					}
				}
				break;
			}
		}
	}

	if(!obj)
	{//hmmm, couldn't find the thing.  That's not good.
		*type = OT_NONE;
		return NULL;
	}

	//let's try back tracking and figuring out how this trigger is triggered

	//try scanning thru the target triggers first.
	while ( (test = G_Find( test, FOFS( target ), obj->targetname )) != NULL )
	//while ( (test = G_Find( test, FOFS( target4 ), obj->classname )) != NULL )
	{
		if(test->flags & FL_INACTIVE)
		{//this entity isn't active, ignore it
			continue;
		}
		else if(strcmp(test->classname, "func_breakable") == 0
			||(strcmp(test->classname, "NPC") == 0))
		{//Destroyable objective or NPC
			if(attacker == 1)
			{//attack
				*type = OT_ATTACK;
				return test;
			}
			else if( attacker == 2)
			{//Defend this target
				*type = OT_DEFEND;
				return test;
			}
			else
			{
				G_Printf("Bad attacker state for func_breakable objective in DetermineObjectiveType().\n");
				return test;
			}
			break;
		}
		else if((strcmp(test->classname, "trigger_multiple") == 0) 
			|| (strcmp(test->classname, "target_relay") == 0)
			|| (strcmp(test->classname, "target_counter") == 0)
			|| (strcmp(test->classname, "func_usable") == 0)
			|| (strcmp(test->classname, "trigger_once") == 0))
		{//ok, you can't do something directly to a trigger_multiple or a target_relay
			//scan for whatever links to this relay
			gentity_t * triggerer = DetermineObjectiveType(team, objective, type, test, attacker);
			if(triggerer)
			{//success!
				return triggerer;
			}
			else if((strcmp(test->classname, "func_usable") == 0) && (test->spawnflags & 64) //useable by player
				|| ((strcmp(test->classname, "trigger_multiple") == 0) && (test->spawnflags & 4)) //need to press the use button to work
				|| (strcmp(test->classname, "trigger_once") == 0) )
			{//ok, so they aren't linked to anything, try using them directly then
				if(test->NPC_targetname)
				{//vehicle objective
					if(attacker == 1)
					{//attack
						*type = OT_VEHICLE;
						return test;
					}
					else if( attacker == 2)
					{//destroy the vehicle
						gentity_t *vehicle = NULL;
						//Find the vehicle
						while ( (vehicle = G_Find( vehicle, FOFS( script_targetname ), test->NPC_targetname )) != NULL )
						{
							if (vehicle->inuse && vehicle->client && vehicle->s.eType == ET_NPC &&
								vehicle->s.NPC_class == CLASS_VEHICLE && vehicle->m_pVehicle)
							{
								break;
							}
						}

						if(!vehicle)
						{//can't find the vehicle?!
							*type = OT_WAIT;
							return NULL;
						}

						test = vehicle;
						*type = OT_ATTACK;
						return test;
					}
					else
					{
						G_Printf("Bad attacker state for vehicle trigger_once objective in DetermineObjectiveType().\n");
						return test;
					}
				}
				else
				{
					if(attacker == 1)
					{//attack
						*type = OT_TOUCH;
						return test;
					}
					else if( attacker == 2)
					{//Defend this target
						*type = OT_DEFEND;
						return test;
					}
					else
					{
						G_Printf("Bad attacker state for func_usable objective in DetermineObjectiveType().\n");
						return test;
					}
				}
				break;
			}
		}
	}

	test = NULL;

	//ok, see obj is triggered by the goaltarget of a capturable misc_siege_item 
	while ( (test = G_Find( test, FOFS( goaltarget ), obj->targetname )) != NULL )
	{
		if(strcmp(test->classname, "misc_siege_item") == 0)
		{//Destroyable objective
			if(attacker == 1)
			{//attack
				*type = OT_CAPTURE;
				return test;
			}
			else if (attacker == 2)
			{//Defend this target
				*type = OT_DEFENDCAPTURE;
				return test;
			}
			else
			{
				G_Printf("Bad attacker state for misc_siege_item objective in DetermineObjectiveType().\n");
				return test;
			}
			break;
		}
	}

	test = NULL;

	//ok, see obj is triggered by the target3 (delivery target) of a capturable misc_siege_item 
	while ( (test = G_Find( test, FOFS( target3 ), obj->targetname )) != NULL )
	{
		if(strcmp(test->classname, "misc_siege_item") == 0)
		{//capturable objective
			if(attacker == 1)
			{//attack
				*type = OT_CAPTURE;
				return test;
			}
			else if (attacker == 2)
			{//Defend this target
				*type = OT_DEFENDCAPTURE;
				return test;
			}
			else
			{
				G_Printf("Bad attacker state for misc_siege_item (target3) objective in DetermineObjectiveType().\n");
				return test;
			}
			break;
		}
	}

	test = NULL;

	//check for a destroyable misc_siege_item that triggers this objective
	while ( (test = G_Find( test, FOFS( target4 ), obj->targetname )) != NULL )
	{
		if(strcmp(test->classname, "misc_siege_item") == 0)
		{
			if(attacker == 1)
			{//attack
				*type = OT_ATTACK;
				return test;
			}
			else if( attacker == 2)
			{//Defend this target
				*type = OT_DEFEND;
				return test;
			}
			else
			{
				G_Printf("Bad attacker state for misc_siege_item (target4) objective in DetermineObjectiveType().\n");
				return test;
			}
			break;
		}
	}

	//no dice
	*type = OT_NONE;
	return NULL;
}


//use/touch the given objective
void objectiveType_Touch(bot_state_t *bs)
{
	vec3_t objOrigin;

	FindOrigin(bs->tacticEntity, objOrigin);

	if(!G_PointInBounds( bs->origin, bs->tacticEntity->r.absmin, bs->tacticEntity->r.absmax ))
	{//move closer
		VectorCopy(objOrigin, bs->DestPosition);
		bs->DestIgnore = bs->tacticEntity->s.number;
		if(bs->currentEnemy)
		{//have a local enemy, attackmove
			TAB_BotBehave_AttackMove(bs);
		}
		else
		{//normal move
			bs->botBehave = BBEHAVE_MOVETO;
		}	
	}
	else
	{//in range hold down use
		bs->useTime = level.time + 100;
		if(bs->tacticEntity->spawnflags & 2 /*FACING*/)
		{//you have to face in the direction of the trigger to have it work
			vec3_t ang;
			vectoangles(bs->tacticEntity->movedir, ang);
			VectorCopy(ang, bs->goalAngles);
		}
	}
}

	
//Basically this is the tactical order for attacking an object with a known location
void objectiveType_Attack(bot_state_t *bs, gentity_t *target)
{
	vec3_t objOrigin;
	vec3_t a;
	trace_t tr;
	float dist;

	FindOrigin(target, objOrigin);

	//Do visual check to target
	VectorSubtract(objOrigin, bs->eye, a);
	dist = TargetDistance(bs, target, objOrigin);
	vectoangles(a, a);

	trap_Trace(&tr, bs->eye, NULL, NULL, objOrigin, bs->client, MASK_PLAYERSOLID);

	if (((tr.entityNum == target->s.number || tr.fraction == 1)
		&& (InFieldOfVision(bs->viewangles, 90, a) || bs->cur_ps.groundEntityNum == target->s.number)
		&& !BotMindTricked(bs->client, target->s.number)) 
		|| BotCanHear(bs, target, dist) || dist < 100)
	{//we see the objective, go for it.
		int desiredweap = FavoriteWeapon(bs, target, qtrue, qtrue, 0);
		if((target->flags & FL_DMG_BY_HEAVY_WEAP_ONLY) && !IsHeavyWeapon(bs, desiredweap))
		{//we currently don't have a heavy weap that we can use to destroy this target
			if(HaveHeavyWeapon(bs->cur_ps.stats[STAT_WEAPONS]))
			{//we have a weapon that could destroy this target but we don't have ammo
				//RAFIXME:  at this point we should have the bot go look for some ammo
				//but for now just defend this area.
				TAB_BotDefend(bs, target);
			}
			else if(g_gametype.integer == GT_SIEGE)
			{//ok, check to see if we should switch classes if noone else can blast this
				ShouldSwitchSiegeClasses(bs, qfalse);
				TAB_BotDefend(bs, target);
			}
			else
			{//go hunting for a weapon that can destroy this object
				//RAFIXME:  Add this code
				TAB_BotDefend(bs, target);
			}
		}
		else if((target->flags & FL_DMG_BY_SABER_ONLY) && !(bs->cur_ps.stats[STAT_WEAPONS] & (1 << WP_SABER)) )
		{//This is only damaged by sabers and we don't have a saber
			ShouldSwitchSiegeClasses(bs, qtrue);
			TAB_BotDefend(bs, target);
		}
		else
		{//cleared to attack
			bs->frame_Enemy_Len = dist;
			bs->frame_Enemy_Vis = 1;
			bs->currentEnemy = target;
			VectorCopy(objOrigin, bs->lastEnemySpotted);
			FindAngles(target, bs->lastEnemyAngles);
			bs->enemySeenTime = level.time + BOT_VISUALLOSETRACKTIME;
			TAB_BotBehave_Attack(bs);
		}
		return;
	}
	else if(bs->currentEnemy == target)
	{//can't see the target so null it out so we can find other enemies.
		bs->currentEnemy = NULL;
		bs->frame_Enemy_Vis = 0;
		bs->frame_Enemy_Len = 0;
	}

	if(strcmp(target->classname, "func_breakable") == 0 
		&& target->paintarget
		&& strcmp(target->paintarget, "shieldgen_underattack") == 0)
	{//dirty hack to get the bots to attack the shield generator on siege_hoth
		vec3_t temp;
		VectorSet(temp, -369, 858, -231);
		if(Distance(bs->origin, temp) < DEFEND_MAXDISTANCE)
		{//automatically see target.
			if(!bs->cur_ps.stats[STAT_WEAPONS] & ( 1 << WP_DEMP2)
				&& !bs->cur_ps.stats[STAT_WEAPONS] & ( 1 << WP_ROCKET_LAUNCHER )
				&& !bs->cur_ps.stats[STAT_WEAPONS] & ( 1 << WP_CONCUSSION )
				&& !bs->cur_ps.stats[STAT_WEAPONS] & ( 1 << WP_REPEATER ))
			{//we currently don't have a heavy weap that can reach this target
				TAB_BotDefend(bs, target);
			}
			else
			{//cleared to attack
				bs->frame_Enemy_Len = dist;
				bs->frame_Enemy_Vis = 1;
				bs->currentEnemy = target;
				VectorCopy(objOrigin, bs->lastEnemySpotted);
				FindAngles(target, bs->lastEnemyAngles);
				bs->enemySeenTime = level.time + BOT_VISUALLOSETRACKTIME;
				TAB_BotBehave_Attack(bs);
			}
			return;
		}
		VectorCopy(temp, objOrigin);
	}

	//ok, we can't see the objective, move towards its location
	VectorCopy(objOrigin, bs->DestPosition);
	bs->DestIgnore = target->s.number;
	if(bs->currentEnemy)
	{//have a local enemy, attackmove
		TAB_BotBehave_AttackMove(bs);
	}
	else
	{//normal move
		bs->botBehave = BBEHAVE_MOVETO;
	}	
}


int FlagColorforObjective(bot_state_t *bs)
{
	if(g_entities[bs->client].client->sess.sessionTeam == TEAM_RED) 
	{
		if(bs->objectiveType == OT_CAPTURE)
		{
			return PW_BLUEFLAG;
		}
		else
		{
			return PW_REDFLAG;
		}
	}
	else
	{
		if(bs->objectiveType == OT_CAPTURE)
		{
			return PW_REDFLAG;
		}
		else
		{
			return PW_BLUEFLAG;
		}
	}
}

qboolean CapObjectiveIsCarried(bot_state_t *bs)
{//check to see if the current objective capture item is being carried
	if(g_gametype.integer == GT_SIEGE)
	{
		if(bs->tacticEntity->genericValue2)
			return qtrue;
	}
	else
	{//capture the flag types
		int flagpr, i;
		gentity_t *carrier;

		//Set which flag powerup we're looking for
		flagpr = FlagColorforObjective(bs);

		// check for carrier on desired flag
		for (i = 0; i < g_maxclients.integer; i++) 
		{
			carrier = g_entities + i;
			if (carrier->inuse && carrier->client->ps.powerups[flagpr])
				return qtrue;
		}
	}
			
	return qfalse;
}


gentity_t *CapObjectiveCarrier(bot_state_t *bs)
{//Returns the gentity for the current carrier of the capture objective
	if(g_gametype.integer == GT_SIEGE)
	{
		return &g_entities[bs->tacticEntity->genericValue8];
	}
	else
	{
		int flagpr, i;
		gentity_t *carrier;

		//Set which flag powerup we're looking for
		flagpr = FlagColorforObjective(bs);

		// find attacker's team's flag carrier
		for (i = 0; i < g_maxclients.integer; i++) 
		{
			carrier = g_entities + i;
			if (carrier->inuse && carrier->client->ps.powerups[flagpr])
				return carrier;
		}
	}

	G_Printf("CapObjectiveCarrier() Error: Couldn't find carrier entity.\n");
	return NULL;
}


qboolean CarryingCapObjective(bot_state_t *bs)
{//Carrying the Capture Objective?
	if(g_gametype.integer == GT_SIEGE)
	{
		if(bs->tacticEntity && bs->client == bs->tacticEntity->genericValue8)
			return qtrue;
	}
	else
	{
		if(g_entities[bs->client].client->ps.powerups[PW_REDFLAG] 
		|| g_entities[bs->client].client->ps.powerups[PW_BLUEFLAG])
			return qtrue;
	}
	return qfalse;
}


gentity_t *FindGoalPointEnt( bot_state_t *bs )
{//Find the goalpoint entity for this capture objective point
	if(g_gametype.integer == GT_SIEGE)
	{
		return G_Find( NULL, FOFS( targetname ), bs->tacticEntity->goaltarget );
	}
	else
	{//Capture the flag
		char *c;
		if(g_entities[bs->client].client->sess.sessionTeam == TEAM_RED)
		{
			c = "team_CTF_redflag";
		}
		else
		{
			c = "team_CTF_blueflag";
		}
		return G_Find( NULL, FOFS( classname ), c );
	}
}


void TAB_BotGrabNearByItems(bot_state_t *bs)
{//go around and pick up nearby items that we don't have.
	gentity_t	*ent = NULL;
	gentity_t	*closestEnt = NULL;
	float		closestDist = 9999;
	vec3_t		closestOrigin;
	int			i;

	//find the closest item we'd like to pick up
	for(i = MAX_CLIENTS; i < level.num_entities; i++)
	{
		vec3_t	entOrigin;
		float	entDist;
		ent = &g_entities[i];

		if(ent->s.eType != ET_ITEM || !BG_CanItemBeGrabbed(g_gametype.integer, &ent->s, &bs->cur_ps))
		{//not something we can pick up.
			continue;
		}

		if(bg_itemlist[ent->s.modelindex].giType == IT_TEAM)
		{//ignore team items, like flags.
			continue;
		}

		if((ent->s.eFlags & EF_ITEMPLACEHOLDER) || (ent->s.eFlags & EF_NODRAW))
		{//item has been picked up already
			continue;
		}

		FindOrigin(ent, entOrigin);

		entDist = Distance(bs->origin, entOrigin);

		if(entDist < closestDist)
		{
			closestDist = entDist;
			closestEnt = ent;
			VectorCopy(entOrigin, closestOrigin);
		}
	}

	if(closestEnt)
	{//found a nearby object that we can pick up.
		VectorCopy(closestOrigin, bs->DestPosition);
		bs->DestIgnore = closestEnt->s.number;
		if(bs->currentEnemy)
		{//have a local enemy, attackmove
			TAB_BotBehave_AttackMove(bs);
		}
		else
		{//normal move
			bs->botBehave = BBEHAVE_MOVETO;
		}
	}	
}


extern teamgame_t teamgame;
void objectiveType_Capture(bot_state_t *bs)
{
	if(!bs->tacticEntity)
	{//This is bad
		G_Printf("This is bad, we've lost out entity in objectiveType_Capture.\n");
		return;
	}

	if(CapObjectiveIsCarried(bs))
	{//objective already being carried
		if(CarryingCapObjective(bs))
		{//I'm carrying the flag.
			//find the goaltarget
			gentity_t *goal = NULL;
			goal = FindGoalPointEnt(bs);
			if(goal && !(bs->MiscBotFlags & BOTFLAG_REACHEDCAPTUREPOINT))
			{//found goal position and we haven't already visited the goal point
				vec3_t goalorigin;
				FindOrigin(goal, goalorigin);
				if(g_gametype.integer != GT_SIEGE && DistanceHorizontal(goalorigin, bs->origin) < BOTAI_CAPTUREDISTANCE)
				{//we've touched the goal point and haven't captured the objective.
					//This means that our team's flag isn't there. Flip the waiting
					//for flag behavior
					bs->MiscBotFlags |= BOTFLAG_REACHEDCAPTUREPOINT;
				}

				VectorCopy(goalorigin, bs->DestPosition);
				bs->DestIgnore = goal->s.number;
				if(bs->currentEnemy)
				{
					TAB_BotBehave_AttackMove(bs);
				}
				else
				{
					bs->botBehave = BBEHAVE_MOVETO;
				}
			}
			else
			{//we've already visited our capture point or the capture point isn't valid.
				//Do our "waiting for flag return" behavior here.

				//check to see if our flag has been returned.
				if(g_entities[bs->client].client->sess.sessionTeam == TEAM_RED) 
				{
					if(teamgame.redStatus == FLAG_ATBASE)
					{
						bs->MiscBotFlags &= ~BOTFLAG_REACHEDCAPTUREPOINT;
						return;
					}
					
				}
				else
				{
					if(teamgame.blueStatus == FLAG_ATBASE)
					{
						bs->MiscBotFlags &= ~BOTFLAG_REACHEDCAPTUREPOINT;
						return;
					}
				}

				//ok, flag hasn't returned, wonder around and grab items in the area
				TAB_BotGrabNearByItems(bs);
			}
			return;
		}
		else
		{//someone else is covering the flag, cover them
			TAB_BotDefend(bs, CapObjectiveCarrier(bs));
			return;
		}
	}
	else
	{//not being carried
		//get the flag!
		vec3_t origin;
		FindOrigin(bs->tacticEntity, origin);
		VectorCopy(origin, bs->DestPosition);
		bs->DestIgnore = bs->tacticEntity->s.number;
		if(bs->currentEnemy)
		{
			TAB_BotBehave_AttackMove(bs);
		}
		else
		{
			bs->botBehave = BBEHAVE_MOVETO;
		}
		return;
	}
}


extern gentity_t *droppedBlueFlag;
extern gentity_t *droppedRedFlag;
gentity_t *FindFlag(bot_state_t *bs)
{//find the flag item entity for this bot's objective entity
	if(FlagColorforObjective(bs) == PW_BLUEFLAG)
	{//blue flag
		return droppedBlueFlag;
	}
	else
	{
		return droppedRedFlag;
	}

	//bad flag?!
	return NULL;
}

	
//Prevent this objective from getting captured.
void objectiveType_DefendCapture(bot_state_t *bs)
{
	if(!CapObjectiveIsCarried(bs))
	{
		if(g_gametype.integer == GT_SIEGE)
		{
			//turns out that you don't normally recap capturable siege items.
			TAB_BotDefend(bs, bs->tacticEntity);
		}
		else
		{//flag of some sort, touch it if it is dropped.  Otherwise, just defend it.
			gentity_t *flag = FindFlag(bs);
			if(flag && flag->flags & FL_DROPPED_ITEM)
			{//dropped, touch it
				vec3_t origin;
				FindOrigin(flag, origin);
				VectorCopy(origin, bs->DestPosition);
				bs->DestIgnore = flag->s.number;
				if(bs->currentEnemy)
				{
					TAB_BotBehave_AttackMove(bs);
				}
				else
				{
					bs->botBehave = BBEHAVE_MOVETO;
				}
			}
			else
			{//objective at homebase, defend it
				TAB_BotDefend(bs, bs->tacticEntity);
			}
		}
	}
	else
	{//object has been taken, attack the carrier
		objectiveType_Attack(bs, CapObjectiveCarrier(bs));
	}
}


//vehicle
void objectiveType_Vehicle(bot_state_t *bs)
{
	gentity_t *vehicle = NULL;
	gentity_t *botEnt = &g_entities[bs->client];

	//find the vehicle that must trigger this trigger.
	while ( (vehicle = G_Find( vehicle, FOFS( script_targetname ), bs->tacticEntity->NPC_targetname )) != NULL )
	{
		if (vehicle->inuse && vehicle->client && vehicle->s.eType == ET_NPC &&
			vehicle->s.NPC_class == CLASS_VEHICLE && vehicle->m_pVehicle)
		{
			break;
		}
	}

	if(!vehicle)
	{//can't find the vehicle?!
		return;
	}

	if (botEnt->inuse && botEnt->client 
			&& botEnt->client->ps.m_iVehicleNum == vehicle->s.number)
	{//in the vehicle
		//move towards trigger point
		vec3_t objOrigin;
		FindOrigin(bs->tacticEntity, objOrigin);

		//RAFIXME:  Get rid of that crappy use stuff when we can.
		bs->noUseTime =+ level.time + 5000;

		bs->DestIgnore = bs->tacticEntity->s.number;
		TAB_BotMove(bs, objOrigin, qfalse, qfalse);
	}
	else if(vehicle->client->ps.m_iVehicleNum)
	{//vehicle already occuped, cover it.
		TAB_BotDefend(bs, vehicle);
	}
	else
	{//go to the vehicle!
		//hack!
		vec3_t vehOrigin;
		FindOrigin(vehicle, vehOrigin);

		//bs->useTime = level.time + 100;
				
		bs->botBehave = BBEHAVE_MOVETO;
		VectorCopy(vehOrigin, bs->DestPosition);		
		bs->DestIgnore = vehicle->s.number;
	}	
}


//Siege Objective attack/defend
void TAB_BotObjective(bot_state_t *bs)
{
	//make sure the objective is still valid
	if(g_gametype.integer != GT_SIEGE)
	{
	}
	else if(gSiegeRoundEnded)
	{//round over, don't do anything
		return;
	}
	else if(bs->tacticObjective <= 0 ||
		!ObjectiveStillActive(bs->tacticObjective))
	{//objective lost/completed. switch to first active objective
		//G_Printf("Bot switched objectives due to objective being lost/won.\n");
		bs->tacticObjective = FindValidObjective(-1);
		//bs->tacticObjective = FindFirstActiveObjective(g_entities[bs->client].client->sess.sessionTeam);
		bs->objectiveType = 0;
		if(bs->tacticObjective == -1)
		{//end of map
			//G_Printf("Couldn't find am open objective in TAB_BotObjective()\n");
			return;
		}
	}

	if(!bs->objectiveType || !bs->tacticEntity 
		|| strcmp(bs->tacticEntity->classname, "freed") == 0)
	{//don't have objective entity type, don't have tacticEntity, or the tacticEntity you had
		//was killed/freed
		bs->tacticEntity = DetermineObjectiveType(g_entities[bs->client].client->sess.sessionTeam, 
			bs->tacticObjective, &bs->objectiveType, NULL, 0);
	}

	if(bs->objectiveType == OT_ATTACK)
	{
		//attack tactical code
		objectiveType_Attack(bs, bs->tacticEntity);
	}
	else if(bs->objectiveType == OT_DEFEND)
	{//defend tactical code
		TAB_BotDefend(bs, bs->tacticEntity);
	}
	else if(bs->objectiveType == OT_CAPTURE)
	{//capture tactical code
		objectiveType_Capture(bs);
	}
	else if(bs->objectiveType == OT_DEFENDCAPTURE)
	{//defend capture tactical
		objectiveType_DefendCapture(bs);
	}
	else if(bs->objectiveType == OT_TOUCH)
	{//touch tactical
		objectiveType_Touch(bs);
	}
	else if(bs->objectiveType == OT_VEHICLE)
	{//vehicle techical
		objectiveType_Vehicle(bs);
	}
	else if(bs->objectiveType == OT_WAIT)
	{//just run around and attack people, since we're waiting for the objective to become valid.
		TAB_BotSearchAndDestroy(bs);
	}
	else
	{
		G_Printf("Bad/Unknown ObjectiveType in TAB_BotObjective.\n");
	}
}


//Find the origin location of a given entity
void FindOrigin(gentity_t *ent, vec3_t origin)
{
	if(!ent->classname)
	{//some funky entity, just set vec3_origin
		VectorCopy(vec3_origin, origin);
		return;
	}

	if(ent->client)
	{
		VectorCopy(ent->client->ps.origin, origin);
	}
	else
	{
		if(strcmp(ent->classname, "func_breakable") == 0 
			|| strcmp(ent->classname, "trigger_multiple") == 0
			|| strcmp(ent->classname, "trigger_once") == 0
			|| strcmp(ent->classname, "func_usable") == 0)
		{//func_breakable's don't have normal origin data
			origin[0] = (ent->r.absmax[0]+ent->r.absmin[0])/2;
			origin[1] = (ent->r.absmax[1]+ent->r.absmin[1])/2;
			origin[2] = (ent->r.absmax[2]+ent->r.absmin[2])/2;
		}
		else
		{
			VectorCopy(ent->r.currentOrigin, origin);
		}
	}

}


//find angles/viewangles for entity
void FindAngles(gentity_t *ent, vec3_t angles)
{
	if(ent->client)
	{//player
		VectorCopy(ent->client->ps.viewangles, angles);
	}
	else
	{//other stuff
		VectorCopy(ent->s.angles, angles);
	}
}


int BotWeapon_Detpack(bot_state_t *bs, gentity_t *target)
{
	gentity_t *dp = NULL;
	gentity_t *bestDet = NULL;
	vec3_t TargOrigin;
	float bestDistance = 9999;
	float tempDist;

	FindOrigin(target, TargOrigin);

	while ( (dp = G_Find( dp, FOFS(classname), "detpack") ) != NULL )
	{
		if (dp && dp->parent && dp->parent->s.number == bs->client)
		{
			tempDist = Distance(TargOrigin, dp->s.pos.trBase);
			if(tempDist < bestDistance)
			{
				bestDistance = tempDist;
				bestDet = dp;
			}

			/*
			//check to make sure the det isn't too close to the bot.
			if(Distance(bs->origin, dp->s.pos.trBase) < DETPACK_DETDISTANCE)
			{//we're too close!
				return qfalse;
			}
			*/
		}
	}

	if (!bestDet || bestDistance > DETPACK_DETDISTANCE)
	{
		return qfalse;
	}

	//check to see if the bot knows that the det is near the target.

	//found the closest det to the target and it is in blow distance.
	if(WP_DET_PACK != bs->cur_ps.weapon)
	{//need to switch to desired weapon
		BotSelectChoiceWeapon(bs, WP_DET_PACK, qtrue);
	}
	else
	{//blast it!
		bs->doAltAttack = qtrue;
	}

	return qtrue;
}



void TAB_BotAimLeading(bot_state_t *bs, vec3_t headlevel, float leadAmount)
{//offset the desired view angles with aim leading in mind.
	//based on the original BotAimLeading except it accounts for non-client entities
	//and sets headlevel to the approprate target point instead of setting the goalAngles.
	float x;
	vec3_t movementVector;
	float vtotal;

	if (!bs->currentEnemy ||
		!bs->currentEnemy->client)
	{//we don't need to do any sort of leading if the enemy isn't a client entity.
		return;
	}

	if (!bs->frame_Enemy_Len)
	{//no distance to enemy for this frame?
		return;
	}

	vtotal = 0;

	if (bs->currentEnemy->client->ps.velocity[0] < 0)
	{
		vtotal += -bs->currentEnemy->client->ps.velocity[0];
	}
	else
	{
		vtotal += bs->currentEnemy->client->ps.velocity[0];
	}

	if (bs->currentEnemy->client->ps.velocity[1] < 0)
	{
		vtotal += -bs->currentEnemy->client->ps.velocity[1];
	}
	else
	{
		vtotal += bs->currentEnemy->client->ps.velocity[1];
	}

	if (bs->currentEnemy->client->ps.velocity[2] < 0)
	{
		vtotal += -bs->currentEnemy->client->ps.velocity[2];
	}
	else
	{
		vtotal += bs->currentEnemy->client->ps.velocity[2];
	}

	//G_Printf("Leadin target with a velocity total of %f\n", vtotal);

	VectorCopy(bs->currentEnemy->client->ps.velocity, movementVector);

	VectorNormalize(movementVector);

	x = bs->frame_Enemy_Len*leadAmount; //hardly calculated with an exact science, but it works

	if (vtotal > 400)
	{
		vtotal = 400;
	}

	if (vtotal)
	{
		x = (bs->frame_Enemy_Len*0.9)*leadAmount*(vtotal*0.0012); //hardly calculated with an exact science, but it works
	}
	else
	{
		x = (bs->frame_Enemy_Len*0.9)*leadAmount; //hardly calculated with an exact science, but it works
	}

	headlevel[0] = headlevel[0] + (movementVector[0]*x);
	headlevel[1] = headlevel[1] + (movementVector[1]*x);
	headlevel[2] = headlevel[2] + (movementVector[2]*x);
}


qboolean TAB_ForcePowersThink(bot_state_t *bs)
{//decide which force powers we're going to use.
	qboolean UseTheForce = qfalse;	//flag to let us know that we want to use our current
									//force power

	if((bs->cur_ps.fd.forcePowersKnown & (1 << FP_SPEED)) 
			&& bs->cur_ps.fd.forcePower > forcePowerNeeded[bs->cur_ps.fd.forcePowerLevel[FP_SPEED]][FP_SPEED]
			&& !(bs->cur_ps.fd.forcePowersActive & (1 << FP_SPEED)))
	{//we have force speed and aren't currently using it.
		if(bs->currentTactic == BOTORDER_OBJECTIVE 
			&& bs->objectiveType == OT_CAPTURE
			&& CarryingCapObjective(bs)
			//still trying to move back to capture point.
			&& !(bs->MiscBotFlags & BOTFLAG_REACHEDCAPTUREPOINT)) 
		{//we're trying to capture the flag and we have the flag, try to use
			//force speed to get back to base faster.
			UseTheForce = qtrue;
		}
		else if(bs->currentTactic == BOTORDER_OBJECTIVE 
			&& bs->objectiveType == OT_DEFENDCAPTURE
			&& CapObjectiveIsCarried(bs))
		{//we're defending a capture and someone has captured it, use force speed to try to
			//catch them.
			gentity_t *Carrier = CapObjectiveCarrier(bs);
			if(Distance(Carrier->client->ps.origin, bs->origin) > 100)
			{//only use Force Speed if we're too far away from the carrier.
				UseTheForce = qtrue;
			}
		}
		else if(bs->currentTactic == BOTORDER_OBJECTIVE 
			&& bs->objectiveType == OT_CAPTURE
			&& CapObjectiveIsCarried(bs)
			&& !CarryingCapObjective(bs))
		{//We're trying to cap an objective and someone other than us has the flag.  If the
			//carrier is using force speed, do the same so we can keep up with them.
			gentity_t *Carrier = CapObjectiveCarrier(bs);
			if(Carrier->client->ps.fd.forcePowersActive & (1 << FP_SPEED))
			{
				UseTheForce = qtrue;
			}
		}

		if(UseTheForce)
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_SPEED;
			trap_EA_ForcePower(bs->client);
			return qtrue;
		}
	}

	return qfalse;
}


void TAB_BotJediMaster(bot_state_t *bs)
{//determine what we want to do in JediMaster games.
	if(!gJMSaberEnt)
	{//JM saber not set yet.  probably very beginning of map or error, just kill things.
		bs->tacticEntity = NULL;
		TAB_BotSearchAndDestroy(bs);
	}
	else if(gJMSaberEnt->enemy)
	{//someone has the saber
		if(gJMSaberEnt->enemy->client->ps.clientNum == bs->client)
		{//we have the saber!
			//just run around and kill people.
			bs->tacticEntity = NULL;
			TAB_BotSearchAndDestroy(bs);
		}
		else
		{//someone else has the saber!  Kill them!
			bs->tacticEntity = gJMSaberEnt->enemy;
			if(bs->currentEnemy != gJMSaberEnt->enemy)
			{//we were targetting someone else, switch to JediMaster and update our visuals.
				bs->currentEnemy = gJMSaberEnt->enemy;
				TAB_EnemyVisualUpdate(bs);
			}
			TAB_BotSearchAndDestroy(bs);
		}
	}
	else
	{//saber down!  Get it!
		bs->currentTactic = BOTORDER_NONE;
		bs->tacticEntity = gJMSaberEnt;
		bs->botBehave = BBEHAVE_MOVETO;
		VectorCopy( gJMSaberEnt->r.currentOrigin, bs->DestPosition );
		bs->DestIgnore = bs->tacticEntity->s.number;
	}
		
}


qboolean TryMoveAroundObsticle( bot_state_t *bs, vec3_t moveDir, int targetNum, 
							   vec3_t hitNormal, int tryNum, qboolean CheckBothWays )
{//attempt to move around objects.
		//int tryNum keeps track of the number of checks in each direction to keep it from
			//exploding on extremely complicated curves and such.
		//qboolean CheckBothWays enables/dsables the checking of the other direction if
			//the current way is blocked.  Only use this flag when you first call this
			//function otherwise you could end up with redudent checks.
		
	int movecom = -1;

	if( tryNum > 3 )
	{//ok, don't get stuff in a loop
		return qfalse;
	}

	if(!VectorCompare(hitNormal, vec3_origin))
	{
		vec3_t cross;
		//float rdot;

		//find the perpendicular vector to the normal.
		PerpendicularVector( cross, hitNormal );
		cross[2] = 0;	//flatten the cross vector to 2d.
		VectorNormalize(cross);

		//determine initial movement direction.
		if(bs->evadeTime > level.time)
		{//already going a set direction, keep going that direction.
			if(bs->evadeDir > 3)
			{//going "left".  switch directions.
				VectorScale(cross, -1, cross);
			}
		}
		else
		{//otherwise, try to move in the direction that takes us in the direction we're
			//trying to move.
			float Dot = DotProduct(cross, moveDir);
			if(Dot < 0)
			{//going in the wrong initial direction, switch!
				VectorScale(cross, -1, cross);
			}
		}

		VectorCopy(cross, moveDir);
		movecom = TAB_TraceJumpCrouchFall(bs, moveDir, targetNum, hitNormal);
		
		if(movecom != -1)
		{
			TAB_MovementCommand(bs, movecom, moveDir);
			return qtrue;
		}
		
		if(!VectorCompare(hitNormal, vec3_origin))
		{//hit another surface while trying to trace along this one, try to move along
			//it instead.
			if(TryMoveAroundObsticle(bs, moveDir, targetNum, hitNormal, tryNum+1, qfalse))
			{
				//set the evade timer because this is often where we can get stuck if
				//tracing the wall sends us in some weird direction.
				bs->evadeTime = level.time + 10000;
				return qtrue;
			}
		}

		if(CheckBothWays)
		{//try the other direction
			VectorScale(cross, -1, cross);

			//toggle the evadeDir since we want to continue moving this direction.
			if(bs->evadeDir > 3)
			{//was left, try right
				bs->evadeDir = 0;
			}
			else
			{//was right, try left
				bs->evadeDir = 7;
			}

			
			VectorCopy(cross, moveDir);
			movecom = TAB_TraceJumpCrouchFall(bs, moveDir, targetNum, hitNormal);
			
			if(movecom != -1)
			{
				TAB_MovementCommand(bs, movecom, moveDir);
				//set the evade timer because this is often where we can get stuck if
				//tracing the wall sends us in some weird direction.
				bs->evadeTime = level.time + 10000;
				return qtrue;
			}	

			//try recursively dealing with this.
			return TryMoveAroundObsticle(bs, moveDir, targetNum, hitNormal, tryNum+1, qfalse);
		}
	}

	return qfalse;
}


void TAB_BotSaberDuelChallenged(gentity_t *bot, gentity_t *player)
{//handle things when player just challenged this bot to a saber duel.
	bot_state_t *bs = botstates[bot->s.number];

	if(!bot_honorableduelacceptance.integer)
	{//don't accept duel challenges if bot's aren't supposed to.
		return;
	}

	//RAFIXME - maybe make this be based on the bot's anger level or something?
	if(bs->botOrder != BOTORDER_NONE)
	{//we're preoccupied with another order
		return;
	}

	if(CarryingCapObjective(bs))
	{//don't be stupid and accept duels while you're carrying the flag.
		return;
	}

	//just autoaccept duels right now.
	//go for it.  Remember that this is a temp tactic choice.
	bs->currentTactic = BOTORDER_SABERDUELCHALLENGE;
	bs->tacticEntity = player;

	//debounce the return challenge so the bot doesn't instantly accept.
	bs->MiscBotFlags |= BOTFLAG_SABERCHALLENGED;
	bs->miscBotFlagsTimer = level.time + Q_irand(2000, 5000);
}


void TAB_BotSaberDuel(bot_state_t *bs)
{//current tactic think for engaging in a saber duel.
	gentity_t *self = &g_entities[bs->client];

	if(!bs->tacticEntity || !bs->tacticEntity->inuse)
	{//bad tactic entity!
		G_Printf("This is bad, we've lost out entity in TAB_BotSaberDuel.\n");
		return;
	}

	if(bs->tacticEntity->health < 1)
	{//our tacticEntity is dead, order finished
		bs->currentTactic = BOTORDER_NONE;
		bs->tacticEntity = NULL;
		if(bs->botOrder == BOTORDER_SABERDUELCHALLENGE)
		{
			bs->botOrder = BOTORDER_NONE;
			bs->orderEntity = NULL;
		}
	}
	else if(!self->client->ps.duelInProgress)
	{//we're not currently in a duel, move to our target and challenge them.
		float dist = DistanceHorizontalSquared(bs->origin, 
							bs->tacticEntity->client->ps.origin);

		if(dist > SABERDUELCHALLENGEDIST * SABERDUELCHALLENGEDIST)
		{//too far away to challenge them, move closer.
			bs->botBehave = BBEHAVE_MOVETO;
			VectorCopy(bs->tacticEntity->client->ps.origin, bs->DestPosition);
			bs->DestIgnore = bs->tacticEntity->s.number;
		}
		else
		{//face them and challenge them
			vec3_t viewDir, ang;

			VectorSubtract(bs->tacticEntity->client->ps.origin, bs->eye, viewDir);
			vectoangles(viewDir, ang);
			VectorCopy(ang, bs->goalAngles);

			//need the saber out to do a challenge
			if(bs->cur_ps.weapon != WP_SABER && bs->virtualWeapon != WP_SABER )
			{//don't have saber selected, try switching to it
				BotSelectChoiceWeapon(bs, WP_SABER, 1);
			}
			else
			{//face opponent and send a challenge
				if( !(bs->MiscBotFlags & BOTFLAG_SABERCHALLENGED) 
					|| bs->miscBotFlagsTimer <= level.time)
				{//don't spam the challenge command!
					Cmd_EngageDuel_f(self);
					bs->MiscBotFlags |= BOTFLAG_SABERCHALLENGED;
					bs->miscBotFlagsTimer = level.time + Q_irand(5000, 10000);
				}
			}
		}
	}
	else
	{//we're in a duel, hunt down and kill our duel opponent.
			TAB_BotSearchAndDestroy(bs);

			//also disable the saber challenge flag and debouncer because we've
			//succeeded at starting the duel.
			bs->miscBotFlagsTimer = level.time;
			bs->MiscBotFlags &= ~BOTFLAG_SABERCHALLENGED;
	}
}


void TAB_BotResupply( bot_state_t *bs, gentity_t* tacticEnt )
{//this behavior makes the bot resupply by picking up or using the tacticEntity
	if(tacticEnt)
	{//we have a valid item to use/pickup
		float dist;
		
		if(((tacticEnt->s.eFlags & EF_NODRAW) 
			|| (tacticEnt->s.eFlags & EF_ITEMPLACEHOLDER))
			&& BotPVSCheck(bs->eye, tacticEnt->r.currentOrigin) 
			&& InFOV2( tacticEnt->r.currentOrigin, &g_entities[bs->client], 100, 100 )
			&& OrgVisible(bs->eye, tacticEnt->r.currentOrigin, bs->client) )
		{//This object has been picked up since we last saw it and we can see that it's
			//been taken.  Tactic complete then.
			if(bs->currentTactic == BOTORDER_RESUPPLY)
			{//this is the current tactic, cancel orders
				bs->currentTactic = BOTORDER_NONE;
				bs->tacticEntity = NULL;
			}

			//clear out the nav destination stuff.
			VectorClear(bs->DestPosition);
			bs->DestIgnore = -1;

			if(bs->botOrder == BOTORDER_RESUPPLY)
			{//order completed
				bs->botOrder = BOTORDER_NONE;
				bs->orderEntity = NULL;
			}
			//G_Printf("Bot %i:  Someone took my item.  Resupply finished.\n", bs->client);
			return;
		}

		dist = DistanceSquared(tacticEnt->r.currentOrigin, bs->origin);

		if( dist < (40 * 40) )
		{//we're touching the item
			if(tacticEnt->r.svFlags & SVF_PLAYER_USABLE)
			{//we have to use this item to get ammo from it.
				if(tacticEnt->count && TAB_WantAmmo(bs, qfalse, TAB_FAVWEAPCARELEVEL_MAX))
				{//the ammo system isn't empty and we still want ammo, so keep taking ammo.
					vec3_t viewDir, ang;
					VectorSubtract(tacticEnt->r.currentOrigin, bs->eye, viewDir);
					vectoangles(viewDir, ang);
					VectorCopy(ang, bs->goalAngles);
			
					if(InFieldOfVision(bs->viewangles, 5, ang))
					{//looking at dispensor, press the use button now.
						bs->useTime = level.time + 100;
					}
					return;
				}
			}

			if(bs->currentTactic == BOTORDER_RESUPPLY)
			{//this is the current tactic, cancel orders
				//picked up our item or used it up, stop this tactic and order
				bs->currentTactic = BOTORDER_NONE;
				bs->tacticEntity = NULL;
			}

			//clear out destination data
			VectorClear(bs->DestPosition);
			bs->DestIgnore = -1;

			if(bs->botOrder == BOTORDER_RESUPPLY)
			{//order completed
				bs->botOrder = BOTORDER_NONE;
				bs->orderEntity = NULL;
			}
			//G_Printf("Bot %i:  I got my item.  Resupply finished.\n", bs->client);
		}
		else
		{//keep moving towards object.
			VectorCopy(tacticEnt->r.currentOrigin, bs->DestPosition);
			bs->DestIgnore = tacticEnt->s.number;

			if(bs->currentEnemy)
			{
				TAB_BotBehave_AttackMove(bs);
			}
			else
			{
				bs->botBehave = BBEHAVE_MOVETO;
			}
		}
	}	
}


qboolean G_ThereIsAMaster(void);
int OJP_PassStandardEnemyChecks(bot_state_t *bs, gentity_t *en)
{//checks to see if this potential enemy is a valid one for this bot.  
	//This is a TABBot port of OJP_PassStandardEnemyChecks since TABBots can attack non-client entities, like walls and such.
	if (!bs || !en)
	{ //shouldn't happen
		return 0;
	}

	if (en->health < 1)
	{ //he's already dead
		return 0;
	}

	if (!en->takedamage)
	{ //a client that can't take damage?
		return 0;
	}

	if (!en->s.solid)
	{ //shouldn't happen
		return 0;
	}

	if (bs->client == en->s.number)
	{ //don't attack yourself
		return 0;
	}

	if(en->client)
	{//client based checks.

		if (en->client->ps.pm_type == PM_INTERMISSION ||
			en->client->ps.pm_type == PM_SPECTATOR ||
			en->client->sess.sessionTeam == TEAM_SPECTATOR)
		{ //don't attack spectators
			return 0;
		}

		if (!en->client->pers.connected)
		{ //a "zombie" client?
			return 0;
		}

		if (OnSameTeam(&g_entities[bs->client], en))
		{ //don't attack teammates
			return 0;
		}

		if (BotMindTricked(bs->client, en->s.number))
		{
			if (bs->currentEnemy && bs->currentEnemy->s.number == en->s.number)
			{ //if mindtricked by this enemy, then be less "aware" of them, even though
				//we know they're there.
				vec3_t vs;
				float vLen = 0;

				VectorSubtract(bs->origin, en->client->ps.origin, vs);
				vLen = VectorLength(vs);

				if (vLen > 64 /*&& (level.time - en->client->dangerTime) > 150*/)
				{
					return 0;
				}
			}
		}

		if (en->client->ps.duelInProgress && en->client->ps.duelIndex != bs->client)
		{ //don't attack duelists unless you're dueling them
			return 0;
		}

		if (bs->cur_ps.duelInProgress && en->s.number != bs->cur_ps.duelIndex)
		{ //ditto, the other way around
			return 0;
		}

		if (g_gametype.integer == GT_JEDIMASTER && !en->client->ps.isJediMaster && !bs->cur_ps.isJediMaster && G_ThereIsAMaster())
		{ //rules for attacking non-JM in JM mode
			vec3_t vs;
			float vLen = 0;

			if (!g_friendlyFire.integer)
			{ //can't harm non-JM in JM mode if FF is off
				return 0;
			}

			VectorSubtract(bs->origin, en->client->ps.origin, vs);
			vLen = VectorLength(vs);

			if (vLen > 350)
			{
				return 0;
			}
		}
	}

	return 1;
}


//[/TABBot]

