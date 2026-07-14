/*******************************************************************************
 * XNetworkMsg_Process.h
 *
 *  Created on: 2025.06.20
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#ifndef _XNETWORKMSG_PROCESS_H_
#define _XNETWORKMSG_PROCESS_H_

#include "XGlobal.h"

BOOL Network_TCP_DebugSend(const uint8_t *msg, size_t len);
void Network_MsgProcess(int MsgId, int MsgLength, char *pData);
// void Network_CmdProcess(int CmdId, int Index, float NewValue);

void TCP_Send(const char *msg);

#endif //@end: _XNETWORKMSG_PROCESS_H_
