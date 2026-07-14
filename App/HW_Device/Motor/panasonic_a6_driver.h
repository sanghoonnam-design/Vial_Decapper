#ifndef __PANASONIC_A6_DRIVER_H__
#define __PANASONIC_A6_DRIVER_H__

#include "project.h"

#ifdef __PANASONIC_A6_DRIVER_C__
#define PANASONIC_A6_DRIVER_EXT
#else
#define PANASONIC_A6_DRIVER_EXT extern
#endif

#define PANASONIC_A6_PPR 	(10000)
#define PANASONIC_A6_CH_MAX (1)

typedef enum A6_FuncIndex_t
{
    A6_FUNC_INDEX_NONE = 0,
    A6_FUNC_INDEX_INIT,
    A6_FUNC_INDEX_EEPROM_WRITE,
    A6_FUNC_INDEX_ENABLE,
    A6_FUNC_INDEX_DISABLE,
    A6_FUNC_INDEX_CLEAR_ALARM,
    A6_FUNC_INDEX_SET_BLOCK_NUM,
    A6_FUNC_INDEX_HOME,
    A6_FUNC_INDEX_JOG,
    A6_FUNC_INDEX_MOVE_VEL,
    A6_FUNC_INDEX_MOVE_ABS,
    A6_FUNC_INDEX_MOVE_REL,
    A6_FUNC_INDEX_STOP,
    A6_FUNC_INDEX_ESTOP,
    A6_FUNC_INDEX_SET_PROFILE,
    A6_FUNC_INDEX_SET_JOG_PROFILE,
    A6_FUNC_INDEX_SET_HOME_PARAM,
    A6_FUNC_INDEX_UPDATE_POS,
    A6_FUNC_INDEX_UPDATE_VEL,
    A6_FUNC_INDEX_UPDATE_STATUS,
    A6_FUNC_INDEX_MAX
} A6_FuncIndex_t;

// For Testing
typedef enum A6_DriverCtrlCommand_t
{
    A6DRIVER_CTRL_CMD_NONE = 0,
    A6DRIVER_CTRL_CMD_INIT = 1,
    A6DRIVER_CTRL_CMD_EEPROM_WRITE = 2,
    A6DRIVER_CTRL_CMD_ENABLE = 3,
    A6DRIVER_CTRL_CMD_DISABLE = 4,
    A6DRIVER_CTRL_CMD_CLEAR_ALARM = 5,
    A6DRIVER_CTRL_CMD_SET_BLOCK_NUM = 6,
    A6DRIVER_CTRL_CMD_HOME = 7,
    A6DRIVER_CTRL_CMD_JOG = 8,
    A6DRIVER_CTRL_CMD_MOVE_VEL = 9,
    A6DRIVER_CTRL_CMD_MOVE_ABS = 10,
    A6DRIVER_CTRL_CMD_MOVE_REL = 11,
    A6DRIVER_CTRL_CMD_STOP = 12,
    A6DRIVER_CTRL_CMD_ESTOP = 13,
    A6DRIVER_CTRL_CMD_SET_PROFILE = 14,
    A6DRIVER_CTRL_CMD_SET_JOG_PROFILE = 15,
    A6DRIVER_CTRL_CMD_SET_HOME_PARAM = 16,
    A6DRIVER_CTRL_CMD_UPDATE_STATUS = 17,
} A6_DriverCtrlCommand_t;

#pragma pack(push, 1)
typedef union LogicalInput_t
{ // 논리입력상태모니터
    struct
    {
        U16 servoOn : 1;
        U16 alarmClear : 1;
        U16 rsvd0 : 14;
    };
    U16 halfword;
} LogicalInput_t;

typedef union VirtualInput_t
{ // 논리입력신호조작
    struct
    {
        U16 servoOn : 1;
        U16 alarmClear : 1;
        U16 rsvd0 : 14;
    };
    U16 halfword;
} VirtualInput_t;

typedef union LogicalOutput_t
{ // 논리출력상태모니터
    struct
    {
        U16 servoReady : 1; // servoOn 해도 되는 상태를 의미, servoOn이 되었는지 아닌지는 아님
        U16 alarm : 1;
        U16 inPos : 1;
        U16 rsvd0 : 1;
        U16 zeroSpeed : 1;
        U16 rsvd1 : 11;
    };
    U16 halfword;
} LogicalOutput_t;

typedef union BlockControlWord_t
{
    struct
    {
        U16 start : 1; // 스트로브입력
        U16 rsvd0 : 1;
        U16 home : 1;  // 홈입력
        U16 hstop : 1; // 즉시 정지
        U16 sStop : 1; // 감속 정지
        U16 rsvd1 : 11;
    };
    U16 halfword;
} BlockControlWord_t;

typedef union BlockStatus_t
{
    struct
    {
        U16 busy : 1;     // 0: 미실행, 1: 실행중
        U16 homeCplt : 1; // 0: 미완료, 1: 완료
        U16 rsvd0 : 14;
    };
    U16 halfword;
} BlockStatus_t;

typedef union A6_DriverStatus_t
{
    struct
    {
        U16 errorCode;
        LogicalInput_t logicalInput;
        LogicalOutput_t logicalOutput;
        BlockStatus_t blockStatus;
        S32 actualPos;
        S32 actualVel;
    };
    U16 halfword[6];
} A6_DriverStatus_t;

typedef struct A6_DriverParam_t
{
    S32 vel; //rpm
    U32 acc_ms;
    U32 dec_ms;
    S32 jogVel;
    U32 jogAcc_ms;
    U32 jogDec_ms;
} A6_DriverParam_t;

typedef union A6_DriverControl_t
{
    struct
    {
        BlockControlWord_t blockCtrl;
        VirtualInput_t virtualInput;
    };
    U16 halfword[2];
} A6_DriverControl_t;

typedef struct A6_DriverHomeParam_t
{
    U08 dir; // 0:cw, 1:ccw
    S32 offset;
    U16 speedHigh; // 0 ~ 20000, RPM
    U16 speedLow;  // 0 ~ 20000, RPM
    U32 acc_ms;       // 0 ~ 10000, ms
} A6_DriverHomeParam_t;

typedef struct A6_DriverData_t
{
    A6_DriverStatus_t status;
    A6_DriverParam_t param;
    A6_DriverControl_t ctrl;
    A6_DriverHomeParam_t homeParam;
} A6_DriverData_t;

// For Testing
typedef struct A6_DriverCtrl_t
{
    U08 id;
    U08 cmd;
    U16 blockNum;
    U08 dir;
    U32 msec;
    S32 pos;
    A6_DriverParam_t profile;
    U08 homeDir;
    S32 homeOffset;
    U16 homeVelH;
    U16 homeVelL;
    U16 homeAcc_ms;
} A6_DriverCtrl_t;
#pragma pack(pop)

PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_EEPROM_Write(U08 id);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_Enable(U08 id);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_Disable(U08 id);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_ClearAlarm(U08 id);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_SetBlockNum(U08 id, U16 num);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_Home(U08 id, U08 dir);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_Jog(U08 id, U08 dir);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_MoveVel(U08 id, U08 dir, U32 msec); //[rpm], [ms]
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_MoveAbs(U08 id, S32 pos);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_MoveRel(U08 id, S32 pos);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_Stop(U08 id);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_EStop(U08 id);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_SetProfile(U08 id, U16 vel, U32 accTime_ms, U32 decTime_ms);    //[rpm], [ms], [ms]
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_SetJogProfile(U08 id, U16 vel, U32 accTime_ms, U32 decTime_ms); //[rpm], [ms], [ms]
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_SetHomeParam(U08 id, S32 offset, U16 velH, U16 velL, U32 accTime);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_UpdatePos(U08 id);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_UpdateVel(U08 id);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_UpdateStatus(U08 id);

PANASONIC_A6_DRIVER_EXT U16 PanasonicA6_GetErrorCode(U08 id);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_IsServoOn(U08 id);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_IsAlarm(U08 id);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_IsInPos(U08 id);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_IsZeroSpeed(U08 id);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_IsMoving(U08 id);
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_IsHomeCompelte(U08 id);
PANASONIC_A6_DRIVER_EXT S32 PanasonicA6_GetPos(U08 id);
PANASONIC_A6_DRIVER_EXT F32 PanasonicA6_GetVel(U08 id);// 초당 pulse
PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_TestRun(A6_DriverCtrl_t *param);

PANASONIC_A6_DRIVER_EXT U08 PanasonicA6_CheckTransaction(A6_FuncIndex_t idx);
PANASONIC_A6_DRIVER_EXT A6_DriverCtrl_t A6_DriverCtrl;

#endif
