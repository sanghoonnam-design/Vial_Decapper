/*******************************************************************************
 * Dev_LED.h
 *
 *  Created on: 2025.10.06
 *      Author: RND. Kang PilSoon
 ******************************************************************************/
#ifndef DEV_LED_H_
#define DEV_LED_H_
#include "_00_HW_Config.h"
#include "_00_Dev_Config.h"
#include "XGlobal.h"

#pragma pack(push, 1)
typedef struct            // [1]. State List
{                         //
    int Status;           // from SDG, LED상태, 1=ON, 0=OFF
} tsXSL_LED;              //
                          //
typedef struct            // [2]. Control Data
{                         //
    int Out;              // from USD, FAN 출력상태, 1=HIGH, 0=LOW
                          //
} tsXCD_LED;              //
                          //
typedef struct            // [3]. Parameter List
{                         //
    int CMD_StartControl; // from PC(client), 제어 할텨=1? 말텨=0?
    int OnOff;            //
} tsXPL_LED;              //
#pragma pack(pop)         //
                          //
typedef struct LEDGroup   // [4]. Action & Method
{                         //
    void (*On)(void);     //
    void (*Off)(void);    //
                          //
    void (*Update)(void); //
} tsXLED;                 //

extern tsXLED xLED;

//==========================================================================
void Initialize_LED(void);

void LED_On(void);
void LED_Off(void);
void LED_Update(void);

#endif /* DEV_LED_H_ */