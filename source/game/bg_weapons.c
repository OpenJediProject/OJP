// Copyright (C) 2001-2002 Raven Software
//
// bg_weapons.c -- part of bg_pmove functionality

#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

// Muzzle point table...
vec3_t WP_MuzzlePoint[WP_NUM_WEAPONS] = 
{//	Fwd,	right,	up.
	{0,		0,		0	},	// WP_NONE,
	{0	,	8,		0	},	// WP_STUN_BATON,
	{0	,	8,		0	},	// WP_MELEE,
	{8	,	16,		0	},	// WP_SABER,				 
	{12,	6,		-6	},	// WP_BRYAR_PISTOL,
	{12,	6,		-6	},	// WP_BLASTER,
	{12,	6,		-6	},	// WP_DISRUPTOR,
	{12,	2,		-6	},	// WP_BOWCASTER,
	{12,	4.5,	-6	},	// WP_REPEATER,
	{12,	6,		-6	},	// WP_DEMP2,
	{12,	6,		-6	},	// WP_FLECHETTE,
	{12,	8,		-4	},	// WP_ROCKET_LAUNCHER,
	{12,	0,		-4	},	// WP_THERMAL,
	{12,	0,		-10	},	// WP_TRIP_MINE,
	{12,	0,		-4	},	// WP_DET_PACK,
	{12,	6,		-6	},	// WP_CONCUSSION
	{12,	6,		-6	},	// WP_BRYAR_OLD,
};

//[DualPistols]
vec3_t WP_MuzzlePoint2[WP_NUM_WEAPONS] = 
{//	Fwd,	right,	up.
	{0,		0,		0	},	// WP_NONE,
	{0	,	8,		0	},	// WP_STUN_BATON,
	{0	,	8,		0	},	// WP_MELEE,
	{8	,	16,		0	},	// WP_SABER,				 
	{12,	-6,		-6	},	// WP_BRYAR_PISTOL,
	{12,	-6,		-6	},	// WP_BLASTER,
	{12,	-6,		-6	},	// WP_DISRUPTOR,
	{12,	-8,		-6	},	// WP_BOWCASTER,
	{12,	-6.5,	-6	},	// WP_REPEATER,
	{12,	-7,		-6	},	// WP_DEMP2,
	{12,	-8,		-6	},	// WP_FLECHETTE,
	{12,	-8,		-4	},	// WP_ROCKET_LAUNCHER,
	//WeaponMod NOTE : en dual thermal on doit décaler aussi celui de la première main
	{12,	-5.5,		-4	},	// WP_THERMAL,
	{12,	0,		-10	},	// WP_TRIP_MINE,
	{12,	0,		-4	},	// WP_DET_PACK,
	{12,	-6,		-6	},	// WP_CONCUSSION		 
	//{12,	-6,		-6	},	// WP_BLASTER_TWIN,
	{12,	-6,		-6	},	// WP_BRYAR_OLD,
};
//[/DualPistols]

weaponData_t weaponData[WP_NUM_WEAPONS] = 
{
	{	// WP_NONE
//		"No Weapon",			//	char	classname[32];		// Spawning name
		AMMO_NONE,				//	int		ammoIndex;			// Index to proper ammo slot
		0,						//	int		ammoLow;			// Count when ammo is low
		0,						//	int		energyPerShot;		// Amount of energy used per shot
		0,						//	int		fireTime;			// Amount of time between firings
		0,						//	int		range;				// Range of weapon
		0,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		0,						//	int		altFireTime;		// Amount of time between alt-firings
		0,						//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		0,						//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		0,						//int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		0						//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_STUN_BATON
//		"Stun Baton",			//	char	classname[32];		// Spawning name
		AMMO_NONE,				//	int		ammoIndex;			// Index to proper ammo slot
		5,						//	int		ammoLow;			// Count when ammo is low
		0,						//	int		energyPerShot;		// Amount of energy used per shot
		400,					//	int		fireTime;			// Amount of time between firings
		8192,					//	int		range;				// Range of weapon
		0,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		400,					//	int		altFireTime;		// Amount of time between alt-firings
		8192,					//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		0,						//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		0,						//int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		0						//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_MELEE
//		"Melee",			//	char	classname[32];		// Spawning name
		AMMO_NONE,				//	int		ammoIndex;			// Index to proper ammo slot
		5,						//	int		ammoLow;			// Count when ammo is low
		0,						//	int		energyPerShot;		// Amount of energy used per shot
		400,					//	int		fireTime;			// Amount of time between firings
		8192,					//	int		range;				// Range of weapon
		0,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		400,					//	int		altFireTime;		// Amount of time between alt-firings
		8192,					//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		0,						//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		0,						//int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		0						//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_SABER,
//		"Lightsaber",			//	char	classname[32];		// Spawning name
		AMMO_NONE,				//	int		ammoIndex;			// Index to proper ammo slot
		5,						//	int		ammoLow;			// Count when ammo is low
		0,						//	int		energyPerShot;		// Amount of energy used per shot
		100,					//	int		fireTime;			// Amount of time between firings
		8192,					//	int		range;				// Range of weapon
		0,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		100,					//	int		altFireTime;		// Amount of time between alt-firings
		8192,					//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		0,						//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		0,						//int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		0						//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_BRYAR_PISTOL,
//		"Bryar Pistol",			//	char	classname[32];		// Spawning name
		AMMO_BLASTER,			//	int		ammoIndex;			// Index to proper ammo slot
		0,//15,						//	int		ammoLow;			// Count when ammo is low
		0,//2,						//	int		energyPerShot;		// Amount of energy used per shot
		//[WeaponSys]
		500,						//	int		fireTime;			// Amount of time between firings was 800
		//800,//400,				//	int		fireTime;			// Amount of time between firings
		//[WeaponSys]
		8192,					//	int		range;				// Range of weapon
		0,//2,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		//[WeaponSys]
		500,						//	int		altFireTime;		// Amount of time between alt-firings
		//800,//400,					//	int		altFireTime;		// Amount of time between alt-firings
		//[/WeaponSys]
		8192,					//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		0,//200,					//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		0,//1,						//int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		0//1500					//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_BLASTER
//		"E11 Blaster Rifle",	//	char	classname[32];		// Spawning name
		AMMO_BLASTER,			//	int		ammoIndex;			// Index to proper ammo slot
		5,						//	int		ammoLow;			// Count when ammo is low
		//[WeaponSys]
		1,						//	int		energyPerShot;		// Amount of energy used per shot was 2
		400,					//	int		fireTime;			// Amount of time between firings was 350
		//2,					//	int		energyPerShot;		// Amount of energy used per shot
		//350,					//	int		fireTime;			// Amount of time between firings
		//[/WeaponSys]
		8192,					//	int		range;				// Range of weapon
		//[WeaponSys]
		1,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire was 3
		250,					//	int		altFireTime;		// Amount of time between alt-firings was 350
		//3,					//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		//150,					//	int		altFireTime;		// Amount of time between alt-firings
		//[/WeaponSys]
		8192,					//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		0,						//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		0,						//int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		0						//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_DISRUPTOR
//		"Tenloss Disruptor Rifle",//	char	classname[32];		// Spawning name
		AMMO_POWERCELL,			//	int		ammoIndex;			// Index to proper ammo slot
		5,						//	int		ammoLow;			// Count when ammo is low
		//[WeaponSys]
		1,						//	int		energyPerShot;		// Amount of energy used per shot
		600,					//	int		fireTime;			// Amount of time between firings
		//5,					//	int		energyPerShot;		// Amount of energy used per shot
		//600,					//	int		fireTime;			// Amount of time between firings
		//[/WeaponSys]
		8192,					//	int		range;				// Range of weapon
		//[WeaponSys]
		1,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		//6,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		//[/WeaponSys]
		1300,					//	int		altFireTime;		// Amount of time between alt-firings
		8192,					//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		200,					//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		3,						//int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		1700					//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_BOWCASTER
//		"Wookiee Bowcaster",	//	char	classname[32];		// Spawning name
		AMMO_POWERCELL,			//	int		ammoIndex;			// Index to proper ammo slot
		5,						//	int		ammoLow;			// Count when ammo is low
		//[WeaponSys]
		1,						//	int		energyPerShot;		// Amount of energy used per shot was 5
		500,					//	int		fireTime;			// Amount of time between firings was 1000
		//5,					//	int		energyPerShot;		// Amount of energy used per shot
		//1000,					//	int		fireTime;			// Amount of time between firings
		//[/WeaponSys]
		8192,					//	int		range;				// Range of weapon
		//[WeaponSys]
		1,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire was 5
		220,					//	int		altFireTime;		// Amount of time between alt-firings was 750 -- was 350 [BowcasterChange]
		//5,					//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		//750,					//	int		altFireTime;		// Amount of time between alt-firings
		//[/WeaponSys]
		8192,					//	int		altRange;			// Range of alt-fire
		400,					//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		0,						//	int		altChargeSubTime;	// above for secondary
		5,						//	int		chargeSub;			// amount to subtract during charge on each interval
		0,						//int		altChargeSub;		// above for secondary
		1700,					//	int		maxCharge;			// stop subtracting once charged for this many ms
		0						//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_REPEATER
//		"Imperial Heavy Repeater",//	char	classname[32];		// Spawning name
		AMMO_METAL_BOLTS,		//	int		ammoIndex;			// Index to proper ammo slot
		5,						//	int		ammoLow;			// Count when ammo is low
		1,						//	int		energyPerShot;		// Amount of energy used per shot
		//[WeaponSys]
		87,						//	int		fireTime;			// Amount of time between firings was 100
//		100,					//	int		fireTime;			// Amount of time between firings
		//[/WeaponSys]
		8192,					//	int		range;				// Range of weapon
		100,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		800,					//	int		altFireTime;		// Amount of time between alt-firings
		8192,					//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		0,						//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		0,						//int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		0						//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_DEMP2
//		"DEMP2",				//	char	classname[32];		// Spawning name
		AMMO_POWERCELL,			//	int		ammoIndex;			// Index to proper ammo slot
		5,						//	int		ammoLow;			// Count when ammo is low
		8,						//	int		energyPerShot;		// Amount of energy used per shot
		500,					//	int		fireTime;			// Amount of time between firings
		8192,					//	int		range;				// Range of weapon
		6,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		900,						//	int		altFireTime;		// Amount of time between alt-firings
		8192,					//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		250,					//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		3,						//	int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		2100					//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_FLECHETTE
//		"Golan Arms Flechette",	//	char	classname[32];		// Spawning name
		AMMO_METAL_BOLTS,		//	int		ammoIndex;			// Index to proper ammo slot
		5,						//	int		ammoLow;			// Count when ammo is low
		1,						//	int		energyPerShot;		// Amount of energy used per shot
		100000,					//	int		fireTime;			// Amount of time between firings
		8192,					//	int		range;				// Range of weapon
		1,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		175,					//	int		altFireTime;		// Amount of time between alt-firings
		8192,					//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		0,						//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		0,						//int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		0						//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_ROCKET_LAUNCHER
//		"Merr-Sonn Missile System",	//	char	classname[32];		// Spawning name
		AMMO_ROCKETS,			//	int		ammoIndex;			// Index to proper ammo slot
		5,						//	int		ammoLow;			// Count when ammo is low
		1,						//	int		energyPerShot;		// Amount of energy used per shot
		//[WeaponSys]
		3600,					//	int		fireTime;			// Amount of time between firings
		//900,					//	int		fireTime;			// Amount of time between firings
		//[/WeaponSys]
		8192,					//	int		range;				// Range of weapon
		//[WeaponSys]
		1,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		3600,					//	int		altFireTime;		// Amount of time between alt-firings
		//2,					//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		//1200,					//	int		altFireTime;		// Amount of time between alt-firings
		//[/WeaponSys]
		8192,					//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		0,						//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		0,						//int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		0						//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_THERMAL
//		"Thermal Detonator",	//	char	classname[32];		// Spawning name
		AMMO_THERMAL,				//	int		ammoIndex;			// Index to proper ammo slot
		0,						//	int		ammoLow;			// Count when ammo is low
		1,						//	int		energyPerShot;		// Amount of energy used per shot
		800,					//	int		fireTime;			// Amount of time between firings
		8192,					//	int		range;				// Range of weapon
		1,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		//[WeaponSys]
		800,					//	int		altFireTime;		// Amount of time between alt-firings
		//400,					//	int		altFireTime;		// Amount of time between alt-firings
		//[/WeaponSys]
		8192,					//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		0,						//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		0,						//int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		0						//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_TRIP_MINE
//		"Trip Mine",			//	char	classname[32];		// Spawning name
		AMMO_TRIPMINE,				//	int		ammoIndex;			// Index to proper ammo slot
		0,						//	int		ammoLow;			// Count when ammo is low
		1,						//	int		energyPerShot;		// Amount of energy used per shot
		800,					//	int		fireTime;			// Amount of time between firings
		8192,					//	int		range;				// Range of weapon
		1,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		400,					//	int		altFireTime;		// Amount of time between alt-firings
		8192,					//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		0,						//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		0,						//int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		0						//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_DET_PACK
//		"Det Pack",				//	char	classname[32];		// Spawning name
		AMMO_DETPACK,				//	int		ammoIndex;			// Index to proper ammo slot
		0,						//	int		ammoLow;			// Count when ammo is low
		1,						//	int		energyPerShot;		// Amount of energy used per shot
		800,					//	int		fireTime;			// Amount of time between firings
		8192,					//	int		range;				// Range of weapon
		0,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		400,					//	int		altFireTime;		// Amount of time between alt-firings
		8192,					//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		0,						//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		0,						//int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		0						//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_CONCUSSION
//		"Concussion Rifle",		//	char	classname[32];		// Spawning name
		AMMO_METAL_BOLTS,		//	int		ammoIndex;			// Index to proper ammo slot
		40,						//	int		ammoLow;			// Count when ammo is low
		40,						//	int		energyPerShot;		// Amount of energy used per shot
		800,					//	int		fireTime;			// Amount of time between firings
		8192,					//	int		range;				// Range of weapon
		50,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		1200,					//	int		altFireTime;		// Amount of time between alt-firings
		8192,					//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		0,						//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		0,						//	int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		0						//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_BRYAR_OLD,
//		"Bryar Pistol",			//	char	classname[32];		// Spawning name
		AMMO_BLASTER,			//	int		ammoIndex;			// Index to proper ammo slot
		15,						//	int		ammoLow;			// Count when ammo is low
		2,						//	int		energyPerShot;		// Amount of energy used per shot
		400,					//	int		fireTime;			// Amount of time between firings
		8192,					//	int		range;				// Range of weapon
		2,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		400,					//	int		altFireTime;		// Amount of time between alt-firings
		8192,					//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		200,					//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		1,						//int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		1500					//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_EMPLCACED_GUN
//		"Emplaced Gun",			//	char	classname[32];		// Spawning name
		/*AMMO_BLASTER*/0,			//	int		ammoIndex;			// Index to proper ammo slot
		/*5*/0,						//	int		ammoLow;			// Count when ammo is low
		/*2*/0,						//	int		energyPerShot;		// Amount of energy used per shot
		100,					//	int		fireTime;			// Amount of time between firings
		8192,					//	int		range;				// Range of weapon
		/*3*/0,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		100,					//	int		altFireTime;		// Amount of time between alt-firings
		8192,					//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		0,						//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		0,						//int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		0						//	int		altMaxCharge;		// above for secondary
	},
	{	// WP_TURRET - NOTE NOT ACTUALLY USEABLE BY PLAYER!
//		"Emplaced Gun",			//	char	classname[32];		// Spawning name
		/*AMMO_BLASTER*/0,			//	int		ammoIndex;			// Index to proper ammo slot
		/*5*/0,						//	int		ammoLow;			// Count when ammo is low
		/*2*/0,						//	int		energyPerShot;		// Amount of energy used per shot
		0,					//	int		fireTime;			// Amount of time between firings
		0,					//	int		range;				// Range of weapon
		/*3*/0,						//	int		altEnergyPerShot;	// Amount of energy used for alt-fire
		0,					//	int		altFireTime;		// Amount of time between alt-firings
		0,					//	int		altRange;			// Range of alt-fire
		0,						//	int		chargeSubTime;		// ms interval for subtracting ammo during charge
		0,						//	int		altChargeSubTime;	// above for secondary
		0,						//	int		chargeSub;			// amount to subtract during charge on each interval
		0,						//int		altChargeSub;		// above for secondary
		0,						//	int		maxCharge;			// stop subtracting once charged for this many ms
		0						//	int		altMaxCharge;		// above for secondary
	}
};

ammoData_t ammoData[AMMO_MAX] = 
{
	{	// AMMO_NONE
//		"",				//	char	icon[32];	// Name of ammo icon file
		0				//	int		max;		// Max amount player can hold of ammo
	},
	{	// AMMO_FORCE
//		"",				//	char	icon[32];	// Name of ammo icon file
		100				//	int		max;		// Max amount player can hold of ammo
	},
	{	// AMMO_BLASTER
//		"",				//	char	icon[32];	// Name of ammo icon file
		300				//	int		max;		// Max amount player can hold of ammo
	},
	{	// AMMO_POWERCELL
//		"",				//	char	icon[32];	// Name of ammo icon file
		300				//	int		max;		// Max amount player can hold of ammo
	},
	{	// AMMO_METAL_BOLTS
//		"",				//	char	icon[32];	// Name of ammo icon file
		//[WeaponSys]
		400				//	int		max;		// Max amount player can hold of ammo; basejka: 300, was 1000
		//300				//	int		max;		// Max amount player can hold of ammo
		//[/WeaponSys]
	},
	{	// AMMO_ROCKETS
//		"",				//	char	icon[32];	// Name of ammo icon file
		//[WeaponSys]
		3				//	int		max;		// Max amount player can hold of ammo
		//25				//	int		max;		// Max amount player can hold of ammo
		//[/WeaponSys]
	},
	{	// AMMO_EMPLACED
//		"",				//	char	icon[32];	// Name of ammo icon file
		800				//	int		max;		// Max amount player can hold of ammo
	},
	{	// AMMO_THERMAL
//		"",				//	char	icon[32];	// Name of ammo icon file
		//[WeaponSys]
		3				//	int		max;		// Max amount player can hold of ammo
		//10				//	int		max;		// Max amount player can hold of ammo
		//[/WeaponSys]
	},
	{	// AMMO_TRIPMINE
//		"",				//	char	icon[32];	// Name of ammo icon file
		10				//	int		max;		// Max amount player can hold of ammo
	},
	{	// AMMO_DETPACK
//		"",				//	char	icon[32];	// Name of ammo icon file
		//[WeaponSys]
		2				//	int		max;		// Max amount player can hold of ammo
		//10				//	int		max;		// Max amount player can hold of ammo
		//[/WeaponSys]
	}
};

ammoData_t ammoPool[NUM_FORCE_POWER_LEVELS][WP_NUM_WEAPONS] = 
{
	{//FORCE_LEVEL_0
		{0},//WP_NONE
		{0},//WP_STUN_BATON
		{0},// WP_MELEE		
		{0},// WP_SABER
		{0},// WP_BRYAR_PISTOL
		{50},// WP_BLASTER	
		{50},// WP_DISRUPTOR	
		{50},// WP_BOWCASTER
		{50},// WP_REPEATER
		{50},// WP_DEMP2
		{50},// WP_FLECHETTE
		{0},// WP_ROCKET_LAUNCHER
		{0},// WP_THERMAL
		{0},// WP_TRIP_MINE
		{0},// WP_DET_PACK
		{50},// WP_CONCUSSION
		{0},// WP_BRYAR_OLD
		{50},// WP_EMPLACED_GUN
		{0},// WP_TURRET
	},
	{//FORCE_LEVEL_1
		{0},//WP_NONE
		{0},//WP_STUN_BATON
		{0},// WP_MELEE		
		{0},// WP_SABER
		{0},// WP_BRYAR_PISTOL
		{100},// WP_BLASTER	
		{100},// WP_DISRUPTOR	
		{100},// WP_BOWCASTER
		{100},// WP_REPEATER
		{100},// WP_DEMP2
		{100},// WP_FLECHETTE
		{0},// WP_ROCKET_LAUNCHER
		{0},// WP_THERMAL
		{0},// WP_TRIP_MINE
		{0},// WP_DET_PACK
		{100},// WP_CONCUSSION
		{0},// WP_BRYAR_OLD
		{100},// WP_EMPLACED_GUN
		{0},// WP_TURRET
	},
	{//FORCE_LEVEL_2
		{0},//WP_NONE
		{0},//WP_STUN_BATON
		{0},// WP_MELEE		
		{0},// WP_SABER
		{0},// WP_BRYAR_PISTOL
		{150},// WP_BLASTER	
		{150},// WP_DISRUPTOR	
		{150},// WP_BOWCASTER
		{150},// WP_REPEATER
		{150},// WP_DEMP2
		{150},// WP_FLECHETTE
		{1},// WP_ROCKET_LAUNCHER
		{0},// WP_THERMAL
		{0},// WP_TRIP_MINE
		{0},// WP_DET_PACK
		{150},// WP_CONCUSSION
		{0},// WP_BRYAR_OLD
		{150},// WP_EMPLACED_GUN
		{0},// WP_TURRET
	},
	{//FORCE_LEVEL_3
		{0},//WP_NONE
		{0},//WP_STUN_BATON
		{0},// WP_MELEE		
		{0},// WP_SABER
		{0},// WP_BRYAR_PISTOL
		{200},// WP_BLASTER	
		{200},// WP_DISRUPTOR	
		{200},// WP_BOWCASTER
		{200},// WP_REPEATER
		{200},// WP_DEMP2
		{200},// WP_FLECHETTE
		{1},// WP_ROCKET_LAUNCHER
		{0},// WP_THERMAL
		{0},// WP_TRIP_MINE
		{0},// WP_DET_PACK
		{200},// WP_CONCUSSION
		{0},// WP_BRYAR_OLD
		{200},// WP_EMPLACED_GUN
		{0},// WP_TURRET
	}
};

int weaponSlots[MAX_WEAP_SLOTS][MAX_WEAPONS_IN_SLOT] = {
	{0,						0,		0,		0	},	// empty slot '0'
	{WP_MELEE,				0,		0,		0	},
	{WP_SABER,				0,		0,		0	},
	{WP_BRYAR_PISTOL,		0,		0,		0	},
	{WP_BLASTER,			WP_BOWCASTER,		WP_REPEATER,		0	},
	{WP_DISRUPTOR,			WP_DEMP2,			WP_FLECHETTE,		0	},
	{WP_ROCKET_LAUNCHER,	WP_CONCUSSION,		0,		0	},
	{WP_THERMAL,			WP_TRIP_MINE,		WP_DET_PACK,		0	},
	{0,						0,		0,		0	}, // extra slot
	{0,						0,		0,		0	}, // extra slot
};
