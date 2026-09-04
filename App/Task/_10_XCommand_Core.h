/*******************************************************************************
 * XCommand_Core.h
 *
 *  Created on: 2025.06.20
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#ifndef _XCOMMAND_CORE_H_
#define _XCOMMAND_CORE_H_

#include "XGlobal.h"
#include "XParser.h"
#include "XBuffer.h"
#include "_10_XNetworkMsg_Process.h"
#include "_10_XSerialCMD_Process.h"
#include "XDebug.h"

extern tsXBuffer *xSendMsg;
extern SemaphoreHandle_t xMutex_Command;

#if 0
#define BUFFER_SIZE_SENDMESSAGE (200)
#else
#define BUFFER_SIZE_SENDMESSAGE (512)
#endif

typedef void (*CommandHandler)(const tsXParsedData *parsedData, U08 useTCP);

typedef struct
{
    bool CommandType;       // [1]. 0:system 명령(실제 사용하는 명령), 1:debugging 명령
    char *Command;          // [2]. 명령어 문자열
    CommandHandler Handler; // [3]. 처리 함수 포인터
    char *Help;             // [4]. 명령어 설명
    char *exHelp;           // [5]. 예제 설명
} tsXCommandMapping;

typedef enum
{
    CLI_COMMAND_SYSTEM = 0, // 실제 상위 app. 에서 사용하는 명령
    CLI_COMMAND_DEBUG = 1   // 개발자 디버깅용 명령
} teXCLI_CommandType;

#define SEND_MESSAGE(__msg, __msgSize, __CommType)                              \
    do                                                                          \
    {                                                                           \
        switch ((int)__CommType)                                                \
        {                                                                       \
        case COMM_RS232C:                                                       \
            Serial_Send(__msg, __msgSize);                                      \
            break;                                                              \
        case COMM_TCP:                                                          \
            TCP_Send(__msg);                                                    \
            break;                                                              \
        case COMM_USB:                                                          \
            CLI_Send_USB(__msg, __msgSize);                                     \
            break;                                                              \
        default:                                                                \
            xprintf("Error: Unsupported communication type: %d\n", __CommType); \
            break;                                                              \
        }                                                                       \
    } while (0)

#define IS_USB(__useTCP) ((__useTCP) == COMM_USB)
#define IS_CMD_GET(__pd) ((__pd)->ParamCount == 0)
#define IS_CMD_PARAM_NONE(__pd) (IS_CMD_GET(__pd))
#define IS_CMD_HELP(__pd) ((__pd)->ParamCount >= 1 && (__pd)->Params[0].value._int == '?')


/************************************************************************* *
 ** @brief Command Handler                                                  *
 ****************************************************************************/
void Init_CommandHandling(void);
void Handle_command(const tsXParsedData *parsed_data, U08 useTC);
void Handle_command_by_string(const char *cmdStr, U08 useTCP);

void CMD_MakeCommandWithCRLF(char *dst, size_t dstSize, const char *src);
bool CMD_ShouldSkip_USBResponse(const tsXParsedData *parsedData);
/************************************************************************* */

/************************************************************************* */
/* module 등록용 */
extern const tsXCommandMapping gModuleCommandTable[];
extern const int gModuleCommandCount;
/************************************************************************* */

/* Command prototype */
void CMD_Handle_VERS(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_GERR(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_GERD(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_CLER(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_REBOOT(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_SetGetIP(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_FACTORY(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_RTC(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_DO(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_DI(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_LOADE(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_LOADF(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_LOADC(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_SAVEE(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_SAVEF(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_SAVEA(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_PrintParams(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_ContFullInfo(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_SetIP(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_TaskList(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_StackSize(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_DebugMode(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_GetSize(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_FWMode(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_HWTest(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_TaskTrigger(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_TimerOnOff(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_NoOperation(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_ClearScreen(const tsXParsedData *parsedData, U08 useTCP);
bool CMD_CheckParamAll_Int(const tsXParsedData *parsedData, int expectedCount);
bool CMD_CheckParamAll_Float(const tsXParsedData *parsedData, int expectedCount);
void CMD_Handle_Help_All(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_Help(const tsXParsedData *parsedData, U08 useTCP);
void CMD_ShowCommandHelp(const tsXParsedData *parsedData);

//=======================================================================

//@ USER CODE - START

//@ USER CODE - END
#endif /* _XCOMMAND_CORE_H_ */
