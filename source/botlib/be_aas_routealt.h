
/*****************************************************************************
 * name:		be_aas_routealt.h
 *
 * desc:		AAS
 *
 * $Archive: /source/code/botlib/be_aas_routealt.h $
 * $Author: razorace $ 
 * $Revision: 1.1 $
 * $Modtime: 10/05/99 3:32p $
 * $Date: 2003/12/01 20:10:23 $
 *
 *****************************************************************************/

#ifdef AASINTERN
void AAS_InitAlternativeRouting(void);
void AAS_ShutdownAlternativeRouting(void);
#endif //AASINTERN


int AAS_AlternativeRouteGoals(vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags,
										aas_altroutegoal_t *altroutegoals, int maxaltroutegoals,
										int type);
