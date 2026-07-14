#ifndef   __ITIMER_H__
#define   __ITIMER_H__

#include "project.h"

#ifdef __ITIMER_C__
	#define ITIMER_EXT
#else
	#define ITIMER_EXT extern
#endif

ITIMER_EXT void ITIMER_Init(void);
// ITIMER_EXT void (*TIM3_Callback)(void);
ITIMER_EXT void Timer2_InterruptStart(void);
ITIMER_EXT void Timer2_InterruptStop(void);

ITIMER_EXT U8   Timer2_GetInterruptStatus(void); // by KPS

ITIMER_EXT uint32_t ITIMER_StartMeasure_us(void);
ITIMER_EXT uint32_t ITIMER_StopMeasure_us(uint32_t startTick);

ITIMER_EXT void ITIMER_ResetSemaphores(void);

#endif //@end: __ITIMER_H__

