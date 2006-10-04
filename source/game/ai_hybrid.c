//[Hybrid]
//includes
#include "g_local.h"
#include "ai_main.h"

//externs
extern bot_state_t	*botstates[MAX_CLIENTS];
extern vmCvar_t bot_fps;
extern vmCvar_t mod_classes;

extern int BotSelectChoiceWeapon(bot_state_t *bs, int weapon, int doselection);
extern int CheckForFunc(vec3_t org, int ignore);
extern float forceJumpStrength[NUM_FORCE_POWER_LEVELS];
extern int BotMindTricked(int botClient, int enemyClient);
extern int BotCanHear(bot_state_t *bs, gentity_t *en, float endist);
extern int PassStandardEnemyChecks(bot_state_t *bs, gentity_t *en);
extern qboolean BotPVSCheck( const vec3_t p1, const vec3_t p2 );
extern int PassLovedOneCheck(bot_state_t *bs, gentity_t *ent);
extern void BotDeathNotify(bot_state_t *bs);
extern void BotReplyGreetings(bot_state_t *bs);
extern void MoveTowardIdealAngles(bot_state_t *bs);
extern int forcePowerNeeded[NUM_FORCE_POWER_LEVELS][NUM_FORCE_POWERS];
extern int ObjectiveDependancy[MAX_OBJECTIVES][MAX_OBJECTIVEDEPENDANCY];
extern qboolean gSiegeRoundEnded;
extern qboolean G_PointInBounds( vec3_t point, vec3_t mins, vec3_t maxs );
extern qboolean G_NameInTriggerClassList(char *list, char *str);
extern qboolean G_HeavyMelee( gentity_t *attacker );
extern void ClearRoute( int Route[MAX_WPARRAY_SIZE] );
extern void SelectBestSiegeClass(int ClientNum, qboolean ForceJoin);
extern void DetermineCTFGoal(bot_state_t *bs);
extern void TAB_ScanforEnemies(bot_state_t *bs);
extern void TAB_BotKneelBeforeZod(bot_state_t *bs);
extern void TAB_BotKneelBeforeZod(bot_state_t *bs);
extern void TAB_BotMoveto(bot_state_t *bs, qboolean strafe);
extern void TAB_BotBehave_VisualScan(bot_state_t *bs);
extern void TAB_BotBeStill(bot_state_t *bs);
extern void TAB_BotSearchAndDestroy(bot_state_t *bs);
extern siegeClass_t *BG_GetClassOnBaseClass(const int team, const short classIndex, const short cntIndex);
extern int FavoriteWeapon(bot_state_t *bs, gentity_t *target);
extern void ResetWPTimers(bot_state_t *bs);
extern void TAB_AdjustforStrafe(bot_state_t *bs, vec3_t moveDir);
extern void TAB_BotObjective(bot_state_t *bs);
extern void FindAngles(gentity_t *ent, vec3_t angles);
extern void FindOrigin(gentity_t *ent, vec3_t origin);
extern gentity_t *GetObjectThatTargets(gentity_t *ent);
extern void TAB_BotBehave_Attack(bot_state_t *bs);
extern void TAB_BotBehave_AttackBasic(bot_state_t *bs, gentity_t* target);
extern float TargetDistance(bot_state_t *bs, gentity_t* target, vec3_t targetorigin);
extern int TAB_GetNearestVisibleWP(vec3_t org, int ignore, int badwp);
extern qboolean AttackLocalBreakable(bot_state_t *bs, vec3_t origin);
extern void TAB_BotBehave_DefendBasic(bot_state_t *bs, vec3_t defpoint);
extern void ShouldSwitchSiegeClasses(bot_state_t *bs, qboolean saber);
extern int NumberofSiegeBasicClass(int team, int BaseClass);
extern void RequestSiegeAssistance(bot_state_t *bs, int BaseClass);
extern qboolean SwitchSiegeIdealClass(bot_state_t *bs, char *idealclass);
extern int NumberofSiegeSpecificClass(int team, const char *classname);
extern int BotWeapon_Detpack(bot_state_t *bs, gentity_t *target);
extern qboolean DontBlockAllies(bot_state_t *bs);
//[HybridDefines]

extern qboolean bot_will_fall[MAX_CLIENTS];
extern int	next_bot_fallcheck[MAX_CLIENTS];
extern vec3_t safePos [MAX_CLIENTS];
extern vec3_t jumpPos [MAX_CLIENTS];
qboolean CheckFall_By_Vectors(vec3_t origin, vec3_t angles, gentity_t *ent);
extern void AIMod_Jump ( bot_state_t *bs );

#ifndef FORCE_LIGHTNING_RADIUS
#define FORCE_LIGHTNING_RADIUS 300
#endif

#ifndef MAX_DRAIN_DISTANCE
#define MAX_DRAIN_DISTANCE 512
#endif

#ifndef MAX_GRIP_DISTANCE
#define MAX_GRIP_DISTANCE 256
#endif

#ifndef MAX_TRICK_DISTANCE
#define MAX_TRICK_DISTANCE 512
#endif

//Guts of the HYBRID Bot's AI - Based on TAB adapted for the project.
void HYBRID_StandardBotAI(bot_state_t *bs, float thinktime)
{
	qboolean UsetheForce = qfalse;
	int forceHostile = 0;

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
		return;

	}

	if(bs->cur_ps.pm_type == PM_INTERMISSION 
		|| g_entities[bs->client].client->sess.sessionTeam == TEAM_SPECTATOR)
	{//in intermission
		//Mash the button to prevent the game from sticking on one level.
		if(g_gametype.integer == GT_SIEGE)
		{//hack to get the bots to spawn into seige games after the game has started
			SelectBestSiegeClass(bs->client, qtrue);
			//siegeClass_t *holdClass = BG_GetClassOnBaseClass( g_entities[bs->client].client->sess.siegeDesiredTeam, irand(SPC_INFANTRY, SPC_HEAVY_WEAPONS), 0);
			//trap_EA_Command(bs->client, va("siegeclass \"%s\"\n", holdClass->name));
		}
		trap_EA_Attack(bs->client);
		return;
	}

	//ripped the death stuff right from the standard AI.
	if (!bs->lastDeadTime)
	{ //just spawned in?
		bs->lastDeadTime = level.time;
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
		{
			BotDeathNotify(bs);
			if (PassLovedOneCheck(bs, bs->lastHurt))
			{
				//CHAT: Died
				bs->chatObject = bs->lastHurt;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "Died", 0);
			}
			else if (!PassLovedOneCheck(bs, bs->lastHurt) &&
				botstates[bs->lastHurt->s.number] &&
				PassLovedOneCheck(botstates[bs->lastHurt->s.number], &g_entities[bs->client]))
			{ //killed by a bot that I love, but that does not love me
				bs->chatObject = bs->lastHurt;
				bs->chatAltObject = NULL;
				BotDoChat(bs, "KilledOnPurposeByLove", 0);
			}

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
	//for now, always do whatever your current order is
	//and jsut attack if you're not given an order.
	if(bs->botOrder == BOTORDER_NONE)
	{
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
		else
		{
			bs->currentTactic = BOTORDER_SEARCHANDDESTROY;
			bs->tacticEntity = NULL;
		}
	}
	else
	{
		bs->currentTactic = bs->botOrder;
		bs->tacticEntity = bs->orderEntity;
	}

	//Scan for enemy targets and update the visual check data
	TAB_ScanforEnemies(bs);

	if (bs->currentEnemy && bs->entitynum < MAX_CLIENTS)
	{
		vec3_t a, ang, b_angle, fwd, trto;

		if (g_entities[bs->entitynum].health <= 0 || bs->currentEnemy->health <= 0 )
		{

		}
		//[PlayerClasses][EnhancedImpliment]
		/*
		else if ( mod_classes.integer == 2 
		&& bs->settings.team == TEAM_RED
		&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_JEDI
		&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_FORCEMASTER )
		{// No jumping for other red team players...

		}
		*/
		//[/PlayerClasses][EnhancedImpliment]
		else
		{
			vec3_t p1, p2, dir;
			float xy;
			qboolean willFall = qfalse;

			bs->goalAngles[PITCH] = 0;
			bs->goalAngles[ROLL] = 0;

			VectorCopy(bs->goalAngles, b_angle);
	
			AngleVectors(b_angle, fwd, NULL, NULL);

			if (!gWPArray && bs->BOTjumpState <= JS_WAITING) // Not in a jump.
			{
				VectorCopy(bs->currentEnemy->r.currentOrigin, trto);
			}

			bs->goalAngles[PITCH] = 0;
			bs->goalAngles[ROLL] = 0;

			VectorCopy(bs->goalAngles, b_angle);
	
			AngleVectors(b_angle, fwd, NULL, NULL);

			if (next_bot_fallcheck[bs->entitynum] < level.time)
			{
				if (CheckFall_By_Vectors(bs->origin, fwd, &g_entities[bs->entitynum]) == qtrue)
					willFall = qtrue;

				if (bot_will_fall[bs->entitynum])

				next_bot_fallcheck[bs->entitynum] = level.time + 50;
				bot_will_fall[bs->entitynum] = willFall;
			}
			else
				willFall = bot_will_fall[bs->entitynum];

			if ( bs->origin[2] > bs->currentEnemy->r.currentOrigin[2] )
			{
				VectorCopy( bs->origin, p1 );
				VectorCopy( bs->currentEnemy->r.currentOrigin, p2 );
			}
			else if ( bs->origin[2] < bs->currentEnemy->r.currentOrigin[2] )
			{
				VectorCopy( bs->currentEnemy->r.currentOrigin, p1 );
				VectorCopy( bs->origin, p2 );
			}
			else
			{
				VectorCopy( bs->origin, p1 );
				VectorCopy( bs->currentEnemy->r.currentOrigin, p2 );
			}

			VectorSubtract( p2, p1, dir );
			dir[2] = 0;

			//Get xy and z diffs
			xy = VectorNormalize( dir );

			// Jumping Stuff.
			if (bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& xy < 300 
				&& bs->currentEnemy->r.currentOrigin[2] > bs->origin[2] + 32
				&& bs->currentEnemy->health > 0
				&& bs->currentEnemy->client
				&& bs->currentEnemy->client->ps.groundEntityNum	!= ENTITYNUM_NONE /*!Falling(bs->currentEnemy)*/)
			{// Jump to enemy. NPC style! They are above us. Do a Jump.
				bs->BOTjumpState = JS_FACING;
				AIMod_Jump ( bs );
				
				VectorCopy(bs->currentEnemy->r.currentOrigin, jumpPos[bs->cur_ps.clientNum]);
			}
			else if (bs->BOTjumpState <= JS_WAITING // Not in a jump right now.
				&& xy > 700 
				&& bs->currentEnemy->r.currentOrigin[2] < bs->origin[2] - 32
				&& bs->currentEnemy->health > 0
				&& bs->currentEnemy->client
				&& bs->currentEnemy->client->ps.groundEntityNum	!= ENTITYNUM_NONE /*&& !Falling(bs->currentEnemy)*/)
			{// Jump to enemy. NPC style! They are below us.
				bs->BOTjumpState = JS_FACING;
				AIMod_Jump ( bs );
				
				VectorCopy(bs->currentEnemy->r.currentOrigin, jumpPos[bs->cur_ps.clientNum]);
			}
			else if (bs->BOTjumpState >= JS_CROUCHING)
			{// Continue any jumps.
				AIMod_Jump ( bs );
			}
		
			if (trto)
			{// We have a possibility.. Check it and maybe use it.
				VectorSubtract(trto, bs->origin, a);
				vectoangles(a, ang);
				VectorCopy(ang, bs->goalAngles);

				if (bs->jumpTime > level.time // In a jump.
					/*|| CheckFall_By_Vectors(bs->origin, ang, &g_entities[bs->entitynum]) == qfalse*/|| willFall) // or we wouldn't fall!
				{
					VectorCopy(trto, bs->goalPosition);			

					VectorSubtract(bs->goalPosition, bs->origin, a);
					vectoangles(a, ang);
					VectorCopy(ang, bs->goalAngles);
				}
				/*else // We would fall. Stay still.
				{
					if (bs->wpCurrent)
					{
						//int wp = GetSharedVisibleWP(&g_entities[bs->cur_ps.clientNum], bs->currentEnemy);
						//bs->wpCurrent = gWPArray[wp];
					}
					else
					{
						VectorCopy(bs->origin, bs->goalPosition);
						bs->beStill = level.time + 1000; //nowhere to go.
					}
				}*/
			}
			else
			{// No location found.
				if (bs->wpCurrent)
				{
					//int wp = GetSharedVisibleWP(&g_entities[bs->cur_ps.clientNum], bs->currentEnemy);
					//bs->wpCurrent = gWPArray[wp];
				}
				//[PlayerClasses][EnhancedImpliment]
				/*
				else if ( mod_classes.integer == 2 
					&& bs->settings.team == TEAM_RED
					&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_JEDI
					&& g_entities[bs->cur_ps.clientNum].client->ps.stats[STAT_CLASSNUMBER] != GCLASS_FORCEMASTER )
				{// No jumping for other red team players...

				}
				*/
				//[/PlayerClasses][EnhancedImpliment]
				else if (bs->jumpTime > level.time) // In a jump.
				{// We could be in jump.. Head to safepos if we have one.
					if (safePos[bs->cur_ps.clientNum])
					{
						VectorCopy(safePos[bs->cur_ps.clientNum], bs->goalPosition);
						VectorSubtract(bs->goalPosition, bs->origin, a);
						vectoangles(a, ang);
						VectorCopy(ang, bs->goalAngles);
					}
					else
					{
						VectorCopy(bs->origin, bs->goalPosition);
						bs->beStill = level.time + 1000; //nowhere to go.
					}
				}
				else
				{
					VectorCopy(bs->origin, bs->goalPosition);
					bs->beStill = level.time + 1000; //nowhere to go.
				}
			}
		}
	}
	//end of new aimod ai

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
	if(bs->cur_ps.lastOnGround + 300 < level.time)
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

	//Force powers are listed in terms of priority

/*	if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH)) && (bs->doForcePush > level.time || bs->cur_ps.fd.forceGripBeingGripped > level.time) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH])
	{//use force push
		level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
		UsetheForce = qtrue;
	}

	if (!UsetheForce && (bs->cur_ps.fd.forcePowersKnown & (1 << FP_PULL)) && bs->doForcePull > level.time && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PULL]][FP_PULL])
	{//use force pull
		level.clients[bs->client].ps.fd.forcePowerSelected = FP_PULL;
		UsetheForce = qtrue;
	}*/
#ifndef FORCEJUMP_INSTANTMETHOD
	if (bs->forceJumpChargeTime > level.time)
	{
		UsetheForce = qtrue;
		forceHostile = 0;
	}

	if (bs->currentEnemy && bs->currentEnemy->client && bs->frame_Enemy_Vis && bs->forceJumpChargeTime < level.time)
#else
	if (bs->currentEnemy && bs->currentEnemy->client && bs->frame_Enemy_Vis)
#endif
	{
		vec3_t a_fo;

		VectorSubtract(bs->currentEnemy->client->ps.origin, bs->eye, a_fo);
		vectoangles(a_fo, a_fo);

		//[NewGameTypes][EnhancedImpliment]
		/*
		if (g_gametype.integer == GT_RPG || g_gametype.integer == GT_COOP)
		{// Coop and RPG have new powers...
			//do this above all things
			if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH)) && (bs->doForcePush > level.time || bs->cur_ps.fd.forceGripBeingGripped > level.time) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
				UsetheForce = 1;
				forceHostile = 1;
			}
			else if (bs->cur_ps.fd.forceSide == FORCE_DARKSIDE)
			{ //try dark side powers
			  //in order of priority top to bottom
				if (g_entities[bs->entitynum].health <= 75 
					&& (bs->cur_ps.fd.forcePowersKnown & (1 << FP_DRAIN)) && bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo) && bs->currentEnemy->client->ps.fd.forcePower > 10 )
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
					UsetheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_LIGHTNING)) && bs->frame_Enemy_Len < FORCE_LIGHTNING_RADIUS && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_LIGHTNING;
					UsetheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) && (bs->cur_ps.fd.forcePowersActive & (1 << FP_GRIP)) && InFieldOfVision(bs->viewangles, 50, a_fo))
				{ //already gripping someone, so hold it
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
					UsetheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) && bs->frame_Enemy_Len < MAX_GRIP_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_GRIP]][FP_GRIP] && InFieldOfVision(bs->viewangles, 50, a_fo))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
					UsetheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_RAGE)) && g_entities[bs->client].health < 25 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_RAGE]][FP_RAGE])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_RAGE;
					UsetheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_DRAIN)) && bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo) && bs->currentEnemy->client->ps.fd.forcePower > 10 && bs->currentEnemy->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
					UsetheForce = 1;
					forceHostile = 1;
				}
			}
			else if (bs->cur_ps.fd.forceSide == FORCE_LIGHTSIDE)
			{ //try light side powers
				if (g_entities[bs->entitynum].health <= 70
					&& bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL) 
					&& level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] )
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
					UsetheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && bs->cur_ps.fd.forceGripCripple &&
					 level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{ //absorb to get out
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					UsetheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && bs->cur_ps.electrifyTime >= level.time &&
					 level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{ //absorb lightning
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					UsetheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_TELEPATHY)) && bs->frame_Enemy_Len < MAX_TRICK_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TELEPATHY]][FP_TELEPATHY] && InFieldOfVision(bs->viewangles, 50, a_fo) && !(bs->currentEnemy->client->ps.fd.forcePowersActive & (1 << FP_SEE)))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TELEPATHY;
					UsetheForce = 1;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && g_entities[bs->client].health < 75 && bs->currentEnemy->client->ps.fd.forceSide == FORCE_DARKSIDE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					UsetheForce = 1;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PROTECT)) && g_entities[bs->client].health < 35 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PROTECT]][FP_PROTECT])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_PROTECT;
					UsetheForce = 1;
					forceHostile = 0;
				}
			}
		}
		else
		*/
		//[/NewGameTypes][EnhancedImpliment]
		{
			//do this above all things
			if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH)) && (bs->doForcePush > level.time || bs->cur_ps.fd.forceGripBeingGripped > level.time) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH] /*&& InFieldOfVision(bs->viewangles, 50, a_fo)*/)
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
				UsetheForce = qtrue;
				forceHostile = 1;
			}
			else if (bs->cur_ps.fd.forceSide == FORCE_DARKSIDE)
			{ //try dark side powers
			  //in order of priority top to bottom
				if (g_entities[bs->entitynum].health <= 75 
					&& (bs->cur_ps.fd.forcePowersKnown & (1 << FP_DRAIN)) && bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo) && bs->currentEnemy->client->ps.fd.forcePower > 10 )
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
					UsetheForce = qtrue;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) && (bs->cur_ps.fd.forcePowersActive & (1 << FP_GRIP)) && InFieldOfVision(bs->viewangles, 50, a_fo))
				{ //already gripping someone, so hold it
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
					UsetheForce = qtrue;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_GRIP)) && bs->frame_Enemy_Len < MAX_GRIP_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_GRIP]][FP_GRIP] && InFieldOfVision(bs->viewangles, 50, a_fo))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_GRIP;
					UsetheForce = qtrue;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_RAGE)) && g_entities[bs->client].health < 25 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_RAGE]][FP_RAGE])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_RAGE;
					UsetheForce = qtrue;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_LIGHTNING)) && bs->frame_Enemy_Len < FORCE_LIGHTNING_RADIUS && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_LIGHTNING;
					UsetheForce = qtrue;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_DRAIN)) && bs->frame_Enemy_Len < MAX_DRAIN_DISTANCE && level.clients[bs->client].ps.fd.forcePower > 50 && InFieldOfVision(bs->viewangles, 50, a_fo) && bs->currentEnemy->client->ps.fd.forcePower > 10 && bs->currentEnemy->client->ps.fd.forceSide == FORCE_LIGHTSIDE)
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_DRAIN;
					UsetheForce = qtrue;
					forceHostile = 1;
				}
			}
			else if (bs->cur_ps.fd.forceSide == FORCE_LIGHTSIDE)
			{ //try light side powers
				if (g_entities[bs->entitynum].health <= 70
					&& bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL) 
					&& level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] )
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
					UsetheForce = qtrue;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && bs->cur_ps.fd.forceGripCripple &&
					 level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{ //absorb to get out
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					UsetheForce = qtrue;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && bs->cur_ps.electrifyTime >= level.time &&
					 level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{ //absorb lightning
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					UsetheForce = qtrue;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_TELEPATHY)) && bs->frame_Enemy_Len < MAX_TRICK_DISTANCE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_TELEPATHY]][FP_TELEPATHY] && InFieldOfVision(bs->viewangles, 50, a_fo) && !(bs->currentEnemy->client->ps.fd.forcePowersActive & (1 << FP_SEE)))
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_TELEPATHY;
					UsetheForce = qtrue;
					forceHostile = 1;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_ABSORB)) && g_entities[bs->client].health < 75 && bs->currentEnemy->client->ps.fd.forceSide == FORCE_DARKSIDE && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_ABSORB]][FP_ABSORB])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_ABSORB;
					UsetheForce = qtrue;
					forceHostile = 0;
				}
				else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PROTECT)) && g_entities[bs->client].health < 35 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PROTECT]][FP_PROTECT])
				{
					level.clients[bs->client].ps.fd.forcePowerSelected = FP_PROTECT;
					UsetheForce = qtrue;
					forceHostile = 0;
				}
			}	
		}

		if (!UsetheForce)
		{ //try neutral powers
			if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PUSH)) && bs->cur_ps.fd.forceGripBeingGripped > level.time && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_PUSH]][FP_PUSH] && InFieldOfVision(bs->viewangles, 50, a_fo))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PUSH;
				UsetheForce = qtrue;
				forceHostile = 1;
			}
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_SPEED)) && g_entities[bs->client].health < 25 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_SPEED]][FP_SPEED])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SPEED;
				UsetheForce = qtrue;
				forceHostile = 0;
			}
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_SEE)) && BotMindTricked(bs->client, bs->currentEnemy->s.number) && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_SEE]][FP_SEE])
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_SEE;
				UsetheForce = qtrue;
				forceHostile = 0;
			}
			else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_PULL)) && bs->frame_Enemy_Len < 256 && level.clients[bs->client].ps.fd.forcePower > 75 && InFieldOfVision(bs->viewangles, 50, a_fo))
			{
				level.clients[bs->client].ps.fd.forcePowerSelected = FP_PULL;
				UsetheForce = qtrue;
				forceHostile = 1;
			}
		}
	}

	if (!UsetheForce)
	{ //try powers that we don't care if we have an enemy for
		if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL)) && g_entities[bs->client].health < 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] && bs->cur_ps.fd.forcePowerLevel[FP_HEAL] > FORCE_LEVEL_1)
		{
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
			UsetheForce = qtrue;
			forceHostile = 0;
		}
		else if ((bs->cur_ps.fd.forcePowersKnown & (1 << FP_HEAL)) && g_entities[bs->client].health < 50 && level.clients[bs->client].ps.fd.forcePower > forcePowerNeeded[level.clients[bs->client].ps.fd.forcePowerLevel[FP_HEAL]][FP_HEAL] && !bs->currentEnemy && bs->isCamping > level.time)
		{ //only meditate and heal if we're camping
			level.clients[bs->client].ps.fd.forcePowerSelected = FP_HEAL;
			UsetheForce = qtrue;
			forceHostile = 0;
		}
	}

	if (UsetheForce && forceHostile)
	{
		if (bs->currentEnemy && bs->currentEnemy->client &&
			!ForcePowerUsableOn(&g_entities[bs->client], bs->currentEnemy, level.clients[bs->client].ps.fd.forcePowerSelected))
		{
			UsetheForce = qfalse;
			forceHostile = 0;
		}
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
//[Hybrid]

