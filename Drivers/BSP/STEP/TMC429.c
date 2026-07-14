#define __TMC429_C__
    #include "TMC429.h"
#undef  __TMC429_C__

#define TMC429_CS_H     HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_SET);
#define TMC429_CS_L     HAL_GPIO_WritePin(GPIOF, GPIO_PIN_6, GPIO_PIN_RESET);

static void GPIO_Init(void);
static void ISPI5_Init(void);

static uIntConverter_t TMC429_RdWrReg(uIntConverter_t WrData);
static U8 TMC429_SetAMax(U8 ch, U32 AMax);
static U8 TMC429_SetVel(U8 ch, S32 Vel);

static SPI_HandleTypeDef        hSpi5;

const U8 DriverChainTable[64] = 
{
    0x10, 0x10, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08,
    0x10, 0x10, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
    0x10, 0x30,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
    0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11
};

void TMC429_Init(void)
{
    U8 ch;
    
    uIntConverter_t WrTemp;
        
    GPIO_Init();
    ISPI5_Init();

    WrTemp._uint    = 0;
    WrTemp._byte[3] = JDX_SMGP;
    WrTemp._byte[2] = 0x01;
    WrTemp._byte[1] = 0x00;
    WrTemp._byte[0] = 0x02;
    TMC429_RdWrReg(WrTemp);
    
    WrTemp._uint     = 0;
    WrTemp._byte[3]  = JDX_IF_CONFIG_429;
    WrTemp._uint    |= IFCONF_EN_SD | IFCONF_INV_DIR;  // IFCONF_INV_DIR | IFCONF_EN_SD | IFCONF_EN_REFR | IFCONF_INV_REF | IFCONF_SDO_INT;
    TMC429_RdWrReg(WrTemp);

    for(ch=0; ch<MOITON_CH_SET; ch++)
    {
        WrTemp._uint    = 0;
        WrTemp._byte[3] = IDX_PULSEDIV_RAMPDIV | (ch<<5);
        WrTemp._byte[2] = 0x00;
        WrTemp._byte[1] = PULSE_DIV<<4 | RAMP_DIV;         //0x16
        WrTemp._byte[0] = 0x04;
        TMC429_RdWrReg(WrTemp);
    }
    
    for(ch=0; ch<MOITON_CH_SET; ch++)
    {
        WrTemp._uint     = 0;
        WrTemp._byte[3]  = IDX_VMIN | (ch<<5);
        WrTemp._uint    |= 1;
        TMC429_RdWrReg(WrTemp);
    }
    
    for(ch=0; ch<MOITON_CH_SET; ch++)
    {
        WrTemp._uint     = 0;
        WrTemp._byte[3]  = IDX_VMAX | (ch<<5);
        WrTemp._uint    |= 1000;
        TMC429_RdWrReg(WrTemp);
    }
    
    for(ch=0; ch<MOITON_CH_SET; ch++)
    {
        TMC429_SetAMax(ch, 50);
    }
        
    TMC429_SetDir(0);

    TMC429_SetRefSwitch(MOTION_CH0, (U8)DISABLE);
    TMC429_SetRefSwitch(MOTION_CH1, (U8)DISABLE);
    TMC429_SetRefSwitch(MOTION_CH2, (U8)DISABLE);
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    __HAL_RCC_GPIOF_CLK_ENABLE();
    
    GPIO_InitStruct.Pin       = GPIO_PIN_7  | GPIO_PIN_8  | GPIO_PIN_9;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI5;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin   = GPIO_PIN_6;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
    
    TMC429_CS_H;
}

static void ISPI5_Init(void)
{
    __HAL_RCC_SPI5_CLK_ENABLE();  
  
    hSpi5.Instance               = SPI5;
    hSpi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;    // 108MHz/64 = 1.6875MHz
    hSpi5.Init.Direction         = SPI_DIRECTION_2LINES;
    hSpi5.Init.CLKPhase          = SPI_PHASE_2EDGE;
    hSpi5.Init.CLKPolarity       = SPI_POLARITY_HIGH;
    hSpi5.Init.DataSize          = SPI_DATASIZE_8BIT;
    hSpi5.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hSpi5.Init.TIMode            = SPI_TIMODE_DISABLE;
    hSpi5.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hSpi5.Init.CRCPolynomial     = 7;
    hSpi5.Init.NSS               = SPI_NSS_SOFT;
    hSpi5.Init.Mode              = SPI_MODE_MASTER;
    HAL_SPI_Init(&hSpi5);
}

static uIntConverter_t TMC429_RdWrReg(uIntConverter_t WrData)
{       
    static U8 tbuf[4], rbuf[4];
    static uIntConverter_t RdData;
    
    tbuf[0] = WrData._byte[3];
    tbuf[1] = WrData._byte[2];
    tbuf[2] = WrData._byte[1];
    tbuf[3] = WrData._byte[0];
      
    TMC429_CS_L;

    HAL_SPI_TransmitReceive(&hSpi5, tbuf, rbuf, 4, 5000); 
    
    TMC429_CS_H;
    
    RdData._byte[3] = rbuf[0];
    RdData._byte[2] = rbuf[1];
    RdData._byte[1] = rbuf[2];
    RdData._byte[0] = rbuf[3];
      
    return RdData;
}

static U8 TMC429_SetAMax(U8 ch, U32 AMax)
{
    uIntConverter_t RdTemp, WrTemp;
    
    F32 p, p_reduced;
    S16 pdiv, pmul, pm, pd;    
    U8 ramp_div;
    U8 pulse_div;
    U8 PulseRampDiv;

    AMax &= 0x000007FF;  //Begrenzen auf den Maximalwert des TMC428 (2047)
    
    RdTemp._uint    = 0;
    WrTemp._uint    = 0;
    WrTemp._byte[3] = IDX_PULSEDIV_RAMPDIV | TMC429_READ | (ch<<5);
    RdTemp          = TMC429_RdWrReg(WrTemp);

    PulseRampDiv = RdTemp._byte[1];
    pulse_div    = PulseRampDiv>>4;
    ramp_div     = PulseRampDiv & 0x0F;

    pm = -1; pd = -1;  // -1 indicates : no valid pair found

    if(ramp_div >= pulse_div)
    {
        p = AMax / ( 128.0 * (1<<(ramp_div-pulse_div)));
    }
    else
    {
        p = AMax / ( 128.0 / (1<<(pulse_div-ramp_div)));
    }

    p_reduced = p*0.988;

#if 1
    for(pdiv=0; pdiv<=13; pdiv++)
    {
        pmul = (S16)(p_reduced * 8.0 * (1<<pdiv)) - 128;

        if((0 <= pmul) && (pmul <= 127))
        {
            pm = pmul + 128;
            pd = pdiv;
        }
    }
#else
    F32 q, p_calc;
    for (pdiv = 0; pdiv <= 13; pdiv++) {
        for (pmul = 128; pmul <= 255; pmul++) {
            p_calc = (float)pmul / (1 << (3 + pdiv));
            q = p_calc / p;   // 실제/목표 비율
            if ((q > 0.95) && (q < 1.0)) {
            	pm = pmul;
            	pd = pdiv;
                break;
            }
        }
    }
#endif
    WrTemp._uint    = 0;
    WrTemp._byte[3] = IDX_PMUL_PDIV | (ch<<5);
    WrTemp._byte[2] = 0;
    WrTemp._byte[1] = (U8)pm;
    WrTemp._byte[0] = (U8)pd;
    TMC429_RdWrReg(WrTemp);
    
    WrTemp._uint     = 0;
    WrTemp._byte[3]  = IDX_AMAX | (ch<<5);
    WrTemp._uint    |= AMax;
    TMC429_RdWrReg(WrTemp);

    return 0;    
}

static U8 TMC429_SetVel(U8 ch, S32 Vel)
{
    uIntConverter_t WrTemp;

    WrTemp._uint     = 0;
    WrTemp._byte[3]  = IDX_VMAX | (ch<<5);
    WrTemp._uint    |= Vel;
    TMC429_RdWrReg(WrTemp);
    
    return 0;
}

static void TMC429_SetRampMode(U8 ch, U8 mode)
{
    uIntConverter_t RdTemp, WrTemp;

    RdTemp._uint = 0;
    
    WrTemp._uint    = 0;
    WrTemp._byte[3] = IDX_REFCONF_RM | TMC429_READ | (ch<<5);
    RdTemp          = TMC429_RdWrReg(WrTemp);
    
    WrTemp._uint    = 0;
    WrTemp._byte[3] = IDX_REFCONF_RM | (ch<<5);
    WrTemp._byte[2] = RdTemp._byte[2];
    WrTemp._byte[1] = RdTemp._byte[1];
    WrTemp._byte[0] = mode;
    TMC429_RdWrReg(WrTemp);
}

void TMC429_SetDir(U8 dir)
{
	uIntConverter_t WrTemp;

    WrTemp._uint     = 0;
    WrTemp._byte[3]  = JDX_IF_CONFIG_429;
    if(dir) WrTemp._uint    |= IFCONF_EN_SD | IFCONF_INV_REF | IFCONF_INV_DIR;
    else 	WrTemp._uint    |= IFCONF_EN_SD | IFCONF_INV_REF;  // IFCONF_INV_DIR | IFCONF_EN_SD | IFCONF_EN_REFR | IFCONF_INV_REF | IFCONF_SDO_INT;
    TMC429_RdWrReg(WrTemp);
}

void TMC429_SetRefSwitch(U8 ch, U8 en)
{
    uIntConverter_t RdTemp, WrTemp;

    RdTemp._uint = 0;
    
    WrTemp._uint    = 0;
    WrTemp._byte[3] = IDX_REFCONF_RM | TMC429_READ | (ch<<5);
    RdTemp          = TMC429_RdWrReg(WrTemp);
    
    WrTemp._uint    = 0;
    WrTemp._byte[3] = IDX_REFCONF_RM | (ch<<5);
    WrTemp._byte[2] = RdTemp._byte[2];
    if(en) WrTemp._byte[1] = REF_LEFT_SWITCH;
    else   WrTemp._byte[1] = NO_REF;
    WrTemp._byte[0] = RdTemp._byte[0];
    TMC429_RdWrReg(WrTemp);
}

U8 TMC429_GetStatus(void)
{
    uIntConverter_t RdTemp, WrTemp;

    WrTemp._uint    = 0;
    WrTemp._byte[3] = JDX_REF_SWITCHES | TMC429_READ;
    RdTemp          = TMC429_RdWrReg(WrTemp);

    return RdTemp._byte[3];
}

S32 TMC429_GetPosition(U8 ch)
{
    uIntConverter_t RdTemp, WrTemp;
    
    WrTemp._uint    = 0;
    WrTemp._byte[3] = IDX_XACTUAL | TMC429_READ | (ch<<5);
    RdTemp          = TMC429_RdWrReg(WrTemp);
     
    if(RdTemp._byte[2] & 0x80) { RdTemp._byte[3] = 0xFF; }
    else                       { RdTemp._byte[3] = 0x00; }        
    
    return RdTemp._int;
}

S32 TMC429_GetVelocity(U8 ch)
{
    uIntConverter_t RdTemp, WrTemp;
    
    WrTemp._uint    = 0;
    WrTemp._byte[3] = IDX_VACTUAL | TMC429_READ | (ch<<5);
    RdTemp          = TMC429_RdWrReg(WrTemp);

    RdTemp._int = RdTemp._int & 0x00000FFF;
    if(RdTemp._int & 0x00000800) { RdTemp._int |= 0xFFFFF000; }
   
    RdTemp._int = RdTemp._int * (S32)STEP_PULSE_RATE_R;

    return RdTemp._int;
}

void TMC429_MotorStop(U8 ch, U32 Acc)
{
    uIntConverter_t WrTemp;
    
    TMC429_SetRampMode(ch, RM_VELOCITY);
    
    TMC429_SetAMax(ch, Acc);

    WrTemp._uint    = 0;
    WrTemp._byte[3] = IDX_VTARGET | (ch<<5);
    TMC429_RdWrReg(WrTemp);
}

void TMC429_SetPosition(U8 ch, S32 Pos)
{
    uIntConverter_t WrTemp;

    WrTemp._uint     = 0;
    WrTemp._byte[3]  = IDX_XACTUAL | (ch<<5);
    WrTemp._uint    |= (Pos & 0xFFFFFF);
    TMC429_RdWrReg(WrTemp);
}

U8 TMC429_VelMove(U8 ch, U32 Acc, S32 Vel)
{
    uIntConverter_t WrTemp;
    U8 dir;

    TMC429_SetRampMode(ch, RM_VELOCITY);
    
    TMC429_SetAMax(ch, Acc);

    WrTemp._uint     = 0;
    WrTemp._byte[3]  = IDX_VMAX | (ch<<5);
    WrTemp._uint    |= 2047;
    TMC429_RdWrReg(WrTemp);

    WrTemp._uint     = 0;
    WrTemp._byte[3]  = IDX_VTARGET | (ch<<5);
    WrTemp._uint    |= (Vel & 0xFFF);
    TMC429_RdWrReg(WrTemp);
    
    if(Vel > 0) { dir = 0; }
    else        { dir = 1; }
    
    return dir;
}

U8 TMC429_RelMove(U8 ch, U32 Acc, S32 Vel, S32 Pos)
{
    uIntConverter_t RdTemp, WrTemp;
    S32 TargetPos;
    U8 dir;

    TMC429_SetVel(ch, Vel);
    TMC429_SetAMax(ch, Acc);
        
    WrTemp._uint    = 0;
    WrTemp._byte[3] = IDX_XACTUAL | TMC429_READ | (ch<<5);
    RdTemp          = TMC429_RdWrReg(WrTemp);
    
    RdTemp._byte[3] = 0;
    TargetPos       = RdTemp._int + Pos;
    
    WrTemp._uint     = 0;
    WrTemp._byte[3]  = IDX_XTARGET | (ch<<5);
    WrTemp._uint    |= (TargetPos & 0xFFFFFF);
    TMC429_RdWrReg(WrTemp);
    
    TMC429_SetRampMode(ch, RM_RAMP);
    
    if(Pos > 0) { dir = 0; }
    else        { dir = 1; } 
    
    return dir;
}

U8 TMC429_AbsMove(U8 ch, U32 Acc, S32 Vel, S32 Pos)
{
    uIntConverter_t RdTemp, WrTemp;
    U8 dir;

    TMC429_SetVel(ch, Vel);
    TMC429_SetAMax(ch, Acc);
    
    WrTemp._uint    = 0;
    WrTemp._byte[3] = IDX_XACTUAL | TMC429_READ | (ch<<5);
    RdTemp          = TMC429_RdWrReg(WrTemp);
    RdTemp._byte[3] = 0;
    
    WrTemp._uint     = 0;
    WrTemp._byte[3]  = IDX_XTARGET | (ch<<5);
    WrTemp._uint    |= (Pos & 0xFFFFFF);
    TMC429_RdWrReg(WrTemp);
    
    TMC429_SetRampMode(ch, RM_RAMP);
    
    if(Pos > RdTemp._int) { dir = 0; }
    else                  { dir = 1; }
    
    return dir;
}
