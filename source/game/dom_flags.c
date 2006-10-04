#include "..\game\g_local.h"

//====================================================================================================
// File         : scenario_flags.c
// Description  : This .c file is for an external file method of loading JK scenario flag positions.
// Author       : Unique1
//====================================================================================================

#define MAX_FLAG_POSITIONS 50
qboolean flags_loaded = qfalse;
extern int number_of_flags; 
extern void SP_Spawn_Scenario_Flag_New ( gentity_t* ent );
extern void Spawn_Scenario_Flag_Auto ( vec3_t origin, int teamowner );
extern void *B_TempAlloc(int size);

qboolean flag_file_loaded = qfalse;

//===========================================================================
// Routine      : DominancE_Flag_Add
// Description  : Adds a flag position to the array.
void DominancE_Flag_Add ( vec3_t origin, int team )
{
	if (g_gametype.integer != GT_SUPREMACY)
		return;

	if (number_of_flags > MAX_FLAG_POSITIONS)
	{
		G_Printf("^1*** ^3DominancE^1: ^3Warning! ^5Hit maximum flag positions (^7%i^5)!\n", MAX_FLAG_POSITIONS);
		return;
	}

	Spawn_Scenario_Flag_Auto ( origin, team );

	G_Printf("^1*** ^3DominancE^1: ^5Flag number ^7%i^5 added at position ^7%f %f %f^5.\n", number_of_flags, origin[0], origin[1], origin[2] );

	number_of_flags++; // Will always be in front of the actual number by one while creating.
}

//===========================================================================
// Routine      : DominancE_Flag_Loadpositions
// Description  : Loads flag positions from .flags file on disk
void DominancE_Flag_Loadpositions( void )
{// Does each online player's data.
	char *s, *t;
	int len;
	fileHandle_t	f;
	char *buf;
	char *loadPath;
	int statnum = 0;
	float stats[50*4]; // 1 extra.
//	int loop = 0;
	int holocron_number = 0;
	vmCvar_t		mapname;

	G_Printf("^1*** ^3DominancE^1: ^5Loading scenario flag position table...\n");

	loadPath = (char *)B_TempAlloc(1024*4);

	trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	Com_sprintf(loadPath, 1024*4, "flag_positions/%s.flags\0", mapname.string);

	len = trap_FS_FOpenFile( loadPath, &f, FS_READ );
	if ( !f )
	{
		G_Printf(" ^3FAILED!!!\n");
		G_Printf("^1*** ^3DominancE^1: ^5No file exists! (^3%s^5)\n", loadPath);
		return;
	}
	if ( !len )
	{ //empty file
		G_Printf(" ^3FAILED!!!\n");
		G_Printf("^1*** ^3DominancE^1: ^5Empty file!\n");
		trap_FS_FCloseFile( f );
		return;
	}

	if ( (buf = BG_TempAlloc(len+1)) == 0 )
	{//alloc memory for buffer
		G_Printf(" ^3FAILED!!!\n");
		G_Printf("^1*** ^3DominancE^1: ^5Unable to allocate buffer.\n");
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
				(int)number_of_flags = atoi(t);
				
				if (number_of_flags < 2)
				{
					G_Printf(" ^3FAILED!!!\n");
					G_Printf("^1*** ^3DominancE^1: ^5You need at least 2 flag points!\n");
					return;
				}
				else
				{
					statnum++;
					t = s;
					continue;
				}
			}

			(float)stats[statnum] = (float)atof(va("%s", t));

			statnum++;
		}

		t = s;
	}

	statnum = 1;

	while (holocron_number < number_of_flags)
	{
		int reference = 0;
		vec3_t origin;
		int team = 0;

		while (reference <= 3)
		{
			if (reference <= 2)
			{
				(float)origin[reference] = (float)stats[statnum];
			}
			else
			{
				(int)team = (float)stats[statnum];
			}

			statnum++;
			reference++;
		}

		G_Printf("Origin is %f %f %f.\n", origin[0], origin[1], origin[2]);

		if (origin[0] == 0 && origin[1] == 0)
			break;

		if (origin[0] == 0 && origin[2] == 64)
			break;

		number_of_flags--;
		Spawn_Scenario_Flag_Auto ( origin, team );

		holocron_number++;
	}

	BG_TempFree(len+1);

	//G_Printf("^3Completed OK.\n");
	G_Printf("^1*** ^3DominancE^1: ^5Total Flag Positions: ^7%i^5.\n", number_of_flags);
	flags_loaded = qtrue;
}

//===========================================================================
// Routine      : DominancE_Flags_Savepositions
// Description  : Saves flag positions to a .flags file on disk
void DominancE_Flags_Savepositions( void )
{
	fileHandle_t	f;
	char			*fileString;
	char			*savePath;
	vmCvar_t		mapname;
	char			lineout[MAX_INFO_STRING];
	int				loop = 0;

	number_of_flags--;

	G_Printf("^3*** ^3AIMod: ^7Saving flag position table.\n");

	fileString = NULL;

	savePath = (char *)B_TempAlloc(1024*4);

	trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );

	Com_sprintf(savePath, 1024*4, "flag_positions/%s.flags\0", mapname.string);

	trap_FS_FOpenFile(savePath, &f, FS_WRITE);

	if ( !f )
	{
		return;
	}

	Com_sprintf( lineout, sizeof(lineout), "%i ", number_of_flags+1);
	trap_FS_Write( lineout, strlen(lineout), f);

	while (loop < number_of_flags+1)
	{
		char lineout[MAX_INFO_STRING];

		Com_sprintf( lineout, sizeof(lineout), "%f %f %f %i ", 
				flag_list[loop].flagentity->s.origin[0],
				flag_list[loop].flagentity->s.origin[1],
				flag_list[loop].flagentity->s.origin[2],
				flag_list[loop].flagentity->s.teamowner);
		
		trap_FS_Write( lineout, strlen(lineout), f);

		loop++;
	}

	G_Printf("^3*** ^3AIMod: ^7Flag Position table saved %i flag positions to file %s.\n", number_of_flags+1, savePath);

	trap_FS_FCloseFile( f );
}

//===========================================================================
// Routine      : DominancE_Create_Flags
// Description  : Put flags on the map...
void DominancE_Create_Flags( void )
{// Load and put saved flags on the map...
	if (number_of_flags >= 2)
		return; // Some are on the map.. Don't need to load external file...

	if (!flag_file_loaded)
	{
		DominancE_Flag_Loadpositions();
		flag_file_loaded = qtrue;
	}
}
