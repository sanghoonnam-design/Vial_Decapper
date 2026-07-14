/*******************************************************************************
 * Dev_LED.h
 *
 * Created on: 2025.10.06
 * Author    : RND. Kang YoungJun
 ******************************************************************************/
#include "Dev_LED.h"
#include "XSystem_DB.h"
#include "XSystemInfo.h"

tsXLED xLED;

void Initialize_LED(void)
{
    memset((char *)&xLED, 0, sizeof(tsXLED));

    xLED.On /*       */ = LED_On;
    xLED.Off /*      */ = LED_Off;
    
    xLED.Update /*   */ = LED_Update;
    //=========================================================================
}

void LED_Update(void)
{
    ;
}

void LED_On(void)
{

}

void LED_Off(void)
{
    
}
