#include "b_local.h"

extern qboolean FighterSuspended( Vehicle_t *pVeh, playerState_t *parentPS );
extern qboolean FighterIsLanded( Vehicle_t *pVeh, playerState_t *parentPS );

//RACC - Bad and unused function.
/*
void NPC_VehicleAutoPilot ( void )
{
	Vehicle_t *pVeh = NPC->m_pVehicle;
	if ( !pVeh )
	{
		return;
	}

	if ( !NPC->client )
	{
		return;
	}
	if ( !pVeh->m_bAutoPilotActive )
	{	
		if ( !Q_irand( 0, 100 ) )
		{//engage autopilot - next think will execute
			pVeh->m_bAutoPilotActive = qtrue;
			return;
		}
	}

	if ( FighterSuspended( pVeh, &NPC->client->ps ) )
	{//suspended

	}
	else if ( FighterIsLanded( pVeh, &NPC->client->ps ) )
	{//landed, need to take off
		ucmd->upmove = 127;//take off
	}
	else
	{//in flight
		if ( !NPC->enemy )
		{//patrol and find and enemy
			NPC_Vehicle_Patrol();
		}
		else
		{//track down the enemy and kill them
			NPC_Vehicle_HuntEnemy();
		}
	}
}
*/