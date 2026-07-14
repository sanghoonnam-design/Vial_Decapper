#ifndef __IPWM_H__
#define __IPWM_H__

#include "project.h"

#ifdef __IPWM_C__
#define IPWM_EXT
#else
#define IPWM_EXT extern
#endif

#define PWM_CH0 0
#define PWM_CH1 1

IPWM_EXT void IPWM_Init(void);

IPWM_EXT void IPWM_Enable(U8 ch);
IPWM_EXT void IPWM_Disable(U8 ch);
IPWM_EXT void IPWM_UpdateDuty(U8 ch, U16 duty);

// by KPS
IPWM_EXT void IPWM_SetPeriod(uint32_t period_ms, uint16_t resolution); // 주기, 분해능 셋팅
IPWM_EXT void IPWM_SetDutyPercent(U8 ch, float percent);			   // duty % 입력
IPWM_EXT void IPWM_SetDutyRaw(U8 ch, uint32_t duty);				   // duty CCR 입력, IPWM_UpdateDuty()와 동일

#endif
