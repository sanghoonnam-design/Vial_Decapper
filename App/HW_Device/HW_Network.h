/*******************************************************************************
 * HW_Network.h
 *
 *  Created on: 2025.06.24
 *      Author: RND. Kang PilSoon.
 *
 *  Description:
 *      - 네트워크 인터페이스 정의
 ******************************************************************************/
#ifndef _HW_NETWORK_H_
#define _HW_NETWORK_H_

#include "XGlobal.h"
#include "socket.h"

#ifdef _HW_NETWORK_C_
#define NETWORK_EXT
#else
#define NETWORK_EXT extern
#endif

extern BOOL DB_isTCPDebuggingMsg; // for debugging

typedef struct NetworkGroup
{
	int isConnected_TCP;
	int isConnected_TCP_Debug;
	int isConnected_UDP;
} tsXNetwork;
extern tsXNetwork xNet;

extern MessageBufferHandle_t xMessageBuffer_TCP;	   // 8000 포트: 모듈에서 사용
extern MessageBufferHandle_t xMessageBuffer_TCP_Debug; // 8001 포트: 디버깅용

VOID TASK_Network(void *pvParameters); // network task

NETWORK_EXT void Network_Init(void);
NETWORK_EXT void Network_TcpServer(SOCKET sn, U8 *pMsg, U16 port);
NETWORK_EXT void Network_UdpComm(SOCKET sn, U8 *pMsg, U16 port);

NETWORK_EXT void Network_CLI_Process(uint8_t sock, uint8_t *data, int len);

#endif /* _HW_NETWORK_H_ */
