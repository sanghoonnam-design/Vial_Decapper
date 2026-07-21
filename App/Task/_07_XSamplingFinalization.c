/*******************************************************************************
 * XSampligFinalization.c
 *
 *  Created on: 2025.06.20
 *      Author: RND, Kang PilSoon.
 ******************************************************************************/
#include "_07_XSamplingFinalization.h"
#include "XSystemInfo.h"
#include "_01_XSystemManagement.h"
#include "_02_XUpdateSIGData.h"
#include "_04_XDiagnose.h"
#include "_06_XAppControl.h"
#include "XSystem_DB.h"
#include "Dev_RobotDoor.h"
#include "XDebug.h"

Semaphore_Handle semHD_SFZ;
extern sMotionStatus_t MotionStatus;

VOID TASK_SamplingFinalization(void *pvParameters)
{
    /** @note: USER CODE, Init. Task */
    uint32_t startTick;

    FOREVER
    {
        if (xSemaphoreTake(semHD_SFZ, RTOS_WAIT_FOREVER) == pdTRUE)
        {
            __TASK_TRIGGER_START_Using(TEST_PORT_1, TP_IDX_taskSFZ);
            //==================================================================
            startTick = ITIMER_StartMeasure_us();
            __xTaskStatus[TP_IDX_taskSFZ] = true;
            if (xSystemInfo.PL_Get_FW_Mode() == FW_MODE_IDLE)
            {
                gTick_SFZ = ITIMER_StopMeasure_us(startTick);
                __TASK_TRIGGER_END_Using(TEST_PORT_1, TP_IDX_taskSFZ);
                continue;
            }
            //==================================================================

            /**************************************************************** */
            /** @note USER CODE - START                                       */
            /**************************************************************** */

            // [0]. 통신으로 노드별 데이터 요청
            __xTime_After(__2sec)
            {
                __xTime_Per(__100msec){
                 //  xServoA6.Update_RS485_Tx();
                }
            }

            TMC2660_ControlCurrent(0);
			Drive_GetStatus(0, &MotionStatus);

            ErrorMonitor();

            /**************************************************************** */
            /** @note USER CODE - END                                         */
            /**************************************************************** */
            //==================================================================
            xSL.Time = (F32)gTriggerTime;
            xCD.Time = (F32)gTriggerTime;
            xPL.Header.Time = (F32)gTriggerTime;
            xCD.SL_Size = (F32)sizeof(tsXStateList);
            memcpy(&xCD.SL, &xSL, sizeof(tsXStateList));

            if (xNet.isConnected_TCP == YES)
            {
                // SystemDB_CD_PushAll();
            }
            SFZ_TaskMonitoring();
            gTick_SFZ = ITIMER_StopMeasure_us(startTick);
            __TASK_TRIGGER_END_Using(TEST_PORT_1, TP_IDX_taskSFZ);
        }
    } /*@end FOREVER{}*/
}

int Init_SamplingFinalization(int Index)
{
    int result = EXIT_SUCCESS;

    semHD_SFZ = xSemaphoreCreateBinary();

    return result;
}

void SFZ_TaskMonitoring(void)
{
    static U32 lastSemErrorTick = 0;
    static U32 lastStackErrorTick[NUM_TASKS] = {0};
    const U32 errorInterval = 1000 / 10; // 100 tick = 1s

    // [1] 세마포어 상태 체크
    if (uxSemaphoreGetCount(semHD_UTT) > 0 ||
        uxSemaphoreGetCount(semHD_USD) > 0 ||
        uxSemaphoreGetCount(semHD_SDG) > 0 ||
        uxSemaphoreGetCount(semHD_APC) > 0 ||
        uxSemaphoreGetCount(semHD_SFZ) > 0)
    {
        if ((gTriggerCount - lastSemErrorTick) >= errorInterval)
        {
            ERR_MSG_SEND("Main-Task Semaphore Not Consumed!");
            lastSemErrorTick = gTriggerCount;
        }
    }

    // [2] 스택 사용량 체크
    TaskHandle_t handle[NUM_TASKS] = {
        gTaskHandle_UTT,
        gTaskHandle_USD,
        gTaskHandle_SDG,
        gTaskHandle_APC,
        gTaskHandle_SFZ};

    for (int i = 0; i < NUM_TASKS; i++)
    {
        if (handle[i] != NULL)
        {
            U32 freeStack = uxTaskGetStackHighWaterMark(handle[i]);
            if (freeStack < 50) // 쪼매만 남았으면.. 에러출력
            {
                if ((gTriggerCount - lastStackErrorTick[i]) >= errorInterval)
                {
                    ERR_MSG_SEND("Task[%d] Stack Margin Low! Free=%lu.", i + 1, freeStack);
                    lastStackErrorTick[i] = gTriggerCount;
                }
            }
        }
    }
}
