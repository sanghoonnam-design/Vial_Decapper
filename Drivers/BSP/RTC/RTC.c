#define __RTC_C__
    #include "RTC.h"
#undef  __RTC_C__

#define DELAY_TIME              (1)

#define REG_SECONDS             0x80
#define REG_MINUTES             0x82
#define REG_HOUR                0x84
#define REG_DATE                0x86
#define REG_MONTH               0x88
#define REG_DAY                 0x8A
#define REG_YEAR                0x8C
#define REG_WP                  0x8E
#define REG_BURST               0xBE

#define CS_HIGH()               (GPIOB->BSRR = GPIO_PIN_9)
#define CS_LOW()                (GPIOB->BSRR = (uint32_t)GPIO_PIN_9<<16U)  

#define CLK_HIGH()              (GPIOB->BSRR = GPIO_PIN_10)
#define CLK_LOW()               (GPIOB->BSRR = (uint32_t)GPIO_PIN_10<<16U)  
   
#define OUT_HIGH()              (GPIOB->BSRR = GPIO_PIN_11)
#define OUT_LOW()               (GPIOB->BSRR = (uint32_t)GPIO_PIN_11<<16U)  
     
#define READ_INPUT()            (GPIOB->IDR & GPIO_PIN_11)

#define INPUT_MODE()            (GPIOB->MODER &= ~(1<<22))
#define OUTPUT_MODE()           (GPIOB->MODER |= (1<<22))

static void GPIO_Init(void);
static void HW_DelayUS(U16 wUS);

static U8 IsWriteProtected(void);
static U8 IsHalted(void);
static void DisableWriteProtect(void);
static void DisableHalt(void);
static void PrepareRead(U8 address);
static void PrepareWrite(U8 address);
static void End(void);
static U8 ReadByte(void);
static void WriteByte(U8 value);
static void NextBit(void);
static U8 Dec2bcd(U8 dec);
static U8 Bcd2dec(U8 bcd);

void ERTC_Init(void)
{
    GPIO_Init();

    CS_LOW();
    CLK_LOW();
    
    if( IsWriteProtected() ) DisableWriteProtect();
    if( IsHalted() )         DisableHalt();

    for(int i=0; i<10000; i++); 
}

void ERTC_SetParam(DateTime_t *dt)
{
    PrepareWrite(REG_WP);
    WriteByte(0x00);
    End();

    PrepareWrite(REG_BURST);
    WriteByte(Dec2bcd(dt->sec    % 60 ));
    WriteByte(Dec2bcd(dt->min    % 60 ));
    WriteByte(Dec2bcd(dt->hour   % 24 ));
    WriteByte(Dec2bcd(dt->date   % 32 ));
    WriteByte(Dec2bcd(dt->month  % 13 ));
    WriteByte(Dec2bcd(dt->day    % 8  ));
    WriteByte(Dec2bcd(dt->year   % 100));
    WriteByte(0x80);
    End();  
}

void ERTC_GetParam(DateTime_t *dt)
{
    PrepareRead(REG_BURST);
    dt->sec    = Bcd2dec(ReadByte() & 0x7F);
    dt->min    = Bcd2dec(ReadByte() & 0x7F);
    dt->hour   = Bcd2dec(ReadByte() & 0x3F);
    dt->date   = Bcd2dec(ReadByte() & 0x3F);
    dt->month  = Bcd2dec(ReadByte() & 0x1F);
    dt->day    = Bcd2dec(ReadByte() & 0x07);
    dt->year   = Bcd2dec(ReadByte() & 0x7F);
    End();
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    OUTPUT_MODE();
    
    CS_LOW();
    CLK_LOW();
    OUT_LOW();
}

static void HW_DelayUS(U16 wUS)
{
    volatile U32 Dly = (U32)wUS*3;
    for(; Dly; Dly--);
}

static U8 IsHalted(void)
{
    PrepareRead(REG_SECONDS);
    U8 seconds = ReadByte();
    End();
    return (seconds & 0x80);
}

static U8 IsWriteProtected(void)
{
    PrepareRead(REG_WP);
    U8 wp = ReadByte();
    End();
    return (wp & 0x80);  
}

static void DisableWriteProtect(void)
{
    PrepareWrite(REG_WP);
    WriteByte(0x00);
    End();
}

static void DisableHalt(void)
{
    PrepareWrite(REG_SECONDS);
    WriteByte(0x00);
    End();
}

static void PrepareRead(U8 address)
{
    OUTPUT_MODE();
    CS_HIGH();
    
    U8 command = 0x81 | address;
    WriteByte(command);
    INPUT_MODE();
}

static void PrepareWrite(U8 address)
{
    OUTPUT_MODE();
    CS_HIGH();
    
    U8 command = 0x80 | address;
    WriteByte(command);
}

static void End(void)
{
    CS_LOW();
}

static U8 ReadByte(void)
{
    U8 byte = 0;

    for(U8 b = 0; b < 8; b++)
    {
        if (READ_INPUT()) byte |= 0x01 << b;
        NextBit();
    }

    return byte;
}

static void WriteByte(U8 value)
{
    for(U8 b = 0; b < 8; b++)
    {
        (value & 0x01) ? OUT_HIGH() : OUT_LOW();
        NextBit();
        value >>= 1;
    }
}

static void NextBit(void)
{
    HW_DelayUS(1);
    CLK_HIGH();
    HW_DelayUS(1);    
    CLK_LOW();
}

static U8 Dec2bcd(U8 dec)
{
    return ((dec / 10 * 16) + (dec % 10));
}

static U8 Bcd2dec(U8 bcd)
{
    return ((bcd / 16 * 10) + (bcd % 16));
}

