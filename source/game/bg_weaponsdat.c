//[WEAPONSDAT]
//g_weaponsdat.c
//Code to parse the weapons.dat file
#include "g_local.h"
#include "bg_weaponsdat.h"

#define WEAPONS_FILE "ext_data/weapons.dat"

//externals
extern int BG_SiegeGetPairedValue(char *buf, char *key, char *outbuf);
extern int BG_SiegeGetValueGroup(char *buf, char *group, char *outbuf);

//ammo type name lookup table.
stringID_table_t AmmoTable[] =
{
	ENUM2STRING(AMMO_NONE),
	ENUM2STRING(AMMO_FORCE),
	ENUM2STRING(AMMO_BLASTER),
	ENUM2STRING(AMMO_POWERCELL),
	ENUM2STRING(AMMO_METAL_BOLTS),
	ENUM2STRING(AMMO_ROCKETS),
	ENUM2STRING(AMMO_EMPLACED),
	ENUM2STRING(AMMO_THERMAL),
	ENUM2STRING(AMMO_TRIPMINE),
	ENUM2STRING(AMMO_DETPACK),
	"", 0 //AMMO_MAX
};

#ifdef QAGAME	//only do fireFunc stuff game side.
static int FindWeaponFuncValue( const char *WeaponFuncName )
{//finds the weapon fire function index for this weapon function name.
	int indx;
	for(indx = 1; fireFunctions[indx].name != NULL; ++indx)
	{
		if(Q_stricmp(WeaponFuncName, fireFunctions[indx].name) == 0)
		{
			return indx;
		}
	}

	return 0;
}
#endif


extern void BG_SiegeStripTabs(char *buf);
char *BG_GetNextValueGroup(char *inbuf, char *outbuf)
{//grabs the next value group that's defined in the inbuf buffer and places it
	//into the oubuf
	int i = 0;
	int j = 0;

	while ( inbuf[i] && inbuf[i] != '{' )
	{//advance to start of value group
		i++;
	}

	if(!inbuf[i])
	{//couldn't find a value group.
		return NULL;
	}

	i++;	//advance past '{'

	while(inbuf[i] && inbuf[i] != '}')
	{//copy all the stuff inside the define group to outbuf
		//Make sure this is a group as opposed to a globally defined value.
		if (inbuf[i] == '/' && inbuf[i+1] == '/')
		{ //stopped on a comment, so first parse to the end of it.
            while (inbuf[i] && inbuf[i] != '\n' && inbuf[i] != '\r')
			{
				i++;
			}
			while (inbuf[i] == '\n' || inbuf[i] == '\r')
			{
				i++;
			}
		}
			
		outbuf[j] = inbuf[i];
		i++;
		j++;
	}

	outbuf[j] = '\0';

	//Verify that we ended up on the closing bracket.
	if (inbuf[i] != '}')
	{
		Com_Error(ERR_DROP, "BG_GetNextValueGroup couldn't find a define group's closing bracket");
		return NULL;
	}

	//Strip the tabs so we're friendly for value parsing.
	BG_SiegeStripTabs(outbuf);

	//slide the buffer pointer to the end of this define group.
	inbuf++;
	inbuf = &inbuf[i];

	return inbuf;
}


extern stringID_table_t WPTable[];
void BG_LoadWeaponsData() 
{
	fileHandle_t	f;
	char buffer[WEAPON_INFO_SIZE];
	char group[WEAPON_INFO_SIZE];
	char *s;
	char text[MAX_QPATH];
	int	 WeaponNum;

	int len = trap_FS_FOpenFile(WEAPONS_FILE, &f, FS_READ);
	if (!f)
	{
		Com_Printf("BG_LoadWeaponsData Error: File Not Found: %s\n", WEAPONS_FILE);
		return;
	}

	trap_FS_Read(buffer, len, f);
	trap_FS_FCloseFile( f );

	s = buffer;

	while( ((s = BG_GetNextValueGroup(s, group)) != NULL)
		&& BG_SiegeGetPairedValue(group, "weapontype", text) )
	{//keep scanning thru the weapon define groups until we're out of define groups.
		//find the weapondata
		int ammotype = -1;
		WeaponNum = GetIDForString( WPTable, text );

		if(WeaponNum == -1)
		{//ok, maybe it's an ammoData define.  Check for that
			WeaponNum = GetIDForString( AmmoTable, text );

			if(WeaponNum == -1)
			{//ok, I have no clue what this is then.  skip it
				continue;
			}

			//We have an ammotype define at this point

			if(BG_SiegeGetPairedValue(group, "AMMOMAX", text))
			{//max possible ammo for this ammo type
				ammoData[WeaponNum].max = atoi(text);
			}
			continue;
		}

		if(BG_SiegeGetPairedValue(group, "ammotype", text))
		{//set ammotype
			ammotype = atoi(text);
			if(ammotype == AMMO_FORCE)
			{//forcing AMMO_FORCE weapons to use AMMO_NONE so those weapons don't
				//use ammo.
				weaponData[WeaponNum].ammoIndex = AMMO_NONE;
			}
			else
			{
				weaponData[WeaponNum].ammoIndex = atoi(text);
			}
		}

		if(BG_SiegeGetPairedValue(group, "ammolowcount", text))
		{//set ammoLow
			weaponData[WeaponNum].ammoLow = atoi(text);
		}

		if(BG_SiegeGetPairedValue(group, "energypershot", text))
		{//set energypershot
			if(ammotype == AMMO_FORCE)
			{//AMMO_FORCE weapons don't use ammo.
				weaponData[WeaponNum].energyPerShot = 0;
			}
			else
			{
				weaponData[WeaponNum].energyPerShot = atoi(text);
			}
		}

		if(BG_SiegeGetPairedValue(group, "firetime", text))
		{//set firetime
			weaponData[WeaponNum].fireTime = atoi(text);
		}

		if(BG_SiegeGetPairedValue(group, "range", text))
		{//set range
			weaponData[WeaponNum].range = atoi(text);
		}

		if(BG_SiegeGetPairedValue(group, "altenergypershot", text))
		{//set altenergypershot
			if(ammotype == AMMO_FORCE)
			{//AMMO_FORCE weapons don't use ammo.
				weaponData[WeaponNum].altEnergyPerShot = 0;
			}
			else
			{
				weaponData[WeaponNum].altEnergyPerShot = atoi(text);
			}
		}

		if(BG_SiegeGetPairedValue(group, "altfiretime", text))
		{//set altfiretime
			weaponData[WeaponNum].altFireTime = atoi(text);
		}

		if(BG_SiegeGetPairedValue(group, "altrange", text))
		{//set altenergypershot
			weaponData[WeaponNum].altRange = atoi(text);
		}
#ifdef QAGAME	//only do fireFunc stuff game side.
		if(BG_SiegeGetPairedValue(group, "missileFuncName", text))
		{//set firefunc
			weaponData[WeaponNum].fireFunctionIndex = FindWeaponFuncValue(text);
		}

		if(BG_SiegeGetPairedValue(group, "altmissileFuncName", text))
		{//set firefunc
			weaponData[WeaponNum].altFireFunctionIndex = FindWeaponFuncValue(text);
		}
#endif
	}
}
//[/WEAPONSDAT]

