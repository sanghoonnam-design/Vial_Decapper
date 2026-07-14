/*******************************************************************************
 * _02_zUpdate_Temperature.c
 *
 * Created on: 2025.06.25
 * Author    : RND, Kang PilSoon.
 *
 ******************************************************************************/
#include "XSystemInfo.h"
#include "_02_zUpdate_Temperature.h"

#include "XDebug.h"

typedef struct
{
    uint8_t ch;
    float value;

} tsXTempMsg;

QueueHandle_t xQueue_Temp[TEMP_CH_MAX];

VOID TASK_UpdateTemperature(void *pvParameters)
{
    tsXTempMsg msg;
    const TickType_t xDelay = pdMS_TO_TICKS(100); // 100ms 주기 = 10Hz

    for (;;)
    {
        // [1]. 제어기 4채널 온도 읽기
        for (U08 ch = 0; ch < TEMP_CH_MAX; ch++)
        {
            msg.ch = ch;
            msg.value = TEMP_GetTemprature(ch); // blocking 함수 호출

            // 이전 값 무시하고 최신 값 유지
            xQueueOverwrite(xQueue_Temp[ch], &msg);
        }

        // [2]. MCU 제어기 내부 온도 최신화
        xSystemInfo.cd_CpuTemperature = IADC_GetMcuInternalTemp();

        vTaskDelay(xDelay);
    }
}

void Init_UpdateTemperature(void)
{
    // TEMP_Init();
    
    for (int ch = 0; ch < TEMP_CH_MAX; ch++)
    {
        xQueue_Temp[ch] = xQueueCreate(1, sizeof(tsXTempMsg));
    }
    
    xTaskCreate(TASK_UpdateTemperature, "Task_Temp", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
}

float USD_GetLatestTemp(U08 ch)
{
    tsXTempMsg msg;

    if (xQueuePeek(xQueue_Temp[ch], &msg, 0) == pdPASS)
    {
        return msg.value;
    }

    return -1000.0f; // 실패 시 에러값 리턴
}