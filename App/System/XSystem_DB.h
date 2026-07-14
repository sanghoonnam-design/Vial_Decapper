/*******************************************************************************
 * XSystem_DB.h
 *
 *  Created on: 2025.09.04
 *      Author: RND. Kang PilSoon.
 *
 ******************************************************************************/
#ifndef __XSYSTEM_DB_H__
#define __XSYSTEM_DB_H__

#include "XGlobal.h"

// @USER CODE START
#include "Dev_RobotDoor.h"
#include "Dev_LED.h"
#include "Dev_RobotDoor.h"
#include "Dev_ServoMotor_A6.h"
#include "Dev_Temperature.h"
// @USER CODE END

#pragma pack(push, 1)      //
typedef struct             //
{                          //
    int isBusy;            //
                           //
    tsXSL_LED LED;         //
    tsXSL_RobotDoor Door;  //
    tsXSL_ServoA6 ServoA6; //
} tsXStateList;

typedef struct
{
    int sl_Size;        // SL 사이즈
    tsXStateList SL;    //
    float filteredData; //
    /**============================== 여기까지 기본 포멧 [수정하지 말것!~] */

    tsXCD_LED LED;         //
    tsXCD_RobotDoor Door;  //
    tsXCD_ServoA6 ServoA6; //

} tsXControlData;

typedef struct
{
    tsXPL_LED LED;         //
    tsXPL_RobotDoor Door;  //
    tsXPL_ServoA6 ServoA6; //

} tsXParameterList;

#pragma pack(pop)

extern tsXStateList xSL;
extern tsXControlData xCD;
extern tsXParameterList xPL;

void SystemDB_Initialize(void);

// []. 부팅시 초기화 부분
void SystemDB_SL_Init(void); // 부팅시 초기화 해야 하는 부분 처리
void SystemDB_CD_Init(void); // 부팅시 초기화 해야 하는 부분 처리
void SystemDB_PL_Init(void); // 부팅할때 마다 EEPROM 에서 읽어와 초기화해야 하는 부분 처리

// []. factory 셋팅에 사용된는 부분
void SystemDB_PL_Init_Factory(tsXParameterList *pl); // EEPROM 에 저장한 factory setting 값 셋팅

// []. 모니터링 프로그램과의 통신 부분
void SystemDB_SL_Push(void);
void SystemDB_CD_Push(void);
void SystemDB_PL_Push(void);

#endif /* __XSYSTEM_DB_H__ */
