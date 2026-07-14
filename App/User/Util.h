#ifndef   __UTIL_H__
#define   __UTIL_H__

#include "project.h"

#ifdef __UTIL_C__
	#define UTIL_EXT
#else
	#define UTIL_EXT extern
#endif

UTIL_EXT void   UTIL_DelayMS(U16 wMS);
UTIL_EXT void   UTIL_DelayUS(U16 wUS);

#endif


