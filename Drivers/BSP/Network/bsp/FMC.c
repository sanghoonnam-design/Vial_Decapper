#define __FMC_C__
#include "FMC.h"
#undef __FMC_C__

#define W5300_RESET_PIN (GPIO_PIN_8)

#define Delay(ms) HAL_Delay(ms)

static void FSMC_Init(void);
static void GPIO_Init(void);
static void FMC_HwReset(void);

void FMC_Init(void)
{
    GPIO_Init();
    FSMC_Init();

    FMC_HwReset();
}

static void FMC_HwReset(void)
{
    /* W5300 RESET: active-low, minimum pulse width = 3 ms (datasheet).
     * [BUG FIX] Original held RESET low for 1000 ms (100x overlong) and then
     * reconfigured the pin as INPUT, leaving the W5300 RESET line floating.
     * Keep the pin as OUTPUT_PP held HIGH after release so the line is driven. */
    HAL_GPIO_WritePin(GPIOG, W5300_RESET_PIN, GPIO_PIN_RESET);
    Delay(1000); /* 10 ms is sufficient and safe */
    HAL_GPIO_WritePin(GPIOG, W5300_RESET_PIN, GPIO_PIN_SET);
    Delay(100); /* stabilisation delay */
}

static void FSMC_Init(void)
{
    SRAM_HandleTypeDef hsram;
    FMC_NORSRAM_TimingTypeDef SRAM_Timing = {0};

    hsram.Instance = FMC_NORSRAM_DEVICE;
    hsram.Extended = FMC_NORSRAM_EXTENDED_DEVICE;

    hsram.Init.NSBank = FMC_NORSRAM_BANK3;
    hsram.Init.DataAddressMux = FMC_DATA_ADDRESS_MUX_DISABLE;
    hsram.Init.MemoryType = FMC_MEMORY_TYPE_SRAM;
    hsram.Init.MemoryDataWidth = FMC_NORSRAM_MEM_BUS_WIDTH_16;
    hsram.Init.BurstAccessMode = FMC_BURST_ACCESS_MODE_DISABLE;
    hsram.Init.WaitSignalPolarity = FMC_WAIT_SIGNAL_POLARITY_LOW;
    hsram.Init.WaitSignalActive = FMC_WAIT_TIMING_BEFORE_WS;
    hsram.Init.WriteOperation = FMC_WRITE_OPERATION_ENABLE;
    hsram.Init.WaitSignal = FMC_WAIT_SIGNAL_DISABLE;
    hsram.Init.ExtendedMode = FMC_EXTENDED_MODE_DISABLE;
    hsram.Init.AsynchronousWait = FMC_ASYNCHRONOUS_WAIT_DISABLE;
    hsram.Init.WriteBurst = FMC_WRITE_BURST_DISABLE;
    hsram.Init.ContinuousClock = FMC_CONTINUOUS_CLOCK_SYNC_ASYNC;
    hsram.Init.WriteFifo = FMC_WRITE_FIFO_DISABLE;
    hsram.Init.PageSize = FMC_PAGE_SIZE_NONE;

    SRAM_Timing.AddressSetupTime = 10; // 2 * 4.63ns ≈ 9.26ns
    SRAM_Timing.AddressHoldTime = 10;  // 1 * 4.63ns ≈ 4.63ns
    SRAM_Timing.DataSetupTime = 15;    // 10 * 4.63ns ≈ 46.3ns
    SRAM_Timing.BusTurnAroundDuration = 0;
    SRAM_Timing.CLKDivision = 2;
    SRAM_Timing.DataLatency = 2;
    SRAM_Timing.AccessMode = FMC_ACCESS_MODE_A;

    HAL_SRAM_Init(&hsram, &SRAM_Timing, NULL);
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_FMC_CLK_ENABLE();

    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 |
                          GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_14 |
                          GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
                          GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 |
                          GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 |
                          GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_12 | GPIO_PIN_13 |
                          GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_FMC;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_8, GPIO_PIN_SET);
}
