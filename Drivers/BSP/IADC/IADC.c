#define __IADC_C__
    #include "IADC.h"
#undef  __IADC_C__

#define TS_CAL1_ADDR   ((uint16_t*)0x1FF0F44C)  // 30 ¡ÆC calibration
#define TS_CAL2_ADDR   ((uint16_t*)0x1FF0F44E)  // 110 ¡ÆC calibration

#define DIGIT2VOLT(digit)	(3.3f * (digit)/4095.f)
#define ADC_V25				(0.76f)
#define ADC_AVG_SLOPE		(0.0025f)

uint16_t ts_cal1;
uint16_t ts_cal2;

ADC_HandleTypeDef       hAdc1;
DMA_HandleTypeDef       hdma_adc1;

static void ADC1_DMA_Init(void);
static void GPIO_Init(void);

#define ADC1_RESOLUTION         4095
#define ADC1_VOLT               (5.0f)
#define ADC1_BUFFER_SIZE        5

U32 adc_buffer[ADC1_BUFFER_SIZE];

void IADC_Init(void)
{
    GPIO_Init();
    ADC1_DMA_Init();

    ts_cal1 = *TS_CAL1_ADDR;
    ts_cal2 = *TS_CAL2_ADDR;
}

F32 IADC_GetMcuInternalTemp(void)
{
#if 1 //calibration
	return ((float)(adc_buffer[4] - ts_cal1) * (110.0f - 30.0f)) / (float)(ts_cal2 - ts_cal1) + 30.0f;
#else
	return (DIGIT2VOLT(adc_buffer[4]) - ADC_V25) / ADC_AVG_SLOPE + 25.f;
#endif
}

F32 IADC_GetVolt(U8 ch)
{
    F32 volt = 0.f;
    U32 sum;
    U32 value;    
    
    if(ch < MAX_ADC_CH)
    {    
        sum = adc_buffer[(ch<<1)] + adc_buffer[(ch<<1)+1];
        value = sum>>1;    
        volt = (F32)value/(F32)ADC1_RESOLUTION * ADC1_VOLT;
    }
   
    return volt;
}

static void ADC1_DMA_Init(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();
    
    hAdc1.Instance                      = ADC1;
    hAdc1.Init.ClockPrescaler           = ADC_CLOCK_SYNC_PCLK_DIV6;
    hAdc1.Init.Resolution               = ADC_RESOLUTION_12B;
    hAdc1.Init.ScanConvMode             = ENABLE;
    hAdc1.Init.ContinuousConvMode       = ENABLE;
    hAdc1.Init.DiscontinuousConvMode    = DISABLE;
    hAdc1.Init.ExternalTrigConvEdge     = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hAdc1.Init.ExternalTrigConv         = ADC_SOFTWARE_START;
    hAdc1.Init.DataAlign                = ADC_DATAALIGN_RIGHT;
    hAdc1.Init.NbrOfConversion          = 5;
    hAdc1.Init.DMAContinuousRequests    = ENABLE;
    hAdc1.Init.EOCSelection             = ADC_EOC_SINGLE_CONV;
    HAL_ADC_Init(&hAdc1);

    ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel      = ADC_CHANNEL_10;
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    HAL_ADC_ConfigChannel(&hAdc1, &sConfig);

    sConfig.Channel      = ADC_CHANNEL_11;
    sConfig.Rank         = 2;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    HAL_ADC_ConfigChannel(&hAdc1, &sConfig);

    sConfig.Channel      = ADC_CHANNEL_12;
    sConfig.Rank         = 3;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    HAL_ADC_ConfigChannel(&hAdc1, &sConfig);

    sConfig.Channel      = ADC_CHANNEL_13;
    sConfig.Rank         = 4;
    sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;
    HAL_ADC_ConfigChannel(&hAdc1, &sConfig);
    
    sConfig.Channel      = ADC_CHANNEL_TEMPSENSOR;
    sConfig.Rank         = 5;
    sConfig.SamplingTime = ADC_SAMPLETIME_144CYCLES;
    HAL_ADC_ConfigChannel(&hAdc1, &sConfig);

    
    hdma_adc1.Instance                  = DMA2_Stream0;
    hdma_adc1.Init.Channel              = DMA_CHANNEL_0;
    hdma_adc1.Init.Direction            = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc            = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc               = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment  = DMA_PDATAALIGN_WORD;
    hdma_adc1.Init.MemDataAlignment     = DMA_MDATAALIGN_WORD;
    hdma_adc1.Init.Mode                 = DMA_CIRCULAR;
    hdma_adc1.Init.Priority             = DMA_PRIORITY_HIGH;
    
    HAL_DMA_Init(&hdma_adc1);

    __HAL_LINKDMA(&hAdc1, DMA_Handle, hdma_adc1);

    HAL_ADC_Start_DMA(&hAdc1, (uint32_t *)adc_buffer, ADC1_BUFFER_SIZE);
    
    // HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 5, 0);
    // HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef  GPIO_InitStruct;
    
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    GPIO_InitStruct.Pin         = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode        = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull        = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}
