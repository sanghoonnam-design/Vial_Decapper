#ifndef   __SWRTC_H__
#define   __SWRTC_H__

#include <stdint.h>

#ifdef __SWRTC_C__
	#define SWRTC_EXT
#else
	#define SWRTC_EXT extern
#endif

typedef struct
{
    uint16_t ms;     // 0 ~ 999
    uint8_t sec;  //00~59
    uint8_t min;  //00~59
    uint8_t hour; //0~23
    uint8_t day;  //1~31
    uint8_t month;//1~12
    uint8_t year; //00~99
} SW_DateTime_t;

SWRTC_EXT void SWRTC_Init(void);
SWRTC_EXT void SWRTC_SetParam(SW_DateTime_t *dt);
SWRTC_EXT void SWRTC_GetParam(SW_DateTime_t *dt);

/* interrupt에서 호출 */
SWRTC_EXT void SWRTC_TickISR(void);

#endif

