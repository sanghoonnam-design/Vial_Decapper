/*******************************************************************************
 * XCommand_Core.h
 *
 *  Created on: 2025.06.20
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#ifndef _XCOMMAND_CORE_H_
#define _XCOMMAND_CORE_H_

#include "XGlobal.h"
#include "XParser.h"
#include "XBuffer.h"
#include "_10_XNetworkMsg_Process.h"
#include "_10_XSerialCMD_Process.h"
#include "XDebug.h"

extern tsXBuffer *xSendMsg;
extern SemaphoreHandle_t xMutex_Command;

#if 0
#define BUFFER_SIZE_SENDMESSAGE (200)
#else
#define BUFFER_SIZE_SENDMESSAGE (512)
#endif

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

#define SEND_MESSAGE(__msg, __msgSize, __CommType)                              \
    do                                                                          \
    {                                                                           \
        switch ((int)__CommType)                                                \
        {                                                                       \
        case COMM_RS232C:                                                       \
            Serial_Send(__msg, __msgSize);                                      \
            break;                                                              \
        case COMM_TCP:                                                          \
            TCP_Send(__msg);                                                    \
            break;                                                              \
        case COMM_USB:                                                          \
            CLI_Send_USB(__msg, __msgSize);                                     \
            break;                                                              \
        default:                                                                \
            xprintf("Error: Unsupported communication type: %d\n", __CommType); \
            break;                                                              \
        }                                                                       \
    } while (0)

#define IS_USB(__useTCP) ((__useTCP) == COMM_USB)
#define IS_CMD_GET(__pd) ((__pd)->ParamCount == 0)
#define IS_CMD_PARAM_NONE(__pd) (IS_CMD_GET(__pd))
#define IS_CMD_HELP(__pd) ((__pd)->ParamCount >= 1 && (__pd)->Params[0].value._int == '?')


/************************************************************************* *
 ** @brief Command Handler                                                  *
 ****************************************************************************/
/** @brief 공유 명령 뮤텍스와 응답 버퍼를 생성한다. */
void Init_CommandHandling(void);
/** @brief 파싱된 명령을 뮤텍스로 직렬화하여 Core → Module 순서로 검색하고 응답을 전송한다. */
void Handle_command(const tsXParsedData *parsed_data, U08 useTC);
/** @brief 문자열에 CRLF를 붙이고 통신 경로별 파싱 결과 구조체를 선택하여 명령을 실행한다. */
void Handle_command_by_string(const char *cmdStr, U08 useTCP);

/** @brief src를 dstSize-3까지 복사한 뒤 CRLF와 널 종료를 붙인다. */
void CMD_MakeCommandWithCRLF(char *dst, size_t dstSize, const char *src);
/** @brief 응답 생략 목록에 명령 이름이 있으면 true를 반환한다. */
bool CMD_ShouldSkip_USBResponse(const tsXParsedData *parsedData);
/************************************************************************* */

/************************************************************************* */
/* module 등록용 */
extern const tsXCommandMapping gModuleCommandTable[];
extern const int gModuleCommandCount;
/************************************************************************* */

/* Command prototype */
/** @brief VER/VERS/VERSION: 인자 없이 펌웨어 버전 표시 문자열을 응답 버퍼에 기록한다. */
void CMD_Handle_VERS(const tsXParsedData *parsedData, U08 useTCP);
/** @brief GERR/ERR: 인자 없이 현재 공통 오류 코드 문자열을 응답한다. */
void CMD_Handle_GERR(const tsXParsedData *parsedData, U08 useTCP);
/** @brief GERD/ERD: 인자 없이 현재 공통 오류의 설명 문자열을 응답한다. */
void CMD_Handle_GERD(const tsXParsedData *parsedData, U08 useTCP);
/** @brief 대기 상태에서만 Decapper 오류를 명시적으로 해제한다. 동작/대기 요청이 있으면 BUSY를 응답한다. */
void CMD_Handle_CLER(const tsXParsedData *parsedData, U08 useTCP);

/** @brief RESET/REBO/REBOOT: 안내 출력 후 백업 도메인을 리셋하고 MCU 소프트웨어 리셋을 요청한다. */
void CMD_Handle_REBOOT(const tsXParsedData *parsedData, U08 useTCP);
/** @brief IP/SETIP: 인자 없으면 IP 4개 옥텟과 포트를 응답하며 4개 인자는 RAM의 IP를 변경한다. */
void CMD_Handle_SetGetIP(const tsXParsedData *parsedData, U08 useTCP);
/** @brief 대기 상태에서 공장 설정을 적용하고 EEPROM을 다시 읽어 결과를 출력한다. */
void CMD_Handle_FACTORY(const tsXParsedData *parsedData, U08 useTCP);
/** @brief RTC/DATE: 인자 없으면 년·월·일·시·분·초를 응답하고 콘솔에 표시한다. */
void CMD_Handle_RTC(const tsXParsedData *parsedData, U08 useTCP);

/** @brief DO는 전체 출력 토글, DO 채널은 단일 토글, DO 채널,값은 지정 출력을 설정한다. */
void CMD_Handle_DO(const tsXParsedData *parsedData, U08 useTCP);
/** @brief DI는 전체 디지털 입력을, DI 채널은 1~NUM_IN 중 지정 입력을 콘솔에 출력한다. */
void CMD_Handle_DI(const tsXParsedData *parsedData, U08 useTCP);

/** @brief 대기 상태에서 EEPROM을 gEEPROM으로 읽고 저장 구조를 출력한다. */
void CMD_Handle_LOADE(const tsXParsedData *parsedData, U08 useTCP);
/** @brief 대기 상태에서 Flash 읽기 함수를 호출한다. */
void CMD_Handle_LOADF(const tsXParsedData *parsedData, U08 useTCP);
/** @brief 대기 상태에서 EEPROM/Flash 비교와 저장 구조 출력을 실행한다. */
void CMD_Handle_LOADC(const tsXParsedData *parsedData, U08 useTCP);

/** @brief 대기 상태에서 PL을 EEPROM에 저장한다. */
void CMD_Handle_SAVEE(const tsXParsedData *parsedData, U08 useTCP);
/** @brief 대기 상태에서 Flash 저장 함수를 실행한다. */
void CMD_Handle_SAVEF(const tsXParsedData *parsedData, U08 useTCP);
/** @brief 대기 상태에서 갱신 시각을 기록하고 EEPROM과 Flash 저장 함수를 실행한다. */
void CMD_Handle_SAVEA(const tsXParsedData *parsedData, U08 useTCP);

/** @brief 대기 상태에서 EEPROM 구조를 진단 출력한다. */
void CMD_Handle_PrintParams(const tsXParsedData *parsedData, U08 useTCP);

/** @brief SYSTEM/SYS: MCU 클럭·PLL·식별자·Flash·벡터·인터럽트·주변장치 정보를 콘솔에 출력한다. */
void CMD_Handle_ContFullInfo(const tsXParsedData *parsedData, U08 useTCP);
/** @brief 현재 테이블에서 사용하지 않는 단순 IP 설정 핸들러이다. */
void CMD_Handle_SetIP(const tsXParsedData *parsedData, U08 useTCP);
/** @brief TASK: USB에서 인자 없이 호출하면 RTOS 태스크 상태를 진단 출력한다. */
void CMD_Handle_TaskList(const tsXParsedData *parsedData, U08 useTCP);
/** @brief STACK: USB에서 인자 없이 호출하면 100 ms 대기 후 태스크 스택 상태를 출력한다. */
void CMD_Handle_StackSize(const tsXParsedData *parsedData, U08 useTCP);

/** @brief DB ?: 디버그 플래그를 출력한다. DB 1~4: 진단 비활성·파싱 출력·CAN 수신/송신 출력을 토글한다. */
void CMD_Handle_DebugMode(const tsXParsedData *parsedData, U08 useTCP);
/** @brief SIZE: 인자 없이 EEPROM/SL/CD/PL 구조체의 바이트 크기를 콘솔에 출력한다. */
void CMD_Handle_GetSize(const tsXParsedData *parsedData, U08 useTCP);
/** @brief MODE 정수: 0 기본, 1 IDLE, 2 타이머 정지, 3 APC_STOP, 4 공장 시험 모드 값을 설정한다. */
void CMD_Handle_FWMode(const tsXParsedData *parsedData, U08 useTCP);
/** @brief HT 인덱스: 하드웨어 시험 플래그를 토글하고 HT 100으로 상태를 출력한다. */
void CMD_Handle_HWTest(const tsXParsedData *parsedData, U08 useTCP);

/** @brief TT 인자 1~3개를 테스트 포트 설정에 복사하고 트리거 제어 함수를 실행한다. */
void CMD_Handle_TaskTrigger(const tsXParsedData *parsedData, U08 useTCP);
/** @brief TIMER: 현재 주기 타이머 상태를 반전하고 로그를 출력한다. */
void CMD_Handle_TimerOnOff(const tsXParsedData *parsedData, U08 useTCP);
/** @brief NOP: 아무 동작도 하지 않는 핸들러이다. 명령 프레임 응답은 공통 처리부가 담당한다. */
void CMD_Handle_NoOperation(const tsXParsedData *parsedData, U08 useTCP);

/** @brief CLC: 콘솔 화면 지우기 ANSI 시퀀스를 출력한다. */
void CMD_Handle_ClearScreen(const tsXParsedData *parsedData, U08 useTCP);
/** @brief 인자 개수가 expectedCount와 같고 모든 타입이 INT이면 true를 반환한다. */
bool CMD_CheckParamAll_Int(const tsXParsedData *parsedData, int expectedCount);
/** @brief 인자 개수가 expectedCount와 같고 모든 타입이 FLOAT이면 true를 반환한다. */
bool CMD_CheckParamAll_Float(const tsXParsedData *parsedData, int expectedCount);
/** @brief USB의 ?? 명령으로 Core/Module에 등록된 모든 명령과 분류·설명·예제를 출력한다. */
void CMD_Handle_Help_All(const tsXParsedData *parsedData, U08 useTCP);
/** @brief USB의 ? 명령으로 CLI_COMMAND_SYSTEM 분류의 등록 명령만 출력한다. */
void CMD_Handle_Help(const tsXParsedData *parsedData, U08 useTCP);
/** @brief 파싱된 명령명을 Module → Core 테이블에서 찾아 개별 설명과 예제를 콘솔에 출력한다. */
void CMD_ShowCommandHelp(const tsXParsedData *parsedData);

//=======================================================================

//@ USER CODE - START

//@ USER CODE - END
#endif /* _XCOMMAND_CORE_H_ */
