/*******************************************************************************
 * XUpdateSigData.c
 *
 * Created on: 2025.06.20
 * Author    : RND, Kang PilSoon.
 *
 ******************************************************************************/
#include "XSystemInfo.h"
#include "XSystem_DB.h"
#include "_02_XUpdateSIGData.h"
#include "_02_zUpdate_Temperature.h"
#include "XFilter.h"
#include "XDebug.h"

Semaphore_Handle semHD_USD;

#define SAMPLING_FREQ (100.0f)         // 100Hz 샘플링
#define CUTOFF_FREQ_LOADCELL (5.0f)    // 차단주파수
#define CUTOFF_FREQ_TEMPERATURE (3.0f) // 차단주파수

VOID TASK_UpdateSIGData(void *pvParameters)
{
    /** @note: USER CODE, Init. Task */
    uint32_t startTick;

    Init_UpdateTemperature(); // temp. 센서값 획득

    FOREVER
    {
        if (xSemaphoreTake(semHD_USD, RTOS_WAIT_FOREVER) == pdTRUE)
        {
            __TASK_TRIGGER_START_Using(TEST_PORT_1, TP_IDX_taskUSD);
            //===============================================================
            startTick = ITIMER_StartMeasure_us();
            __xTaskStatus[TP_IDX_taskUSD] = true;

            if (xSystemInfo.PL_Get_FW_Mode() == FW_MODE_IDLE)
            {
                gTick_USD = ITIMER_StopMeasure_us(startTick);
                __TASK_TRIGGER_END_Using(TEST_PORT_1, TP_IDX_taskUSD);
                continue;
            }
            XTP_CheckTaskUsingLED(XHW_STATUS_LED_2);
            //===============================================================

            xDoor.Update(); 
            xServoA6.Update();

            xSL.isBusy = xDoor.IsBusy() || xServoA6.IsBusy();

            //===============================================================
            gTick_USD = ITIMER_StopMeasure_us(startTick);
            __TASK_TRIGGER_END_Using(TEST_PORT_1, TP_IDX_taskUSD);
        }
    } /*@end FOREVER{}*/
}

int Init_UpdateSIGData(int Index)
{
    int result = EXIT_SUCCESS;

    /*[]. init. semaphore */
    semHD_USD = xSemaphoreCreateBinary();

    return result;
}
