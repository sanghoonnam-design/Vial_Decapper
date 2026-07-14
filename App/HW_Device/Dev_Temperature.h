/*******************************************************************************
 * Dev_Temperature.h
 *
 *  Created on: 2025.10.06
 *      Author: RND. Kim BumSu.
 * 
 * 1. 냉장고 추가 버전
 * 
 ******************************************************************************/
#ifndef DEV_TEMPERATURE_H_
#define DEV_TEMPERATURE_H_
#include "_00_HW_Config.h"
#include "_00_Dev_Config.h"
#include "XGlobal.h"

#pragma pack(push, 1)
typedef struct                  // [1]. State List
{                               //
    int Status;                 // from SDG, LED상태, 1=ON, 0=OFF
} tsXSL_Temp;                   //
                                //
typedef struct                  // [2]. Control Data
{                               //
    int Out;                    // from USD, FAN 출력상태, 1=HIGH, 0=LOW
                                //
} tsXCD_Temp;                   //
                                //
typedef struct                  // [3]. Parameter List
{                               //
    int CMD_StartControl;       // from PC(client), 제어 할텨=1? 말텨=0?
    int OnOff;                  //
} tsXPL_Temp;                   //
#pragma pack(pop)               //
                                //
typedef struct TemperatureGroup // [4]. Action & Method
{                               //
    void (*On)(void);           //
    void (*Off)(void);          //
                                //
    void (*Update_Tx)(void);    //
    void (*Update_Rx)(void);    //
} tsXTemp;                      //

extern tsXTemp xTemp;

//==========================================================================
void Initialize_Temperature(void);

void Temp_On(void);
void Temp_Off(void);

void Temp_Update_Tx(void);
void Temp_Update_Rx(void);

#endif /* DEV_TEMPERATURE_H_ */
