/*******************************************************************************
 * XCommandHandling.h
 *
 *  Created on: 2025.06.20
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#ifndef _XCOMMANDHANDLING_H_
#define _XCOMMANDHANDLING_H_

#include "XGlobal.h"
#include "XParser.h"
#include "XBuffer.h"
#include "_10_XNetworkMsg_Process.h"
#include "_10_XSerialCMD_Process.h"
#include "XDebug.h"

extern tsXBuffer *xSendMsg;

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

void Init_CommandHandling(void);

/************************************************************************* *
** @brief Command Handler                                                  *
****************************************************************************/
void Handle_command(const tsXParsedData *parsed_data, U08 useTC);
/************************************************************************* */

/* Command prototype */
void CMD_Handle_VERS(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_MODEL(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_GSTA(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_PSTA(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_GERR(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_GERD(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_CLER(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_SAVEE(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_SAVEF(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_SAVEA(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_LOADE(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_LOADF(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_LOADC(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_FACTORY(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_PrintParams(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_DI(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_DO(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_ContFullInfo(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_SetIP(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_TaskList(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_StackSize(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_REBOOT(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_DebugMode(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_GetSize(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_FWMode(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_HWTest(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_TaskTrigger(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_TimerOnOff(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_NoOperation(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_ClearScreen(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_Help_All(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_Help(const tsXParsedData *parsedData, U08 useTCP);
void CMD_ShowCommandHelp(const tsXParsedData *parsedData);

//=======================================================================

void CMD_Handle_Test_LongRun(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_ENABLE(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_DISABLE(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_SAVEA6(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_MRDO(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_ORG(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_HOME(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_CENT(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_MOVS(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_RESET(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_SERV(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_STOP(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_ESTOP(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_SASP(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_JOGS(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_GPOS(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_STIME(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_MOVA(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_MOVI(const tsXParsedData *parsedData, U08 useTCP);

//@ USER CODE - START


//@ USER CODE - END
#endif /* _XCOMMANDHANDLING_H_ */