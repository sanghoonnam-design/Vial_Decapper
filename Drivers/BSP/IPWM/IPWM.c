#define __IPWM_C__
#include "IPWM.h"
#undef __IPWM_C__

static TIM_HandleTypeDef hTim8;

static void Timer8_Init(void);
static void GPIO_Init(void);

void IPWM_Init(void)
{
    GPIO_Init();
    Timer8_Init();
}

void IPWM_Enable(U8 ch)
{
    switch (ch)
    {
    case PWM_CH0:
        HAL_TIM_PWM_Start(&hTim8, TIM_CHANNEL_1);
        break;
    case PWM_CH1:
        HAL_TIM_PWM_Start(&hTim8, TIM_CHANNEL_2);
        break;
    default:
        break;
    }
}

void IPWM_Disable(U8 ch)
{
    switch (ch)
    {
    case PWM_CH0:
        HAL_TIM_PWM_Stop(&hTim8, TIM_CHANNEL_1);
        break;
    case PWM_CH1:
        HAL_TIM_PWM_Stop(&hTim8, TIM_CHANNEL_2);
        break;
    default:
        break;
    }
}

void IPWM_UpdateDuty(U8 ch, U16 duty)
{
    switch (ch)
    {
    case PWM_CH0:
        TIM8->CCR1 = duty;
        break;
    case PWM_CH1:
        TIM8->CCR2 = duty;
        break;
    default:
        break;
    }
}

#if 0 // by KPS
static void Timer8_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};
    
    __HAL_RCC_TIM8_CLK_ENABLE();
    
      /* Set TIMx instance */
    hTim8.Instance = TIM8;

    hTim8.Init.Period            = 1000 - 1;
    hTim8.Init.Prescaler         = 216 - 1;
    hTim8.Init.ClockDivision     = 0;
    hTim8.Init.CounterMode       = TIM_COUNTERMODE_UP;
    hTim8.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    hTim8.Init.RepetitionCounter = 0;
    hTim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&hTim8);
    
    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
                          // sConfigOC.Pulse      = 500;
    sConfigOC.Pulse = 0;  // by KPS
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    
    HAL_TIM_PWM_ConfigChannel(&hTim8, &sConfigOC, TIM_CHANNEL_1);
    HAL_TIM_PWM_ConfigChannel(&hTim8, &sConfigOC, TIM_CHANNEL_2);
}
#else
static void Timer8_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    __HAL_RCC_TIM8_CLK_ENABLE();

    /* Set TIMx instance */
    hTim8.Instance = TIM8;

    //AC Control Period
#if 0
    hTim8.Init.Prescaler = 18000 - 1; // 216MHz / 1800000 = 12000Hz
	hTim8.Init.Period = 12000 - 1;     // 12000Hz × 12000 = 1초
#endif

	hTim8.Init.Prescaler = 21600 - 1;  // 216MHz / 21600 = 10000Hz
	hTim8.Init.Period = 10000 - 1;     // 10000Hz × 10000 = 1초

    hTim8.Init.CounterMode = TIM_COUNTERMODE_UP;
    hTim8.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    hTim8.Init.RepetitionCounter = 0;
    hTim8.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_PWM_Init(&hTim8);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    // sConfigOC.Pulse      = 500;
    sConfigOC.Pulse = 0; // by KPS
    sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    HAL_TIM_PWM_ConfigChannel(&hTim8, &sConfigOC, TIM_CHANNEL_1);
    HAL_TIM_PWM_ConfigChannel(&hTim8, &sConfigOC, TIM_CHANNEL_2);

#if 1
    HAL_TIM_PWM_Start(&hTim8, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&hTim8, TIM_CHANNEL_2);
#endif
}
#endif

static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF3_TIM8;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

#if 0 // prescaler 만 조절하는 방식
       /**
 * @brief  원하는 PWM 주기(ms)와 분해능(step)을 동적으로 설정
 * @param  period_ms   : PWM 한 사이클 시간 [ms] (예: 1000 ⇒ 1 s)
 * @param  resolution  : 분해능(카운트 수). 1000 ⇒ CCR 0~999
 * @note   Duty 값은 현재 값을 유지하되, 새 Period보다 크면 자동으로 잘라 줍니다.
 *         prescaler 는 16-bit(0xFFFF) 한계를 넘지 않도록 클램프합니다.
 */
void IPWM_SetPeriod(uint32_t period_ms, uint16_t resolution)
{
    /*--------------------------------------------------------------------
     * 1) 현재 TIM8 클럭 구하기 (APB2 prescaler 에 따라 ×2 필요)
     *------------------------------------------------------------------*/
    uint32_t tim_clk = HAL_RCC_GetPCLK2Freq();           // APB2 clock
    if ((RCC->CFGR & RCC_CFGR_PPRE2) != RCC_HCLK_DIV1)   // APB2 prescaler ≠ 1 ?
        tim_clk *= 2;                                    // Timer clock x2

    /*--------------------------------------------------------------------
     * 2) prescaler 계산
     *    tick_us = (주기[us]) / 분해능
     *             = (period_ms * 1000) / resolution
     *    prescaler = tim_clk * tick_us / 1 000 000
     *------------------------------------------------------------------*/
    uint64_t tick_us   = ((uint64_t)period_ms * 1000ULL) / resolution;
    uint64_t prescaler = ( (uint64_t)tim_clk * tick_us + 500000ULL ) / 1000000ULL; // 반올림
    if (prescaler == 0)      prescaler = 1;        // 0 금지
    if (prescaler > 0xFFFF)  prescaler = 0xFFFF;   // 16-bit 한계

    uint32_t arr = resolution - 1;                 // Auto-Reload(Period)

    /*--------------------------------------------------------------------
     * 3) 기존 듀티값 보존 & 범위 조정
     *------------------------------------------------------------------*/
    uint32_t duty1 = TIM8->CCR1;
    uint32_t duty2 = TIM8->CCR2;
    if (duty1 > arr) duty1 = arr;
    if (duty2 > arr) duty2 = arr;

    /*--------------------------------------------------------------------
     * 4) 타이머 재설정
     *    - PWM을 잠시 멈추고 prescaler / ARR / CCR 업데이트
     *------------------------------------------------------------------*/
    __HAL_TIM_DISABLE(&hTim8);               // 출력 정지
    __HAL_TIM_SET_PRESCALER(&hTim8, prescaler - 1);
    __HAL_TIM_SET_AUTORELOAD(&hTim8, arr);

    __HAL_TIM_SET_COMPARE(&hTim8, TIM_CHANNEL_1, duty1);
    __HAL_TIM_SET_COMPARE(&hTim8, TIM_CHANNEL_2, duty2);

    __HAL_TIM_ENABLE(&hTim8);                // 출력 재개
}
#else

/** 사용 예시
 * IPWM_SetPeriod( 500,  4000);  // 주기 0.5초,  분해능 4000  → ARR: 3999  → 1 tick = 0.125ms
 * IPWM_SetPeriod( 800,  4000);  // 주기 0.8초,  분해능 4000  → ARR: 3999  → 1 tick = 0.2ms
 * IPWM_SetPeriod(1000,  5000);  // 주기 1.0초,  분해능 5000  → ARR: 4999  → 1 tick = 0.2ms
 * IPWM_SetPeriod(1000, 10000);  // 주기 1.0초,  분해능 10000 → ARR: 9999  → 1 tick = 0.1ms
 * IPWM_SetPeriod(1200,  6000);  // 주기 1.2초,  분해능 6000  → ARR: 5999  → 1 tick = 0.2ms
 * IPWM_SetPeriod(1500,  4000);  // 주기 1.5초,  분해능 4000  → ARR: 3999  → 1 tick = 0.375ms
 * IPWM_SetPeriod(2000,  2000);  // 주기 2.0초,  분해능 2000  → ARR: 1999  → 1 tick = 1ms
 */
void IPWM_SetPeriod(uint32_t period_ms, uint16_t resolution)
{
    uint32_t tim_clk = HAL_RCC_GetPCLK2Freq(); // APB2 클럭
    if ((RCC->CFGR & RCC_CFGR_PPRE2) != RCC_HCLK_DIV1)
        tim_clk *= 2; // APB2 prescaler ≠ 1 → 타이머 클럭은 2배

    // 1초 = 1000ms → 1000000us
    uint64_t total_ticks = ((uint64_t)period_ms * (uint64_t)tim_clk) / 1000;

    // 분해능 기반으로 ARR/PSC 나누기
    if (resolution == 0)
        resolution = 1; // 보호

    uint32_t arr = resolution - 1;
    uint64_t prescaler = total_ticks / resolution;

    // prescaler 범위 제한 (16bit)
    if (prescaler < 1)
        prescaler = 1;
    if (prescaler > 0xFFFF)
        prescaler = 0xFFFF;

    arr = (uint32_t)(total_ticks / prescaler) - 1;

    // 현재 듀티값 백업 및 보정
    uint32_t duty1 = TIM8->CCR1;
    uint32_t duty2 = TIM8->CCR2;
    if (duty1 > arr)
        duty1 = arr;
    if (duty2 > arr)
        duty2 = arr;

    // 타이머 재설정
    __HAL_TIM_DISABLE(&hTim8);
    __HAL_TIM_SET_PRESCALER(&hTim8, prescaler - 1);
    __HAL_TIM_SET_AUTORELOAD(&hTim8, arr);
    __HAL_TIM_SET_COMPARE(&hTim8, TIM_CHANNEL_1, duty1);
    __HAL_TIM_SET_COMPARE(&hTim8, TIM_CHANNEL_2, duty2);
    __HAL_TIM_ENABLE(&hTim8);
}
#endif

/**
 * @brief 채널별 PWM 듀티사이클을 퍼센트(%)로 설정
 * @param ch      : PWM_CH0 또는 PWM_CH1
 * @param percent : 듀티사이클 [%] (예: 0 ~ 100)
 */
void IPWM_SetDutyPercent(U8 ch, float percent)
{
    // saturation
    if (percent < 0.0f)
        percent = 0.0f;
    if (percent > 100.0f)
        percent = 100.0f;

    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&hTim8);
    uint32_t ccr = (uint32_t)((percent / 100.0f) * (arr + 1));

    switch (ch)
    {
    case PWM_CH0:
        __HAL_TIM_SET_COMPARE(&hTim8, TIM_CHANNEL_1, ccr);
        break;
    case PWM_CH1:
        __HAL_TIM_SET_COMPARE(&hTim8, TIM_CHANNEL_2, ccr);
        break;
    default:
        break;
    }
}

/**
 * @brief PWM 듀티사이클을 CCR(raw) 값으로 설정
 * @param ch  : PWM_CH0 또는 PWM_CH1
 * @param duty: 0 ~ resolution-1 범위의 raw duty 값
 */
void IPWM_SetDutyRaw(U8 ch, uint32_t duty)
{
    // saturation
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&hTim8);
    if (duty > arr)
        duty = arr;

    switch (ch)
    {
    case PWM_CH0:
        __HAL_TIM_SET_COMPARE(&hTim8, TIM_CHANNEL_1, duty);
        break;
    case PWM_CH1:
        __HAL_TIM_SET_COMPARE(&hTim8, TIM_CHANNEL_2, duty);
        break;
    default:
        break;
    }
}
