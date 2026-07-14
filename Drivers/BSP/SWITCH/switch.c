#define __SWITCH_C__
    #include "switch.h"
#undef  __SWITCH_C__

static void GPIO_Init(void);

void SW_Init(void)
{
    GPIO_Init();
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOG_CLK_ENABLE();

    GPIO_InitStruct.Pin   = GPIO_PIN_0  | GPIO_PIN_1  | GPIO_PIN_2  | GPIO_PIN_3;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    SW_Read = SW_Input;
}

U8 SW_Input(ESwNum num)
{
    U8 sw = 0;
    
    switch(num)
    {
    case SW_ALL : sw = (~GPIOG->IDR & 0x0F);    break;
    case SW1    : sw = PSW1();                  break;
    case SW2    : sw = PSW2();                  break;
    case SW3    : sw = PSW3();                  break;
    case SW4    : sw = PSW4();                  break;
    default: break;
    }
    
    return sw;
}
