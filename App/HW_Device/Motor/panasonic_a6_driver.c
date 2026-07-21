#define __PANASONIC_A6_DRIVER_C__
#include "panasonic_a6_driver.h"
#undef __PANASONIC_A6_DRIVER_C__

#include "HW_RS485_Process.h"
#include "XDebug.h"

#define USE_MIRROR_ADDR

// Modbus Register list

#define MIRROR_ADDR_1_ERROR_CODE (0x4418)
#define MIRROR_ADDR_2_LOGICAL_INPUT (0x4419)
#define MIRROR_ADDR_3_LOGICAL_OUTPUT (0x441A)
#define MIRROR_ADDR_4_BLOCK_STATUS_FLAG (0x441B)
// #define MIRROR_ADDR_5_BLOCK_NUMBER (0x441C)
// #define MIRROR_ADDR_6_BLOCK_VEL_0 (0x441D)
// #define MIRROR_ADDR_7_BLOCK_ACC_0 (0x441E)
// #define MIRROR_ADDR_8_BLOCK_DEC_0 (0x441F)
// #define MIRROR_ADDR_9_BLOCK_CMD_0_LOW (0x4420)
// #define MIRROR_ADDR_10_BLOCK_CMD_0_HIGH (0x4421)
// #define MIRROR_ADDR_11_BLOCK_DATA_0_LOW (0x4422)
// #define MIRROR_ADDR_12_BLOCK_DATA_0_HIGH (0x4423)
// #define MIRROR_ADDR_13_BLOCK_CONTROL_WORD (0x4424)
// #define MIRROR_ADDR_14_VIRTUAL_INPUT (0x4425)

#define SAVE_ALL_PARAM (0x1020) // 2BYTE

#define ADDR_ERROR_CODE (0x4001) // 2BYTE

#define ADDR_LOGICAL_INPUT (0x4402)     // 4BYTE
#define ADDR_VIRTUAL_INPUT (0x4406)     // 4BYTE
#define ADDR_LOGICAL_OUTPUT (0x440A)    // 4BYTE
#define ADDR_BLOCK_CTRL_WORD (0x4411)   // 2BYTE
#define ADDR_BLOCK_STATUS_FLAG (0x4413) // 2BYTE
#define ADDR_BLOCK_NUMBER (0x4414)      // 2BYTE

#define ADDR_BLOCK_VEL_BASE (0x4600)
#define ADDR_BLOCK_VEL(num) (ADDR_BLOCK_VEL_BASE + num)

#define ADDR_BLOCK_ACC_BASE (0x4610)
#define ADDR_BLOCK_ACC(num) (ADDR_BLOCK_ACC_BASE + num)

#define ADDR_BLOCK_DEC_BASE (0x4620)
#define ADDR_BLOCK_DEC(num) (ADDR_BLOCK_DEC_BASE + num)

#define ADDR_BLOCK_METHODS (0x4630) // 2BYTE

#define ADDR_BLOCK_HOME_OFFSET (0x4631)       // 4BYTE
#define ADDR_BLOCK_HOMING_SPEED_HIGH (0x4637) // 2BYTE
#define ADDR_BLOCK_HOMING_SPEED_LOW (0x4638)  // 2BYTE
#define ADDR_HOMING_ACC (0x4639)              // 2BYTE

#define ADDR_BLOCK_HOMEINGLESS (0x463A) // 2BYTE
#define ADDR_BLOCK_ACC_UNIT (0x463B)    // 2BYTE
#define ADDR_BLOCK_DEC_UNIT (0x463C)    // 2BYTE

#define ADDR_BLOCK_CMD_BASE (0x4800)
#define ADDR_BLOCK_DATA_BASE (0x4802)

#define ADDR_BLOCK_CMD(num) (ADDR_BLOCK_CMD_BASE + 4 * num)
#define ADDR_BLOCK_DATA(num) (ADDR_BLOCK_DATA_BASE + 4 * num)

#define ADDR_POS_ACTUAL_VALUE (0x600F) // 4BYTE
#define ADDR_VEL_ACTUAL_VALUE (0x601C) // 4BYTE

// Block Command Code
#define BLOCK_COMMAND_CODE_REL_MOVE (0x1)
#define BLOCK_COMMAND_CODE_ABS_MOVE (0x2)
#define BLOCK_COMMAND_CODE_JOG (0x3)
#define BLOCK_COMMAND_CODE_HOME (0x4)
#define BLOCK_COMMAND_CODE_UPDATE_VEL (0x6)
#define BLOCK_COMMAND_CODE_COND_EQUAL (0xA)

#define BLOCK_NUM_HOME_CW (0)
#define BLOCK_NUM_HOME_CCW (1)
#define BLOCK_NUM_JOG_CW (2)
#define BLOCK_NUM_JOG_CCW (3)
#define BLOCK_NUM_MOVE (4)

#pragma pack(push, 1)
typedef union BlockCommand_t
{
    struct
    {
        U32 rsvd0 : 8;
        U32 arg5 : 2;
        U32 arg4 : 2;
        U32 arg3 : 4;
        U32 arg2 : 4;
        U32 arg1 : 4;
        U32 cmdCode : 8;
    };
    U32 word;
} BlockCommand_t;

typedef union Block_t
{
    struct
    {
        BlockCommand_t cmd;
        S32 data;
    };
    U16 word[4];
} Block_t;
#pragma pack(pop)

static U08 _modbusWriteMultipleRegisters(U08 id, U16 addr, U16 quantity, U16 *dataIn, U08 *flag);
static U08 _modbusReadHoldingRegisters(U08 id, U16 addr, U16 quantity, U16 *dataOut, U08 *flag);
static U08 *_getFlagAddress(A6_FuncIndex_t idx);

static void _SetUpdateVelocity(Block_t *p, U8 velNum, U8 end);
static void _SetBlockCondtionBranch(Block_t *p, S32 vel, U8 nextBlockNum, U8 end);

static A6_DriverData_t _A6_DriverData[PANASONIC_A6_CH_MAX];
static U08 _A6_MsgProcessed[2][A6_FUNC_INDEX_MAX] = {
    {
        0, // A6_FUNC_INDEX_NONE
        0, // A6_FUNC_INDEX_INIT
        0, // A6_FUNC_INDEX_EEPROM_WRITE
        0, // A6_FUNC_INDEX_ENABLE
        0, // A6_FUNC_INDEX_DISABLE
        0, // A6_FUNC_INDEX_CLEAR_ALARM
        0, // A6_FUNC_INDEX_SET_BLOCK_NUM
        0, // A6_FUNC_INDEX_HOME
        0, // A6_FUNC_INDEX_JOG
        0, // A6_FUNC_INDEX_MOVE_VEL
        0, // A6_FUNC_INDEX_MOVE_ABS
        0, // A6_FUNC_INDEX_MOVE_REL
        0, // A6_FUNC_INDEX_STOP
        0, // A6_FUNC_INDEX_ESTOP
        0, // A6_FUNC_INDEX_SET_PROFILE
        0, // A6_FUNC_INDEX_SET_JOG_PROFILE
        0, // A6_FUNC_INDEX_SET_HOME_PARAM
        0, // A6_FUNC_INDEX_UPDATE_POS
        0, // A6_FUNC_INDEX_UPDATE_VEL
        0, // A6_FUNC_INDEX_UPDATE_STATUS
    },
    {
        0, // A6_FUNC_INDEX_NONE
        9, // A6_FUNC_INDEX_INIT
        1, // A6_FUNC_INDEX_EEPROM_WRITE
        1, // A6_FUNC_INDEX_ENABLE
        1, // A6_FUNC_INDEX_DISABLE
        2, // A6_FUNC_INDEX_CLEAR_ALARM
        1, // A6_FUNC_INDEX_SET_BLOCK_NUM
        3, // A6_FUNC_INDEX_HOME
        3, // A6_FUNC_INDEX_JOG
        4, // A6_FUNC_INDEX_MOVE_VEL
        4, // A6_FUNC_INDEX_MOVE_ABS
        4, // A6_FUNC_INDEX_MOVE_REL
        2, // A6_FUNC_INDEX_STOP
        2, // A6_FUNC_INDEX_ESTOP
        5, // A6_FUNC_INDEX_SET_PROFILE
        5, // A6_FUNC_INDEX_SET_JOG_PROFILE
        4, // A6_FUNC_INDEX_SET_HOME_PARAM
        1, // A6_FUNC_INDEX_UPDATE_POS
        1, // A6_FUNC_INDEX_UPDATE_VEL
        1, // A6_FUNC_INDEX_UPDATE_STATUS
    }};

enum
{
    ACC_DEC_UNIT_0_1_MS = 0,
    ACC_DEC_UNIT_0_5_MS,
    ACC_DEC_UNIT_1_0_MS,
    ACC_DEC_UNIT_10_0_MS,
    ACC_DEC_UNIT_100_0_MS,
};

const U16 _AccDecUnit[5] = {
    1,
    5,
    10,
    100,
    1000};

U08 PanasonicA6_Init(U08 id)
{
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_INIT);
    Block_t block;
    U16 data;

    data = 2;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_METHODS, 1, (U16 *)&data, flag);

    data = 0;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_HOMEINGLESS, 1, (U16 *)&data, flag);

    // Set Block Command, Data
    // 0: home
    block.cmd.word = 0;
    block.cmd.cmdCode = BLOCK_COMMAND_CODE_HOME;
    block.cmd.arg1 = 1;
    block.cmd.arg4 = 0; // cw
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CMD(0), 2, (U16 *)block.word, flag);

    // 1: home
    block.cmd.word = 0;
    block.cmd.cmdCode = BLOCK_COMMAND_CODE_HOME;
    block.cmd.arg1 = 1;
    block.cmd.arg4 = 1; // ccw
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CMD(1), 2, (U16 *)block.word, flag);

    // 2: jog+
    block.cmd.word = 0;
    block.cmd.cmdCode = BLOCK_COMMAND_CODE_JOG;
    block.cmd.arg1 = 0;
    block.cmd.arg2 = 0;
    block.cmd.arg3 = 0;
    block.cmd.arg4 = 0; // cw
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CMD(2), 2, (U16 *)block.word, flag);

    // 3: jog-
    block.cmd.word = 0;
    block.cmd.cmdCode = BLOCK_COMMAND_CODE_JOG;
    block.cmd.arg1 = 0;
    block.cmd.arg2 = 0;
    block.cmd.arg3 = 0;
    block.cmd.arg4 = 1; // ccw
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CMD(3), 2, (U16 *)block.word, flag);

    // 4:abs, rel
    block.cmd.word = 0;
    block.cmd.cmdCode = BLOCK_COMMAND_CODE_REL_MOVE;
    block.cmd.arg1 = 1;
    block.cmd.arg2 = 1;
    block.cmd.arg3 = 1;
    block.cmd.arg5 = 0;
    block.data = 0;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CMD(4), 4, (U16 *)block.word, flag);

    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_ACC_UNIT, 1, (U16 *)&_AccDecUnit[ACC_DEC_UNIT_100_0_MS], flag);
    ret = _modbusWriteMultipleRegisters(id, ADDR_BLOCK_DEC_UNIT, 1, (U16 *)&_AccDecUnit[ACC_DEC_UNIT_100_0_MS], flag);

    return ret;
}

static void __attribute__((unused)) _SetBlockCondtionBranch(Block_t *p, S32 vel, U8 nextBlockNum, U8 end)
{
    p->cmd.word = 0;
    p->cmd.cmdCode = BLOCK_COMMAND_CODE_COND_EQUAL;
    p->cmd.arg1 = 4; // COMPARE MOTOR RPM
    p->cmd.arg2 = (nextBlockNum >> 6) & 0xF;
    p->cmd.arg3 = (nextBlockNum >> 2) & 0xF;
    p->cmd.arg4 = nextBlockNum & 0x3;
    if (end)
        p->cmd.arg5 = 1;
    else
        p->cmd.arg5 = 3;
    p->data = vel;
}

static void __attribute__((unused)) _SetUpdateVelocity(Block_t *p, U8 velNum, U8 end)
{
    p->cmd.word = 0;
    p->cmd.cmdCode = BLOCK_COMMAND_CODE_UPDATE_VEL;
    p->cmd.arg1 = velNum;
    p->cmd.arg2 = 0;
    p->cmd.arg3 = 0;
    p->cmd.arg4 = 0;
    if (end)
        p->cmd.arg5 = 0;
    else
        p->cmd.arg5 = 2;
}

U08 PanasonicA6_EEPROM_Write(U08 id)
{
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_EEPROM_WRITE);
    U32 data = 0x6173;

    ret = _modbusWriteMultipleRegisters(id, SAVE_ALL_PARAM, 2, (U16 *)&data, flag);
    return ret;
}

U08 PanasonicA6_Enable(U08 id)
{
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_ENABLE);

    _A6_DriverData[id - 1].ctrl.virtualInput.servoOn = 1;
    ret = _modbusWriteMultipleRegisters(id, ADDR_VIRTUAL_INPUT, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.virtualInput.halfword, flag);
    return ret;
}

U08 PanasonicA6_Disable(U08 id)
{
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_DISABLE);

    _A6_DriverData[id - 1].ctrl.virtualInput.servoOn = 0;
    ret = _modbusWriteMultipleRegisters(id, ADDR_VIRTUAL_INPUT, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.virtualInput.halfword, flag);
    return ret;
}

U08 PanasonicA6_ClearAlarm(U08 id)
{ // panaterm software reg 5.16 -> 1
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_CLEAR_ALARM);

    _A6_DriverData[id - 1].ctrl.virtualInput.alarmClear = 1;
    _modbusWriteMultipleRegisters(id, ADDR_VIRTUAL_INPUT, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.virtualInput.halfword, flag);
    _A6_DriverData[id - 1].ctrl.virtualInput.alarmClear = 0;
    ret = _modbusWriteMultipleRegisters(id, ADDR_VIRTUAL_INPUT, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.virtualInput.halfword, flag);

    return ret;
}

U08 PanasonicA6_SetBlockNum(U08 id, U16 num)
{
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_SET_BLOCK_NUM);
    U16 blockNum = 0;

    ret = _modbusWriteMultipleRegisters(id, ADDR_BLOCK_NUMBER, 1, (U16 *)&blockNum, flag);

    return ret;
}

U08 PanasonicA6_Home(U08 id, U08 dir)
{
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_HOME);
    U16 blockNum = 0;

    _A6_DriverData[id - 1].ctrl.blockCtrl.start = 0;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CTRL_WORD, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.blockCtrl.halfword, flag);

    if (dir == 0)
        blockNum = BLOCK_NUM_HOME_CW;
    else
        blockNum = BLOCK_NUM_HOME_CCW;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_NUMBER, 1, (U16 *)&blockNum, flag);

    _A6_DriverData[id - 1].ctrl.blockCtrl.start = 1;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CTRL_WORD, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.blockCtrl.halfword, flag);

    return ret;
}

U08 PanasonicA6_Jog(U08 id, U08 dir)
{
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_HOME);
    U16 blockNum = 0;

    _A6_DriverData[id - 1].ctrl.blockCtrl.start = 0;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CTRL_WORD, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.blockCtrl.halfword, flag);

    if (dir == 0)
        blockNum = BLOCK_NUM_JOG_CW;
    else
        blockNum = BLOCK_NUM_JOG_CCW;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_NUMBER, 1, (U16 *)&blockNum, flag);

    _A6_DriverData[id - 1].ctrl.blockCtrl.start = 1;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CTRL_WORD, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.blockCtrl.halfword, flag);

    return ret;
}

U08 PanasonicA6_MoveVel(U08 id, U08 dir, U32 msec)
{
    U08 ret = 0;
    Block_t block;
    U16 blockNum = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_MOVE_VEL);
    F32 rpm, acc_ms, const_ms, dec_ms;

    rpm = _A6_DriverData[id - 1].param.vel;
    acc_ms = _A6_DriverData[id - 1].param.acc_ms;
    dec_ms = _A6_DriverData[id - 1].param.dec_ms;

    const_ms = msec - acc_ms - dec_ms;
    if (const_ms < 0)
        const_ms = 0;

    F32 Ta = acc_ms / 1000.f;
    F32 Tv = const_ms / 1000.f;
    F32 Td = dec_ms / 1000.f;

    F32 pps = (rpm / 60.f) * PANASONIC_A6_PPR;
    F32 acc_pulse = 0.5f * pps * Ta;
    F32 const_pulse = pps * Tv;
    F32 dec_pulse = 0.5 * pps * Td;
    S32 total_pulse = (S32)(acc_pulse + const_pulse + dec_pulse);

    if (dir)
    {
        total_pulse *= -1;
    }

    _A6_DriverData[id - 1].ctrl.blockCtrl.start = 0;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CTRL_WORD, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.blockCtrl.halfword, flag);

    blockNum = BLOCK_NUM_MOVE;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_NUMBER, 1, (U16 *)&blockNum, flag);

    block.cmd.word = 0;
    block.cmd.cmdCode = BLOCK_COMMAND_CODE_REL_MOVE;
    block.cmd.arg1 = 1;
    block.cmd.arg2 = 1;
    block.cmd.arg3 = 1;
    block.data = total_pulse;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CMD(4), 4, (U16 *)block.word, flag);

    _A6_DriverData[id - 1].ctrl.blockCtrl.start = 1;
    ret = _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CTRL_WORD, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.blockCtrl.halfword, flag);

    return ret;
}

U08 PanasonicA6_MoveAbs(U08 id, S32 pos)
{
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_MOVE_ABS);
    Block_t block;
    U16 blockNum = 0;

    _A6_DriverData[id - 1].ctrl.blockCtrl.start = 0;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CTRL_WORD, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.blockCtrl.halfword, flag);

    blockNum = BLOCK_NUM_MOVE;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_NUMBER, 1, (U16 *)&blockNum, flag);

    block.cmd.word = 0;
    block.cmd.cmdCode = BLOCK_COMMAND_CODE_ABS_MOVE;
    block.cmd.arg1 = 1;
    block.cmd.arg2 = 1;
    block.cmd.arg3 = 1;
    block.data = pos;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CMD(4), 4, (U16 *)block.word, flag);

    _A6_DriverData[id - 1].ctrl.blockCtrl.start = 1;
    ret = _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CTRL_WORD, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.blockCtrl.halfword, flag);

    return ret;
}

U08 PanasonicA6_MoveRel(U08 id, S32 pos)
{
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_MOVE_REL);
    Block_t block;
    U16 blockNum = 0;

    _A6_DriverData[id - 1].ctrl.blockCtrl.start = 0;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CTRL_WORD, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.blockCtrl.halfword, flag);

    blockNum = BLOCK_NUM_MOVE;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_NUMBER, 1, (U16 *)&blockNum, flag);

    block.cmd.word = 0;
    block.cmd.cmdCode = BLOCK_COMMAND_CODE_REL_MOVE;
    block.cmd.arg1 = 1;
    block.cmd.arg2 = 1;
    block.cmd.arg3 = 1;
    block.data = pos;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CMD(4), 4, (U16 *)block.word, flag);

    _A6_DriverData[id - 1].ctrl.blockCtrl.start = 1;
    ret = _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CTRL_WORD, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.blockCtrl.halfword, flag);

    return ret;
}

U08 PanasonicA6_Stop(U08 id)
{
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_STOP);

    _A6_DriverData[id - 1].ctrl.blockCtrl.sStop = 1;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CTRL_WORD, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.blockCtrl.halfword, flag);
    _A6_DriverData[id - 1].ctrl.blockCtrl.sStop = 0;
    ret = _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CTRL_WORD, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.blockCtrl.halfword, flag);

    return ret;
}

U08 PanasonicA6_EStop(U08 id)
{
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_ESTOP);

    _A6_DriverData[id - 1].ctrl.blockCtrl.hstop = 1;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CTRL_WORD, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.blockCtrl.halfword, flag);
    _A6_DriverData[id - 1].ctrl.blockCtrl.hstop = 0;
    ret = _modbusWriteMultipleRegisters(id, ADDR_BLOCK_CTRL_WORD, 1, (U16 *)&_A6_DriverData[id - 1].ctrl.blockCtrl.halfword, flag);

    return ret;
}

U08 PanasonicA6_SetProfile(U08 id, U16 vel, U32 accTime_ms, U32 decTime_ms)
{
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_SET_PROFILE);
    U16 acc, dec;
    U32 time_ms;

    _A6_DriverData[id - 1].param.vel = vel;
    _A6_DriverData[id - 1].param.acc_ms = accTime_ms;
    _A6_DriverData[id - 1].param.dec_ms = decTime_ms;

    acc = (accTime_ms * 3000L / 100) / vel;
    if (acc > 10000)
    {
        time_ms = (10000 * 100) / 3000.f * vel;
        LOG_MSG_SEND("Set Profile acc time lower than %d[ms].", time_ms);
        acc = 10000;
    }
    if (acc == 0)
    {
        time_ms = (1 * 100) / 3000.f * vel;
        LOG_MSG_SEND("Set Profile acc time upper than %d[ms].", time_ms);
        acc = 1;
    }

    dec = (decTime_ms * 3000L / 100) / vel;
    if (dec > 10000)
    {
        time_ms = (10000 * 100) / 3000.f * vel;
        LOG_MSG_SEND("Set Profile dec time lower than %d[ms].", time_ms);
        dec = 10000;
    }
    if (dec == 0)
    {
        time_ms = (1 * 100) / 3000.f * vel;
        LOG_MSG_SEND("Set Profile dec time upper than %d[ms].", time_ms);
        dec = 1;
    }

    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_VEL(1), 1, (U16 *)&vel, flag);
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_ACC(1), 1, (U16 *)&acc, flag);
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_DEC(1), 1, (U16 *)&dec, flag);

    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_ACC_UNIT, 1, (U16 *)&_AccDecUnit[ACC_DEC_UNIT_100_0_MS], flag);
    ret = _modbusWriteMultipleRegisters(id, ADDR_BLOCK_DEC_UNIT, 1, (U16 *)&_AccDecUnit[ACC_DEC_UNIT_100_0_MS], flag);

    return ret;
}

U08 PanasonicA6_SetJogProfile(U08 id, U16 vel, U32 accTime_ms, U32 decTime_ms)
{
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_SET_JOG_PROFILE);
    U16 acc, dec;
    U32 time_ms;

    _A6_DriverData[id - 1].param.jogVel = vel;
    _A6_DriverData[id - 1].param.jogAcc_ms = accTime_ms;
    _A6_DriverData[id - 1].param.jogDec_ms = decTime_ms;

    acc = (accTime_ms * 3000L / 100) / vel;
    if (acc > 10000)
    {
        time_ms = (10000 * 100) / 3000.f * vel;
        LOG_MSG_SEND("Set Jog Profile acc time lower than %d[ms].", time_ms);
        acc = 10000;
    }
    if (acc == 0)
    {
        time_ms = (1 * 100) / 3000.f * vel;
        LOG_MSG_SEND("Set Jog Profile acc time upper than %d[ms].", time_ms);
        acc = 1;
    }

    dec = (decTime_ms * 3000L / 100) / vel;
    if (dec > 10000)
    {
        time_ms = (10000 * 100) / 3000.f * vel;
        LOG_MSG_SEND("Set Jog Profile dec time lower than %d[ms].", time_ms);
        dec = 10000;
    }
    if (dec == 0)
    {
        time_ms = (1 * 100) / 3000.f * vel;
        LOG_MSG_SEND("Set Jog Profile dec time upper than %d[ms].", time_ms);
        dec = 1;
    }

    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_VEL(0), 1, (U16 *)&_A6_DriverData[id - 1].param.jogVel, flag);
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_ACC(0), 1, (U16 *)&acc, flag);
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_DEC(0), 1, (U16 *)&dec, flag);

    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_ACC_UNIT, 1, (U16 *)&_AccDecUnit[ACC_DEC_UNIT_100_0_MS], flag);
    ret = _modbusWriteMultipleRegisters(id, ADDR_BLOCK_DEC_UNIT, 1, (U16 *)&_AccDecUnit[ACC_DEC_UNIT_100_0_MS], flag);

    return ret;
}

U08 PanasonicA6_SetHomeParam(U08 id, S32 offset, U16 velH, U16 velL, U32 accTime_ms)
{
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_SET_HOME_PARAM);
    U16 acc = (accTime_ms * 3000L) / velH;

    _A6_DriverData[id - 1].homeParam.offset = offset;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_HOME_OFFSET, 2, (U16 *)&_A6_DriverData[id - 1].homeParam.offset, flag);
    _A6_DriverData[id - 1].homeParam.speedHigh = velH;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_HOMING_SPEED_HIGH, 1, (U16 *)&_A6_DriverData[id - 1].homeParam.speedHigh, flag);
    _A6_DriverData[id - 1].homeParam.speedLow = velL;
    _modbusWriteMultipleRegisters(id, ADDR_BLOCK_HOMING_SPEED_LOW, 1, (U16 *)&_A6_DriverData[id - 1].homeParam.speedLow, flag);
    _A6_DriverData[id - 1].homeParam.acc_ms = accTime_ms;
    ret = _modbusWriteMultipleRegisters(id, ADDR_HOMING_ACC, 1, (U16 *)&acc, flag);

    return ret;
}

U08 PanasonicA6_UpdatePos(U08 id)
{
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_UPDATE_POS);
    S32 *p = &_A6_DriverData[id - 1].status.actualPos;

    ret = _modbusReadHoldingRegisters(id, ADDR_POS_ACTUAL_VALUE, 2, (U16 *)p, flag);

    return ret;
}

U08 PanasonicA6_UpdateVel(U08 id)
{
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_UPDATE_VEL);
    S32 *p = &_A6_DriverData[id - 1].status.actualVel;

    ret = _modbusReadHoldingRegisters(id, ADDR_VEL_ACTUAL_VALUE, 2, (U16 *)p, flag);

    return ret;
}

U08 PanasonicA6_UpdateStatus(U08 id)
{
    U08 ret = 0;
    U08 *flag = _getFlagAddress(A6_FUNC_INDEX_UPDATE_STATUS);
    A6_DriverStatus_t *p = &_A6_DriverData[id - 1].status;

#ifdef USE_MIRROR_ADDR
    ret = _modbusReadHoldingRegisters(id, MIRROR_ADDR_1_ERROR_CODE, 4, (U16 *)p->halfword, flag);
#else
    _modbusReadHoldingRegisters(id, ADDR_ERROR_CODE, 1, (U16 *)&p->errorCode);
    _modbusReadHoldingRegisters(id, ADDR_LOGICAL_OUTPUT, 1, (U16 *)&p->logicalOutput);
    _modbusReadHoldingRegisters(id, ADDR_BLOCK_STATUS_FLAG, 1, (U16 *)&p->blockStatus);
    _modbusReadHoldingRegisters(id, ADDR_LOGICAL_INPUT, 1, (U16 *)&p->logicalInput);
    ret = _modbusReadHoldingRegisters(id, ADDR_POS_ACTUAL_VALUE, 2, (U16 *)&p->actualPosition);
#endif

    return ret;
}

U16 PanasonicA6_GetErrorCode(U08 id)
{
    return _A6_DriverData[id - 1].status.errorCode;
}

U08 PanasonicA6_IsServoOn(U08 id)
{
    return _A6_DriverData[id - 1].status.logicalInput.servoOn;
}

U08 PanasonicA6_IsAlarm(U08 id)
{
    return _A6_DriverData[id - 1].status.logicalOutput.alarm;
}

U08 PanasonicA6_IsInPos(U08 id)
{
    return _A6_DriverData[id - 1].status.logicalOutput.inPos;
}

U08 PanasonicA6_IsZeroSpeed(U08 id)
{
    return _A6_DriverData[id - 1].status.logicalOutput.zeroSpeed;
}

U08 PanasonicA6_IsMoving(U08 id)
{
    return _A6_DriverData[id - 1].status.blockStatus.busy;
}

U08 PanasonicA6_IsHomeCompelte(U08 id)
{
    return _A6_DriverData[id - 1].status.blockStatus.homeCplt;
}

S32 PanasonicA6_GetPos(U08 id)
{
    return _A6_DriverData[id - 1].status.actualPos;
}

F32 PanasonicA6_GetVel(U08 id)
{
    S32 vel_pps = _A6_DriverData[id - 1].status.actualVel;
    F32 velRpm = (F32)(((F32)vel_pps / PANASONIC_A6_PPR) * 60.f);

    return velRpm;
}

U08 PanasonicA6_CheckTransaction(A6_FuncIndex_t idx)
{
    U08 ret = 0;

    if (_A6_MsgProcessed[0][idx] == _A6_MsgProcessed[1][idx])
        ret = 1;

    return ret;
}

U08 PanasonicA6_IsConnected(U08 id)
{
    return RS485_IsConnected();
}

U08 PanasonicA6_TestRun(A6_DriverCtrl_t *param)
{
    U08 ret = 0;

    switch (param->cmd)
    {
    case A6DRIVER_CTRL_CMD_INIT:
        ret = PanasonicA6_Init(param->id);
        break;
    case A6DRIVER_CTRL_CMD_EEPROM_WRITE:
        ret = PanasonicA6_EEPROM_Write(param->id);
        break;
    case A6DRIVER_CTRL_CMD_ENABLE:
        ret = PanasonicA6_Enable(param->id);
        break;
    case A6DRIVER_CTRL_CMD_DISABLE:
        ret = PanasonicA6_Disable(param->id);
        break;
    case A6DRIVER_CTRL_CMD_CLEAR_ALARM:
        ret = PanasonicA6_ClearAlarm(param->id);
        break;
    case A6DRIVER_CTRL_CMD_SET_BLOCK_NUM:
        ret = PanasonicA6_SetBlockNum(param->id, param->blockNum);
        break;
    case A6DRIVER_CTRL_CMD_HOME:
        ret = PanasonicA6_Home(param->id, param->homeDir);
        break;
    case A6DRIVER_CTRL_CMD_JOG:
        ret = PanasonicA6_Jog(param->id, param->dir);
        break;
    case A6DRIVER_CTRL_CMD_MOVE_VEL:
        ret = PanasonicA6_MoveVel(param->id, param->dir, param->msec);
        break;
    case A6DRIVER_CTRL_CMD_MOVE_ABS:
        ret = PanasonicA6_MoveAbs(param->id, param->pos);
        break;
    case A6DRIVER_CTRL_CMD_MOVE_REL:
        ret = PanasonicA6_MoveRel(param->id, param->pos);
        break;
    case A6DRIVER_CTRL_CMD_STOP:
        ret = PanasonicA6_Stop(param->id);
        break;
    case A6DRIVER_CTRL_CMD_ESTOP:
        ret = PanasonicA6_EStop(param->id);
        break;
    case A6DRIVER_CTRL_CMD_SET_PROFILE:
        ret = PanasonicA6_SetProfile(param->id, param->profile.vel, param->profile.acc_ms, param->profile.dec_ms);
        break;
    case A6DRIVER_CTRL_CMD_SET_JOG_PROFILE:
        ret = PanasonicA6_SetJogProfile(param->id, param->profile.jogVel, param->profile.jogAcc_ms, param->profile.jogDec_ms);
        break;
    case A6DRIVER_CTRL_CMD_SET_HOME_PARAM:
        ret = PanasonicA6_SetHomeParam(param->id, param->homeOffset, param->homeVelH, param->homeVelL, param->homeAcc_ms);
        break;
    case A6DRIVER_CTRL_CMD_UPDATE_STATUS:
        ret = PanasonicA6_UpdateStatus(param->id);
        break;
    }

    param->cmd = A6DRIVER_CTRL_CMD_NONE;

    return ret;
}

static U08 _modbusWriteMultipleRegisters(U08 id, U16 addr, U16 quantity, U16 *dataIn, U08 *flag)
{
    return RS485_ModbusWriteFunc(RS485_MSG_PRIORITY_HIGH, id, MODBUS_FUNC_CODE_WRITE_MULTIPLE_REG, addr, quantity, dataIn, flag);
}

static U08 _modbusReadHoldingRegisters(U08 id, U16 addr, U16 quantity, U16 *dataOut, U08 *flag)
{
    return RS485_ModbusReadFunc(RS485_MSG_PRIORITY_LOW, id, MODBUS_FUNC_CODE_READ_HOLDING_REG, addr, quantity, dataOut, flag);
}

static U08 *_getFlagAddress(A6_FuncIndex_t idx)
{
    _A6_MsgProcessed[0][idx] = 0;
    return &_A6_MsgProcessed[0][idx];
}
