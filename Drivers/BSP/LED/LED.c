
#define __LED_C__
    #include "LED.h"
#undef  __LED_C__

static void GPIO_Init(void);

void LED_Init(void)
{
    GPIO_Init();
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();
    
    GPIO_InitStruct.Pin = GPIO_PIN_4  | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_3  | GPIO_PIN_6  | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_4  | GPIO_PIN_5  | GPIO_PIN_6  | GPIO_PIN_11 | GPIO_PIN_15; 
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4  | GPIO_PIN_5,  GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_4  | GPIO_PIN_5,  GPIO_PIN_SET);
    
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_3  | GPIO_PIN_6,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOF, GPIO_PIN_10 | GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_6, GPIO_PIN_RESET);
    
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7,  GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_11 | GPIO_PIN_15,  GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_2,  GPIO_PIN_SET);    
}

void LED_OnOff(ELedNum num, U8 OnOff)
{
    if(OnOff == true)
    {
        switch(num)
        {
        case STATUS2 : HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);   break;
        case STATUS3 : HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);   break;
        case STATUS0 : HAL_GPIO_WritePin(GPIOG, GPIO_PIN_4, GPIO_PIN_RESET);   break;
        case STATUS1 : HAL_GPIO_WritePin(GPIOG, GPIO_PIN_5, GPIO_PIN_RESET);   break;
        }
    }
    else
    {
        switch(num)
        {
        case STATUS2 : HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);     break;
        case STATUS3 : HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);     break;
        case STATUS0 : HAL_GPIO_WritePin(GPIOG, GPIO_PIN_4, GPIO_PIN_SET);     break;
        case STATUS1 : HAL_GPIO_WritePin(GPIOG, GPIO_PIN_5, GPIO_PIN_SET);     break;
        }
    }
}

void LED_Toggle(ELedNum num)
{
    switch(num)
    {
    case STATUS2 : HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_4);       break;
    case STATUS3 : HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_5);       break;
    case STATUS0 : HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_4);       break;
    case STATUS1 : HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_5);       break;
    }
}

void LED_Toggle_h(ELedNum num, U32 interval)
{
    static uint32_t previousTick[4] = {0};

    uint32_t currentTick = HAL_GetTick();

    if ((currentTick - previousTick[num]) > interval) // 속도 조절
    {
        switch (num)
        {
        case STATUS2: HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_4); break;
        case STATUS3: HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_5); break;
        case STATUS0: HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_4);  break;
        case STATUS1: HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_5);  break;
        }

        previousTick[num] = HAL_GetTick();
    }
}


void LED_DriveOnOff(U8 ch, ELedDir dir, U8 OnOff)
{
    if(OnOff == true)
    {
        if(ch==0)
        {
            switch(dir)
            {
            case LED_CW  : HAL_GPIO_WritePin(GPIOI, GPIO_PIN_2, GPIO_PIN_RESET);        break;
            case LED_CCW : HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);        break;
            }
        }
        else
        {
            switch(dir)
            {
            case LED_CW  : HAL_GPIO_WritePin(GPIOG, GPIO_PIN_11, GPIO_PIN_RESET);       break;
            case LED_CCW : HAL_GPIO_WritePin(GPIOG, GPIO_PIN_15, GPIO_PIN_RESET);       break;
            }
        }
    }
    else
    {
        if(ch==0)
        {
            switch(dir)
            {
            case LED_CW  : HAL_GPIO_WritePin(GPIOI, GPIO_PIN_2, GPIO_PIN_SET);          break;
            case LED_CCW : HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);          break;
            }
        }
        else
        {
            switch(dir)
            {
            case LED_CW  : HAL_GPIO_WritePin(GPIOG, GPIO_PIN_11, GPIO_PIN_SET);         break;
            case LED_CCW : HAL_GPIO_WritePin(GPIOG, GPIO_PIN_15, GPIO_PIN_SET);         break;
            }
        }
    }
}

void TP_OnOff(ETpNum num, U8 OnOff)
{
    if(OnOff == true)
    {
        switch(num)
        {
        case TP10 : TP10_HIGH();        break;
        case TP11 : TP11_HIGH();        break;
        case TP12 : TP12_HIGH();        break;
        case TP15 : TP15_HIGH();        break;
        case TP16 : TP16_HIGH();        break;
        }
    }
    else
    {
        switch(num)
        {
        case TP10 : TP10_LOW();         break;
        case TP11 : TP11_LOW();         break;
        case TP12 : TP12_LOW();         break;
        case TP15 : TP15_LOW();         break;
        case TP16 : TP16_LOW();         break;
        }
    }
}

void TP_Toggle(ETpNum num)
{
    switch(num)
    {
    case TP10 : HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_3);  break;
    case TP11 : HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_6);  break;
    case TP12 : HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_10); break;
    case TP15 : HAL_GPIO_TogglePin(GPIOF, GPIO_PIN_11); break;
    case TP16 : HAL_GPIO_TogglePin(GPIOG, GPIO_PIN_6);  break;
    }
}


