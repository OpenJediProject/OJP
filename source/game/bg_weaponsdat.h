//[WEAPONSDAT]
#ifndef _WEAPONSDAT_H
#define _WEAPONSDAT_H

#define WEAPON_INFO_SIZE	30000	//max size allowed for weapons.dat file.

#ifdef QAGAME	//only do fireFunc stuff game side.
extern qboolean WP_CallFireFunction( int, qboolean, gentity_t* );

extern void WP_PrimaryFireStunBaton( gentity_t* );
extern void WP_PrimaryFireMelee( gentity_t* );
extern void WP_PrimaryFireBryarPistol( gentity_t* );
extern void WP_PrimaryFireConcussion( gentity_t* );
extern void WP_PrimaryFireBryarPistol( gentity_t* );
extern void WP_PrimaryFireBlaster( gentity_t* );
extern void WP_PrimaryFireDisruptor( gentity_t* );
extern void WP_PrimaryFireBowcaster( gentity_t* );
extern void WP_PrimaryFireRepeater( gentity_t* );
extern void WP_PrimaryFireDEMP2( gentity_t* );
extern void WP_PrimaryFireFlechette( gentity_t* );
extern void WP_PrimaryFireRocket( gentity_t* );
extern void WP_PrimaryFireThermalDetonator( gentity_t* );
extern void WP_PrimaryPlaceLaserTrap( gentity_t* );
extern void WP_PrimaryDropDetPack( gentity_t* );
extern void WP_PrimaryFireEmplaced( gentity_t* );

extern void WP_AltFireStunBaton( gentity_t* );
extern void WP_AltFireMelee( gentity_t* );
extern void WP_AltFireBryarPistol( gentity_t* );
extern void WP_AltFireConcussion( gentity_t* );
extern void WP_AltFireBryarPistol( gentity_t* );
extern void WP_AltFireBlaster( gentity_t* );
extern void WP_AltFireDisruptor( gentity_t* );
extern void WP_AltFireBowcaster( gentity_t* );
extern void WP_AltFireRepeater( gentity_t* );
extern void WP_AltFireDEMP2( gentity_t* );
extern void WP_AltFireFlechette( gentity_t* );
extern void WP_AltFireRocket( gentity_t* );
extern void WP_AltFireThermalDetonator( gentity_t* );
extern void WP_AltPlaceLaserTrap( gentity_t* );
extern void WP_AltDropDetPack( gentity_t* );
extern void WP_AltFireEmplaced( gentity_t* );

typedef void (*FnFireWeapon_t)(gentity_t*);
typedef struct {
	const char* name;
	FnFireWeapon_t func;
} weaponFuncMap_t;
static weaponFuncMap_t fireFunctions[] = {
	{ NULL, NULL },
	{ "firestunbaton", WP_PrimaryFireStunBaton },
	{ "altfirestunbaton", WP_AltFireStunBaton },
	{ "firemelee", WP_PrimaryFireMelee },
	{ "altfiremelee", WP_AltFireMelee },
	{ "firebryarpistol", WP_PrimaryFireBryarPistol },
	{ "altfirebryarpistol", WP_AltFireBryarPistol },
	{ "fireconcussion", WP_PrimaryFireConcussion },
	{ "altfireconcussion", WP_AltFireConcussion },
	{ "fireblasterpistol", WP_PrimaryFireBryarPistol },
	{ "altfireblasterpistol", WP_AltFireBryarPistol },
	{ "fireblaster", WP_PrimaryFireBlaster },
	{ "altfireblaster", WP_AltFireBlaster },
	{ "firedisruptor", WP_PrimaryFireDisruptor },
	{ "altfiredisruptor", WP_AltFireDisruptor },
	{ "firebowcaster", WP_PrimaryFireBowcaster },
	{ "altfirebowcaster", WP_AltFireBowcaster },
	{ "firerepeater", WP_PrimaryFireRepeater },
	{ "altfirerepeater", WP_AltFireRepeater },
	{ "firedemp2", WP_PrimaryFireDEMP2 },
	{ "altfiredemp2", WP_AltFireDEMP2 },
	{ "fireflechette", WP_PrimaryFireFlechette },
	{ "altfireflechette", WP_AltFireFlechette },
	{ "firerocket", WP_PrimaryFireRocket },
	{ "altfirerocket", WP_AltFireRocket },
	{ "firethermaldetonator", WP_PrimaryFireThermalDetonator },
	{ "altfirethermaldetonator", WP_AltFireThermalDetonator },
	{ "placelasertrap", WP_PrimaryPlaceLaserTrap },
	{ "altplacelasertrap", WP_AltPlaceLaserTrap },
	{ "dropdetpack", WP_PrimaryDropDetPack },
	{ "altdropdetpack", WP_AltDropDetPack },
	{ NULL, NULL }
};
#endif


#endif
//[/WEAPONSDAT]

