/*******************************************************************************
 * HW_WatchDog.c
 *
 *  Created on: 2024.12.20
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#include "HW_WatchDog.h"
#include "IWDG.h"
#include "XSystemInfo.h"
#include "_01_XSystemManagement.h"
#include "XDebug.h"

volatile bool __xTaskStatus[NUM_TASKS] = {false};

/** ********************************************************************************
 * @brief 독립적인 Watchdog 관리 태스크 운영
 * @note
 *        독립적인 Watchdog 관리 태스크를 생성하고,
 *        각 태스크의 정상 동작을 확인하는 플래그(예: __xTaskStatus[] 배열)를 사용.
 *        각 태스크는 자신의 작업이 완료될 때마다 Watchdog 관리 태스크로 플래그를
 *        설정하고, Watchdog 관리 태스크는 모든 플래그를 확인한 후에 Watchdog을
 *        리셋함.
 *
 *        [WatchDog 신뢰성 향상]  
 *        이 방식은 가장 신뢰성이 높으며, 각 태스크가 정상 동작했는지 명확하게 확인 할 수 있음.
 * *********************************************************************************/
#if configWatchDog_ENABLE
VOID TASK_Watchdog(void *pvParameters)
{
    static U8 timerStatus     = START;
    static U8 old_timerStatus = STOP;

    while (1)
    {
        //__xTime_Before(__1sec) continue;

        bool allTasksHealthy = true;

          /*[]. 모든 태스크의 상태 확인 */
        timerStatus = XTimer_GetStatus();
        if (timerStatus == STOP)
        {                  // 이경우 메인 태스크가 멈춰있어 watchDog 실행됨.
            IWDG_Reset();  // 제어기 리셋을 방지, XTimer_Start()/XTimer_Stop() 사용시 발생
        }
        else if (timerStatus == START)
        {
            if(timerStatus != old_timerStatus)
            {
                IWDG_Reset();  // 제어기 리셋을 방지, XTimer_Start()/XTimer_Stop() 사용시 발생
            }
            else if (xSystemInfo.PL_Get_FW_Mode() == FW_MODE_DEFAULT)  // timer가 동작중일때만 확인. /
            {
                for (int i = 0; i < NUM_TASKS; i++)
                {
                    if (!__xTaskStatus[i])
                    {
                        xcprintf(ANSI_TX_LightYellow);
                        xcprintf("\r\n[WatchDog] Oops!~.  Reboot...");
                        xcprintf(ANSI_TX_ORG);

                        vTaskDelay(50);

                        allTasksHealthy = false;
                        break;
                    }
                }
            }
        }

        if (allTasksHealthy)
        {
            IWDG_Reset();  // Watchdog 리셋
        }

        /*[]. __xTaskStatus 초기화 */
        for (int i = 0; i < NUM_TASKS; i++)
        {
            __xTaskStatus[i] = false;
        }

        old_timerStatus = timerStatus;

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
#endif
