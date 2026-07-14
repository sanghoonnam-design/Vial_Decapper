#ifndef   __IADC_H__
#define   __IADC_H__

#include "project.h"

#ifdef __IADC_C__
	#define IADC_EXT
#else
	#define IADC_EXT extern
#endif

#define ADC_CH0         0
#define ADC_CH1         1

#define MAX_ADC_CH      2

IADC_EXT void IADC_Init(void);
IADC_EXT F32 IADC_GetVolt(U8 ch);
IADC_EXT F32 IADC_GetMcuInternalTemp(void);
#endif

