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
extern bool gZCapUpPosSavePending;
extern S32 gZCapUpPosPendingValue;

typedef enum
{
    /* --- Diagnose (PL Header) --- */
    RPL_SET_DG_CPU_TEMP_OVERHEAT = 1,
    RPL_SET_DG_CPU_TEMP_ALARM_INTERVAL,
    RPL_SET_DG_IS_DIAGNOSIS_ENABLED,
    RPL_SET_DG_CLIENT2HOST_LOGMODE,

    /* --- Robot Door --- */
    RPL_SET_DOOR_DIRECTION,
    RPL_SET_DOOR_CONTROL_TIMEOUT,
    RPL_SET_DOOR_CLOSE_OVERTIME,
    RPL_SET_DOOR_OPEN_OVERTIME,
    RPL_SET_DOOR_RELATIVE_DISTANCE,
    RPL_SET_DOOR_MOTOR_SPEED,
    RPL_SET_DOOR_MOTOR_ACCEL,
    RPL_SET_DOOR_MOTOR_NORMAL_CURRENT,
    RPL_SET_DOOR_MOTOR_HOLDING_CURRENT,
    RPL_SET_DOOR_MOTOR_RESOLUTION,

    /* --- Servo A6 --- */
    RPL_SET_SERVO_DIRECTION,
    RPL_SET_SERVO_PULSE_PER_REV,
    RPL_SET_SERVO_HOME_FWD_SPEED,
    RPL_SET_SERVO_HOME_BWD_SPEED,
    RPL_SET_SERVO_HOME_ACCEL,
    RPL_SET_SERVO_HOME_OFFSET,
    RPL_SET_SERVO_SLOT_SPEED,
    RPL_SET_SERVO_SLOT_ACCEL,
    RPL_SET_SERVO_SLOT_DECEL,
    RPL_SET_SERVO_SLOT_POSITION_OFFSET,                                        /* slot 1 offset, 이후 SLOT_COUNT 개 index 를 slot 1~6 로 연속 사용 */
    RPL_SET_SERVO_JOG_SPEED = RPL_SET_SERVO_SLOT_POSITION_OFFSET + SLOT_COUNT, /* slot 2~6 index 확보 후 다음 */
    RPL_SET_SERVO_JOG_ACCEL,
    RPL_SET_SERVO_JOG_DECEL,
    RPL_SET_SERVO_BASE_SPEED,
    RPL_SET_SERVO_BASE_ACCEL,
    RPL_SET_SERVO_BASE_DECEL,

    /* --- Vial Decapper --- */
    RPL_SET_DECAP_RUN_CUR_Z,
    RPL_SET_DECAP_RUN_CUR_R,
    RPL_SET_DECAP_SEL_MAX_CUR_Z,
    RPL_SET_DECAP_SEL_MAX_CUR_R,
    RPL_SET_DECAP_STOP_CUR_RATE_Z,
    RPL_SET_DECAP_STOP_CUR_RATE_R,
    RPL_SET_DECAP_STEP_RESOLUTION,
    RPL_SET_DECAP_LIMIT_POS_Z,
    RPL_SET_DECAP_LIMIT_POS_R,
    RPL_SET_DECAP_SW_NEG_LIMIT_Z,
    RPL_SET_DECAP_SW_POS_LIMIT_Z,
    RPL_SET_DECAP_SW_NEG_LIMIT_R,
    RPL_SET_DECAP_SW_POS_LIMIT_R,
    RPL_SET_DECAP_SOFT_LIMIT_ENABLE,
    RPL_SET_DECAP_Z_ACC,
    RPL_SET_DECAP_Z_VEL,
    RPL_SET_DECAP_Z_CAP_UP_POS,
    RPL_SET_DECAP_Z_CAP_SIDE_POS,
    RPL_SET_DECAP_Z_ORIGIN_POS,
    RPL_SET_DECAP_R_ACC,
    RPL_SET_DECAP_R_VEL,
    RPL_SET_DECAP_R_POS,

} teRPL_SetIndex;


//=======================================================================
//User Code
void CMD_Handle_GSTA(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_SPEED(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_HOME(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_STOP(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_PAUSE(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_RESUME(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_DECAP(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_CAP(const tsXParsedData *parsedData, U08 useTCP);

//Debug Code
void CMD_Handle_Print_SL(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_Print_CD(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_Print_PL(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_ORIGIN(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_MOVE(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_READY(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_UDECAP(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_UCAP(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_LongRun(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_BGRIP(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_CGRIP(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_DO(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_DI(const tsXParsedData *parsedData, U08 useTCP);

void CMD_Handle_RPOS(const tsXParsedData *parsedData, U08 useTCP);

//Unused Command
void CMD_Handle_PSTA(const tsXParsedData *parsedData, U08 useTCP);

#endif /* _XCOMMAND_MODULE_H_ */
