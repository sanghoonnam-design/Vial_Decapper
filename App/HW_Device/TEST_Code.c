/*******************************************************************************
 * TEST_Code.h
 *
 * Created on: 2025.10.06
 * Author    : RND. Kang PilSoon
 *
 ******************************************************************************/
#include "TEST_Code.h"
#include "XSystem_DB.h"
#include "XSystemInfo.h"
#include "XStateMachine.h"
#include "XErrorCode.h"
#include "_10_XCommand_Core.h"
#include "_01_HW_and_Device.h"

tsXTest xTest;

void Initialize_Test(void)
{
    memset((char *)&xTest, 0, sizeof(tsXTest));

    xTest.SateMachine /*    */ = TEST_StateMachine;
    //========================================================================
}

//============================================================================
//============================================================================
/** @brief Test FSM                                                         */
//============================================================================
//============================================================================
static U08 step = STEP_TEST_IDLE;
static U08 subStep = 0;
static U08 returnStep = STEP_TEST_IDLE;
static U08 returnSubStep = 0;
static U32 waitCount = 0;
static U32 delayTime = 0;

// Test Step 함수 선언
static void Step_Test_Idle(void);
static void Step_Test_LongRun_Robot_TY(void);
static void Step_Test_LongRun_RobotDoor(void);
static void Step_Test_Stop(void);
static void Step_Test_Delay(void);
static void Step_Test_EndErr(void);
static void Step_Test_EndOk(void);

// ControlMode → Step 매핑
static const teStep_Test testModeToStep[] = {
    [MODE_TEST_NONE] /*                */ = STEP_TEST_IDLE,
    [MODE_TEST_ROBOT_TY] /*            */ = STEP_TEST_LONGRUN_ROBOT_TY,
    [MODE_TEST_ROBOTDOOR] /*           */ = STEP_TEST_LONGRUN_ROBOTDOOR,
    [MODE_TEST_STOP] /*                */ = STEP_TEST_STOP};

// Step 테이블
static FSM_StepHandler stepFunctionTable[] = {
    [STEP_TEST_IDLE] /*                */ = Step_Test_Idle,
    [STEP_TEST_LONGRUN_ROBOT_TY] /*    */ = Step_Test_LongRun_Robot_TY,
    [STEP_TEST_LONGRUN_ROBOTDOOR] /*   */ = Step_Test_LongRun_RobotDoor,
    [STEP_TEST_STOP] /*                */ = Step_Test_Stop,
    [STEP_TEST_DELAY] /*               */ = Step_Test_Delay,
    [STEP_TEST_END_ERR] /*             */ = Step_Test_EndErr,
    [STEP_TEST_END_OK] /*              */ = Step_Test_EndOk};

void TEST_StateMachine(void)
{
    if (xTest.CMD_StartControl == YES)
    {
        if (xTest.TestMode < ARRAY_SIZE(testModeToStep))
        {
            step = testModeToStep[xTest.TestMode];
            subStep = 0;
        }
        else
        {
            step = STEP_TEST_IDLE;
        }

        xTest.CMD_StartControl = NO;
    }

    if (step < ARRAY_SIZE(stepFunctionTable) && stepFunctionTable[step])
    {
        stepFunctionTable[step]();
    }
}

static void Step_Test_Idle(void) { return; }

static void Step_Test_LongRun_Robot_TY(void)
{
//    static const char *commands[] = {
//
//        "ledcol 3"};
//
//    static const int numCommands = sizeof(commands) / sizeof(commands[0]);
//    static int commandIndex = 0; // 현재 실행할 명령어
//    static U32 lastTrigger = 0;  // 마지막 실행 시각
//
//    switch (subStep)
//    {
//    case 0: // 초기화
//        commandIndex = 0;
//        lastTrigger = gTriggerCount;
//        subStep = 1;
//        break;
//
//    case 1:                                        // 0.5초마다 명령 실행
////        if (((gTriggerCount - lastTrigger) >= 50) && (xSL.RobotTY.isMoving_TY == NO)) // 10ms * 50 = 500ms
////        {
////            teXParsingErrorCode result =
////                xParser_ProcessReceivedData(commands[commandIndex], &xParsedData_Network, COMM_USB);
////
////            if (result == PARSER_ERR_SUCCESS)
////            {
////                Handle_command(&xParsedData_Network, COMM_USB);
////            }
////
////            // 다음 명령으로 이동
////            commandIndex++;
////            if (commandIndex >= numCommands)
////            {
////                commandIndex = 0;
////            }
////
////            lastTrigger = gTriggerCount;
////        }
//        break;
//    default:
//        break;
//    }
}

static void Step_Test_LongRun_RobotDoor(void)
{
    switch (subStep)
    {
    case 0:

        subStep++;
        break;
    case 1:
        subStep++;
        break;
    case 2:
        step = STEP_TEST_END_OK;
        break;
    default:
        break;
    }
}
static void Step_Test_Stop(void)
{
    switch (subStep)
    {
    case 0:
        // Rodotdoor stop
//        xPL.Door.CMD_StartControl = YES;
//        xPL.Door.ControlMode = DOOR_MODE_AUTO;
//        xPL.Door.TargetMotion = ROBOTDOOR_COMMAND_STOP;
//        subStep++;
        break;
    case 1:
        // motor stop
//        xPL.RobotTY.CMD_StartControl = YES;
//        xPL.RobotTY.ControlMode = STEP_ROBOT_STOP;
//        subStep++;
        break;
    case 2: // sol stop
//        xPL.Solvalve.startSolvalveAllClose = YES;
        subStep++;
        break;
    case 3:
        step = STEP_TEST_END_OK;
        break;
    default:
        break;
    }
}
static void Step_Test_Delay(void)
{
    FSM_CheckDelay(&step, &returnStep, &waitCount, &delayTime);
}
static void Step_Test_EndErr(void)
{
    ERR_MSG_SEND_N("E%d : %s", GetErrorCode_int(), GetErrorMessage());
    step = STEP_TEST_IDLE;
    subStep = 0;
}
static void Step_Test_EndOk(void)
{
    step = STEP_TEST_IDLE;
    subStep = 0;
    returnStep = 0;
    returnSubStep = 0;
    waitCount = 0;
    delayTime = 0;
}
