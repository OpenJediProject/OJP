//[dynamicMusic]

//dynamic music code header
#define DMS_INFO_SIZE	30000	//size of DMS file buffer

#define DMS_DEATH_MUSIC		"music/death_music.mp3"	//death music for the DMS
#define DMS_DEATH_MUSIC_TIME 5000	//length of death music
#define MAX_DMS_TRANSITIONS	4	//maximum possible number of DMS transitions
#define MAX_DMS_EXITPOINTS	8	//maximum number of exit points per transition 
								//music
//filename for DMS music length filename
#define DMS_MUSICLENGTH_FILENAME "ext_data/musiclength.dat"	
#define DMS_MUSICFILE_DEFAULT 21000	//default musicfile length

#define DMS_TRANSITIONFUDGEFACTOR	500 //how close do we have to be to the exit point 
										//to use it. in msecs.

//dynamic music stats
typedef enum //# dynamicMusic_e
{
	DM_AUTO,	//# let the game determine the dynamic music as normal
	DM_SILENCE,	//# stop the music
	DM_EXPLORE,	//# force the exploration music to play
	DM_ACTION,	//# force the action music to play
	DM_BOSS,	//# force the boss battle music to play (if there is any)
	DM_DEATH	//# force the "player dead" music to play
} dynamicMusic_t;

struct DynamicTransition_s
{
	char fileName[MAX_QPATH];
	int fileLength;
	int exitPoints[MAX_DMS_EXITPOINTS];
	int numExitPoints;
};

typedef struct DynamicTransition_s DynamicTransition_t;

struct DynamicMusicSet_s
{
	char fileName[MAX_QPATH];
	int	 fileLength;
	DynamicTransition_t Transitions[MAX_DMS_TRANSITIONS];
	int numTransitions;
	qboolean valid;
};

typedef struct DynamicMusicSet_s DynamicMusicSet_t;

struct DynamicMusicGroup_s
{
	DynamicMusicSet_t exploreMusic;
	DynamicMusicSet_t actionMusic;
	DynamicMusicSet_t bossMusic;

	int	dmState;			//current dynamic music state
	int olddmState;			
	int dmDebounceTime;		//debouncer for music states
	int dmBeatTime;			//debouncer for dynamic state monitoring (transitions between
							//action and explore when the state isn't forced).
	int dmStartTime;		//used by transitioning song states to determine which/went to 
							//transition.
	qboolean valid;			//is the DMS in use?
};

typedef struct DynamicMusicGroup_s DynamicMusicGroup_t;

extern DynamicMusicGroup_t DMSData;		//holds all our dynamic music data

void SetDMSState( int DMSState );
void G_DynamicMusicUpdate( void );
//[/dynamicMusic]

