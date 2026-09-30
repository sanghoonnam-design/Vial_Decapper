/*******************************************************************************
 * XErrorCode.h
 *
 *  Created on: 2024.10.15
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#ifndef XERRORCODE_H_
#define XERRORCODE_H_
#include "XGlobal.h"

/* 에러 심각도 정의 */
typedef enum
{
    /* 상위와의 소통용 */ //
    _INFO,                // 정보 전달용
    _WARNING,             // 경고용
    _CRITICAL,            // 에러 알람용
                          //
    /* 디버깅용 */        //
    _INTERNAL             // 내부 에러.

} teErrorSeverity;

/* 에러 코드 정의 */
typedef enum
{
    ERROR_CODE_NONE /*                     */ = 0,
    ERROR_CODE_DEFAULT /*                  */ = 0,

    ERROR_CODE_INVALID_COMMAND /*          */ = 2100,
    ERROR_CODE_INVALID_ARGUMENT /*         */ = 2110,

    /** @brief USER CODE BEGIN */
    ERROR_CODE_SYSTEM_MOVING_STATUS /*     */ = 2105,

    ERROR_CODE_ROBOT_DOOR_NOT_INIT /*      */ = 2106, // 초기화가 안되어 있음.
    ERROR_CODE_ROBOT_DOOR_TIMEOUT /*       */ = 2107, // 제어가 정상적이지 않고 timeout 이 발생했어.
    ERROR_CODE_ROBOT_DOOR_ALREADY_OPEN /*  */ = 2117, // 이미 문 열려있어 이놈아.
    ERROR_CODE_ROBOT_DOOR_ALREADY_CLOSE /* */ = 2118, // 이미 문 닫혀있어 이놈아.
    ERROR_CODE_ROBOT_DOOR_MOVING /*        */ = 2119, // 로봇 움직이고 있어.
    ERROR_CODE_ROBOT_DOOR_NOT_CLOSE /*     */ = 2120, // 로봇 door 닫히지 않음.
    ERROR_CODE_ROBOT_DOOR_NOT_OPEN /*      */ = 2121, // 로봇 door 열리지 않음.

    ERROR_CODE_CENT_BLOCKED_DOOR_OPEN /*   */ = 2130, // Safety interlock: door open blocks centrifuge
    // ERROR_CODE_CENT_RUNNING /*             */ = 2135, // centrifuge running

    ERROR_CODE_LESS_TOTAL_TIME /*          */ = 2220,
    ERROR_CODE_LESS_ACCEL_DECEL_TIME /*    */ = 2230,

    ERROR_CODE_REFRIGERATOR_TH1_ERROR /*   */ = 2410,
    ERROR_CODE_REFRIGERATOR_TH2_ERROR /*   */ = 2420,
    ERROR_CODE_REFRIGERATOR_NOT_COMM /*    */ = 2430,
    ERROR_CODE_REFRIGERATOR_NOT_COOLING /* */ = 2440,
    ERROR_CODE_REFRIGERATOR_NOT_INIT /*    */ = 2450,

    ERROR_CODE_A6_DRIVER_ERROR /*          */ = 2500,
    ERROR_CODE_A6_DRIVER_NOT_CONNECTED /*  */ = 2510,

    ERROR_CODE_A6_SERVO_TIMEOUT /*         */ = 2600,
    ERROR_CODE_A6_SERVO_NOT_INIT /*        */ = 2610,
    ERROR_CODE_A6_SERVO_NOT_HOME /*        */ = 2620,
    ERROR_CODE_A6_SERVO_OFF /*             */ = 2630,
    ERROR_CODE_A6_SERVO_ON_FAIL /*         */ = 2640,
    ERROR_CODE_A6_SERVO_OFF_FAIL /*        */ = 2650,
    ERROR_CODE_A6_SERVO_HOME_FAIL /*       */ = 2660,
    ERROR_CODE_A6_SERVO_JOG_FAIL /*        */ = 2670,
    ERROR_CODE_A6_SERVO_CENT_FAIL /*       */ = 2680,
    ERROR_CODE_A6_SERVO_SLOT_FAIL /*       */ = 2690,
    ERROR_CODE_A6_SERVO_MOVE_FAIL /*       */ = 2700,

    ERROR_CODE_DECAP_TIMEOUT = 2800,
    ERROR_CODE_DECAP_LIMIT = 2810,
    ERROR_CODE_DECAP_STATE = 2820,

    ERROR_CODE_ERROR_STATE_MACHINE /*      */ = 6000,
    ERROR_CODE_INVALID_SUBSTEP /*          */ = 6010,

    // ERROR_CODE_NO_REACH_TARGET_SPEED /*    */ = 2701

    /** @brief USER CODE END */

} teErrorCode;

#define ERROR_CODE_MAKE(__base, __moduleId) ((__base) + (__moduleId))
#define MODULE_ID_NONE (0x0)

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
    U08 moduleId;             // 모듈 몇 번째인가 (NONE 가능)
    teErrorCode Code;         // errorcode: integer type
    char strCode[8];          // errorcode: char type
    teErrorSeverity severity; // 에러 종류: 정보(INFO), 경고(WARNING), 에러(ERROR) 등...
    const char *message;      // 에러 description
                              //
    const char *funcName;     // 에러 발생 함수 이름
    int line;                 // 에러 발생 라인
} tsCurrentError;

extern tsCurrentError gCurrentError;

//==============================================================================
/** ERROR API  */
//==============================================================================
void InitErrorCode(void);

void ClearError(void);
int IsError(void);                         // 리턴: 에러상태 여부
int IsError_Critical(void);                // 리턴: 치명적 에러상태 여부
int IsCurrentError(teErrorCode errorCode); // 리턴: 현재 에러가 지정한 에러코드인지 여부

// Setters
void SetErrorCode(teErrorCode errorCode, const char *funcName, int line);                   // 공통, 단독모듈인 경우
void SetErrorCode_Id(teErrorCode errorCode, U08 module_Id, const char *funcName, int line); // 하나의 제어기에 여러 모듈이 붙은경우

// Getters
int GetErrorCode_int(void);
const char *GetErrorCode_char(void);                      // current error code in char format (e.g. "E1402")
const char *GetErrorMessage(void);                        // current error message (description)
const char *GetErrorMessageByCode(teErrorCode errorCode); // error message (description) for a given error code
teErrorSeverity GetErrorSeverity(void);
const char *GetErrorSeverity_char(teErrorSeverity sev);

// Debug / Monitor
void ErrorMonitor(void);
void ErrorMonitor_LED(void);
void xPrintError(int errorCode); // for debug

#endif /* XERRORCODE_H_ */