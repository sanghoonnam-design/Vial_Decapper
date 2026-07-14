/*******************************************************************************
 * _01_XSystemManagement.c
 *
 * Created on: 2025.06.20
 * Author    : RND. Kang pilSoon.
 * - 시스템을 관리하는 상태, 시간 등에 대해 작성된 코드
 *
 ******************************************************************************/
#include "XSystemInfo.h"
#include "_01_XSystemManagement.h"
#include "_02_XUpdateSigData.h"
#include "XDebug.h"

static uint32_t GetTaskStackSize(const char *taskName);

/**-------------------------------------------------------------------------- */
/** define global var. about time. */
Semaphore_Handle semHD_UTT;     //
                                //
unsigned int gTriggerCount = 0; // 트리거 카운트
float gTriggerTime;             // 시스템 리얼타임
float _Hz;                      // frequency
float _dt;                      // time : sampling time.(sec)
int _dt_ms;                     // time : sampling time.(msec)
float _sec;                     // second

SW_DateTime_t gSWRTC; // 소프트웨어 RTC 시간 정보
/**-------------------------------------------------------------------------- */

/**-------------------------------------------------------------------------- */
static U32 __ulIdleTickCount = 0; // Idle Task에서 소모된 틱 수
static U32 __ulLastTickCount = 0; // 마지막으로 계산된 전체 틱 수
static F32 __fCpuUsage = 0.0f;    // 계산된 CPU 사용률
/**-------------------------------------------------------------------------- */

VOID TASK_UpdateTriggerTime(void *pvParameters)
{
    /** @note: USER CODE, Init. Task */
    uint32_t startTick;

    FOREVER
    {
        if (xSemaphoreTake(semHD_UTT, RTOS_WAIT_FOREVER) == pdTRUE)
        {
            __TASK_TRIGGER_START_Using(TEST_PORT_1, TP_IDX_taskUTT);
            startTick = ITIMER_StartMeasure_us();
            //===============================================================
            __xTaskStatus[TP_IDX_taskUTT] = true;
            XTP_CheckTaskUsingLED(XHW_STATUS_LED_1);
            if (xSystemInfo.PL_Get_FW_Mode() == FW_MODE_IDLE)
            {
                __TASK_TRIGGER_END_Using(TEST_PORT_1, TP_IDX_taskUTT);
                continue;
            }
            __xTime_At(__100msec) xPrint_SystemInfo();
            __xTime_At(__1sec) xSystemInfo.SL_Set_isStartMainLoop(YES);
            //===============================================================

            /* 시스템 트리거 시간(리얼타임) 관리 및 계산 ***********************/
            // gTriggerCount: 트리거 발생 횟수
            // _Hz: 샘플링 주파수(Hz)
            // gTriggerTime: 현재까지 경과한 리얼타임(초)
            gTriggerTime = ((float)(gTriggerCount) / _Hz);

            /*[]. RTC */
            SWRTC_GetParam(&gSWRTC); // 소프트웨어 RTC에서 시간 정보 읽기

            Log_CheckDateChange(); // RTC 날짜 변경 체크 및 플래그 설정

            //===============================================================
            gTick_UTT = ITIMER_StopMeasure_us(startTick);
            __TASK_TRIGGER_END_Using(TEST_PORT_1, TP_IDX_taskUTT);
        }

    } /*@end: FOREVER{}*/
}

int Init_SystemTrigger(int Index)
{
    int result = EXIT_SUCCESS;

    semHD_UTT = xSemaphoreCreateBinary();

    gTriggerTime = 0.0;

    _Hz /*    */ = 100.0f;
    _dt /*    */ = 0.01f;
    _dt_ms /* */ = 10;
    _sec /*   */ = _Hz;

    return result;
}

float GetTriggerTime(void)
{
    return gTriggerTime;
}

VOI XTask_Idle_Start(void)
{
    xSystemInfo.pl_FW_Mode = FW_MODE_IDLE;
}

VOI XTask_Idle_Stop(void)
{
    xSystemInfo.pl_FW_Mode = FW_MODE_DEFAULT;
}

void vIdle_CalculateCpuUsage(void)
{
    /* []. CPU 사용율 계산 */
    /** @brief CPU 점유율 (%) = 100 − ( Idle Task가 차지한 시간 / 전체 시간 x 100)*/

    U32 ulCurrentTickCount = xTaskGetTickCount();                                     // 전체 틱 수 가져오기
    U32 ulElapsedTicks = (U32)((TickType_t)(ulCurrentTickCount - __ulLastTickCount)); // 지난 틱 수 계산

    if (ulElapsedTicks > 0)
    {
        // CPU 사용률 계산: 전체 틱 중 Idle Tick이 차지한 비율
        __fCpuUsage = 100.0f - ((float)__ulIdleTickCount / ulElapsedTicks * 100.0f);

        // LOG_MSG_SEND("%d, %d, %f, %f",__ulIdleTickCount, ulElapsedTicks, ((float)__ulIdleTickCount / ulElapsedTicks * 100.0f), __fCpuUsage);

        __ulLastTickCount = ulCurrentTickCount;
        __ulIdleTickCount = 0;
    }
}

F32 vIdle_GetCpuUsage(void)
{
    return __fCpuUsage;
}

//==============================================================================
/*             IdleHook Start ,  configUSE_IDLE_HOOK 1/0                      */
//==============================================================================
void vApplicationIdleHook(void)
{
    __TASK_TRIGGER_START_Using(TEST_PORT_4, TP_IDX_taskIdling);

#if 0
    /*[]. Idle 상태에서 주기적으로 시스템 상태를 로깅하거나 디버깅 정보를 출력 */
    static uint32_t idleCount = 0;
    idleCount++;

    if (idleCount % 1000000 == 0)
    {
        printf("Idle hook running, count: %lu\n", idleCount);
    }
#endif

    static U32 ulLastTick = 0;
    U32 ulCurrentTick = xTaskGetTickCount();

    // 1ms 간격으로 유휴 시간 추적
    if ((ulCurrentTick - ulLastTick) >= 1) // abt. 1ms마다
    {
        __ulIdleTickCount++;
        ulLastTick = ulCurrentTick;
    }

    /*[]. 백그라운드 작업 수행 */
    // TODO

    __TASK_TRIGGER_END_Using(TEST_PORT_4, TP_IDX_taskIdling);
}
//==============================================================================

/** ********************************************************************************
 * @brief FreeRTOS 타이머 태스크를 위한 메모리 할당
 *
 * FreeRTOSConfig.h에서 configUSE_TIMERS가 1로 설정된 경우,
 * configSUPPORT_STATIC_ALLOCATION이 활성화되면 이 함수를 정의해야 함.
 */
StaticTask_t xTimerTaskTCB;
StackType_t xTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

/* 정적 메모리 제공 함수 정의 */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;               // 타이머 태스크의 TCB 버퍼
    *ppxTimerTaskStackBuffer = xTimerTaskStack;            // 타이머 태스크의 스택 버퍼
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH; // 타이머 태스크 스택 크기
}
/***********************************************************************************/

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("Stack overflow in task: %s\r\n", pcTaskName);

    while (1)
        ;
}

// @note vApplicationMallocFailedHook() 사용하려면 아래항목 추가
#define configUSE_MALLOC_FAILED_HOOK 1
void vApplicationMallocFailedHook(void)
{
    /* 여기서 LED 점멸, 로그 출력, 시스템 리셋 등 수행 */
    printf("Malloc failed! free=%u  min=%u\r\n",
           xPortGetFreeHeapSize(),
           xPortGetMinimumEverFreeHeapSize());

    taskDISABLE_INTERRUPTS();
    for (;;)
        ; /* 디버깅용 무한루프 */
}

void xPrint_SystemInfo(void)
{
    XTimer_Stop();
    printf(ANSI_CLEAR_TERMINAL ANSI_BG_ORG ANSI_TX_ORG); // 콘솔창 클리어
    printf("===================================================\r\n");
    printf("  RND Embedded System - v1.0.0\r\n");
    printf("  Build : %s  %s\r\n", __DATE__, __TIME__);
    printf("  MCU   : STM32F746IGK6 @ 216MHz\r\n");
    printf("===================================================\r\n");

    xCheck_ApplicationFW(); // Application Info
    xCheck_RtosStatus();    // RTOS Info
    xCheck_Peripheral();    // Peripheral & Sensor
    xCheck_Task();          // Task info

    // printf("RND>");
    __prompt();
    XTimer_Start();
    // xCheck_Stack();         //
}

void xCheck_ApplicationFW(void)
{
    XTimer_Stop();

    // @USER CODE START
    printf("App. FW. ver          : %s\r\n", xSystemInfo.cd_FWVersion_str);
    printf("Network IP, port      : %2d.%d.%d.%d, %u\r\n",
           gEEPROM.hwInfo.network.ip[0],
           gEEPROM.hwInfo.network.ip[1],
           gEEPROM.hwInfo.network.ip[2],
           gEEPROM.hwInfo.network.ip[3], gEEPROM.hwInfo.network.portNum);
    // @USER CODE END
    printf("---------------------------------------------------\r\n");
    vTaskDelay(pdMS_TO_TICKS(10));
    XTimer_Start();
}

void xCheck_RtosStatus(void)
{
    XTimer_Stop();
    printf("FreeRTOS Version      : %s\r\n", tskKERNEL_VERSION_NUMBER);
    printf("Heap Total            : %d bytes\r\n", configTOTAL_HEAP_SIZE);
    printf("Heap Free             : %u bytes\r\n", xPortGetFreeHeapSize());
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
    printf("Current Task          : %s\r\n", pcTaskGetName(currentTask));
    printf("System Tick Rate      : %lu Hz\r\n", configTICK_RATE_HZ);
    TickType_t ticks = xTaskGetTickCount();
    printf("Current Tick Count    : %lu ticks (%.2f sec)\r\n", ticks, ticks / (float)configTICK_RATE_HZ);
    printf("---------------------------------------------------\r\n");
    vTaskDelay(pdMS_TO_TICKS(10));
    XTimer_Start();
}

void xCheck_Peripheral(void)
{
    XTimer_Stop();

    printf("EEPROM                : %s\r\n", isEEPROM_OK ? "OK" : "FAIL");
    printf("RS485 CH1             : %s\r\n", _ConfigUSE_HW_RS485_CH_1_ ? "Enable" : "Disable");
    printf("Watch Dog             : %s\r\n", _ConfigUSE_HW_WATCH_DOG_ ? "Enable" : "Disable");
    printf("---------------------------------------------------\r\n");
    vTaskDelay(pdMS_TO_TICKS(10));

    XTimer_Start();
}

void xCheck_Task(void)
{
    static char buffer[1024];

    memset(buffer, 0, sizeof(buffer));

    XTimer_Stop();

#if (configUSE_TRACE_FACILITY == 1) && (configUSE_STATS_FORMATTING_FUNCTIONS == 1)
    vTaskList(buffer);

    UBaseType_t taskCount = uxTaskGetNumberOfTasks();
    printf("\r\n [Task List]  Number of Tasks - %lu \r\n", taskCount);
    printf("---------------------------------------------------\r\n");
    printf("  Task Name\tState\tPrio.\tStack\tRun Num\r\n");
    printf("---------------------------------------------------\r\n");
    char *line = strtok(buffer, "\n");
    while (line != NULL)
    {
        printf("%s\r\n", line);
        vTaskDelay(pdMS_TO_TICKS(1));
        line = strtok(NULL, "\r\n");
    }
    printf("---------------------------------------------------\r\n");

    // printf(" Free heap size: %u bytes → max stack size: %d words.\r\n", (unsigned)xPortGetFreeHeapSize(), (unsigned)((xPortGetFreeHeapSize() - 88) / 4));
    // printf(" Minimum ever free heap size: %u bytes.\r\n", (unsigned)xPortGetMinimumEverFreeHeapSize());

#endif
    XTimer_Start();
}

/* TODO: total = 512; 이부분은 추후 개선 필요 */
#define BAR_LEN 20
void xCheck_Stack(void)
{
    XTimer_Stop();

    printf("\r\n [Stack Usage] \r\n");

    static TaskStatus_t taskStatusArray[24];
    UBaseType_t count = uxTaskGetSystemState(taskStatusArray, 24, NULL);

    printf("%-18s %-6s %-6s %-6s %s\r\n", "Task Name", "Used", "Free", "Usage", "[Usage Bar]");
    printf("------------------------------------------------------------\r\n");

    uint32_t total_stack_all = 0;
    uint32_t total_used_all = 0;

    for (UBaseType_t i = 0; i < count; i++)
    {
        TaskStatus_t *ts = &taskStatusArray[i];
        uint32_t total = GetTaskStackSize(ts->pcTaskName);
        uint32_t freeWords = (ts->usStackHighWaterMark > total) ? total : ts->usStackHighWaterMark;
        uint32_t usedWords = total - freeWords;

        total_stack_all += total;
        total_used_all += usedWords;

        uint32_t usage_percent = (usedWords * 100) / total;
        uint32_t used_bar_len = (usedWords * BAR_LEN) / total;

        printf("%-18s %-6lu %-6lu %3lu%%   ", ts->pcTaskName, usedWords, freeWords, usage_percent);

        for (uint32_t j = 0; j < BAR_LEN; j++)
        {
            if (j < used_bar_len)
                printf("\033[106m#\033[0m"); // cyan background
            else
                printf("-");
        }
        printf("\r\n");
    }

    // 전체 사용량 시각화
    printf("------------------------------------------------------------\r\n");

    uint32_t heap_words = configTOTAL_HEAP_SIZE / 4;
    uint32_t used_bar = (total_used_all * BAR_LEN) / heap_words;
    uint32_t alloc_bar = (total_stack_all * BAR_LEN) / heap_words;

    uint32_t unused_alloc_bar = (alloc_bar > used_bar) ? (alloc_bar - used_bar) : 0;
    uint32_t unallocated_bar = (BAR_LEN > alloc_bar) ? (BAR_LEN - alloc_bar) : 0;

    uint32_t total_usage_percent = (total_used_all * 100) / heap_words;

    printf("%-18s %-6lu %-6lu %3lu%%   ", "Total Stack Usage", total_used_all, heap_words - total_used_all, total_usage_percent);

    for (uint32_t j = 0; j < used_bar; j++)
        printf("\033[44m#\033[0m"); // 파란색 = 실제 사용영역
    for (uint32_t j = 0; j < unused_alloc_bar; j++)
        printf("\033[42m=\033[0m"); // 초록색 = 할당됐지만 미사용 영역
    for (uint32_t j = 0; j < unallocated_bar; j++)
        printf("-"); // 회색 = 아직 미할당, 여유 영역
    printf("\r\n");

    printf("------------------------------------------------------------\r\n");
    printf(" Free heap size: %u bytes → max stack size: %u words.\r\n",
           (unsigned)xPortGetFreeHeapSize(),
           (unsigned)((xPortGetFreeHeapSize() - 88) / 4));
    printf(" Minimum ever free heap size: %u bytes.\r\n\n",
           (unsigned)xPortGetMinimumEverFreeHeapSize());

    XTimer_Start();
}

static uint32_t GetTaskStackSize(const char *taskName)
{
    // @USER CODE : 변경시 업데이트 해서 사용, 거참..거시기 하네..쩝!~
    // FreeRTOS는 기본적으로 태스크 생성 시 사용자가 설정한 스택 크기 (stack depth)를 나중에 직접 조회할 수 있는 API를 제공하지 x
    if (strcmp(taskName, "TASK_LOGO") == 0)
        return 256;
    if (strcmp(taskName, "TASK_mainUTT") == 0)
        return 256;
    if (strcmp(taskName, "TASK_mainUSD") == 0)
        return 256;
    if (strcmp(taskName, "TASK_mainDG") == 0)
        return 256;
    if (strcmp(taskName, "TASK_mainAPP") == 0)
        return 512;
    if (strcmp(taskName, "TASK_mainSFZ") == 0)
        return 256;
    if (strcmp(taskName, "TASK_CMD_Serial") == 0)
        return 256;
    if (strcmp(taskName, "TASK_CMD_Net") == 0)
        return 512;
    if (strcmp(taskName, "Task_CLI_RX") == 0)
        return 256;
    if (strcmp(taskName, "TASK_CLI_TX") == 0)
        return 400;
    if (strcmp(taskName, "TASK_RS485") == 0)
        return 256;
    if (strcmp(taskName, "Task_WDG") == 0)
        return 128;
    if (strcmp(taskName, "Task_Temp") == 0)
        return 256;
    if (strcmp(taskName, "Tmr Svc") == 0)
        return configTIMER_TASK_STACK_DEPTH;
    if (strcmp(taskName, "IDLE") == 0)
        return configMINIMAL_STACK_SIZE;

    return 128; // default
}

U32 GetCurrentDate(void)
{
    // 소프트웨어 RTC에서 날짜와 시간 정보를 가져와서 YYMMDD 형식의 정수로 반환
    // 예: 2026년 3월 15일 -> 260315
    return (gSWRTC.year % 100) * 10000 + gSWRTC.month * 100 + gSWRTC.day;
}

void Log_CheckDateChange(void)
{
    static int prevDay = -1;
    static int preIsUpdate = NO;

    int isUpdated = xSystemInfo.sl_isDateChanged;

    if ((isUpdated && !preIsUpdate) || (prevDay != gSWRTC.day))
    {
        preIsUpdate = YES;
        prevDay = gSWRTC.day;

        Debug_LogMessageHelper(
            "\r\n===== "
            "DATE UPDATED : 20%02d-%02d-%02d, %02d:%02d:%02d"
            " =====\r\n",
            gSWRTC.year,
            gSWRTC.month,
            gSWRTC.day,
            gSWRTC.hour,
            gSWRTC.min,
            gSWRTC.sec);
    }
}
