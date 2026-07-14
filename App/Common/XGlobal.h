/*******************************************************************************
 * XGlobal.h
 *
 *  Created on: 2024.10.14
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#ifndef __XGLOBAL_H__
#define __XGLOBAL_H__

#include <stdio.h>

#include "project.h"
#include "XDefinition.h"
#include "XEnumeration.h"

#ifndef isDevelopmentStage
#define isDevelopmentStage
#endif

extern uint32_t gTick_LOG;
extern uint32_t gTick_UTT;
extern uint32_t gTick_USD;
extern uint32_t gTick_SDG;
extern uint32_t gTick_APC;
extern uint32_t gTick_SFZ;
extern uint32_t gTick_NWPT;
extern uint32_t gTick_SCPT;

/* task handle ===============================================================*/
extern TaskHandle_t gTaskHandle_LOG;  // Task_Handle for Program [L][O][G]o
extern TaskHandle_t gTaskHandle_UTT;  // ...for [U]padate [T]rigger [T]ime
extern TaskHandle_t gTaskHandle_USD;  // ...for [U]pdate [S]ignalBoard(ELMO+SSP) [D]ata
extern TaskHandle_t gTaskHandle_SDG;  // ...for [S]ignal [D]ia[G]nosis
extern TaskHandle_t gTaskHandle_APC;  // ...for [A][P]plication [C]ontrol
extern TaskHandle_t gTaskHandle_SFZ;  // ...for [S]ampling[F]inali[Z]ation
extern TaskHandle_t gTaskHandle_NWPT; // ...for [N]ewt[W]ork [P]ro[T]ocol
extern TaskHandle_t gTaskHandle_SCPT; // ...for [S]erial [C]ommand [P]ro[T]ocol
/*============================================================================*/

/* priority of task ==========================================================*/
// 우선순위: configMAX_PRIORITIES - 1
// ex) configMAX_PRIORITIES = 9 ==> 우선순위는 0 ~ 8
/** @note 수정하지 말것!~                                                      */

#define TP_STARTUP_LOGO /*          */ (1)
#define TP_UPDATE_TRIGGER_TIME /*  */ (8)
#define TP_UPDATE_SIG_DATA /*      */ (7)
#define TP_DIAGNOSE /*             */ (6)
#define TP_APPLICATION_CONTROL /*  */ (5)
#define TP_SAMPLING_FINALIZATION /**/ (4)
#define TP_UART_COMMAND_PROCESS /* */ (3) // while(1)
#define TP_TCP_COMMUNICATION /*    */ (3) // while(1) { if, message Queue }
#define TP_RS485 /*                */ (2) // message Queue
#define TP_RS232 /*                */ (2) // while(1)
#define TP_COMMAND_INTERFACE /*    */ (2) // RX, message Queue, cli
#define TP_MESSAGE_LOG_PRINT /*    */ (1) // TX, message Queue, cli, log

/*============================================================================*/

typedef enum
{
    COMM_NONE /*  */ = 0, // 통신 없음 / 미할당
    COMM_RS232C /**/ = 1,
    COMM_TCP /*   */ = 2,
    COMM_USB /*   */ = 3,
    COMM_COUNT
} teCommunicationType;

//==============================================================================
int Init_Global(int Index);

#define Delay(ms) HAL_Delay(ms)

void xDelay(uint32_t loopcnt);
void xDelay2(unsigned int delayValue);
void xDelay_u(uint32_t loopcnt);

// void UTIL_DelayMS(U16 wMS);
// void UTIL_DelayUS(U16 wUS);

void swap(float *a, float *b);
char asctohex(char value);

void __xuint8ToBit_Fomat4bit(unsigned char v);
void __xuint32ToBit(unsigned int v);
void __xuint32ToBit_Fomat4bit(unsigned int v);

char upper_ch(char str);
char lower_ch(char str);

int f2i10(float value);
int f2i100(float value);
int f2i1000(float value);

int isInt10(float value);
int isInt100(float value);
int isInt1000(float value);

int has_code(char *line, char chr);
const char *get_str(char *line, char chr);
int get_int(char *line, char chr);
uint32_t get_uint(char *line, char chr);
float get_float(char *linr, char chr);
uint32_t get_bool(char *line, char chr);

#endif /* XGLOBAL_H_ */
