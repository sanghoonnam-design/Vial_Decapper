#define __TEMP_C__
    #include "TEMP.h"
#undef  __TEMP_C__

#define AD7124_CS_H()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
#define AD7124_CS_L()   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

#define TC1_TYPE()      (GPIOB->IDR & GPIO_PIN_8) ? 1 : 0;
#define TC2_TYPE()      (GPIOB->IDR & GPIO_PIN_9) ? 1 : 0;
#define TC3_TYPE()      (GPIOB->IDR & GPIO_PIN_10) ? 1: 0;
#define TC4_TYPE()      (GPIOB->IDR & GPIO_PIN_11) ? 1: 0;

static SPI_HandleTypeDef        hSpi2;

U8 SensorType[TEMP_CH_MAX];

U8 TEMP_SensorType(U8 ch);
F32 TEMP_GetPT100(U8 ch);

static void GPIO_Init(void);
static void SPI2_Init(void);

static void WriteReg(U8 reg, U32 data, U8 size);
static U32 ReadReg(U8 reg, U8 size);

static void AD7124_Init(void);
static void AD7124_SwReset(void);

static void AD7124_SetAdcCtrl(void);
//static void AD7124_SetIoCtrl(U8 OutCh, U8 Cur);
static void AD7124_SetIoCtrl(U8 iout0_ch, U8 iout1_ch,U8 iout0, U8 iout1);
static void AD7124_SetCh(U8 ch, U8 ainp, U8 ainm, U8 setup);
static void AD7124_ClearCh(U8 ch);

static S32 AD7124_ReadData(U8 ch, U8 ainp, U8 ainm, U8 setup);

static F32 AD7124_CJC_Temp(U32 raw);
static F32 AD7124_AdcTomV(U32 raw);
static F32 AD7124_CompTemp(F32 mv, F32 cjc);

static F32 AD7124_AdcToRes(U32 raw);
static F32 AD7124_ResToTemp(F32 r);

double RTD_ResistanceToTemperature(double R);

uint8_t ad7124_id;
void TEMP_Init(void)
{
    U8 ch;
    
    GPIO_Init();
    SPI2_Init();
    
    UTIL_DelayMS(1);   
    
    for(ch=0; ch<TEMP_CH_MAX; ch++)
    {
        SensorType[ch] = TEMP_SensorType(ch);
    }
    
    AD7124_Init(); 

    ad7124_id = ReadReg(0x05,1);
}

F32 TEMP_GetTemprature(U8 ch)
{
    F32 temp;
    
    if(ch > TEMP_CH3) return 0.f;

    if(SensorType[ch] == TEMP_PT100)
    {
        temp = TEMP_GetPT100(ch);
    }
    else
    {
        temp = TEMP_GetKTYPE(ch);
    }
    
    return temp;
}

F32 TEMP_GetKTYPE(U8 ch)
{ 
    U32 raw;
    F32 mv;
    F32 cjc;
    F32 kTemp;
    
    U8 chn, ainp, ainm;
    U8 chc;
    
    if(ch > TEMP_CH3) return 0.f;
    
    switch(ch)
    {
    case 0: chn = 8;  ainp = 2;  ainm = 3;      break;
    case 1: chn = 9;  ainp = 4;  ainm = 5;      break;
    case 2: chn = 10; ainp = 8;  ainm = 9;      break;
    case 3: chn = 11; ainp = 12; ainm = 13;     break;
    }
    
    chc = 15;
    
    raw = AD7124_ReadData(chn, ainp, ainm, CONFIG0);
    mv  = AD7124_AdcTomV(raw);
    
    raw = AD7124_ReadData(chc, CH_SENSOR, CH_AVSS, CONFIG2);
    cjc = AD7124_CJC_Temp(raw);
    
    kTemp = AD7124_CompTemp(mv, cjc);
    
    return kTemp;
}

U32 adcValueTest;
F64 adcVolt;
F64 pt100_res;
F64 PT100_TEMP;
F64 pt100TempGain = 1.025f;
F64 pt100TempOffset = -5.66;
F32 TEMP_GetPT100(U8 ch)
{
    U32 raw[2] = {0};
    U8  ain;
    U8  chn;
    U8  ainp, ainm;
    F32 r[2] = {0};
    F32 rt;
    
    F32 TcTemp;
    
    if(ch > TEMP_CH3) return 0.f;   
    
    switch(ch)
    {
    case 0: chn = 0;  ain = 0;   ainp = 2;   ainm = 3;  break;
    case 1: chn = 2;  ain = 6;   ainp = 4;   ainm = 5;  break;
    case 2: chn = 4;  ain = 10;  ainp = 8;   ainm = 9;  break;
    case 3: chn = 6;  ain = 14;  ainp = 12;  ainm = 13; break;
    }

    //AD7124_SetIoCtrl(ain, IO1_CUR500);
    AD7124_SetIoCtrl(0, 1, 3, 3);
    adcValueTest = AD7124_ReadData(0, 2, 3, CONFIG1);

    adcVolt = adcValueTest * (double)4.6566e-9f;
    pt100_res = adcVolt*4000.f;
    PT100_TEMP = (RTD_ResistanceToTemperature(pt100_res) - pt100TempOffset)*pt100TempGain;
    /*
    raw[0] = AD7124_ReadData(chn, ainp, ainm, CONFIG1);
    raw[1] = AD7124_ReadData(chn+1, ainp, ainm, CONFIG1);

    r[0] = AD7124_AdcToRes(raw[0]);
    r[1] = AD7124_AdcToRes(raw[1]);
    rt   = (r[0]+r[1])/2.0f;
    

    AD7124_SetIoCtrl(ain, 0);
    
    if(rt == 0.0)
    {
        TcTemp = 0.0f;
    }
    else
    {
        TcTemp = AD7124_ResToTemp(rt);
    }
    
    if(TcTemp > 100.f)
    {
        TcTemp = 0.0f;
    }
    */
    return (F32)PT100_TEMP;
    //return TcTemp;
}

U8 TEMP_SensorType(U8 ch)
{
    U8 type = TEMP_KTYPE;
    
    switch(ch)
    {
    case TEMP_CH0: type = TC1_TYPE();  break;
    case TEMP_CH1: type = TC2_TYPE();  break;
    case TEMP_CH2: type = TC3_TYPE();  break;
    case TEMP_CH3: type = TC4_TYPE();  break;
         default : break;
    }
    
    return type;
}

static void GPIO_Init(void)
{   
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    GPIO_InitStruct.Pin       = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = GPIO_PIN_12;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin   = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    AD7124_CS_H();
}    
     
static void SPI2_Init(void)
{
    __HAL_RCC_SPI2_CLK_ENABLE();  
  
    hSpi2.Instance               = SPI2;
    hSpi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;    // 54MHz/64 = 0.84375MHz
    hSpi2.Init.Direction         = SPI_DIRECTION_2LINES;
    hSpi2.Init.CLKPhase          = SPI_PHASE_2EDGE;
    hSpi2.Init.CLKPolarity       = SPI_POLARITY_HIGH;
    hSpi2.Init.DataSize          = SPI_DATASIZE_8BIT;
    hSpi2.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hSpi2.Init.TIMode            = SPI_TIMODE_DISABLE;
    hSpi2.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hSpi2.Init.CRCPolynomial     = 10;
    hSpi2.Init.NSS               = SPI_NSS_SOFT;
    hSpi2.Init.Mode              = SPI_MODE_MASTER;
    HAL_SPI_Init(&hSpi2);
}

static void WriteReg(U8 reg, U32 data, U8 size) 
{
    U8 tx[5] = {0};

    tx[0] = 0x00 | (reg & 0x3F);
    
    switch(size)
    {
    case 1: 
        tx[1] = data & 0xFF;
        break;
        
    case 2: 
        tx[1] = (data >> 8) & 0xFF;
        tx[2] = data & 0xFF;
        break;
        
    case 3: 
        tx[1] = (data >> 16) & 0xFF;
        tx[2] = (data >> 8) & 0xFF;
        tx[3] = data & 0xFF;
        break;
        
    default: 
        size = 0;
        break;
    }

    AD7124_CS_L();
    HAL_SPI_Transmit(&hSpi2, &tx[0], 1, HAL_MAX_DELAY);
    HAL_SPI_Transmit(&hSpi2, &tx[1], size, HAL_MAX_DELAY);
    AD7124_CS_H();
}

static U32 ReadReg(U8 reg, U8 size) 
{
    U8 tx;
    U8  rx[4] = {0};
    U32 data  = 0;
    
    if(size > 3) { return 0; }
    
    tx = 0x40 | (reg & 0x3F);

    AD7124_CS_L();
    HAL_SPI_Transmit(&hSpi2, &tx, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hSpi2, rx, size, HAL_MAX_DELAY);
    AD7124_CS_H();

    switch(size)
    {
    case 1: 
        data = (U32)rx[0];
        break;
    case 2: 
        data = ((U32)rx[0]<<8) | (U32)rx[1];
        break;
    case 3: 
        data = ((U32)rx[0]<<16) | ((U32)rx[1]<<8) | (U32)rx[2];
        break;
    default: 
        data = 0;
        break;
    }
    
    return data;
}

static void AD7124_Init(void)
{    
    U32 id         = 0;
    U32 sts        = 0;
    U32 err        = 0;
    U32 rConfig[3] = {0};
    U8 ch;
    
    AD7124_SwReset();
    
    UTIL_DelayUS(1000);
    
    id  = ReadReg(REG_ID, SIZE_ID);
    err = ReadReg(REG_ERROR, SIZE_ERROR);
    sts = ReadReg(REG_STATUS, SIZE_STATUS);
    
    for(ch=0; ch<16; ch++)
    {
        WriteReg(REG_CH0+ch, 0x0000, SIZE_CH);
    }
    
    //WriteReg(REG_ADC_CTRL, MODE_REF_ENABLE | MODE_FULL_POWER | MODE_SINGLE, SIZE_ADC_CTRL);

    WriteReg(REG_CONFIG0, CFG_UNIPOLAR | CFG_REF_BUFP | CFG_REF_BUFM | CFG_AIN_BUFP | CFG_AIN_BUFM | CFG_REF_SEL2 | CFG_GAIN_128, SIZE_CONFIG);  //CONFIG0 : K-TYPE
    WriteReg(REG_CONFIG1, CFG_UNIPOLAR | CFG_REF_BUFP | CFG_REF_BUFM | CFG_AIN_BUFP | CFG_AIN_BUFM | CFG_REF_SEL0 | CFG_GAIN_32, SIZE_CONFIG);                                 //CONFIG1 : PT100
    WriteReg(REG_CONFIG2, CFG_BIPOLAR | CFG_REF_BUFP | CFG_REF_BUFM | CFG_AIN_BUFP | CFG_AIN_BUFM | CFG_REF_SEL2 | CFG_GAIN_1, SIZE_CONFIG);     //CONFIG7 : INTERNAL TEMP

    WriteReg(REG_FILTER0, FLT_FS128, SIZE_FILTER);  //FILTER0 : K-TYPE
    WriteReg(REG_FILTER1, FLT_FS128, SIZE_FILTER);  //FILTER1 : PT100
    WriteReg(REG_FILTER2, FLT_FS128, SIZE_FILTER);  //FILTER2 : INTERNAL TEMP

    rConfig[0] = ReadReg(REG_CONFIG0, SIZE_CONFIG);
    rConfig[1] = ReadReg(REG_CONFIG1, SIZE_CONFIG);
    rConfig[2] = ReadReg(REG_CONFIG2, SIZE_CONFIG);

    sts = 0;
}

/*
static void AD7124_Init(void)
{
    U32 id         = 0;
    U32 sts        = 0;
    U32 err        = 0;
    U32 rConfig[3] = {0};
    U8 ch;

    AD7124_SwReset();

    UTIL_DelayUS(1000);

    id  = ReadReg(REG_ID, SIZE_ID);
    err = ReadReg(REG_ERROR, SIZE_ERROR);
    sts = ReadReg(REG_STATUS, SIZE_STATUS);

    for(ch=0; ch<16; ch++)
    {
        WriteReg(REG_CH0+ch, 0x0000, SIZE_CH);
    }

    WriteReg(REG_ADC_CTRL, MODE_REF_ENABLE | MODE_FULL_POWER | MODE_SINGLE, SIZE_ADC_CTRL);
    
    WriteReg(REG_CONFIG0, CFG_UNIPOLAR | CFG_REF_BUFP | CFG_REF_BUFM | CFG_AIN_BUFP | CFG_AIN_BUFM | CFG_REF_SEL2 | CFG_GAIN_128, SIZE_CONFIG);  //CONFIG0 : K-TYPE
    WriteReg(REG_CONFIG1, CFG_UNIPOLAR | CFG_REF_BUFM | CFG_AIN_BUFM | CFG_REF_SEL0 | CFG_GAIN_16, SIZE_CONFIG);                                 //CONFIG1 : PT100
    WriteReg(REG_CONFIG2, CFG_BIPOLAR | CFG_REF_BUFP | CFG_REF_BUFM | CFG_AIN_BUFP | CFG_AIN_BUFM | CFG_REF_SEL2 | CFG_GAIN_1, SIZE_CONFIG);     //CONFIG7 : INTERNAL TEMP
    
    WriteReg(REG_FILTER0, FLT_FS128, SIZE_FILTER);  //FILTER0 : K-TYPE
    WriteReg(REG_FILTER1, FLT_FS128, SIZE_FILTER);  //FILTER1 : PT100
    WriteReg(REG_FILTER2, FLT_FS128, SIZE_FILTER);  //FILTER2 : INTERNAL TEMP
    
    rConfig[0] = ReadReg(REG_CONFIG0, SIZE_CONFIG);
    rConfig[1] = ReadReg(REG_CONFIG1, SIZE_CONFIG);
    rConfig[2] = ReadReg(REG_CONFIG2, SIZE_CONFIG);
    
    sts = 0;
}
*/

static void AD7124_SwReset(void)
{
    U8 tx[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    AD7124_CS_L();
    HAL_SPI_Transmit(&hSpi2, tx, 8, HAL_MAX_DELAY);
    AD7124_CS_H();  
}

static void AD7124_SetAdcCtrl(void)
{
    U32 reg = 0;
    
    //reg = (U32)MODE_REF_ENABLE | (U32)MODE_FULL_POWER | (U32)MODE_SINGLE;
    reg = (U32)MODE_FULL_POWER | (U32)MODE_SINGLE;
    WriteReg(REG_ADC_CTRL, reg, SIZE_ADC_CTRL);
}

static void AD7124_SetIoCtrl(U8 iout0_ch, U8 iout1_ch,U8 iout0, U8 iout1)
{
    U32 reg = 0;

    reg |= iout0_ch&0xF;
    reg |= (iout1_ch&0xF)<<4;
    reg |= (iout0&0x07)<<8;
    reg |= (iout1&0x07)<<11;

    WriteReg(REG_IO_CTRL1, reg, SIZE_IO_CTRL1);
}

/*
static void AD7124_SetIoCtrl(U8 OutCh, U8 Cur)
{
    U32 reg = 0;

    reg |= (Cur&0x07)<<8;
    reg |= (OutCh&0x0F);
    
    WriteReg(REG_IO_CTRL1, reg, SIZE_IO_CTRL1);
}
*/

static void AD7124_SetCh(U8 ch, U8 ainp, U8 ainm, U8 setup)
{
    U32 reg;
    
    reg = (U32)CH_ENABLE | ((U32)(setup & 0x07) << 12) | ((U32)(ainp & 0x1F) << 5) | (U32)(ainm & 0x1F);
    WriteReg(REG_CH0 + ch, reg, SIZE_CH);
}

static void AD7124_ClearCh(U8 ch)
{
    U32 reg = 0;

    WriteReg(REG_CH0 + ch, reg, SIZE_CH);
}

static S32 AD7124_ReadData(U8 ch, U8 ainp, U8 ainm, U8 setup)
{
    U32 sts;
    U32 data;
    
    AD7124_SetCh(ch, ainp, ainm, setup); 
    AD7124_SetAdcCtrl();

    do{
        sts = ReadReg(REG_STATUS, SIZE_STATUS);
    }while((sts&0x80)!=0);
        
    data = ReadReg(REG_DATA, SIZE_DATA);
    
    AD7124_ClearCh(ch);
    
    return data;
}

static F32 AD7124_CJC_Temp(U32 raw)
{
    const F32 CJC_COEFF  = 13584.0f;
    const F32 CJC_OFFSET = 272.5f;
    
    S32 diff;
    F32 temp;   
    
    diff = (S32)raw-0x800000;
    temp = ((F32)diff/CJC_COEFF) - CJC_OFFSET;
    
    return temp;
}

static F32 AD7124_AdcTomV(U32 raw)
{
    F32 mV;
    
    mV = ((float)raw / RESOLUTION) * (REF_VOLT/G128) * 1000.0f;
    
    return mV;
}

double t_type_k_mv_to_temp_pos(double mv) 
{
    const double c[] = 
    {
     0.000000000000E+00,
     2.508355E+01,
     7.860106E-02,
    -2.503131E-01,
     8.315270E-02,
    -1.228034E-02,
     9.804036E-04,
    -4.413030E-05,
     1.057734E-06,
    -1.052755E-08
    };
    
    double mv_pow = 1.0;
    double temp = 0.0;
    for (int i = 0; i < 10; i++)
    {
        temp += c[i] * mv_pow;
        mv_pow *= mv;
    }
    
    return temp;
}

static F32 AD7124_CompTemp(F32 mv, F32 cjc)
{
    F32 temp;
    
    if(mv < -10.0f || mv > 19.0f)
    {
        return 0.f;
    }    
    
    temp = (F32)t_type_k_mv_to_temp_pos((double)mv) + cjc;
    
    return temp;
}

static F32 AD7124_AdcToRes(U32 raw)
{
    F32 r;
    F32 v;
    F32 i = 500e-6f;
    
    v = ((F32)raw / RESOLUTION) * (REF_VOLT/G16);    
    r = v / i;
    
    return r;      
}

#define R0      100.0f
#define A       3.9083e-3f
#define B       -5.775e-7f

// 저항 값(R)으로부터 온도(T)를 계산 (0°C 이상에서만 유효)
double RTD_ResistanceToTemperature(double R) {
    double discriminant = A*A - 4*B*(1 - R/R0);

    if (discriminant < 0) {
        // 근이 없음 (비정상 입력 또는 0°C 이하)
        return -999.9;
    }

    double sqrt_val = sqrt(discriminant);
    double T = (-A + sqrt_val) / (2 * B);  // 양의 근 선택

    return T;
}

static F32 AD7124_ResToTemp(F32 r)
{
    F32 temp;
    
    temp = (r - 100.0f) / 0.385f;
//  temp = (-A + sqrtf(A*A -4*B*(1-(r/R0)))) / (2*B);
    
    return temp;      
}
