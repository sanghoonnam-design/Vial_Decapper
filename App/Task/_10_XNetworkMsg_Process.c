/*******************************************************************************
 * XNetworkMsg_Process.c
 *
 *  Created on: 2025.06.20
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#include "_10_XNetworkMsg_Process.h"
#include "_10_XCommandHandling.h"
#include "XParser.h"
#include "XErrorCode.h"

/*******************************************************************************
 ** @brief TCP transmit message handling
 *******************************************************************************/
void TCP_Send(const char *msg)
{
    size_t len = strlen(msg);

    // 8000 포트로 메시지 전송
    size_t sent = xMessageBufferSend(xMessageBuffer_TCP, msg, len, 0);

    if (sent != len)
    {
        ERR_MSG_SEND_N("[TCP_Send: 8000] Message buffer full or send failed. Sent %u / %u bytes.", sent, len);
    }

    // TODO: GSTA 같은 응답은 생략 , 8001로 보내지 말것. SD 같이 메모리 고려

    // 8001 포트로 디버깅 메시지 전송
    // 폭주함.
    // Network_TCP_DebugSend((uint8_t *)msg, len);
}

BOOL Network_TCP_DebugSend(const uint8_t *msg, size_t len)
{
    size_t sent;

    // SD
    // 초기화가 되어 있으면,
    // SD q 에 입력

    // TCP
    if (xMessageBuffer_TCP_Debug == NULL ||
        xNet.isConnected_TCP_Debug == DISCONNECTED)
        return FALSE;

    sent = xMessageBufferSend(xMessageBuffer_TCP_Debug, msg, len, 0);

    if (sent != len)
    {
        ERR_MSG_SEND_N("[TCP_Send: 8002] Buffer full or send failed (%u / %u)",
                       (unsigned int)sent,
                       (unsigned int)len);
        return FALSE;
    }

    return TRUE;
}

/*******************************************************************************
** @brief This function is a function of parsing and processing data received
**        through TCP communication.
*******************************************************************************/
#if 0
void Network_MsgProcess(int MsgId, int MsgLength, char *pData)
{
    if (gTriggerCount < __5sec) /* [주의]: 수정 금지 */
    {
        /* [] 상태머신 시작전에는 무시. */
        // 부팅후 5초간 상위로직에서 보내는 명령 무시
        // 개발간에 FW 다운로드시에 PC 에서 명령을 빠르게 계속 보내면 문제가 발생할수 있다.
        return; // 제어 로직 안정화 시간
    }

    teXParsingErrorCode result = xParser_ProcessReceivedData(pData, &xParsedData_Network, COMM_TCP);

    if (result == PARSER_ERR_SUCCESS)
    {
        Handle_command(&xParsedData_Network, COMM_TCP);

        if (gCurrentError.severity == _INFO || gCurrentError.severity == _WARNING)
        {
            SetErrorCode(ERROR_CODE_NONE);
            // xPrintError(gCurrentError.Code);
        }
        else // _CRITICAL
        {
            // TODO: USER CODE
            // _CRITICAL : 이면.. 에러 유지
            //
        }
    }
}
#else
void Network_MsgProcess(int MsgId, int MsgLength, char *pData)
{
    (void)MsgId;
    static char oneFrame[1460];
    int start = 0;
    int handled = 0;

    if (gTriggerCount < __5sec) /* [주의]: 수정 금지 */
    {
        /* [] 상태머신 시작전에는 무시. */
        // 부팅후 5초간 상위로직에서 보내는 명령 무시
        // 개발간에 FW 다운로드시에 PC 에서 명령을 빠르게 계속 보내면 문제가 발생할수 있다.
        return; // 제어 로직 안정화 시간
    }

    if (pData == NULL || MsgLength <= 0)
    {
        return;
    }

    for (int i = 0; i < MsgLength - 1; i++)
    {
        if (pData[i] == '\r' && pData[i + 1] == '\n')
        {
            int len = i - start + 2; // \r\n 포함 길이

            memcpy(oneFrame, &pData[start], len);
            oneFrame[len] = '\0';
            teXParsingErrorCode result = xParser_ProcessReceivedData(oneFrame, &xParsedData_Network, COMM_TCP);
            if (result == PARSER_ERR_SUCCESS)
            {
                Handle_command(&xParsedData_Network, COMM_TCP);
                if (gCurrentError.severity == _INFO || gCurrentError.severity == _WARNING)
                {
                    SetErrorCode(ERROR_CODE_NONE);
                }
                else
                {
                    ; // _CRITICAL 처리
                }
            }
            else
            {
                xParser_HandleError(__func__, result, __LINE__);
            }

            handled = 1;
            start = i + 2;
            i++; // LF 건너뛰기
        }
    }

    /*
     * CRLF가 하나도 없거나,
     * 마지막 명령 뒤에 CRLF 없는 데이터가 남은 경우
     */
    if (handled == 0 || start < MsgLength)
    {
        SetErrorCode(ERROR_CODE_INVALID_COMMAND);
    }
}
#endif
