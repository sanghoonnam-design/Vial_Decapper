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

bool gZCapUpPosSavePending = false;
S32 gZCapUpPosPendingValue = 0;

/* 명령 핸들러는 인자를 검사하고 요청을 등록한다. 구동은 주기 FSM에서 수행한다.
 * 응답 버퍼의 OK/home/Cap 등은 접수 결과이며 완료 상태는 GSTA/SL로 확인한다.
 * useTCP는 통신 경로 구분값이며 일부 진단 출력/도움말은 USB 조건을 사용한다.
 */
const tsXCommandMapping gModuleCommandTable[] =
    {
        /** @note USER CODE BEGIN */

        /*User Command*/
        {0, "GSTA", /*      */ CMD_Handle_GSTA, /*            */ "Get the system State.", /*            */ "GSTA"},     // 시스템 상태 반환
        {0, "ST", /*        */ CMD_Handle_GSTA, /*            */ "Get the system State.", /*            */ "ST"},       // 시스템 상태 반환

        /** @note 각 장비에 해당하는 명령어 */
		{0, "SPD", /*       */ CMD_Handle_SPEED, /*           */ "Get/set motor speed percent.", /*		*/ "SPD [1~100]"},
		{0, "SPEED", /*     */ CMD_Handle_SPEED, /*           */ "Get/set motor speed percent.", /*    	*/ "SPEED [1~100]"},

		{0, "HOME", /*      */ CMD_Handle_HOME, /*            */ "Run homing sequence.", /*         	*/ "HOME"}, // Z 원점 탐색 및 Y High 복귀 요청

		{0, "STOP", /*      */ CMD_Handle_STOP, /*            */ "Stop all Decapper motion.", /*    	*/ "STOP"}, // 전체 모션 정지 요청
		{0, "PAUSE", /*     */ CMD_Handle_PAUSE, /*           */ "Pause current operation.", /*     	*/ "PAUSE"}, // 자동 동작 일시정지 요청
		{0, "RESUME", /*    */ CMD_Handle_RESUME, /*          */ "Resume paused operation.", /*     	*/ "RESUME"}, // 일시정지 동작 재개 요청

		{0, "DECAP", /*     */ CMD_Handle_DECAP, /*           */ "Run automatic decapping.", /*     	*/ "DECAP"}, // 자동 뚜껑 분리 요청
		{0, "CAP", /*       */ CMD_Handle_CAP, /*             */ "Run automatic capping.", /*       	*/ "CAP"}, // 자동 뚜껑 체결 요청

		/*Debug Command*/
		{1, "SL", /*        */ CMD_Handle_Print_SL, /*        */ "Print SL.", /*                        */ "SL"},       // SL 출력
		{1, "CD", /*        */ CMD_Handle_Print_CD, /*        */ "Print CD.", /*                        */ "CD"},       // CD 출력
		{1, "PL", /*        */ CMD_Handle_Print_PL, /*        */ "Print PL.", /*                        */ "PL <...>"}, // PL 출력

		{1, "ORG", /*       */ CMD_Handle_ORIGIN, /*          */ "Move to origin position.", /*     	*/ "ORG"}, // 기존 원점 좌표로 복귀 요청
		{1, "MOVE", /*      */ CMD_Handle_MOVE, /*            */ "Move Z/R axis.", /*               	*/ "MOVE <R|A> <Z|R> <pulse>"}, // Z/R 축 상대/절대 위치 이동
		{1, "READY", /*     */ CMD_Handle_READY, /*            */ "Move Y axis to limit.", /*          	*/ "READY <0|1>"}, // 0: Y High, 1: Y Low

		{1, "UDECAP", /*    */ CMD_Handle_UDECAP, /*          */ "Run unit decapping.", /*          	*/ "UDECAP"}, // Z/R 중심의 유닛 분리 시험
		{1, "UCAP", /*      */ CMD_Handle_UCAP, /*            */ "Run unit capping.", /*            	*/ "UCAP"}, // Z/R 중심의 유닛 체결 시험

		{1, "CAPDECAPLR", /**/ CMD_Handle_LongRun, /*         */ "Run long-run test.", /*           	*/ "CAPDECAPLR"}, // 롱런 테스트
		{1, "LR", /*        */ CMD_Handle_LongRun, /*         */ "Run long-run test.", /*           	*/ "LR"}, // 롱런 테스트

		{1, "BGRIP", /*     */ CMD_Handle_BGRIP, /*           */ "Set body gripper output.", /*     	*/ "BGRIP <0|1>"}, // Body 그리퍼 출력 요청
		{1, "CGRIP", /*     */ CMD_Handle_CGRIP, /*           */ "Set cap gripper output.", /*      	*/ "CGRIP <0|1>"}, // Cap 그리퍼 출력 요청
		{1, "RPOS", /*		*/ CMD_Handle_RPOS, /*            */ "Read current Z position.", /*      	*/ "RPOS"}, // 현재 Z 좌표 조회 및 SAVEE 저장 후보 설정

        /** @note USER CODE END */
};

const int gModuleCommandCount = sizeof(gModuleCommandTable) / sizeof(tsXCommandMapping);

static int GetParamInt(const tsXParsedData *parsedData, int index, int *out);
static int GetParamFloat(const tsXParsedData *parsedData, int index, float *out);
static bool CMD_RejectIfDecapperBusy(void);

/*==============================================================================
 * Local helpers
 *============================================================================*/
/**
 * @brief 동작 명령의 사전 조건을 확인한다. 오류 또는 대기 아님이면 응답 버퍼에 원인을 기록하고 true를 반환한다.
 * false 뒤에도 실제 접수는 TryRequestAction으로 다시 보호해야 유지보수 작업과의 경쟁을 막을 수 있다.
 */
static bool CMD_RejectIfDecapperBusy(void) {
    if (xSL.isError){
        CDecap_ReportError();
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        return true;
    }
    if (!CDecap_IsIdle()) {
        XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);
        return true;
    }

    return false;
}

/**
 * @brief 지정 인덱스의 숫자 인자를 int로 읽어 out에 저장한다.
 * 정수 또는 실수를 허용하며 실수는 정수 변환된다. 잘못된 포인터·인덱스·타입은 NO, 성공은 YES이다.
 */
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

/**
 * @brief 지정 인덱스의 숫자 인자를 float로 읽어 out에 저장한다.
 * 정수도 실수로 변환한다. 잘못된 포인터·인덱스·타입은 NO, 성공은 YES이다.
 */
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
/**
 * @brief GSTA/ST 응답에 Busy, Enable, Homed, Error와 오류 코드를 순서대로 기록한다.
 * Decapper 오류를 공통 오류 값에 동기화한 뒤 보고한다. 동작 완료 여부는 이 상태로 확인한다.
 */
void CMD_Handle_GSTA(const tsXParsedData *parsedData, U08 useTCP){
    CDecap_ReportError();
    if (parsedData->ParamCount == 0) {
        //  ==============================================================================
        // default info.
        /* [01] */ XBuffer_AddInt(xSendMsg, xSL.isBusy, COMMA); //= (!xServoA6.IsStop() && xDoor.IsStop())
        /* [02] */ XBuffer_AddInt(xSendMsg, xSL.isEnable, COMMA);
        /* [03] */ XBuffer_AddInt(xSendMsg, xSL.isHomed, COMMA);
        /* [04] */ XBuffer_AddInt(xSendMsg, xSL.isError, COMMA);
        /* [05] */ XBuffer_AddString(xSendMsg, GetErrorCode_char(), COMMA);
        //  ==============================================================================
    }
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);

        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);

        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief SPD/SPEED를 처리한다. 인자 없으면 현재 비율 조회, 정수 1~100이면 비율 설정 후 응답한다.
 * 속도 비율은 새 모션 명령에 사용되며 이 명령은 이동 자체를 시작하지 않는다.
 */
void CMD_Handle_SPEED(const tsXParsedData *parsedData, U08 useTCP){
    if (parsedData->ParamCount == 0) {
		XBuffer_AddCommandString(xSendMsg, "SPEED", NO_COMMA);
		XBuffer_AddInt(xSendMsg, (int)CDecap_GetSpeedPercent(), NO_COMMA);
    }
	else if ((parsedData->ParamCount == 1) &&
			 (parsedData->Params[0].type == PARAM_TYPE_INT) &&
			 (parsedData->Params[0].value._int >= 1) &&
			 (parsedData->Params[0].value._int <= 100)) {
		CDecap_SetSpeedPercent((U08)parsedData->Params[0].value._int);
		XBuffer_AddCommandString(xSendMsg, "SPEED", NO_COMMA);
		XBuffer_AddInt(xSendMsg, (int)CDecap_GetSpeedPercent(), NO_COMMA);
	}
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
		if ((parsedData->ParamCount > 0) &&
			(parsedData->Params[0].value._int != '?'))
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief 인자 없는 HOME을 검사하고 원점 탐색 요청을 등록한다.
 * 응답 home은 접수 의미이며 실제 완료는 GSTA의 Homed/Busy로 확인한다.
 */
void CMD_Handle_HOME(const tsXParsedData *parsedData, U08 useTCP) {
    if (parsedData->ParamCount == 0) {
        if (CMD_RejectIfDecapperBusy()) return;

        if (!CDecap_TryRequestAction(ACTION_HOME)){

            XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);

            return;

        }
        XBuffer_AddString(xSendMsg, "home", NO_COMMA);
    }
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief 인자 없는 STOP을 독립 정지 요청으로 등록한다.
 * 일반 Busy 검사 없이 오류·동작 중에도 접수하며 실제 정지는 다음 FSM 실행에서 처리한다.
 */
void CMD_Handle_STOP(const tsXParsedData *parsedData, U08 useTCP) {
    if (parsedData->ParamCount == 0) {
        CDecap_RequestStop();
        XBuffer_AddString(xSendMsg, "stop", NO_COMMA);
    }
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief Busy일 때 일시정지 요청을 등록한다.
 * 실제 허용 대상은 FSM의 CAP/DECAP/LONGRUN이다. pause 응답만으로 일시정지 완료를 뜻하지 않는다.
 */
void CMD_Handle_PAUSE(const tsXParsedData *parsedData, U08 useTCP) {
    if (parsedData->ParamCount == 0) {
        if (xSL.isBusy == YES) {
            xAT = ACTION_PAUSE;
            XBuffer_AddString(xSendMsg, "pause", NO_COMMA);
        }
        else {
            XBuffer_AddString(xSendMsg, "isbusy = no -> not pause", NO_COMMA);
        }
    }
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief Busy일 때 재개 요청을 등록한다.
 * FSM이 일시정지 상태와 저장 문맥을 확인한 뒤 재개 여부를 결정한다.
 */
void CMD_Handle_RESUME(const tsXParsedData *parsedData, U08 useTCP) {
    if (parsedData->ParamCount == 0) {
        if (xSL.isBusy == YES) {
            xAT = ACTION_RESUME;
            XBuffer_AddString(xSendMsg, "resume", NO_COMMA);
        }
        else {
            XBuffer_AddString(xSendMsg, "isbusy = no -> not pause", NO_COMMA);
        }
    }
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief 인자 없는 자동 DECAP 요청을 대기·오류 상태 확인 후 등록한다.
 * CT 감지와 HOME 완료 판단 및 실제 구동은 Decapper FSM이 수행한다.
 */
void CMD_Handle_DECAP(const tsXParsedData *parsedData, U08 useTCP) {
    if (parsedData->ParamCount == 0) {
        if (CMD_RejectIfDecapperBusy())
            return;

        if (!CDecap_TryRequestAction(ACTION_DECAP)){

            XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);

            return;

        }
        XBuffer_AddString(xSendMsg, "Decap", NO_COMMA);
    }
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief 인자 없는 자동 CAP 요청을 대기·오류 상태 확인 후 등록한다.
 * 응답은 접수 결과이며 체결·복귀 완료는 상태 조회로 확인한다.
 */
void CMD_Handle_CAP(const tsXParsedData *parsedData, U08 useTCP) {
    if (parsedData->ParamCount == 0) {
        if (CMD_RejectIfDecapperBusy())
            return;

        if (!CDecap_TryRequestAction(ACTION_CAP)){

            XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);

            return;

        }
        XBuffer_AddString(xSendMsg, "Cap", NO_COMMA);
    }
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}


/*Debug Command*/
/**
 * @brief SL 명령으로 공통 상태와 Decapper 센서·모터·오류 상태를 진단 출력한다.
 * SL은 관측값이며 PL 설정이나 CD 요청값과 구분한다.
 */
void CMD_Handle_Print_SL(const tsXParsedData *parsedData, U08 useTCP) {
    if ((useTCP == COMM_USB) &&
        (parsedData->ParamCount == 1) &&
        (parsedData->Params[0].value._int == '?')) {
        return;
    }

    if (parsedData->ParamCount == 0) {
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
        xprintf("\t  %-42s : %10d", "xSL.isError", xSL.isError);
        xprintf("\t  %-42s : %10d", "xSL.isEnable", xSL.isEnable);
        xprintf("\t  %-42s : %10d", "xSL.isHomed", xSL.isHomed);
        xprintf("\t  %-42s : %10d", "xSL.errorCode", xSL.errorCode);
        xprintf("\t  %-42s : %10lu Bytes", "sizeof(tsXStateList)", (unsigned long)sizeof(tsXStateList));

        xcprintf(ANSI_TX_LightGreen);
        xprintf("\t----------------------------------------------------------------------");

        xprintf("\t[ xSL.Decapper ]");
        xprintf("\t  %-42s : %10d", "xSL.Decapper.Cap_is", xSL.Decapper.Cap_is);
        xprintf("\t  %-42s : %10d", "xSL.Decapper.Body_is", xSL.Decapper.Body_is);
        xprintf("\t  %-42s : %10d", "xSL.Decapper.Z_HL_isError", xSL.Decapper.Z_HL_isError);
        xprintf("\t  %-42s : %10d", "xSL.Decapper.Y_HL_isError", xSL.Decapper.Y_HL_isError);
        xprintf("\t  %-42s : %10d", "xSL.Decapper.CT_Cap_Grip_isError", xSL.Decapper.CT_Cap_Grip_isError);
        xprintf("\t  %-42s : %10d", "xSL.Decapper.CT_Body_Grip_isError", xSL.Decapper.CT_Body_Grip_isError);

        xprintf("\t----------------------------------------------------------------------");
        xprintf("\t[ xSL.CDecapping_Sensor ]");
        xprintf("\t  %-42s : %10d", "xSL.CDecapping_Sensor.Z_H_Limit_Sensor", xSL.CDecapping_Sensor.Z_H_Limit_Sensor);
        xprintf("\t  %-42s : %10d", "xSL.CDecapping_Sensor.Z_L_Limit_Sensor", xSL.CDecapping_Sensor.Z_L_Limit_Sensor);
        xprintf("\t  %-42s : %10d", "xSL.CDecapping_Sensor.Y_H_Limit_Sensor", xSL.CDecapping_Sensor.Y_H_Limit_Sensor);
        xprintf("\t  %-42s : %10d", "xSL.CDecapping_Sensor.Y_L_Limit_Sensor", xSL.CDecapping_Sensor.Y_L_Limit_Sensor);
        xprintf("\t  %-42s : %10d", "xSL.CDecapping_Sensor.CT_Body_Grip_Detect_Open", xSL.CDecapping_Sensor.CT_Body_Grip_Detect_Open);
        xprintf("\t  %-42s : %10d", "xSL.CDecapping_Sensor.CT_Body_Grip_Detect_Close", xSL.CDecapping_Sensor.CT_Body_Grip_Detect_Close);
        xprintf("\t  %-42s : %10d", "xSL.CDecapping_Sensor.CT_Cap_Grip_Detect_Open", xSL.CDecapping_Sensor.CT_Cap_Grip_Detect_Open);
        xprintf("\t  %-42s : %10d", "xSL.CDecapping_Sensor.CT_Cap_Grip_Detect_Close", xSL.CDecapping_Sensor.CT_Cap_Grip_Detect_Close);
        xprintf("\t  %-42s : %10d", "xSL.CDecapping_Sensor.CT_Detect_Sensor", xSL.CDecapping_Sensor.CT_Detect_Sensor);

        xprintf("\t----------------------------------------------------------------------");
        xprintf("\t[ Motor Run ]");
        xprintf("\t  %-42s : %10d", "xSL.xZ_Motor_Run.Motor_Run", xSL.xZ_Motor_Run.Motor_Run);
        xprintf("\t  %-42s : %10d", "xSL.xR_Motor_Run.Motor_Run", xSL.xR_Motor_Run.Motor_Run);

        xcprintf(ANSI_TX_LightYellow);
        xprintf("\t======================================================================");
        xcprintf(ANSI_TX_ORG);
    }
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);

        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief CD 명령으로 Decapper 목표·속도 비율·롱런 및 CT 그리퍼 요청값을 출력한다.
 * 실시간 상태가 아닌 제어 요청 데이터이며 xCDecap 콜백 구조체는 출력 대상에서 제외한다.
 */
void CMD_Handle_Print_CD(const tsXParsedData *parsedData, U08 useTCP) {
    if ((useTCP == COMM_USB) &&
        (parsedData->ParamCount == 1) &&
        (parsedData->Params[0].value._int == '?')) {
        return;
    }

    if (parsedData->ParamCount == 0) {
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
        xprintf("\t[ xCD.Decapper ]");
        xcprintf(ANSI_TX_ORG);
        xprintf("\t  %-42s : %10ld pulse", "xCD.Decapper.Motor_CurPos",
                (long)xCD.Decapper.Motor_CurPos);
        xprintf("\t  %-42s : %10ld pulse", "xCD.Decapper.Motor_TargetPos[Z]",
                (long)xCD.Decapper.Motor_TargetPos[aZ]);
        xprintf("\t  %-42s : %10ld pulse", "xCD.Decapper.Motor_TargetPos[R]",
                (long)xCD.Decapper.Motor_TargetPos[aR]);
        xprintf("\t  %-42s : %10d (%s)", "xCD.Decapper.phaseDecapCap",
                (int)xCD.Decapper.phaseDecapCap,
                (xCD.Decapper.phaseDecapCap == LONGRUN_CAP) ? "CAP" : "DECAP");
        xprintf("\t  %-42s : %10lu", "xCD.Decapper.LongRunCount",
                (unsigned long)xCD.Decapper.LongRunCount);
        xprintf("\t  %-42s : %10u %%", "xCD.Decapper.SpeedPercent",
                (unsigned int)xCD.Decapper.SpeedPercent);
        xprintf("\t  %-42s : %10d", "xCD.Decapper.SystemInfo.isSWLimit",
                xCD.Decapper.SystemInfo.isSWLimit);
        xprintf("\t  %-42s : %10ld", "xCD.Decapper.chMotor", (long)xCD.Decapper.chMotor);

        xprintf("\t  [ Debug Data ]");
        for (int i = 0; i < DEBUG_CD_SIZE; i++) {
            xprintf("\t  %-36s[%2d] : %10d", "xCD.Decapper.debug", i, xCD.Decapper.debug[i]);
        }

        xcprintf(ANSI_TX_LightGreen);
        xprintf("\t----------------------------------------------------------------------");
        xprintf("\t[ xCD.CT : Cap / Body Grip Command ]");
        xcprintf(ANSI_TX_ORG);
        xprintf("\t  %-42s : %10u", "xCD.CT.Body", (unsigned int)xCD.CT.Body);
        xprintf("\t  %-42s : %10u", "xCD.CT.Cap", (unsigned int)xCD.CT.Cap);

        xcprintf(ANSI_TX_LightYellow);
        xprintf("\t======================================================================");
        xcprintf(ANSI_TX_ORG);
    }
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);

        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief PL 인덱스와 두 번째 인자를 검사하여 해당 RAM 설정값을 갱신한다.
 * 성공 YES, 잘못된 인덱스·값은 NO를 반환한다. 호출부가 유지보수 잠금을 획득해야 한다.
 * EEPROM 저장은 별도 명령이며 이 함수는 모든 설정을 드라이버에 즉시 재적용하는 함수가 아니다.
 */
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

    /* --- Vial Decapper --- */
    case RPL_SET_DECAP_RUN_CUR_Z:
        if (GetParamFloat(parsedData, 1, &value_f) == NO || value_f < 0.0f)
            return NO;
        xPL.Decapper.RunCur[aZ] = value_f;
        return YES;

    case RPL_SET_DECAP_RUN_CUR_R:
        if (GetParamFloat(parsedData, 1, &value_f) == NO || value_f < 0.0f)
            return NO;
        xPL.Decapper.RunCur[aR] = value_f;
        return YES;

    case RPL_SET_DECAP_SEL_MAX_CUR_Z:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;
        xPL.Decapper.SelMaxCur[aZ] = value_i;
        return YES;

    case RPL_SET_DECAP_SEL_MAX_CUR_R:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;
        xPL.Decapper.SelMaxCur[aR] = value_i;
        return YES;

    case RPL_SET_DECAP_STOP_CUR_RATE_Z:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0 || value_i > 100)
            return NO;
        xPL.Decapper.StopCurRate[aZ] = value_i;
        return YES;

    case RPL_SET_DECAP_STOP_CUR_RATE_R:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0 || value_i > 100)
            return NO;
        xPL.Decapper.StopCurRate[aR] = value_i;
        return YES;

    case RPL_SET_DECAP_STEP_RESOLUTION:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i <= 0)
            return NO;
        xPL.Decapper.StepResolution = value_i;
        return YES;

    case RPL_SET_DECAP_LIMIT_POS_Z:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;
        xPL.Decapper.Limit_PosZ = (S32)value_i;
        return YES;

    case RPL_SET_DECAP_LIMIT_POS_R:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;
        xPL.Decapper.Limit_PosR = (S32)value_i;
        return YES;

    case RPL_SET_DECAP_SW_NEG_LIMIT_Z:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < -8388608 || value_i >= xPL.Decapper.SwPosLimit[aZ])
            return NO;
        xPL.Decapper.SwNegLimit[aZ] = (S32)value_i;
        return YES;

    case RPL_SET_DECAP_SW_POS_LIMIT_Z:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i > 8388607 || value_i <= xPL.Decapper.SwNegLimit[aZ])
            return NO;
        xPL.Decapper.SwPosLimit[aZ] = (S32)value_i;
        return YES;

    case RPL_SET_DECAP_SW_NEG_LIMIT_R:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < -8388608 || value_i >= xPL.Decapper.SwPosLimit[aR])
            return NO;
        xPL.Decapper.SwNegLimit[aR] = (S32)value_i;
        return YES;

    case RPL_SET_DECAP_SW_POS_LIMIT_R:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i > 8388607 || value_i <= xPL.Decapper.SwNegLimit[aR])
            return NO;
        xPL.Decapper.SwPosLimit[aR] = (S32)value_i;
        return YES;

    case RPL_SET_DECAP_SOFT_LIMIT_ENABLE:
        if (GetParamInt(parsedData, 1, &value_i) == NO || (value_i != 0 && value_i != 1))
            return NO;
        xPL.Decapper.SoftLimitEnable = (U8)value_i;
        return YES;

    case RPL_SET_DECAP_Z_ACC:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i <= 0)
            return NO;
        xPL.Decapper.ZDecapAcc = (U32)value_i;
        return YES;

    case RPL_SET_DECAP_Z_VEL:
        if (GetParamInt(parsedData, 1, &value_i) == NO)
            return NO;
        xPL.Decapper.ZDecapVel = (S32)value_i;
        return YES;

    case RPL_SET_DECAP_Z_CAP_UP_POS:
        if (GetParamInt(parsedData, 1, &value_i) == NO)
            return NO;
        xPL.Decapper.ZCap_UpPos = (S32)value_i;
        return YES;

    case RPL_SET_DECAP_Z_CAP_SIDE_POS:
        if (GetParamInt(parsedData, 1, &value_i) == NO)
            return NO;
        xPL.Decapper.ZCap_SidePos = (S32)value_i;
        return YES;

    case RPL_SET_DECAP_Z_ORIGIN_POS:
        if (GetParamInt(parsedData, 1, &value_i) == NO)
            return NO;
        xPL.Decapper.ZCap_Origin_Position = (S32)value_i;
        return YES;

    case RPL_SET_DECAP_R_ACC:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i <= 0)
            return NO;
        xPL.Decapper.RDecapAcc = (U32)value_i;
        return YES;

    case RPL_SET_DECAP_R_VEL:
        if (GetParamInt(parsedData, 1, &value_i) == NO)
            return NO;
        xPL.Decapper.RDecapVel = (S32)value_i;
        return YES;

    case RPL_SET_DECAP_R_POS:
        if (GetParamInt(parsedData, 1, &value_i) == NO)
            return NO;
        xPL.Decapper.RDecapPos = (S32)value_i;
        return YES;

    default:
        return NO;
    }
}

/**
 * @brief 현재 RAM의 공통 및 Decapper PL 설정을 이름·단위와 함께 진단 출력한다.
 * 저장 매체를 다시 읽거나 모터 설정을 변경하지 않는다.
 */
static void RPL_GetParameter(void) {
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
    xprintf("\t[ Decapper Parameter ]");

    xprintf("\t  [ Motor Current ]");
    xprintf("\t  %-42s : %10.3f A", "xPL.Decapper.RunCur[Z]", (double)xPL.Decapper.RunCur[aZ]);
    xprintf("\t  %-42s : %10.3f A", "xPL.Decapper.RunCur[R]", (double)xPL.Decapper.RunCur[aR]);
    xprintf("\t  %-42s : %10d", "xPL.Decapper.SelMaxCur[Z]", xPL.Decapper.SelMaxCur[aZ]);
    xprintf("\t  %-42s : %10d", "xPL.Decapper.SelMaxCur[R]", xPL.Decapper.SelMaxCur[aR]);
    xprintf("\t  %-42s : %10d %%", "xPL.Decapper.StopCurRate[Z]", xPL.Decapper.StopCurRate[aZ]);
    xprintf("\t  %-42s : %10d %%", "xPL.Decapper.StopCurRate[R]", xPL.Decapper.StopCurRate[aR]);

    xprintf("\t  [ Motor Configuration ]");
    xprintf("\t  %-42s : %10d", "xPL.Decapper.StepResolution", xPL.Decapper.StepResolution);
    xprintf("\t  %-42s : %10s", "xPL.Decapper.SoftLimitEnable",
            xPL.Decapper.SoftLimitEnable ? "ON" : "OFF");

    xprintf("\t  [ Motion Limit ]");
    xprintf("\t  %-42s : %10ld pulse", "xPL.Decapper.Limit_PosZ", (long)xPL.Decapper.Limit_PosZ);
    xprintf("\t  %-42s : %10ld pulse", "xPL.Decapper.Limit_PosR", (long)xPL.Decapper.Limit_PosR);
    xprintf("\t  %-42s : %10ld pulse", "xPL.Decapper.SwNegLimit[Z]", (long)xPL.Decapper.SwNegLimit[aZ]);
    xprintf("\t  %-42s : %10ld pulse", "xPL.Decapper.SwPosLimit[Z]", (long)xPL.Decapper.SwPosLimit[aZ]);
    xprintf("\t  %-42s : %10ld pulse", "xPL.Decapper.SwNegLimit[R]", (long)xPL.Decapper.SwNegLimit[aR]);
    xprintf("\t  %-42s : %10ld pulse", "xPL.Decapper.SwPosLimit[R]", (long)xPL.Decapper.SwPosLimit[aR]);

    xprintf("\t  [ Z Capping / Decapping ]");
    xprintf("\t  %-42s : %10lu pulse/s^2", "xPL.Decapper.ZDecapAcc", (unsigned long)xPL.Decapper.ZDecapAcc);
    xprintf("\t  %-42s : %10ld pulse/s", "xPL.Decapper.ZDecapVel", (long)xPL.Decapper.ZDecapVel);
    xprintf("\t  %-42s : %10ld pulse", "xPL.Decapper.ZCap_UpPos", (long)xPL.Decapper.ZCap_UpPos);
    xprintf("\t  %-42s : %10ld pulse", "xPL.Decapper.ZCap_SidePos", (long)xPL.Decapper.ZCap_SidePos);
    xprintf("\t  %-42s : %10ld pulse", "xPL.Decapper.ZCap_Origin_Position",
            (long)xPL.Decapper.ZCap_Origin_Position);

    xprintf("\t  [ R Capping / Decapping ]");
    xprintf("\t  %-42s : %10lu pulse/s^2", "xPL.Decapper.RDecapAcc", (unsigned long)xPL.Decapper.RDecapAcc);
    xprintf("\t  %-42s : %10ld pulse/s", "xPL.Decapper.RDecapVel", (long)xPL.Decapper.RDecapVel);
    xprintf("\t  %-42s : %10ld pulse", "xPL.Decapper.RDecapPos", (long)xPL.Decapper.RDecapPos);
    xprintf("\t======================================================================");
    xcprintf(ANSI_TX_ORG);
}

/**
 * @brief PL 조회·설정 형식과 항목별 인덱스 및 허용값 안내를 출력한다.
 * 설정 처리는 RPL_SetParameter가 담당하므로 항목 추가 시 두 함수를 함께 맞춘다.
 */
static void RPL_PrintHelp(void){
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
    xprintf("\t----------------------------------------------------------------------");

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
    xprintf("\t[ Decapper Settable Parameter ]");
    xprintf("\t  [%2d] %-51s : %8.3f : F32, A",
            RPL_SET_DECAP_RUN_CUR_Z, "xPL.Decapper.RunCur[Z]", (double)xPL.Decapper.RunCur[aZ]);
    xprintf("\t  [%2d] %-51s : %8.3f : F32, A",
            RPL_SET_DECAP_RUN_CUR_R, "xPL.Decapper.RunCur[R]", (double)xPL.Decapper.RunCur[aR]);
    xprintf("\t  [%2d] %-51s : %8d : int",
            RPL_SET_DECAP_SEL_MAX_CUR_Z, "xPL.Decapper.SelMaxCur[Z]", xPL.Decapper.SelMaxCur[aZ]);
    xprintf("\t  [%2d] %-51s : %8d : int",
            RPL_SET_DECAP_SEL_MAX_CUR_R, "xPL.Decapper.SelMaxCur[R]", xPL.Decapper.SelMaxCur[aR]);
    xprintf("\t  [%2d] %-51s : %8d : int, 0~100 %%",
            RPL_SET_DECAP_STOP_CUR_RATE_Z, "xPL.Decapper.StopCurRate[Z]", xPL.Decapper.StopCurRate[aZ]);
    xprintf("\t  [%2d] %-51s : %8d : int, 0~100 %%",
            RPL_SET_DECAP_STOP_CUR_RATE_R, "xPL.Decapper.StopCurRate[R]", xPL.Decapper.StopCurRate[aR]);
    xprintf("\t  [%2d] %-51s : %8d : int, > 0",
            RPL_SET_DECAP_STEP_RESOLUTION, "xPL.Decapper.StepResolution", xPL.Decapper.StepResolution);
    xprintf("\t  [%2d] %-51s : %8ld : S32, pulse",
            RPL_SET_DECAP_LIMIT_POS_Z, "xPL.Decapper.Limit_PosZ", (long)xPL.Decapper.Limit_PosZ);
    xprintf("\t  [%2d] %-51s : %8ld : S32, pulse",
            RPL_SET_DECAP_LIMIT_POS_R, "xPL.Decapper.Limit_PosR", (long)xPL.Decapper.Limit_PosR);
    xprintf("\t  [%2d] %-51s : %8ld : S32, pulse",
            RPL_SET_DECAP_SW_NEG_LIMIT_Z, "xPL.Decapper.SwNegLimit[Z]", (long)xPL.Decapper.SwNegLimit[aZ]);
    xprintf("\t  [%2d] %-51s : %8ld : S32, pulse",
            RPL_SET_DECAP_SW_POS_LIMIT_Z, "xPL.Decapper.SwPosLimit[Z]", (long)xPL.Decapper.SwPosLimit[aZ]);
    xprintf("\t  [%2d] %-51s : %8ld : S32, pulse",
            RPL_SET_DECAP_SW_NEG_LIMIT_R, "xPL.Decapper.SwNegLimit[R]", (long)xPL.Decapper.SwNegLimit[aR]);
    xprintf("\t  [%2d] %-51s : %8ld : S32, pulse",
            RPL_SET_DECAP_SW_POS_LIMIT_R, "xPL.Decapper.SwPosLimit[R]", (long)xPL.Decapper.SwPosLimit[aR]);
    xprintf("\t  [%2d] %-51s : %8u : U8, 0/1",
            RPL_SET_DECAP_SOFT_LIMIT_ENABLE, "xPL.Decapper.SoftLimitEnable",
            (unsigned int)xPL.Decapper.SoftLimitEnable);
    xprintf("\t  [%2d] %-51s : %8lu : U32, pulse/s^2",
            RPL_SET_DECAP_Z_ACC, "xPL.Decapper.ZDecapAcc", (unsigned long)xPL.Decapper.ZDecapAcc);
    xprintf("\t  [%2d] %-51s : %8ld : S32, pulse/s",
            RPL_SET_DECAP_Z_VEL, "xPL.Decapper.ZDecapVel", (long)xPL.Decapper.ZDecapVel);
    xprintf("\t  [%2d] %-51s : %8ld : S32, pulse",
            RPL_SET_DECAP_Z_CAP_UP_POS, "xPL.Decapper.ZCap_UpPos", (long)xPL.Decapper.ZCap_UpPos);
    xprintf("\t  [%2d] %-51s : %8ld : S32, pulse",
            RPL_SET_DECAP_Z_CAP_SIDE_POS, "xPL.Decapper.ZCap_SidePos", (long)xPL.Decapper.ZCap_SidePos);
    xprintf("\t  [%2d] %-51s : %8ld : S32, pulse",
            RPL_SET_DECAP_Z_ORIGIN_POS, "xPL.Decapper.ZCap_Origin_Position",
            (long)xPL.Decapper.ZCap_Origin_Position);
    xprintf("\t  [%2d] %-51s : %8lu : U32, pulse/s^2",
            RPL_SET_DECAP_R_ACC, "xPL.Decapper.RDecapAcc", (unsigned long)xPL.Decapper.RDecapAcc);
    xprintf("\t  [%2d] %-51s : %8ld : S32, pulse/s",
            RPL_SET_DECAP_R_VEL, "xPL.Decapper.RDecapVel", (long)xPL.Decapper.RDecapVel);
    xprintf("\t  [%2d] %-51s : %8ld : S32, pulse",
            RPL_SET_DECAP_R_POS, "xPL.Decapper.RDecapPos", (long)xPL.Decapper.RDecapPos);

    xprintf("\t----------------------------------------------------------------------");
    xprintf("\tExample:");
    xprintf("\t  PL %d,70.0      -> CPU overheat criteria = 70.0 degC", RPL_SET_DG_CPU_TEMP_OVERHEAT);
    xprintf("\t  PL %d,20000     -> Z decapper velocity = 20000 pulse/s", RPL_SET_DECAP_Z_VEL);
    xprintf("\t  PL %d,115200    -> Z cap upper position = 115200 pulse", RPL_SET_DECAP_Z_CAP_UP_POS);
    xprintf("\t  PL %d,1         -> Soft limit enable", RPL_SET_DECAP_SOFT_LIMIT_ENABLE);
    xcprintf(ANSI_TX_LightYellow);
    xprintf("\t======================================================================");
    xcprintf(ANSI_TX_ORG);
}

/**
 * @brief PL 조회, USB 도움말, 두 인자 설정을 분기한다.
 * 설정 시 대기 상태 확인 후 유지보수 잠금을 잡고 성공·실패 모두 잠금을 해제한다.
 */
void CMD_Handle_Print_PL(const tsXParsedData *parsedData, U08 useTCP) {
    int index;

    /* Help */
    if ((useTCP == COMM_USB) &&
        (parsedData->ParamCount == 1) &&
        (parsedData->Params[0].value._int == '?')) {
        RPL_PrintHelp();
        return;
    }

    /* Set : PL <idx>,<value> */
    if (parsedData->ParamCount == 2) {
        if (!CDecap_IsIdle()){
            XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);
            return;
        }
        if (GetParamInt(parsedData, 0, &index) == NO) {
            SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
            return;
        }

        if (!CDecap_BeginMaintenance()){
            XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);
            return;
        }
        if (RPL_SetParameter(index, parsedData) == NO) {
            CDecap_EndMaintenance();
            SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
            return;
        }

        CDecap_EndMaintenance();
        XBuffer_AddString(xSendMsg, "OK", NO_COMMA);
        return;
    }

    /* Get */
    if (parsedData->ParamCount == 0) {
        RPL_GetParameter();

        return;
    }

    /* Invalid */
    SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
    XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);

    if (parsedData->ParamCount > 0 &&
        parsedData->Params[0].value._int != '?') {
        xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief ORG 요청을 등록하여 기존 HOME 좌표계의 Z=0 및 Y High 복귀를 실행하게 한다.
 * HOME을 새로 수행하는 명령이 아니며 미원점 상태의 처리는 FSM이 결정한다.
 */
void CMD_Handle_ORIGIN(const tsXParsedData *parsedData, U08 useTCP) {
    if (parsedData->ParamCount == 0){
        if (CMD_RejectIfDecapperBusy())
            return;

        if (!CDecap_TryRequestAction(ACTION_ORIGIN)){

            XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);

            return;

        }
        XBuffer_AddString(xSendMsg, "origin", NO_COMMA);
    }
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief MOVE의 상대/절대 모드(R/A), 축(Z/R), pulse 값을 확인하여 CD에 저장하고 요청한다.
 * 상대 모드의 pulse는 증분, 절대 모드는 좌표이다. 최종 범위 검사는 실제 이동 발행 단계에서 수행한다.
 */
void CMD_Handle_MOVE(const tsXParsedData *parsedData, U08 useTCP) {
    int mode;
    int axis;
    int pulse;

    if ((parsedData->ParamCount == 3) &&
        (GetParamInt(parsedData, 0, &mode) == YES) &&
        (GetParamInt(parsedData, 1, &axis) == YES) &&
        (GetParamInt(parsedData, 2, &pulse) == YES) &&
        ((mode == 'R') || (mode == 'A')) &&
        ((axis == 'Z') || (axis == 'R'))) {
        if (CMD_RejectIfDecapperBusy())
            return;

        if (axis == 'Z') {
            xCD.Decapper.Motor_TargetPos[aZ] = (S32)pulse;
            if (!CDecap_TryRequestAction((mode == 'R') ? ACTION_RMOVEZ : ACTION_AMOVEZ)){
                XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);
                return;
            }
        }
        else {
            xCD.Decapper.Motor_TargetPos[aR] = (S32)pulse;
            if (!CDecap_TryRequestAction((mode == 'R') ? ACTION_RROTATE : ACTION_AROTATE)){
                XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);
                return;
            }
        }

        XBuffer_AddString(xSendMsg, "OK", NO_COMMA);
    }
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if ((parsedData->ParamCount > 0) &&
            (parsedData->Params[0].value._int != '?'))
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief READY 0은 공압 Y High, READY 1은 Y Low 이동을 요청한다.
 * Y는 pulse 위치 제어가 아니며 FSM이 리미트 센서와 타임아웃으로 완료를 판단한다.
 */
void CMD_Handle_READY(const tsXParsedData *parsedData, U08 useTCP) {
    int value;

    if ((parsedData->ParamCount == 1) &&
        (GetParamInt(parsedData, 0, &value) == YES) &&
        ((value == 0) || (value == 1))) {
        if (CMD_RejectIfDecapperBusy())
            return;

        if (!CDecap_TryRequestAction((value == 1) ? ACTION_Y_L_MOVE : ACTION_Y_H_MOVE)){

            XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);

            return;

        }
        XBuffer_AddString(xSendMsg, "OK", NO_COMMA);
    }
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if ((parsedData->ParamCount > 0) &&
            (parsedData->Params[0].value._int != '?'))
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief Y/Body 자동 준비 과정을 생략하는 유닛 DECAP 시험 요청을 등록한다.
 * 대기·오류 확인 후 접수하며 시험에 필요한 기구 준비는 별도로 되어 있어야 한다.
 */
void CMD_Handle_UDECAP(const tsXParsedData *parsedData, U08 useTCP) {
    if (parsedData->ParamCount == 0) {
        if (CMD_RejectIfDecapperBusy())
            return;

        if (!CDecap_TryRequestAction(ACTION_UDECAP)){

            XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);

            return;

        }
        XBuffer_AddString(xSendMsg, "udecap", NO_COMMA);
    }
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief Y/Body 자동 준비 과정을 생략하는 유닛 CAP 시험 요청을 등록한다.
 * 대기·오류 확인 후 접수하며 실제 진행은 유닛 FSM이 담당한다.
 */
void CMD_Handle_UCAP(const tsXParsedData *parsedData, U08 useTCP) {
    if (parsedData->ParamCount == 0) {
        if (CMD_RejectIfDecapperBusy())
            return;

        if (!CDecap_TryRequestAction(ACTION_UCAP)){

            XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);

            return;

        }
        XBuffer_AddString(xSendMsg, "ucap", NO_COMMA);
    }
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief LR/CAPDECAPLR 요청으로 유닛 DECAP/CAP 반복 시험을 시작하게 한다.
 * FSM 진입 시 반복 횟수를 초기화하며 STOP 또는 오류로 종료한다.
 */
void CMD_Handle_LongRun(const tsXParsedData *parsedData, U08 useTCP) {
    /* LR : long-run 시작, 정지는 STOP 명령으로 처리 */
    if (parsedData->ParamCount == 0) {
        if (CMD_RejectIfDecapperBusy())
            return;

        if (!CDecap_TryRequestAction(ACTION_LONGRUN)){

            XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);

            return;

        }
        XBuffer_AddString(xSendMsg, "LR", NO_COMMA);
        return;
    }

    SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
    XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);

    if (parsedData->Params[0].value._int != '?')
        xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
}

/**
 * @brief BGRIP 0/1을 Body 그리퍼 요청값으로 저장하고 수동 출력 액션을 등록한다.
 * OK는 접수 결과이며 그리퍼 센서 도착을 확인한 응답은 아니다.
 */
void CMD_Handle_BGRIP(const tsXParsedData *parsedData, U08 useTCP) {
    int value;

    if ((parsedData->ParamCount == 1) &&
    		(GetParamInt(parsedData, 0, &value) == YES) &&
        ((value == 0) || (value == 1))) {
        if (CMD_RejectIfDecapperBusy())
            return;

        CDecap_SetBodyGripCommand((U08)value);
        if (!CDecap_TryRequestAction(ACTION_BODY_GRIP)){
            XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);
            return;
        }
        XBuffer_AddString(xSendMsg, "OK", NO_COMMA);
    }
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if ((parsedData->ParamCount > 0) &&
            (parsedData->Params[0].value._int != '?'))
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief CGRIP 0/1을 Cap 그리퍼 요청값으로 저장하고 수동 출력 액션을 등록한다.
 * OK는 접수 결과이며 그리퍼 센서 도착을 확인한 응답은 아니다.
 */
void CMD_Handle_CGRIP(const tsXParsedData *parsedData, U08 useTCP) {
    int value;

    if ((parsedData->ParamCount == 1) &&
        (GetParamInt(parsedData, 0, &value) == YES) &&
        ((value == 0) || (value == 1))) {
        if (CMD_RejectIfDecapperBusy())
            return;

        CDecap_SetCapGripCommand((U08)value);
        if (!CDecap_TryRequestAction(ACTION_CAP_GRIP)){
            XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);
            return;
        }
        XBuffer_AddString(xSendMsg, "OK", NO_COMMA);
    }
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if ((parsedData->ParamCount > 0) &&
            (parsedData->Params[0].value._int != '?'))
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief 현재 Z pulse를 조회하고 ZCap_UpPos 저장 후보로 보관한다.
 * 여기서는 PL/EEPROM을 변경하지 않는다. 이후 SAVEE가 후보를 PL에 반영하고 저장한다.
 */
void CMD_Handle_RPOS(const tsXParsedData *parsedData, U08 useTCP) {
    if (parsedData->ParamCount == 0){
        gZCapUpPosPendingValue = CDecap_GetZPosition();
        XBuffer_AddInt(xSendMsg, (int)gZCapUpPosPendingValue, NO_COMMA);
        gZCapUpPosSavePending = true;
        xprintf("Currunt Z_Position = %ld",xCD.Decapper.Motor_CurPos);
    }
    else {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/*Unused Command*/
// debugging code
/**
 * @brief 공통 Busy·오류 상태를 진단 출력하는 미등록 디버그 핸들러이다.
 * 현재 모듈 명령 테이블에서 사용하는 SL/GSTA와 구분한다.
 */
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
