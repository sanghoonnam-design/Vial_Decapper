#define __EEPROM_C__
#include "EEPROM.h"
#undef __EEPROM_C__

#define SLAVE_ADDRESS 0xA0
#define PAGE_SIZE 64
#define PAGE_NUM 512 // 64 X 512 = 32

static void GPIO_Init(void);
static void I2C1_Init(void);

static I2C_HandleTypeDef I2c1Handle;

void EEPROM_Init(void)
{
    GPIO_Init();
    I2C1_Init();
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct;

    RCC_PeriphCLKInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2C1;
    RCC_PeriphCLKInitStruct.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
    HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
}

static void I2C1_Init(void)
{
    I2c1Handle.Instance = I2C1;
    I2c1Handle.Init.Timing = 0x50330309;
    I2c1Handle.Init.OwnAddress1 = 0;
    I2c1Handle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    I2c1Handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    I2c1Handle.Init.OwnAddress2 = 0xFF;
    I2c1Handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    I2c1Handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    HAL_I2C_Init(&I2c1Handle);

    HAL_I2CEx_ConfigAnalogFilter(&I2c1Handle, I2C_ANALOGFILTER_ENABLE);
    HAL_I2CEx_ConfigDigitalFilter(&I2c1Handle, 0);
}

void EEPROM_WriteBytes(U16 addr, U8 *bytes, U16 size)
{
    U16 newAddr, newSize;
    U8 stream[PAGE_SIZE + 2];
    U8 *pData = bytes;
    U16 cnt = 0;

    if (addr > (PAGE_SIZE * PAGE_NUM - 1))
        return;
    if (size > PAGE_SIZE * PAGE_NUM)
        return;

    newAddr = addr;
    if ((addr % PAGE_SIZE + size) / PAGE_SIZE > 0)
        newSize = PAGE_SIZE - (addr % PAGE_SIZE);
    else
        newSize = size;

    while (size)
    {
        stream[0] = newAddr >> 8;
        stream[1] = newAddr & 0xFF;
        memcpy(&stream[2], pData, newSize);
        pData += newSize;

        while (HAL_I2C_Master_Transmit(&I2c1Handle, (uint16_t)SLAVE_ADDRESS, (uint8_t *)stream, newSize + 2, 500) != HAL_OK)
        {
            if (HAL_I2C_GetError(&I2c1Handle) != HAL_I2C_ERROR_AF)
            {
            }
        }

        newAddr += newSize;
        size -= newSize;
        if (size > PAGE_SIZE)
            newSize = PAGE_SIZE;
        else
            newSize = size;

        if (cnt++ > PAGE_NUM)
            break;
    }
}

void EEPROM_ReadBytes(U16 addr, U8 *bytes, U16 size)
{
    U8 txStream[2];

    if (addr > (PAGE_SIZE * PAGE_NUM - 1))
        return;
    if (size > PAGE_SIZE * PAGE_NUM)
        return;

    txStream[0] = addr >> 8;
    txStream[1] = addr & 0xFF;

    while (HAL_I2C_Master_Transmit(&I2c1Handle, (uint16_t)SLAVE_ADDRESS, (uint8_t *)txStream, 2, 500) != HAL_OK)
    {
        if (HAL_I2C_GetError(&I2c1Handle) != HAL_I2C_ERROR_AF)
        {
            // Error_Handler();
        }
    }

    while (HAL_I2C_Master_Receive(&I2c1Handle, (uint16_t)SLAVE_ADDRESS, (uint8_t *)bytes, size, 500) != HAL_OK)
    {
        if (HAL_I2C_GetError(&I2c1Handle) != HAL_I2C_ERROR_AF)
        {
            // Error_Handler();
        }
    }
}


