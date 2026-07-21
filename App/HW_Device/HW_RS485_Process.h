#ifndef __HW_RS485_PROCESS_H__
#define __HW_RS485_PROCESS_H__

#include "project.h"

#ifdef __HW_RS485_PROCESS_C__
#define HW_RS485_PROCESS_EXT
#else
#define HW_RS485_PROCESS_EXT extern
#endif

#define RS485_MSG_DATA_SIZE (4)
#define RS485_MODBUS_DATA_SIZE_MAX (10)

typedef enum MODBUS_FUNC_ERROR_CODE_t
{
    MODBUS_FUNC_ERROR_CODE_NONE = 0,
    MODBUS_FUNC_ERROR_CODE_QUEUE_FULL,
    MODBUS_FUNC_ERROR_CODE_SIZE,
} MODBUS_FUNC_ERROR_CODE_t;

typedef enum RS485_MsgPriority_t
{
    RS485_MSG_PRIORITY_HIGH,
    RS485_MSG_PRIORITY_LOW
} RS485_MsgPriority_t;

typedef enum ModbusFuncCode_t
{
    MODBUS_FUNC_CODE_READ_COLIS = 0x1,
    MODBUS_FUNC_CODE_READ_DISCRETE_INPUTS,
    MODBUS_FUNC_CODE_READ_HOLDING_REG,
    MODBUS_FUNC_CODE_READ_INPUT_REG,
    MODBUS_FUNC_CODE_WRITE_SINGLE_COIL,
    MODBUS_FUNC_CODE_WRITE_SINGLE_REG,
    MODBUS_FUNC_CODE_WRITE_MULTIPLE_COILS = 0x0F,
    MODBUS_FUNC_CODE_WRITE_MULTIPLE_REG
} ModbusFuncCode_t;

typedef enum RS485_ProtocolType_t
{
    RS485_PROTOCOL_NONE,
    RS485_PROTOCOL_MODBUS,
    RS485_PROTOCOL_FFU,
    RS485_PROTOCOL_ASCII
} RS485_ProtocolType_t;

#pragma pack(push, 1)
typedef struct ModbusMsg_t
{
    U08 id;
    U08 funcCode;
    U16 address;
    U16 quantity;
    U16 dataIn[RS485_MODBUS_DATA_SIZE_MAX];
    U16 *dataOut;
    U08 *flag;
} ModbusMsg_t;

typedef struct FFU_Msg_t
{
    U08 cmd;
    U08 *data;
} FFU_Msg_t;

typedef struct ASCII_Msg_t
{
    U08 cmd;
    U08 *data;
} ASCII_Msg_t;

typedef struct RS485_Msg_t
{
    U08 protocol;
    union
    {
        ModbusMsg_t mb;
        FFU_Msg_t ffu;
        ASCII_Msg_t ascii;
    };
} RS485_Msg_t;
#pragma pack(pop)

HW_RS485_PROCESS_EXT U32 retryCount_RS485[3];
HW_RS485_PROCESS_EXT void RS485_Init(void); // RS485 통신 초기화
HW_RS485_PROCESS_EXT U08 RS485_ModbusWriteFunc(U08 priority, U08 id, U08 fc, U16 addr, U16 quantity, U16 *dataIn, U08 *flag);
HW_RS485_PROCESS_EXT U08 RS485_ModbusReadFunc(U08 priority, U08 id, U08 fc, U16 addr, U16 quantity, U16 *dataOut, U08 *flag);
HW_RS485_PROCESS_EXT uint8_t RS485_IsConnected(void);
#endif
