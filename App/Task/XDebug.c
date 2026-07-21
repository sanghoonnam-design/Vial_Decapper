/*******************************************************************************
 * XDebug.c
 *
 *  Created on: 2024.10.14
 *      Author: RND. Kang PilSoon.
 *
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <stdarg.h>

#include "XDebug.h"
#include "XDebug_User.h"
#include "HW_Serial.h"
#include "_10_XCommand_Core.h"
#include "_04_XDiagnose.h"
#include "XParser.h"
#include "XEEPROMParam.h"

tsXSerial_cpu xUsb; // USB 포트, 디버깅용

/** ****************************************************************************
 * @defgroup [Debugging Code 3] usb 포트를 이용한 디버깅 코드
 * @note     - 메시지 큐를 이용하여 주요 태스크가 수행된 후에 메시지처리가
 *             되도록 설계됨.
 * *****************************************************************************/
QueueHandle_t xQueue_txLog; // 고정길이 큐 사용
uint8_t __message[__MSGLOG_SIZE];

#ifdef used_STATIC_MEMORY
static uint8_t __buffer[__MSGLOG_NUM * __MSGLOG_SIZE]; // __attribute__((aligned(4))); // 메시지 저장용 버퍼(static)
static StaticQueue_t xStaticQueue_txLog;               // 큐 제어 구조체
#endif
SemaphoreHandle_t xUsbMutex;

/** *****************************************************************************
 * @brief [Debugging Code 4] CLI(command line interface)
 * @note  - USB 포트를 이용한 CLI 구현 자원
 *        - Tera-Term 기준으로 개발됨.
 *        - J1C에서는 정상동작 안할 수 있음.
 * ******************************************************************************/
QueueHandle_t xQueue_rxCLI;                                           // CLI command 처리용
static uint8_t __bufferCLI[__CLI_QUEUE_SIZE * __CLI_QUEUE_ITEM_SIZE]; // 큐에 할당될 메모리 버퍼(정적)
static StaticQueue_t xStaticQueue_rxCLI;                              // 정적 큐 객체

static tsXCommandHistory cmdHistory[__CLI_MAX_HISTORY_SIZE]; // 명령어 히스토리 관리 변수
static int historyIndex = 0;                                 // 히스토리 배열에서 최신 명령어의 위치
static uint8_t historyCount = 0;                             // 현재 저장된 명령어 개수

char inputBuffer[__CLI_MAX_CMD_LENGTH]; // 현재 입력 상태
uint16_t inputIndex = 0;                // 입력 글자 인덱스

static void CLI_UART_RxCallback(U8 data); /** @note : IUART1_Callback() */
U08 gNewLine = NO;

//================================================================================ [Save 관리]
int gWait_SaveConfirm = 0;
TickType_t gSaveConfirmTick = 0;
//================================================================================

/**
 * @brief USB port Log Message 처리 태스크 : TX 처리
 */
VOID TASK_USBMessagePrint(void *pvParameters)
{
    uint8_t buf[__MSGLOG_SIZE];

    FOREVER
    {
        if (gTriggerCount < __50msec || xUsbMutex == NULL || xQueue_txLog == NULL)
        {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        if (xQueueReceive(xQueue_txLog, buf, RTOS_WAIT_FOREVER) == pdPASS)
        {
            __TASK_TRIGGER_START_Using(TEST_PORT_4, TP_IDX_taskMSG);

            U16 len = (U16)strlen((const char *)buf);
            if (xUsbMutex != NULL)
            {
                if (xSemaphoreTake(xUsbMutex, portMAX_DELAY) == pdTRUE)
                {
                    if (gNewLine)
                    { // [1]. 줄바꿈
                        xUsb.write(buf, len);
                        Network_TCP_DebugSend(buf, len);
                        gNewLine = NO;
                    }
                    else
                    { // [2]. 로그 출력
                        const char *clr = ANSI_DELETE_LINE "\r     ";
                        U16 clrLen = (U16)strlen(clr);

                        xUsb.write((uint8_t *)clr, clrLen);
                        Network_TCP_DebugSend((uint8_t *)clr, clrLen);

                        xUsb.write(buf, len);
                        Network_TCP_DebugSend(buf, len);
                    }

                    // [3]. 프롬프트 출력 : 항상 프롬프트는 밑에 출력한다.
                    if (!gWait_SaveConfirm)
                    {
                        const char *prompt = "\rRND> ";
                        U16 promptLen = 6;

                        xUsb.write((uint8_t *)prompt, promptLen);
                        Network_TCP_DebugSend((uint8_t *)prompt, promptLen);

                        U16 printLen = inputIndex;
                        while (printLen > 0 && inputBuffer[printLen - 1] == '\n')
                            printLen--;

                        if (printLen > 0)
                        {
                            xUsb.write((uint8_t *)inputBuffer, printLen);
                            Network_TCP_DebugSend((uint8_t *)inputBuffer, printLen);
                        }
                    }

                    xSemaphoreGive(xUsbMutex);
                }
            }
            __TASK_TRIGGER_END_Using(TEST_PORT_4, TP_IDX_taskMSG);
        }
    } //@end: FOREVER
}

/**
 * @brief USB port CLI 인터페이스 : RX 처리 루틴
 */
VOID Task_USBCommandLineInterface(void *pvParameters)
{
    char c;

#ifdef LOG_CONTINUE_FLAG
    static U32 connetedElapsedCount = 0;
#endif

    FOREVER
    {
        // if (xQueueReceive(xQueue_rxCLI, &c, portMAX_DELAY))  // USB 입력 대기
        if (xQueueReceive(xQueue_rxCLI, &c, 1000)) // USB 입력 대기, 맨위 상태창 업데이트 위해 수정
        {
            __TASK_TRIGGER_START_Using(TEST_PORT_3, TP_IDX_taskCLI);

            if (xSystemInfo.SL_Get_isStartMainLoop() == NO)
                continue; // 노이즈 패스

            isConnectedUSB = YES;
#ifdef LOG_CONTINUE_FLAG
            if (isConnectedUSB)
                connetedElapsedCount = gTriggerCount;
#endif

            // 2단계 입력 처리
            if (CLI_HandleSave(c))
                continue;

            // 1단계 입력 처리
            if (c == '\r' || c == '\n') // Enter 입력 (명령어 입력 완료)
            {
                if (c == '\r') // CR, LF 붙이기 마무리 작업
                {
                    inputBuffer[inputIndex++] = c; // CR 입력
                }
                inputBuffer[inputIndex++] = '\n'; // LF 입력, 사실 원칙적으로는 LF가 들어오면 앞에 CR이 있는지 조사하고 처리해야함.
                inputBuffer[inputIndex] = '\0';   // 마무리

                if (inputIndex > 2) // CR,LF이 기본으로 추가됨. 2:엔터 에러 방지
                {
                    CLI_AddToHistory(inputBuffer); // 히스토리에 추가

                    /***********************************************************************************/
                    teXParsingErrorCode result = xParser_ProcessReceivedData(inputBuffer, &xParsedData_USB, COMM_USB);

                    if (result == PARSER_ERR_SUCCESS)
                    {
                        __newLine();
                        Handle_command(&xParsedData_USB, COMM_USB);

                        // if (gCurrentError.severity == _INFO || gCurrentError.severity == _WARNING)
                        // {
                        //     SetErrorCode(ERROR_CODE_NONE, __func__, __LINE__);
                        // }
                    }
                    else
                    {
                        ERR_MSG_SEND("%s(): oops!~ Invalid Command.", __func__);
                        inputIndex = 0; // 입력 버퍼 초기화
                        continue;
                    }
                    /***********************************************************************************/
                }
                else
                {
                    __newLine();
                }

                inputIndex = 0;            // 입력 버퍼 초기화
                xQueueReset(xQueue_rxCLI); // '\n' 삭제, TODO
            }
            else if (c == '\b') // 백스페이스 처리
            {
                if (inputIndex > 0)
                {
                    inputIndex--;
                    xcprintf("\b \b");
                }
            }
            else if (c == ANSI_ESC) // '\033' , ESC 키 입력 처리
            {
                char seq[8] = {0};
                if (xQueueReceive(xQueue_rxCLI, &c, pdMS_TO_TICKS(50)))
                {
                    if (c == '[') // ESC[
                    {
                        int i = 0;

                        // 연속적으로 시퀀스 문자 수집 (예: "15~", "13~", "A", "B")
                        while (i < 6)
                        {
                            if (xQueueReceive(xQueue_rxCLI, &seq[i], pdMS_TO_TICKS(10)) != pdTRUE)
                                break;

                            if (isalpha((unsigned char)seq[i]) || seq[i] == '~')
                                break;
                            i++;
                        }

                        // 1) 방향키 처리 (A~D)
                        if (seq[0] >= 'A' && seq[0] <= 'D')
                        {
                            CLI_HandleSpecialKeys(seq[0]);
                        }
                        // 2) F1~F12 처리
                        else if (strcmp(seq, "11~") == 0)
                            CLI_HandleSpecialKeys(KEY_F1);
                        else if (strcmp(seq, "12~") == 0)
                            CLI_HandleSpecialKeys(KEY_F2);
                        else if (strcmp(seq, "13~") == 0)
                            CLI_HandleSpecialKeys(KEY_F3);
                        else if (strcmp(seq, "14~") == 0)
                            CLI_HandleSpecialKeys(KEY_F4);
                        else if (strcmp(seq, "15~") == 0)
                            CLI_HandleSpecialKeys(KEY_F5);
                        else if (strcmp(seq, "17~") == 0)
                            CLI_HandleSpecialKeys(KEY_F6);
                        else if (strcmp(seq, "18~") == 0)
                            CLI_HandleSpecialKeys(KEY_F7);
                        else if (strcmp(seq, "19~") == 0)
                            CLI_HandleSpecialKeys(KEY_F8);
                        else if (strcmp(seq, "20~") == 0)
                            CLI_HandleSpecialKeys(KEY_F9);
                        else if (strcmp(seq, "21~") == 0)
                            CLI_HandleSpecialKeys(KEY_F10);
                        else if (strcmp(seq, "23~") == 0)
                            CLI_HandleSpecialKeys(KEY_F11);
                        else if (strcmp(seq, "24~") == 0)
                            CLI_HandleSpecialKeys(KEY_F12);
                        else
                        {
                            xcprintf("Unknown ESC sequence: [%s]\r\n", seq);
                        }
                    }
                    // 터미널마다 다를수 있음.
                    else if (c == 'O') // ESC O P~S (F1~F4)
                    {
                        if (xQueueReceive(xQueue_rxCLI, &c, pdMS_TO_TICKS(50)))
                        {
                            CLI_HandleSpecialKeys(c); // 'P','Q','R','S' -> F1~F4
                        }
                    }
                    else
                    {
                        ;
                    }
                }
                else
                {
                    CLI_HandleESCKey(); // ESC 단독 입력
                }
            }
            else if (c >= 1 && c <= 26) // Ctrl 키 조합 처리 (Ctrl+A ~ Ctrl+Z)
            {
                CLI_HandleCtrl_PressedKeys(c + ANSI_CTRL_OFFSET);
            }
            else if (c == '`') // ` 키 처리
            {
                CLI_HandleBacktickKey();
            }

            /***********************************************************************************/
            else if (c >= 32 && c <= 126) // 콘솔창 처리: 일반 문자 echo 처리
            {
                if (inputIndex < __CLI_MAX_CMD_LENGTH - 1)
                {
                    inputBuffer[inputIndex++] = c;
                    xcprintf("%c", c); // echo
                }
            }
            /***********************************************************************************/

            else
            {
                xcprintf("Unknown Key Pressed: 0x%02X\r\n", c);
            }

            __TASK_TRIGGER_END_Using(TEST_PORT_3, TP_IDX_taskCLI);
        } //@end: if (xQueueReceive(xQueue_rxCLI, &c, 1000))

#ifdef LOG_CONTINUE_FLAG
        if (isConnectedUSB && (gTriggerCount - connetedElapsedCount) > __1day)
        { // 하루지나면 로그 출력 안됨.
            isConnectedUSB = NO;
        }
#endif

#if 0
        CLI_UpdateStatusPanel(); // 화면 상단 우측 : 상태 업데이트
#endif

        CLI_CheckSaveConfirmTimeout();

        vTaskDelay(1);

    } //@end: FOREVER
}

/**
 * @brief USB 포트 사용에 필요한 자원 장착!~
 */
int Init_Debug(int index)
{
    // usb 포트 함수맵핑 초기화
    xUsb.write = IUART1_WriteBytes;
    xUsb.available = IUART1_GetRecvSize;
    xUsb.getSize = IUART1_GetRecvSize;
    xUsb.read = IUART1_ReadByte;
    xUsb.rxClear = IUART1_RecvBufClear;

    //[]. init. mutex
    xUsbMutex = xSemaphoreCreateMutex();

    //[]. tx 큐 생성
#ifdef used_STATIC_MEMORY
    // [방법 1] 정적 할당인 경우
    xQueue_txLog = xQueueCreateStatic(__MSGLOG_NUM, __MSGLOG_SIZE, __buffer, &xStaticQueue_txLog);
#else
    // [방법 2] 동적 할당인 경우 : -FreeRTOSConfig.h  -> configTOTAL_HEAP_SIZE   조절 필요, 사용하지 말것!~
    xQueue_txLog = xQueueCreate(__MSGLOG_NUM, __MSGLOG_SIZE);
#endif
    if (xQueue_txLog == NULL)
    {
        printf("%s : [xQueue_txLog] creation failed!\n", __func__);
        while (1)
        {
            ; // 큐 생성 실패 시 멈춤
        }
    }

    //[]. rx(CLI command 용) 큐 생성
    xQueue_rxCLI = xQueueCreateStatic(__CLI_QUEUE_SIZE, __CLI_QUEUE_ITEM_SIZE, __bufferCLI, &xStaticQueue_rxCLI);
    if (xQueue_rxCLI == NULL)
    {
        printf("%s : [xQueue_rxCLI] creation failed!\n", __func__);
        while (1)
        {
            ; // 정지
        }
    }

#ifndef USE_RX_BUFFER_USB
    //[]. CLI callback function 셋팅
    IUART1_Callback = CLI_UART_RxCallback;
#endif

    return EXIT_SUCCESS;
}

//===========================================================================
// 명령어 히스토리 저장 함수
void CLI_AddToHistory(const char *cmd)
{
    /*[]. 끝에 있는 '\r', '\n' 제거 */
    size_t len = strlen(cmd);

    while (len > 0 && (cmd[len - 1] == '\r' || cmd[len - 1] == '\n'))
        len--;
    char trimmedCmd[__CLI_MAX_CMD_LENGTH];
    strncpy(trimmedCmd, cmd, len);
    trimmedCmd[len] = '\0'; // NULL 종료

    /*[]. 히스토리 여유 체크 */
    if (historyCount < __CLI_MAX_HISTORY_SIZE)
    {
        historyCount++;
    }
    else
    {
        // 히스토리가 꽉 찼을 경우 오래된 명령어 제거 (FIFO 방식) ==> TODO: 원형 linked-list 구조로 변경할것. 머리가 안돌아간다. ㅠ
        for (uint8_t i = 0; i < __CLI_MAX_HISTORY_SIZE - 1; i++)
        { // command를 shift 시킨다.
            strncpy(cmdHistory[i].command, cmdHistory[i + 1].command, __CLI_MAX_CMD_LENGTH);
        }
    }

    /*[]. 최신 명령어 추가 */
    strncpy(cmdHistory[historyCount - 1].command, trimmedCmd, __CLI_MAX_CMD_LENGTH);
    // historyIndex = historyCount - 1;
    historyIndex = historyCount;
}

// 히스토리에서 이전 명령어 가져오기
const char *CLI_GetPrevHistory(void)
{
    if (historyCount == 0)
        return NULL;

    // 이미 첫 번째 명령이면 그대로 유지
    if (historyIndex <= 0)
    {
        historyIndex = 0;
        return cmdHistory[0].command;
    }

    historyIndex--;
    return cmdHistory[historyIndex].command;
}

// 히스토리에서 다음 명령어 가져오기
const char *CLI_GetNextHistory(void)
{
    if (historyCount == 0)
        return NULL;

    if (historyIndex >= historyCount - 1)
    {
        historyIndex = historyCount - 1;
        return NULL; // 더 이상 없음
    }

    historyIndex++;
    return cmdHistory[historyIndex].command;
}

void CLI_UpdateStatusPanel(void)
{
    if (isConnectedUSB) // && xSystemInfo.SL_Get_isStartMainLoop() == YES)
    {
        xcprintf_xy(ANSI_BG_BRIGHT_Magenta
                    "[ CPU:%3d%%, %3.1f°C | Network: %s ]" ANSI_BG_ORG,
                    1, 60,
                    (int)xSystemInfo.CD_Get_CpuUsage(), xSystemInfo.CD_Get_CpuTemperature(),
                    xNet.isConnected_TCP ? "Connected" : "Disconnected");
    }
}

/** ********************************************************
 * @brief callback 함수 : USB 포트 ISR 에서 호출함
 * ********************************************************/
static void CLI_UART_RxCallback(U8 data)
{
    /* 수신된 데이터를 큐에 넣음 */
    /** @note pdFALSE: 컨텍스트 스위칭이 즉시 발생하지 않음.
     *        중요한 내용이 아니므로 실시간 처리 안함.
     */
    xQueueSendFromISR(xQueue_rxCLI, &data, pdFALSE);
}

void Debug_LogMessageHelper(const char *fmt, ...)
{
    va_list args;

    if (xQueue_txLog != NULL ||
        (isConnectedUSB && xSystemInfo.SL_Get_isStartMainLoop() == YES)) // 메인루프가 시작되면 실행하세요.
    {                                                                    // 초기화가 안된상태에서 다른모듈에서 사용하면 문제가 되는 부분을 해결함.

        va_start(args, fmt);
        vsnprintf((char *)__message, __MSGLOG_SIZE, fmt, args);
        va_end(args);

        if (uxQueueMessagesWaiting(xQueue_txLog) > 6) // 워!~워!~ 천천히..
        {
            vTaskDelay(50);
        }

        xQueueSend(xQueue_txLog, __message, pdMS_TO_TICKS(100));
    }
}

void CLI_ClearCurrentInput(void)
{
    xcprintf("\r");

    xcprintf("RND> ");

    for (int i = 0; i < inputIndex; i++)
        xcprintf(" ");

    xcprintf("\rRND> ");

    inputIndex = 0;
    inputBuffer[0] = '\0';
}

void CLI_Send_USB(const char *msg, uint16_t size)
{
    // xUsb.write((uint8_t *)msg, size);
    xcprintf("%s", msg);
}

//==============================================================================
bool CLI_HandleSave(char c)
{
    if (!gWait_SaveConfirm)
        return false; // Save 모드 아님 -> 다른 로직 계속

    xUsb.write((U8 *)&c, 1);
    xUsb.write((U8 *)"\r\n\0", 3);

    XTimer_Stop();
    if (c == 'Y' || c == 'y')
    {
        //        xPL.Header.UpdateDate = GetCurrentDate();
        xprintf("..........."); // delay
        // [YYMMDD] parameter를 EEPROM에 저장한 날짜 저장
        xPL.Header.UpdateDate = SWRTC_GetTime_YYMMDDHH();
        EEPROMPL_SaveToEEPROM();
    }
    else
    {
        LOG_MSG_SEND("Save canceled.");
    }

    gWait_SaveConfirm = false;

    XTimer_Start();

    return true;
}

void CLI_CheckSaveConfirmTimeout(void)
{
    if (gWait_SaveConfirm)
    {
        TickType_t now = xTaskGetTickCount();
        if ((now - gSaveConfirmTick) > pdMS_TO_TICKS(SAVE_CONFIRM_TIMEOUT_MS))
        {
            gWait_SaveConfirm = false;
            __newLine();
            LOG_MSG_SEND("Save canceled (timeout).");
        }
    }
}

void CLI_PrintAligned(int width, int index,
                      const char *contentsFmt,
                      const char *valueFmt,
                      const char *label,
                      ...)
{
    char valueName[30];
    char valueBuf[128];

    va_list args1, args2;
    int len1, len2;

    valueName[0] = '\0';
    valueBuf[0] = '\0';

    va_start(args1, label);
    va_copy(args2, args1);

    len1 = vsnprintf(valueName, sizeof(valueName),
                     (contentsFmt != NULL) ? contentsFmt : "",
                     args1);

    len2 = vsnprintf(valueBuf, sizeof(valueBuf),
                     (valueFmt != NULL) ? valueFmt : "",
                     args2);

    va_end(args2);
    va_end(args1);

    if (len1 < 0)
        valueName[0] = '\0';
    else if (len1 >= (int)sizeof(valueName))
        valueName[sizeof(valueName) - 1] = '\0';

    if (len2 < 0)
        valueBuf[0] = '\0';
    else if (len2 >= (int)sizeof(valueBuf))
        valueBuf[sizeof(valueBuf) - 1] = '\0';

    xprintf("\t[%02d] %*s " ANSI_TX_LightGreen "%-*s" ANSI_TX_ORG " : %-*s",
            index,
            COL_CONTENTS_WIDTH, valueName,
            width, valueBuf,
            COL_LABEL_WIDTH, (label != NULL) ? label : "");
}

//==============================================================================
U32 HW_Get_RAM_usage(void)
{
    ;
    return 0;
}

U32 HW_Get_Flash_usage(void)
{
    ;
    return 0;
}
//==============================================================================
