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
#include "_10_XCommand_Module.h"

tsXBuffer *xSendMsg; // host -> client 전송 메시지버퍼
SemaphoreHandle_t xMutex_Command;

/* 실제로 입력 가능한 이름은 테이블의 Command 열이다.
 * 예: 저장은 SAVE, 읽기는 LOAD, 오류 해제는 CLR/DRT/CLEAR로 등록되어 있다.
 * 함수명·도움말 예제의 SAVEE/LOADE/CLER가 자동으로 별칭 등록되지는 않는다.
 * CommandType은 도움말 분류이며 ExecuteFromTable의 실행 권한 검사가 아니다.
 */
tsXCommandMapping gCoreCommandTable[] = {
	/*User Command*/
	{0, "VER",	/*		*/ CMD_Handle_VERS, /*					*/ "Get the FW version.", /*			*/ "VERSION"},   // 버전 정보 읽기
    {0, "VERS", /*      */ CMD_Handle_VERS, /*					*/ "Get the FW version.", /*			*/ "VERS"},      // 버전 정보 읽기
    {0, "VERSION", /*	*/ CMD_Handle_VERS, /*					*/ "Get the FW version.", /*			*/ "VERSION"},   // 버전 정보 읽기

	{0, "GERR", /*      */ CMD_Handle_GERR, /*					*/ "Get the Error-code.", /*			*/ "GERR"},      // 에러 코드 반환
    {0, "ERR", /*       */ CMD_Handle_GERR, /*					*/ "Get the Error-code.", /*			*/ "ERR"},       // 에러 코드 반환

	{0, "GERD", /*      */ CMD_Handle_GERD, /*					*/ "Get the Error-message.", /*			*/ "GERD"},      // 에러 메시지 반환
    {0, "ERD", /*       */ CMD_Handle_GERD, /*					*/ "Get the Error-message.", /*			*/ "ERD"},       // 에러 메시지 반환

	{0, "CLR", /*		*/ CMD_Handle_CLER, /*					*/ "Clear errors.", /*					*/ "CLER"},      // 에러 클리어
    {0, "DRT", /*       */ CMD_Handle_CLER, /*					*/ "Clear errors.", /*					*/ "DRT"},       // 에러 클리어
    {0, "CLEAR", /*     */ CMD_Handle_CLER, /*					*/ "Clear errors.", /*					*/ "CLEAR"},     // 에러 클리어
	/*Dubug Command*/
	{1, "RESET", /*     */ CMD_Handle_REBOOT, /*				*/ "Reboototing.", /*					*/ "REBO"},                                 // 리부트 실행
	{1, "REBO", /*      */ CMD_Handle_REBOOT, /*				*/ "Reboing.", /*						*/ "REBOOT"},              // 리부트 실행
	{1, "REBOOT", /*    */ CMD_Handle_REBOOT, /*				*/ "Rebooting.", /*						*/ "RESET"},                                // 리부트 실행

	{1, "IP", /*        */ CMD_Handle_SetGetIP, /*				*/ "Set/Get IP", /*						*/ "IP 192,168,0,150"},  // IP 셋팅/읽기
	{1, "SETIP", /*	    */ CMD_Handle_SetGetIP, /*				*/ "Set/Get IP", /*						*/ "IP 192,168,0,150"},  // IP 셋팅/읽기

	{1, "RTC", /*       */ CMD_Handle_RTC, /*					*/ "Set/Get the RTC", /*				*/ "RTC y,m,d,h,min,s"}, // RTC 조회/설정
	{1, "DATE", /*      */ CMD_Handle_RTC, /*					*/ "Set/Get the RTC", /*				*/ "RTC y,m,d,h,min,s"}, // 리부트 실행

	{1, "SYSTEM", /*    */ CMD_Handle_ContFullInfo, /*			*/ "Controller information", /*			*/ "SYSTEM"},              // 하드웨어 정보 출력
	{1, "SYS", /*		*/ CMD_Handle_ContFullInfo, /*			*/ "Controller information", /*			*/ "SYSTEM"},              // 하드웨어 정보 출력

    {1, "FACT", /*      */ CMD_Handle_FACTORY, /*				*/ "Factory setting.", /*				*/ "FACT"},      // factory 셋팅
    {1, "FACTORY", /*   */ CMD_Handle_FACTORY, /*				*/ "Factory setting.", /*				*/ "FACTORY"},   // factory 셋팅

    {1, "DO", /*        */ CMD_Handle_DO, /*					*/ "Set GPIO output.", /*				*/ "DO (ch 1~24), (val 1/0)"},              // GPIO 출력값 쓰기 (중복)
    {1, "DI", /*        */ CMD_Handle_DI, /*					*/ "Get GPIO input.", /*				*/ "DI (ch 1~16)"},                         // GPIO 출력값 쓰기

	{1, "LOAD", /*		*/ CMD_Handle_LOADE, /*					*/ "Read Params from EEPROM.(Ctrl+L)", /*	*/ "LOADE"}, // EEPROM 에서 읽어옴.
	{1, "SAVE", /*      */ CMD_Handle_SAVEE, /*					*/ "Param. save to EEPROM.(Ctrl+S)", /*		*/ "SAVEE"},   // EEPROM 저장

	/*===============================Vial Decapper API 이외의 Command=================================================================================================================================*/
	//{1, "SETIP", /*     */ CMD_Handle_SetIP, /*               */ "Set IP", /*                      */ "SETIP 192,168,0,150"}, // IP 셋팅
	{1, "PPARAM", /*    */ CMD_Handle_PrintParams, /*			*/ "Display Params.", /*				*/ "PPARAM"},    // EEPROM 셋팅값 읽어오기
    {1, "TASK", /*      */ CMD_Handle_TaskList, /*				*/ "Check Task state", /*				*/ "TASK"},                // Task  정보 출력
    {1, "STACK", /*     */ CMD_Handle_StackSize, /*				*/ "Check Task Stack-Size", /*			*/ "STACK"},               // stack size 정보 출력
                                                                                                                                               //
    {1, "DB", /*        */ CMD_Handle_DebugMode, /*				*/ "Toggle Debugging-Flags", /*			*/ "DEBUG ? or (flag index)"},              // @USER CODE, 각 모듈별 디버그 모드를 셋팅한다.
    {1, "SIZE", /*      */ CMD_Handle_GetSize, /*				*/ "Get Size of strut.", /*				*/ "SIZE"},                                 // @USER CODE, 데이터 사이즈 정보 출력용
    {1, "MODE", /*      */ CMD_Handle_FWMode, /*				*/ "Handle the FW-Mode", /*				*/ "MODE 0(0=default,1=idle,2=timer off)"}, // @USER CODE, 시스템 모드
    {1, "HT", /*        */ CMD_Handle_HWTest, /*				*/ "Controller HW Test", /*				*/ "HT 0(?)"},                              // @USER CODE, 제어기 HW 테스트
                                                                                                                                               //
    {1, "TT", /*        */ CMD_Handle_TaskTrigger, /*			*/ "Debug-Trigger on/off", /*			*/ "TRIGGER 0~9"},                          // @USER CODE, TestPort 동작 제어
    {1, "TIMER", /*     */ CMD_Handle_TimerOnOff, /*			*/ "HW Timer on/off(Toggle)", /*		*/ "Timer"},                                // @USER CODE, HW Timer On/Off
    {1, "NOP", /*       */ CMD_Handle_NoOperation, /*			*/ "No Operation Command", /*			*/ "NOP"},                                  // No Operation Code
                                                                                                                                               //
    {1, "CLC", /*       */ CMD_Handle_ClearScreen, /*			*/ "Clear Screen(Console)", /*			*/ "clc"},                                  // clear screen(콘솔)
    {1, "??", /*        */ CMD_Handle_Help_All, /*				*/ "Help(all command)", /*				*/ "??"},                                   // 모든 Help 명령 출력
    {1, "?", /*         */ CMD_Handle_Help, /*					*/ "Help", /*							*/ "?"},                                    // 실제 시스템 에서 사용하는 명령 출력
};

static const int gCoreCommandCount = sizeof(gCoreCommandTable) / sizeof(tsXCommandMapping);
static int gWidth_Command = 23; // default width for help display
static int gWidth_Help = 10;    // default width for command display in help
static void CLI_GetMaxHelpWidth(void);

/**
 * @brief 공유 명령 뮤텍스와 응답 버퍼를 생성한다.
 * 명령 처리 전에 호출한다. 뮤텍스 생성 실패는 대기 루프, 버퍼 생성 실패는 오류 로그로 처리한다.
 */
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
/**
 * @brief 테이블에서 명령 문자열이 일치하는 첫 핸들러를 호출한다.
 * true는 일치 항목을 실행했다는 뜻이며 명령 성공 여부는 핸들러의 응답으로 판단한다.
 */
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

/**
 * @brief 파싱된 명령을 뮤텍스로 직렬화하여 Core → Module 순서로 검색하고 응답을 전송한다.
 * parsedData는 유효한 파싱 결과여야 하며 useTCP는 COMM_USB/TCP/RS232C 구분값이다.
 * USB의 명령 ?는 도움말 출력 후에도 핸들러에 전달한다. 응답 필터는 현재 통신 경로와 무관하게 호출된다.
 */
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

/**
 * @brief 문자열에 CRLF를 붙이고 통신 경로별 파싱 결과 구조체를 선택하여 명령을 실행한다.
 * NULL/빈 문자열·지원하지 않는 경로·파싱 실패는 실행하지 않는다.
 * 임시 버퍼가 static이며 파싱은 명령 뮤텍스 획득 전이므로 동시 호출을 안전하게 보장하는 진입점은 아니다.
 */
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

/**
 * @brief src를 dstSize-3까지 복사한 뒤 CRLF와 널 종료를 붙인다.
 * 초과 입력은 잘리며 기존 CRLF를 제거하지 않는다. 포인터가 없거나 공간이 3 미만이면 쓰지 않는다.
 */
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

/**
 * @brief 응답 생략 목록에 명령 이름이 있으면 true를 반환한다.
 * 이름과 달리 이 함수는 통신 경로를 받지 않는다. 현재 호출부에서는 TCP/직렬 응답에도 이 필터가 적용된다.
 */
bool CMD_ShouldSkip_USBResponse(const tsXParsedData *parsedData)
{
    // USB로 응답/에코 생략할 명령어 리스트, 필요에 따라 추가
    static const char *noEchoOnUSB[] = {
        "CLC", "TASK", "STACK", "?", "??", "??R", "LOADE", "PSTA"};

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

/**
 * @brief VER/VERS/VERSION: 인자 없이 펌웨어 버전 표시 문자열을 응답 버퍼에 기록한다.
 * 모델 접미사가 포함된 xSystemInfo.cd_FWVersion_str을 그대로 사용한다.
 */
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

/**
 * @brief 현재 Core 명령 테이블에 등록되지 않은 FAS I/O 확장용 핸들러이다.
 * 인자 없는 본문은 주석만 있어 I/O 값을 응답하지 않는다.
 */
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

/**
 * @brief GERR/ERR: 인자 없이 현재 공통 오류 코드 문자열을 응답한다.
 * 오류를 해제하거나 Decapper 오류 상태를 새로 동기화하지는 않는다.
 */
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

/**
 * @brief GERD/ERD: 인자 없이 현재 공통 오류의 설명 문자열을 응답한다.
 */
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

/**
 * @brief 대기 상태에서만 Decapper 오류를 명시적으로 해제한다. 동작/대기 요청이 있으면 BUSY를 응답한다.
 */
void CMD_Handle_CLER(const tsXParsedData *parsedData, U08 useTCP){
    if (parsedData->ParamCount == 0){
        if (!CDecap_IsIdle()){
            XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);
            return;
        }
    	CDecap_Error_Clear();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief 명령 처리용 유지보수 잠금을 획득한다. 실패하면 BUSY를 응답하고 false를 반환한다.
 * 성공한 호출부는 타이머 복구 후 EndMaintenance를 호출해야 한다.
 */
static bool CMD_BeginMaintenance(void){
    if (CDecap_BeginMaintenance()) return true;
    XBuffer_AddString(xSendMsg, "BUSY", NO_COMMA);
    return false;
}

/**
 * @brief 대기 상태에서 PL을 EEPROM에 저장한다.
 * RPOS 저장 후보가 있으면 먼저 ZCap_UpPos에 반영한다. 유지보수 잠금 획득 후에만 주기 타이머를 정지한다.
 */
void CMD_Handle_SAVEE(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (!CMD_BeginMaintenance()) return;
        XTimer_Stop();
        if (gZCapUpPosSavePending)
        {
            xPL.Decapper.ZCap_UpPos = gZCapUpPosPendingValue;
            gZCapUpPosSavePending = false;
        }
        // [YYMMDD] parameter를 EEPROM에 저장한 날짜 저장
        xPL.Header.UpdateDate = SWRTC_GetTime_YYMMDDHH();

        EEPROMPL_SaveToEEPROM();
        XTimer_Start();
        CDecap_EndMaintenance();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief 대기 상태에서 Flash 저장 함수를 실행한다.
 * 유지보수 잠금으로 새 모션 접수를 막고 타이머 복구 뒤 잠금을 해제한다.
 */
void CMD_Handle_SAVEF(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (!CMD_BeginMaintenance()) return;
        XTimer_Stop();
        EEPROMPL_SaveToFlash();
        XTimer_Start();
        CDecap_EndMaintenance();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief 대기 상태에서 갱신 시각을 기록하고 EEPROM과 Flash 저장 함수를 실행한다.
 * SAVEE와 달리 RPOS 저장 후보를 반영하는 코드는 이 핸들러에 없다.
 */
void CMD_Handle_SAVEA(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (!CMD_BeginMaintenance()) return;
        XTimer_Stop();
        // [YYMMDD] parameter를 EEPROM에 저장한 날짜 저장
        xPL.Header.UpdateDate = SWRTC_GetTime_YYMMDDHH();

        EEPROMPL_SaveToEEPROMandFlash();
        XTimer_Start();
        CDecap_EndMaintenance();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief 대기 상태에서 EEPROM을 gEEPROM으로 읽고 저장 구조를 출력한다.
 * 읽기 동안 유지보수 잠금을 유지한다. Decapper 드라이버 재초기화를 직접 호출하지 않는다.
 */
void CMD_Handle_LOADE(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (!CMD_BeginMaintenance()) return;
        XTimer_Stop();
        EEPROMPL_LoadFromEEPROM(&gEEPROM);
        EEPROMPL_PrintEepromStructure_user();
        XTimer_Start();
        CDecap_EndMaintenance();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief 대기 상태에서 Flash 읽기 함수를 호출한다.
 * 주기 타이머 정지 전 유지보수 잠금을 얻고 복구 후 해제한다.
 */
void CMD_Handle_LOADF(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (!CMD_BeginMaintenance()) return;
        XTimer_Stop();
        EEPROMPL_LoadFromFlash(&gEEPROM);
        XTimer_Start();
        CDecap_EndMaintenance();
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
/**
 * @brief 대기 상태에서 EEPROM/Flash 비교와 저장 구조 출력을 실행한다.
 * 타이머를 정지하는 진단 작업이므로 새 모션 요청을 유지보수 잠금으로 막는다.
 */
void CMD_Handle_LOADC(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (!CMD_BeginMaintenance()) return;
        XTimer_Stop();
        EEPROMPL_CompareEEPROMandFlash();
        EEPROMPL_PrintEepromStructure_user();
        XTimer_Start();
        CDecap_EndMaintenance();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief 대기 상태에서 공장 설정을 적용하고 EEPROM을 다시 읽어 결과를 출력한다.
 * Decapper 동작 중에는 BUSY로 거절하며 잠금은 타이머 복구 후 해제한다.
 */
void CMD_Handle_FACTORY(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (!CMD_BeginMaintenance()) return;
        XTimer_Stop(); // EEPROM 을 다룰때는 반드시 HW Timer를 끄고 한다.
        xprintf("wait....");
        SYSPL_FactorySetting();                       // Factory 셋팅하고,
        memset(&gEEPROM, 0, sizeof(tsEEPROM_Config)); // 현재 구조체 리셋,
        EEPROMPL_LoadFromEEPROM(&gEEPROM);            // 저장이 잘되었는지 다시 읽어오고,
        EEPROMPL_PrintEepromStructure_user();         // 읽어온거 확인한다..
        XTimer_Start();
        CDecap_EndMaintenance();                               // HW Timer 다시 enable
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief 대기 상태에서 EEPROM 구조를 진단 출력한다.
 * 현재 구현이 타이머를 정지하므로 출력 작업에도 유지보수 잠금을 적용한다.
 */
void CMD_Handle_PrintParams(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        if (!CMD_BeginMaintenance()) return;
        XTimer_Stop();
        EEPROMPL_PrintEepromStructure_user();
        XTimer_Start();
        CDecap_EndMaintenance();
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief DI는 전체 디지털 입력을, DI 채널은 1~NUM_IN 중 지정 입력을 콘솔에 출력한다.
 * 사용자 채널 번호를 0 기준으로 바꿔 읽는다. 응답 버퍼에 입력 목록을 넣는 방식은 아니다.
 */
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

/**
 * @brief DO는 전체 출력 토글, DO 채널은 단일 토글, DO 채널,값은 지정 출력을 설정한다.
 * 채널은 1 기준이다. 값은 양수면 HIGH, 나머지는 LOW로 해석한다.
 * 99는 I/O 초기화, 100은 호출마다 다음 출력 하나를 토글한다. 실행 전 디지털 루프백을 해제한다.
 * Decapper FSM을 거치지 않고 공압/그리퍼와 공유하는 실제 출력을 직접 변경하는 시험 명령이다.
 */
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

/**
 * @brief RESET/REBO/REBOOT: 안내 출력 후 백업 도메인을 리셋하고 MCU 소프트웨어 리셋을 요청한다.
 * 인자 개수와 Busy를 검사하지 않으며 정상 응답 전송 전에 재부팅될 수 있다.
 */
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


/**
 * @brief RTC/DATE: 인자 없으면 년·월·일·시·분·초를 응답하고 콘솔에 표시한다.
 * 설정은 정수 6개이며 년은 0~99이다. 각 필드 범위를 검사하고 날짜 변경 플래그를 설정한다.
 * 월별 일수와 윤년 조합 검사는 이 핸들러에 없다.
 */
void CMD_Handle_RTC(const tsXParsedData *parsedData, U08 useTCP)
{
    SW_DateTime_t dt;

    // help
    if (IS_USB(useTCP) && IS_CMD_HELP(parsedData))
    {
        int i = 1;
        xprintf("\tCommand: [RTC] [year, month, day, hour, min, sec]");
        xprintf("\t===============================");
        xprintf("\t %d) RTC                      : Get the current RTC.", i++);
        xprintf("\t %d) RTC xx,xx,xx,xx,xx,xx,xx : Set a new RTC.", i++);
        xprintf("\r\n");
        xprintf("\t ex) Get : RTC ");
        xprintf("\t ex) Set : RTC 25,02,22,3,12,10 => 25/2/22, 03:12:10");
        return;
    }

    // Get the current RTC
    if (IS_CMD_PARAM_NONE(parsedData))
    {
        SWRTC_GetParam(&dt);
        XBuffer_AddU08(xSendMsg, dt.year, COMMA);
        XBuffer_AddU08(xSendMsg, dt.month, COMMA);
        XBuffer_AddU08(xSendMsg, dt.day, COMMA);
        XBuffer_AddU08(xSendMsg, dt.hour, COMMA);
        XBuffer_AddU08(xSendMsg, dt.min, COMMA);
        XBuffer_AddU08(xSendMsg, dt.sec, NO_COMMA);

        xprintf("Date: " ANSI_TX_LightGreen
                "20%02d-%02d-%02d, %02d:%02d:%02d" ANSI_TX_ORG,
                dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);
        return;
    }

    // set a new RTC
    else if (CMD_CheckParamAll_Int(parsedData, 6) &&
             parsedData->Params[0].value._int >= 0 && parsedData->Params[0].value._int <= 99 && // year
             parsedData->Params[1].value._int >= 1 && parsedData->Params[1].value._int <= 12 && // month
             parsedData->Params[2].value._int >= 1 && parsedData->Params[2].value._int <= 31 && // date
             parsedData->Params[3].value._int >= 0 && parsedData->Params[3].value._int <= 23 && // hour
             parsedData->Params[4].value._int >= 0 && parsedData->Params[4].value._int <= 59 && // min
             parsedData->Params[5].value._int >= 0 && parsedData->Params[5].value._int <= 59)   // sec
    {
        dt.year /*  */ = parsedData->Params[0].value._int;
        dt.month /* */ = parsedData->Params[1].value._int;
        dt.day /*  */ = parsedData->Params[2].value._int;
        dt.hour /*  */ = parsedData->Params[3].value._int;
        dt.min /*   */ = parsedData->Params[4].value._int;
        dt.sec /*   */ = parsedData->Params[5].value._int;

        SWRTC_SetParam(&dt);

        SWRTC_GetParam(&dt);
        LOG_MSG_SEND(ANSI_TX_LightGreen "[RTC Set]: 20%02d-%02d-%02d, %02d:%02d:%02d" ANSI_TX_ORG,
                     dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);

        xSystemInfo.sl_isDateChanged = YES; // 날짜 변경 플래그 세팅
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);
        if (parsedData->Params[0].value._int != '?')
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 0);
    }
}

/**
 * @brief IP/SETIP: 인자 없으면 IP 4개 옥텟과 포트를 응답하며 4개 인자는 RAM의 IP를 변경한다.
 * 현재 EEPROM 저장과 Network_Init 호출은 주석 상태이므로 여기서 영구 저장/즉시 네트워크 재설정을 하지 않는다.
 * 현재 구현은 검사 전에 타이머를 멈추며 잘못된 옥텟이나 전체 0/255 주소의 조기 반환에서는 타이머를 복구하지 않는다.
 */
void CMD_Handle_SetGetIP(const tsXParsedData *parsedData, U08 useTCP)
{
    // help
    if (IS_USB(useTCP) && IS_CMD_HELP(parsedData))
    {
        int i = 1;
        xprintf("\tCommand: [IP] [xx,xx,xx,xx]");
        xprintf("\t===============================");
        xprintf("\t %d) IP             : Get the current IP address.", i++);
        xprintf("\t %d) IP xx,xx,xx,xx : Set a new IP address. " ANSI_TX_LightGreen "Use commas (,) as separators." ANSI_TX_ORG, i++);
        return;
    }

    // Get current IP address if no parameters, or set new IP if 4 parameters are provided
    if (parsedData->ParamCount == 0)
    {
        XBuffer_AddInt(xSendMsg, xSystemInfo.network.ip[0], COMMA);
        XBuffer_AddInt(xSendMsg, xSystemInfo.network.ip[1], COMMA);
        XBuffer_AddInt(xSendMsg, xSystemInfo.network.ip[2], COMMA);
        XBuffer_AddInt(xSendMsg, xSystemInfo.network.ip[3], COMMA);
        XBuffer_AddInt(xSendMsg, xSystemInfo.network.portNum, NO_COMMA);

        xprintf("current IP : %d.%d.%d.%d, %d",
                (int)xSystemInfo.network.ip[0],
                (int)xSystemInfo.network.ip[1],
                (int)xSystemInfo.network.ip[2],
                (int)xSystemInfo.network.ip[3],
                (int)xSystemInfo.network.portNum);
    }
    else if (parsedData->ParamCount == 4)
    {
        U8 ip[4];

        XTimer_Stop();
        for (int i = 0; i < 4; i++)
        {
            int value = parsedData->Params[i].value._int;

            if (value < 0 || value > 255)
            {
                SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
                ERR_MSG_SEND("%s(%d): Invalid IP octet %d (must be 0–255)", __func__, __LINE__, value);
                return;
            }

            ip[i] = (U8)value;
        }

        if ((ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0) ||
            (ip[0] == 255 && ip[1] == 255 && ip[2] == 255 && ip[3] == 255))
        {
            SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
            ERR_MSG_SEND("%s(%d): Invalid IP address (all 0s or all 255s)", __func__, __LINE__);
            return;
        }

        SystemInfo_Network_SetIP(&xSystemInfo.network, ip);
        //// Network_Init(); // 수정된거 반영
        // EEPROMPL_SaveToEEPROM();

        __newLine();
        LOG_MSG_SEND("IP address set to " ANSI_TX_LightGreen "%d.%d.%d.%d" ANSI_TX_ORG, ip[0], ip[1], ip[2], ip[3]);
        LOG_MSG_SEND(ANSI_TX_LightRed "Please [REBOOT] after changing the IP address." ANSI_TX_ORG);

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

/**
 * @brief DB ?: 디버그 플래그를 출력한다. DB 1~4: 진단 비활성·파싱 출력·CAN 수신/송신 출력을 토글한다.
 * 5/6 분기는 현재 주석만 있어 동작하지 않는다. 1번은 true가 진단 비활성을 의미한다.
 */
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

/**
 * @brief SIZE: 인자 없이 EEPROM/SL/CD/PL 구조체의 바이트 크기를 콘솔에 출력한다.
 * SL/CD/PL 크기가 4의 배수가 아니면 진단 로그를 남긴다. 메모리 배치를 수정하지는 않는다.
 */
void CMD_Handle_GetSize(const tsXParsedData *parsedData, U08 useTCP)
{
    if (parsedData->ParamCount == 0)
    {
        __newLine();
        xprintf("\tex) SIZE --> Get size of structures.\r\n");

        int j = 1;
        U32 size;

        xprintf("\t\t[idx] [size] : [structure]");
        xprintf("\t\t====================================");
        xprintf("\t\t[%2d] %4d Bytes: size of gEEPROM.", j++, sizeof(tsEEPROM_Config));
        
        size = sizeof(tsXStateList);
        xprintf("\t\t[%2d] %4d Bytes: size of xSL.", j++, size);
        if (size % 4 != 0)
            ERR_MSG_SEND("[ALIGN WARNING] tsXStateList size is not multiple of 4 (%d)", size);

        size = sizeof(tsXControlData);
        xprintf("\t\t[%2d] %4d Bytes: size of xCD.", j++, size);
        if (size % 4 != 0)
            ERR_MSG_SEND("[ALIGN WARNING] tsXControlData size is not multiple of 4 (%d)", size);

        size = sizeof(tsXParameterList);
        xprintf("\t\t[%2d] %4d Bytes: size of xPL.", j++, size);
        if (size % 4 != 0)
            ERR_MSG_SEND("[ALIGN WARNING] tsXParameterList size is not multiple of 4 (%d)", size);
    }
    else
    {
        SetErrorCode(ERROR_CODE_INVALID_ARGUMENT, __func__, __LINE__);
        XBuffer_AddString(xSendMsg, GetErrorCode_char(), NO_COMMA);

        if (parsedData->Params[0].value._int != '?')
            ERR_MSG_SEND("Oops!~ Invalid Command format.");
    }
}

/**
 * @brief MODE 정수: 0 기본, 1 IDLE, 2 타이머 정지, 3 APC_STOP, 4 공장 시험 모드 값을 설정한다.
 * 0/3/4는 필요 시 타이머를 시작한다. 실제 모드별 동작은 각 태스크가 해당 값을 검사하는 방식에 따른다.
 * 모터 정지 명령을 보내는 함수가 아니므로 STOP과 구분한다.
 */
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

/**
 * @brief HT 인덱스: 하드웨어 시험 플래그를 토글하고 HT 100으로 상태를 출력한다.
 * 실제 구현은 0~4 루프백, 10 EEPROM, 11 SD, 13 스위치이다. 6~9는 빈 분기다.
 * 도움말에 표시되는 5 DAC/12 RTC는 현재 실행 분기가 없다. 시험 실행은 해당 플래그를 읽는 쪽에서 이루어진다.
 */
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

/**
 * @brief TT 인자 1~3개를 테스트 포트 설정에 복사하고 트리거 제어 함수를 실행한다.
 * 그 밖의 개수는 첫 설정을 0으로 한다. 이 함수는 타입/범위를 검증하거나 남은 설정을 모두 초기화하지 않는다.
 */
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

/**
 * @brief TIMER: 현재 주기 타이머 상태를 반전하고 로그를 출력한다.
 * 인자나 Decapper Busy를 검사하지 않으며 하드웨어 모터 정지와는 별개이다.
 */
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

/**
 * @brief NOP: 아무 동작도 하지 않는 핸들러이다. 명령 프레임 응답은 공통 처리부가 담당한다.
 */
void CMD_Handle_NoOperation(const tsXParsedData *parsedData, U08 useTCP)
{
    // No Operation Code
}

/**
 * @brief CLC: 콘솔 화면 지우기 ANSI 시퀀스를 출력한다.
 * 장비 상태·오류·데이터를 초기화하는 명령은 아니다.
 */
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
/**
 * @brief SYSTEM/SYS: MCU 클럭·PLL·식별자·Flash·벡터·인터럽트·주변장치 정보를 콘솔에 출력한다.
 * 레지스터/HAL 조회값을 사용하며 클럭이나 하드웨어 설정을 변경하지 않는다.
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

/**
 * @brief 현재 테이블에서 사용하지 않는 단순 IP 설정 핸들러이다.
 * 인자 4개를 U8로 변환하여 RAM에 복사하며 값 범위 검사·저장·네트워크 재초기화는 하지 않는다.
 * 등록된 IP/SETIP는 CMD_Handle_SetGetIP로 연결되어 있다.
 */
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

/**
 * @brief TASK: USB에서 인자 없이 호출하면 RTOS 태스크 상태를 진단 출력한다.
 * 다른 경로 또는 인자가 있으면 잘못된 인자로 처리한다.
 */
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

/**
 * @brief STACK: USB에서 인자 없이 호출하면 100 ms 대기 후 태스크 스택 상태를 출력한다.
 * 통신 응답 버퍼 대신 진단 출력 함수를 사용한다.
 */
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

/**
 * @brief 인자 개수가 expectedCount와 같고 모든 타입이 INT이면 true를 반환한다.
 * NULL은 false이다. 숫자 범위나 의미는 검사하지 않으며 expectedCount는 호출부가 유효하게 지정해야 한다.
 */
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

/**
 * @brief 인자 개수가 expectedCount와 같고 모든 타입이 FLOAT이면 true를 반환한다.
 * 정수를 실수로 자동 인정하지 않는다. 값의 범위 검사는 호출부의 책임이다.
 */
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

/**
 * @brief Core/Module 테이블의 명령명과 도움말 최대 길이로 출력 열 너비를 계산한다.
 * 현재 unused 보조 함수이며 호출되지 않으면 초기 열 너비를 사용한다.
 */
__attribute__((unused)) static void CLI_GetMaxHelpWidth(void)
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

/**
 * @brief USB의 ?? 명령으로 Core/Module에 등록된 모든 명령과 분류·설명·예제를 출력한다.
 * 미등록 핸들러는 표시하지 않는다. 다른 통신 경로에서는 출력하지 않는다.
 */
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

/**
 * @brief USB의 ? 명령으로 CLI_COMMAND_SYSTEM 분류의 등록 명령만 출력한다.
 * 디버그 명령을 포함한 전체 목록은 ??를 사용한다.
 */
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

/**
 * @brief 파싱된 명령명을 Module → Core 테이블에서 찾아 개별 설명과 예제를 콘솔에 출력한다.
 * 실행부의 검색 순서와는 다르며 여기서는 핸들러를 실행하지 않는다.
 */
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
