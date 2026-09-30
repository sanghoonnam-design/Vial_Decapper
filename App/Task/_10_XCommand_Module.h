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
/** @brief GSTA/ST 응답에 Busy, Enable, Homed, Error와 오류 코드를 순서대로 기록한다. */
void CMD_Handle_GSTA(const tsXParsedData *parsedData, U08 useTCP);

/** @brief SPD/SPEED를 처리한다. 인자 없으면 현재 비율 조회, 정수 1~100이면 비율 설정 후 응답한다. */
void CMD_Handle_SPEED(const tsXParsedData *parsedData, U08 useTCP);
/** @brief 인자 없는 HOME을 검사하고 원점 탐색 요청을 등록한다. */
void CMD_Handle_HOME(const tsXParsedData *parsedData, U08 useTCP);

/** @brief 인자 없는 STOP을 독립 정지 요청으로 등록한다. */
void CMD_Handle_STOP(const tsXParsedData *parsedData, U08 useTCP);
/** @brief Busy일 때 일시정지 요청을 등록한다. */
void CMD_Handle_PAUSE(const tsXParsedData *parsedData, U08 useTCP);
/** @brief Busy일 때 재개 요청을 등록한다. */
void CMD_Handle_RESUME(const tsXParsedData *parsedData, U08 useTCP);

/** @brief 인자 없는 자동 DECAP 요청을 대기·오류 상태 확인 후 등록한다. */
void CMD_Handle_DECAP(const tsXParsedData *parsedData, U08 useTCP);
/** @brief 인자 없는 자동 CAP 요청을 대기·오류 상태 확인 후 등록한다. */
void CMD_Handle_CAP(const tsXParsedData *parsedData, U08 useTCP);

//Debug Code
/** @brief SL 명령으로 공통 상태와 Decapper 센서·모터·오류 상태를 진단 출력한다. */
void CMD_Handle_Print_SL(const tsXParsedData *parsedData, U08 useTCP);
/** @brief CD 명령으로 Decapper 목표·속도 비율·롱런 및 CT 그리퍼 요청값을 출력한다. */
void CMD_Handle_Print_CD(const tsXParsedData *parsedData, U08 useTCP);
/** @brief PL 조회, USB 도움말, 두 인자 설정을 분기한다. */
void CMD_Handle_Print_PL(const tsXParsedData *parsedData, U08 useTCP);

/** @brief ORG 요청을 등록하여 기존 HOME 좌표계의 Z=0 및 Y High 복귀를 실행하게 한다. */
void CMD_Handle_ORIGIN(const tsXParsedData *parsedData, U08 useTCP);
/** @brief MOVE의 상대/절대 모드(R/A), 축(Z/R), pulse 값을 확인하여 CD에 저장하고 요청한다. */
void CMD_Handle_MOVE(const tsXParsedData *parsedData, U08 useTCP);
/** @brief READY 0은 공압 Y High, READY 1은 Y Low 이동을 요청한다. */
void CMD_Handle_READY(const tsXParsedData *parsedData, U08 useTCP);

/** @brief Y/Body 자동 준비 과정을 생략하는 유닛 DECAP 시험 요청을 등록한다. */
void CMD_Handle_UDECAP(const tsXParsedData *parsedData, U08 useTCP);
/** @brief Y/Body 자동 준비 과정을 생략하는 유닛 CAP 시험 요청을 등록한다. */
void CMD_Handle_UCAP(const tsXParsedData *parsedData, U08 useTCP);
/** @brief LR/CAPDECAPLR 요청으로 유닛 DECAP/CAP 반복 시험을 시작하게 한다. */
void CMD_Handle_LongRun(const tsXParsedData *parsedData, U08 useTCP);

/** @brief BGRIP 0/1을 Body 그리퍼 요청값으로 저장하고 수동 출력 액션을 등록한다. */
void CMD_Handle_BGRIP(const tsXParsedData *parsedData, U08 useTCP);
/** @brief CGRIP 0/1을 Cap 그리퍼 요청값으로 저장하고 수동 출력 액션을 등록한다. */
void CMD_Handle_CGRIP(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_DO(const tsXParsedData *parsedData, U08 useTCP);
void CMD_Handle_DI(const tsXParsedData *parsedData, U08 useTCP);

/** @brief 현재 Z pulse를 조회하고 ZCap_UpPos 저장 후보로 보관한다. */
void CMD_Handle_RPOS(const tsXParsedData *parsedData, U08 useTCP);

//Unused Command
/** @brief 공통 Busy·오류 상태를 진단 출력하는 미등록 디버그 핸들러이다. */
void CMD_Handle_PSTA(const tsXParsedData *parsedData, U08 useTCP);

#endif /* _XCOMMAND_MODULE_H_ */
