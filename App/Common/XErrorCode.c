/*******************************************************************************
 * XErrorCode.c
 *
 *  Created on: 2024.10.15
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#include "XErrorCode.h"
#include "XSystemInfo.h"
#include "XDebug.h"

#include <stdio.h>

/**
 * @brief 일단, 에러에 대한 정의를 한다.
 */
const tsErrorEntry gErrorTable[] =
    {
        /* error code */ /* severity */ /* error message */
        /*---------------------------------------------------------------------------------------------*/
        {ERROR_CODE_NONE, /*                     */ _INFO, /*     */ "No Error."},
        {ERROR_CODE_DEFAULT, /*                  */ _INFO, /*     */ "No Error."},

        {ERROR_CODE_INVALID_COMMAND, /*          */ _WARNING, /*  */ "Invalid command."},
        {ERROR_CODE_INVALID_ARGUMENT, /*         */ _WARNING, /*  */ "The input parameter is invalid."},

        //==================================================================
        // USER DEFINED ERROR CODES
        //==================================================================
        {ERROR_CODE_SYSTEM_MOVING_STATUS, /*     */ _WARNING, /* */ "Centrifuge is running."},

        {ERROR_CODE_ROBOT_DOOR_NOT_INIT, /*      */ _CRITICAL, /* */ "Robot door is not initialized."},
        {ERROR_CODE_ROBOT_DOOR_TIMEOUT, /*       */ _CRITICAL, /* */ "Robot door operation timed out."},
        {ERROR_CODE_ROBOT_DOOR_ALREADY_OPEN, /*  */ _WARNING, /*  */ "The robot door is already open."},
        {ERROR_CODE_ROBOT_DOOR_ALREADY_CLOSE, /* */ _WARNING, /*  */ "The robot door is already closed."},
        {ERROR_CODE_ROBOT_DOOR_MOVING, /*        */ _WARNING, /*  */ "The robot door is moving."},
        {ERROR_CODE_ROBOT_DOOR_NOT_CLOSE, /*     */ _WARNING, /*  */ "The robot door is not closed."},
        {ERROR_CODE_ROBOT_DOOR_NOT_OPEN, /*      */ _WARNING, /*  */ "The robot door is not open."},

        {ERROR_CODE_CENT_BLOCKED_DOOR_OPEN, /*   */ _CRITICAL, /* */ "Centrifuge blocked: Robot door is open."},
        // {ERROR_CODE_CENT_RUNNING, /*             */ _CRITICAL, /* */ "Centrifuge is running."},

        {ERROR_CODE_LESS_TOTAL_TIME, /*          */ _CRITICAL, /* */ "Total time is too short."},
        {ERROR_CODE_LESS_ACCEL_DECEL_TIME, /*    */ _CRITICAL, /* */ "Acceleration or deceleration time is too short."},

        {ERROR_CODE_REFRIGERATOR_TH1_ERROR, /*   */ _CRITICAL, /* */ "Refrigerator sensor 1 failure."},
        {ERROR_CODE_REFRIGERATOR_TH2_ERROR, /*   */ _CRITICAL, /* */ "Refrigerator sensor 2 failure."},
        {ERROR_CODE_REFRIGERATOR_NOT_COMM, /*    */ _CRITICAL, /* */ "Refrigerator communication error."},
        {ERROR_CODE_REFRIGERATOR_NOT_COOLING, /* */ _CRITICAL, /* */ "Refrigerator not cooling properly."},
        {ERROR_CODE_REFRIGERATOR_NOT_INIT, /*    */ _CRITICAL, /* */ "Refrigerator not initialized."},

        {ERROR_CODE_A6_DRIVER_ERROR, /*          */ _CRITICAL, /* */ "Servo driver error detected."},
        {ERROR_CODE_A6_DRIVER_NOT_CONNECTED, /*  */ _CRITICAL, /* */ "Servo driver not connected."},

        {ERROR_CODE_A6_SERVO_TIMEOUT, /*         */ _CRITICAL, /* */ "Servo driver operation timed out."},
        {ERROR_CODE_A6_SERVO_NOT_INIT, /*        */ _CRITICAL, /* */ "Servo driver: not initialized."},
        {ERROR_CODE_A6_SERVO_NOT_HOME, /*        */ _CRITICAL, /* */ "Servo driver: not homed."},
        {ERROR_CODE_A6_SERVO_OFF, /*             */ _CRITICAL, /* */ "Servo driver is OFF."},
        {ERROR_CODE_A6_SERVO_ON_FAIL, /*         */ _CRITICAL, /* */ "Servo ON failed."},
        {ERROR_CODE_A6_SERVO_OFF_FAIL, /*        */ _CRITICAL, /* */ "Servo OFF failed."},
        {ERROR_CODE_A6_SERVO_HOME_FAIL, /*       */ _CRITICAL, /* */ "[Home] command failed."},
        {ERROR_CODE_A6_SERVO_JOG_FAIL, /*        */ _CRITICAL, /* */ "[Jog] command failed."},
        {ERROR_CODE_A6_SERVO_CENT_FAIL, /*       */ _CRITICAL, /* */ "[Cent] command failed."},
        {ERROR_CODE_A6_SERVO_SLOT_FAIL, /*       */ _CRITICAL, /* */ "[Slot] command failed."},
        {ERROR_CODE_A6_SERVO_MOVE_FAIL, /*       */ _CRITICAL, /* */ "movement failed."},

        {ERROR_CODE_ERROR_STATE_MACHINE, /*      */ _INTERNAL, /*  */ "State-machine error."},
        {ERROR_CODE_INVALID_SUBSTEP, /*          */ _INTERNAL, /*  */ "Invalid sub-step."},


        // {ERROR_CODE_NO_REACH_TARGET_SPEED, /*    */ _CRITICAL, /* */ "Target speed (RPM) not reached."},

        //==================================================================
        /** @brief USER CODE END */
        //==================================================================
        {(teErrorCode)9999, (teErrorSeverity)0, 0}};

static const char *ErrorSeverityStr[] = {
    "INFO",     // _INFO
    "WARNING",  // _WARNING
    "CRITICAL", // _CRITICAL
    "INTERNAL"  // _INTERNAL_ERROR
};

static void _MakeErrorCode_2_String(char *buf, U16 code);
static void _SetError_Internal(teErrorCode code, U08 moduleId, const char *funcName, int line);

// 현재 에러 상태
tsCurrentError gCurrentError;

static void _MakeErrorCode_2_String(char *buf, U16 code)
{ // sprintf() 대체용
    static const char digit[] = "0123456789";

    buf[0] = 'E';
    buf[1] = digit[(code / 1000) % 10];
    buf[2] = digit[(code / 100) % 10];
    buf[3] = digit[(code / 10) % 10];
    buf[4] = digit[code % 10];
    buf[5] = '\0';
}

#if 0
static void _SetErrorInternal(teErrorCode code, U08 moduleId)
{
    const tsErrorEntry *entry = gErrorTable;

    while (entry->ErrorCode != 9999)
    {
        if (entry->ErrorCode == code)
        {
            gCurrentError.Code = code;
            gCurrentError.moduleId = moduleId;

            sprintf(gCurrentError.strCode, "E%04d",
                    ERROR_CODE_MAKE(code, moduleId));

            gCurrentError.severity = entry->severity;
            gCurrentError.message = entry->message;

            xSystemInfo.cd_ErrorCount++;
            return;
        }
        entry++;
    }

    /* fallback */
    gCurrentError.Code = 9999;
    gCurrentError.moduleId = MODULE_ID_NONE;
    strcpy(gCurrentError.strCode, "E9999");
    gCurrentError.severity = _INTERNAL;
    gCurrentError.message = "Unknown error";
}
#else
static void _SetError_Internal(teErrorCode code, U08 moduleId, const char *funcName, int line)
{
    const tsErrorEntry *entry = gErrorTable;

    while (entry->ErrorCode != 9999)
    {
        if (entry->ErrorCode == code)
        {
            gCurrentError.moduleId = moduleId;
            gCurrentError.Code = ERROR_CODE_MAKE(code, moduleId);
            
            _MakeErrorCode_2_String(
                gCurrentError.strCode,
                gCurrentError.Code);

            gCurrentError.severity = entry->severity;
            gCurrentError.message = entry->message;

            gCurrentError.funcName = funcName;
            gCurrentError.line = line;

            xSystemInfo.cd_ErrorCount++;

            ErrorMonitor();
            return;
        }
        entry++;
    }

    /* fallback */
    gCurrentError.Code = 9999;
    gCurrentError.moduleId = MODULE_ID_NONE;
    strcpy(gCurrentError.strCode, "E9999");
    gCurrentError.severity = _INTERNAL;
    gCurrentError.message = "Unknown error";
}
#endif

void InitErrorCode(void)
{
    gCurrentError.moduleId = MODULE_ID_NONE;
    gCurrentError.Code = ERROR_CODE_NONE;
    sprintf(gCurrentError.strCode, "E0000");
    gCurrentError.severity = _INFO;
    gCurrentError.message = "No Error.";
}

void ClearError(void)
{
    gCurrentError.moduleId = MODULE_ID_NONE;
    gCurrentError.Code = ERROR_CODE_NONE;
    sprintf(gCurrentError.strCode, "E0000");
    gCurrentError.severity = _INFO;
    gCurrentError.message = "No Error.";
}

int IsError(void)
{
    int code = GetErrorCode_int();

    return (code != ERROR_CODE_NONE) ? YES : NO;
}

int IsError_Critical(void)
{
    int code = GetErrorCode_int();
    int severity = (int)GetErrorSeverity();
    // ERROR_CODE_COMMAND_ENQUEUE_FAIL 는 예외 처리
    return (code != ERROR_CODE_NONE && severity == _CRITICAL) ? YES : NO;
}

int IsCurrentError(teErrorCode errorCode)
{
    int cur = (gCurrentError.Code / 10) * 10; // 1의 자리 제거, 에러발생 모듈 ID 제거

    return (cur == errorCode) ? YES : NO;
}

void SetErrorCode(teErrorCode errorCode, const char *funcName, int line)
{
    _SetError_Internal(errorCode, MODULE_ID_NONE, funcName, line);
}

void SetErrorCode_Id(teErrorCode errorCode, U08 moduleId, const char *funcName, int line)
{
    _SetError_Internal(errorCode, moduleId, funcName, line);
}

int GetErrorCode_int(void)
{
    return (int)gCurrentError.Code;
}

const char *GetErrorCode_char(void)
{
    return gCurrentError.strCode;
}

const char *GetErrorMessage(void)
{
    return gCurrentError.message;
}

const char *GetErrorMessageByCode(teErrorCode errorCode)
{
    for (int i = 0; i < ARRAY_SIZE(gErrorTable); i++)
    {
        if (gErrorTable[i].ErrorCode == errorCode)
        {
            return gErrorTable[i].message;
        }
    }

    return "Unknown error";
}

teErrorSeverity GetErrorSeverity(void)
{
    return gCurrentError.severity;
}

const char *GetErrorSeverity_char(teErrorSeverity sev)
{
    if (sev < 0 || sev > _INTERNAL) // internal error
        return "UNKNOWN";
    return ErrorSeverityStr[sev];
}

void ErrorMonitor(void)
{
    static int old_ErrorCode = ERROR_CODE_NONE;
    int errorCode;

    errorCode = gCurrentError.Code;

    if (errorCode != ERROR_CODE_NONE && old_ErrorCode != errorCode)
    {
        xPrintError(errorCode);
    }

#if 0
    if (gCurrentError.severity == _INFO || gCurrentError.severity == _WARNING)
    {
        SetErrorCode(ERROR_CODE_NONE);

        errorCode = gCurrentError.Code;
    }
    else // _CRITICAL
    {
        // TODO: USER CODE
        // _CRITICAL : 이면.. 에러 유지
        //
    }
#endif

    old_ErrorCode = errorCode;
}

void ErrorMonitor_LED(void)
{
    if(gCurrentError.Code != ERROR_CODE_NONE)
    {
        // LED_OnOff(LED_FAULT, ON);
    }
    else
    {
        // LED_OnOff(LED_FAULT, OFF);
    }
}

// 에러 메시지를 출력하는 함수 : 디버깅용
void xPrintError(int errorCode)
{
    int baseErrorCode = (errorCode / 10) * 10; // 1의 자리 제거(모듈 ID 제거)

    for (int i = 0; gErrorTable[i].ErrorCode != 9999; i++)
    {
        if (gErrorTable[i].ErrorCode == baseErrorCode)
        {
            ERR_MSG_SEND("ErrorCode: %d, Severity: " ANSI_TX_LightYellow "%s" ANSI_TX_ORG ", Msg: %s, %s(%d)",
                         errorCode, // gErrorTable[i].ErrorCode,
                         //  xParsedData_Network.Command, // TODO: usb는 어쩔껴?
                         GetErrorSeverity_char(gErrorTable[i].severity),
                         gErrorTable[i].message,
                         gCurrentError.funcName,
                         gCurrentError.line);
            return;
        }
    }

    LOG_MSG_SEND("Unknown Error Code: %d\n", baseErrorCode);
}
