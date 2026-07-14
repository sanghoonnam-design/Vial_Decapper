/*******************************************************************************
 * XSerialCMD_Process.h
 *
 * Created on: 2025.06.20
 * Author    : RND. Kang PilSoon.
 *
 ******************************************************************************/
#ifndef __XSERIALCMD_PROCESS_H__
#define __XSERIALCMD_PROCESS_H__

#include "XGlobal.h"
#include "XParser.h"

#define CR_LF_TIMEOUT_MS (5) // 5 msec

extern U08 gPL_isControl_PSTA; // 디버깅용 코드 TODO

int Init_SerialCommand(int Index);

/**
 * @brief 명령어 처리 태스크
 * */
VOID TASK_SerialCommandLoop(void *pvParameters);
void Serial_Send(const char *msg, uint16_t size);
#endif /* __XSERIALCMD_PROCESS_H__ */