#ifndef   __DRIVE_H__
#define   __DRIVE_H__

#include "project.h"
#include "TMC2660.h"

#ifdef __DRIVE_C__
	#define DRIVE_EXT
#else
	#define DRIVE_EXT extern
#endif

typedef enum 
{ 
    MODE_STOP = 0,
    MODE_SET_POS, //1
    MODE_CONTINUOUS, //2
    MODE_VEL, //3
    MODE_REL, //4
    MODE_ABS, //5
    MODE_HOMING, //6
    MODE_NONE //7
}eMOTION_MODE;

#pragma pack(push,1)

typedef struct
{
    U8      Axis;
    U8      Mode;
    U32     Acc;
    S32     Vel;
    S32     Pos;
}sMotionCommand_t;

#pragma pack(pop)

DRIVE_EXT void Drive_Init(void);

DRIVE_EXT void Drive_PowerEnable(U8 ch, U8 OnOff);
DRIVE_EXT void Drive_SelMaxCurrent(U8 ch, U8 type);
DRIVE_EXT void Drive_SetCurrent(U8 ch, F32 rCur, U8 sRate);
DRIVE_EXT void Drive_SetResoultion(U8 ch, U8 res);

DRIVE_EXT void Drive_SetHwLimit(U8 ch, sMotionLimit_t limit);
DRIVE_EXT void Drive_SetSwLimitPos(U8 ch, sMotionSwLimitPos_t SwLimit);

DRIVE_EXT void Drive_GetStatus(U8 ch, sMotionStatus_t* Status);

DRIVE_EXT void Drive_Stop(sMotionCommand_t* pCmd);
DRIVE_EXT void Drive_SetPosition(sMotionCommand_t* pCmd);
DRIVE_EXT void Drive_ContinusVelMove(sMotionCommand_t* pCmd);
DRIVE_EXT void Drive_VelMove(sMotionCommand_t* pCmd);
DRIVE_EXT void Drive_RelMove(sMotionCommand_t* pCmd);
DRIVE_EXT void Drive_AbsMove(sMotionCommand_t* pCmd);
DRIVE_EXT void Drive_Homing(sMotionCommand_t* pCmd);
DRIVE_EXT void Drive_HomeControl(U8 ch);

DRIVE_EXT void Drive_GetIO(void);
DRIVE_EXT void Drive_LimitControl(void);

#define DRIVE_DEBUG
#ifdef DRIVE_DEBUG
DRIVE_EXT sMotionCommand_t     TestMotionCmd[2];
DRIVE_EXT sMotionStatus_t      TestMotionStatus[2];
DRIVE_EXT uReadResponse_t TestTMC2660_Status[2];
DRIVE_EXT void Drive_Test(void);
#endif

#endif
