/*******************************************************************************
 * HW_CAN_Process.h
 *
 *  Created on: 2024.10.15
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#ifndef __XCAN_PROCESS_H__
#define __XCAN_PROCESS_H__
#include "XGlobal.h"
#include "_00_HW_Config.h"

typedef enum
{
    CAN_CH_1,
    CAN_CHANNEL_MAX_NUM
} teCAN_Channel;

#define CAN_DATA_SIZE (8) // 8 bytes
// #define FOR_ALL_CAN for (int i = 0; i < CAN_CHANNEL_MAX_NUM; i++)
// 

extern U08 DB_isCANRxFrameDisplayEnabled;  // 디버깅 코드용
extern U08 DB_isCANTxFrameDisplayEnabled;  // RX/TX frame usb로 디스플레이

typedef U32 (*CAN_GetSize_Func_t)(void);
// typedef void (*CAN_WriteData_Func_t)(CanPacket_t* cp);
typedef void (*CAN_WriteData_Func_t)(CanPacket_t cp);
typedef void (*CAN_ReadData_Func_t)(CanPacket_t *cp);
typedef void (*CAN_ClearRxBuffer_Func_t)(void);

typedef struct
{
    CAN_GetSize_Func_t getSize;
    CAN_WriteData_Func_t write;
    CAN_ReadData_Func_t read;
    CAN_ClearRxBuffer_Func_t clear;
} tsXCAN_Func;

/****************************************************************************/
/* TX 처리 */
void xCAN_SendMessage_1(CanPacket_t msg);
// void xCAN_SendMessage_2(CanPacket_t msg);

/* CAN 통신 에러관련 */
void xCAN_LogError(int channel, const char *errorMsg, const char *functionName);

/* Debugging code */
void xCAN_DisplayDataToBinary(int channel, teComm_MSG_Type type, CanPacket_t *cp);

#endif /* __HW_CAN_PROCESS_H__ */