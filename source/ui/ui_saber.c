//
/*
=======================================================================

USER INTERFACE SABER LOADING & DISPLAY CODE

=======================================================================
*/

// leave this at the top of all UI_xxxx files for PCH reasons...
//
//#include "../server/exe_headers.h"
#include "ui_local.h"
#include "ui_shared.h"

//[DynamicMemory_Sabers]

//NOTE: this is working 100% up to the point where trap_TrueMalloc is ONLY defined for game and cgame, NOT ui
//thousand curses upon that.

#define DYNAMICMEMORY_SABERS


#ifndef DYNAMICMEMORY_SABERS
//#define MAX_SABER_DATA_SIZE 0x8000]
#define MAX_SABER_DATA_SIZE 0x80000
#endif
//[/DynamicMemory_Sabers]

// Just make sure that the saber data size is the same
// Damn. OK. Gotta fix this again. Later.

//[DynamicMemory_Sabers]
#ifdef DYNAMICMEMORY_SABERS
char *SaberParms;
//void trap_TrueMalloc(void **ptr, int size);
//void trap_TrueFree(void **ptr);
void UI_AllocMem(void **ptr, int sze);
void UI_FreeMem(void *ptr);
void UI_ReaAllocMem(void **ptr, int sze, int count);

int saberSingleHiltCount;
char **saberSingleHiltInfo;

int saberStaffHiltCount;
char **saberStaffHiltInfo;
#else
static char	SaberParms[MAX_SABER_DATA_SIZE];
#endif
//[/DynamicMemory_Sabers]

qboolean	ui_saber_parms_parsed = qfalse;

static qhandle_t redSaberGlowShader;
static qhandle_t redSaberCoreShader;
static qhandle_t orangeSaberGlowShader;
static qhandle_t orangeSaberCoreShader;
static qhandle_t yellowSaberGlowShader;
static qhandle_t yellowSaberCoreShader;
static qhandle_t greenSaberGlowShader;
static qhandle_t greenSaberCoreShader;
static qhandle_t blueSaberGlowShader;
static qhandle_t blueSaberCoreShader;
static qhandle_t purpleSaberGlowShader;
static qhandle_t purpleSaberCoreShader;

//[RGBSabers]
static qhandle_t rgbSaberCoreShader;
static qhandle_t rgbSaberGlowShader;
static qhandle_t rgbSaberCore2Shader;
static qhandle_t blackSaberGlowShader;
//[/RGBSabers]

void UI_CacheSaberGlowGraphics( void )
{//FIXME: these get fucked by vid_restarts
	redSaberGlowShader			= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/red_glow" );
	redSaberCoreShader			= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/red_line" );
	orangeSaberGlowShader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/orange_glow" );
	orangeSaberCoreShader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/orange_line" );
	yellowSaberGlowShader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/yellow_glow" );
	yellowSaberCoreShader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/yellow_line" );
	greenSaberGlowShader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/green_glow" );
	greenSaberCoreShader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/green_line" );
	blueSaberGlowShader			= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/blue_glow" );
	blueSaberCoreShader			= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/blue_line" );
	purpleSaberGlowShader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/purple_glow" );
	purpleSaberCoreShader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/purple_line" );
	//[RGBSabers]
	rgbSaberGlowShader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/rgb_glow" );
	rgbSaberCoreShader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/rgb_line" );
	rgbSaberCore2Shader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/rgb_core" );
	blackSaberGlowShader		= trap_R_RegisterShaderNoMip( "gfx/effects/sabers/black_glow" );
	//[/RGBSabers]

}

qboolean UI_ParseLiteral( const char **data, const char *string ) 
{
	const char	*token;

	token = COM_ParseExt( data, qtrue );
	if ( token[0] == 0 ) 
	{
		Com_Printf( "unexpected EOF\n" );
		return qtrue;
	}

	if ( Q_stricmp( token, string ) ) 
	{
		Com_Printf( "required string '%s' missing\n", string );
		return qtrue;
	}

	return qfalse;
}

qboolean UI_ParseLiteralSilent( const char **data, const char *string ) 
{
	const char	*token;

	token = COM_ParseExt( data, qtrue );
	if ( token[0] == 0 ) 
	{
		return qtrue;
	}

	if ( Q_stricmp( token, string ) ) 
	{
		return qtrue;
	}

	return qfalse;
}

qboolean UI_SaberParseParm( const char *saberName, const char *parmname, char *saberData ) 
{
	const char	*token;
	const char	*value;
	const char	*p;

	if ( !saberName || !saberName[0] ) 
	{
		return qfalse;
	}

	//try to parse it out
	p = SaberParms;
	// A bogus name is passed in
	COM_BeginParseSession("saberinfo");

	// look for the right saber
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
		{
			return qfalse;
		}

		if ( !Q_stricmp( token, saberName ) ) 
		{
			break;
		}

		SkipBracedSection( &p );
	}
	if ( !p ) 
	{
		return qfalse;
	}

	if ( UI_ParseLiteral( &p, "{" ) ) 
	{
		return qfalse;
	}
		
	// parse the saber info block
	while ( 1 ) 
	{
		token = COM_ParseExt( &p, qtrue );
		if ( !token[0] ) 
		{
			Com_Printf( S_COLOR_RED"ERROR: unexpected EOF while parsing '%s'\n", saberName );
			return qfalse;
		}

		if ( !Q_stricmp( token, "}" ) ) 
		{
			break;
		}

		if ( !Q_stricmp( token, parmname ) ) 
		{
			if ( COM_ParseString( &p, &value ) ) 
			{
				continue;
			}
			strcpy( saberData, value );
			return qtrue;
		}

		SkipRestOfLine( &p );
		continue;
	}

	return qfalse;
}


qboolean UI_SaberModelForSaber( const char *saberName, char *saberModel )
{
	return UI_SaberParseParm( saberName, "saberModel", saberModel );
}

qboolean UI_SaberSkinForSaber( const char *saberName, char *saberSkin )
{
	return UI_SaberParseParm( saberName, "customSkin", saberSkin );
}

qboolean UI_SaberTypeForSaber( const char *saberName, char *saberType )
{
	return UI_SaberParseParm( saberName, "saberType", saberType );
}

int UI_SaberNumBladesForSaber( const char *saberName )
{
	int numBlades;
	char	numBladesString[8]={0};
	UI_SaberParseParm( saberName, "numBlades", numBladesString );
	numBlades = atoi( numBladesString );
	if ( numBlades < 1 )
	{
		numBlades = 1;
	}
	else if ( numBlades > 8 )
	{
		numBlades = 8;
	}
	return numBlades;
}

qboolean UI_SaberShouldDrawBlade( const char *saberName, int bladeNum )
{
	int bladeStyle2Start = 0, noBlade = 0;
	char	bladeStyle2StartString[8]={0};
	char	noBladeString[8]={0};
	UI_SaberParseParm( saberName, "bladeStyle2Start", bladeStyle2StartString );
	if ( bladeStyle2StartString
		&& bladeStyle2StartString[0] )
	{
		bladeStyle2Start = atoi( bladeStyle2StartString );
	}
	if ( bladeStyle2Start
		&& bladeNum >= bladeStyle2Start )
	{//use second blade style
		UI_SaberParseParm( saberName, "noBlade2", noBladeString );
		if ( noBladeString
			&& noBladeString[0] )
		{
			noBlade = atoi( noBladeString );
		}
	}
	else
	{//use first blade style
		UI_SaberParseParm( saberName, "noBlade", noBladeString );
		if ( noBladeString
			&& noBladeString[0] )
		{
			noBlade = atoi( noBladeString );
		}
	}
	return ((qboolean)(noBlade==0));
}


qboolean UI_IsSaberTwoHanded( const char *saberName )
{
	int twoHanded;
	char	twoHandedString[8]={0};
	UI_SaberParseParm( saberName, "twoHanded", twoHandedString );
	if ( !twoHandedString[0] )
	{//not defined defaults to "no"
		return qfalse;
	}
	twoHanded = atoi( twoHandedString );
	return ((qboolean)(twoHanded!=0));
}

float UI_SaberBladeLengthForSaber( const char *saberName, int bladeNum )
{
	char	lengthString[8]={0};
	float	length = 40.0f;
	UI_SaberParseParm( saberName, "saberLength", lengthString );
	if ( lengthString[0] )
	{
		length = atof( lengthString );
		if ( length < 0.0f )
		{
			length = 0.0f;
		}
	}

	UI_SaberParseParm( saberName, va("saberLength%d", bladeNum+1), lengthString );
	if ( lengthString[0] )
	{
		length = atof( lengthString );
		if ( length < 0.0f )
		{
			length = 0.0f;
		}
	}

	return length;
}

float UI_SaberBladeRadiusForSaber( const char *saberName, int bladeNum )
{
	char	radiusString[8]={0};
	float	radius = 3.0f;
	UI_SaberParseParm( saberName, "saberRadius", radiusString );
	if ( radiusString[0] )
	{
		radius = atof( radiusString );
		if ( radius < 0.0f )
		{
			radius = 0.0f;
		}
	}

	UI_SaberParseParm( saberName, va("saberRadius%d", bladeNum+1), radiusString );
	if ( radiusString[0] )
	{
		radius = atof( radiusString );
		if ( radius < 0.0f )
		{
			radius = 0.0f;
		}
	}

	return radius;
}

qboolean UI_SaberProperNameForSaber( const char *saberName, char *saberProperName )
{
	char	stringedSaberName[1024];
	qboolean ret = UI_SaberParseParm( saberName, "name", stringedSaberName );
	// if it's a stringed reference translate it
	if( ret && stringedSaberName && stringedSaberName[0] == '@')
	{
		trap_SP_GetStringTextString(&stringedSaberName[1], saberProperName, 1024);
	}
	else
	{
		// no stringed so just use it as it
		strcpy( saberProperName, stringedSaberName );
	}

	return ret;
	
}

qboolean UI_SaberValidForPlayerInMP( const char *saberName )
{
	char allowed [8]={0};
	if ( !UI_SaberParseParm( saberName, "notInMP", allowed ) )
	{//not defined, default is yes
		return qtrue;
	}
	if ( !allowed[0] )
	{//not defined, default is yes
		return qtrue;
	}
	else
	{//return value
		return ((qboolean)(atoi(allowed)==0));
	}
}

//[DynamicMemory_Sabers]
void UI_FreeSabers(void){
#ifdef DYNAMICMEMORY_SABERS
	UI_FreeMem(SaberParms);
	SaberParms = NULL;
#endif
}
//[/DynamicMemory_Sabers]

void UI_SaberLoadParms( void ) 
{
	int			len, totallen, saberExtFNLen, fileCnt, i;
	char		*holdChar, *marker;
	char		saberExtensionListBuf[2048];			//	The list of file names read in
	fileHandle_t f;
	char buffer[MAX_MENUFILE];
	
	//[DynamicMemory_Sabers]
#ifdef DYNAMICMEMORY_SABERS
	int maxLen;
#endif
	//[/DynamicMemory_Sabers]

	//ui.Printf( "UI Parsing *.sab saber definitions\n" );
	
	ui_saber_parms_parsed = qtrue;
	UI_CacheSaberGlowGraphics();

//[DynamicMemory_Sabers] moved down lower
	/*
	//set where to store the first one
	totallen = 0;
	marker = SaberParms;
	marker[0] = '\0';
	*/

	//we werent initilizing saberExtFNLen, which is Bad
	//this is just a general bug fix, not for the purpose of [DynamicMemory_Sabers]
	saberExtFNLen = 0;
///[DynamicMemory_Sabers]


	//now load in the extra .npc extensions
	fileCnt = trap_FS_GetFileList("ext_data/sabers", ".sab", saberExtensionListBuf, sizeof(saberExtensionListBuf) );

	holdChar = saberExtensionListBuf;

//[DynamicMemory_Sabers]
#ifdef DYNAMICMEMORY_SABERS
	maxLen = 0;
	saberExtFNLen = -1;
	for ( i = 0; i < fileCnt; i++, holdChar += saberExtFNLen + 1 ) {
		saberExtFNLen = strlen( holdChar );
		len = trap_FS_FOpenFile( va( "ext_data/sabers/%s", holdChar), &f, FS_READ );
		if(!f)
			continue;
		trap_FS_FCloseFile(f);
		maxLen += len;
	}
	//what do we do if totallen is zero?  will never happen, but COULD happen in theory...
	//trap_TrueMalloc(&SaberParms, totallen+1); //+1 for null char, needed?
	maxLen++; //for ending null char
	UI_AllocMem((void**)&SaberParms, maxLen);
	if(!SaberParms)
		//ERR_FATAL or any level isnt used with Com_Error
		Com_Error(ERR_FATAL, "Saber parsing: Out of memory!");
	holdChar = saberExtensionListBuf;
#endif
//[/DynamicMemory_Sabers]

//[DynamicMemory_Sabers] moved to here
	//set where to store the first one
	totallen = 0;
	marker = SaberParms;
	marker[0] = '\0';

	saberExtFNLen = -1;
///[DynamicMemory_Sabers]

	for ( i = 0; i < fileCnt; i++, holdChar += saberExtFNLen + 1 ) 
	{
		saberExtFNLen = strlen( holdChar );

		len = trap_FS_FOpenFile( va( "ext_data/sabers/%s", holdChar), &f, FS_READ );

		if (!f)
		{
			continue;
		}

		if ( len == -1 ) 
		{
			Com_Printf( "UI_SaberLoadParms: error reading %s\n", holdChar );
		}
		else
		{
			if (len > sizeof(buffer) )
			{
				Com_Error( ERR_FATAL, "UI_SaberLoadParms: file %s too large to read (max=%d)", holdChar, sizeof(buffer) );
			}
			trap_FS_Read( buffer, len, f );
			trap_FS_FCloseFile( f );
			buffer[len] = 0;

			if ( totallen && *(marker-1) == '}' )
			{//don't let it end on a } because that should be a stand-alone token
				strcat( marker, " " );
				totallen++;
				marker++; 
			}
			len = COM_Compress( buffer );

//[DynamicMemory_Sabers]
#ifndef DYNAMICMEMORY_SABERS
			if ( totallen + len >= MAX_SABER_DATA_SIZE ) {
				Com_Error( ERR_FATAL, "UI_SaberLoadParms: ran out of space before reading %s\n(you must make the .sab files smaller)", holdChar );
			}
#else
			if ( totallen + len >= maxLen ) {
				Com_Error( ERR_FATAL, "UI_SaberLoadParms: ran out of space before reading %s\n(This should never happen)", holdChar );
			}
#endif
//[/DynamicMemory_Sabers]
			strcat( marker, buffer );

			totallen += len;
			marker += len;
		}
	}
}

//[RGBSabers]
void RGB_LerpColor(vec3_t from, vec3_t to, float frac, vec3_t out)
{
	vec3_t diff;
	int i;

	VectorSubtract(to,from,diff);

	VectorCopy(from,out);

	for(i=0;i<3;i++)
		out[i] += diff[i] * frac;

}

int getint(char **buf)
{
	double temp;
	temp = strtod(*buf,buf);
	return (int)temp;
}

void ParseRGBSaber(char * str, vec3_t c)
{
	char *p = str;
	int i;

	for(i=0;i<3;i++)
	{
		c[i] = getint(&p);
		p++;
	}
}

vec3_t  ScriptedColors[10][2]	= {0};
int		ScriptedTimes[10][2]	= {0};
int		ScriptedNum[2]			= {0}; //number of colors
int		ScriptedActualNum[2]	= {0};
int		ScriptedStartTime[2]	= {0};
int		ScriptedEndTime[2]		= {0};

void UI_ParseScriptedSaber(char *script, int snum)
{
	int n =0,l;
	char *p = script;
//	vec3_t yop;

	l = strlen(p);
	p++; //skip the 1st ':'

//	Com_Printf("saber[%i] > %s\n",snum,p);
	while(p[0] && p-script < l && n<10)
	{
		ParseRGBSaber(p,ScriptedColors[n][snum]);
		while(p[0]!=':')
			p++;
		p++;            //skipped ':' 

		ScriptedTimes[n][snum] = getint(&p);

//		VectorCopy(ScriptedColors[n][snum],yop);
//		Com_Printf("saber[%i] > %i %i %i > %i\n",snum,(int)yop[0],(int)yop[1],(int)yop[2],ScriptedTimes[n][snum]);

		p++;
		n++;
	}
	ScriptedNum[snum] = n;
}


void RGB_AdjustSciptedSaberColor(vec3_t color, int n)
{
	int actual;
	float frac;
	int time = uiInfo.uiDC.realTime,i;

//	Com_Printf("%i\n",time);

	if(!ScriptedStartTime[n])
	{
//		Com_Printf("startnewColor\n");
		ScriptedActualNum[n] = 0;
		ScriptedStartTime[n] = time;
		ScriptedEndTime[n] = time + ScriptedTimes[0][n];
	}
	else if(ScriptedEndTime[n] < time)
	{
		ScriptedActualNum[n] = (ScriptedActualNum[n] + 1) % ScriptedNum[n];
		actual = ScriptedActualNum[n];
		ScriptedStartTime[n] = time;
		ScriptedEndTime[n] = time + ScriptedTimes[actual][n];
	}

	actual = ScriptedActualNum[n];

	frac = (float)(time - ScriptedStartTime[n]) / (float)(ScriptedEndTime[n] - ScriptedStartTime[n]);


	if(actual+1 != ScriptedNum[n])
		RGB_LerpColor(ScriptedColors[actual][n], ScriptedColors[actual+1][n], frac, color);
	else
		RGB_LerpColor(ScriptedColors[actual][n], ScriptedColors[0][n], frac, color);

	for(i=0;i<3;i++)
		color[i]/=255;
//	Com_Printf("%i %i %i\n",(int)color[0],(int)color[1],(int)color[2]);

}

#define PIMP_MIN_INTESITY 120

void RGB_RandomRGB(vec3_t c)
{
	int i;
	for(i=0;i<3;i++)
		c[i]=0;

	while(c[0]+c[1]+c[2] < PIMP_MIN_INTESITY)
		for(i=0;i<3;i++)
			c[i] = rand() % 255;

//	Com_Printf("color : %i %i %i\n",(int)c[0],(int)c[1],(int)c[2]);
}

int PimpStartTime[2];
int PimpEndTime[2];
vec3_t PimpColorFrom[2];
vec3_t PimpColorTo[2];

void RGB_AdjustPimpSaberColor(vec3_t color, int n)
{
	int time,i;
	float frac;

	if(!PimpStartTime[n])
	{
		PimpStartTime[n] = uiInfo.uiDC.realTime;
		RGB_RandomRGB(PimpColorFrom[n]);
		RGB_RandomRGB(PimpColorTo[n]);
		time = 250 + rand()%250;
		PimpEndTime[n] = uiInfo.uiDC.realTime + time;
	}
	else if(PimpEndTime[n] < uiInfo.uiDC.realTime)
	{
		VectorCopy(PimpColorTo[n], PimpColorFrom[n]);
		RGB_RandomRGB(PimpColorTo[n]);
		time = 250 + rand()%250;
		PimpStartTime[n] = uiInfo.uiDC.realTime;
		PimpEndTime[n] = uiInfo.uiDC.realTime + time;
	}

	frac = (float)(uiInfo.uiDC.realTime - PimpStartTime[n]) / (float)(PimpEndTime[n] - PimpStartTime[n]);

	RGB_LerpColor(PimpColorFrom[n], PimpColorTo[n], frac, color);

	for(i=0;i<3;i++)
		color[i]/=255;

}

void UI_DoSaber( vec3_t origin, vec3_t dir, float length, float lengthMax, float radius, saber_colors_t color, int snum )
//[/RGBSabers]
{
	vec3_t		mid, rgb={1,1,1};
	qhandle_t	blade = 0, glow = 0;
	refEntity_t saber;
	float radiusmult;
	float radiusRange;
	float radiusStart;

//[RGBSabers]
	int i;
//[/RGBSabers]

	if ( length < 0.5f )
	{
		// if the thing is so short, just forget even adding me.
		return;
	}

	// Find the midpoint of the saber for lighting purposes
	VectorMA( origin, length * 0.5f, dir, mid );

	switch( color )
	{
		case SABER_RED:
			glow = redSaberGlowShader;
			blade = redSaberCoreShader;
			VectorSet( rgb, 1.0f, 0.2f, 0.2f );
			break;
		case SABER_ORANGE:
			glow = orangeSaberGlowShader;
			blade = orangeSaberCoreShader;
			VectorSet( rgb, 1.0f, 0.5f, 0.1f );
			break;
		case SABER_YELLOW:
			glow = yellowSaberGlowShader;
			blade = yellowSaberCoreShader;
			VectorSet( rgb, 1.0f, 1.0f, 0.2f );
			break;
		case SABER_GREEN:
			glow = greenSaberGlowShader;
			blade = greenSaberCoreShader;
			VectorSet( rgb, 0.2f, 1.0f, 0.2f );
			break;
		case SABER_BLUE:
			glow = blueSaberGlowShader;
			blade = blueSaberCoreShader;
			VectorSet( rgb, 0.2f, 0.4f, 1.0f );
			break;
		case SABER_PURPLE:
			glow = purpleSaberGlowShader;
			blade = purpleSaberCoreShader;
			VectorSet( rgb, 0.9f, 0.2f, 1.0f );
			break;
//[RGBSabers]
		case SABER_RGB:
			{
				if(snum == 0)
					VectorSet( rgb, ui_sab1_r.value, ui_sab1_g.value, ui_sab1_b.value );
				else
					VectorSet( rgb, ui_sab2_r.value, ui_sab2_g.value, ui_sab2_b.value );

				for(i=0;i<3;i++)
					rgb[i]/=255;

				glow = rgbSaberGlowShader;
				blade = rgbSaberCoreShader;
			}
			break;
		case SABER_PIMP:
			glow = rgbSaberGlowShader;
			blade = rgbSaberCoreShader;
			RGB_AdjustPimpSaberColor(rgb,snum);
			break;
		case SABER_WHITE:
			glow = rgbSaberGlowShader;
			blade = rgbSaberCoreShader;
			VectorSet( rgb, 1.0f, 1.0f, 1.0f );
			break;
		case SABER_BLACK:
			glow = blackSaberGlowShader;
			blade = rgbSaberCoreShader;
			VectorSet( rgb, .0f, .0f, .0f );
			break;
		case SABER_SCRIPTED:
			glow = rgbSaberGlowShader;
			blade = rgbSaberCoreShader;
			RGB_AdjustSciptedSaberColor(rgb,snum);
			break;
//[/RGBSabers]

	}

	// always add a light because sabers cast a nice glow before they slice you in half!!  or something...
	/*
	if ( doLight )
	{//FIXME: RGB combine all the colors of the sabers you're using into one averaged color!
		cgi_R_AddLightToScene( mid, (length*2.0f) + (random()*8.0f), rgb[0], rgb[1], rgb[2] );
	}
	*/

	memset( &saber, 0, sizeof( refEntity_t ));

	// Saber glow is it's own ref type because it uses a ton of sprites, otherwise it would eat up too many
	//	refEnts to do each glow blob individually
	saber.saberLength = length;

	// Jeff, I did this because I foolishly wished to have a bright halo as the saber is unleashed.  
	// It's not quite what I'd hoped tho.  If you have any ideas, go for it!  --Pat
	if (length < lengthMax )
	{
		radiusmult = 1.0 + (2.0 / length);		// Note this creates a curve, and length cannot be < 0.5.
	}
	else
	{
		radiusmult = 1.0;
	}

	radiusRange = radius * 0.075f;
	radiusStart = radius-radiusRange;

	saber.radius = (radiusStart + crandom() * radiusRange)*radiusmult;
	//saber.radius = (2.8f + crandom() * 0.2f)*radiusmult;


	VectorCopy( origin, saber.origin );
	VectorCopy( dir, saber.axis[0] );
	saber.reType = RT_SABER_GLOW;
	saber.customShader = glow;
//[RGBSabers]
	if(color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
		saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
	else
	{
		int i;
		for(i=0;i<3;i++)
			saber.shaderRGBA[i] = rgb[i]*255;
		saber.shaderRGBA[3] = 255;
	}
//[/RGBSabers]
	//saber.renderfx = rfx;

	trap_R_AddRefEntityToScene( &saber );

	// Do the hot core
	VectorMA( origin, length, dir, saber.origin );
	VectorMA( origin, -1, dir, saber.oldorigin );
	saber.customShader = blade;
	saber.reType = RT_LINE;
	radiusStart = radius/3.0f;
	saber.radius = (radiusStart + crandom() * radiusRange)*radiusmult;
//	saber.radius = (1.0 + crandom() * 0.2f)*radiusmult;

	trap_R_AddRefEntityToScene( &saber );

	//[RGBSabers]
	if(color != SABER_RGB && color != SABER_PIMP && color != SABER_WHITE && color != SABER_SCRIPTED)
		return;

	saber.customShader = rgbSaberCore2Shader;
	saber.reType = RT_LINE;
	saber.shaderTexCoord[0] = saber.shaderTexCoord[1] = 1.0f;
	saber.shaderRGBA[0] = saber.shaderRGBA[1] = saber.shaderRGBA[2] = saber.shaderRGBA[3] = 0xff;
	saber.radius = (radiusStart + crandom() * radiusRange)*radiusmult;
	trap_R_AddRefEntityToScene( &saber );
	//[/RGBSabers]

}

char * SaberColorToString(saber_colors_t color)
{
	if ( color == SABER_RED)
		return "red";
	
	if ( color == SABER_ORANGE)
		return "orange";

	if ( color == SABER_YELLOW)
		return "yellow";

	if ( color == SABER_GREEN)
		return "green";

	if (color == SABER_BLUE)
		return "blue";

	if ( color == SABER_PURPLE)
		return "purple";

	//[RGBSabers]
	if ( color == SABER_WHITE)
		return "white";

	if ( color == SABER_BLACK)
		return "black";

	if ( color == SABER_RGB)
		return "rgb";

	if ( color == SABER_PIMP)
		return "pimp";

	if ( color == SABER_SCRIPTED)
		return "scripted";
	//[/RGBSabers]


	return NULL;
}
saber_colors_t TranslateSaberColor( const char *name ) 
{
	if ( !Q_stricmp( name, "red" ) ) 
	{
		return SABER_RED;
	}
	if ( !Q_stricmp( name, "orange" ) ) 
	{
		return SABER_ORANGE;
	}
	if ( !Q_stricmp( name, "yellow" ) ) 
	{
		return SABER_YELLOW;
	}
	if ( !Q_stricmp( name, "green" ) ) 
	{
		return SABER_GREEN;
	}
	if ( !Q_stricmp( name, "blue" ) ) 
	{
		return SABER_BLUE;
	}
	if ( !Q_stricmp( name, "purple" ) ) 
	{
		return SABER_PURPLE;
	}
	//[RGBSabers]
	if ( !Q_stricmp( name, "rgb" ) ) 
	{
		return SABER_RGB;
	}
	if ( !Q_stricmp( name, "pimp" ) ) 
	{
		return SABER_PIMP;
	}
	if ( !Q_stricmp( name, "white" ) ) 
	{
		return SABER_WHITE;
	}
	if ( !Q_stricmp( name, "black" ) ) 
	{
		return SABER_BLACK;
	}
	if ( !Q_stricmp( name, "scripted" ) ) 
	{
		return SABER_SCRIPTED;
	}
	//[/RGBSabers]
	if ( !Q_stricmp( name, "random" ) ) 
	{
		return ((saber_colors_t)(Q_irand( SABER_ORANGE, SABER_PURPLE )));
	}
	return SABER_BLUE;
}

saberType_t TranslateSaberType( const char *name ) 
{
	if ( !Q_stricmp( name, "SABER_SINGLE" ) ) 
	{
		return SABER_SINGLE;
	}
	if ( !Q_stricmp( name, "SABER_STAFF" ) ) 
	{
		return SABER_STAFF;
	}
	if ( !Q_stricmp( name, "SABER_BROAD" ) ) 
	{
		return SABER_BROAD;
	}
	if ( !Q_stricmp( name, "SABER_PRONG" ) ) 
	{
		return SABER_PRONG;
	}
	if ( !Q_stricmp( name, "SABER_DAGGER" ) ) 
	{
		return SABER_DAGGER;
	}
	if ( !Q_stricmp( name, "SABER_ARC" ) ) 
	{
		return SABER_ARC;
	}
	if ( !Q_stricmp( name, "SABER_SAI" ) ) 
	{
		return SABER_SAI;
	}
	if ( !Q_stricmp( name, "SABER_CLAW" ) ) 
	{
		return SABER_CLAW;
	}
	if ( !Q_stricmp( name, "SABER_LANCE" ) ) 
	{
		return SABER_LANCE;
	}
	if ( !Q_stricmp( name, "SABER_STAR" ) ) 
	{
		return SABER_STAR;
	}
	if ( !Q_stricmp( name, "SABER_TRIDENT" ) ) 
	{
		return SABER_TRIDENT;
	}
	if ( !Q_stricmp( name, "SABER_SITH_SWORD" ) ) 
	{
		return SABER_SITH_SWORD;
	}
	return SABER_SINGLE;
}

void UI_SaberDrawBlade( itemDef_t *item, char *saberName, int saberModel, saberType_t saberType, vec3_t origin, vec3_t angles, int bladeNum )
{

	char bladeColorString[MAX_QPATH];
	saber_colors_t bladeColor;
	float bladeLength,bladeRadius;
	vec3_t	bladeOrigin={0};
	vec3_t	axis[3]={0};
//	vec3_t	angles={0};
	mdxaBone_t	boltMatrix;
	qboolean tagHack = qfalse;
	char *tagName;
	int bolt;
	float scale;
	//[RGBSabers]
	int snum;
	//[/RGBSabers]

	if ( (item->flags&ITF_ISSABER) && saberModel < 2 )
	{
		//[RGBSabers]
		snum = 0;
		trap_Cvar_VariableStringBuffer("ui_saber_color", bladeColorString, sizeof(bladeColorString) );
	}
	else//if ( item->flags&ITF_ISSABER2 ) - presumed
	{
		snum = 1;
		//[/RGBSabers]
		trap_Cvar_VariableStringBuffer("ui_saber2_color", bladeColorString, sizeof(bladeColorString) );
	}

	if ( !trap_G2API_HasGhoul2ModelOnIndex(&(item->ghoul2),saberModel) )
	{//invalid index!
		return;
	}

	bladeColor = TranslateSaberColor( bladeColorString );

	bladeLength = UI_SaberBladeLengthForSaber( saberName, bladeNum );
	bladeRadius = UI_SaberBladeRadiusForSaber( saberName, bladeNum );

	tagName = va( "*blade%d", bladeNum+1 );
	bolt = trap_G2API_AddBolt( item->ghoul2,saberModel, tagName );
	
	if ( bolt == -1 )
	{
		tagHack = qtrue;
		//hmm, just fall back to the most basic tag (this will also make it work with pre-JKA saber models
		bolt = trap_G2API_AddBolt( item->ghoul2,saberModel, "*flash" );
		if ( bolt == -1 )
		{//no tag_flash either?!!
			bolt = 0;
		}
	}
	
//	angles[PITCH] = curYaw;
//	angles[ROLL] = 0;

	trap_G2API_GetBoltMatrix( item->ghoul2, saberModel, bolt, &boltMatrix, angles, origin, uiInfo.uiDC.realTime, NULL, vec3_origin );//NULL was cgs.model_draw

	// work the matrix axis stuff into the original axis and origins used.
	BG_GiveMeVectorFromMatrix(&boltMatrix, ORIGIN, bladeOrigin);
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_Y, axis[0]);//front (was NEGATIVE_Y, but the md3->glm exporter screws up this tag somethin' awful)
																//		...changed this back to NEGATIVE_Y		
	BG_GiveMeVectorFromMatrix(&boltMatrix, NEGATIVE_X, axis[1]);//right ... and changed this to NEGATIVE_X
	BG_GiveMeVectorFromMatrix(&boltMatrix, POSITIVE_Z, axis[2]);//up

	// Where do I get scale from?
//	scale = DC->xscale;
	scale = 1.0f;

	if ( tagHack )
	{
		switch ( saberType )
		{
		case SABER_SINGLE:
				VectorMA( bladeOrigin, scale, axis[0], bladeOrigin );
			break;
		case SABER_DAGGER:
		case SABER_LANCE:
			break;
		case SABER_STAFF:
			if ( bladeNum == 0 )
			{
				VectorMA( bladeOrigin, 12*scale, axis[0], bladeOrigin );
			}
			if ( bladeNum == 1 )
			{
				VectorScale( axis[0], -1, axis[0] );
				VectorMA( bladeOrigin, 12*scale, axis[0], bladeOrigin );
			}
			break;
		case SABER_BROAD:
			if ( bladeNum == 0 )
			{
				VectorMA( bladeOrigin, -1*scale, axis[1], bladeOrigin );
			}
			else if ( bladeNum == 1 )
			{
				VectorMA( bladeOrigin, 1*scale, axis[1], bladeOrigin );
			}
			break;
		case SABER_PRONG:
			if ( bladeNum == 0 )
			{
				VectorMA( bladeOrigin, -3*scale, axis[1], bladeOrigin );
			}
			else if ( bladeNum == 1 )
			{
				VectorMA( bladeOrigin, 3*scale, axis[1], bladeOrigin );
			}
			break;
		case SABER_ARC:
			VectorSubtract( axis[1], axis[2], axis[1] );
			VectorNormalize( axis[1] );
			switch ( bladeNum )
			{
			case 0:
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				VectorScale( axis[0], 0.75f, axis[0] );
				VectorScale( axis[1], 0.25f, axis[1] );
				VectorAdd( axis[0], axis[1], axis[0] );
				break;
			case 1:
				VectorScale( axis[0], 0.25f, axis[0] );
				VectorScale( axis[1], 0.75f, axis[1] );
				VectorAdd( axis[0], axis[1], axis[0] );
				break;
			case 2:
				VectorMA( bladeOrigin, -8*scale, axis[0], bladeOrigin );
				VectorScale( axis[0], -0.25f, axis[0] );
				VectorScale( axis[1], 0.75f, axis[1] );
				VectorAdd( axis[0], axis[1], axis[0] );
				break;
			case 3:
				VectorMA( bladeOrigin, -16*scale, axis[0], bladeOrigin );
				VectorScale( axis[0], -0.75f, axis[0] );
				VectorScale( axis[1], 0.25f, axis[1] );
				VectorAdd( axis[0], axis[1], axis[0] );
				break;
			}
			break;
		case SABER_SAI:
			if ( bladeNum == 1 )
			{
				VectorMA( bladeOrigin, -3*scale, axis[1], bladeOrigin );
			}
			else if ( bladeNum == 2 )
			{
				VectorMA( bladeOrigin, 3*scale, axis[1], bladeOrigin );
			}
			break;
		case SABER_CLAW:
			switch ( bladeNum )
			{
			case 0:
				VectorMA( bladeOrigin, 2*scale, axis[0], bladeOrigin );
				VectorMA( bladeOrigin, 2*scale, axis[2], bladeOrigin );
				break;
			case 1:
				VectorMA( bladeOrigin, 2*scale, axis[0], bladeOrigin );
				VectorMA( bladeOrigin, 2*scale, axis[2], bladeOrigin );
				VectorMA( bladeOrigin, 2*scale, axis[1], bladeOrigin );
				break;
			case 2:
				VectorMA( bladeOrigin, 2*scale, axis[0], bladeOrigin );
				VectorMA( bladeOrigin, 2*scale, axis[2], bladeOrigin );
				VectorMA( bladeOrigin, -2*scale, axis[1], bladeOrigin );
				break;
			}
			break;
		case SABER_STAR:
			switch ( bladeNum )
			{
			case 0:
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 1:
				VectorScale( axis[0], 0.33f, axis[0] );
				VectorScale( axis[2], 0.67f, axis[2] );
				VectorAdd( axis[0], axis[2], axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 2:
				VectorScale( axis[0], -0.33f, axis[0] );
				VectorScale( axis[2], 0.67f, axis[2] );
				VectorAdd( axis[0], axis[2], axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 3:
				VectorScale( axis[0], -1, axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 4:
				VectorScale( axis[0], -0.33f, axis[0] );
				VectorScale( axis[2], -0.67f, axis[2] );
				VectorAdd( axis[0], axis[2], axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			case 5:
				VectorScale( axis[0], 0.33f, axis[0] );
				VectorScale( axis[2], -0.67f, axis[2] );
				VectorAdd( axis[0], axis[2], axis[0] );
				VectorMA( bladeOrigin, 8*scale, axis[0], bladeOrigin );
				break;
			}
			break;
		case SABER_TRIDENT:
			switch ( bladeNum )
			{
			case 0:
				VectorMA( bladeOrigin, 24*scale, axis[0], bladeOrigin );
				break;
			case 1:
				VectorMA( bladeOrigin, -6*scale, axis[1], bladeOrigin );
				VectorMA( bladeOrigin, 24*scale, axis[0], bladeOrigin );
				break;
			case 2:
				VectorMA( bladeOrigin, 6*scale, axis[1], bladeOrigin );
				VectorMA( bladeOrigin, 24*scale, axis[0], bladeOrigin );
				break;
			case 3:
				VectorMA( bladeOrigin, -32*scale, axis[0], bladeOrigin );
				VectorScale( axis[0], -1, axis[0] );
				break;
			}
			break;
		case SABER_SITH_SWORD:
			//no blade
			break;
		}
	}
	if ( saberType == SABER_SITH_SWORD )
	{//draw no blade
		return;
	}

//[RGBSabers]
	UI_DoSaber( bladeOrigin, axis[0], bladeLength, bladeLength, bladeRadius, bladeColor, snum );
//[/RGBSabers]

}

/*
void UI_SaberDrawBlades( itemDef_t *item, vec3_t origin, vec3_t angles )
{
	//NOTE: only allows one saber type in view at a time
	char saber[MAX_QPATH];
	if ( item->flags&ITF_ISSABER )
	{
		trap_Cvar_VariableStringBuffer("ui_saber", saber, sizeof(saber) );
		if ( !UI_SaberValidForPlayerInMP( saber ) )
		{
			trap_Cvar_Set( "ui_saber", "kyle" );
			trap_Cvar_VariableStringBuffer("ui_saber", saber, sizeof(saber) );
		}
	}
	else if ( item->flags&ITF_ISSABER2 )
	{
		trap_Cvar_VariableStringBuffer("ui_saber2", saber, sizeof(saber) );
		if ( !UI_SaberValidForPlayerInMP( saber ) )
		{
			trap_Cvar_Set( "ui_saber2", "kyle" );
			trap_Cvar_VariableStringBuffer("ui_saber2", saber, sizeof(saber) );
		}
	}
	else
	{
		return;
	}
	if ( saber[0] )
	{
		saberType_t saberType;
		int curBlade;
		int numBlades = UI_SaberNumBladesForSaber( saber );
		if ( numBlades )
		{//okay, here we go, time to draw each blade...
			char	saberTypeString[MAX_QPATH]={0};
			UI_SaberTypeForSaber( saber, saberTypeString );
			saberType = TranslateSaberType( saberTypeString );
			for ( curBlade = 0; curBlade < numBlades; curBlade++ )
			{
				UI_SaberDrawBlade( item, saber, saberType, origin, angles, curBlade );
			}
		}
	}
}
*/

void UI_GetSaberForMenu( char *saber, int saberNum )
{
	char saberTypeString[MAX_QPATH]={0};
	saberType_t saberType = SABER_NONE;

	if ( saberNum == 0 )
	{
		trap_Cvar_VariableStringBuffer("ui_saber", saber, MAX_QPATH );
		if ( !UI_SaberValidForPlayerInMP( saber ) )
		{
			trap_Cvar_Set( "ui_saber", "kyle" );
			trap_Cvar_VariableStringBuffer("ui_saber", saber, MAX_QPATH );
		}
	}
	else
	{
		trap_Cvar_VariableStringBuffer("ui_saber2", saber, MAX_QPATH );
		if ( !UI_SaberValidForPlayerInMP( saber ) )
		{
			trap_Cvar_Set( "ui_saber2", "kyle" );
			trap_Cvar_VariableStringBuffer("ui_saber2", saber, MAX_QPATH );
		}
	}
	//read this from the sabers.cfg
	UI_SaberTypeForSaber( saber, saberTypeString );
	if ( saberTypeString[0] )
	{
		saberType = TranslateSaberType( saberTypeString );
	}

	switch ( uiInfo.movesTitleIndex )
	{
	case 0://MD_ACROBATICS:
		break;
	case 1://MD_SINGLE_FAST:
	case 2://MD_SINGLE_MEDIUM:
	case 3://MD_SINGLE_STRONG:
		if ( saberType != SABER_SINGLE )
		{
			Q_strncpyz(saber,"single_1",MAX_QPATH);
		}
		break;
	case 4://MD_DUAL_SABERS:
		if ( saberType != SABER_SINGLE )
		{
			Q_strncpyz(saber,"single_1",MAX_QPATH);
		}
		break;
	case 5://MD_SABER_STAFF:
		if ( saberType == SABER_SINGLE || saberType == SABER_NONE )
		{
			Q_strncpyz(saber,"dual_1",MAX_QPATH);
		}
		break;
	}

}

void UI_SaberDrawBlades( itemDef_t *item, vec3_t origin, vec3_t angles )
{
	//NOTE: only allows one saber type in view at a time
	char saber[MAX_QPATH];
	int saberNum = 0;
	int saberModel = 0;
	int	numSabers = 1;

	if ( (item->flags&ITF_ISCHARACTER)//hacked sabermoves sabers in character's hand
		&& uiInfo.movesTitleIndex == 4 /*MD_DUAL_SABERS*/ )
	{
		numSabers = 2;
	}

	for ( saberNum = 0; saberNum < numSabers; saberNum++ )
	{
		if ( (item->flags&ITF_ISCHARACTER) )//hacked sabermoves sabers in character's hand
		{
			UI_GetSaberForMenu( saber, saberNum );
			saberModel = saberNum + 1;
		}
		else if ( (item->flags&ITF_ISSABER) )
		{
			trap_Cvar_VariableStringBuffer("ui_saber", saber, sizeof(saber) );
			if ( !UI_SaberValidForPlayerInMP( saber ) )
			{
				trap_Cvar_Set( "ui_saber", "kyle" );
				trap_Cvar_VariableStringBuffer("ui_saber", saber, sizeof(saber) );
			}
			saberModel = 0;
		}
		else if ( (item->flags&ITF_ISSABER2) )
		{
			trap_Cvar_VariableStringBuffer("ui_saber2", saber, sizeof(saber) );
			if ( !UI_SaberValidForPlayerInMP( saber ) )
			{
				trap_Cvar_Set( "ui_saber2", "kyle" );
				trap_Cvar_VariableStringBuffer("ui_saber2", saber, sizeof(saber) );
			}
			saberModel = 0;
		}
		else
		{
			return;
		}
		if ( saber[0] )
		{
			saberType_t saberType;
			int curBlade = 0;
			int numBlades = UI_SaberNumBladesForSaber( saber );
			if ( numBlades )
			{//okay, here we go, time to draw each blade...
				char	saberTypeString[MAX_QPATH]={0};
				UI_SaberTypeForSaber( saber, saberTypeString );
				saberType = TranslateSaberType( saberTypeString );
				for ( curBlade = 0; curBlade < numBlades; curBlade++ )
				{
					if ( UI_SaberShouldDrawBlade( saber, curBlade ) )
					{
						UI_SaberDrawBlade( item, saber, saberModel, saberType, origin, angles, curBlade );
					}
				}
			}
		}
	}
}

void UI_SaberAttachToChar( itemDef_t *item )
{
	int	numSabers = 1;
 	int	saberNum = 0;

	if ( trap_G2API_HasGhoul2ModelOnIndex(&(item->ghoul2),2) )
	{//remove any extra models
		trap_G2API_RemoveGhoul2Model(&(item->ghoul2), 2);
	}
	if ( trap_G2API_HasGhoul2ModelOnIndex(&(item->ghoul2),1) )
	{//remove any extra models
		trap_G2API_RemoveGhoul2Model(&(item->ghoul2), 1);
	}

	if ( uiInfo.movesTitleIndex == 4 /*MD_DUAL_SABERS*/ )
	{
		numSabers = 2;
	}

	for ( saberNum = 0; saberNum < numSabers; saberNum++ )
	{
		//bolt sabers
		char modelPath[MAX_QPATH];
		char skinPath[MAX_QPATH];
		char saber[MAX_QPATH]; 

		UI_GetSaberForMenu( saber, saberNum );

		if ( UI_SaberModelForSaber( saber, modelPath ) )
		{//successfully found a model
			int g2Saber = trap_G2API_InitGhoul2Model( &(item->ghoul2), modelPath, 0, 0, 0, 0, 0 ); //add the model
			if ( g2Saber )
			{
				int boltNum;
				//get the customSkin, if any
				if ( UI_SaberSkinForSaber( saber, skinPath ) )
				{
					int g2skin = trap_R_RegisterSkin(skinPath);
					trap_G2API_SetSkin( item->ghoul2, g2Saber, 0, g2skin );//this is going to set the surfs on/off matching the skin file
				}
				else
				{
					trap_G2API_SetSkin( item->ghoul2, g2Saber, 0, 0 );//turn off custom skin
				}
				if ( saberNum == 0 )
				{
					boltNum = trap_G2API_AddBolt( item->ghoul2, 0, "*r_hand");
				}
				else
				{
					boltNum = trap_G2API_AddBolt( item->ghoul2, 0, "*l_hand");
				}
				trap_G2API_AttachG2Model( item->ghoul2, g2Saber, item->ghoul2, boltNum, 0);
			}
		}
	}
}


//[DynamicMemory_Sabers]
char *UI_GetSaberHiltInfo(qboolean TwoHanded, int index){
	if(TwoHanded)
		return saberStaffHiltInfo[index];
	else
		return saberSingleHiltInfo[index];
}

int UI_GetSaberCount(qboolean TwoHanded){
	//[UITweaks]
	if(TwoHanded == qfalse){
		return saberSingleHiltCount;
	}
	else{
		return saberStaffHiltCount;
	}
	/* basejka code
		int count = 0, i;
	if(TwoHanded == qfalse){
		for (i=0;i<saberSingleHiltCount;i++)
		{
			if (saberSingleHiltInfo[i])
			{
				count++;
			}
			else
			{//done
				break;
			}
		}
		return count;
	}
	else{
		for (i=0;i<saberStaffHiltCount;i++)
		{
			if (saberStaffHiltInfo[i])
			{
				count++;
			}
			else
			{//done
				break;
			}
		}
		return count;
	*/
	//[/UITweaks]
}

//#define MAX_SABER_HILTS	64

//void UI_SaberGetHiltInfo( const char *singleHilts[MAX_SABER_HILTS], const char *staffHilts[MAX_SABER_HILTS] )
void UI_SaberGetHiltInfo(void){
	int	numSingleHilts = 0, numStaffHilts = 0;
	const char	*saberName;
	const char	*token;
	const char	*p;

	//go through all the loaded sabers and put the valid ones in the proper list
	p = SaberParms;
	COM_BeginParseSession("saberlist");

	// look for a saber
	while ( p )
	{
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 )
		{//invalid name
			continue;
		}
		saberName = String_Alloc( token );
		//see if there's a "{" on the next line
		SkipRestOfLine( &p );

		if ( UI_ParseLiteralSilent( &p, "{" ) ) 
		{//nope, not a name, keep looking
			continue;
		}

		//this is a saber name
		if ( !UI_SaberValidForPlayerInMP( saberName ) )
		{
			SkipBracedSection( &p );
			continue;
		}

		if ( UI_IsSaberTwoHanded( saberName ) )
		{
#ifndef DYNAMICMEMORY_SABERS
			if ( numStaffHilts < MAX_SABER_HILTS-1 )//-1 because we have to NULL terminate the list
			{
				staffHilts[numStaffHilts++] = saberName;
			}
			else
			{
				Com_Printf( "WARNING: too many two-handed sabers, ignoring saber '%s'\n", saberName );
			}
#else
			UI_ReaAllocMem((void **)saberStaffHiltInfo, sizeof(char *), numStaffHilts+1);
			saberStaffHiltInfo[numStaffHilts++] = (char *) saberName;
#endif
		}
		else
		{
#ifndef DYNAMICMEMORY_SABERS
			if ( numSingleHilts < MAX_SABER_HILTS-1 )//-1 because we have to NULL terminate the list
			{
				singleHilts[numSingleHilts++] = saberName;
			}
			else
			{
				Com_Printf( "WARNING: too many one-handed sabers, ignoring saber '%s'\n", saberName );
			}
#else
			UI_ReaAllocMem((void **)saberSingleHiltInfo, sizeof(char *), numSingleHilts+1);
			saberSingleHiltInfo[numSingleHilts++] = (char *) saberName;
#endif
		}
		//skip the whole braced section and move on to the next entry
		SkipBracedSection( &p );
	}
	//null terminate the list so the UI code knows where to stop listing them
#ifndef DYNAMICMEMORY_SABERS
	singleHilts[numSingleHilts] = NULL;
	staffHilts[numStaffHilts] = NULL;
#else
	saberSingleHiltCount = numSingleHilts;
	saberStaffHiltCount = numStaffHilts;
#endif
}
//[/DynamicMemory_Sabers]
