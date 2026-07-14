#ifndef   __FMC_H__
#define   __FMC_H__

#include "project.h"

#ifdef __FMC_C__
	#define FMC_EXT
#else
	#define FMC_EXT extern
#endif

FMC_EXT void FMC_Init(void);

#endif

