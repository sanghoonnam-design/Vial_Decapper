#define __ITIMER_C__
#include "ITIMER.h"
#undef __ITIMER_C__

#include "_01_XSystemManagement.h"
#include "_02_XUpdateSIGData.h"
#include "_04_XDiagnose.h"
#include "_06_XAppControl.h"
#include "_07_XSamplingFinalization.h"
#include "SWRTC.h"

//==============================================================================
/**
 * @brief 태스크 sampling 간격 / 시작시간 조절 변수
 * \note  Timer 3은 1KHz로 셋팅 하고,
 *        아래 샘플링 조절 변수로 app. 샘플링 간격과, 시작시간(shift)를 조절함.
 * *****************************************************************************/
static const int TRIGGER_DOWN_SAMPLING = 10;
static const int USD_DOWN_SAMPLING = 10;
static const int SDG_DOWN_SAMPLING = 10;
static const int APC_DOWN_SAMPLING = 10;
static const int SFZ_DOWN_SAMPLING = 10;

/* time shifting parameters */
static const int TRIGGER_Timeout = 0; // shift timeout
static const int USD_Timeout = 0;
static const int SDG_Timeout = 0;
static const int APC_Timeout = 0; // 2000;
static const int SFZ_Timeout = 0;

/* static variables. */
static int TRIGGER_TimeCount; // Synchronizing the frequency(Hz) of the Control
static int USD_TimeCount;
static int SDG_TimeCount;
static int APC_TimeCount;
static int SFZ_TimeCount;
//==============================================================================

static TIM_HandleTypeDef hTim2;

static TIM_HandleTypeDef hTim5;

static void Timer2_Init(void);
static void Timer5_Init(void);

void ITIMER_Init(void)
{
    Timer2_Init();
    Timer5_Init();
}

static void Timer2_Init(void)
{
    // TIM3의 클럭을 활성화
    __HAL_RCC_TIM2_CLK_ENABLE();

    /* Set TIMx = Timer3 instance */
    hTim2.Instance = TIM2;

    // 타이머 초기화
    hTim2.Init.Period = 1000 - 1;
    hTim2.Init.Prescaler = 108 - 1;
    hTim2.Init.ClockDivision = 0;
    hTim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    hTim2.Init.RepetitionCounter = 0;
    hTim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&hTim2);

    // 인터럽트 우선순위 설정 및 활성화
    HAL_NVIC_SetPriority(TIM2_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    // TIM3_Callback = NULL;  // not used.

    // 타이머 시작 : main() 에서 활성화
    // HAL_TIM_Base_Start_IT(&hTim2);

    // 태스크 sampling 간격 / 시작시간 조절, by KPS
    TRIGGER_TimeCount = TRIGGER_DOWN_SAMPLING + TRIGGER_Timeout;
    USD_TimeCount = USD_DOWN_SAMPLING + USD_Timeout;
    SDG_TimeCount = SDG_DOWN_SAMPLING + SDG_Timeout;
    APC_TimeCount = APC_DOWN_SAMPLING + APC_Timeout;
    SFZ_TimeCount = SFZ_DOWN_SAMPLING + SFZ_Timeout;
}

uint32_t ITIMER_StartMeasure_us(void)
{
    return __HAL_TIM_GET_COUNTER(&hTim5);
}

uint32_t ITIMER_StopMeasure_us(uint32_t startTick)
{
    uint32_t endTick = __HAL_TIM_GET_COUNTER(&hTim5);
    uint32_t elapsedTick;

    if (endTick >= startTick)
        elapsedTick = endTick - startTick;
    else
        elapsedTick = (0xFFFFFFFF - startTick + endTick + 1); // overflow 보정

    return elapsedTick; // us 단위 시간
}

static void Timer5_Init(void)
{
    // TIM3의 클럭을 활성화
    __HAL_RCC_TIM5_CLK_ENABLE();

    /* Set TIMx = Timer3 instance */
    hTim5.Instance = TIM5;
    hTim5.Init.Period = 0xFFFFFFFF;
    hTim5.Init.Prescaler = 108 - 1;
    hTim5.Init.ClockDivision = 0;
    hTim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    hTim5.Init.RepetitionCounter = 0;
    hTim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&hTim5);
    HAL_TIM_Base_Start(&hTim5);
}

void Timer2_InterruptStart(void)
{
    hTim2.Instance->CNT = 0;
    HAL_TIM_Base_Start_IT(&hTim2);
}

void Timer2_InterruptStop(void)
{
    if (__HAL_TIM_GET_IT_SOURCE(&hTim2, TIM_IT_UPDATE))
        HAL_TIM_Base_Stop_IT(&hTim2);
}

U8 Timer2_GetInterruptStatus(void)
{
    return __HAL_TIM_GET_IT_SOURCE(&hTim2, TIM_IT_UPDATE);
}

/** ****************************************************************************
 * @brief Timer 3 ISR (Interrupt Service Routine)
 * ****************************************************************************/
void TIM2_IRQHandler(void)
{
    static BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (__HAL_TIM_GET_FLAG(&hTim2, TIM_FLAG_UPDATE) != RESET)
    {
        if (__HAL_TIM_GET_IT_SOURCE(&hTim2, TIM_IT_UPDATE) != RESET)
        {
            __HAL_TIM_CLEAR_IT(&hTim2, TIM_IT_UPDATE);

            TRIGGER_TimeCount--;
            USD_TimeCount--;
            SDG_TimeCount--;
            APC_TimeCount--;
            SFZ_TimeCount--;

            SWRTC_TickISR();
#if 1
            if (!TRIGGER_TimeCount)
            {
                gTriggerCount++;
                xSemaphoreGiveFromISR(semHD_UTT, &xHigherPriorityTaskWoken);
                TRIGGER_TimeCount = TRIGGER_DOWN_SAMPLING;
            }
#endif

#if 1
            if (!USD_TimeCount)
            {
                xSemaphoreGiveFromISR(semHD_USD, &xHigherPriorityTaskWoken);
                USD_TimeCount = USD_DOWN_SAMPLING;
            }
#endif

#if 1
            if (!SDG_TimeCount)
            {
                xSemaphoreGiveFromISR(semHD_SDG, &xHigherPriorityTaskWoken);
                SDG_TimeCount = SDG_DOWN_SAMPLING;
            }
#endif

#if 1
            if (!APC_TimeCount)
            {
                xSemaphoreGiveFromISR(semHD_APC, &xHigherPriorityTaskWoken);
                APC_TimeCount = APC_DOWN_SAMPLING;
            }
#endif

#if 1
            if (!SFZ_TimeCount)
            {
                xSemaphoreGiveFromISR(semHD_SFZ, &xHigherPriorityTaskWoken);
                SFZ_TimeCount = SFZ_DOWN_SAMPLING;
            }
#endif

            // context switching 시작
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
    }
}

void ITIMER_ResetSemaphores(void)
{
    xQueueReset(semHD_UTT);
    xQueueReset(semHD_USD);
    xQueueReset(semHD_SDG);
    xQueueReset(semHD_APC);
    xQueueReset(semHD_SFZ);
}
