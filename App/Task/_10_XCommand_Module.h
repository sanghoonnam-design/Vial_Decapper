/*******************************************************************************
 * XCommand_Module.h
 *
 *  Created on: 2026.02.14
 *      Author: RND. Kang PilSoon.
 *     
 * @brief
 *            1. 
 ******************************************************************************/
#ifndef _XCOMMAND_MODULE_H_
#define _XCOMMAND_MODULE_H_

#include "XGlobal.h"
#include "_10_XCommand_Core.h"

extern const tsXCommandMapping gModuleCommandTable[];
extern const int gModuleCommandCount;

//=======================================================================
//@ USER CODE - START
void CMD_Handle_GSTA(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_PSTA(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_MODEL(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_Print_SL(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_Print_CD(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_Print_PL(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_Print_SBSStateFlags(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_LongRun(const tsXParsedData *parsedData, U08 useTCP);
//=======================================================================
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


//@ USER CODE - END

#endif /* _XCOMMAND_MODULE_H_ */
