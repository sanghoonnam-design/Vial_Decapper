/** ****************************************************************************
 * XCommand_Module.c
 *
 * Created on: 2026.02.14
 * Author    : RND. Kang PilSoon.
 * @brief
 *             1. 상위 client 명령 실행함수 처리 모듈
 * @note
 *             1. 최대 USB 명령, RS232/485, TCP/UDP 통신 모듈이 공통으로 사용하는 구조로
 *                설계됨.
 ******************************************************************************/
#include "XSystemInfo.h"
#include "_10_XCommand_Module.h"
#include "XSystem_DB.h"
#include "_04_XDiagnose.h"

// #include "_04_XDiagnose_Def.h"

const tsXCommandMapping gModuleCommandTable[] =
    {
        /** @note USER CODE BEGIN */

        /*User Command*/
        {0, "GSTA", /*      */ CMD_Handle_GSTA, /*            */ "Get the system State.", /*            */ "GSTA"},     // 시스템 상태 반환
        {0, "ST", /*        */ CMD_Handle_GSTA, /*            */ "Get the system State.", /*            */ "ST"},       // 시스템 상태 반환

        /** @note 각 장비에 해당하는 명령어 */
		{0, "SPD", /*       */ CMD_Handle_SPEED, /*           */ "Get/set motor speed percent.", /*		*/ "SPD [1~100]"},
		{0, "SPEED", /*     */ CMD_Handle_SPEED, /*           */ "Get/set motor speed percent.", /*    	*/ "SPEED [1~100]"},

		{0, "HOME", /*      */ CMD_Handle_HOME, /*            */ "Run homing sequence.", /*         	*/ "HOME"}, // 원심분리기 축 및 도어 초기 위치 복귀 (Homing)

		{0, "STOP", /*      */ CMD_Handle_STOP, /*            */ "Stop all Decapper motion.", /*    	*/ "STOP"}, // 원심분리기 축 및 도어 초기 위치 복귀 (Homing)
		{0, "PAUSE", /*     */ CMD_Handle_PAUSE, /*           */ "Pause current operation.", /*     	*/ "PAUSE"}, // 원심분리기 축 및 도어 초기 위치 복귀 (Homing)
		{0, "RESUME", /*    */ CMD_Handle_RESUME, /*          */ "Resume paused operation.", /*     	*/ "RESUME"}, // 원심분리기 축 및 도어 초기 위치 복귀 (Homing)

		{0, "DECAP", /*     */ CMD_Handle_DECAP, /*           */ "Run automatic decapping.", /*     	*/ "DECAP"}, // 원심분리기 축 및 도어 초기 위치 복귀 (Homing)
		{0, "CAP", /*       */ CMD_Handle_CAP, /*             */ "Run automatic capping.", /*       	*/ "CAP"}, // 원심분리기 축 및 도어 초기 위치 복귀 (Homing)

		/*Debug Command*/
		{1, "SL", /*        */ CMD_Handle_Print_SL, /*        */ "Print SL.", /*                        */ "SL"},       // SL 출력
		{1, "CD", /*        */ CMD_Handle_Print_CD, /*        */ "Print CD.", /*                        */ "CD"},       // CD 출력
		{1, "PL", /*        */ CMD_Handle_Print_PL, /*        */ "Print PL.", /*                        */ "PL <...>"}, // PL 출력

		{1, "ORG", /*       */ CMD_Handle_ORIGIN, /*          */ "Move to origin position.", /*     	*/ "ORG"},  // 원심분리기 축 및 도어 초기 위치 복귀 (Homing)
		{1, "MOVE", /*      */ CMD_Handle_MOVE, /*            */ "Move Z/R axis.", /*               	*/ "MOVE <R|A> <Z|R> <pulse>"}, // Z/R 축 상대/절대 위치 이동
		{1, "READY", /*     */ CMD_Handle_READY, /*            */ "Move Y axis to limit.", /*          	*/ "READY <0|1>"}, // 0: Y High, 1: Y Low

		{1, "UDECAP", /*    */ CMD_Handle_UDECAP, /*          */ "Run unit decapping.", /*          	*/ "UDECAP"}, // 서보/스텝모터 절대 위치 이동 명령 (deg 또는 step 기준)
		{1, "UCAP", /*      */ CMD_Handle_UCAP, /*            */ "Run unit capping.", /*            	*/ "UCAP"}, // 서보/스텝모터 절대 위치 이동 명령 (deg 또는 step 기준)

		{1, "CAPDECAPLR", /**/ CMD_Handle_LongRun, /*         */ "Run long-run test.", /*           	*/ "CAPDECAPLR"}, // 롱런 테스트
		{1, "LR", /*        */ CMD_Handle_LongRun, /*         */ "Run long-run test.", /*           	*/ "LR"}, // 롱런 테스트

		{1, "BGRIP", /*     */ CMD_Handle_BGRIP, /*           */ "Set body gripper output.", /*     	*/ "BGRIP <0|1>"}, // 서보/스텝모터 절대 위치 이동 명령 (deg 또는 step 기준)
		{1, "CGRIP", /*     */ CMD_Handle_CGRIP, /*           */ "Set cap gripper output.", /*      	*/ "CGRIP <0|1>"}, // 서보/스텝모터 절대 위치 이동 명령 (deg 또는 step 기준)
		{1, "RPOS", /*		*/ CMD_Handle_RPOS, /*            */ "Read current Z position.", /*      	*/ "RPOS"}, // 서보/스텝모터 절대 위치 이동 명령 (deg 또는 step 기준)

        /** @note USER CODE END */
};

const int gModuleCommandCount = sizeof(gModuleCommandTable) / sizeof(tsXCommandMapping);

static int GetParamInt(const tsXParsedData *parsedData, int index, int *out);
static int GetParamFloat(const tsXParsedData *parsedData, int index, float *out);
static bool CMD_RejectIfDecapperBusy(void);

/*==============================================================================
 * Local helpers
 *============================================================================*/
static bool CMD_RejectIfDecapperBusy(void)
{
    if ((xSL.isBusy == YES) || (xAT != ACTION_NONE))
    {
        XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);
        return true;
    }

    return false;
}

__attribute__((unused)) static int GetParamInt(const tsXParsedData *parsedData, int index, int *out)
{
    if (parsedData == NULL || out == NULL)
        return NO;

    if (index < 0 || index >= parsedData->ParamCount)
        return NO;

    if (parsedData->Params[index].type == PARAM_TYPE_INT)
    {
        *out = parsedData->Params[index].value._int;
        return YES;
    }

    if (parsedData->Params[index].type == PARAM_TYPE_FLOAT)
    {
        *out = (int)parsedData->Params[index].value._float;
        return YES;
    }

    return NO;
}

__attribute__((unused)) static int GetParamFloat(const tsXParsedData *parsedData, int index, float *out)
{
    if (parsedData == NULL || out == NULL)
        return NO;

    if (index < 0 || index >= parsedData->ParamCount)
        return NO;

    if (parsedData->Params[index].type == PARAM_TYPE_FLOAT)
    {
        *out = parsedData->Params[index].value._float;
        return YES;
    }

    if (parsedData->Params[index].type == PARAM_TYPE_INT)
    {
        *out = (float)parsedData->Params[index].value._int;
        return YES;
    }

    return NO;
}

//======================================================================================
// @USER CODE START
//======================================================================================

/*User Command*/
void CMD_Handle_GSTA(const tsXParsedData *parsedData, U08 useTCP){
    if (parsedData->ParamCount == 0)
    {
        //  ==============================================================================
        // default info.
        /* [01] */ XBuffer_AddInt(xSendMsg, xSL.isBusy, COMMA); //= (!xServoA6.IsStop() && xDoor.IsStop())
        /* [02] */ XBuffer_AddInt(xSendMsg, xSL.isEnable, COMMA);
        /* [03] */ XBuffer_AddInt(xSendMsg, xSL.isHomed, COMMA);
        /* [04] */ XBuffer_AddInt(xSendMsg, xSL.isError, COMMA);
        /* [05] */ XBuffer_AddString(xSendMsg, GetErrorCode_char(), COMMA);
        //  ==============================================================================
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);

        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);

        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_SPEED(const tsXParsedData *parsedData, U08 useTCP){
    if (parsedData->ParamCount == 0)
    {
		XBuffer_AddCommandString(xSendMsg, "SPEED", NO_COMMA);
		XBuffer_AddInt(xSendMsg, (int)CDecap_GetSpeedPercent(), NO_COMMA);
    }
	else if ((parsedData->ParamCount == 1) &&
			 (parsedData->Params[0].type == PARAM_TYPE_INT) &&
			 (parsedData->Params[0].value._int >= 1) &&
			 (parsedData->Params[0].value._int <= 100))
	{
		CDecap_SetSpeedPercent((U08)parsedData->Params[0].value._int);
		XBuffer_AddCommandString(xSendMsg, "SPEED", NO_COMMA);
		XBuffer_AddInt(xSendMsg, (int)CDecap_GetSpeedPercent(), NO_COMMA);
	}
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
		if ((parsedData->ParamCount > 0) &&
			(parsedData->Params[0].value._int != '?'))
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_HOME(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (CMD_RejectIfDecapperBusy()) return;

        xAT = ACTION_HOME;
        XBuffer_AddString(xSendMsg, "home", NO_COMMA);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_STOP(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        xAT = ACTION_STOP;
        XBuffer_AddString(xSendMsg, "stop", NO_COMMA);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_PAUSE(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (xSL.isBusy == YES)
        {
            xAT = ACTION_PAUSE;
            XBuffer_AddString(xSendMsg, "pause", NO_COMMA);
        }
        else
        {
            XBuffer_AddString(xSendMsg, "isbusy = no -> not pause", NO_COMMA);
        }
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_RESUME(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (xSL.isBusy == YES)
        {
            xAT = ACTION_RESUME;
            XBuffer_AddString(xSendMsg, "resume", NO_COMMA);
        }
        else
        {
            XBuffer_AddString(xSendMsg, "isbusy = no -> not pause", NO_COMMA);
        }
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_DECAP(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (CMD_RejectIfDecapperBusy())
            return;

        xAT = ACTION_DECAP;
        XBuffer_AddString(xSendMsg, "Decap", NO_COMMA);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_CAP(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (CMD_RejectIfDecapperBusy())
            return;

        xAT = ACTION_CAP;
        XBuffer_AddString(xSendMsg, "Cap", NO_COMMA);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}


/*Debug Command*/
void CMD_Handle_Print_SL(const tsXParsedData *parsedData, U08 useTCP)
{
    if ((useTCP == COMM_USB) &&
        (parsedData->ParamCount == 1) &&
        (parsedData->Params[0].value._int == '?'))
    {
        return;
    }

    if (parsedData->ParamCount == 0)
    {
        xcprintf(ANSI_TX_LightYellow);
        xprintf("\t======================================================================");
        xprintf("\t                         [ State List : xSL ]                         ");
        xprintf("\t======================================================================");
        xcprintf(ANSI_TX_ORG);

        xcprintf(ANSI_TX_LightCyan);
        xprintf("\t[ Common ]");
        xcprintf(ANSI_TX_ORG);
        xprintf("\t  %-42s : %10.3f sec", "xSL.Time", (double)xSL.Time);
        xprintf("\t  %-42s : %10d", "xSL.isBusy", xSL.isBusy);
        xprintf("\t  %-42s : %10lu Bytes", "sizeof(tsXStateList)", (unsigned long)sizeof(tsXStateList));

        xcprintf(ANSI_TX_LightGreen);
        xprintf("\t----------------------------------------------------------------------");
        xprintf("\t[ LED State ]");
        xcprintf(ANSI_TX_ORG);
        //xprintf("\t  %-42s : %10d", "xSL.LED.Status", xSL.LED.Status);

        xcprintf(ANSI_TX_LightGreen);
        xprintf("\t----------------------------------------------------------------------");
        xprintf("\t[ Robot Door State ]");
        xcprintf(ANSI_TX_ORG);
        //xprintf("\t  %-42s : %10d", "xSL.Door.isInitialized", xSL.Door.isInitialized);


        xcprintf(ANSI_TX_LightGreen);
        xprintf("\t----------------------------------------------------------------------");
        xprintf("\t[ Servo A6 State ]");
        xcprintf(ANSI_TX_ORG);
        //xprintf("\t  %-42s : %10d", "xSL.ServoA6.isInitialized", xSL.ServoA6.isInitialized);

        xcprintf(ANSI_TX_LightYellow);
        xprintf("\t======================================================================");
        xcprintf(ANSI_TX_ORG);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);

        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_Print_CD(const tsXParsedData *parsedData, U08 useTCP)
{
    if ((useTCP == COMM_USB) &&
        (parsedData->ParamCount == 1) &&
        (parsedData->Params[0].value._int == '?'))
    {
        return;
    }

    if (parsedData->ParamCount == 0)
    {
        xcprintf(ANSI_TX_LightYellow);
        xprintf("\t======================================================================");
        xprintf("\t                       [ Control Data : xCD ]                         ");
        xprintf("\t======================================================================");
        xcprintf(ANSI_TX_ORG);

        xcprintf(ANSI_TX_LightCyan);
        xprintf("\t[ Common / Header ]");
        xcprintf(ANSI_TX_ORG);
        xprintf("\t  %-42s : %10.3f", "xCD.ID", (double)xCD.ID);
        xprintf("\t  %-42s : %10.3f", "xCD.FilteredData", (double)xCD.FilteredData);
        xprintf("\t  %-42s : %10.3f degC", "xCD.CPU_Temperature", (double)xCD.CPU_Temperature);
        xprintf("\t  %-42s : %10.3f degC", "xCD.BOARD_Temperature", (double)xCD.BOARD_Temperature);
        xprintf("\t  %-42s : %10.0f Bytes", "xCD.SL_Size", (double)xCD.SL_Size);
        xprintf("\t  %-42s : %10.3f sec", "xCD.Time", (double)xCD.Time);
        xprintf("\t  %-42s : %10lu Bytes", "sizeof(tsXControlData)", (unsigned long)sizeof(tsXControlData));

        xcprintf(ANSI_TX_LightMagenta);
        xprintf("\t----------------------------------------------------------------------");
        xprintf("\t[ xCD.LED ]");
        xcprintf(ANSI_TX_ORG);
        //xprintf("\t  %-42s : %10d", "xCD.LED.Out", xCD.LED.Out);

        xcprintf(ANSI_TX_LightGreen);
        xprintf("\t----------------------------------------------------------------------");
        xprintf("\t[ xCD.Door : Robot Door I/O ]");
        xcprintf(ANSI_TX_ORG);
        //xprintf("\t  %-42s : %10d", "xCD.Door.CloseSensor", xCD.Door.CloseSensor);

        xcprintf(ANSI_TX_LightCyan);
        xprintf("\t----------------------------------------------------------------------");
        xprintf("\t[ xCD.ServoA6 : Speed / Position / Centrifuge ]");
        xcprintf(ANSI_TX_ORG);
        //xprintf("\t  %-42s : %10d pps", "xCD.ServoA6.CurrentSpeed.pps", (int)xCD.ServoA6.CurrentSpeed.pps);

        xcprintf(ANSI_TX_LightCyan);
        xprintf("\t----------------------------------------------------------------------");
        xprintf("\t[ xCD.SL Snapshot ]");
        xcprintf(ANSI_TX_ORG);
        xprintf("\t  %-42s : %10d", "xCD.SL.isBusy", xCD.SL.isBusy);
        //xprintf("\t  %-42s : %10d", "xCD.SL.Door.Status (0:err,1:mv,2:cl,3:op)", xCD.SL.Door.Status);

        xcprintf(ANSI_TX_LightYellow);
        xprintf("\t======================================================================");
        xcprintf(ANSI_TX_ORG);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);

        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

static int RPL_SetParameter(int index, const tsXParsedData *parsedData)
{
    int value_i;
    float value_f;

    switch (index)
    {
    /* --- Diagnose (PL Header) --- */
    case RPL_SET_DG_CPU_TEMP_OVERHEAT:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        xPL.Header.DG_CPU_Temperature_Overheat_Criteria = value_f;
        LOG_MSG_SEND("[PL] Set DG_CPU_Temperature_Overheat_Criteria = %.3f", (double)value_f);
        return YES;

    case RPL_SET_DG_CPU_TEMP_ALARM_INTERVAL:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;

        xPL.Header.DG_CPU_Temp_Alarm_Interval_10msec = (U32)value_i;
        LOG_MSG_SEND("[PL] Set DG_CPU_Temp_Alarm_Interval_10msec = %lu",
                     (unsigned long)xPL.Header.DG_CPU_Temp_Alarm_Interval_10msec);
        return YES;

    case RPL_SET_DG_IS_DIAGNOSIS_ENABLED:
        if (GetParamInt(parsedData, 1, &value_i) == NO)
            return NO;

        xPL.Header.DG_IsDiagnosisEnabled = (value_i != 0) ? YES : NO;
        LOG_MSG_SEND("[PL] Set DG_IsDiagnosisEnabled = %d", xPL.Header.DG_IsDiagnosisEnabled);
        return YES;

    case RPL_SET_DG_CLIENT2HOST_LOGMODE:
        if (GetParamInt(parsedData, 1, &value_i) == NO)
            return NO;

        xPL.Header.DG_Client2Host_LogMode = value_i;
        LOG_MSG_SEND("[PL] Set DG_Client2Host_LogMode = %d", xPL.Header.DG_Client2Host_LogMode);
        return YES;

    /* --- Robot Door --- */
    case RPL_SET_DOOR_DIRECTION:
        if (GetParamInt(parsedData, 1, &value_i) == NO)
            return NO;

//        xPL.Door.Direction = value_i;
//        LOG_MSG_SEND("[PL] Set Door.Direction = %d", xPL.Door.Direction);
        return YES;

    case RPL_SET_DOOR_CONTROL_TIMEOUT:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;

        return YES;

    case RPL_SET_DOOR_CLOSE_OVERTIME:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;

        return YES;

    case RPL_SET_DOOR_OPEN_OVERTIME:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;

        return YES;

    case RPL_SET_DOOR_RELATIVE_DISTANCE:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;

        return YES;

    case RPL_SET_DOOR_MOTOR_SPEED:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;

        return YES;

    case RPL_SET_DOOR_MOTOR_ACCEL:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;

        return YES;

    case RPL_SET_DOOR_MOTOR_NORMAL_CURRENT:
        if (GetParamFloat(parsedData, 1, &value_f) == NO || value_f < 0.0f)
            return NO;

        return YES;

    case RPL_SET_DOOR_MOTOR_HOLDING_CURRENT:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;

        return YES;

    case RPL_SET_DOOR_MOTOR_RESOLUTION:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;

        return YES;

    /* --- Servo A6 --- */
    case RPL_SET_SERVO_DIRECTION:
        if (GetParamInt(parsedData, 1, &value_i) == NO)
            return NO;

//        xPL.ServoA6.Direction = value_i;
//        LOG_MSG_SEND("[PL] Set ServoA6.Direction = %d", xPL.ServoA6.Direction);
        return YES;

    case RPL_SET_SERVO_PULSE_PER_REV:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i <= 0)
            return NO;

        return YES;

    case RPL_SET_SERVO_HOME_FWD_SPEED:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        return YES;

    case RPL_SET_SERVO_HOME_BWD_SPEED:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        return YES;

    case RPL_SET_SERVO_HOME_ACCEL:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        return YES;

    case RPL_SET_SERVO_HOME_OFFSET:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        return YES;

    case RPL_SET_SERVO_SLOT_SPEED:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        return YES;

    case RPL_SET_SERVO_SLOT_ACCEL:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        return YES;

    case RPL_SET_SERVO_SLOT_DECEL:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        return YES;

    case RPL_SET_SERVO_SLOT_POSITION_OFFSET ...(RPL_SET_SERVO_SLOT_POSITION_OFFSET + SLOT_COUNT - 1):
    {
        /* slot 위치 오프셋: slot 1~6 을 index 연속으로 사용, PL <idx>,<value> */
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;

        return YES;
    }

    case RPL_SET_SERVO_JOG_SPEED:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        return YES;

    case RPL_SET_SERVO_JOG_ACCEL:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        return YES;

    case RPL_SET_SERVO_JOG_DECEL:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        return YES;

    case RPL_SET_SERVO_BASE_SPEED:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        return YES;

    case RPL_SET_SERVO_BASE_ACCEL:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        return YES;

    case RPL_SET_SERVO_BASE_DECEL:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        return YES;

    default:
        return NO;
    }
}

static void RPL_GetParameter(void)
{
    xcprintf(ANSI_TX_LightYellow);
    xprintf("\t======================================================================");
    xprintf("\t                     [ Parameter List : xPL ]                         ");
    xprintf("\t======================================================================");
    xcprintf(ANSI_TX_ORG);

    xcprintf(ANSI_TX_LightCyan);
    xprintf("\t[ Header ]");
    xcprintf(ANSI_TX_ORG);
    xprintf("\t  %-42s : %10d", "xPL.Header.ID", xPL.Header.ID);
    xprintf("\t  %-42s : %10d Bytes", "xPL.Header.Size_PL", xPL.Header.Size_PL);
    xprintf("\t  %-42s : %10d Bytes", "xPL.Header.Size_Header", xPL.Header.Size_Header);
    xprintf("\t  %-42s : %10s ver.", "xPL.Header.FW_Version", xSystemInfo.cd_FWVersion_str);
    xprintf("\t  %-42s : %10lu YYMMDD", "xPL.Header.UpdateDate", (unsigned long)xPL.Header.UpdateDate);
    xprintf("\t  %-42s : %10.3f degC", "xPL.Header.DG_CPU_Temp_Overheat_Criteria",
            (double)xPL.Header.DG_CPU_Temperature_Overheat_Criteria);
    xprintf("\t  %-42s : %10lu 10ms", "xPL.Header.DG_CPU_Temp_Alarm_Interval_10ms",
            (unsigned long)xPL.Header.DG_CPU_Temp_Alarm_Interval_10msec);
    xprintf("\t  %-42s : %10d", "xPL.Header.DG_IsDiagnosisEnabled", xPL.Header.DG_IsDiagnosisEnabled);
    xprintf("\t  %-42s : %10d", "xPL.Header.DG_Client2Host_LogMode", xPL.Header.DG_Client2Host_LogMode);
    xprintf("\t  %-42s : %10.3f sec", "xPL.Header.Time", (double)xPL.Header.Time);

    xcprintf(ANSI_TX_LightGreen);
    xprintf("\t----------------------------------------------------------------------");
    xprintf("\t[ LED Parameter ]");
    xcprintf(ANSI_TX_ORG);
//    xprintf("\t  %-42s : %10d", "xPL.LED.CMD_StartControl", xPL.LED.CMD_StartControl);

    xcprintf(ANSI_TX_LightGreen);
    xprintf("\t----------------------------------------------------------------------");
    xprintf("\t[ Robot Door Parameter ]");
    xcprintf(ANSI_TX_ORG);
//    xprintf("\t  %-42s : %10d", "xPL.Door.CMD_StartControl", xPL.Door.CMD_StartControl);
    xcprintf(ANSI_TX_LightGreen);
//    xprintf("\t  %-42s : %10lu ms", "xPL.Door.CloseSensorOverTime_ms", (unsigned long)xPL.Door.CloseSensorOverTime_ms);
    xcprintf(ANSI_TX_ORG);
//	xprintf("\t  %-42s : %10lu pulse", "xPL.Door.RelativeDistance_Count", (unsigned long)xPL.Door.RelativeDistance_Count);

    xcprintf(ANSI_TX_LightMagenta);
    xprintf("\t----------------------------------------------------------------------");
    xprintf("\t[ Servo A6 Parameter ]");
    xcprintf(ANSI_TX_ORG);
//    xprintf("\t  %-42s : %10d", "xPL.ServoA6.CMD_StartControl", xPL.ServoA6.CMD_StartControl);

    xcprintf(ANSI_TX_LightGreen);
//    xprintf("\t  %-42s : %10.3f rpm", "xPL.ServoA6.Home.Speed_Forward_rpm", (double)xPL.ServoA6.Home.Speed_Forward_rpm);

    xcprintf(ANSI_TX_ORG);

//    xprintf("\t  %-42s : %10.3f rpm", "xPL.ServoA6.Slot.Speed_rpm", (double)xPL.ServoA6.Slot.Speed_rpm);

    xcprintf(ANSI_TX_LightGreen);
    for (int slot = 0; slot < SLOT_COUNT; slot++)
    {
//        xprintf("\t  %-39s[%d] : %10lu pulse",
//                "xPL.ServoA6.Slot.PositionOffset_pulse", slot,
//                (unsigned long)xPL.ServoA6.Slot.PositionOffset_pulse[slot]);
    }
    xcprintf(ANSI_TX_ORG);

//    xprintf("\t  %-42s : %10.3f rpm", "xPL.ServoA6.Jog.Speed_rpm", (double)xPL.ServoA6.Jog.Speed_rpm);


    xcprintf(ANSI_TX_LightYellow);
    xprintf("\t======================================================================");
    xcprintf(ANSI_TX_ORG);
}

static void RPL_PrintHelp(void)
{
    xcprintf(ANSI_TX_LightYellow);
    xprintf("\t======================================================================");
    xprintf("\t                            [ PL Help ]                               ");
    xprintf("\t======================================================================");
    xcprintf(ANSI_TX_ORG);

    xprintf("\tPL                 : Print Parameter List.");
    xprintf("\tPL ?               : Print this help.");
    xprintf("\tPL <idx>,<value>   : Set parameter value.");
    xprintf("\t----------------------------------------------------------------------");

    xcprintf(ANSI_TX_LightCyan);
    xprintf("\t[ Settable Parameter Index ]");
    xcprintf(ANSI_TX_ORG);

    xprintf("\t  [Idx] %-50s : %8s : %s", "Parameter", "Current", "Type");

    /* --- Diagnose (PL Header) --- */
    xprintf("\t  [%2d] %-51s : %8.3f : F32, degC",
            RPL_SET_DG_CPU_TEMP_OVERHEAT,
            "xPL.Header.DG_CPU_Temperature_Overheat_Criteria",
            (double)xPL.Header.DG_CPU_Temperature_Overheat_Criteria);

    xprintf("\t  [%2d] %-51s : %8lu : U32, 10ms",
            RPL_SET_DG_CPU_TEMP_ALARM_INTERVAL,
            "xPL.Header.DG_CPU_Temp_Alarm_Interval_10msec",
            (unsigned long)xPL.Header.DG_CPU_Temp_Alarm_Interval_10msec);

    xprintf("\t  [%2d] %-51s : %8d : int, 0/1",
            RPL_SET_DG_IS_DIAGNOSIS_ENABLED,
            "xPL.Header.DG_IsDiagnosisEnabled",
            xPL.Header.DG_IsDiagnosisEnabled);

    xprintf("\t  [%2d] %-51s : %8d : int",
            RPL_SET_DG_CLIENT2HOST_LOGMODE,
            "xPL.Header.DG_Client2Host_LogMode",
            xPL.Header.DG_Client2Host_LogMode);

    xprintf("\t----------------------------------------------------------------------");

    /* --- Robot Door --- */
//    xprintf("\t  [%2d] %-51s : %8d : int",
//            RPL_SET_DOOR_DIRECTION,
//            "xPL.Door.Direction",
//            xPL.Door.Direction);


    xprintf("\t----------------------------------------------------------------------");

    /* --- Servo A6 --- */
//    xprintf("\t  [%2d] %-51s : %8d : int",
//            RPL_SET_SERVO_DIRECTION,
//            "xPL.ServoA6.Direction",
//            xPL.ServoA6.Direction);

    xcprintf(ANSI_TX_LightGreen);
//    for (int slot = 0; slot < SLOT_COUNT; slot++)
//    {
//        xprintf("\t  [%2d] %-48s[%d] : %8lu : U32, pulse",
//                RPL_SET_SERVO_SLOT_POSITION_OFFSET + slot,
//                "xPL.ServoA6.Slot.PositionOffset_pulse", slot,
//                (unsigned long)xPL.ServoA6.Slot.PositionOffset_pulse[slot]);
//    }
    xcprintf(ANSI_TX_ORG);

//    xprintf("\t  [%2d] %-51s : %8.3f : F32, rpm",
//            RPL_SET_SERVO_JOG_SPEED,
//            "xPL.ServoA6.Jog.Speed_rpm",
//            (double)xPL.ServoA6.Jog.Speed_rpm);


    xprintf("\t----------------------------------------------------------------------");
    xprintf("\tExample:");
    xprintf("\t  PL %d,70.0      -> CPU overheat criteria = 70.0 degC", RPL_SET_DG_CPU_TEMP_OVERHEAT);
    xprintf("\t  PL %d,10000     -> Robot door control timeout = 10000 ms", RPL_SET_DOOR_CONTROL_TIMEOUT);
    xprintf("\t  PL %d,20000    -> Robot door motor speed = 20000 pps", RPL_SET_DOOR_MOTOR_SPEED);
    xprintf("\t  PL %d,30.0     -> Servo home forward speed = 30.0 rpm", RPL_SET_SERVO_HOME_FWD_SPEED);
    xprintf("\t  PL %d,1000     -> Servo slot[3] position offset = 1000 pulse", RPL_SET_SERVO_SLOT_POSITION_OFFSET + 2);

    xcprintf(ANSI_TX_LightYellow);
    xprintf("\t======================================================================");
    xcprintf(ANSI_TX_ORG);
}

void CMD_Handle_Print_PL(const tsXParsedData *parsedData, U08 useTCP)
{
    int index;

    /* Help */
    if ((useTCP == COMM_USB) &&
        (parsedData->ParamCount == 1) &&
        (parsedData->Params[0].value._int == '?'))
    {
        RPL_PrintHelp();
        return;
    }

    /* Set : PL <idx>,<value> */
    if (parsedData->ParamCount == 2)
    {
        if (GetParamInt(parsedData, 0, &index) == NO)
        {
            SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
            return;
        }

        if (RPL_SetParameter(index, parsedData) == NO)
        {
            SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
            return;
        }

        XBuffer_AddString(xSendMsg, "OK", NO_COMMA);
        return;
    }

    /* Get */
    if (parsedData->ParamCount == 0)
    {
        RPL_GetParameter();

        return;
    }

    /* Invalid */
    SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
    XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);

    if (parsedData->ParamCount > 0 &&
        parsedData->Params[0].value._int != '?')
    {
        xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_ORIGIN(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0){
        if (CMD_RejectIfDecapperBusy())
            return;

        xAT = ACTION_ORIGIN;
        XBuffer_AddString(xSendMsg, "origin", NO_COMMA);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_MOVE(const tsXParsedData *parsedData, U08 useTCP)
{
    int mode;
    int axis;
    int pulse;

    if ((parsedData->ParamCount == 3) &&
        (GetParamInt(parsedData, 0, &mode) == YES) &&
        (GetParamInt(parsedData, 1, &axis) == YES) &&
        (GetParamInt(parsedData, 2, &pulse) == YES) &&
        ((mode == 'R') || (mode == 'A')) &&
        ((axis == 'Z') || (axis == 'R')))
    {
        if (CMD_RejectIfDecapperBusy())
            return;

        if (axis == 'Z')
        {
            xCD.Decapper.Motor_TargetPos[aZ] = (S32)pulse;
            xAT = (mode == 'R') ? ACTION_RMOVEZ : ACTION_AMOVEZ;
        }
        else
        {
            xCD.Decapper.Motor_TargetPos[aR] = (S32)pulse;
            xAT = (mode == 'R') ? ACTION_RROTATE : ACTION_AROTATE;
        }

        XBuffer_AddString(xSendMsg, "OK", NO_COMMA);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if ((parsedData->ParamCount > 0) &&
            (parsedData->Params[0].value._int != '?'))
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_READY(const tsXParsedData *parsedData, U08 useTCP)
{
    int value;

    if ((parsedData->ParamCount == 1) &&
        (GetParamInt(parsedData, 0, &value) == YES) &&
        ((value == 0) || (value == 1)))
    {
        if (CMD_RejectIfDecapperBusy())
            return;

        xAT = (value == 1) ? ACTION_Y_L_MOVE : ACTION_Y_H_MOVE;
        XBuffer_AddString(xSendMsg, "OK", NO_COMMA);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if ((parsedData->ParamCount > 0) &&
            (parsedData->Params[0].value._int != '?'))
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_UDECAP(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (CMD_RejectIfDecapperBusy())
            return;

        xAT = ACTION_UDECAP;
        XBuffer_AddString(xSendMsg, "udecap", NO_COMMA);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_UCAP(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (CMD_RejectIfDecapperBusy())
            return;

        xAT = ACTION_UCAP;
        XBuffer_AddString(xSendMsg, "ucap", NO_COMMA);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_LongRun(const tsXParsedData *parsedData, U08 useTCP)
{
    /* LR : long-run 시작, 정지는 STOP 명령으로 처리 */
    if (parsedData->ParamCount == 0)
    {
        if (CMD_RejectIfDecapperBusy())
            return;

        xAT = ACTION_LONGRUN;
        XBuffer_AddString(xSendMsg, "LR", NO_COMMA);
        return;
    }

    SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
    XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);

    if (parsedData->Params[0].value._int != '?')
        xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
}

void CMD_Handle_BGRIP(const tsXParsedData *parsedData, U08 useTCP)
{
    int value;

    if ((parsedData->ParamCount == 1) &&
    		(GetParamInt(parsedData, 0, &value) == YES) &&
        ((value == 0) || (value == 1)))
    {
        if (CMD_RejectIfDecapperBusy())
            return;

        CDecap_SetBodyGripCommand((U08)value);
        xAT = ACTION_BODY_GRIP;
        XBuffer_AddString(xSendMsg, "OK", NO_COMMA);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if ((parsedData->ParamCount > 0) &&
            (parsedData->Params[0].value._int != '?'))
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_CGRIP(const tsXParsedData *parsedData, U08 useTCP)
{
    int value;

    if ((parsedData->ParamCount == 1) &&
        (GetParamInt(parsedData, 0, &value) == YES) &&
        ((value == 0) || (value == 1)))
    {
        if (CMD_RejectIfDecapperBusy())
            return;

        CDecap_SetCapGripCommand((U08)value);
        xAT = ACTION_CAP_GRIP;
        XBuffer_AddString(xSendMsg, "OK", NO_COMMA);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if ((parsedData->ParamCount > 0) &&
            (parsedData->Params[0].value._int != '?'))
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}
/*Debug Setting/Getting Command*/
void CMD_Handle_RPOS(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0){
        XBuffer_AddInt(xSendMsg, (int)CDecap_GetZPosition(), NO_COMMA);
        xprintf("Currunt Z_Position = %ld",xCD.Decapper.Motor_CurPos);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/*Unused Command*/
// debugging code
void CMD_Handle_PSTA(const tsXParsedData *parsedData, U08 useTCP)
{
    U08 i = 1;

    if (parsedData->ParamCount == 0)
    {
        __newLine();
        xcprintf(ANSI_TX_Yellow);
        /*[ 1]*/ xprintf("\t[%2d] %6d : xSL.isBusy", i++, xSL.isBusy);
        //*[ 2]*/ xprintf("\t[%2d] %6d : xServoA6.IsServoOn()", i++, xServoA6.IsServoOn());
        //*[ 3]*/ xprintf("\t[%2d] %6d : xServoA6.IsHomed()", i++, xServoA6.IsHomed());
        /*[ 4]*/ xprintf("\t[%2d] %6d : IsError()", i++, IsError());
        /*[ 5]*/ xprintf("\t[%2d] %6s : error code.", i++, GetErrorCode_char());
        xcprintf(ANSI_TX_Cyan);
        //*[ 7]*/ xprintf("\t[%2d] %6d : xServoA6.GetPosition_SlotNum() + 1", i++, xServoA6.GetPosition_SlotNum() + 1);
        xcprintf(ANSI_TX_ORG);
        //*[ 9]*/ xprintf("\t[%2d] %6d : xServoA6.IsDriverError()", i++, xServoA6.IsDriverError());
        xcprintf(ANSI_TX_Red);
        /*[13]*/ xprintf("\t[%2d] %6d : rsv.", i++, 0);
        /*[14]*/ xprintf("\t[%2d] %6d : rsv.", i++, 0);
        /*[15]*/ xprintf("\t[%2d] %6d : rsv.", i++, 0);
        /*[16]*/ xprintf("\t[%2d] %6d : rsv.", i++, 0);
        /*[17]*/ xprintf("\t[%2d] %6d : rsv.", i++, 0);
        /*[18]*/ xprintf("\t[%2d] %6d : rsv.", i++, 0);
        /*[19]*/ xprintf("\t[%2d] %6d : rsv.", i++, 0);
        /*[20]*/ xprintf("\t[%2d] %6d : rsv.", i++, 0);
        xcprintf(ANSI_TX_ORG);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}
