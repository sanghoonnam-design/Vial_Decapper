/** ****************************************************************************
 * XCommandHandling.c
 *
 * Created on: 2025.06.20
 * Author    : RND. Kang PilSoon.
 *
 * @note
 *             1. USB 명령, 상위 client 명령 실행함수 처리 모듈
 *             2. 시스템에 맞게 수정해서 사용하세요!~
 *
 ******************************************************************************/
#include "XSystemInfo.h"
#include "_10_XCommandHandling.h"
#include "HW_CAN_Process.h"
#include "HW_RS485_Process.h"
#include "_04_XDiagnose.h"
#include "XEEPROMParam.h"
#include "XSystem_DB.h"

#include "Dev_ServoMotor_A6.h"
#include "panasonic_a6_driver.h"

SemaphoreHandle_t xMutex_Command;
#if 0
#define BUFFER_SIZE_SENDMESSAGE (200)
#else
#define BUFFER_SIZE_SENDMESSAGE (512)
#endif

tsXBuffer *xSendMsg; // host -> client 전송 메시지버퍼

typedef void (*CommandHandler)(const tsXParsedData *parsedData, U08 useTCP);

typedef struct
{
    bool CommandType;       // [1]. 0:system 명령(실제 사용하는 명령), 1:debugging 명령
    char *Command;          // [2]. 명령어 문자열
    CommandHandler Handler; // [3]. 처리 함수 포인터
    char *Help;             // [4]. 명령어 설명
    char *exHelp;           // [5]. 예제 설명
} tsXCommandMapping;

typedef enum
{
    CLI_COMMAND_SYSTEM = 0, // 실제 상위 app. 에서 사용하는 명령
    CLI_COMMAND_DEBUG = 1   // 개발자 디버깅용 명령
} teXCLI_CommandType;

tsXCommandMapping commandTable[] = {

    /** @note USER CODE BEGIN */

    {1, "LR", /*        */ CMD_Handle_Test_LongRun, /*        */ "Long-Run test", /*               */ "LR <mode>"}, // 롱런 테스트

    {0, "ENABLE", /*    */ CMD_Handle_ENABLE, /*              */ "Enable(Step & Servo)", /*        */ "ENABLE"},                         // 스텝모터, 서보 Enable
    {0, "DISABLE", /*   */ CMD_Handle_DISABLE, /*             */ "Disable(Step & Servo)", /*       */ "DISABLE"},                        // 스텝모터, 서보 Disable
    {0, "MRDO", /*      */ CMD_Handle_MRDO, /*                */ "RobotDoor Open/Close", /*        */ "MRDO <1=Open/0=Close>"},          // 로봇 챔버 도어 개폐 제어 (스텝모터 제어)
    {1, "ORG", /*       */ CMD_Handle_ORG, /*                 */ "Origine Operation", /*           */ "ORG"},                            // 원심분리기 축 및 도어 초기 위치 복귀 (Homing)
    {0, "HOME", /*      */ CMD_Handle_HOME, /*                */ "Homing Operation", /*            */ "HOME"},                           // 원심분리기 축 및 도어 초기 위치 복귀 (Homing)
    {0, "CENT", /*      */ CMD_Handle_CENT, /*                */ "Centrifuge Run", /*              */ "CENT <rpm, time_sec>"},           // 원심분리 구동 명령, rpm 속도와 시간(s) 지정 (서보모터 제어)
    {0, "MOVS", /*      */ CMD_Handle_MOVS, /*                */ "Move Slot", /*                   */ "MOVS <slot(1~6)>"},               // 시료 슬롯 이동 명령 (로봇 내부 샘플 트레이 회전/이동)
    {1, "SLOT", /*      */ CMD_Handle_MOVS, /*                */ "Move Slot", /*                   */ "MOVS <slot(1~6)>"},               // 시료 슬롯 이동 명령 (로봇 내부 샘플 트레이 회전/이동)
    {0, "RESET", /*     */ CMD_Handle_RESET, /*               */ "Emergency Reset", /*             */ "RESET"},                          // 비상 정지 이후 시스템 상태 초기화
    {1, "SERV", /*      */ CMD_Handle_SERV, /*                */ "Servo Power On/Off", /*          */ "SERV <1=ON/0=OFF>"},              // 서보모터 전원 제어 (Enable/Disable)
    {1, "MSTP", /*      */ CMD_Handle_STOP, /*                */ "Motor Stop", /*                  */ "MSTP"},                           // 정지 명령
    {0, "STOP", /*      */ CMD_Handle_STOP, /*                */ "Motor Stop", /*                  */ "STOP"},                           // 정지 명령
    {0, "ESTOP", /*     */ CMD_Handle_ESTOP, /*               */ "Motor EMG-Stop", /*              */ "ESTOP"},                          // 즉시 정지 명령
    {1, "SASP", /*      */ CMD_Handle_SASP, /*                */ "Set Auto Slot Position", /*      */ "SASP"},                           // 슬롯 자동위치 티칭 (초기 셋업용)
    {1, "JOGS", /*      */ CMD_Handle_JOGS, /*                */ "Jog Motion", /*                  */ "JOGS <Pulse>"},                   // 조그 이동 명령, dir=CW(1),CCW(0),
    {1, "GPOS", /*      */ CMD_Handle_GPOS, /*                */ "Read current position(pulse)", /**/ "GPOS"},                           // 현재 위치 읽기
    {1, "STIME", /*     */ CMD_Handle_STIME, /*               */ "Set Robo-Door delay time", /*    */ "STIME <close(0)/open(1), msec>"}, // 로봇도어 동작 지연 시간 셋팅
    {1, "SAVEA6", /*    */ CMD_Handle_SAVEA6, /*              */ "Save A6 driver", /*              */ "SAVEA6"},                         // A6 드라이버 EEPROM 저장

    {1, "MOVA", /*      */ CMD_Handle_MOVA, /*                */ "Absolute Move", /*               */ "MOVA <ch, pulse>"}, // 서보/스텝모터 절대 위치 이동 명령 (deg 또는 step 기준)
    {1, "MOVI", /*      */ CMD_Handle_MOVI, /*                */ "Incremental Move", /*            */ "MOVI <ch, p>"},     // 서보/스텝모터 상대 이동 명령 (현재 위치 기준)

    /** @note USER CODE END */

    {0, "VERS", /*      */ CMD_Handle_VERS, /*                */ "Get the FW version.", /*         */ "VERS"},      // 버전 정보 읽기
    {1, "VERSION", /*   */ CMD_Handle_VERS, /*                */ "Get the FW version.", /*         */ "VERSION"},   // 버전 정보 읽기
    {1, "MODELINFO", /* */ CMD_Handle_MODEL, /*               */ "Get the System info.", /*        */ "MODEL"},     // 시스템 모델 읽기
    {0, "GSTA", /*      */ CMD_Handle_GSTA, /*                */ "Get system States.", /*       */ "GSTA"},         // 시스템 상태 반환
    {0, "ST", /*        */ CMD_Handle_GSTA, /*                */ "Get system States.", /*       */ "ST"},           // 시스템 상태 반환
    {1, "PSTA", /*      */ CMD_Handle_PSTA, /*                */ "Get Detailed GSTA Info.", /*     */ "PSTA"},      // 시스템 상세 상태
    {0, "GERR", /*      */ CMD_Handle_GERR, /*                */ "Get the Error-code.", /*         */ "GERR"},      // 에러 코드 반환
    {1, "ERR", /*       */ CMD_Handle_GERR, /*                */ "Get the Error-code.", /*         */ "ERR"},       // 에러 코드 반환
    {0, "GERD", /*      */ CMD_Handle_GERD, /*                */ "Get the Error-message.", /*      */ "GERD"},      // 에러 메시지 반환
    {1, "ERD", /*       */ CMD_Handle_GERD, /*                */ "Get the Error-message.", /*      */ "ERD"},       // 에러 메시지 반환
    {0, "CLER", /*      */ CMD_Handle_CLER, /*                */ "Clear errors.", /*               */ "CLER"},      // 에러 클리어
    {0, "DRT", /*       */ CMD_Handle_CLER, /*                */ "Clear errors.", /*               */ "DRT"},       // 에러 클리어
    {1, "CLEAR", /*     */ CMD_Handle_CLER, /*                */ "Clear errors.", /*               */ "CLEAR"},     // 에러 클리어
    {0, "SAVEE", /*     */ CMD_Handle_SAVEE, /*               */ "Param. save to EEPROM.(Ctrl+S)", /**/ "SAVEE"},   // EEPROM 저장
    {1, "LOADE", /*     */ CMD_Handle_LOADE, /*               */ "Read Params from EEPROM.(Ctrl+L)", /**/ "LOADE"}, // EEPROM 에서 읽어옴.
    {0, "FACT", /*      */ CMD_Handle_FACTORY, /*             */ "Factory setting.", /*            */ "FACT"},      // factory 셋팅
    {1, "FACTORY", /*   */ CMD_Handle_FACTORY, /*             */ "Factory setting.", /*            */ "FACTORY"},   // factory 셋팅
    {1, "PPARAM", /*    */ CMD_Handle_PrintParams, /*         */ "Display Params.", /*             */ "PPARAM"},    // EEPROM 셋팅값 읽어오기

    {1, "SYSTEM", /*    */ CMD_Handle_ContFullInfo, /*        */ "Controller information", /*      */ "SYSTEM"},                               // 하드웨어 정보 출력
    {1, "SETIP", /*     */ CMD_Handle_SetIP, /*               */ "Set IP", /*                      */ "SETIP 192,168,0,150"},                  // IP 셋팅
    {1, "TASK", /*      */ CMD_Handle_TaskList, /*            */ "Check Task state", /*            */ "TASK"},                                 // Task  정보 출력
    {1, "STACK", /*     */ CMD_Handle_StackSize, /*           */ "Check Task Stack-Size", /*       */ "STACK"},                                // stack size 정보 출력
    {0, "REBO", /*      */ CMD_Handle_REBOOT, /*              */ "Reboing.", /*                    */ "REBOOT"},                               // 리부트 실행
    {1, "RESET", /*     */ CMD_Handle_REBOOT, /*              */ "Reboototing.", /*                */ "REBO"},                                 // 리부트 실행
    {1, "REBOOT", /*    */ CMD_Handle_REBOOT, /*              */ "Rebooting.", /*                  */ "RESET"},                                // 리부트 실행
                                                                                                                                               //
    {1, "DI", /*        */ CMD_Handle_DI, /*                  */ "Get GPIO input.", /*             */ "DI (ch 1~16)"},                         // GPIO 출력값 쓰기
    {1, "DO", /*        */ CMD_Handle_DO, /*                  */ "Set GPIO output.", /*            */ "DO (ch 1~24), (val 1/0)"},              // GPIO 출력값 쓰기 (중복)
                                                                                                                                               //
    {1, "DB", /*        */ CMD_Handle_DebugMode, /*           */ "Toggle Debugging-Flags", /*      */ "DEBUG ? or (flag index)"},              // @USER CODE, 각 모듈별 디버그 모드를 셋팅한다.
    {1, "SIZE", /*      */ CMD_Handle_GetSize, /*             */ "Get Size of strut.", /*          */ "SIZE"},                                 // @USER CODE, 데이터 사이즈 정보 출력용
    {1, "MODE", /*      */ CMD_Handle_FWMode, /*              */ "Handle the FW-Mode", /*          */ "MODE 0(0=default,1=idle,2=timer off)"}, // @USER CODE, 시스템 모드
    {1, "HT", /*        */ CMD_Handle_HWTest, /*              */ "Controller HW Test", /*          */ "HT 0(?)"},                              // @USER CODE, 제어기 HW 테스트
                                                                                                                                               //
    {1, "TT", /*        */ CMD_Handle_TaskTrigger, /*         */ "Debug-Trigger on/off", /*        */ "TRIGGER 0~9"},                          // @USER CODE, TestPort 동작 제어
    {1, "TIMER", /*     */ CMD_Handle_TimerOnOff, /*          */ "HW Timer on/off(Toggle)", /*     */ "Timer"},                                // @USER CODE, HW Timer On/Off
    {1, "NOP", /*       */ CMD_Handle_NoOperation, /*         */ "No Operation Command", /*        */ "NOP"},                                  // No Operation Code
                                                                                                                                               //
    {1, "CLC", /*       */ CMD_Handle_ClearScreen, /*         */ "Clear Screen(Console)", /*       */ "clc"},                                  // clear screen(콘솔)
    {1, "??", /*        */ CMD_Handle_Help_All, /*            */ "Help(all command)", /*           */ "??"},                                   // 모든 Help 명령 출력
    {1, "?", /*         */ CMD_Handle_Help, /*                */ "Help", /*                        */ "?"},                                    // 실제 시스템 에서 사용하는 명령 출력
};

void Init_CommandHandling(void)
{
    xMutex_Command = xSemaphoreCreateMutex();
    if (xMutex_Command == NULL)
    {
        printf("Mutex-create failed!\n");
        while (1)
            ;
    }

    xSendMsg = XBuffer_Create(BUFFER_SIZE_SENDMESSAGE);

    if (xSendMsg == NULL)
    {
        ERR_MSG_SEND_N("%s(): Memory allocation failed. [xSendMsg]", __func__);
    }
    else
    {
        XBuffer_Start(xSendMsg); // 버퍼 초기화.
    }
}

/**********************************************************************************************/
/**********************************************************************************************
 * @brief Commmand 처리
 **********************************************************************************************/
/**********************************************************************************************/
void Handle_command(const tsXParsedData *parsedData, U08 useTCP)
{
    /** 개별 명령어 도움말 처리 */ // --> 디버깅용, 1차 help 디스플레이
    if (useTCP == COMM_USB && parsedData->ParamCount == 1 && parsedData->Params[0].value._int == '?')
    {
        __newLine();
        CMD_ShowCommandHelp(parsedData);
    }

    /** 명령어 처리 */
    for (int i = 0; i < sizeof(commandTable) / sizeof(tsXCommandMapping); i++)     // 명령어 리스트 돌려,
    {                                                                              //
        if (strcmp(parsedData->Command, commandTable[i].Command) == 0)             // 명령어 리스트에 해당 명령이 있으면,
        {                                                                          //
            if (xSemaphoreTake(xMutex_Command, NO_WAIT) == pdTRUE)                 //
            {                                                                      //
                /* [1]. Command 처리 */                                            //
                XBuffer_Clear(xSendMsg);                                           // 버퍼 초기화 하고,
                XBuffer_AddCommandString(xSendMsg, parsedData->Command, NO_COMMA); // 명령어 작성하고,
                                                                                   //
                commandTable[i].Handler(parsedData, useTCP);                       // 해당 명령어 내용 처리하고, RX 메시지 만들고,
                                                                                   //
                XBuffer_End(xSendMsg);                                             // '\0' 추가하고
                                                                                   //

                /** 조건: USB이고 skip 대상이면 응답 생략, 그 외에는 응답 */
                const char *noEchoOnUSB[] = {
                    "?", "??", "TT", "HT", "DEBUG", "SIZE", "FACTORY", "PPARAM", "TASK", "STACK", "PSTA"};

                bool skipUSBResponse = false;
                for (int i = 0; i < sizeof(noEchoOnUSB) / sizeof(noEchoOnUSB[0]); i++)
                {
                    if (strcmp(parsedData->Command, noEchoOnUSB[i]) == 0)
                    {
                        skipUSBResponse = true;
                        break;
                    }
                }

                if (!(useTCP == COMM_USB && skipUSBResponse))
                {
                    SEND_MESSAGE(XBuffer_GetBuffer(xSendMsg), (uint16_t)XBuffer_Length(xSendMsg), useTCP);
                }

                xSemaphoreGive(xMutex_Command);
            }

            /* [3]. 여기까지 왔으면 명령어 처리 완료 */
            return;
        }
    }

    /** Command Error(E2100) : 여기까지 왔으면 리스트에 없는 Command. 넌 큰일났다! */
    XBuffer_Clear(xSendMsg);
    XBuffer_AddCommandString(xSendMsg, parsedData->Command, NO_COMMA);
    SetErrorCode(ERROR_CODE_INVALID_COMMAND);
    XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
    XBuffer_End(xSendMsg);
    SEND_MESSAGE(XBuffer_GetBuffer(xSendMsg), (uint16_t)XBuffer_Length(xSendMsg), useTCP);
}
/**********************************************************************************************/
/**********************************************************************************************/

void CMD_Handle_VERS(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        XBuffer_AddString(xSendMsg, xSystemInfo.cd_FWVersion_str, NO_COMMA);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0); // The output is only though the USB port.
    }
}

void CMD_Handle_MODEL(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        XBuffer_AddString(xSendMsg, "System Type : ", NO_COMMA);
        XBuffer_AddU08(xSendMsg, xSystemInfo.pl_systemType, COMMA);
        XBuffer_AddString(xSendMsg, " Model Type : ", NO_COMMA);
        XBuffer_AddU08(xSendMsg, xSystemInfo.pl_modelType, NO_COMMA);

        xprintf("\r\n\t[System]"); // --> USB port , USER CODE
        xprintf("\t  UNKNOWN = 0");
        xprintf("\t  SYNTHESIZER_1  = 1");
        xprintf("\t  SYNTHESIZER_2  = 2");
        xprintf("\t  SYNTHESIZER_3  = 3");
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_Get_FAS_IO_State(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        //        /* [1] */ XBuffer_AddInt(xSendMsg, (int)xCD.Ezi.DO_raw[FAS_DO_BD_1].bit0, COMMA); /* [1] */    // OPIN_REAGENT_M1_1
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);

        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);

        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

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
        /* [09] */ XBuffer_AddInt(xSendMsg, xServoA6.IsDriverError(), COMMA); // debugging code.
        /* [10] */ XBuffer_AddInt(xSendMsg, xServoA6.IsStop(), COMMA);        // debugging code.
                                                                              // TODO: 여기부터, 냉장고 정보

        /* [11] */ XBuffer_AddInt(xSendMsg, xCD.ServoA6.Centrifugal_Force, COMMA);           // debugging code.
        /* [12] */ XBuffer_AddInt(xSendMsg, xCD.ServoA6.CentCommand.Time_sec_Remain, COMMA); // debugging code.
        /* [13] */ XBuffer_Addfloat(xSendMsg, 0.0f, COMMA);                                  // temperature
        /* [14] */ XBuffer_AddInt(xSendMsg, 0, COMMA);                                       // rsv.
        /* [15] */ XBuffer_AddInt(xSendMsg, 0, COMMA);                                       // rsv.
        /* [16] */ XBuffer_AddInt(xSendMsg, 0, COMMA);                                       // rsv.
        /* [17] */ XBuffer_AddInt(xSendMsg, 0, COMMA);                                       // rsv.
        /* [18] */ XBuffer_AddInt(xSendMsg, 0, COMMA);                                       // rsv.
        /* [19] */ XBuffer_AddInt(xSendMsg, 0, COMMA);                                       // rsv.
        /* [20] */ XBuffer_AddInt(xSendMsg, retryCount_RS485[0], NO_COMMA);                     // rsv. // 여기까지 냉장고 온도
        //  ==============================================================================
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);

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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_GERR(const tsXParsedData *parsedData, U08 useTCP)
{

    if (parsedData->ParamCount == 0)
    {
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_GERD(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        XBuffer_AddString(xSendMsg, GetErrorMessage(), NO_COMMA);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_CLER(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (xServoA6.IsDriverError())
        {
            xPL.ServoA6.CMD_StartControl = YES;
            xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_RESET;
        }
        else
        {
            ClearError();

            // xPL.ServoA6.CMD_StartControl = YES;
            // xPL.ServoA6.CMD_ControlMode = A6_CONTROL_MODE_ERRCLEAR;
        }
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_SAVEE(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        XTimer_Stop();
        EEPROMPL_SaveToEEPROM();
        XTimer_Start();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_SAVEF(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        XTimer_Stop();
        EEPROMPL_SaveToFlash();
        XTimer_Start();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_SAVEA(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        XTimer_Stop();
        EEPROMPL_SaveToEEPROMandFlash();
        XTimer_Start();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_LOADE(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        XTimer_Stop();
        EEPROMPL_LoadFromEEPROM(&gEEPROM);
        EEPROMPL_PrintEepromStructure_user();
        XTimer_Start();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_LOADF(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        XTimer_Stop();
        EEPROMPL_LoadFromFlash(&gEEPROM);
        XTimer_Start();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

// Compare the values of Flash and EEPROM.
void CMD_Handle_LOADC(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        XTimer_Stop();
        EEPROMPL_CompareEEPROMandFlash();
        EEPROMPL_PrintEepromStructure_user();
        XTimer_Start();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_FACTORY(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        XTimer_Stop(); // EEPROM 을 다룰때는 반드시 HW Timer를 끄고 한다.
        xprintf("wait....");
        SYSPL_FactorySetting();                       // Factory 셋팅하고,
        memset(&gEEPROM, 0, sizeof(tsEEPROM_Config)); // 현재 구조체 리셋,
        EEPROMPL_LoadFromEEPROM(&gEEPROM);            // 저장이 잘되었는지 다시 읽어오고,
        EEPROMPL_PrintEepromStructure_user();         // 읽어온거 확인한다..
        XTimer_Start();                               // HW Timer 다시 enable
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_PrintParams(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        XTimer_Stop();
        EEPROMPL_PrintEepromStructure_user();
        XTimer_Start();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_DI(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        char str[50] = {0};
        IOEXP_GetDigitalInput_str(str, sizeof(str));
        xprintf(" %s", str);
    }
    else if (parsedData->ParamCount == 1 && (parsedData->Params[0].value._int >= 1 && parsedData->Params[0].value._int <= NUM_IN))
    {
        xcprintf("%d\r\n", digitalRead(READ_IN, parsedData->Params[0].value._int - 1));
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_DO(const tsXParsedData *parsedData, U08 useTCP)
{
    static U08 state = 0;
    char str[50] = {0};
    static U08 pin = 0;
    U08 val = 0;

    if (xHwTest.isLoopbackEnable_Digital)
    {
        xHwTest.isLoopbackEnable_Digital = NO; // 정지
        IOEXP_WriteIOclear();                  // clear
        vTaskDelay(10);
    }

    if (parsedData->ParamCount == 0)
    {

        state ^= 0x01;
        if (state)
        {
            FOR_ALL_DO digitalWrite(i, 0x01);
        }
        else
        {
            IOEXP_WriteIOclear(); // DO all clear
        }

        IOEXP_GetDigitalOutput_str(str, sizeof(str)); // 출력
        xprintf(" %s", str);

        pin = 0; // reset
    }
    else if (parsedData->ParamCount == 1 && parsedData->Params[0].type == PARAM_TYPE_INT)
    {
        if (parsedData->Params[0].value._int == '?')
        {
            __newLine();
            xprintf("\t========================================");
            xprintf("\tDO            : Toggle all channels.");
            xprintf("\tDO 99         : init. digital input/output port.");
            xprintf("\tDO 100        : Toggles all DO ports one by one in sequence.");
            xprintf("\tDO 1~16       : Toggle each channel.");
            xprintf("\tDO (ch),(1/0) : [default command], Toggle the channel.");
        }
        else if (parsedData->Params[0].value._int == 100)
        {
            val = IOEXP_ReadIObit(READ_OUT, pin);
            val ^= 0x01;
            IOEXP_WriteIObit(pin, val);
            pin++;
            if (pin == XHW_PIN_DO_NUM)
                pin = 0;
            IOEXP_GetDigitalOutput_str(str, sizeof(str)); // 출력
            xprintf(" %s", str);
        }
        else if (parsedData->Params[0].value._int == 99)
        {
            IOEXP_Init();
        }
        else if (parsedData->Params[0].value._int > 0 && parsedData->Params[0].value._int <= XHW_PIN_DO_NUM)
        {
            pin = parsedData->Params[0].value._int - 1;
            val = IOEXP_ReadIObit(READ_OUT, pin);
            val ^= 0x01;

            IOEXP_WriteIObit(pin, val);

            IOEXP_GetDigitalOutput_str(str, sizeof(str)); // 출력
            xprintf(" %s", str);
        }
        else
        {
            SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
            if (parsedData->Params[0].value._int != '?')
                xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
        }
    }
    else if (parsedData->ParamCount == 2 && parsedData->Params[0].type == PARAM_TYPE_INT)
    {
        if (parsedData->Params[0].value._int > 0 && parsedData->Params[0].value._int <= XHW_PIN_DO_NUM)
        {
            pin = parsedData->Params[0].value._int - 1;
            if (parsedData->Params[1].value._int > 0)
                val = HIGH;
            else
                val = LOW;

            IOEXP_WriteIObit(pin, val);

            IOEXP_GetDigitalOutput_str(str, sizeof(str)); // 출력
            xprintf(" %s", str);
        }
        else
        {
            SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
            if (parsedData->Params[0].value._int != '?')
                xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
        }
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_REBOOT(const tsXParsedData *parsedData, U08 useTCP)
{
    xcprintf(ANSI_BG_BRIGHT_Red);
    xcprintf(ANSI_TX_Yellow);
    xcprintf("   Bye Bye Bye!~   ");
    xcprintf(ANSI_BG_ORG);
    xcprintf(ANSI_TX_ORG);

    vTaskDelay(100); // 출력시간 기다리고..

    // [현상]: 3번 이상 연속으로 리셋하면 CPU 죽음. 젠장!~
    // [해결]: Backup domain reset : 빠른 연속 리셋 시,
    //         백업 레지스터 값이 비정상적으로 유지되어 부팅 오류를 유발할 수 있음.
    // [결과] : 양호
    // [해결 2]: 만약 이것두 안되면 강제로 watchDog 실행
    RCC->BDCR |= RCC_BDCR_BDRST;
    RCC->BDCR &= ~RCC_BDCR_BDRST;

    for (volatile int i = 0; i < 100000; i++)
        ;

    NVIC_SystemReset(); // 리셋하시오.
}

void CMD_Handle_DebugMode(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 1 && parsedData->Params[0].type == PARAM_TYPE_INT)
    {
        int param1 = parsedData->Params[0].value._int;

        __newLine();
        if (param1 == '?')
        {
            int idx = 1;
            xprintf("\tex) Debug 1 ==> Toggle the variable [DB_isDiagnosisEnabled].\r\n");
            xprintf("\tThe following indicates the status of flags related to debugging.\r\n");

            int j = 1;
            xprintf("\t[%d]. Debug-Flags Status. ", idx++);
            xprintf("\t\t[Idx] [State] [Variable]");
            xprintf("\t\t====================================");
            xprintf("\t\t[%2d] %d : DB_isDiagnosisDisabled", j++, DB_isDiagnosisDisabled);
            xprintf("\t\t[%2d] %d : DB_isPrintParsingDataEnabled", j++, DB_isPrintParsingDataEnabled);
            xprintf("\t\t[%2d] %d : DB_isCANRxFrameDisplayEnabled", j++, DB_isCANRxFrameDisplayEnabled);
            xprintf("\t\t[%2d] %d : DB_isCANTxFrameDisplayEnabled", j++, DB_isCANTxFrameDisplayEnabled);
            //            xprintf("\t\t[%2d] %d : DB_isRS485RxDataDisplayEnabled", j++, DB_isRS485RxDataDisplayEnabled);
            //            xprintf("\t\t[%2d] %d : DB_isRS485TxDataDisplayEnabled", j++, DB_isRS485TxDataDisplayEnabled);
        }
        else if (param1 == 1)
        {
            DB_isDiagnosisDisabled ^= 0x01;
            LOG_MSG_SEND("DB_isDiagnosisDisabled : %d (%s).", DB_isDiagnosisDisabled, DB_isDiagnosisDisabled ? "Disable" : "Enable");
        }
        else if (param1 == 2)
        {
            DB_isPrintParsingDataEnabled ^= 0x01;
            LOG_MSG_SEND("DB_isPrintParsingDataEnabled : %d (%s).", DB_isPrintParsingDataEnabled, DB_isPrintParsingDataEnabled ? "Enable" : "Disable");
        }
        else if (param1 == 3)
        {
            DB_isCANRxFrameDisplayEnabled ^= 0x01;
            LOG_MSG_SEND("DB_isCANRxFrameDisplayEnabled : %d (%s).", DB_isCANRxFrameDisplayEnabled, DB_isCANRxFrameDisplayEnabled ? "Enable" : "Disable");
        }
        else if (param1 == 4)
        {
            DB_isCANTxFrameDisplayEnabled ^= 0x01;
            LOG_MSG_SEND("DB_isCANTxFrameDisplayEnabled : %d (%s).", DB_isCANTxFrameDisplayEnabled, DB_isCANTxFrameDisplayEnabled ? "Enable" : "Disable");
        }
        else if (param1 == 5)
        {
            // DB_isRS485RxDataDisplayEnabled ^= 0x01;
            // LOG_MSG_SEND("DB_isRS485RxDataDisplayEnabled : %d (%s).", DB_isRS485RxDataDisplayEnabled, DB_isRS485RxDataDisplayEnabled ? "Enable" : "Disable");
        }
        else if (param1 == 6)
        {
            // DB_isRS485TxDataDisplayEnabled ^= 0x01;
            // LOG_MSG_SEND("DB_isRS485TxDataDisplayEnabled : %d (%s).", DB_isRS485TxDataDisplayEnabled, DB_isRS485TxDataDisplayEnabled ? "Enable" : "Disable");
        }

        else
        {
            ERR_MSG_SEND("Oops!~ Invalid Index.");
        }
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);

        ERR_MSG_SEND("Oops!~ Invalid Command format.");
    }
}

void CMD_Handle_GetSize(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        __newLine();
        xprintf("\tex) SIZE --> Get size of structures.\r\n");

        int j = 1;
        xprintf("\t\t[idx] [size] : [structure]");
        xprintf("\t\t====================================");
        xprintf("\t\t[%2d] %4d Bytes: size of gEEPROM.", j++, sizeof(tsEEPROM_Config));
        xprintf("\t\t[%2d] %4d Bytes: size of xSL.", j++, sizeof(tsXStateList));
        xprintf("\t\t[%2d] %4d Bytes: size of xCD.", j++, sizeof(tsXControlData));
        xprintf("\t\t[%2d] %4d Bytes: size of xPL.", j++, sizeof(tsXParameterList));
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);

        if (parsedData->Params[0].value._int != '?')
            ERR_MSG_SEND("Oops!~ Invalid Command format.");
    }
}

void CMD_Handle_FWMode(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 1 && parsedData->Params[0].type == PARAM_TYPE_INT)
    {
        __newLine();
        if (parsedData->Params[0].value._int == '?' || parsedData->Params[0].value._int == 'h' || parsedData->Params[0].value._int == 'H')
        {
            xprintf("\t[Set the FW-Mode]");
            xprintf("\t===============================");
            xprintf("\t\tMODE  0 : Default mode");
            xprintf("\t\tMODE  1 : Set all tasks to idle state");
            xprintf("\t\tMODE  2 : HW-Timer Off mode\r\n");
            xprintf("\t\tMODE  3 : Set APC control task to idle state");
            xprintf("\t\tMODE  4 : Factory test mode\r\n");
        }
        else if (parsedData->Params[0].value._int == 0)
        {
            if (!Timer2_GetInterruptStatus())
                XTimer_Start();
            xSystemInfo.PL_Set_FW_Mode(FW_MODE_DEFAULT);
            LOG_MSG_SEND("FW Mode = %d (FW_MODE_DEFAULT)", xSystemInfo.PL_Get_FW_Mode());
        }
        else if (parsedData->Params[0].value._int == 1)
        {
            xSystemInfo.PL_Set_FW_Mode(FW_MODE_IDLE);
            LOG_MSG_SEND("FW Mode = %d (FW_MODE_IDLE)", xSystemInfo.PL_Get_FW_Mode());
        }
        else if (parsedData->Params[0].value._int == 2)
        {
            XTimer_Stop();
            xSystemInfo.PL_Set_FW_Mode(FW_MODE_TIMER_STOP);
            LOG_MSG_SEND("FW Mode = %d (FW_MODE_TIMER_STOP)", xSystemInfo.PL_Get_FW_Mode());
        }
        else if (parsedData->Params[0].value._int == 3)
        {
            if (!Timer2_GetInterruptStatus())
                XTimer_Start();
            xSystemInfo.PL_Set_FW_Mode(FW_MODE_APC_STOP);
            LOG_MSG_SEND("FW Mode = %d (FW_MODE_APC_STOP)", xSystemInfo.PL_Get_FW_Mode());
        }
        else if (parsedData->Params[0].value._int == 4)
        {
            if (!Timer2_GetInterruptStatus())
                XTimer_Start();
            xSystemInfo.PL_Set_FW_Mode(FW_MODE_FACTORY_TEST);
            LOG_MSG_SEND("FW Mode = %d (FW_MODE_FACTORY_TEST)", xSystemInfo.PL_Get_FW_Mode());
        }
        else
        {
            ERR_MSG_SEND("Oops!~ Invalid Index.");
        }
    }
    else
    {
        ERR_MSG_SEND("Oops!~ Invalid Command.");
    }
}

void CMD_Handle_HWTest(const tsXParsedData *parsedData, U08 useTCP) // 제어기 하드웨어 테스트
{
    if (parsedData->ParamCount == 1 && parsedData->Params[0].type == PARAM_TYPE_INT)
    {
        __newLine();
        if (parsedData->Params[0].value._int == '?' || parsedData->Params[0].value._int == 'h' || parsedData->Params[0].value._int == 'H')
        {
            xprintf("\t[HW Test]");
            xprintf("\t===============================");
            xprintf("\t HT  0 : Toggle isLoopbackEnable_All");
            xprintf("\t HT  1 : Toggle isLoopbackEnable_Digital");
            xprintf("\t HT  2 : Toggle isLoopbackEnable_RS232C");
            xprintf("\t HT  3 : Toggle isLoopbackEnable_RS485");
            xprintf("\t HT  4 : Toggle isLoopbackEnable_CAN");
            xprintf("\t HT  5 : Toggle isLoopbackEnable_DAC");

            xprintf("\t HT 10 : Toggle isTestEnable_EEPROM");
            xprintf("\t HT 11 : Toggle isTestEnable_SD");
            xprintf("\t HT 12 : Toggle isTestEnable_RTC");
            xprintf("\t HT 13 : Toggle isTestEnable_Switch");

            xprintf("\r\n\tHT 100 : read states of test-flag.\r\n");
        }
        else if (parsedData->Params[0].value._int == 0)
        {
            xHwTest.isLoopbackEnable_All ^= 0x01;
            LOG_MSG_SEND("isLoopbackEnable_All = %s", xHwTest.isLoopbackEnable_All == 1 ? "YES" : "NO");
        }
        else if (parsedData->Params[0].value._int == 1)
        {
            xHwTest.isLoopbackEnable_Digital ^= 0x01;
            LOG_MSG_SEND("isLoopbackEnable_Digital = %s", xHwTest.isLoopbackEnable_Digital == 1 ? "YES" : "NO");
        }
        else if (parsedData->Params[0].value._int == 2)
        {
            xHwTest.isLoopbackEnable_RS232C ^= 0x01;
            LOG_MSG_SEND("isLoopbackEnable_RS232C = %s", xHwTest.isLoopbackEnable_RS232C == 1 ? "YES" : "NO");
        }
        else if (parsedData->Params[0].value._int == 3)
        {
            xHwTest.isLoopbackEnable_RS485 ^= 0x01;
            LOG_MSG_SEND("isLoopbackEnable_RS485 = %s", xHwTest.isLoopbackEnable_RS485 == 1 ? "YES" : "NO");
        }
        else if (parsedData->Params[0].value._int == 4)
        {
            xHwTest.isLoopbackEnable_CAN ^= 0x01;
            LOG_MSG_SEND("isLoopbackEnable_CAN = %s", xHwTest.isLoopbackEnable_CAN == 1 ? "YES" : "NO");
        }
        else if (parsedData->Params[0].value._int == 6) // TODO
        {
            //   xHwTest. ^= 0x01;
            // LOG_MSG_SEND("isLoopbackEnable_DAC = %s", xHwTest.isLoopbackEnable_DAC == 1 ? "YES" : "NO");
        }
        else if (parsedData->Params[0].value._int == 7) // TODO
        {
            // xHwTest.
            // LOG_MSG_SEND("isLoopbackEnable_All = %s", xHwTest.isLoopbackEnable_All == 1 ? "YES" : "NO");
        }
        else if (parsedData->Params[0].value._int == 8) // TODO
        {
            // xHwTest.
            // LOG_MSG_SEND("isLoopbackEnable_All = %s", xHwTest.isLoopbackEnable_All == 1 ? "YES" : "NO");
        }
        else if (parsedData->Params[0].value._int == 9) // TODO
        {
            // xHwTest.
            // LOG_MSG_SEND("isLoopbackEnable_All = %s", xHwTest.isLoopbackEnable_All == 1 ? "YES" : "NO");
        }
        else if (parsedData->Params[0].value._int == 10)
        {
            xHwTest.isTestEnable_EEPROM ^= 0x01;
            LOG_MSG_SEND("isTestEnable_EEPROM = %s", xHwTest.isTestEnable_EEPROM == 1 ? "YES" : "NO");
        }
        else if (parsedData->Params[0].value._int == 11)
        {
            xHwTest.isTestEnable_SD ^= 0x01;
            LOG_MSG_SEND("isTestEnable_SD = %s", xHwTest.isTestEnable_SD == 1 ? "YES" : "NO");
        }
        else if (parsedData->Params[0].value._int == 13)
        {
            xHwTest.isTestEnable_Switch ^= 0x01;
            LOG_MSG_SEND("isTestEnable_Switch = %s", xHwTest.isTestEnable_Switch == 1 ? "YES" : "NO");
        }
        else if (parsedData->Params[0].value._int == 100)
        {
            xprintf("\t%d : xHwTest.isLoopbackEnable_All", xHwTest.isLoopbackEnable_All);
            xprintf("\t%d : xHwTest.isLoopbackEnable_Digital", xHwTest.isLoopbackEnable_Digital);
            xprintf("\t%d : xHwTest.isLoopbackEnable_RS232C", xHwTest.isLoopbackEnable_RS232C);
            xprintf("\t%d : xHwTest.isLoopbackEnable_RS485", xHwTest.isLoopbackEnable_RS485);
            xprintf("\t%d : xHwTest.isLoopbackEnable_CAN", xHwTest.isLoopbackEnable_CAN);
            xprintf("\t%d : xHwTest.isTestEnable_EEPROM", xHwTest.isTestEnable_EEPROM);
            xprintf("\t%d : xHwTest.isTestEnable_SD", xHwTest.isTestEnable_SD);
            xprintf("\t%d : xHwTest.isTestEnable_Switch\r\n", xHwTest.isTestEnable_Switch);
        }
        else
        {
            ERR_MSG_SEND("Oops!~ Invalid Index.");
        }
    }
    else
    {
        ERR_MSG_SEND("Oops!~ Invalid Command.");
    }
}

void CMD_Handle_TaskTrigger(const tsXParsedData *parsedData, U08 useTCP)
{
    xTrigger.enabled = YES;

    if (parsedData->ParamCount > 0 && parsedData->ParamCount <= 3)
    {
        for (int i = 0; i < parsedData->ParamCount; i++)
        {
            xTrigger.param[i] = parsedData->Params[i].value._int;
        }
    }
    else
    {
        xTrigger.param[0] = 0;
    }

    //[]. 테스트 포트 제어
    XTP_ControlTriggerPort(); // 디버깅용 //TODO usb로만 프린트 하게 수정 할것
}

void CMD_Handle_TimerOnOff(const tsXParsedData *parsedData, U08 useTCP)
{
    // toggle
    if (XTimer_GetStatus())
    {
        XTimer_Stop();
        LOG_MSG_SEND(ANSI_TX_Cyan "The HW-Timer is disabled." ANSI_TX_ORG);
    }
    else
    {
        XTimer_Start();
        LOG_MSG_SEND(ANSI_TX_Cyan "The HW-Timer is enabled." ANSI_TX_ORG);
    }
}

void CMD_Handle_NoOperation(const tsXParsedData *parsedData, U08 useTCP)
{
    // No Operation Code
}

void CMD_Handle_ClearScreen(const tsXParsedData *parsedData, U08 useTCP)
{
    xcprintf(ANSI_CLEAR_TERMINAL);
}

/**
 *          PLL Output (ex: 216MHz)
 *                |
 *             SYSCLK
 *                |
 *             ┌──┴──────┐
 *           HCLK       (CPU, DMA, SRAM, GPIO)
 *             |
 *      ┌──────┴───────┐
 *     PCLK1         PCLK2
 *   (APB1)          (APB2)
 *     ↓               ↓
 *   TIM2~7          TIM1, TIM8
 *   (×2 if divided)  (×2 if divided)
 */
void CMD_Handle_ContFullInfo(const tsXParsedData *parsedData, U08 useTCP)
{
    // 시스템 정보 전체 출력 함수

    //---------------- Clock ----------------//
    // HAL을 통해 시스템 및 버스 클럭 주파수 얻기
    uint32_t sysclk = HAL_RCC_GetSysClockFreq(); // 시스템 클럭 (SYSCLK)
    uint32_t hclk = HAL_RCC_GetHCLKFreq();       // AHB 버스 클럭 (HCLK)
    uint32_t pclk1 = HAL_RCC_GetPCLK1Freq();     // APB1 버스 클럭
    uint32_t pclk2 = HAL_RCC_GetPCLK2Freq();     // APB2 버스 클럭

    // APB 버스가 1분주가 아니면, 해당 버스의 타이머는 클럭이 2배가 됨
    uint32_t tim_clk1 = ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1) ? (pclk1 * 2) : pclk1;
    uint32_t tim_clk2 = ((RCC->CFGR & RCC_CFGR_PPRE2) != RCC_CFGR_PPRE2_DIV1) ? (pclk2 * 2) : pclk2;

    //---------------- PLL ----------------//
    RCC_OscInitTypeDef osc = {0};
    HAL_RCC_GetOscConfig(&osc); // PLL 설정 정보 가져오기

    // uint8_t pll_src = osc.PLL.PLLSource; // PLL 입력 소스 (HSE 또는 HSI)
    uint32_t PLLM = osc.PLL.PLLM; // PLL 입력 분주 값
    uint32_t PLLN = osc.PLL.PLLN; // PLL 곱셈 계수
    // PLL 출력 분주 값 (정의된 값으로 해석)
    uint32_t PLLP = (osc.PLL.PLLP == RCC_PLLP_DIV2) ? 2 : (osc.PLL.PLLP == RCC_PLLP_DIV4) ? 4
                                                      : (osc.PLL.PLLP == RCC_PLLP_DIV6)   ? 6
                                                      : (osc.PLL.PLLP == RCC_PLLP_DIV8)   ? 8
                                                                                          : 0;
    uint32_t PLLQ = osc.PLL.PLLQ; // USB, RNG, SDIO 등에 사용되는 PLL 출력 분주 값

    //---------------- CPU ID ----------------//
    uint32_t cpuid = SCB->CPUID;                // ARM Cortex-M에서 제공하는 코어 ID 정보
    uint8_t implementer = (cpuid >> 24) & 0xFF; // 코어 제조사 (0x41 = ARM)
    uint8_t variant = (cpuid >> 20) & 0xF;      // 코어 버전 (rX)
    uint16_t part_no = (cpuid >> 4) & 0xFFF;    // 코어 종류 (Cortex-M4/M7 등)
    uint8_t revision = (cpuid >> 0) & 0xF;      // 패치 리비전 (pX)

    //---------------- FPU / MPU ----------------//
    // CPACR 레지스터를 통해 FPU 사용 여부 확인 (20~23비트가 0b1111이면 사용 중)
    uint8_t fpu_present = ((SCB->CPACR & (0xF << 20)) == (0xF << 20)) ? 1 : 0;
    // MPU->TYPE 레지스터 하위 8비트 > 0이면 MPU 존재
    uint8_t mpu_present = ((MPU->TYPE & 0xFF) > 0) ? 1 : 0;

    //---------------- Debug ----------------//
    uint32_t debug_id = DBGMCU->IDCODE;                      // 디버그 MCU ID 레지스터 (Device ID + Revision ID)
    uint16_t dev_id = (uint16_t)(debug_id & 0x0FFF);         // 하드웨어 Device ID
    uint16_t rev_id = (uint16_t)((debug_id >> 16) & 0xFFFF); // 하드웨어 Revision ID

    //---------------- Flash / UID ----------------//
    uint32_t flash_kb = *(uint16_t *)FLASHSIZE_BASE; // MCU에 내장된 Flash 용량 (KB 단위)
    // Unique ID (96비트) 읽기 - 제조 시 고정된 유일한 값
    uint32_t uid0 = *(uint32_t *)(UID_BASE);
    uint32_t uid1 = *(uint32_t *)(UID_BASE + 4);
    uint32_t uid2 = *(uint32_t *)(UID_BASE + 8);

    //---------------- Vector Table ----------------//
    uint32_t vtor_addr = SCB->VTOR; // 현재 인터럽트 벡터 테이블이 위치한 주소

    //---------------- NVIC ----------------//
    int nvic_count = 0;         // 활성화된 IRQ 개수 카운팅
    for (int i = 0; i < 8; i++) // 최대 8개의 ISER (총 256개의 IRQ 지원)
    {
        uint32_t iser = NVIC->ISER[i]; // 각 ISER 레지스터에서
        while (iser)
        {
            if (iser & 0x1)
                nvic_count++; // 비트가 1이면 활성화된 IRQ
            iser >>= 1;       // 다음 비트로 이동
        }
    }

    //---------------- Print ----------------//
    xcprintf("\r\n========== CONTROLLER INFO ==========\r\n");

    // 클럭 정보 출력
    xcprintf("[Clock Info]\r\n");
    xcprintf("SYSCLK              : %lu Hz\r\n", sysclk);   // 216 MHz: MCU 전체 기준 클럭 (PLL 출력). CPU 클럭이기도 함
    xcprintf("HCLK                : %lu Hz\r\n", hclk);     // 216 MHz: AHB 버스 클럭. SRAM, DMA, GPIO에 사용됨
    xcprintf("PCLK1               : %lu Hz\r\n", pclk1);    //  54 MHz: APB1 (저속 주변장치) 버스 클럭. 예: UART2, TIM2
    xcprintf("PCLK2               : %lu Hz\r\n", pclk2);    // 108 MHz: APB2 (고속 주변장치) 버스 클럭. 예: USART1, TIM1
    xcprintf("TIMx (APB1)         : %lu Hz\r\n", tim_clk1); // 108 MHz: 타이머용 클럭. APB1 분주 ≠ 1 → 2배로 클럭 공급됨
    xcprintf("TIMx (APB2)         : %lu Hz\r\n", tim_clk2); // 216 MHz: 타이머용 클럭. APB2도 분주 ≠ 1 → 2배

    // PLL 설정 정보 출력
    xcprintf("\r\n[PLL Info]\r\n");
    // xcprintf("PLL Source          : %s\r\n",
    //          (pll_src == RCC_PLLSOURCE_HSE) ? "HSE" : "HSI"); // HSI:	내부 16MHz 클럭 사용 (HSE 아님)
    xcprintf("PLL Source          : %s\r\n",
             ((RCC->PLLCFGR & RCC_PLLSOURCE_HSE) != 0) ? "HSE" : "HSI");
    xcprintf("PLLM                : %lu\r\n", PLLM); //   4:	입력 클럭 16MHz / 4 = 4MHz
    xcprintf("PLLN                : %lu\r\n", PLLN); // 216:	4MHz × 216 = 864MHz (VCO 출력)
    xcprintf("PLLP                : %lu\r\n", PLLP); //   2:	864 / 2 = 216MHz → SYSCLK
    xcprintf("PLLQ                : %lu\r\n", PLLQ); //   2:	864 / 2 = 432MHz → USB 등엔 너무 높음

    // Flash 및 UID 정보 출력
    xcprintf("\r\n[Flash & UID]\r\n");
    xcprintf("Flash Size          : %lu KB\r\n", flash_kb);                    // 1024 KB	내장 플래시 1MB
    xcprintf("UID                 : %08lX-%08lX-%08lX\r\n", uid0, uid1, uid2); // 0032004E-32375108-36333438	고유한 96bit 디바이스 식별자 (제조 시 부여됨)

    // 메모리 매핑 정보 출력
    xcprintf("\r\n[Memory Map]\r\n");
    xcprintf("VTOR Address        : 0x%08lX\r\n", vtor_addr); // 0x08040000

    // CPU Core 정보 출력
    xcprintf("\r\n[CPU Core Info]\r\n");
    xcprintf("Implementer         : 0x%02X (%s)\r\n", implementer, (implementer == 0x41) ? "ARM" : "Unknown");
    xcprintf("CPU Variant         : r%u\r\n", variant);
    xcprintf("CPU Part Number     : 0x%03X (%s)\r\n", part_no,
             (part_no == 0xC20) ? "Cortex-M0" : (part_no == 0xC60) ? "Cortex-M0+"
                                            : (part_no == 0xC23)   ? "Cortex-M3"
                                            : (part_no == 0xC24)   ? "Cortex-M4"
                                            : (part_no == 0xC27)   ? "Cortex-M7"
                                                                   : "Unknown");
    xcprintf("CPU Revision        : p%u\r\n", revision);

    // FPU, MPU 존재 여부 출력
    xcprintf("\r\n[FPU / MPU]\r\n");
    xcprintf("FPU Present         : %s\r\n", fpu_present ? "Yes" : "No");
    xcprintf("MPU Present         : %s\r\n", mpu_present ? "Yes" : "No");

    // 디버그 ID 정보 출력
    xcprintf("\r\n[Debug Info]\r\n");
    xcprintf("DBGMCU->IDCODE       : 0x%08lX\r\n", debug_id);
    xcprintf("Device ID           : 0x%03X\r\n", dev_id);
    xcprintf("Revision ID         : 0x%04X\r\n", rev_id);

    // NVIC 활성화된 IRQ 개수 출력
    xcprintf("\r\n[NVIC Info]\r\n");
    xcprintf("Active IRQ Count    : %d\r\n", nvic_count); // 5	현재 NVIC에 활성화된 인터럽트 5개 존재 (사용 중인 IRQ 수)

    // 현재 활성화된 클럭 비트 출력
    xcprintf("\r\n[Enabled Clocks]\r\n");
    xcprintf("RCC->AHB1ENR         : 0x%08lX\r\n", RCC->AHB1ENR); // 0x0050007F	GPIOA~E, CRC, DMA1, DMA2, FMC 활성화됨
    xcprintf("RCC->AHB2ENR         : 0x%08lX\r\n", RCC->AHB2ENR); // 0x00000000	DCMI, OTG-FS 등 사용 안함
    xcprintf("RCC->APB1ENR         : 0x%08lX\r\n", RCC->APB1ENR); // 0x122A4002	USART2, TIM2~7, I2C1/2/3 등 일부 장치 사용 중
    xcprintf("RCC->APB2ENR         : 0x%08lX\r\n", RCC->APB2ENR); // 0x00107112	TIM1, TIM8, USART1, SPI1, SYSCFG 등 활성화됨

    xcprintf("=========================================\r\n");
}

void CMD_Handle_SetIP(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 4)
    {
        U8 ip[4];

        ip[0] = parsedData->Params[0].value._int;
        ip[1] = parsedData->Params[1].value._int;
        ip[2] = parsedData->Params[2].value._int;
        ip[3] = parsedData->Params[3].value._int;

        SystemInfo_Network_SetIP(&xSystemInfo.network, ip);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_TaskList(const tsXParsedData *parsedData, U08 useTCP)
{
    if (useTCP == COMM_USB && parsedData->ParamCount == 0)
    {
        xCheck_Task();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_StackSize(const tsXParsedData *parsedData, U08 useTCP)
{
    if (useTCP == COMM_USB && parsedData->ParamCount == 0)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        xCheck_Stack(); // check stack size
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

void CMD_Handle_Help_All(const tsXParsedData *parsedData, U08 useTCP)
{
    if (useTCP == COMM_USB)
    {
        LOG_MSG_SEND("[Available Commands]\n");
        xcprintf("\t    %-10s  %-25s\t%-s\r\n", "[Command]", "[Desctiption]", "[Example]");
        xcprintf("\t-------------------------------------------------------------\r\n");

        for (int i = 0; i < sizeof(commandTable) / sizeof(tsXCommandMapping); i++)
        {
            xcprintf("\t[%2d] [%d] " ANSI_TX_Yellow "%-8s" ANSI_TX_ORG ": %-23s\tex) %s\r\n\0",
                     i + 1,
                     commandTable[i].CommandType,
                     commandTable[i].Command,
                     commandTable[i].Help,
                     commandTable[i].exHelp);
        }

        xcprintf("\r\n");
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_COMMAND);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
    }
}

void CMD_Handle_Help(const tsXParsedData *parsedData, U08 useTCP)
{
    U08 k = 0;
    if (useTCP == COMM_USB)
    {
        LOG_MSG_SEND("[Available Commands]\n");
        xcprintf("\t    %-10s  %-25s\t%-s\r\n", "[Command]", "[Desctiption]", "[Example]\0");
        xcprintf("\t-------------------------------------------------------------\r\n");

        for (int i = 0; i < sizeof(commandTable) / sizeof(tsXCommandMapping); i++)
        {
            if (commandTable[i].CommandType == CLI_COMMAND_SYSTEM) // system 명령만 출력한다.
            {
                k++;
                xcprintf("\t[%2d] " ANSI_TX_Yellow "%-9s" ANSI_TX_ORG ": %-25s\tex) %s\r\n\0",
                         k,
                         commandTable[i].Command,
                         commandTable[i].Help,
                         commandTable[i].exHelp);
            }
        }
        xcprintf("\r\n");
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_COMMAND);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
    }
}

void CMD_ShowCommandHelp(const tsXParsedData *parsedData)
{
    for (int i = 0; i < sizeof(commandTable) / sizeof(tsXCommandMapping); i++)
    {
        if (strcmp(parsedData->Command, commandTable[i].Command) == 0)
        {
            xprintf(ANSI_TX_Yellow "\t%-8s" ANSI_TX_ORG ": %-25s\tex) %s\r\n\0",
                    commandTable[i].Command,
                    commandTable[i].Help,
                    commandTable[i].exHelp);
        }
    }
}

//======================================================================================
//======================================================================================
//======================================================================================
//======================================================================================
//======================================================================================
//======================================================================================
//======================================================================================
//======================================================================================
// @USER CORD START
//======================================================================================
//======================================================================================
//======================================================================================
//======================================================================================
//======================================================================================
//======================================================================================
//======================================================================================
//======================================================================================
// 장비 점검용 명령

void CMD_Handle_Test_LongRun(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        //     xTest.CMD_StartControl = YES;
        //     xTest.TestMode = MODE_TEST_STOP;
    }
    else if (parsedData->ParamCount == 1)
    {
        // int c = parsedData->Params[0].value._int;

        // if (c == '?')
        // {
        //     int i = 1;
        //     __newLine();
        //     xprintf("\t== LR <mode> Command ==");
        //     xprintf("\t  %d) mode = 1 : TY-Motion long-run test", i++);
        //     xprintf("\t  %d) mode = 2 : RobotDoor long-run test", i++);
        //     xprintf("\t  %d) mode = 99 : RobotDoor long-run test", i++);
        // }
        // else if (c == 1) // robot-TY
        // {
        //     xTest.CMD_StartControl = YES;
        //     xTest.TestMode = MODE_TEST_ROBOT_TY;
        // }
        // else if (c == 2) // robot-door
        // {
        //     xTest.CMD_StartControl = YES;
        //     xTest.TestMode = MODE_TEST_ROBOTDOOR;
        // }
        // else if (c == 999)
        // {
        //     xTest.CMD_StartControl = YES;
        //     xTest.TestMode = MODE_TEST_STOP;

        //     xPL.RobotTY.CMD_StartControl = YES;
        //     xPL.RobotTY.ControlMode = MODE_ROBOT_STOP;
        // }
        // else
        // {
        //     SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        //     XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        // }
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?' && useTCP == COMM_USB)
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
            SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        }
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
            SetErrorCode(ERROR_CODE_CENT_BLOCKED_DOOR_OPEN);
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        }
        else if (!xServoA6.IsHomed())
        {
            SetErrorCode(ERROR_CODE_A6_SERVO_NOT_HOME);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
            SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
            SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
            SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        }
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}
