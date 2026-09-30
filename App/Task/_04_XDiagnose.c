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
#include "CDecap.h"

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

/**
 * @brief 진단 주기에 Decapper 센서 조합을 검사하고 오류 표시를 갱신한다.
 * FW_MODE_IDLE 또는 진단 비활성 상태에서는 건너뛴다. 센서 오류 플래그의 실제 정지 연결은 FSM 설정과 구분한다.
 */
VOID TASK_Diagnose(void *pvParameters)
{
    /** @note: USER CODE, Init. Task */
    uint32_t startTick;

    FOREVER
    {
        if (xSemaphoreTake(semHD_SDG, RTOS_WAIT_FOREVER) == pdTRUE)
        {
            __TASK_TRIGGER_START_Using(TEST_PORT_1, TP_IDX_taskSDG);
            //==================================================================
            startTick = ITIMER_StartMeasure_us();
            __xTaskStatus[TP_IDX_taskSDG] = true;

            if (xSystemInfo.PL_Get_FW_Mode() == FW_MODE_IDLE || DB_isDiagnosisDisabled == YES)
            {
                gTick_SDG = ITIMER_StopMeasure_us(startTick);
                __TASK_TRIGGER_END_Using(TEST_PORT_1, TP_IDX_taskSDG);
                continue;
            }
            XTP_CheckTaskUsingLED(XHW_STATUS_LED_3);
            //==================================================================
            xCDecap.CheckSensorValidity();
            //==================================================================
            ErrorMonitor_LED();
            // SystemInfo_CheckCpuTemperature();
            gTick_SDG = ITIMER_StopMeasure_us(startTick);
            __TASK_TRIGGER_END_Using(TEST_PORT_1, TP_IDX_taskSDG);
        }
    } /*@end FOREVER{}*/
}

/**
 * @brief 진단 태스크의 주기 실행을 깨우는 이진 세마포어를 생성한다.
 */
int Init_Diagnose(int Index)
{
    int result = EXIT_SUCCESS;

    semHD_SDG = xSemaphoreCreateBinary();

    return result;
}

