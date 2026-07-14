#define __UTIL_C__
#include "Util.h"
#undef  __UTIL_C__

void UTIL_DelayMS(U16 wMS)
{
    register U16 i;

    for (i=0; i<wMS; i++)
        UTIL_DelayUS(1000);         // 1000us => 1ms
}

void UTIL_DelayUS(U16 wUS)
{
    volatile U32 Dly = (U32)wUS*8;
    for(; Dly; Dly--);
}