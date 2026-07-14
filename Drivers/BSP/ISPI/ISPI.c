#define __ISPI_C__
    #include "ISPI.h"
#undef  __ISPI_C__

static void ISPI1_GPIO_Init(void);
static void ISPI2_GPIO_Init(void);

static SPI_HandleTypeDef Spi1Handle;
static SPI_HandleTypeDef Spi2Handle;

void ISPI1_Init(void)
{   
    ISPI1_GPIO_Init();
    
    __HAL_RCC_SPI1_CLK_ENABLE();  

    Spi1Handle.Instance               = SPI1;
    Spi1Handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;   //9MhZ
    Spi1Handle.Init.Direction         = SPI_DIRECTION_2LINES;
    Spi1Handle.Init.CLKPhase          = SPI_PHASE_2EDGE;
    Spi1Handle.Init.CLKPolarity       = SPI_POLARITY_HIGH;
    Spi1Handle.Init.DataSize          = SPI_DATASIZE_8BIT;
    Spi1Handle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    Spi1Handle.Init.TIMode            = SPI_TIMODE_DISABLE;
    Spi1Handle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    Spi1Handle.Init.CRCPolynomial     = 7;
    Spi1Handle.Init.NSS               = SPI_NSS_SOFT;
    Spi1Handle.Init.Mode              = SPI_MODE_MASTER;
    HAL_SPI_Init(&Spi1Handle);  
    
    __HAL_SPI_ENABLE(&Spi1Handle);
}

void ISPI2_Init(void)
{    
    ISPI2_GPIO_Init();
    
    __HAL_RCC_SPI2_CLK_ENABLE();  

    Spi2Handle.Instance               = SPI2;
    Spi2Handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;   //9MhZ
    Spi2Handle.Init.Direction         = SPI_DIRECTION_2LINES;
    Spi2Handle.Init.CLKPhase          = SPI_PHASE_2EDGE;
    Spi2Handle.Init.CLKPolarity       = SPI_POLARITY_HIGH;
    Spi2Handle.Init.DataSize          = SPI_DATASIZE_8BIT;
    Spi2Handle.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    Spi2Handle.Init.TIMode            = SPI_TIMODE_DISABLE;
    Spi2Handle.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    Spi2Handle.Init.CRCPolynomial     = 7;
    Spi2Handle.Init.NSS               = SPI_NSS_SOFT;
    Spi2Handle.Init.Mode              = SPI_MODE_MASTER;
    HAL_SPI_Init(&Spi2Handle);  
    
    __HAL_SPI_ENABLE(&Spi2Handle);
}

static void ISPI1_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin   = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;                           //GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
    GPIO_InitStruct.Pin   = GPIO_PIN_4 | GPIO_PIN_11;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
    SPI1_CSH();
}

static void ISPI2_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin   = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
    GPIO_InitStruct.Pin   = GPIO_PIN_12;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
	
    SPI2_CSH();
}

void SPI1_RST_H(void)   { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_SET);   }
void SPI1_RST_L(void)   { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET); }

void SPI1_CSH(void)     { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);   }
void SPI1_CSL(void)     { HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); }

void SPI2_CSH(void)     { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);   }
void SPI2_CSL(void)     { HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); }

U8 ISPI1_SendData(U8 data)
{
    while ( (SPI1->SR & SPI_FLAG_TXE) == RESET );
    SPI1->DR = data;
    while ( (SPI1->SR & SPI_FLAG_RXNE) == RESET );
	
    return SPI1->DR;
}

U8 ISPI2_SendData(U8 data)
{    
    while ( (SPI2->SR & SPI_FLAG_TXE) == RESET );
    SPI2->DR = data;
    while ( (SPI2->SR & SPI_FLAG_RXNE) == RESET );

    return SPI2->DR;
}