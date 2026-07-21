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

#include "Dev_ServoMotor_A6.h"
#include "panasonic_a6_driver.h"

const tsXCommandMapping gModuleCommandTable[] =
    {
        /** @note USER CODE BEGIN */

        // 공통 명령어: 시스템 상태, 시스템 정보, 롱런 테스트 등, 수정해서 사용하세요.
        {0, "GSTA", /*      */ CMD_Handle_GSTA, /*            */ "Get the system State.", /*              */ "GSTA"},     // 시스템 상태 반환
        {0, "ST", /*        */ CMD_Handle_GSTA, /*            */ "Get the system State.", /*              */ "ST"},       // 시스템 상태 반환
        {1, "PSTA", /*      */ CMD_Handle_PSTA, /*            */ "Get Detailed GSTA Info.", /*            */ "PSTA"},     // 시스템 상세 상태 return
        {1, "MODEL", /*     */ CMD_Handle_MODEL, /*           */ "Get the System info.", /*               */ "MODEL"},    // 시스템 모델 읽기
        {1, "SL", /*        */ CMD_Handle_Print_SL, /*        */ "Print SL.", /*                          */ "SL"},       // SL 출력
        {1, "CD", /*        */ CMD_Handle_Print_CD, /*        */ "Print CD.", /*                          */ "CD"},       // CD 출력
        {1, "PL", /*        */ CMD_Handle_Print_PL, /*        */ "Print PL.", /*                          */ "PL <...>"}, // PL 출력
        {1, "DG", /*        */ CMD_Handle_Print_SBSStateFlags, /* */ "Print SBS state flags.", /*         */ "DG"},       // SBS state flag 출력

        {1, "LR", /*        */ CMD_Handle_LongRun, /*         */ "Long-Run test", /*                      */ "LR <mode>"}, // 롱런 테스트

        /** @note 각 장비에 해당하는 명령어 */
        {0, "ENABLE", /*    */ CMD_Handle_ENABLE, /*          */ "Enable(Step & Servo)", /*        */ "ENABLE"},  // 스텝모터, 서보 Enable
        {0, "DISABLE", /*   */ CMD_Handle_DISABLE, /*         */ "Disable(Step & Servo)", /*       */ "DISABLE"}, // 스텝모터, 서보 Disable

        {0, "MRDO", /*      */ CMD_Handle_MRDO, /*            */ "RobotDoor Open/Close", /*        */ "MRDO <1=Open/0=Close>"}, // 로봇 챔버 도어 개폐 제어 (스텝모터 제어)

        {1, "ORG", /*       */ CMD_Handle_ORG, /*             */ "Origine Operation", /*           */ "ORG"},  // 원심분리기 축 및 도어 초기 위치 복귀 (Homing)
        {0, "HOME", /*      */ CMD_Handle_HOME, /*            */ "Homing Operation", /*            */ "HOME"}, // 원심분리기 축 및 도어 초기 위치 복귀 (Homing)

        {0, "CENT", /*      */ CMD_Handle_CENT, /*            */ "Centrifuge Run", /*              */ "CENT <rpm, time_sec>"}, // 원심분리 구동 명령, rpm 속도와 시간(s) 지정 (서보모터 제어)
        {0, "MOVS", /*      */ CMD_Handle_MOVS, /*            */ "Move Slot", /*                   */ "MOVS <slot(1~6)>"},     // 시료 슬롯 이동 명령 (로봇 내부 샘플 트레이 회전/이동)
        {1, "SLOT", /*      */ CMD_Handle_MOVS, /*            */ "Move Slot", /*                   */ "MOVS <slot(1~6)>"},     // 시료 슬롯 이동 명령 (로봇 내부 샘플 트레이 회전/이동)

        {0, "RESET", /*     */ CMD_Handle_RESET, /*           */ "Emergency Reset", /*             */ "RESET"},             // 비상 정지 이후 시스템 상태 초기화
        {1, "SERV", /*      */ CMD_Handle_SERV, /*            */ "Servo Power On/Off", /*          */ "SERV <1=ON/0=OFF>"}, // 서보모터 전원 제어 (Enable/Disable)
        {1, "MSTP", /*      */ CMD_Handle_STOP, /*            */ "Motor Stop", /*                  */ "MSTP"},              // 정지 명령
        {0, "STOP", /*      */ CMD_Handle_STOP, /*            */ "Motor Stop", /*                  */ "STOP"},              // 정지 명령
        {0, "ESTOP", /*     */ CMD_Handle_ESTOP, /*           */ "Motor EMG-Stop", /*              */ "ESTOP"},             // 즉시 정지 명령

        {1, "SASP", /*      */ CMD_Handle_SASP, /*            */ "Set Auto Slot Position", /*      */ "SASP"},                           // 슬롯 자동위치 티칭 (초기 셋업용)
        {1, "JOGS", /*      */ CMD_Handle_JOGS, /*            */ "Jog Motion", /*                  */ "JOGS <Pulse>"},                   // 조그 이동 명령, dir=CW(1),CCW(0),
        {1, "GPOS", /*      */ CMD_Handle_GPOS, /*            */ "Read current position(pulse)", /**/ "GPOS"},                           // 현재 위치 읽기
        {1, "STIME", /*     */ CMD_Handle_STIME, /*           */ "Set Robo-Door delay time", /*    */ "STIME <close(0)/open(1), msec>"}, // 로봇도어 동작 지연 시간 셋팅
        {1, "SAVEA6", /*    */ CMD_Handle_SAVEA6, /*          */ "Save A6 driver", /*              */ "SAVEA6"},                         // A6 드라이버 EEPROM 저장

        {1, "MOVA", /*      */ CMD_Handle_MOVA, /*            */ "Absolute Move", /*               */ "MOVA <ch, pulse>"}, // 서보/스텝모터 절대 위치 이동 명령 (deg 또는 step 기준)
        {1, "MOVI", /*      */ CMD_Handle_MOVI, /*            */ "Incremental Move", /*            */ "MOVI <ch, p>"},     // 서보/스텝모터 상대 이동 명령 (현재 위치 기준)

        /** @note USER CODE END */
};

const int gModuleCommandCount = sizeof(gModuleCommandTable) / sizeof(tsXCommandMapping);

static int GetParamInt(const tsXParsedData *parsedData, int index, int *out);
static int GetParamFloat(const tsXParsedData *parsedData, int index, float *out);

/*==============================================================================
 * Local helpers
 *============================================================================*/
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
void CMD_Handle_GSTA(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        //  ==============================================================================
        // default info.
        /* [01] */ XBuffer_AddInt(xSendMsg, xSL.isBusy, COMMA); //= (!xServoA6.IsStop() && xDoor.IsStop())
        /* [02] */ XBuffer_AddInt(xSendMsg, xServoA6.IsServoOn(), COMMA);
        /* [03] */ XBuffer_AddInt(xSendMsg, xServoA6.IsHomed(), COMMA);
        /* [04] */ XBuffer_AddInt(xSendMsg, IsError(), COMMA);
        /* [05] */ XBuffer_AddString(xSendMsg, GetErrorCode_char(), COMMA);
        /* [06] */ XBuffer_Addfloat(xSendMsg, xCD.ServoA6.CurrentSpeed.rpm, COMMA);
        /* [07] */ XBuffer_AddInt(xSendMsg, xServoA6.GetPosition_SlotNum() + 1, COMMA);
        /* [08] */ XBuffer_AddInt(xSendMsg, xSL.Door.Status, COMMA);
        /* [09] */ XBuffer_AddInt(xSendMsg, xServoA6.IsDriverError(), COMMA);                // debugging code.
        /* [10] */ XBuffer_AddInt(xSendMsg, xServoA6.IsStop(), COMMA);                       // debugging code.
        /* [11] */ XBuffer_AddInt(xSendMsg, xCD.ServoA6.Centrifugal_Force, COMMA);           // debugging code.
        /* [12] */ XBuffer_AddInt(xSendMsg, xCD.ServoA6.CentCommand.Time_sec_Remain, COMMA); // debugging code.
                                                                                             // TODO: 여기부터, 냉장고 정보
        /* [13] */ XBuffer_Addfloat(xSendMsg, 0.0f, COMMA);                                  // temperature
        /* [14] */ XBuffer_AddInt(xSendMsg, 0, COMMA);                                       // rsv.
        /* [15] */ XBuffer_AddInt(xSendMsg, 0, COMMA);                                       // rsv.
        /* [16] */ XBuffer_AddInt(xSendMsg, 0, COMMA);                                       // rsv.
        /* [17] */ XBuffer_AddInt(xSendMsg, 0, COMMA);                                       // rsv.
        /* [18] */ XBuffer_AddInt(xSendMsg, 0, COMMA);                                       // rsv.
        /* [19] */ XBuffer_AddInt(xSendMsg, 0, COMMA);                                       // rsv.
        /* [20] */ XBuffer_AddInt(xSendMsg, retryCount_RS485[0], NO_COMMA);                  // rsv. // 여기까지 냉장고 온도
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

// debugging code
void CMD_Handle_PSTA(const tsXParsedData *parsedData, U08 useTCP)
{
    U08 i = 1;

    if (parsedData->ParamCount == 0)
    {
        __newLine();
        xcprintf(ANSI_TX_Yellow);
        /*[ 1]*/ xprintf("\t[%2d] %6d : xSL.isBusy", i++, xSL.isBusy);
        /*[ 2]*/ xprintf("\t[%2d] %6d : xServoA6.IsServoOn()", i++, xServoA6.IsServoOn());
        /*[ 3]*/ xprintf("\t[%2d] %6d : xServoA6.IsHomed()", i++, xServoA6.IsHomed());
        /*[ 4]*/ xprintf("\t[%2d] %6d : IsError()", i++, IsError());
        /*[ 5]*/ xprintf("\t[%2d] %6s : error code.", i++, GetErrorCode_char());
        xcprintf(ANSI_TX_Cyan);
        /*[ 6]*/ xprintf("\t[%2d] %6.2f : xCD.ServoA6.CurrentSpeed.rpm", i++, xCD.ServoA6.CurrentSpeed.rpm);
        /*[ 7]*/ xprintf("\t[%2d] %6d : xServoA6.GetPosition_SlotNum() + 1", i++, xServoA6.GetPosition_SlotNum() + 1);
        /*[ 8]*/ xprintf("\t[%2d] %6d : xDoor.GetState(), 0:err, 1:moving, 2:closed, 3:open", i++, xDoor.GetState());
        xcprintf(ANSI_TX_ORG);
        /*[ 9]*/ xprintf("\t[%2d] %6d : xServoA6.IsDriverError()", i++, xServoA6.IsDriverError());
        /*[10]*/ xprintf("\t[%2d] %6d : xServoA6.IsStop()", i++, xServoA6.IsStop());
        /*[11]*/ xprintf("\t[%2d] %6d : xCD.ServoA6.Centrifugal_Force", i++, xCD.ServoA6.Centrifugal_Force);
        /*[12]*/ xprintf("\t[%2d] %6d : xCD.ServoA6.CentCommand.Time_sec_Remain", i++, xCD.ServoA6.CentCommand.Time_sec_Remain);
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

void CMD_Handle_MODEL(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        XBuffer_AddU08(xSendMsg, xSystemInfo.pl_systemType, COMMA);
        XBuffer_AddU08(xSendMsg, xSystemInfo.pl_modelType, NO_COMMA);

        __newLine();
        xprintf("\t[System & Model Information]"); // --> USB port , USER CODE
        xprintf("\t====================================");
        xprintf("\t  ex) MODEL [System Type],[Model Type]");
        xprintf("\t      SYSTEM : " ANSI_TX_LightGreen "%s (%d)" ANSI_TX_ORG, GetSystemTypeString(), xSystemInfo.pl_systemType);
        xprintf("\t      MODEL  : " ANSI_TX_LightGreen "%s (%d)" ANSI_TX_ORG, GetModelTypeString(), xSystemInfo.pl_modelType);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

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
        xprintf("\t  %-42s : %10d", "xSL.LED.Status", xSL.LED.Status);

        xcprintf(ANSI_TX_LightGreen);
        xprintf("\t----------------------------------------------------------------------");
        xprintf("\t[ Robot Door State ]");
        xcprintf(ANSI_TX_ORG);
        xprintf("\t  %-42s : %10d", "xSL.Door.isInitialized", xSL.Door.isInitialized);
        xprintf("\t  %-42s : %10d", "xSL.Door.isControllable", xSL.Door.isControllable);
        xprintf("\t  %-42s : %10d", "xSL.Door.Status  (0:err,1:mv,2:cl,3:op)", xSL.Door.Status);
        xprintf("\t  %-42s : %10d", "xSL.Door.isBusy", xSL.Door.isBusy);
        xprintf("\t  %-42s : %10d", "xSL.Door.isLogicRunning", xSL.Door.isLogicRunning);
        xprintf("\t  %-42s : %10d", "xSL.Door.Motor.isEnabled", xSL.Door.Motor.isEnabled);
        xprintf("\t  %-42s : %10d", "xSL.Door.Motor.isMoving", xSL.Door.Motor.isMoving);

        xcprintf(ANSI_TX_LightGreen);
        xprintf("\t----------------------------------------------------------------------");
        xprintf("\t[ Servo A6 State ]");
        xcprintf(ANSI_TX_ORG);
        xprintf("\t  %-42s : %10d", "xSL.ServoA6.isInitialized", xSL.ServoA6.isInitialized);
        xprintf("\t  %-42s : %10d", "xSL.ServoA6.isControllable", xSL.ServoA6.isControllable);
        xprintf("\t  %-42s : %10d", "xSL.ServoA6.Driver.isConnected", xSL.ServoA6.Driver.isConnected);
        xprintf("\t  %-42s : %10d", "xSL.ServoA6.Driver.isEnabled", xSL.ServoA6.Driver.isEnabled);
        xprintf("\t  %-42s : %10d", "xSL.ServoA6.Driver.isMoving", xSL.ServoA6.Driver.isMoving);
        xprintf("\t  %-42s : %10d", "xSL.ServoA6.Driver.isHomed", xSL.ServoA6.Driver.isHomed);
        xprintf("\t  %-42s : %10d", "xSL.ServoA6.Driver.isInPosition", xSL.ServoA6.Driver.isInPosition);
        xprintf("\t  %-42s : %10d", "xSL.ServoA6.Driver.isError", xSL.ServoA6.Driver.isError);
        xprintf("\t  %-42s : %10d", "xSL.ServoA6.Driver.ErrorCode", xSL.ServoA6.Driver.ErrorCode);
        xprintf("\t  %-42s : %10d", "xSL.ServoA6.isLogicRunning", xSL.ServoA6.isLogicRunning);
        xprintf("\t  %-42s : %10d", "xSL.ServoA6.isCentrifugeRunning", xSL.ServoA6.isCentrifugeRunning);
        xprintf("\t  %-42s : %10d", "xSL.ServoA6.isBusy", xSL.ServoA6.isBusy);
        xprintf("\t  %-42s : %10d", "xSL.ServoA6.isHomed", xSL.ServoA6.isHomed);

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
        xprintf("\t  %-42s : %10d", "xCD.LED.Out", xCD.LED.Out);

        xcprintf(ANSI_TX_LightGreen);
        xprintf("\t----------------------------------------------------------------------");
        xprintf("\t[ xCD.Door : Robot Door I/O ]");
        xcprintf(ANSI_TX_ORG);
        xprintf("\t  %-42s : %10d", "xCD.Door.CloseSensor", xCD.Door.CloseSensor);
        xprintf("\t  %-42s : %10d", "xCD.Door.OpenSensor", xCD.Door.OpenSensor);
        xprintf("\t  %-42s : %10lu pulse", "xCD.Door.CurrentPosition", (unsigned long)xCD.Door.CurrentPosition);
        xprintf("\t  %-42s : %10lu", "xCD.Door.TotalRetryCount", (unsigned long)xCD.Door.TotalRetryCount);

        xcprintf(ANSI_TX_LightCyan);
        xprintf("\t----------------------------------------------------------------------");
        xprintf("\t[ xCD.ServoA6 : Speed / Position / Centrifuge ]");
        xcprintf(ANSI_TX_ORG);
        xprintf("\t  %-42s : %10ld pps", "xCD.ServoA6.CurrentSpeed.pps", (long)xCD.ServoA6.CurrentSpeed.pps);
        xprintf("\t  %-42s : %10.3f rps", "xCD.ServoA6.CurrentSpeed.rps", (double)xCD.ServoA6.CurrentSpeed.rps);
        xprintf("\t  %-42s : %10.3f rpm", "xCD.ServoA6.CurrentSpeed.rpm", (double)xCD.ServoA6.CurrentSpeed.rpm);
        xprintf("\t  %-42s : %10ld pulse", "xCD.ServoA6.CurrentPosition.Pulse", (long)xCD.ServoA6.CurrentPosition.Pulse);
        xprintf("\t  %-42s : %10d", "xCD.ServoA6.CurrentPosition.SlotNum", xCD.ServoA6.CurrentPosition.SlotNum);
        xprintf("\t  %-42s : %10.3f", "xCD.ServoA6.Centrifugal_Force", (double)xCD.ServoA6.Centrifugal_Force);
        xprintf("\t  %-42s : %10.3f rpm", "xCD.ServoA6.CentCommand.rpm", (double)xCD.ServoA6.CentCommand.rpm);
        xprintf("\t  %-42s : %10lu ms", "xCD.ServoA6.CentCommand.Time_msec_Accel", (unsigned long)xCD.ServoA6.CentCommand.Time_msec_Accel);
        xprintf("\t  %-42s : %10lu ms", "xCD.ServoA6.CentCommand.Time_msec_Run", (unsigned long)xCD.ServoA6.CentCommand.Time_msec_Run);
        xprintf("\t  %-42s : %10lu ms", "xCD.ServoA6.CentCommand.Time_msec_Decel", (unsigned long)xCD.ServoA6.CentCommand.Time_msec_Decel);
        xprintf("\t  %-42s : %10lu ms", "xCD.ServoA6.CentCommand.Time_msec_Total", (unsigned long)xCD.ServoA6.CentCommand.Time_msec_Total);
        xprintf("\t  %-42s : %10ld sec", "xCD.ServoA6.CentCommand.Time_sec_Remain", (long)xCD.ServoA6.CentCommand.Time_sec_Remain);

        xcprintf(ANSI_TX_LightCyan);
        xprintf("\t----------------------------------------------------------------------");
        xprintf("\t[ xCD.SL Snapshot ]");
        xcprintf(ANSI_TX_ORG);
        xprintf("\t  %-42s : %10d", "xCD.SL.isBusy", xCD.SL.isBusy);
        xprintf("\t  %-42s : %10d", "xCD.SL.Door.Status", xCD.SL.Door.Status);
        xprintf("\t  %-42s : %10d", "xCD.SL.ServoA6.isHomed", xCD.SL.ServoA6.isHomed);
        xprintf("\t  %-42s : %10d", "xCD.SL.ServoA6.isBusy", xCD.SL.ServoA6.isBusy);
        xprintf("\t  %-42s : %10d", "xCD.SL.ServoA6.isCentrifugeRunning", xCD.SL.ServoA6.isCentrifugeRunning);

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

typedef enum
{
    /* --- Diagnose (PL Header) --- */
    RPL_SET_DG_CPU_TEMP_OVERHEAT = 1,
    RPL_SET_DG_CPU_TEMP_ALARM_INTERVAL,
    RPL_SET_DG_IS_DIAGNOSIS_ENABLED,
    RPL_SET_DG_CLIENT2HOST_LOGMODE,

    /* --- Robot Door --- */
    RPL_SET_DOOR_CONTROL_TIMEOUT,
    RPL_SET_DOOR_CLOSE_OVERTIME,
    RPL_SET_DOOR_OPEN_OVERTIME,
    RPL_SET_DOOR_RELATIVE_DISTANCE,
    RPL_SET_DOOR_MOTOR_SPEED,
    RPL_SET_DOOR_MOTOR_ACCEL,

    /* --- Servo A6 --- */
    RPL_SET_SERVO_HOME_FWD_SPEED,
    RPL_SET_SERVO_HOME_BWD_SPEED,
    RPL_SET_SERVO_HOME_ACCEL,
    RPL_SET_SERVO_SLOT_SPEED,
    RPL_SET_SERVO_SLOT_ACCEL,
    RPL_SET_SERVO_SLOT_DECEL,
    RPL_SET_SERVO_JOG_SPEED,
    RPL_SET_SERVO_BASE_SPEED,

} teRPL_SetIndex;

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

    xprintf("\t  [Idx] %-50s : %8s : %s",
            "Parameter",
            "Current",
            "Type");

    /* --- Diagnose (PL Header) --- */
    xprintf("\t  [%2d] %-51s : %8.3f : float, degC",
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
    xprintf("\t  [%2d] %-51s : %8d : int, ms",
            RPL_SET_DOOR_CONTROL_TIMEOUT,
            "xPL.Door.Door_ControlTimeout_ms",
            xPL.Door.Door_ControlTimeout_ms);

    xprintf("\t  [%2d] %-51s : %8lu : U32, ms",
            RPL_SET_DOOR_CLOSE_OVERTIME,
            "xPL.Door.CloseSensorOverTime_ms",
            (unsigned long)xPL.Door.CloseSensorOverTime_ms);

    xprintf("\t  [%2d] %-51s : %8lu : U32, ms",
            RPL_SET_DOOR_OPEN_OVERTIME,
            "xPL.Door.OpenSensorOverTime_ms",
            (unsigned long)xPL.Door.OpenSensorOverTime_ms);

    xprintf("\t  [%2d] %-51s : %8lu : U32, pulse",
            RPL_SET_DOOR_RELATIVE_DISTANCE,
            "xPL.Door.RelativeDistance_Count",
            (unsigned long)xPL.Door.RelativeDistance_Count);

    xprintf("\t  [%2d] %-51s : %8lu : U32, pps",
            RPL_SET_DOOR_MOTOR_SPEED,
            "xPL.Door.Motor.Speed_pps",
            (unsigned long)xPL.Door.Motor.Speed_pps);

    xprintf("\t  [%2d] %-51s : %8lu : U32, ppss",
            RPL_SET_DOOR_MOTOR_ACCEL,
            "xPL.Door.Motor.Accel_ppss",
            (unsigned long)xPL.Door.Motor.Accel_ppss);

    xprintf("\t----------------------------------------------------------------------");

    /* --- Servo A6 --- */
    xprintf("\t  [%2d] %-51s : %8.3f : float, rpm",
            RPL_SET_SERVO_HOME_FWD_SPEED,
            "xPL.ServoA6.Home.Speed_Forward_rpm",
            (double)xPL.ServoA6.Home.Speed_Forward_rpm);

    xprintf("\t  [%2d] %-51s : %8.3f : float, rpm",
            RPL_SET_SERVO_HOME_BWD_SPEED,
            "xPL.ServoA6.Home.Speed_Backward_rpm",
            (double)xPL.ServoA6.Home.Speed_Backward_rpm);

    xprintf("\t  [%2d] %-51s : %8.3f : float, ms",
            RPL_SET_SERVO_HOME_ACCEL,
            "xPL.ServoA6.Home.Time_Accel_millis",
            (double)xPL.ServoA6.Home.Time_Accel_millis);

    xprintf("\t  [%2d] %-51s : %8.3f : float, rpm",
            RPL_SET_SERVO_SLOT_SPEED,
            "xPL.ServoA6.Slot.Speed_rpm",
            (double)xPL.ServoA6.Slot.Speed_rpm);

    xprintf("\t  [%2d] %-51s : %8.3f : float, ms",
            RPL_SET_SERVO_SLOT_ACCEL,
            "xPL.ServoA6.Slot.Time_Accel_millis",
            (double)xPL.ServoA6.Slot.Time_Accel_millis);

    xprintf("\t  [%2d] %-51s : %8.3f : float, ms",
            RPL_SET_SERVO_SLOT_DECEL,
            "xPL.ServoA6.Slot.Time_Decel_millis",
            (double)xPL.ServoA6.Slot.Time_Decel_millis);

    xprintf("\t  [%2d] %-51s : %8.3f : float, rpm",
            RPL_SET_SERVO_JOG_SPEED,
            "xPL.ServoA6.Jog.Speed_rpm",
            (double)xPL.ServoA6.Jog.Speed_rpm);

    xprintf("\t  [%2d] %-51s : %8.3f : float, rpm",
            RPL_SET_SERVO_BASE_SPEED,
            "xPL.ServoA6.BaseMove.Speed_rpm",
            (double)xPL.ServoA6.BaseMove.Speed_rpm);

    xprintf("\t----------------------------------------------------------------------");
    xprintf("\tExample:");
    xprintf("\t  PL 1,70.0     -> CPU overheat criteria = 70.0 degC");
    xprintf("\t  PL 5,10000    -> Robot door control timeout = 10000 ms");
    xprintf("\t  PL 9,20000    -> Robot door motor speed = 20000 pps");
    xprintf("\t  PL 11,30.0    -> Servo home forward speed = 30.0 rpm");

    xcprintf(ANSI_TX_LightYellow);
    xprintf("\t======================================================================");
    xcprintf(ANSI_TX_ORG);
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
    case RPL_SET_DOOR_CONTROL_TIMEOUT:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;

        xPL.Door.Door_ControlTimeout_ms = value_i;
        LOG_MSG_SEND("[PL] Set Door.Door_ControlTimeout_ms = %d ms", xPL.Door.Door_ControlTimeout_ms);
        return YES;

    case RPL_SET_DOOR_CLOSE_OVERTIME:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;

        xPL.Door.CloseSensorOverTime_ms = (U32)value_i;
        LOG_MSG_SEND("[PL] Set Door.CloseSensorOverTime_ms = %lu ms",
                     (unsigned long)xPL.Door.CloseSensorOverTime_ms);
        return YES;

    case RPL_SET_DOOR_OPEN_OVERTIME:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;

        xPL.Door.OpenSensorOverTime_ms = (U32)value_i;
        LOG_MSG_SEND("[PL] Set Door.OpenSensorOverTime_ms = %lu ms",
                     (unsigned long)xPL.Door.OpenSensorOverTime_ms);
        return YES;

    case RPL_SET_DOOR_RELATIVE_DISTANCE:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;

        xPL.Door.RelativeDistance_Count = (U32)value_i;
        LOG_MSG_SEND("[PL] Set Door.RelativeDistance_Count = %lu pulse",
                     (unsigned long)xPL.Door.RelativeDistance_Count);
        return YES;

    case RPL_SET_DOOR_MOTOR_SPEED:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;

        xPL.Door.Motor.Speed_pps = (U32)value_i;
        LOG_MSG_SEND("[PL] Set Door.Motor.Speed_pps = %lu pps",
                     (unsigned long)xPL.Door.Motor.Speed_pps);
        return YES;

    case RPL_SET_DOOR_MOTOR_ACCEL:
        if (GetParamInt(parsedData, 1, &value_i) == NO || value_i < 0)
            return NO;

        xPL.Door.Motor.Accel_ppss = (U32)value_i;
        LOG_MSG_SEND("[PL] Set Door.Motor.Accel_ppss = %lu ppss",
                     (unsigned long)xPL.Door.Motor.Accel_ppss);
        return YES;

    /* --- Servo A6 --- */
    case RPL_SET_SERVO_HOME_FWD_SPEED:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        xPL.ServoA6.Home.Speed_Forward_rpm = value_f;
        LOG_MSG_SEND("[PL] Set ServoA6.Home.Speed_Forward_rpm = %.3f rpm", (double)value_f);
        return YES;

    case RPL_SET_SERVO_HOME_BWD_SPEED:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        xPL.ServoA6.Home.Speed_Backward_rpm = value_f;
        LOG_MSG_SEND("[PL] Set ServoA6.Home.Speed_Backward_rpm = %.3f rpm", (double)value_f);
        return YES;

    case RPL_SET_SERVO_HOME_ACCEL:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        xPL.ServoA6.Home.Time_Accel_millis = value_f;
        LOG_MSG_SEND("[PL] Set ServoA6.Home.Time_Accel_millis = %.3f ms", (double)value_f);
        return YES;

    case RPL_SET_SERVO_SLOT_SPEED:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        xPL.ServoA6.Slot.Speed_rpm = value_f;
        LOG_MSG_SEND("[PL] Set ServoA6.Slot.Speed_rpm = %.3f rpm", (double)value_f);
        return YES;

    case RPL_SET_SERVO_SLOT_ACCEL:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        xPL.ServoA6.Slot.Time_Accel_millis = value_f;
        LOG_MSG_SEND("[PL] Set ServoA6.Slot.Time_Accel_millis = %.3f ms", (double)value_f);
        return YES;

    case RPL_SET_SERVO_SLOT_DECEL:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        xPL.ServoA6.Slot.Time_Decel_millis = value_f;
        LOG_MSG_SEND("[PL] Set ServoA6.Slot.Time_Decel_millis = %.3f ms", (double)value_f);
        return YES;

    case RPL_SET_SERVO_JOG_SPEED:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        xPL.ServoA6.Jog.Speed_rpm = value_f;
        LOG_MSG_SEND("[PL] Set ServoA6.Jog.Speed_rpm = %.3f rpm", (double)value_f);
        return YES;

    case RPL_SET_SERVO_BASE_SPEED:
        if (GetParamFloat(parsedData, 1, &value_f) == NO)
            return NO;

        xPL.ServoA6.BaseMove.Speed_rpm = value_f;
        LOG_MSG_SEND("[PL] Set ServoA6.BaseMove.Speed_rpm = %.3f rpm", (double)value_f);
        return YES;

    default:
        return NO;
    }
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

    /* Set */
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
        xprintf("\t  %-42s : %10d", "xPL.LED.CMD_StartControl", xPL.LED.CMD_StartControl);
        xprintf("\t  %-42s : %10d", "xPL.LED.OnOff", xPL.LED.OnOff);

        xcprintf(ANSI_TX_LightGreen);
        xprintf("\t----------------------------------------------------------------------");
        xprintf("\t[ Robot Door Parameter ]");
        xcprintf(ANSI_TX_ORG);
        xprintf("\t  %-42s : %10d", "xPL.Door.CMD_StartControl", xPL.Door.CMD_StartControl);
        xprintf("\t  %-42s : %10d", "xPL.Door.TargetMotion", xPL.Door.TargetMotion);
        xprintf("\t  %-42s : %10d", "xPL.Door.Direction", xPL.Door.Direction);
        xprintf("\t  %-42s : %10d ms", "xPL.Door.Door_ControlTimeout_ms", xPL.Door.Door_ControlTimeout_ms);
        xprintf("\t  %-42s : %10lu ms", "xPL.Door.CloseSensorOverTime_ms",
                (unsigned long)xPL.Door.CloseSensorOverTime_ms);
        xprintf("\t  %-42s : %10lu ms", "xPL.Door.OpenSensorOverTime_ms",
                (unsigned long)xPL.Door.OpenSensorOverTime_ms);
        xprintf("\t  %-42s : %10lu pulse", "xPL.Door.RelativeDistance_Count",
                (unsigned long)xPL.Door.RelativeDistance_Count);
        xprintf("\t  %-42s : %10lu pps", "xPL.Door.Motor.Speed_pps",
                (unsigned long)xPL.Door.Motor.Speed_pps);
        xprintf("\t  %-42s : %10lu ppss", "xPL.Door.Motor.Accel_ppss",
                (unsigned long)xPL.Door.Motor.Accel_ppss);
        xprintf("\t  %-42s : %10.3f A", "xPL.Door.Motor.NormalCurrent_A",
                (double)xPL.Door.Motor.NormalCurrent_A);
        xprintf("\t  %-42s : %10lu %%", "xPL.Door.Motor.HoldingCurrent_Percent",
                (unsigned long)xPL.Door.Motor.HoldingCurrent_Percent);
        xprintf("\t  %-42s : %10d", "xPL.Door.Motor.Resolution", xPL.Door.Motor.Resolution);

        xcprintf(ANSI_TX_LightMagenta);
        xprintf("\t----------------------------------------------------------------------");
        xprintf("\t[ Servo A6 Parameter ]");
        xcprintf(ANSI_TX_ORG);
        xprintf("\t  %-42s : %10d", "xPL.ServoA6.CMD_StartControl", xPL.ServoA6.CMD_StartControl);
        xprintf("\t  %-42s : %10d", "xPL.ServoA6.CMD_ControlMode", xPL.ServoA6.CMD_ControlMode);
        xprintf("\t  %-42s : %10d", "xPL.ServoA6.Direction", xPL.ServoA6.Direction);
        xprintf("\t  %-42s : %10ld", "xPL.ServoA6.PulsePerRevolution", (long)xPL.ServoA6.PulsePerRevolution);

        xprintf("\t  %-42s : %10.3f rpm", "xPL.ServoA6.Home.Speed_Forward_rpm",
                (double)xPL.ServoA6.Home.Speed_Forward_rpm);
        xprintf("\t  %-42s : %10.3f rpm", "xPL.ServoA6.Home.Speed_Backward_rpm",
                (double)xPL.ServoA6.Home.Speed_Backward_rpm);
        xprintf("\t  %-42s : %10.3f ms", "xPL.ServoA6.Home.Time_Accel_millis",
                (double)xPL.ServoA6.Home.Time_Accel_millis);

        xprintf("\t  %-42s : %10.3f rpm", "xPL.ServoA6.Slot.Speed_rpm",
                (double)xPL.ServoA6.Slot.Speed_rpm);
        xprintf("\t  %-42s : %10.3f ms", "xPL.ServoA6.Slot.Time_Accel_millis",
                (double)xPL.ServoA6.Slot.Time_Accel_millis);
        xprintf("\t  %-42s : %10.3f ms", "xPL.ServoA6.Slot.Time_Decel_millis",
                (double)xPL.ServoA6.Slot.Time_Decel_millis);

        for (int slot = 0; slot < SLOT_COUNT; slot++)
        {
            xprintf("\t  %-38s[%d] : %10lu pulse",
                    "xPL.ServoA6.Slot.PositionOffset_pulse", slot,
                    (unsigned long)xPL.ServoA6.Slot.PositionOffset_pulse[slot]);
        }

        xprintf("\t  %-42s : %10.3f rpm", "xPL.ServoA6.Jog.Speed_rpm",
                (double)xPL.ServoA6.Jog.Speed_rpm);
        xprintf("\t  %-42s : %10.3f ms", "xPL.ServoA6.Jog.Time_Accel_millis",
                (double)xPL.ServoA6.Jog.Time_Accel_millis);
        xprintf("\t  %-42s : %10.3f ms", "xPL.ServoA6.Jog.Time_Decel_millis",
                (double)xPL.ServoA6.Jog.Time_Decel_millis);

        xprintf("\t  %-42s : %10.3f rpm", "xPL.ServoA6.BaseMove.Speed_rpm",
                (double)xPL.ServoA6.BaseMove.Speed_rpm);
        xprintf("\t  %-42s : %10.3f ms", "xPL.ServoA6.BaseMove.Time_Accel_millis",
                (double)xPL.ServoA6.BaseMove.Time_Accel_millis);
        xprintf("\t  %-42s : %10.3f ms", "xPL.ServoA6.BaseMove.Time_Decel_millis",
                (double)xPL.ServoA6.BaseMove.Time_Decel_millis);

        xcprintf(ANSI_TX_LightYellow);
        xprintf("\t======================================================================");
        xcprintf(ANSI_TX_ORG);

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

//======================================================================================
typedef struct
{
    U32 mask;
    const char *name;
    const char *desc;

} tsSBSStateFlagInfo;

static const tsSBSStateFlagInfo gSBSStateFlagInfo[] =
    {
        //        {SBS_STATE_LOADER_INIT, "SBS_STATE_LOADER_INIT", "Loader initialize completed."},
        //        {SBS_STATE_LOADER_HOMED, "SBS_STATE_LOADER_HOMED", "Loader home completed."},
        //        {SBS_STATE_LOADER_BUSY, "SBS_STATE_LOADER_BUSY", "Loader running."},
        //
        //        {SBS_STATE_SOL_INIT, "SBS_STATE_SOL_INIT", "Solvalve initialize completed."},
        //        {SBS_STATE_SOL_BUSY, "SBS_STATE_SOL_BUSY", "Solvalve running."},
        //
        //        {SBS_STATE_SENSOR_ERROR, "SBS_STATE_SENSOR_ERROR", "Sensor error exists."},
        //        {SBS_STATE_SBS_DETECTED, "SBS_STATE_SBS_DETECTED", "SBS detected."},
        //
        //        {SBS_STATE_GRIPPER_OPEN, "SBS_STATE_GRIPPER_OPEN", "Gripper open."},
        //        {SBS_STATE_GRIPPER_CLOSE, "SBS_STATE_GRIPPER_CLOSE", "Gripper close."},
        //        {SBS_STATE_GRIPPER_GRIP, "SBS_STATE_GRIPPER_GRIP", "Gripper grip."},
        //        {SBS_STATE_GRIPPER_ERROR, "SBS_STATE_GRIPPER_ERROR", "Gripper sensor error."},
        //
        //        {SBS_STATE_Z_UP, "SBS_STATE_Z_UP", "Z axis up."},
        //        {SBS_STATE_Z_MIDDLE, "SBS_STATE_Z_MIDDLE", "Z axis middle."},
        //        {SBS_STATE_Z_DOWN, "SBS_STATE_Z_DOWN", "Z axis down."},
        //        {SBS_STATE_Z_ERROR, "SBS_STATE_Z_ERROR", "Z axis sensor error."},
        //
        //        {SBS_STATE_PUSHER_UP, "SBS_STATE_PUSHER_UP", "Pusher up."},
        //        {SBS_STATE_PUSHER_DOWN, "SBS_STATE_PUSHER_DOWN", "Pusher down."},
        //
        //        {SBS_STATE_ROTATE_COMBI, "SBS_STATE_ROTATE_COMBI", "Rotate combi position."},
        //        {SBS_STATE_ROTATE_BUFFER, "SBS_STATE_ROTATE_BUFFER", "Rotate buffer position."},
        //        {SBS_STATE_ROTATE_MOVING, "SBS_STATE_ROTATE_MOVING", "Rotate moving."},
        //        {SBS_STATE_ROTATE_ERROR, "SBS_STATE_ROTATE_ERROR", "Rotate sensor error."},
};

void CMD_Handle_Print_SBSStateFlags(const tsXParsedData *parsedData, U08 useTCP)
{
    // U32 state;
    // U08 i;
    // int isOn;

    // (void)useTCP;

    // if ((useTCP == COMM_USB) &&
    //     (parsedData->ParamCount == 1) &&
    //     (parsedData->Params[0].value._int == '?'))
    // {
    //     xprintf("\tCommand: [DG]");
    //     xprintf("\t==============================================================");
    //     xprintf("\tDG : Print SBS Loader state flags for diagnostic-Policy.");
    //     return;
    // }

    // if (parsedData->ParamCount != 0)
    // {
    //     SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
    //     XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);

    //     if (parsedData->Params[0].value._int != '?')
    //         xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);

    //     return;
    // }

    // state = SDG_GetStateFlags();

    // xcprintf(ANSI_TX_LightYellow);
    // xprintf("\t======================================================================");
    // xprintf("\t                    [ SBS Loader State Flags : DG ]                  ");
    // xprintf("\t======================================================================");
    // xcprintf(ANSI_TX_ORG);

    // xcprintf(ANSI_TX_LightCyan);
    // xprintf("\t  State Mask : 0x%08lX", (unsigned long)state);
    // xprintf("\t----------------------------------------------------------------------");
    // xprintf("\t  [No] [Bit] [Val] [Mask]       [Name]                         [Desc]");
    // xprintf("\t----------------------------------------------------------------------");
    // xcprintf(ANSI_TX_ORG);

    // for (i = 0; i < ARRAY_SIZE(gSBSStateFlagInfo); i++)
    // {
    //     isOn = ((state & gSBSStateFlagInfo[i].mask) != 0U) ? 1 : 0;

    //     if (isOn)
    //         xcprintf(ANSI_TX_LightGreen);
    //     else
    //         xcprintf(ANSI_TX_White);

    //     xprintf("\t  [%2d] [%2d]   %d   0x%08lX  %-30s %s",
    //             i + 1,
    //             i,
    //             isOn,
    //             (unsigned long)gSBSStateFlagInfo[i].mask,
    //             gSBSStateFlagInfo[i].name,
    //             gSBSStateFlagInfo[i].desc);

    //     xcprintf(ANSI_TX_ORG);
    // }

    // xcprintf(ANSI_TX_LightYellow);
    // xprintf("\t======================================================================");
    // xcprintf(ANSI_TX_ORG);
}
//======================================================================================

void CMD_Handle_LongRun(const tsXParsedData *parsedData, U08 useTCP)
{
    // int mode;
    // int delay;
    // U32 delay_ms = DEV_TEST_LR_DEFAULT_DELAY_ms;

    // if (IS_USB(useTCP) && IS_CMD_HELP(parsedData))
    // {
    //     DevTest_PrintLongRunHelp();
    //     return;
    // }

    // /*
    //  * LR : long-run 정지
    //  * 현재 진행 중인 FSM 동작은 강제 중단하지 않고,
    //  * 다음 반복 명령만 더 이상 내리지 않는다.
    //  */
    // if (parsedData->ParamCount == 0)
    // {
    //     DevTest_StopLongRun();
    //     XBuffer_AddString(xSendMsg, "STOP", NO_COMMA);
    //     return;
    // }

    // /*
    //  * LR <mode>            : default delay 적용
    //  * LR <mode>,<delay_ms> : 입력 delay 적용
    //  */
    // if ((parsedData->ParamCount == 1 || parsedData->ParamCount == 2) &&
    //     (GetParamInt(parsedData, 0, &mode) == YES) &&
    //     (mode >= DEV_TEST_LR_OPEN_CLOSE) &&
    //     (mode <= DEV_TEST_LR_MCOMB_GLID_PLID_MBUFF))
    // {
    //     if (parsedData->ParamCount == 2)
    //     {
    //         if ((GetParamInt(parsedData, 1, &delay) == NO) || (delay < 0))
    //         {
    //             SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
    //             XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
    //             return;
    //         }

    //         delay_ms = (U32)delay;
    //     }

    //     if (DevTest_StartLongRun(mode, delay_ms) == YES)
    //     {
    //         XBuffer_AddString(xSendMsg, "START", COMMA);
    //         XBuffer_AddInt(xSendMsg, mode, COMMA);
    //         XBuffer_AddInt(xSendMsg, (int)delay_ms, COMMA);
    //         XBuffer_AddString(xSendMsg, DevTest_GetLongRunModeStr(mode), NO_COMMA);
    //     }
    //     else
    //     {
    //         XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
    //     }

    //     return;
    // }

    // SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
    // XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);

    // if (parsedData->Params[0].value._int != '?')
    //     xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
}

//======================================================================================
//======================================================================================
//======================================================================================
//======================================================================================
//======================================================================================
//======================================================================================
//======================================================================================
void CMD_Handle_ENABLE(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        xPL.ServoA6.CMD_StartControl = YES;
        xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_ENABLE;

        xDoor.Motor.Enable();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_DISABLE(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        xPL.ServoA6.CMD_StartControl = YES;
        xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_DISABLE;

        xDoor.Motor.Disable();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_SAVEA6(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        XTimer_Stop();
        xServoA6.Set_Param_Home();
        PanasonicA6_EEPROM_Write(A6_ID);
        XTimer_Start();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_MRDO(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 1)
    {
        int command = parsedData->Params[0].value._int;

        if (command == '?')
        {
            __newLine();
            xprintf("\t== MRDO <open=1, close=0> ==");
            xprintf("\t  MRDO 0  : CLOSE");
            xprintf("\t  MRDO 1  : OPEN");
            xprintf("\t  MRDO 2  : STOP");
        }
        else if (command >= ROBOTDOOR_COMMAND_CLOSE && command <= ROBOTDOOR_COMMAND_STOP)
        {
            if (SDG_RobotDoor_CheckError(command))
            {
                XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
            }
            else
            {
                xPL.Door.CMD_StartControl = YES;
                xPL.Door.TargetMotion = command;
            }
        }
        else
        {
            SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
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

void CMD_Handle_ORG(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (SDG_ServoA6_CheckError((int)A6_CONTROL_MODE_ORG))
        {
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        }
        else
        {
            xPL.ServoA6.CMD_StartControl = YES;
            xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_ORG;
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

void CMD_Handle_HOME(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (SDG_ServoA6_CheckError((int)A6_CONTROL_MODE_HOME))
        {
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        }
        else
        {
            xPL.ServoA6.CMD_StartControl = YES;
            xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_HOME;
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

void CMD_Handle_CENT(const tsXParsedData *parsedData, U08 useTCP)
{
    float rpm;   //
    U32 runTime; // [sec] --> T2
    // U32 totalTime; // [sec] --> T1 + T2 + T3

    if (parsedData->ParamCount == 2)
    {
        if (parsedData->Params[0].type == PARAM_TYPE_INT)
            rpm = (float)parsedData->Params[0].value._int;
        else
            rpm = parsedData->Params[0].value._float;

        runTime = parsedData->Params[1].value._int;

        if (SDG_ServoA6_CheckError((int)A6_CONTROL_MODE_CENT))
        {
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        }
        else if (xDoor.IsOpen())
        {
            SetErrorCode(ERROR_CODE_CENT_BLOCKED_DOOR_OPEN, __func__, __LINE__);
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        }
        else if (!xServoA6.IsHomed())
        {
            SetErrorCode(ERROR_CODE_A6_SERVO_NOT_HOME, __func__, __LINE__);
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        }
        else
        {
            xPL.ServoA6.CMD_StartControl = YES;
            xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_CENT;
            xPL.ServoA6.CMD_Param.Cent.Speed_rpm = rpm;
            xPL.ServoA6.CMD_Param.Cent.Time_sec = runTime;

            xCD.ServoA6.CentCommand.rpm = (F32)rpm;
            xCD.ServoA6.CentCommand.Time_msec_Run = runTime * 1000;
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

// move to slot
void CMD_Handle_MOVS(const tsXParsedData *parsedData, U08 useTCP)
{
    int slotNum;

    if (parsedData->ParamCount == 1)
    {
        slotNum = parsedData->Params[0].value._int - 1;

        if (SDG_ServoA6_CheckError((int)A6_CONTROL_MODE_SLOT))
        {
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        }
        // 유지보수를 위해 도어가 열려 있어도 실행되어야 함.
        // else if (xDoor.IsOpen())
        // {
        //     SetErrorCode(ERROR_CODE_CENT_BLOCKED_DOOR_OPEN);
        //     XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        // }
        else if (!(SLOT_1 <= slotNum && slotNum <= SLOT_6))
        {
            SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        }
        else
        {
            xPL.ServoA6.CMD_StartControl = YES;
            xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_SLOT;
            xPL.ServoA6.CMD_Param.SlotNum = slotNum;
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

void CMD_Handle_RESET(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        xPL.ServoA6.CMD_StartControl = YES;
        xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_RESET;
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_SERV(const tsXParsedData *parsedData, U08 useTCP)
{
    int onOff;

    if (parsedData->ParamCount == 1)
    {
        onOff = parsedData->Params[0].value._int;

        if (onOff != OFF && onOff != ON)
        {
            SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        }
        else
        {
            xPL.ServoA6.CMD_StartControl = YES;

            if (onOff == ON)
                xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_ENABLE;
            else
                xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_DISABLE;
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

void CMD_Handle_STOP(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        xPL.Door.CMD_StartControl = YES;
        xPL.Door.TargetMotion = ROBOTDOOR_COMMAND_STOP;

        xPL.ServoA6.CMD_StartControl = YES;
        xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_STOP;
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_ESTOP(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        xPL.ServoA6.CMD_StartControl = YES;
        xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_ESTOP;
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

// set auto-slot-position: 슬롯 티칭할때 사용됨.
void CMD_Handle_SASP(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        long offset = xCD.ServoA6.CurrentPosition.Pulse;

        FOR_ALL_SLOT
        {
            xPL.ServoA6.Slot.PositionOffset_pulse[i] = offset;
        }

        xPL.ServoA6.Home.Offset = xPL.ServoA6.Slot.PositionOffset_pulse[SLOT_1];

        // [주의] RAM 에만 반영됨.. 티칭후 save 해야함.
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_JOGS(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 1)
    {
        //        bool dir = (parsedData->Params[0].value._int != 0);
        int pulse = parsedData->Params[0].value._int;

        if (SDG_ServoA6_CheckError((int)A6_CONTROL_MODE_JOG))
        {
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        }
        else
        {
            xPL.ServoA6.CMD_StartControl = YES;
            xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_JOG;
            //            xPL.ServoA6.CMD_Param.Direction = (dir) ? (int)CW : (int)CCW;
            xPL.ServoA6.CMD_Param.Position_Pulse = pulse;
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

void CMD_Handle_GPOS(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        XBuffer_AddInt(xSendMsg, (int)xCD.ServoA6.CurrentPosition.SlotNum, COMMA);
        XBuffer_AddInt(xSendMsg, (int)xCD.ServoA6.CurrentPosition.Pulse, NO_COMMA);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_STIME(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        XBuffer_AddInt(xSendMsg, xPL.Door.CloseSensorOverTime_ms, COMMA);
        XBuffer_AddInt(xSendMsg, xPL.Door.OpenSensorOverTime_ms, NO_COMMA);
    }
    else if (parsedData->ParamCount == 2)
    {
        int openClose = parsedData->Params[0].value._int;
        int delayTime_msec = parsedData->Params[1].value._int;

        if (openClose == CLOSE)
        {
            xPL.Door.CloseSensorOverTime_ms = delayTime_msec;
        }
        else if (openClose == OPEN)
        {
            xPL.Door.OpenSensorOverTime_ms = delayTime_msec;
        }
        else
        {
            SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
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

void CMD_Handle_MOVA(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 1)
    {
        int pulse = parsedData->Params[0].value._int;

        if (SDG_ServoA6_CheckError((int)A6_CONTROL_MODE_ABS))
        {
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        }
        else
        {
            xPL.ServoA6.CMD_StartControl = YES;
            xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_ABS;
            xPL.ServoA6.CMD_Param.Position_Pulse = pulse;
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

void CMD_Handle_MOVI(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 1)
    {
        int pulse = parsedData->Params[0].value._int;

        if (SDG_ServoA6_CheckError((int)A6_CONTROL_MODE_REL))
        {
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        }
        else
        {
            xPL.ServoA6.CMD_StartControl = YES;
            xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_REL;
            xPL.ServoA6.CMD_Param.Position_Pulse = pulse;
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
