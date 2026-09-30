/*******************************************************************************
 * XApControl.c
 *
 *  Created on: 2024.10.14
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#include "_06_XAppControl.h"
#include "XSystemInfo.h"
#include "_01_XSystemManagement.h"
#include "_02_XUpdateSigData.h"
#include "_10_XSerialCMD_Process.h"
#include "_10_XCommand_Core.h"
#include "XSystem_DB.h"
#include "XBuffer.h"
#include "XDebug.h"
#include "XStateMachine.h"

#include "CDecap.h"

Semaphore_Handle semHD_APC;

static U32 Control_StartTime = __2sec;

/**
 * @brief 주기 세마포어 수신 시 xAT 요청을 Decapper FSM 콜백으로 전달한다.
 * 부팅 초기 대기 시간 또는 FW_MODE_IDLE에서는 제어를 건너뛴다. FSM은 호출마다 현재 단계만 진행한다.
 */
VOID TASK_ApplicationControl(void *pvParameters)
{
    /** @note: USER CODE, Init. Task */
    uint32_t startTick;

    //    A6_Driver_test_Init();

    FOREVER
    {
        if (xSemaphoreTake(semHD_APC, RTOS_WAIT_FOREVER) == pdTRUE)
        {
            __TASK_TRIGGER_START_Using(TEST_PORT_1, TP_IDX_taskAPC);
            //==================================================================
            startTick = ITIMER_StartMeasure_us();
            __xTaskStatus[TP_IDX_taskAPC] = true;

            if (xSystemInfo.PL_Get_FW_Mode() == FW_MODE_IDLE || gTriggerCount < Control_StartTime)
            {
                gTick_APC = ITIMER_StopMeasure_us(startTick);
                __TASK_TRIGGER_END_Using(TEST_PORT_1, TP_IDX_taskAPC);
                continue;
            }
            XTP_CheckTaskUsingLED(XHW_STATUS_LED_4);
            //==================================================================
            xCDecap.Action_Fnc(xAT);
            //==================================================================
            gTick_APC = ITIMER_StopMeasure_us(startTick);
            __TASK_TRIGGER_END_Using(TEST_PORT_1, TP_IDX_taskAPC);
        }

    } /*@end FOREVER{}*/
}

/**
 * @brief 제어 태스크의 세마포어를 생성하고 Decapper 콜백·드라이버·상태를 초기화한다.
 * CDecap_Init이 PL을 사용하므로 설정 로드 이후의 초기화 순서가 중요하다.
 */
int Init_ApplicationControl(int Index)
{
    semHD_APC = xSemaphoreCreateBinary();
    CDecap_Init();
    return EXIT_SUCCESS;
}
