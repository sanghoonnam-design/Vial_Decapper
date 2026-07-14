#define __IOEXP_C__
#include "IOEXP.h"
#undef __IOEXP_C__

#define IN_CS_H         HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);       
#define IN_CS_L         HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);    

#define OUT_CS_H        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);       
#define OUT_CS_L        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

#define RESET_H         HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);       
#define RESET_L         HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);      

static void MCP23S17_Init(void);
static void GPIO_Init(void);
static void SPI1_Init(void);

static void WriteReg_In(U8 ch, U8 addr, U16 data);
static U16 ReadReg_In(U8 ch, U8 addr);

static void WriteReg_Out(U8 ch, U8 addr, U16 data);
static U16 ReadReg_Out(U8 ch, U8 addr);

static SPI_HandleTypeDef        Spi1Handle;

void IOEXP_Init(void)
{
    GPIO_Init();
    SPI1_Init();
    
    RESET_H;
    UTIL_DelayMS(1);
    RESET_L;
    UTIL_DelayMS(1);
    RESET_H;
    UTIL_DelayMS(1);
    
    MCP23S17_Init();
    
    IOEXP_WriteIOclear();

    // by KPS
    digitalRead  = IOEXP_ReadIObit;
    digitalWrite = IOEXP_WriteIObit;
    digitalClear = IOEXP_WriteIOclear;
}

U8 IOEXP_ReadIO(void)
{
    U8 data;
    
    data = ReadReg_In(IO_CH0, REG_GPIO);

    return data;
}

void IOEXP_WriteIO(U16 val)
{
    WriteReg_Out(IO_CH0, REG_GPIO, ~val);
}

void IOEXP_WriteIObit(U8 bit, U8 OnOff)
{
    static U16 data;
    
    if(bit < 16)
    {
        data = ReadReg_Out(IO_CH0, REG_GPIO);
        
        if  (OnOff == true) data &= ~(1<<bit);
        else data                |= 1<<bit;
          
        WriteReg_Out(IO_CH0, REG_GPIO, data);
    }
}

U8 IOEXP_ReadIObit(U8 inout, U8 bit)
{
    U16 data   = 0;
    U8  getbit = false;
    
    switch(inout)
    {
    case READ_IN: 
        if(bit < 16) { data = ReadReg_In(IO_CH0, REG_GPIO); }
        else         { data = 0; bit = 0; }
        
        getbit = (data>>bit)&0x01;
        break;
          
    case READ_OUT: 
        if(bit < 16) { data = ReadReg_Out(IO_CH0, REG_GPIO); }
        else         { data = 1; bit = 0; }
        
        getbit = (data>>bit)&0x01 ? false : true;
        break;
    } 
    
    return getbit;
}

void IOEXP_WriteIOclear(void)
{
    IOEXP_WriteIO(0x0000);
}

static void MCP23S17_Init(void)
{    
    WriteReg_In(IO_CH0, REG_IOCON, 0x0000);
    WriteReg_Out(IO_CH0, REG_IOCON, 0x0000);
    
    WriteReg_In(IO_CH0, REG_IPOL,  0xFFFF);
    WriteReg_In(IO_CH0, REG_IODIR, 0xFFFF);
    
    ReadReg_In(IO_CH0, REG_GPIO);
   
    WriteReg_Out(IO_CH0, REG_IODIR, 0x0000);
}

static void GPIO_Init(void)
{   
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    GPIO_InitStruct.Pin       = GPIO_PIN_5  | GPIO_PIN_6  | GPIO_PIN_7;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin   = GPIO_PIN_4  | GPIO_PIN_8;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin   = GPIO_PIN_2;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    IN_CS_H;
    OUT_CS_H;
}    
     
static void SPI1_Init(void)
{
    __HAL_RCC_SPI1_CLK_ENABLE();  
  
    Spi1Handle.Instance               = SPI1;
    Spi1Handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
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
}

static void WriteReg_In(U8 ch, U8 addr, U16 data)
{
    U8 buf[4];
    
    buf[0] = 0x40 | ch<<1;
    buf[1] = addr;
    buf[2] = data&0xFF;
    buf[3] = (data>>8)&0xFF;
    
    IN_CS_L;
    HAL_SPI_TransmitReceive(&Spi1Handle, buf, buf, 4, 5000);  
    IN_CS_H;
}

static U16 ReadReg_In(U8 ch, U8 addr)
{
    U8 buf[4];
    U16 data;
    
    buf[0] = 0x41 | ch<<1;
    buf[1] = addr;
    buf[2] = 0x00;
    buf[3] = 0x00;
    
    IN_CS_L;
    HAL_SPI_TransmitReceive(&Spi1Handle, buf, buf, 4, 5000);
    IN_CS_H;
    
    data = (((U16)buf[3]<<8)&0xFF00) | (buf[2]&0xFF);
    
    return data;
}

static void WriteReg_Out(U8 ch, U8 addr, U16 data)
{
    U8 buf[4];
    
    buf[0] = 0x40 | ch<<1;
    buf[1] = addr;
    buf[2] = data&0xFF;
    buf[3] = (data>>8)&0xFF;
    
    OUT_CS_L;
    HAL_SPI_TransmitReceive(&Spi1Handle, buf, buf, 4, 5000);  
    OUT_CS_H;
}

static U16 ReadReg_Out(U8 ch, U8 addr)
{
    U8 buf[4];
    U16 data;
    
    buf[0] = 0x41 | ch<<1;
    buf[1] = addr;
    buf[2] = 0x00;
    
    OUT_CS_L;
    HAL_SPI_TransmitReceive(&Spi1Handle, buf, buf, 4, 5000);
    OUT_CS_H;
    
    data = (((U16)buf[3]<<8)&0xFF00) | (buf[2]&0xFF);
    
    return data;
}

void IOEXP_GetDigitalInput_str(char *str, int size) // 비트출력, by KPS
{
    int digitalInput[NUM_IN];
    int offset = 0;

    memset(str, 0, size);

    for (int i = 0; i < NUM_IN; i++)
    {
        digitalInput[i] = IOEXP_ReadIObit(READ_IN, i);
        offset += snprintf(str + offset, size - offset, "%d", digitalInput[i]);

        if (((i + 1) % 4) == 0 && (i + 1) <= NUM_IN)
            offset += snprintf(str + offset, size - offset, " ");
    }
}

void IOEXP_GetDigitalOutput_str(char *str, int size)
{
    int digitalOutput[NUM_OUT];
    int offset = 0;

    memset(str, 0, size);

    for (int i = 0; i < NUM_OUT; i++)
    {
        digitalOutput[i] = IOEXP_ReadIObit(READ_OUT, i);
        offset += snprintf(str + offset, size - offset, "%d", digitalOutput[i]);

        if (((i + 1) % 4) == 0 && (i + 1) <= NUM_OUT)
            offset += snprintf(str + offset, size - offset, " ");
    }
}
