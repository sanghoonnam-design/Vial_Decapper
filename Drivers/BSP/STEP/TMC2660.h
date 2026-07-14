#ifndef   __TMC2660_H__
#define   __TMC2660_H__

#include "project.h"

#ifdef __TMC2660_C__
	#define TMC2660_EXT
#else
	#define TMC2660_EXT extern
#endif

#define STEP_CH_MAX             2
#define STEP_CH0                0
#define STEP_CH1                1

#define CUR_MAX_31A             0
#define CUR_MAX_17A             1

typedef enum{
    R_51200 = 0,
    R_25600,
    R_12800,
    R_6400,
    R_3200,
    R_1600,
    R_800,
    R_400,
    R_200,         
}STEP_RESOLUTION;

#pragma pack(push,1)

typedef struct
{
    S32 Pos;
    S32 Vel;
    S32 Enc;

    struct 
    {          
        U16 NegLimit     : 1;
        U16 PosLimit     : 1;
        U16 bHomeComplete: 1;
        U16 bHomingRun   : 1;
        U16 bMoving      : 1;
        U16 Rsvd         : 11;
    }Bit;
    
}sMotionStatus_t;

typedef struct
{
    U8  iHold;
    U8  iRun;        
}sMotorCurrent_t;

typedef struct
{
    U8 EnableNegLimit  : 1;
    U8 PolarityNegLimit: 1;
    U8 EnablePosLimit  : 1;
    U8 PolarityPosLimit: 1;
    U8 EnableSoftLimit : 1;
    U8 Rsvd            : 3;
}sMotionLimit_t;

typedef struct
{
    S32 SwNegLimit;
    S32 SwPosLimit;        
}sMotionSwLimitPos_t;

#pragma pack(pop)

/*
#pragma pack(push,1)
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

#pragma pack(pop)


TMC2660_EXT eReadBackSeq_t           ReadBackSeq;
TMC2660_EXT uStepDirConfig_t         StepDirConfig[AXIS_MAX];
TMC2660_EXT uChopperConfig_t         ChopperConfig[AXIS_MAX];        
TMC2660_EXT uSmartEnergyControl_t    SmartEnergyControl[AXIS_MAX];
TMC2660_EXT uDriverConfig_t          DriverConfig[AXIS_MAX];
TMC2660_EXT uStallGuardConfig_t      StallGuardConfig[AXIS_MAX];
*/

//TMC2660_EXT U8 MotorRun[STEP_CH_MAX];


typedef union
{
    struct
    {
        U32 StallGuard2     : 1;
        U32 OverTempShutdown: 1;
        U32 OverTempWarning : 1;
        U32 ShortDetection  : 2;
        U32 OpenLoad        : 2; //코일 전류가 모터 전류 설정값의 1/16 이상으로 올라가야 이 플래그가 자동으로 해제됨. 아주 작은 전류에서는 항상 "Open Load"로 잘못 감지될 수 있음., 이 비트는 모터가 천천히 구동 중일 때만 신뢰해야 함.
        U32 StandStill      : 1;
        U32 reserved0       : 2;
        U32 ReadBack        : 10;
        U32 reserved        : 12;
    }bit;

    U32 dWord;
}uReadResponse_t;


TMC2660_EXT void TMC2660_Init(void);

TMC2660_EXT void TMC2660_PowerEnable(U8 ch, U8 OnOff);
TMC2660_EXT void TMC2660_SetMaxCurrent(U8 ch, U8 type);
TMC2660_EXT F32  TMC2660_GetMaxCurrent(U8 ch);
TMC2660_EXT void TMC2660_SetCurrentScale(U8 ch, sMotorCurrent_t cur);
TMC2660_EXT void TMC2660_SetResolution(U8 ch, U8 res);

TMC2660_EXT void TMC2660_SetRunCurrent(U8 ch);
TMC2660_EXT void TMC2660_SetHoldCurrent(U8 ch);
TMC2660_EXT U8 TMC2660_GetMotorRun(U8 ch);

TMC2660_EXT uReadResponse_t TMC2660_ReadResponse(U8 ch, U8 rdSel);
TMC2660_EXT void TMC2660_ControlCurrent(U8 ch); //must use after delay function 10ms
#endif
