/*******************************************************************************
 * XErrorCode.h
 *
 *  Created on: 2024.10.15
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#ifndef XERRORCODE_H_
#define XERRORCODE_H_

/* 에러 심각도 정의 */
typedef enum
{
    /* 상위와의 소통용 */
    _INFO,     // 정보 전달용
    _WARNING,  // 경고용
    _CRITICAL, // 에러 알람용
               //

    /* 디버깅용 */
    _INTERNAL_ERROR // 내부 에러

} teErrorSeverity;

/* 에러 코드 정의 */
typedef enum
{
    ERROR_CODE_NONE /*                     */ = 0,
    ERROR_CODE_DEFAULT /*                  */ = 0,

    ERROR_CODE_INVALID_COMMAND /*          */ = 2100,
    ERROR_CODE_INVALID_ARGUMENT /*         */ = 2101,

    /** @brief USER CODE BEGIN */
    ERROR_CODE_SYSTEM_MOVING_STATUS /*     */ = 2105,

    ERROR_CODE_ROBOT_DOOR_NOT_INIT /*      */ = 2106, // 초기화가 안되어 있음.
    ERROR_CODE_ROBOT_DOOR_TIMEOUT /*       */ = 2107, // 제어가 정상적이지 않고 timeout 이 발생했어.
    ERROR_CODE_ROBOT_DOOR_ALREADY_OPEN /*  */ = 2117, // 이미 문 열려있어 이놈아.
    ERROR_CODE_ROBOT_DOOR_ALREADY_CLOSE /* */ = 2118, // 이미 문 닫혀있어 이놈아.
    ERROR_CODE_ROBOT_DOOR_MOVING /*        */ = 2119, // 로봇 움직이고 있어.

    ERROR_CODE_CENT_BLOCKED_DOOR_OPEN /*   */ = 2130, // Safety interlock: door open blocks centrifuge
    // ERROR_CODE_CENT_RUNNING /*             */ = 2135, // centrifuge running

    ERROR_CODE_LESS_TOTAL_TIME /*          */ = 2202,
    ERROR_CODE_LESS_ACCEL_DECEL_TIME /*    */ = 2203,

    ERROR_CODE_REFRIGERATOR_TH1_ERROR /*   */ = 2401,
    ERROR_CODE_REFRIGERATOR_TH2_ERROR /*   */ = 2402,
    ERROR_CODE_REFRIGERATOR_NOT_COMM /*    */ = 2403,
    ERROR_CODE_REFRIGERATOR_NOT_COOLING /* */ = 2404,
    ERROR_CODE_REFRIGERATOR_NOT_INIT /*    */ = 2400,

    ERROR_CODE_A6_DRIVER_ERROR /*          */ = 2500,

    ERROR_CODE_A6_SERVO_NOT_INIT /*        */ = 2601,
    ERROR_CODE_A6_SERVO_NOT_HOME /*        */ = 2602,
    ERROR_CODE_A6_SERVO_OFF /*             */ = 2620,
    ERROR_CODE_A6_SERVO_ON_FAIL /*         */ = 2621,
    ERROR_CODE_A6_SERVO_OFF_FAIL /*        */ = 2622,
    ERROR_CODE_A6_SERVO_HOME_FAIL /*       */ = 2630,
    ERROR_CODE_A6_SERVO_JOG_FAIL /*        */ = 2631,
    ERROR_CODE_A6_SERVO_CENT_FAIL /*       */ = 2632,
    ERROR_CODE_A6_SERVO_SLOT_FAIL /*       */ = 2633,
    ERROR_CODE_A6_SERVO_MOVE_FAIL /*       */ = 2634,

    // ERROR_CODE_NO_REACH_TARGET_SPEED /*    */ = 2701

    /** @brief USER CODE END */

} teErrorCode;

/* 에러 정의를 위한 구조체 */
typedef struct
{
    teErrorCode ErrorCode;    // errorcode: integer
    teErrorSeverity severity; //
    const char *message;      //
} tsErrorEntry;

/* 현재 에러를 다루는 구조체 */
typedef struct
{
    teErrorCode Code;         // errorcode: integer type
    char strCode[7];          // errorcode: char type
    teErrorSeverity severity; // 에러 종류: 정보(INFO), 경고(WARNING), 에러(ERROR)
    const char *message;      // 에러 description
} tsCurrentError;
extern tsCurrentError gCurrentError;

//==============================================================================
void InitErrorCode(void);

void ClearError(void);
int IsError(void);
void SetErrorCode(teErrorCode errorCode);
int GetErrorCode_int(void);
const char *GetErrorCode_char(void);
const char *GetErrorMessage(void);
const char* GetErrorSeverity_char(teErrorSeverity sev);

void ErrorMonitor(void);

void xPrintError(int errorCode); // for debug

#endif /* XERRORCODE_H_ */