//[HolocronFiles]
#include "g_local.h"

extern void *B_TempAlloc(int size);
extern void SP_misc_holocron(gentity_t *ent);

//====================================================================================================
// File         : aotctc_holocron.c
// Description  : This .c file is for an external file method of loading JK holocron positions.
// Author       : Unique1
//====================================================================================================

#define MAX_HOLOCRON_POSITIONS 50
qboolean holocrons_loaded = qfalse;
int number_of_holocronpositions = 0; 

//[Linux]
#ifdef __LINUX__
typedef enum
#else
enum
#endif
//[/Linux]
{// A list of the possible types..
	HC_HEAL,
	HC_LEVITATION,
	HC_SPEED,
	HC_PUSH,
	HC_PULL,
	HC_TELEPATHY,
	HC_GRIP,
	HC_LIGHTNING,
	HC_RAGE,
	HC_PROTECT,
	HC_ABSORB,
	HC_TEAM_HEAL,
	HC_TEAM_FORCE,
	HC_DRAIN,
	HC_SEE,
	HC_SABER_OFFENSE,
	HC_SABER_DEFENSE,
	HC_SABERTHROW,
	HC_RANDOM,
	
	NUM_HOLOCRON_TYPES
};
typedef int holocrontypes_t;

typedef struct holocrons_s           // Holocon Structure
{
    vec3_t      origin;         // Holocron Origin
	qboolean	inuse;			// Mark if it's been spawned already...
    int         type;           // Holocron Type -- For later... uq1
} holocrons_t; 

holocrons_t holocrons[MAX_HOLOCRON_POSITIONS]; 
short int holocron_table[MAX_HOLOCRON_POSITIONS];

//===========================================================================
// Routine      : Init_Holocron_Table
// Description  : Initialize the holocron table.
void Init_Holocron_Table ( void )
{
	memset(&holocrons, 0, sizeof(holocrons_t));
}

//===========================================================================
// Routine      : AOTCTC_Holocron_Add
// Description  : Adds a holocron position to the array.
void AOTCTC_Holocron_Add ( gentity_t *ent )
{
	if (g_gametype.integer != GT_HOLOCRON)
		return;

	if (number_of_holocronpositions > MAX_HOLOCRON_POSITIONS)
	{
		G_Printf("^3Warning! ^5Hit maximum holocron positions (^7%i^5)!\n", MAX_HOLOCRON_POSITIONS);
		return;
	}

	VectorCopy(ent->r.currentOrigin, holocrons[number_of_holocronpositions].origin);
	holocrons[number_of_holocronpositions].type = HC_RANDOM; // For now randomly lay them from places in the list.

	G_Printf("^5Holocron number ^7%i^5 added at position ^7%f %f %f^5.\n", 
		number_of_holocronpositions, 
		holocrons[number_of_holocronpositions].origin[0],
		holocrons[number_of_holocronpositions].origin[1],
		holocrons[number_of_holocronpositions].origin[2] );

	number_of_holocronpositions++; // Will always be in front of the actual number by one while creating.
}

//===========================================================================
// Routine      : AOTCTC_Holocron_Loadpositions
// Description  : Loads holocron positions from .hpf file on disk
void AOTCTC_Holocron_Loadpositions( void )
{// Does each online player's data.
	char *s, *t;
	int len;
	fileHandle_t	f;
	char *buf;
	//[DynamicMemoryTweaks]
	char	loadPath[MAX_QPATH];
	//[/DynamicMemoryTweaks]
	int statnum = 0;
	float stats[50*3]; // 1 extra.
	int holocron_number = 0;
	//[RawMapName]
	//vmCvar_t		mapname;
	//[/RawMapName]

	G_Printf("^5Loading holocron position table...");

	//[RawMapName]
	//loadPath = (char *)B_TempAlloc(1024*4);
	//trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	//Com_sprintf(loadPath, 1024*4, "holocron_positions/%s.hpf\0", mapname.string);
	Com_sprintf(loadPath, sizeof(loadPath), "holocron_positions/%s.hpf", level.rawmapname);
	//[/RawMapName]

	len = trap_FS_FOpenFile( loadPath, &f, FS_READ );
	if ( !f )
	{
		G_Printf(" ^3FAILED!!!\n");
		G_Printf("^5No file exists! (^3%s^5)\n", loadPath);
		return;
	}
	if ( !len )
	{ //empty file
		G_Printf(" ^3FAILED!!!\n");
		G_Printf("^5Empty file!\n");
		trap_FS_FCloseFile( f );
		return;
	}

	if ( (buf = BG_TempAlloc(len+1)) == 0 )
	{//alloc memory for buffer
		G_Printf(" ^3FAILED!!!\n");
		G_Printf("^5Unable to allocate buffer.\n");
		return;
	}
	trap_FS_Read( buf, len, f );
	trap_FS_FCloseFile( f );

	for (t = s = buf; *t; /* */ ) 
	{
		s = strchr(s, ' ');
		
		if (!s)
			break;

		while (*s == ' ')
			*s++ = 0;

		if (*t)
		{
			if (statnum == 0)
			{
				number_of_holocronpositions = atoi(t);
				
				if (number_of_holocronpositions <= 18)
				{
					G_Printf(" ^3FAILED!!!\n");
					G_Printf("^5You need at least 18 holocron points!\n");
					return;
				}
			}

			stats[statnum] = (float)atof(t);
			statnum++;
		}

		t = s;
	}

	statnum = 1;

	while (holocron_number < number_of_holocronpositions)
	{
		int reference = 0;

		while (reference <= 2)
		{
			holocrons[holocron_number].origin[reference] = stats[statnum];
			statnum++;
			reference++;
		}

		holocron_number++;
	}

	BG_TempFree(len+1);

	G_Printf(" ^3Completed OK.\n");
	G_Printf("^5Total Holocron Positions: ^7%i^5.\n", number_of_holocronpositions);
	holocrons_loaded = qtrue;
}

//===========================================================================
// Routine      : AOTCTC_Holocron_Savepositions
// Description  : Saves holocron positions to a .hpf file on disk
void AOTCTC_Holocron_Savepositions( void )
{
	fileHandle_t	f;
	char			*fileString;
	//[DynamicMemoryTweaks]
	char			savePath[MAX_QPATH];
	//[/DynamicMemoryTweaks]
	//[RawMapName]
	//vmCvar_t		mapname;
	//[/RawMapName]
	char			lineout[MAX_INFO_STRING];
	int				loop = 0;

	number_of_holocronpositions--;

	G_Printf("^7Saving holocron position table.\n");

	fileString = NULL;

	//[RawMapName]
	//savePath = (char *)B_TempAlloc(1024*4);
	//trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	//Com_sprintf(savePath, 1024*4, "holocron_positions/%s.hpf\0", mapname.string);
	Com_sprintf(savePath, sizeof(savePath), "holocron_positions/%s.hpf", level.rawmapname);
	//[/RawMapName]

	trap_FS_FOpenFile(savePath, &f, FS_WRITE);

	if ( !f )
	{
		return;
	}

	Com_sprintf( lineout, sizeof(lineout), "%i ", number_of_holocronpositions);
	trap_FS_Write( lineout, strlen(lineout), f);

	while (loop < number_of_holocronpositions)
	{
		char lineout[MAX_INFO_STRING];

		Com_sprintf( lineout, sizeof(lineout), "%f %f %f ", 
				holocrons[loop].origin[0],
				holocrons[loop].origin[1],
				holocrons[loop].origin[2] );
		
		trap_FS_Write( lineout, strlen(lineout), f);

		loop++;
	}

	G_Printf("^7Holocron Position table saved %i holocron positions to file %s.\n", number_of_holocronpositions, savePath);

	trap_FS_FCloseFile( f );
}

//===========================================================================
// Routine      : AOTCTC_Create_Holocron
// Description  : Put a single holocron on the map...
void AOTCTC_Create_Holocron( int type, vec3_t point )
{// Put a single holocron on the map...
	gentity_t *holocron = G_Spawn();
	vec3_t angles;

	VectorSet(angles, 0, 0, 0);
	G_SetOrigin( holocron, point );
	G_SetAngles( holocron, angles );
	holocron->count = type;
	holocron->classname = "misc_holocron";
	VectorCopy(point, holocron->s.pos.trBase);
	VectorCopy(point, holocron->s.origin);
	trap_LinkEntity(holocron);
	SP_misc_holocron( holocron );
}

//===========================================================================
// Routine      : AOTCTC_Create_Holocron
// Description  : Put holocrons on the map...
void AOTCTC_Create_Holocrons( void )
{// Put holocrons on the map...
	int type = 0;

	// Initialize the table...
	Init_Holocron_Table();

	// Load holocron positions from file...
	AOTCTC_Holocron_Loadpositions();

	if (holocrons_loaded != qtrue)
	{// No holocron file for this map.. Don't try to spawn...
		return;
	}

	while (type < NUM_FORCE_POWERS)
	{// Add each type of holocron in order, but in a randomly chosen position...
		int choice;

		//[ExpSys]
		//disabled the holocrons for force powers we've disabled.
		if (type == HC_TEAM_HEAL || type == HC_TEAM_FORCE || type == HC_DRAIN
			|| type == HC_PROTECT || type == HC_RAGE || type == HC_TELEPATHY || type == HC_HEAL)
		//if (type == HC_TEAM_HEAL || type == HC_TEAM_FORCE)
		//[/ExpSys]
		{// Don't do these...
			type++;
			continue;
		}

		choice = rand()%number_of_holocronpositions;
		
		// Find a point not in use already...
		while (holocrons[choice].inuse)
			choice = rand()%number_of_holocronpositions;

		AOTCTC_Create_Holocron( type, holocrons[choice].origin );

		holocrons[choice].inuse = qtrue;
		type++;
	}
}
//[/HolocronFiles]
