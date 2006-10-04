//[VisualWeapons]

//header file for holstered weapons data
#ifndef _H_HOLSTER_
#define _H_HOLSTER_

typedef struct holster_s holster_t;
struct holster_s
{
	qhandle_t	boneIndex;			//bolt index to base this weapon off of.
	vec3_t		posOffset;			//the positional offset of the weapon
	vec3_t		angOffset;			//the angular offset of the weapon
};

//enum for the types of holstered weapons you can have
enum
{
	HLR_NONE,
	HLR_SINGLESABER_1,	//first single saber
	HLR_SINGLESABER_2,	//second single saber
	HLR_STAFFSABER,		//staff saber
	HLR_PISTOL_L,		//left hip blaster pistol
	HLR_PISTOL_R,		//right hip blaster pistol
	HLR_BLASTER_L,		//left hip blaster rifle
	HLR_BLASTER_R,		//right hip blaster rifle
	HLR_BRYARPISTOL_L,	//left hip bryer pistol
	HLR_BRYARPISTOL_R,	//right hip bryer pistol
	HLR_BOWCASTER,		//bowcaster
	HLR_ROCKET_LAUNCHER,//rocket launcher
	HLR_DEMP2,			//demp2
	HLR_CONCUSSION,		//concussion
	HLR_REPEATER,		//repeater
	HLR_FLECHETTE,		//flechette
	HLR_DISRUPTOR,		//disruptor
	MAX_HOLSTER		//max possible holster weapon positions
};

enum
{
	HOLSTER_NONE,
	HOLSTER_UPPERBACK,	
	HOLSTER_LOWERBACK,
	HOLSTER_LEFTHIP,
	HOLSTER_RIGHTHIP,
};

//max char size of individual holster.cfg files
#define		MAX_HOLSTER_INFO_SIZE					8192
//max char size for the individual holster.cfg holster type data
#define		MAX_HOLSTER_GROUP_SIZE					MAX_HOLSTER_INFO_SIZE/3

#endif
//[/VisualWeapons]

