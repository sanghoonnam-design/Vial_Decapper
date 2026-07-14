#ifndef   __SWITCH_H__
#define   __SWITCH_H__

#include "project.h"

#ifdef __SWITCH_C__
	#define SWITCH_EXT
#else
	#define SWITCH_EXT extern
#endif

#define PSW1()          (GPIOG->IDR & GPIO_PIN_0) ? 0: 1;
#define PSW2()          (GPIOG->IDR & GPIO_PIN_1) ? 0: 1;
#define PSW3()          (GPIOG->IDR & GPIO_PIN_2) ? 0: 1;
#define PSW4()          (GPIOG->IDR & GPIO_PIN_3) ? 0: 1;

typedef enum
{
  SW_ALL,
  SW1,
  SW2,
  SW3,
  SW4,
  SW_COUNT
} ESwNum;

SWITCH_EXT void SW_Init(void);
SWITCH_EXT U8   SW_Input(ESwNum num);
SWITCH_EXT U8   (*SW_Read)(ESwNum);

#endif


