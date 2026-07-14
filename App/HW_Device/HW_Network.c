/*******************************************************************************
 * XNetwork.c
 *
 *  Created on: 2025.06.24
 *      Author: RND. Kang PilSoon.
 *
 *  Description:
 *      - 네트워크 초기화 및 송수신 처리
 ******************************************************************************/
#define _HW_NETWORK_C_
#include "HW_Network.h"
#undef _HW_NETWORK_C_

#include "FMC.h"
#include "wizchip_conf.h"
#include "socket.h"

#include "_10_XNetwork_Protocol_Def.h"
#include "_10_XNetworkMsg_Process.h"
#include "XSystemInfo.h"
#include "XDebug.h"

#include "http_server_task.h"

#define S0 0
#define S1 1
#define S2 2
#define S3 3
#define S4 4
#define S5 5
#define S6 6
#define S7 7

#define PORT 5000

typedef enum
{
    ST_PASSIVE_LISTEN,   // passive socket listening
    ST_SWITCH_TO_ACTIVE, // passive → active 교체
    ST_ACTIVE_CONNECTED  // active에서 통신 유지
} TCP_STATE;

typedef enum
{
    TCP_ROLE_APP = 0,  // 8000 포트: 모듈에서 사용
    TCP_ROLE_ENGINEER, // 8001 포트: 디버깅용
} TCP_ROLE;

typedef struct
{
    uint8_t active;
    uint8_t passive;
    TCP_STATE state;
    MessageBufferHandle_t msgBuffer;
    TCP_ROLE role;
} tcp_ctrl_t;

static void tcp_listen_retry(uint8_t sock, uint16_t port);
static void tcp_close_safe(uint8_t sock);
static void TcpServer_DualSocket_FSM(tcp_ctrl_t *tcp, U8 *pMsg, U16 port);

MessageBufferHandle_t xMessageBuffer_TCP;
MessageBufferHandle_t xMessageBuffer_TCP_Debug;

tcp_ctrl_t _appSocket = {
    .active /*   */ = S1,
    .passive /*  */ = S2,
    .state /*    */ = ST_PASSIVE_LISTEN,
    .msgBuffer /**/ = NULL,
    .role /*     */ = TCP_ROLE_APP};

tcp_ctrl_t _appMonitorSocket = {
    .active /*   */ = S3,
    .passive /*  */ = S4,
    .state /*    */ = ST_PASSIVE_LISTEN,
    .msgBuffer /**/ = NULL,
    .role /*     */ = TCP_ROLE_APP};

tcp_ctrl_t _engineerSocket = {
    .active /*   */ = S5,
    .passive /*  */ = S6,
    .state /*    */ = ST_PASSIVE_LISTEN,
    .msgBuffer /**/ = NULL,
    .role /*     */ = TCP_ROLE_ENGINEER};

tsXNetwork xNet;

static wiz_NetInfo netInfo;
static wiz_NetInfo netInfoRd;
static U32 NetworkTaskTick; // 디버깅 코드

static U8 TcpMsgBuf[TCP_MSS]; // TCP_MSS (1460)
// static U8 UdpMsgBuf[UDP_MSS]; // UDP_MSS (1472)

BOOL DB_isTCPDebuggingMsg = NO;
static U16 ChipID;
static U8 ChipReadFail;
void Network_Init(void)
{
    uint8_t txsize[8] = {8, 8, 8, 8, 8, 8, 8, 8};
    uint8_t rxsize[8] = {8, 8, 8, 8, 8, 8, 8, 8};

    FMC_Init();

    ChipReadFail = 0;
    ChipID = getIDR();
    if (ChipID != 0x5300)
    {
        ChipReadFail = 1;
    }

    // wiznet init begin
    wizchip_init(txsize, rxsize);
    memcpy(netInfo.ip, xSystemInfo.network.ip, 4);
    memcpy(netInfo.sn, xSystemInfo.network.subnet, 4);
    memcpy(netInfo.gw, xSystemInfo.network.gw, 4);
    netInfo.mac[0] = xSystemInfo.network.ip[0];
    netInfo.mac[1] = xSystemInfo.network.ip[1];
    netInfo.mac[2] = xSystemInfo.network.ip[2];
    netInfo.mac[3] = xSystemInfo.network.ip[3];
    netInfo.mac[4] = netInfo.ip[2];
    netInfo.mac[5] = netInfo.ip[3];
    wizchip_setnetinfo(&netInfo);
    wizchip_getnetinfo(&netInfoRd);

    setRTR(4000); // 0.1ms * 4000 = 400ms
    setRCR(0x07); // retry count = 1 + 7
    // wiznet init end

    memset((char *)&xNet, 0, sizeof(tsXNetwork));
    xNet.isConnected_TCP = DISCONNECTED;
    xNet.isConnected_UDP = DISCONNECTED;
}

/**
 * @brief Network Task
 *    - TCP/IP : client 통신용, 제어 및 디버깅용
 *    - UDP    : client 를 통해 FW 를 다운로드 하여 업데이트 하는 용도
 */
U32 NetworkTickElapseTimeMax = 0;
U32 NetworkTickElapseTime = 0;

VOID TASK_Network(void *pvParameters)
{
    xMessageBuffer_TCP = xMessageBufferCreate(1024 * 5);        // 8000 포트: 모듈에서 사용
    xMessageBuffer_TCP_Debug = xMessageBufferCreate(1024 * 5); // 8002 포트: 디버깅용

    _appSocket.msgBuffer = xMessageBuffer_TCP;
    _appMonitorSocket.msgBuffer = xMessageBuffer_TCP;
    _engineerSocket.msgBuffer = xMessageBuffer_TCP_Debug;

    NetworkTaskTick = 0;

    while (xSystemInfo.SL_Get_isStartMainLoop() != YES)
        ;

    FOREVER
    {
        U32 startTick = ITIMER_StartMeasure_us();

        TcpServer_DualSocket_FSM(&_appSocket, TcpMsgBuf, xSystemInfo.network.portNum);
        TcpServer_DualSocket_FSM(&_appMonitorSocket, TcpMsgBuf, xSystemInfo.network.portNum + 1);
        TcpServer_DualSocket_FSM(&_engineerSocket, TcpMsgBuf, xSystemInfo.network.portNum + 2);

        http_server_task_all();

        NetworkTickElapseTime = ITIMER_StopMeasure_us(startTick);

        if (NetworkTickElapseTimeMax < NetworkTickElapseTime)
            NetworkTickElapseTimeMax = NetworkTickElapseTime;

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

#if 1
static void tcp_listen_retry(uint8_t sock, uint16_t port)
{
    socket(sock, Sn_MR_TCP, port, 0);

    // W5300의 Sn_KPALVTR 는 5초 단위
    setSn_KPALVTR(sock, 1); // 5초마다 패킷 송신

    listen(sock);

    // LISTEN 성공 확인
    if (getSn_SR(sock) != SOCK_LISTEN)
    {
        LOG_MSG_SEND("[WARN] socket %d LISTEN failed → retry", sock);
        close(sock);
        vTaskDelay(5);
        socket(sock, Sn_MR_TCP, port, 0);
        listen(sock);
    }

    LOG_MSG_SEND("[TCP] socket %d LISTEN start", sock);
}

static void tcp_close_safe(uint8_t sock)
{
    uint8_t st = getSn_SR(sock);

    LOG_MSG_SEND("[TCP] safe close socket %d (state=%d)", sock, st);

    if (st == SOCK_ESTABLISHED || st == SOCK_CLOSE_WAIT)
        disconnect(sock);

    if (sock == _appSocket.active)
        xNet.isConnected_TCP = DISCONNECTED;

    if (sock == _engineerSocket.active)
        xNet.isConnected_TCP_Debug = DISCONNECTED;

    close(sock);
}

static void TcpServer_DualSocket_FSM(tcp_ctrl_t *tcp, U8 *pMsg, U16 port)
{
    uint8_t as = tcp->active;  // Active Socket
    uint8_t ps = tcp->passive; // Passive Socket
    uint8_t sr_as = getSn_SR(as);
    uint8_t sr_ps = getSn_SR(ps);
    uint8_t ir_as = getSn_IR(as);
    uint8_t ir_ps = getSn_IR(ps);
    MessageBufferHandle_t msgBuffer = tcp->msgBuffer;
    uint16_t size = 0;
    static uint8_t msgSndRcv[1024];
    size_t recvLen = 0;
    int msgLen = 0;

    int *pConnectionFlag = (tcp->role == TCP_ROLE_APP)
                               ? &xNet.isConnected_TCP
                               : &xNet.isConnected_TCP_Debug;

    if (sr_as == SOCK_ESTABLISHED)
        *pConnectionFlag = CONNECTED;
    else
        *pConnectionFlag = DISCONNECTED;

    /* -------------------------
     * Sn_IR 기반 이벤트 처리
     * -------------------------*/
    if (ir_as & Sn_IR_TIMEOUT)
    {
        LOG_MSG_SEND("[EVENT] TIMEOUT on active %d → closing", as);
        setSn_IR(as, Sn_IR_TIMEOUT);
        tcp_close_safe(as);
        tcp->state = ST_PASSIVE_LISTEN;
    }

    if (ir_ps & Sn_IR_TIMEOUT)
    {
        LOG_MSG_SEND("[EVENT] TIMEOUT on passive %d → LISTEN restart", ps);
        setSn_IR(ps, Sn_IR_TIMEOUT);
        tcp_close_safe(ps);
        tcp_listen_retry(ps, port);
    }

    if (ir_as & Sn_IR_DISCON)
    {
        LOG_MSG_SEND("[EVENT] active %d DISCON received", as);
        setSn_IR(as, Sn_IR_DISCON);
        tcp_close_safe(as);
        tcp->state = ST_PASSIVE_LISTEN;
    }

    if (ir_ps & Sn_IR_DISCON)
    {
        LOG_MSG_SEND("[EVENT] passive %d DISCON → LISTEN", ps);
        setSn_IR(ps, Sn_IR_DISCON);
        tcp_close_safe(ps);
        tcp_listen_retry(ps, port);
    }

    switch (tcp->state)
    {
    case ST_PASSIVE_LISTEN:
        if (sr_ps == SOCK_CLOSED)
        {
            LOG_MSG_SEND("[EVENT] passive %d CLOSED → LISTEN", ps);
            tcp_listen_retry(ps, port);
        }

        if (sr_ps == SOCK_ESTABLISHED)
        {
            LOG_MSG_SEND("[EVENT] passive %d got new connection", ps);
            tcp->state = ST_SWITCH_TO_ACTIVE;
        }

        break;

    case ST_SWITCH_TO_ACTIVE:
        if (sr_as == SOCK_ESTABLISHED || sr_as == SOCK_CLOSE_WAIT ||
            sr_as == SOCK_FIN_WAIT || sr_as == SOCK_CLOSING)
        {
            switch (sr_as)
            {
            case SOCK_ESTABLISHED:
                LOG_MSG_SEND("[EVENT] closing previous ACTIVE %d SOCK_ESTABLISHED", as);
                break;
            case SOCK_CLOSE_WAIT:
                LOG_MSG_SEND("[EVENT] closing previous ACTIVE %d SOCK_CLOSE_WAIT", as);
                break;
            case SOCK_FIN_WAIT:
                LOG_MSG_SEND("[EVENT] closing previous ACTIVE %d SOCK_FIN_WAIT", as);
                break;
            case SOCK_CLOSING:
                LOG_MSG_SEND("[EVENT] closing previous ACTIVE %d SOCK_CLOSING", as);
                break;
            }

            tcp_close_safe(as);
        }

        LOG_MSG_SEND("[EVENT] promote passive %d → ACTIVE", ps);

        tcp->active = ps;
        tcp->passive = as;

        LOG_MSG_SEND("[EVENT] new passive %d LISTEN", tcp->passive);
        tcp_listen_retry(tcp->passive, port);

        tcp->state = ST_ACTIVE_CONNECTED;
        break;

    case ST_ACTIVE_CONNECTED:
        if (sr_as == SOCK_ESTABLISHED)
        {
            size = getSn_RX_RSR(as);

            if (size > 0)
            {
                msgLen = recv(as, pMsg, size);

                if (tcp->role == TCP_ROLE_ENGINEER)
                {
                    Network_CLI_Process(as, pMsg, msgLen); // 디버깅용 CLI 처리
                }
                else
                {
                    Network_MsgProcess(0, msgLen, (char *)pMsg); // 기존 프로토콜
                }

                if (msgLen <= 0)
                {
                    LOG_MSG_SEND("[ERROR] recv error on %d → closing", as);
                    tcp_close_safe(as);
                    tcp->state = ST_PASSIVE_LISTEN;
                }
            }

#if 0
            recvLen = xMessageBufferReceive(xMessageBuffer_TCP, (void *)&msgSndRcv, sizeof(msgSndRcv), 0);
			if (recvLen > 0)
			{
				send(as, msgSndRcv, recvLen);
			}
#else
            while ((recvLen = xMessageBufferReceive(msgBuffer, msgSndRcv, sizeof(msgSndRcv), 0)) > 0)
            {
                send(as, msgSndRcv, recvLen);
            }
#endif
        }
        else if (sr_as == SOCK_CLOSE_WAIT || sr_as == SOCK_FIN_WAIT || sr_as == SOCK_CLOSING)
        {
            LOG_MSG_SEND("[EVENT] active %d ended(CLOSE_WAIT/FIN_WAIT)", as);
            tcp_close_safe(as);
            tcp->state = ST_PASSIVE_LISTEN;
        }
        else if (sr_as == SOCK_CLOSED)
        {
            LOG_MSG_SEND("[EVENT] active %d unexpectedly CLOSED", as);
            tcp->state = ST_PASSIVE_LISTEN;
        }

        if (sr_ps == SOCK_ESTABLISHED)
        {
            LOG_MSG_SEND("[EVENT] passive %d got new connection → switch ,SOCK_ESTABLISHED", ps);
            tcp->state = ST_SWITCH_TO_ACTIVE;
        }

        break;
    }
}

#elif 0
void Network_TcpServer(SOCKET sn, U8 *pMsg, U16 port)
{
    uint16_t size = 0;
    uint8_t msgSndRcv[300];
    size_t recvLen = 0;
    int msgLen = 0;

    uint8_t currentState = getSn_SR(sn);

    switch (currentState)
    {
    case SOCK_ESTABLISHED:
        if (getSn_IR(sn) & Sn_IR_CON)
        {
            setSn_IR(sn, Sn_IR_CON);
        }

        if (currentState != prevSocketState[sn])
        {
            LOG_MSG_SEND("Socket[%d] TCP: accepted.. -> state %d", sn, currentState);

            if (isConnected_TCP[connectedSocketNum[0]] == CONNECTED)
            {
                disconnect(connectedSocketNum[0]);
            }

            connectedSocketNum[0] = sn;
            isConnected_TCP[sn] = CONNECTED;
        }

        if ((size = getSn_RX_RSR(sn)) > 0)
        {
            // LED_Toggle_h((ELedNum)XHW_STATUS_LED_1, 4); // 통신 상태 점검용

            msgLen = recv(sn, pMsg, size);
            Network_MsgProcess(0, msgLen, (char *)pMsg); // 수신처리
        }

        /** @brief Send Process */
        recvLen = xMessageBufferReceive(xMessageBuffer_TCP, (void *)&msgSndRcv, sizeof(msgSndRcv), 0);
        if (recvLen > 0)
        {
            send(sn, msgSndRcv, recvLen);
        }
        //====================================================================== // @USER CODE END
        break;
    case SOCK_CLOSE_WAIT:
        if (currentState != prevSocketState[sn])
            LOG_MSG_SEND("Socket[%d] TCP: SOCK_CLOSE_WAIT -> Closing socket.", sn);
        disconnect(sn);
        close(sn);
        break;
    case SOCK_INIT:
        if (currentState != prevSocketState[sn])
        {
            LOG_MSG_SEND("Socket[%d] TCP: SOCK_INIT -> Setting to LISTEN.", sn);
        }
        listen(sn);
        break;
    case SOCK_CLOSED:
        if (currentState != prevSocketState[sn])
            LOG_MSG_SEND("Socket[%d] TCP: SOCK_CLOSED -> Reopening socket.", sn);
        isConnected_TCP[sn] = DISCONNECTED;
        ChipID = getIDR();
        socket(sn, Sn_MR_TCP, port, 0x00);
        wizchip_getnetinfo(&netInfo);
        break;
    default:
        break;
    }

    prevSocketState[sn] = currentState;
}

static void Network_TcpServerEng(SOCKET sn, U8 *pMsg, U16 port)
{
    uint16_t size = 0;
    uint8_t msgSndRcv[300];
    size_t recvLen = 0;
    int msgLen = 0;

    uint8_t currentState = getSn_SR(sn);

    switch (currentState)
    {
    case SOCK_ESTABLISHED:
        if (getSn_IR(sn) & Sn_IR_CON)
        {
            setSn_IR(sn, Sn_IR_CON);
        }

        if (currentState != prevSocketState[sn])
        {
            LOG_MSG_SEND("Socket[%d] TCP: accepted.. -> state %d", sn, currentState);

            if (isConnected_TCP[connectedSocketNum[1]] == CONNECTED)
            {
                disconnect(connectedSocketNum[1]);
            }

            connectedSocketNum[1] = sn;
            isConnected_TCP[sn] = CONNECTED;
        }

        if ((size = getSn_RX_RSR(sn)) > 0)
        {
            // LED_Toggle_h((ELedNum)XHW_STATUS_LED_1, 4); // 통신 상태 점검용

            msgLen = recv(sn, pMsg, size);
            Network_MsgProcess(0, msgLen, (char *)pMsg); // 수신처리
        }

        /** @brief Send Process */
        recvLen = xMessageBufferReceive(xMessageBuffer_TCP, (void *)&msgSndRcv, sizeof(msgSndRcv), 0);
        if (recvLen > 0)
        {
            send(sn, msgSndRcv, recvLen);
        }
        //====================================================================== // @USER CODE END
        break;
    case SOCK_CLOSE_WAIT:
        if (currentState != prevSocketState[sn])
            LOG_MSG_SEND("Socket[%d] TCP: SOCK_CLOSE_WAIT -> Closing socket.", sn);
        disconnect(sn);
        break;
    case SOCK_INIT:
        if (currentState != prevSocketState[sn])
        {
            LOG_MSG_SEND("Socket[%d] TCP: SOCK_INIT -> Setting to LISTEN.", sn);
        }
        listen(sn);
        break;
    case SOCK_CLOSED:
        if (currentState != prevSocketState[sn])
            LOG_MSG_SEND("Socket[%d] TCP: SOCK_CLOSED -> Reopening socket.", sn);
        isConnected_TCP[sn] = DISCONNECTED;
        close(sn);
        ChipID = getIDR();
        socket(sn, Sn_MR_TCP, port, 0x00);
        wizchip_getnetinfo(&netInfo);
        break;
    default:
        break;
    }

    prevSocketState[sn] = currentState;
}

#else
void Network_TcpServer(SOCKET sn, U8 *pMsg, U16 port)
{
    // tsXMessage_TCP msgSndRcv;
    uint16_t size = 0;
    uint8_t msgSndRcv[300];
    size_t recvLen = 0;
    int msgLen = 0;

    // 디버깅 코드-START
    static uint32_t listenTime = 0; // SOCK_LISTEN 상태 지속 시간 저장용
    static uint8_t prevSocketState = 0xFF;
    static TickType_t socketStuckTick = 0;
    static TickType_t lastPrintTick_LISTEN = 0;
    TickType_t now = 0;
    // 디버깅 코드-END

    uint8_t currentState = getSn_SR(sn);

#if (0) && DB_isTCPDebuggingMsg // 상태 점검용 디버깅 코드
    if (currentState != prevSocketState)
    {
        LOG_MSG_SEND("TCP: state 0x%X", currentState);
    }
#endif

    switch (currentState)
    {
    case SOCK_ESTABLISHED:
        if (currentState != prevSocketState)
            LOG_MSG_SEND("TCP: accepted.. -> state %d", currentState);
        //====================================================================== // @USER CODE BEGIN
        /** @brief Receive Process */
        if ((size = getSn_RX_RSR(sn)) > 0)
        {
            LED_Toggle_h((ELedNum)XHW_STATUS_LED_1, 4); // 통신 상태 점검용

            msgLen = recv(sn, pMsg, size);
            // Socket_Send(Sock, pMsg, msgLen);               // test code: echo
            // xcprintf("RcvMsg Len = %d, %s", msgLen, pMsg); // test code: loopback

            Network_MsgProcess(0, msgLen, (char *)pMsg); // 수신처리
        }

        /** @brief Send Process */
        recvLen = xMessageBufferReceive(xMessageBuffer_TCP, (void *)&msgSndRcv, sizeof(msgSndRcv), pdMS_TO_TICKS(1));
        if (recvLen > 0)
        {
            send(sn, msgSndRcv, recvLen);
        }
        //====================================================================== // @USER CODE END

        socketStuckTick = 0;
        listenTime = 0;
        break;

    case SOCK_CLOSE_WAIT:
        if (currentState != prevSocketState)
            LOG_MSG_SEND("TCP: SOCK_CLOSE_WAIT -> Closing socket.");
        disconnect(sn);
        break;

    case SOCK_FIN_WAIT:
    case SOCK_TIME_WAIT:
    case SOCK_LAST_ACK:
    case SOCK_CLOSING:
        if (socketStuckTick == 0)
            socketStuckTick = xTaskGetTickCount();
        else if ((xTaskGetTickCount() - socketStuckTick) > pdMS_TO_TICKS(5000))
        {
            LOG_MSG_SEND("TCP: Stuck in state %d -> Reopening socket.", currentState);
            close(sn);
            vTaskDelay(pdMS_TO_TICKS(100));
            if (socket(sn, Sn_MR_TCP, port, 0x00) != sn)
                ERR_MSG_SEND("TCP: Failed to reopen socket.");
            socketStuckTick = 0;
        }
        break;
    case SOCK_CLOSED:
        if (currentState != prevSocketState)
            LOG_MSG_SEND("TCP: SOCK_CLOSED -> Reopening socket.");
        xNet.isConnected_TCP = DISCONNECTED;
        close(sn);
        vTaskDelay(pdMS_TO_TICKS(100));
        if (socket(sn, Sn_MR_TCP, port, 0x00) != sn)
            ERR_MSG_SEND("TCP: Failed to open socket %d", sn);
        break;

    case SOCK_INIT:
        if (currentState != prevSocketState)
        {
            // LOG_MSG_SEND("TCP: SOCK_INIT -> Setting to LISTEN.");
        }
        listen(sn);
        break;

    case SOCK_ARP:
        if (currentState != prevSocketState)
        {
            LOG_MSG_SEND("TCP: SOCK_ARP -> Waiting for ARP resolution.");
        }
        break;
    case SOCK_LISTEN:
        now = xTaskGetTickCount();

        if (now - lastPrintTick_LISTEN > pdMS_TO_TICKS(2000)) // 2 간격 제한
        {
            // LOG_MSG_SEND("TCP: SOCK_LISTEN -> Waiting for connection...");
            lastPrintTick_LISTEN = now;
        }

        if (listenTime == 0)
        {
            listenTime = xTaskGetTickCount(); // LISTEN 상태 진입 시 타이머 시작
        }
        else if ((xTaskGetTickCount() - listenTime) > pdMS_TO_TICKS(5000))
        {
            // LISTEN 상태가 5초 이상 지속되면 소켓 재설정
            // LOG_MSG_SEND("TCP: LISTEN timeout -> Restarting socket.");
            close(sn);
            vTaskDelay(pdMS_TO_TICKS(500));
            if (socket(sn, Sn_MR_TCP, port, 0x00) != sn)
                ERR_MSG_SEND("TCP: Failed to open socket %d", sn);
            listenTime = 0;
        }
        break;
    case SOCK_SYNSENT:
        if (currentState != prevSocketState)
            LOG_MSG_SEND("TCP: SOCK_SYNSENT -> Connection request sent.");
        break;
    case SOCK_SYNRECV:
        if (currentState != prevSocketState)
            LOG_MSG_SEND("TCP: SOCK_SYNRECV -> Connection request received.");
        break;
    case SOCK_IPRAW:
    case SOCK_MACRAW:
    case SOCK_PPPoE:
        if (currentState != prevSocketState)
            LOG_MSG_SEND("TCP: Unsupported socket mode detected (%d). Closing socket.", currentState);
        close(sn);
        break;

    default:
        // if (currentState != prevSocketState)
        //     LOG_MSG_SEND("TCP: Oops!~ Unknown socket state %d", currentState);
        // Socket_Close(Sock);
        // vTaskDelay(pdMS_TO_TICKS(100));
        // if (!Socket_Open(Sock, Sn_MR_TCP, xSystemInfo.network.portNum))
        //     ERR_MSG_SEND("TCP: Failed to open socket %d", Sock);
        break;
    }

    prevSocketState = currentState;

    /** 상태 전이 예시(TCP)
        INITIAL → SOCK_CLOSED
            ↓ Sn_CR_OPEN
        SOCK_INIT
            ↓ Sn_CR_LISTEN (서버)
        SOCK_LISTEN
            ↓ 클라이언트 접속
        SOCK_ESTABLISHED
            ↓ Sn_CR_DISCON or FIN from peer
        SOCK_CLOSE_WAIT or FIN_WAIT
            ↓ Sn_CR_CLOSE or timeout
        SOCK_CLOSED
     **/
}
#endif

void Network_UdpComm(SOCKET sn, U8 *pMsg, U16 port)
{
#if 0
    U16 len = 0;
    SockAddr_t dest_addr;

    if((W5300_GetSocketRxReceivedSize(Sock)) > 0)                   // check the size of received data
    {
        len = Socket_Recvfrom(sn, (void*)pMsg, &dest_addr);
        if(len)
        {
            Socket_Sendto(sn,pMsg,len,&dest_addr);
        }
    }
#endif
}

void Network_CLI_Process(uint8_t sock, uint8_t *data, int len)
{
    char c;

    for (int i = 0; i < len; i++)
    {
        c = (char)data[i];

        xQueueSend(xQueue_rxCLI, &c, 0);
    }
}
