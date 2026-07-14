/*******************************************************************************
 * XDebug_User.h
 *
 *  Created on: 2026.01.02
 *      Author: RND. Kang PilSoon.
 *
 ******************************************************************************/
#ifndef __XDEBUG_USER_H__
#define __XDEBUG_USER_H__

#include "XGlobal.h"

#define CLI_LINE()      xprintf("\t========================================")
#define CLI_SECTION(t)  do { CLI_LINE(); xprintf("\t[%s]", (t)); } while(0)

// =============================================================================
#define STAGE_DEVELOPMENT  //<- 사용자가 선택

#ifdef STAGE_DEVELOPMENT
/**/ #define LOG_CONTINUE_FLAG
#endif
// =============================================================================

extern U08 isConnectedUSB;

// =============================================================================
// cli key handling functions
void CLI_HandleESCKey(void);
void CLI_HandleSpecialKeys(char c);
void CLI_HandleCtrl_PressedKeys(char c);
void CLI_HandleBacktickKey(void); //'`'

// =============================================================================

// =============================================================================
bool Debug_IsDebuggingFlagTrue(void);
// =============================================================================

#endif /* __XDEBUG_USER_H__ */
