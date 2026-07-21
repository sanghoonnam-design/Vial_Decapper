/** ****************************************************************************
 * XCommand_Core.c
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
#include "_10_XCommand_Core.h"
#include "HW_CAN_Process.h"
#include "HW_RS485_Process.h"
#include "_04_XDiagnose.h"
#include "XEEPROMParam.h"
#include "XSystem_DB.h"

tsXBuffer *xSendMsg; // host -> client 전송 메시지버퍼
SemaphoreHandle_t xMutex_Command;

tsXCommandMapping gCoreCommandTable[] = {

    {0, "VERS", /*      */ CMD_Handle_VERS, /*                */ "Get the FW version.", /*         */ "VERS"},      // 버전 정보 읽기
    {1, "VERSION", /*   */ CMD_Handle_VERS, /*                */ "Get the FW version.", /*         */ "VERSION"},   // 버전 정보 읽기
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

    {1, "SYSTEM", /*    */ CMD_Handle_ContFullInfo, /*        */ "Controller information", /*      */ "SYSTEM"},              // 하드웨어 정보 출력
    {1, "SETIP", /*     */ CMD_Handle_SetIP, /*               */ "Set IP", /*                      */ "SETIP 192,168,0,150"}, // IP 셋팅
    {1, "TASK", /*      */ CMD_Handle_TaskList, /*            */ "Check Task state", /*            */ "TASK"},                // Task  정보 출력
    {1, "STACK", /*     */ CMD_Handle_StackSize, /*           */ "Check Task Stack-Size", /*       */ "STACK"},               // stack size 정보 출력
    {0, "REBO", /*      */ CMD_Handle_REBOOT, /*              */ "Reboing.", /*                    */ "REBOOT"},              // 리부트 실행
    // {1, "RESET", /*     */ CMD_Handle_REBOOT, /*              */ "Reboototing.", /*                */ "REBO"},                                 // 리부트 실행
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

static const int gCoreCommandCount = sizeof(gCoreCommandTable) / sizeof(tsXCommandMapping);
static int gWidth_Command = 23; // default width for help display
static int gWidth_Help = 10;    // default width for command display in help
static void CLI_GetMaxHelpWidth(void);

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
static bool ExecuteFromTable(const tsXCommandMapping *table, int count,
                             const tsXParsedData *parsedData, U08 useTCP)
{
    for (int i = 0; i < count; i++)
    {
        if (strcmp(parsedData->Command, table[i].Command) == 0)
        {
            table[i].Handler(parsedData, useTCP);
            return true;
        }
    }
    return false;
}

void Handle_command(const tsXParsedData *parsedData, U08 useTCP)
{
    if (xSemaphoreTake(xMutex_Command, portMAX_DELAY) != pdTRUE)
        return;

    bool handled = false;

    /* [0]. 개별 명령어 도움말 처리 */ // --> 디버깅용, 1차 help 디스플레이
    if (useTCP == COMM_USB && parsedData->ParamCount == 1 && parsedData->Params[0].value._int == '?')
    {
        // __newLine();
        CMD_ShowCommandHelp(parsedData);
    }

    XBuffer_Clear(xSendMsg);
    XBuffer_AddCommandString(xSendMsg, parsedData->Command, NO_COMMA);

    /* [1]. core 명령 실행 */
    handled = ExecuteFromTable(gCoreCommandTable, gCoreCommandCount, parsedData, useTCP);

    /* [2]. module 명령 실행 */
    if (!handled)
    {
        handled = ExecuteFromTable(gModuleCommandTable, gModuleCommandCount, parsedData, useTCP);
    }

    //    if (!handled)
    //    {
    //        handled = ExecuteFromTable(gRobotCommandTable, gRobotCommandCount, parsedData, useTCP);
    //    }

    /* [3]. invalid 처리 */
    if (!handled)
    {
        SetErrorCode(ERROR_CODE_INVALID_COMMAND, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
    }

    XBuffer_End(xSendMsg);

    /* [4]. 메시지 전송: 필터링 후 출력 */
    if (!CMD_ShouldSkip_USBResponse(parsedData))
    {
        SEND_MESSAGE(XBuffer_GetBuffer(xSendMsg), (uint16_t)XBuffer_Length(xSendMsg), useTCP);
    }

    xSemaphoreGive(xMutex_Command);
}

void Handle_command_by_string(const char *cmdStr, U08 useTCP)
{
    teXParsingErrorCode result;
    tsXParsedData *pData = NULL;

    static char tempBuffer[BUFFER_SIZE_RX_MSG] = {0};

    if (!cmdStr || cmdStr[0] == '\0')
        return;

    // [1]. \r\n 붙이기
    CMD_MakeCommandWithCRLF(tempBuffer, sizeof(tempBuffer), cmdStr);

    // [2] parser 대상 선택
    switch (useTCP)
    {
    case COMM_RS232C:
        pData = &xParsedData_RS232C;
        break;
    case COMM_TCP:
        pData = &xParsedData_Network;
        break;
    case COMM_USB:
        pData = &xParsedData_USB;
        break;
    default:
        return;
    }

    // [3] 명령어 파싱 및 실행
    result = xParser_ProcessReceivedData(tempBuffer, pData, useTCP);
    if (result == PARSER_ERR_SUCCESS)
    {
        Handle_command(pData, useTCP);
    }
}

void CMD_MakeCommandWithCRLF(char *dst, size_t dstSize, const char *src)
{
    if (!dst || !src || dstSize < 3) // 3-->최소: "\r\n\0"
        return;

    size_t len = 0;

    while (src[len] != '\0' && len < (dstSize - 3))
    {
        dst[len] = src[len];
        len++;
    }

    dst[len++] = '\r';
    dst[len++] = '\n';
    dst[len] = '\0';
}

bool CMD_ShouldSkip_USBResponse(const tsXParsedData *parsedData)
{
    // USB로 응답/에코 생략할 명령어 리스트, 필요에 따라 추가
    static const char *noEchoOnUSB[] = {
        "CLC", "TASK", "STACK", "?", "??", "??R", "FACT", "SAVEE", "LOADE", "PSTA"};

    if (parsedData == NULL)
        return false;

    for (size_t i = 0; i < (sizeof(noEchoOnUSB) / sizeof(noEchoOnUSB[0])); i++)
    {
        if (strcmp(parsedData->Command, noEchoOnUSB[i]) == 0)
            return true;
    }

    return false;
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0); // The output is only though the USB port.
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);

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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
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
        // [YYMMDD] parameter를 EEPROM에 저장한 날짜 저장
        xPL.Header.UpdateDate = SWRTC_GetTime_YYMMDDHH();

        EEPROMPL_SaveToEEPROM();
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
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
        // [YYMMDD] parameter를 EEPROM에 저장한 날짜 저장
        xPL.Header.UpdateDate = SWRTC_GetTime_YYMMDDHH();

        EEPROMPL_SaveToEEPROMandFlash();
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
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
            SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
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
            SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
            XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
            if (parsedData->Params[0].value._int != '?')
                xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
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
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

bool CMD_CheckParamAll_Int(const tsXParsedData *parsedData, int expectedCount)
{
    if (parsedData == NULL)
        return false;

    if (parsedData->ParamCount != expectedCount)
        return false;

    for (int i = 0; i < expectedCount; i++)
    {
        if (parsedData->Params[i].type != PARAM_TYPE_INT)
            return false;
    }

    return true;
}

bool CMD_CheckParamAll_Float(const tsXParsedData *parsedData, int expectedCount)
{
    if (parsedData == NULL)
        return false;

    if (parsedData->ParamCount != expectedCount)
        return false;

    for (int i = 0; i < expectedCount; i++)
    {
        if (parsedData->Params[i].type != PARAM_TYPE_FLOAT)
            return false;
    }

    return true;
}

__attribute__ ((unused)) static void CLI_GetMaxHelpWidth(void)
{
    int max = 0;

    // 명령어 이름 최대 길이 구하기
    //    for (int i = 0; i < gRobotCommandCount; i++)
    //    {
    //        int len = strlen(gRobotCommandTable[i].Command);
    //        if (len > max)
    //            max = len;
    //    }

    for (int i = 0; i < gModuleCommandCount; i++)
    {
        int len = strlen(gModuleCommandTable[i].Command);
        if (len > max)
            max = len;
    }

    for (int i = 0; i < gCoreCommandCount; i++)
    {
        int len = strlen(gCoreCommandTable[i].Command);
        if (len > max)
            max = len;
    }
    gWidth_Command = max + 2;

    // 명령어 설명 최대 길이 구하기
    max = 0;

    for (int i = 0; i < gModuleCommandCount; i++)
    {
        int len = strlen(gModuleCommandTable[i].Help);
        if (len > max)
            max = len;
    }

    for (int i = 0; i < gCoreCommandCount; i++)
    {
        int len = strlen(gCoreCommandTable[i].Help);
        if (len > max)
            max = len;
    }
    gWidth_Help = max + 2; // 여유 공간 2칸 추가
}

void CMD_Handle_Help_All(const tsXParsedData *parsedData, U08 useTCP)
{
    int i, k = 0;

    if (useTCP == COMM_USB)
    {
        LOG_MSG_SEND("[Available Commands]\n");
        xcprintf("\t    %-*s  %-*s\t%-s\r\n", gWidth_Command, "[Command]", gWidth_Help, "[Desctiption]", "[Example]");
        xcprintf("\t-------------------------------------------------------------------\r\n");

        for (i = 0; i < gModuleCommandCount; i++)
        {
            k++;
            xcprintf("\t[%2d] [%d] " ANSI_TX_LightGreen "%-*s" ANSI_TX_ORG ": %-*s  : ex) %s\r\n",
                     k,
                     gModuleCommandTable[i].CommandType,
                     gWidth_Command,
                     gModuleCommandTable[i].Command,
                     gWidth_Help,
                     gModuleCommandTable[i].Help,
                     gModuleCommandTable[i].exHelp);
        }

        for (i = 0; i < gCoreCommandCount; i++)
        {
            k++;
            xcprintf("\t[%2d] [%d] " ANSI_TX_Yellow "%-*s" ANSI_TX_ORG ": %-*s  : ex) %s\r\n",
                     k,
                     gCoreCommandTable[i].CommandType,
                     gWidth_Command,
                     gCoreCommandTable[i].Command,
                     gWidth_Help,
                     gCoreCommandTable[i].Help,
                     gCoreCommandTable[i].exHelp);
        }

        xcprintf("\t-------------------------------------------------------------------\r\n");
        // xcprintf("\r\n");
    }
}

void CMD_Handle_Help(const tsXParsedData *parsedData, U08 useTCP)
{
    U08 k = 0;
    if (useTCP == COMM_USB)
    {
        LOG_MSG_SEND("[Available Commands]\n");
        xcprintf("\t    %-*s  %-*s\t%-s\r\n", gWidth_Command, "[Command]", gWidth_Help, "[Desctiption]", "[Example]");
        xcprintf("\t-------------------------------------------------------------------\r\n");

        for (int i = 0; i < gModuleCommandCount; i++)
        {
            if (gModuleCommandTable[i].CommandType == CLI_COMMAND_SYSTEM) // system 명령만 출력한다.
            {
                k++;
                xcprintf("\t[%2d] " ANSI_TX_LightGreen "%-*s" ANSI_TX_ORG ": %-*s  : ex) %s\r\n",
                         k,
                         gWidth_Command,
                         gModuleCommandTable[i].Command,
                         gWidth_Help,
                         gModuleCommandTable[i].Help,
                         gModuleCommandTable[i].exHelp);
            }
        }

        for (int i = 0; i < gCoreCommandCount; i++)
        {
            if (gCoreCommandTable[i].CommandType == CLI_COMMAND_SYSTEM) // system 명령만 출력한다.
            {
                k++;
                xcprintf("\t[%2d] " ANSI_TX_Yellow "%-*s" ANSI_TX_ORG ": %-*s  : ex) %s\r\n",
                         k,
                         gWidth_Command,
                         gCoreCommandTable[i].Command,
                         gWidth_Help,
                         gCoreCommandTable[i].Help,
                         gCoreCommandTable[i].exHelp);
            }
        }

        xcprintf("\t-------------------------------------------------------------------\r\n");
        // xcprintf("\r\n");
    }
}

void CMD_ShowCommandHelp(const tsXParsedData *parsedData)
{
    for (int i = 0; i < gModuleCommandCount; i++)
    {
        if (strcmp(parsedData->Command, gModuleCommandTable[i].Command) == 0)
        {
            xprintf(ANSI_TX_LightGreen "\t%-8s" ANSI_TX_ORG ": %-23s\tex) %s\r\n",
                    gModuleCommandTable[i].Command,
                    gModuleCommandTable[i].Help,
                    gModuleCommandTable[i].exHelp);
            return;
        }
    }

    for (int i = 0; i < gCoreCommandCount; i++)
    {
        if (strcmp(parsedData->Command, gCoreCommandTable[i].Command) == 0)
        {
            xprintf(ANSI_TX_LightYellow "\t%-8s" ANSI_TX_ORG ": %-23s\tex) %s\r\n",
                    gCoreCommandTable[i].Command,
                    gCoreCommandTable[i].Help,
                    gCoreCommandTable[i].exHelp);
        }
    }
}
