/*******************************************************************************
 * XSerialCMD_Process.c
 *
 *  Created on: 2025.06.20
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#include "_10_XSerialCMD_Process.h"
#include "_10_XCommand_Core.h"
#include "XErrorCode.h"
#include "XRingBuffer.h"
#include "XDebug.h"
#include "HW_Serial.h" // TODO: should be delted.  임시 테스트용

static tsXSerial_cpu _xSerial;
static tsXQueue CommandQueue; // 수신 데이터를 저장할 링버퍼

void Serial_ResetOnInvalidCommand(teXParsingErrorCode errCode, U08 position);

/** ****************************************************************************
 * @brief FPGA RS232C 1ch를 이용한 명령어 처리 태스크
 * ****************************************************************************/
VOID TASK_SerialCommandLoop(void *pvParameters)
{
    char ch;                        // 읽은 문자
    static BOOL receivedCR = false; // CR을 받았는지 여부를 추적
    TickType_t lastCRTime = 0;      // CR 수신 시각을 기록하는 변수
    teXParsingErrorCode result;     // 파싱한 결과 저장

    FOREVER
    {
        while (_xSerial.available()) 
        {
            __TASK_TRIGGER_START_Using(TEST_PORT_2, TP_IDX_taskRS232Rx);

            ch = (char)_xSerial.read();

            /* [예외처리] CR 없이 LF만 수신되는 경우 */
            if (ch == '\n' && !receivedCR)
            {
                Serial_ResetOnInvalidCommand(PARSER_ERR_INVALID_CMD, 1);
                receivedCR = false;
                continue;
            }

            if (receivedCR)
            {
                if (ch == '\n')
                {
                    char dataBuffer[BUFFER_SIZE_RX_MSG] = {0};
                    U16 dataSize = xQueue_GetSize(&CommandQueue);

                    // 링버퍼에서 데이터를 읽어와 null-terminated(\0) 문자열로 구성
                    for (int i = 0; i < dataSize; i++)
                    {
                        xQueue_Pop(&CommandQueue, (U08 *)&dataBuffer[i]);
                    }
                    dataBuffer[dataSize] = '\0';

                    // 데이터 처리
                    result = xParser_ProcessReceivedData(dataBuffer, &xParsedData_RS232C, COMM_RS232C);
                    if (result == PARSER_ERR_SUCCESS)
                    {
                        /** ******************************************************
                         * @note 실제 명령어 처리 하는 곳
                         * ******************************************************/
                        Handle_command(&xParsedData_RS232C, COMM_RS232C);

#if 1
                        /** ******************************************************
                         * @note 에러에 대한 마무리 작업
                         *       - FW 개발시 시스템에 맞게 수정해서 사용할것!~
                         * *******************************************************/
                        if (gCurrentError.severity == _INFO || gCurrentError.severity == _WARNING) // 심각한 에러가 아니면,
                        {                                                                          // 단발성으로 처리하자!
                            SetErrorCode(ERROR_CODE_NONE, __func__, __LINE__);                                         // 에러 초기화
                        }
#endif
                    }
                    else
                    {
                        /* 노이즈 또는 오류 발생 시 남은 시리얼 데이터를 비워서 초기화*/
                        Serial_ResetOnInvalidCommand(PARSER_ERR_INVALID_CMD, 2);
                        receivedCR = false;
                        continue;
                    }

                    // 링버퍼와 상태 초기화
                    xQueue_Clear(&CommandQueue);
                    receivedCR = false; // CR-LF 시퀀스 처리 완료 후 리셋
                }
                else if (ch == '\r')
                {
                    /* [예외처리] 연속된 CR을 수신한 경우 */
                    continue; // 추가 CR은 무시 해라마.짜샤!~
                }
                else
                {
                    /* [예외처리] CR 뒤에 LF가 오지 않으면 잘못된 데이터이므로 리셋 */
                    Serial_ResetOnInvalidCommand(PARSER_ERR_INVALID_CMD, 3);
                    receivedCR = false;
                    continue;
                }
            }
            else
            {
                if (ch == '\r') // CR을 받으면 다음에 LF를 기다림
                {
                    receivedCR = true;                // CR을 받았다고 플래그 설정
                    lastCRTime = xTaskGetTickCount(); // 현재 시간 기록
                }
                else
                {
                    /* 링버퍼에 문자를 추가 */
                    if (!xQueue_Push(&CommandQueue, ch))
                    {
                        /* [예외처리] 버퍼 오버플로우 처리 */
                        Serial_ResetOnInvalidCommand(PARSER_ERR_INVALID_CMD, 4);
                        receivedCR = false;
                        continue;
                    }
                }
            }
        }
        // vTaskDelay(pdMS_TO_TICKS(1));

        /* [예외처리] 일정 시간 동안 LF가 수신되지 않으면 타임아웃으로 리셋 */
        if (receivedCR && (xTaskGetTickCount() - lastCRTime) > pdMS_TO_TICKS(CR_LF_TIMEOUT_MS))
        {
            /* 타임아웃 발생 시 상태 초기화*/
            Serial_ResetOnInvalidCommand(PARSER_ERR_TIMEOUT, 5);
            receivedCR = false;
            continue;
        }
        else
        {
            /** ****************************************
             * @note [매우 중요] 너 혼자 cpu 쓰지마!~
             * ****************************************/
            vTaskDelay(pdMS_TO_TICKS(5));
        }

        __TASK_TRIGGER_END_Using(TEST_PORT_2, TP_IDX_taskRS232Rx);
    } //@end: while (_xSerial.available())
}

int Init_SerialCommand(int Index)
{
    if (!xQueue_Init(&CommandQueue, BUFFER_SIZE_RX_MSG * 5))
    {
        ERR_MSG_SEND("%s(): failed.", __func__);
    }

    Init_CommandHandling();

    // RS485 사용할 경우
//    _xSerial.write = IUART4_WriteBytes;
//    _xSerial.available = IUART4_GetRecvSize;
//    _xSerial.getSize = IUART4_GetRecvSize;
//    _xSerial.read = IUART4_ReadByte;
//    _xSerial.rxClear = IUART4_RecvBufClear;

    return EXIT_SUCCESS;
}

void Serial_ResetOnInvalidCommand(teXParsingErrorCode errCode, U08 position)
{
    char ch;

    xParser_HandleError(__func__, errCode, position);

    xQueue_Clear(&CommandQueue); // RingBuffer Clear

    while (_xSerial.available())
    {
        ch = (char)_xSerial.read();

        if (ch == '\n')
        {
            break; // 다음 명령을 받을 준비가 되면 종료
        }
    }
}

void Serial_Send(const char *msg, uint16_t size)
{
    _xSerial.write((uint8_t *)msg, size);
}
