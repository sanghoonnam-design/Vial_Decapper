#define __TMC2660_C__
    #include "TMC2660.h"
#undef  __TMC2660_C__

#include "LED.h"

#define EN_TMC2660_1    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_RESET);
#define DS_TMC2660_1    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_SET);
#define EN_TMC2660_2    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET);
#define DS_TMC2660_2    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET);

#define TMC1_CS_H       HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
#define TMC1_CS_L       HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);
#define TMC2_CS_H       HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_SET);
#define TMC2_CS_L       HAL_GPIO_WritePin(GPIOE, GPIO_PIN_4, GPIO_PIN_RESET);

static void GPIO_Init(void);
static void ISPI4_Init(void);

static U32 TMC2660_RdWrReg(U8 ch, U32 WrData);

static SPI_HandleTypeDef        hSpi4;

typedef enum
{
    RB_CHOPPER, 
    RB_DRIVER, 
    RB_SMART_ENERGY, 
    RB_STALL_GUARD, 
    RB_STEP_DIR
}eReadBackSeq_t;

typedef union
{
    struct
    {
        U32 MRes     : 4;
        U32 reserved0: 4;
        U32 DEdge    : 1;
        U32 Intpol   : 1;
        U32 reserved1: 8;
        U32 RegAddr  : 2;
        U32 reserved : 12;
    }bit;
  
    U32 dWord;    
}uStepDirConfig_t;

typedef union
{
    struct
    {
        U32 TOff           : 4;
        U32 HysteresisStart: 3;
        U32 HysteresisEnd  : 4;
        U32 HysteresisDecay: 2;
        U32 RandomTOff     : 1;
        U32 ChopperMode    : 1;
        U32 BlankTime      : 2;
        U32 RegAddr        : 3;
        U32 reserved       : 12;
    }bit;
  
    U32 dWord;    
}uChopperConfig_t;

typedef union
{
    struct
    {
        U32 SmartStallLevelMin: 4;
        U32 reserved0         : 1;
        U32 SmartUpStep       : 2;
        U32 reserved1         : 1;
        U32 SmartStallLevelMax: 4;
        U32 reserved2         : 1;
        U32 SmartDownStep     : 2;
        U32 SmartIMin         : 1;
        U32 reserved3         : 1;
        U32 RegAddr           : 3;
        U32 reserved          : 12;
    }bit;
  
    U32 dWord;    
}uSmartEnergyControl_t;

typedef union
{
    struct
    {
        U32 ShortFailEnable     : 1;
        U32 FastDecayEnable     : 1;
        U32 GndShortDetection   : 1;
        U32 OverTempSens        : 1;
        U32 ReadBackSelect      : 2;
        U32 VSenseScale         : 1;
        U32 StepDirectionDisable: 1;
        U32 ProtectionTimer     : 2;
        U32 ProtectionDisable   : 1;
        U32 reserved0           : 1;
        U32 SlopeLowSide        : 2;
        U32 SlopeHighSide       : 2;
        U32 TestMode            : 1;
        U32 RegAddr             : 3;
        U32 reserved            : 12;
    }bit;
  
    U32 dWord;    
}uDriverConfig_t;

typedef union
{
    struct
    {
        U32 CurrentScale       : 5;
        U32 reserved0          : 3;
        U32 StallGuardThreshold: 7;
        U32 reserved1          : 1;
        U32 FilterEnable       : 1;
        U32 RegAddr            : 3;
        U32 reserved           : 12;
    }bit;

    U32 dWord;
}uStallGuardConfig_t;


//static void TMC2660_ControlCurrent(U8 ch, uReadResponse_t rr);
//static void TMC2660_Status(uReadResponse_t rr0, uReadResponse_t rr1);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static eReadBackSeq_t           ReadBackSeq;
#pragma GCC diagnostic pop

static uStepDirConfig_t         StepDirConfig[STEP_CH_MAX];
static uChopperConfig_t         ChopperConfig[STEP_CH_MAX];        
static uSmartEnergyControl_t    SmartEnergyControl[STEP_CH_MAX];
static uDriverConfig_t          DriverConfig[STEP_CH_MAX];
static uStallGuardConfig_t      StallGuardConfig[STEP_CH_MAX];
static uReadResponse_t          ReadResponse[STEP_CH_MAX];
        
static sMotorCurrent_t          CurrentScale[STEP_CH_MAX];

void TMC2660_Init(void)
{
    U8 ch;
    
    GPIO_Init();
    ISPI4_Init();
    
    DS_TMC2660_1;
    DS_TMC2660_2;

    for(ch=0; ch<STEP_CH_MAX; ch++)
    {
        StallGuardConfig  [ch].dWord = 0;
        DriverConfig      [ch].dWord = 0;
        SmartEnergyControl[ch].dWord = 0;
        StepDirConfig     [ch].dWord = 0;
        ChopperConfig     [ch].dWord = 0;

        DriverConfig[ch].bit.RegAddr              = 7;
        DriverConfig[ch].bit.SlopeHighSide        = 3;
        DriverConfig[ch].bit.SlopeLowSide         = 3;
        DriverConfig[ch].bit.ProtectionDisable    = 0;
        DriverConfig[ch].bit.ProtectionTimer      = 0;
        DriverConfig[ch].bit.StepDirectionDisable = 0;
        DriverConfig[ch].bit.VSenseScale          = 0;  // 0:2.2Arms(0.06875)-3.11Apeak(0.0972125) / 1:1.2Arms(0.0375)-1.6968(0.053025)
        DriverConfig[ch].bit.ReadBackSelect       = 0;
        DriverConfig[ch].bit.FastDecayEnable      = 1;

        StallGuardConfig[ch].bit.RegAddr             = 6;
        StallGuardConfig[ch].bit.FilterEnable        = 1;
        StallGuardConfig[ch].bit.StallGuardThreshold = 2;
        StallGuardConfig[ch].bit.CurrentScale        = 0;

        SmartEnergyControl[ch].bit.RegAddr            = 5;
        SmartEnergyControl[ch].bit.SmartIMin          = 0;
        SmartEnergyControl[ch].bit.SmartDownStep      = 0;
        SmartEnergyControl[ch].bit.SmartStallLevelMax = 0;
        SmartEnergyControl[ch].bit.SmartUpStep        = 0;
        SmartEnergyControl[ch].bit.SmartStallLevelMin = 0;

        ChopperConfig[ch].bit.RegAddr         = 4;
        ChopperConfig[ch].bit.BlankTime       = 2;
        ChopperConfig[ch].bit.ChopperMode     = 0;
        ChopperConfig[ch].bit.RandomTOff      = 0;
        ChopperConfig[ch].bit.HysteresisDecay = 0;
        ChopperConfig[ch].bit.HysteresisEnd   = 4;
        ChopperConfig[ch].bit.HysteresisStart = 6;
        ChopperConfig[ch].bit.TOff            = 4;

        StepDirConfig[ch].bit.RegAddr = 0;
        StepDirConfig[ch].bit.Intpol  = 1;
        StepDirConfig[ch].bit.DEdge   = 0;
        StepDirConfig[ch].bit.MRes    = 3;
    }
    
    for(ch=0; ch<STEP_CH_MAX; ch++)
    {
    	TMC2660_RdWrReg(ch,DriverConfig[ch].dWord);
    	TMC2660_RdWrReg(ch,ChopperConfig[ch].dWord);
        TMC2660_RdWrReg(ch,SmartEnergyControl[ch].dWord);
        TMC2660_RdWrReg(ch,StallGuardConfig[ch].dWord);
        TMC2660_RdWrReg(ch,StepDirConfig[ch].dWord);
    }
    
    //ReadBackSeq = RB_CHOPPER;
}

static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOE_CLK_ENABLE();
    
    GPIO_InitStruct.Pin       = GPIO_PIN_2 | GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_MEDIUM;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI4;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    
    GPIO_InitStruct.Pin   = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_4;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    
    TMC1_CS_H;
    TMC2_CS_H;
}

static void ISPI4_Init(void)
{
    __HAL_RCC_SPI4_CLK_ENABLE();  
  
    hSpi4.Instance               = SPI4;
    hSpi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;    // 108MHz/64 = 1.6875MHz
    hSpi4.Init.Direction         = SPI_DIRECTION_2LINES;
    hSpi4.Init.CLKPhase          = SPI_PHASE_2EDGE;
    hSpi4.Init.CLKPolarity       = SPI_POLARITY_HIGH;
    hSpi4.Init.DataSize          = SPI_DATASIZE_8BIT;
    hSpi4.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    hSpi4.Init.TIMode            = SPI_TIMODE_DISABLE;
    hSpi4.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLE;
    hSpi4.Init.CRCPolynomial     = 7;
    hSpi4.Init.NSS               = SPI_NSS_SOFT;
    hSpi4.Init.Mode              = SPI_MODE_MASTER;
    HAL_SPI_Init(&hSpi4);
}

static U32 TMC2660_RdWrReg(U8 ch, U32 WrData)
{    
    static U8 tbuf[3];
    static U8 rbuf[3] = {0};
    static U32 RdData;
    
    if(ch == 0) { TMC1_CS_L; }
    else        { TMC2_CS_L; }
    
    tbuf[0] = (WrData>>16)&0xFF;
    tbuf[1] = (WrData>>8)&0xFF;
    tbuf[2] = WrData&0xFF;

    HAL_SPI_TransmitReceive(&hSpi4, tbuf, rbuf, 3, 5000);
    
    RdData = (U32)rbuf[0]<<12 | (U32)rbuf[1]<<4 | (U32)rbuf[2]>>4;
    
    if(ch == 0) { TMC1_CS_H; }
    else        { TMC2_CS_H; }
    
    return RdData;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

uReadResponse_t TMC2660_ReadResponse(U8 ch, U8 rdSel)
{
	DriverConfig[ch].bit.ReadBackSelect = rdSel;

	TMC2660_RdWrReg(ch, DriverConfig[ch].dWord);
	ReadResponse[ch].dWord = TMC2660_RdWrReg(ch, DriverConfig[ch].dWord);

	TMC2660_RdWrReg(ch, StallGuardConfig[ch].dWord);

	return ReadResponse[ch];
}
#pragma GCC diagnostic pop

void TMC2660_PowerEnable(U8 ch, U8 OnOff)
{
    if(OnOff == true)
    {
        switch(ch)
        {
        case STEP_CH0: EN_TMC2660_1;   break;
        case STEP_CH1: EN_TMC2660_2;   break;
        }
    }
    else
    {
        switch(ch)
        {
        case STEP_CH0: DS_TMC2660_1;   break;
        case STEP_CH1: DS_TMC2660_2;   break;
        }
    }
}

void TMC2660_SetMaxCurrent(U8 ch, U8 type)
{
    if(type > CUR_MAX_17A) { type = CUR_MAX_31A; }
    
    DriverConfig[ch].bit.VSenseScale = type;
    
    TMC2660_RdWrReg(ch,DriverConfig[ch].dWord);
}

F32 TMC2660_GetMaxCurrent(U8 ch)
{
    F32 cur;
    
    if(DriverConfig[ch].bit.VSenseScale == CUR_MAX_31A) { cur = 3.1f; }
    else                                                { cur = 1.7f; } 
    
    return cur;
}

void TMC2660_SetCurrentScale(U8 ch, sMotorCurrent_t cur)
{
    if(cur.iHold > 31) { cur.iHold = 31; }
    if(cur.iRun > 31)  { cur.iRun = 31;  }
    
    CurrentScale[ch].iHold = cur.iHold;
    CurrentScale[ch].iRun  = cur.iRun;
    
    TMC2660_SetHoldCurrent(ch);
}

void TMC2660_SetResolution(U8 ch, U8 res)
{
    StepDirConfig[ch].bit.MRes = res;
    TMC2660_RdWrReg(ch,StepDirConfig[ch].dWord);
}

void TMC2660_SetRunCurrent(U8 ch)
{
	if(StallGuardConfig[ch].bit.CurrentScale != CurrentScale[ch].iRun)
	{
		StallGuardConfig[ch].bit.CurrentScale = CurrentScale[ch].iRun;
		TMC2660_RdWrReg(ch,StallGuardConfig[ch].dWord);
	}
}

void TMC2660_SetHoldCurrent(U8 ch)
{
	if(StallGuardConfig[ch].bit.CurrentScale != CurrentScale[ch].iHold)
	{
		StallGuardConfig[ch].bit.CurrentScale = CurrentScale[ch].iHold;
		TMC2660_RdWrReg(ch,StallGuardConfig[ch].dWord);
	}
}

U8 TMC2660_GetMotorRun(U8 ch)
{
	DriverConfig[ch].bit.ReadBackSelect = 0;
	TMC2660_RdWrReg(ch, DriverConfig[ch].dWord);
	ReadResponse[ch].dWord = TMC2660_RdWrReg(ch, DriverConfig[ch].dWord);

	return !ReadResponse[ch].bit.StandStill;
}

void TMC2660_ControlCurrent(U8 ch)
{
	if(TMC2660_GetMotorRun(ch) == 0)
	{
		TMC2660_SetHoldCurrent(ch);
	}
	else
	{
		TMC2660_SetRunCurrent(ch);
	}
}

/*
static void TMC2660_ControlCurrent(U8 ch, uReadResponse_t rr)
{
    if(!rr.bit.StandStill) { StallGuardConfig[ch].bit.CurrentScale = CurrentScale[ch].iRun; }
    else                   { StallGuardConfig[ch].bit.CurrentScale = CurrentScale[ch].iHold; }
}

static void TMC2660_Status(uReadResponse_t rr0, uReadResponse_t rr1)
{        
    static U16 AlmCnt[STEP_CH_MAX];
    static U16 TogCnt[STEP_CH_MAX];
    static U16 Alarm[STEP_CH_MAX] = {0};
    
    if(!rr0.bit.StandStill)
    {
        MotorRun[STEP_CH0] = TRUE;
        // LED_OnOff(STATUS2, true);
    }
    else
    {
        MotorRun[STEP_CH0] = FALSE;
        if(Alarm[STEP_CH0] == false)
        {
            // LED_OnOff(STATUS2, false);
        }
    }
    
    if(!rr1.bit.StandStill)
    {
        MotorRun[STEP_CH1] = TRUE;
        // LED_OnOff(STATUS3, true);
        
    }
    else
    {
        MotorRun[STEP_CH1] = FALSE;
        if(Alarm[STEP_CH1] == false)
        {
            // LED_OnOff(STATUS3, false);
        }
    }
    
    if(((rr0.bit.OverTempShutdown == 1)  ||
        (rr0.bit.OverTempWarning == 1)   ||
        (rr0.bit.ShortDetection == 3)    ||
        (rr0.bit.OpenLoad == 3))         &&
        (!rr0.bit.StandStill)) 
    {
        AlmCnt[STEP_CH0]++;
        if(AlmCnt[STEP_CH0] > 10)
        {
            AlmCnt[STEP_CH0] = 10;
            TogCnt[STEP_CH0]++;
            if(TogCnt[STEP_CH0] > 10)
            {
                TogCnt[STEP_CH0] = 0;
                Alarm[STEP_CH0]  = true;
                // LED_Toggle(STATUS2);
            }
        }
    }
    else
    {
        AlmCnt[STEP_CH0] = 0;
        Alarm[STEP_CH0]  = false;
    }
    
    if(((rr1.bit.OverTempShutdown == 1)  ||
        (rr1.bit.OverTempWarning == 1)   ||
        (rr1.bit.ShortDetection == 3)    ||
        (rr1.bit.OpenLoad == 3))         &&
        (!rr1.bit.StandStill))
    {
        AlmCnt[STEP_CH1]++;
        if(AlmCnt[STEP_CH1] > 10)
        {
            AlmCnt[STEP_CH1] = 10;
            TogCnt[STEP_CH1]++;
            if(TogCnt[STEP_CH1] > 10)
            {
                TogCnt[STEP_CH1] = 0;
                Alarm[STEP_CH1]  = true;
                // LED_Toggle(STATUS3);
            }
        }
    }
    else
    {
        AlmCnt[STEP_CH1] = 0;
        Alarm[STEP_CH1]  = false;
    }   
}

void TMC2660_ControlProcess(void)
{
    U32 WrData[2];
       
    switch(ReadBackSeq)
    {
    case RB_CHOPPER: 
        WrData[STEP_CH0] = ChopperConfig[STEP_CH0].dWord;
        WrData[STEP_CH1] = ChopperConfig[STEP_CH1].dWord;
        ReadBackSeq      = RB_DRIVER;
        break;
    
    case RB_DRIVER: 
        WrData[STEP_CH0] = DriverConfig[STEP_CH0].dWord;
        WrData[STEP_CH1] = DriverConfig[STEP_CH1].dWord;
        ReadBackSeq      = RB_SMART_ENERGY;
        break;
        
    case RB_SMART_ENERGY: 
        WrData[STEP_CH0] = SmartEnergyControl[STEP_CH0].dWord;
        WrData[STEP_CH1] = SmartEnergyControl[STEP_CH1].dWord;
        ReadBackSeq      = RB_STALL_GUARD;//RB_STEP_DIR;//RB_STALL_GUARD;
        break;
    
    case RB_STALL_GUARD: 
        WrData[STEP_CH0] = StallGuardConfig[STEP_CH0].dWord;
        WrData[STEP_CH1] = StallGuardConfig[STEP_CH1].dWord;
        ReadBackSeq      = RB_STEP_DIR;
        break;
        
    case RB_STEP_DIR: 
        WrData[STEP_CH0] = StepDirConfig[STEP_CH0].dWord;
        WrData[STEP_CH1] = StepDirConfig[STEP_CH1].dWord;
        ReadBackSeq      = RB_CHOPPER;
        break;
        
    default: 
        WrData[STEP_CH0] = ChopperConfig[STEP_CH0].dWord;
        WrData[STEP_CH1] = ChopperConfig[STEP_CH1].dWord;
        ReadBackSeq      = RB_CHOPPER;
        break;
    }
    
    ReadResponse[STEP_CH0].dWord = TMC2660_RdWrReg(STEP_CH0, WrData[STEP_CH0]);
    ReadResponse[STEP_CH1].dWord = TMC2660_RdWrReg(STEP_CH1, WrData[STEP_CH1]);
    
    TMC2660_ControlCurrent(STEP_CH0, ReadResponse[STEP_CH0]);
    TMC2660_ControlCurrent(STEP_CH1, ReadResponse[STEP_CH1]);
    
    TMC2660_Status(ReadResponse[STEP_CH0],ReadResponse[STEP_CH1]);
}
*/
