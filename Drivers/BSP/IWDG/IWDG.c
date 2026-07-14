#define __IWDG_C__
#include "IWDG.h"
#undef __IWDG_C__

static IWDG_HandleTypeDef hiwdg;

static void WDG_Init(void);

void IWDG_Init(void)
{
	__HAL_DBGMCU_FREEZE_IWDG();
    WDG_Init();
}

static void WDG_Init(void)
{
    /* Set TIMx instance */
    hiwdg.Instance = IWDG;

    hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
    hiwdg.Init.Window = 4095;
    // hiwdg.Init.Reload    = 2000 - 1;           // 2초로 셋팅
    hiwdg.Init.Reload = 5000 - 1; // 5초로 셋팅

    HAL_IWDG_Init(&hiwdg);

    //    if(HAL_IWDG_Init(&hiwdg) != HAL_OK)
    //    {
    //        Error_Handler();
    //    }
}

void IWDG_Start(void)
{
    __HAL_IWDG_START(&hiwdg);
}

void IWDG_Reset(void)
{
    HAL_IWDG_Refresh(&hiwdg);
}
