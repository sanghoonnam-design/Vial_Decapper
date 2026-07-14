/*******************************************************************************
 * TEST_Code.h
 *
 *  Created on: 2025.10.06
 *      Author: RND. Kang PilSoon
 *
 ******************************************************************************/
#ifndef TEST_CODE_H_
#define TEST_CODE_H_
#include "_00_HW_Config.h"
#include "_00_Dev_Config.h"
#include "XGlobal.h"

typedef enum // mode
{
    MODE_TEST_NONE /*     */ = 0,  // default
    MODE_TEST_ROBOT_TY /* */ = 1,  // TY Robot long-run test
    MODE_TEST_ROBOTDOOR /**/ = 2,  // Robot Door long-run test
    MODE_TEST_STOP /*     */ = 999 // Stop
                                   //
} teControlMode_Test;

typedef enum // step
{
    STEP_TEST_IDLE,
    STEP_TEST_LONGRUN_ROBOT_TY,
    STEP_TEST_LONGRUN_ROBOTDOOR,
    STEP_TEST_STOP,
    STEP_TEST_DELAY,
    STEP_TEST_END_ERR,
    STEP_TEST_END_OK
} teStep_Test;

typedef struct TestGroup       //
{                              //
    int CMD_StartControl;      // from PC(client), 명령 수행 여부
    int TestMode;              // from PC(client), test 모드,
                               //
    void (*SateMachine)(void); // **최종 상태머신
} tsXTest;                     //

extern tsXTest xTest;

//==========================================================================
void Initialize_Test(void);

void TEST_StateMachine(void);

#endif /* TEST_CODE_H_ */