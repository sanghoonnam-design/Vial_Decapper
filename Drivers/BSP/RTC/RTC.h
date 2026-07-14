#ifndef   __RTC_H__
#define   __RTC_H__

#include "project.h"

#ifdef __RTC_C__
	#define RTC_EXT
#else
	#define RTC_EXT extern
#endif

// ¿ÜºÎ RTC

typedef struct
{
    U8 sec;  //00~59
    U8 min;  //00~59
    U8 hour; //0~23
    U8 date; //1~31
    U8 month;//1~12
    U8 day;  //1~7
    U8 year; //00~99
} DateTime_t;

RTC_EXT void ERTC_Init(void);
RTC_EXT void ERTC_SetParam(DateTime_t *dt);
RTC_EXT void ERTC_GetParam(DateTime_t *dt);

#endif

