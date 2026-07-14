/*******************************************************************************
 * XDiagnose.c
 *
 *  Created on: 2025.10.30
 *      Author: RND. Kang PilSoon.
 *
 ******************************************************************************/

#include "_04_XDiagnose.h"
#include "XSystemInfo.h"
#include "_01_XSystemManagement.h"
#include "XDebug.h"

Semaphore_Handle semHD_SDG;

//==============================================================================
#if 1 // debugging flag.
BOOL DB_isDiagnosisDisabled = NO;
#else
BOOL DB_isDiagnosisEnabled = YES;
#endif
//==============================================================================

typedef struct
{
    U08 isPossible;
    teErrorCode errorCode;
} tsControlAvailability;

static tsControlAvailability SDG_Internal_CheckRobotDoor(int command);
static tsControlAvailability SDG_Internal_CheckServoA6(int controlMode);

VOID TASK_Diagnose(void *pvParameters)
{
    /** @note: USER CODE, Init. Task */
    uint32_t startTick;

    FOREVER
    {
        if (xSemaphoreTake(semHD_SDG, RTOS_WAIT_FOREVER) == pdTRUE)
        {
            __TASK_TRIGGER_START_Using(TEST_PORT_1, TP_IDX_taskSDG);
            startTick = ITIMER_StartMeasure_us();
            //==================================================================
            __xTaskStatus[TP_IDX_taskSDG] = true;
            XTP_CheckTaskUsingLED(XHW_STATUS_LED_3);
            if (xSystemInfo.PL_Get_FW_Mode() == FW_MODE_IDLE || DB_isDiagnosisDisabled == YES)
            {
                __TASK_TRIGGER_END_Using(TEST_PORT_1, TP_IDX_taskSDG);
                continue;
            }
            //==================================================================

            SDG_RobotDoor_CheckControlAvailability();
            SDG_ServoA6_CheckControlAvailability();

            //==================================================================
            gTick_SDG = ITIMER_StopMeasure_us(startTick);
            __TASK_TRIGGER_END_Using(TEST_PORT_1, TP_IDX_taskSDG);
        }
    } /*@end FOREVER{}*/
}

int Init_Diagnose(int Index)
{
    int result = EXIT_SUCCESS;

    semHD_SDG = xSemaphoreCreateBinary();

    return result;
}

static tsControlAvailability SDG_Internal_CheckRobotDoor(int command)
{
    tsControlAvailability result = {.isPossible = YES, .errorCode = ERROR_CODE_DEFAULT};

    // 평상시에 실행됨.
    if (xSL.Door.isInitialized == NO)
    {
        result.isPossible = NO;
        result.errorCode = ERROR_CODE_ROBOT_DOOR_NOT_INIT;
    }
    else
    {
#if 0
        if (xSL.Door.Status == ROBOTDOOR_STATE_MOVING)
        {
            result.isPossible = NO;
            result.errorCode = ERROR_CODE_ROBOT_DOOR_MOVING;

            return result;
        }
#endif
        if (command != ROBOTDOOR_COMMAND_NONE)
        {
            if (command == ROBOTDOOR_COMMAND_CLOSE && xSL.Door.Status == ROBOTDOOR_STATE_CLOSED)
            {
                result.isPossible = NO;
                result.errorCode = ERROR_CODE_ROBOT_DOOR_ALREADY_CLOSE;

                return result;
            }
            else if (command == ROBOTDOOR_COMMAND_OPEN && xSL.Door.Status == ROBOTDOOR_STATE_OPEN)
            {
                result.isPossible = NO;
                result.errorCode = ERROR_CODE_ROBOT_DOOR_ALREADY_OPEN;

                return result;
            }
            // [CENT] 명령 동작중에는 로봇도어 [open] 제어 불가능
            else if (xSL.ServoA6.isCentrifugeRunning && command == ROBOTDOOR_COMMAND_OPEN) 
            {
                result.isPossible = NO;
                result.errorCode = ERROR_CODE_SYSTEM_MOVING_STATUS;
            }
            else
            {
                result.isPossible = YES;
                result.errorCode = ERROR_CODE_DEFAULT;
            }
        }
    }

    return result;
}

static tsControlAvailability SDG_Internal_CheckServoA6(int ControlMode)
{
    tsControlAvailability result = {.isPossible = YES, .errorCode = ERROR_CODE_DEFAULT};

    // 실시간 처리
    if (!xSL.Door.isInitialized)
    {
        result.isPossible = NO;
        result.errorCode = ERROR_CODE_A6_SERVO_NOT_INIT;
    }
    else
    {   // 비동기 부분
        if (ControlMode != A6_CONTROL_MODE_NONE)
        {
            // 예외: HOME 또는 ORG 일 때는 에러여도 조건 통과
            bool isHomeOrOrg = (ControlMode == A6_CONTROL_MODE_HOME ||
                                ControlMode == A6_CONTROL_MODE_ORG);

            if (xServoA6.IsDriverError() && !isHomeOrOrg) 
            {
                result.isPossible = NO;
                result.errorCode = ERROR_CODE_A6_DRIVER_ERROR;
            }
            else if (!xServoA6.IsServoOn() && !isHomeOrOrg) 
            {
                result.isPossible = NO;
                result.errorCode = ERROR_CODE_A6_SERVO_OFF;
            }
            else if (xServoA6.IsBusy())
            {
                result.isPossible = NO;
                result.errorCode = ERROR_CODE_SYSTEM_MOVING_STATUS;
            }
            else
            {
                result.isPossible = YES;
                result.errorCode = ERROR_CODE_DEFAULT;
            }
        }
    }

    return result;
}

VOI SDG_RobotDoor_CheckControlAvailability(void)
{
    tsControlAvailability result = SDG_Internal_CheckRobotDoor((int)ROBOTDOOR_COMMAND_NONE);

    xSL.Door.isControllable = result.isPossible;
}

VOI SDG_ServoA6_CheckControlAvailability(void)
{
    tsControlAvailability result = SDG_Internal_CheckServoA6((int)A6_CONTROL_MODE_NONE);

    xSL.ServoA6.isControllable = result.isPossible;
}

U08 SDG_RobotDoor_CheckError(int command)
{
    tsControlAvailability result = SDG_Internal_CheckRobotDoor(command);

    if (result.errorCode != ERROR_CODE_DEFAULT)
    {
        SetErrorCode(result.errorCode);
        return true;
    }

    return false;
}

U08 SDG_ServoA6_CheckError(int controlMode)
{
    tsControlAvailability result = SDG_Internal_CheckServoA6(controlMode);

    if (result.errorCode != ERROR_CODE_DEFAULT)
    {
        SetErrorCode(result.errorCode);
        return true;
    }

    return false;
}
