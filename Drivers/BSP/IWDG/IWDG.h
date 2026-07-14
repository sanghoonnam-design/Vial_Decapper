#ifndef   __IWDG_H__
#define   __IWDG_H__

#include "project.h"

#ifdef __IWDG_C__
	#define IWDG_EXT
#else
	#define IWDG_EXT extern
#endif

IWDG_EXT void IWDG_Init(void);
IWDG_EXT void IWDG_Start(void);
IWDG_EXT void IWDG_Reset(void);

#endif

