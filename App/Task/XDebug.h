/*******************************************************************************
 * XDebug.h
 *
 *  Created on: 2024.10.14
 *      Author: RND. Kang PilSoon.
 *
 ******************************************************************************/
#ifndef __DEBUG_XDEBUG_H__
#define __DEBUG_XDEBUG_H__

#include "XGlobal.h"
#include "XSystemInfo.h"
#include "HW_Serial.h"
#include "_01_XSystemManagement.h"

/** ****************************************************************************
 * @brief [Debugging Code 3] usb 포트를 이용한 디버깅 코드
 * @note   - 메시지 큐를 이용하여 주요 태스크가 수행된 후에 로그 메시지가
 *           처리되도록 설계됨
 * *****************************************************************************/
#define used_STATIC_MEMORY
#ifdef used_STATIC_MEMORY
/**/ #define __MSGLOG_SIZE (200) // [NOTION] -사이즈를 키우지 마세요!~
/**/ #define __MSGLOG_NUM (10)   //          -변경 하지 마요.
#else
/**/                                                     \
#define __MSGLOG_SIZE                                      \
    (180) // [NOTION] -사이즈를 키우지 마세요!~
/**/ \
#define __MSGLOG_NUM                                       \
    (10) //          -변경 하지 마요.
#endif

extern uint8_t __message[__MSGLOG_SIZE];
extern QueueHandle_t xQueue_txLOG;
#define xcprintf(__fmt, ...) /*             */ Debug_LogMessageHelper(__fmt, ##__VA_ARGS__)
#define xcprintf_xy(__fmt, __x, __y, ...) /**/ Debug_LogMessageHelper("\033[s\033[" #__x ";" #__y "H" __fmt "\033[u", ##__VA_ARGS__)
#define xcprintf_t(__fmt, ...) /*           */ Debug_LogMessageHelper(ANSI_TX_LightCyan "\r\n%02d%02d%02d %02d:%02d:%02d.%02d " ANSI_TX_ORG __fmt, gSWRTC.year, gSWRTC.month, gSWRTC.day, gSWRTC.hour, gSWRTC.min, gSWRTC.sec, gSWRTC.ms / 10, ##__VA_ARGS__);
#define xprintf(__fmt, ...) /*              */ Debug_LogMessageHelper(__fmt "\r\n\0", ##__VA_ARGS__)
#define LOG_MSG_SEND(__fmt, ...) /*         */ Debug_LogMessageHelper(ANSI_TX_LightCyan "\r%02d%02d%02d %02d:%02d:%02d.%02d " ANSI_TX_ORG __fmt "\r\n\0", gSWRTC.year, gSWRTC.month, gSWRTC.day, gSWRTC.hour, gSWRTC.min, gSWRTC.sec, gSWRTC.ms / 10, ##__VA_ARGS__)
#define LOG_MSG_SEND_N(__fmt, ...) /*       */ Debug_LogMessageHelper(ANSI_TX_LightCyan "\r\n%02d%02d%02d %02d:%02d:%02d.%02d " ANSI_TX_ORG __fmt "\r\n\0", gSWRTC.year, gSWRTC.month, gSWRTC.day, gSWRTC.hour, gSWRTC.min, gSWRTC.sec, gSWRTC.ms / 10, ##__VA_ARGS__) // new line
#define ERR_MSG_SEND(__fmt, ...) /*         */ Debug_LogMessageHelper(ANSI_TX_LightCyan "\r%02d%02d%02d %02d:%02d:%02d.%02d " ANSI_TX_LightRed "[ERR] " ANSI_TX_ORG __fmt "\r\n\0", gSWRTC.year, gSWRTC.month, gSWRTC.day, gSWRTC.hour, gSWRTC.min, gSWRTC.sec, gSWRTC.ms / 10, ##__VA_ARGS__)
#define ERR_MSG_SEND_N(__fmt, ...) /*       */ Debug_LogMessageHelper(ANSI_TX_LightCyan "\r\n%02d%02d%02d %02d:%02d:%02d.%02d " ANSI_TX_LightRed "[ERR] " ANSI_TX_ORG __fmt "\r\n\0", gSWRTC.year, gSWRTC.month, gSWRTC.day, gSWRTC.hour, gSWRTC.min, gSWRTC.sec, gSWRTC.ms / 10, ##__VA_ARGS__) // new line

#define ERR_MSG_SEND2(__fmt, ...) /*         */ Debug_LogMessageHelper(ANSI_TX_LightCyan "\r%02d%02d%02d %02d:%02d:%02d.%02d " ANSI_TX_LightMagenta "[ERR] " ANSI_TX_ORG __fmt "\r\n\0", gSWRTC.year, gSWRTC.month, gSWRTC.day, gSWRTC.hour, gSWRTC.min, gSWRTC.sec, gSWRTC.ms / 10, ##__VA_ARGS__)
void Debug_LogMessageHelper(const char *fmt, ...);
VOID TASK_USBMessagePrint(void *pvParameters);
//==============================================================================

/** ****************************************************************************
 * @brief [Debugging Code 4] CLI(command line interface)
 * @note    - USB 포트를 이용한 CLI
 *          - Tera Term 기준으로 개발됨.
 *          - J1C는 일반 콘솔프로그램이 아니므로 정상동작 안할 수 있음.
 * *****************************************************************************/
extern QueueHandle_t xQueue_rxCLI;  // USB CLI command 처리용
#define __CLI_MAX_CMD_LENGTH (50)   // 최대 명령어 길이
#define __CLI_MAX_HISTORY_SIZE (15) // 히스토리 최대 크기

#define __CLI_QUEUE_SIZE (__CLI_MAX_CMD_LENGTH) // 큐에 저장할 수 있는 최대 메시지 수
#define __CLI_QUEUE_ITEM_SIZE (1)               // 큐에서 한 번에 수신할 수 있는 데이터 크기 (1바이트)

extern U08 gNewLine;
#define __prompt() /*    */ xcprintf("RND> ");
#define __prompt_ln() /* */ xcprintf("\r\nRND> ");
#define __prompt_r() /*  */ xcprintf("\rRND> ");
#define __prompt_n() /*  */ xcprintf("\nRND> ");
#define __newLine()         \
    do                      \
    {                       \
        gNewLine = YES;     \
        xcprintf("\r\n\0"); \
    } while (0);

#define moveCursor(__row, __col) /*      */ xcprintf("\033[%d;%dH", (row), (col))
#define clearConsole() /*                */ xcprintf(ANSI_CLEAR_TERMINAL)
#define clearConsole2() /*               */ xcprintf(ANSI_CLEAR_TERMINAL_2)
#define ANSI_CLEAR_TERMINAL /*           */ "\033[H\033[J"  // 콘솔창 클리어 (화면 전체)
#define ANSI_CLEAR_TERMINAL_2 /*         */ "\033[2J\033[H" // 콘솔창 클리어 (콘솔 전체)
#define ANSI_ESC /*                      */ '\033'          // ANSI Escape 시작
#define ANSI_CTRL_OFFSET /*              */ (64)            // Ctrl 키 조합의 ASCII 계산 기준 (대문자)
#define ANSI_SAVE_CURSOR_POSITION /*     */ "\033[s"        // 현재 커서 위치 저장 (Save Cursor Position).
#define ANSI_RESTORE_CURSOR_POSITION /*  */ "\033[u"        // 저장된 위치로 복원 (Restore Cursor Position).
#define ANSI_DELETE_LEFTSIDE /*          */ "\033[1K"       // 커서 기준 왼쪽라인 삭제
#define ANSI_DELETE_RIGHTSIDE /*         */ "\033[K"        // 커서 기준 오른쪽라인 삭제
#define ANSI_DELETE_LINE /*              */ "\033[2K"       // 현재 줄 전체 삭제
/* Text */                                                  //
#define ANSI_TX_ORG /*                   */ "\033[39m"      // 기본 텍스트 색으로 복구
#define ANSI_TX_Black /*                 */ "\033[30m"      // 검정색 텍스트
#define ANSI_TX_Red /*                   */ "\033[31m"      // 빨간색 텍스트
#define ANSI_TX_Green /*                 */ "\033[32m"      // 초록색 텍스트
#define ANSI_TX_Yellow /*                */ "\033[33m"      // 노란색 텍스트
#define ANSI_TX_Blue /*                  */ "\033[34m"      // 파란색 텍스트
#define ANSI_TX_Magenta /*               */ "\033[35m"      // 자홍색(보라색) 텍스트
#define ANSI_TX_Cyan /*                  */ "\033[36m"      // 청록색 텍스트
#define ANSI_TX_White /*                 */ "\033[37m"      // 흰색 텍스트
                                                            //
/* Text: 밝은 색 (High Intensity)*/                         //
#define ANSI_TX_Gray /*                  */ "\033[90m"      // 밝은 검정 (회색)
#define ANSI_TX_LightRed /*              */ "\033[91m"      //
#define ANSI_TX_LightGreen /*            */ "\033[92m"      //
#define ANSI_TX_LightYellow /*           */ "\033[93m"      //
#define ANSI_TX_LightBlue /*             */ "\033[94m"      //
#define ANSI_TX_LightMagenta /*          */ "\033[95m"      //
#define ANSI_TX_LightCyan /*             */ "\033[96m"      //
#define ANSI_TX_BrightWhite /*           */ "\033[97m"      //
                                                            //
/* Background */                                            //
#define ANSI_BG_ORG /*                   */ "\033[49m"      // 기본 배경색으로 복구
#define ANSI_BG_BRIGHT_Black /*          */ "\033[100m"     // 밝은 검정(회색) 배경
#define ANSI_BG_BRIGHT_Red /*            */ "\033[101m"     // 밝은 빨강 배경
#define ANSI_BG_BRIGHT_Green /*          */ "\033[102m"     // 밝은 초록 배경
#define ANSI_BG_BRIGHT_Yellow /*         */ "\033[103m"     // 밝은 노랑 배경
#define ANSI_BG_BRIGHT_Blue /*           */ "\033[104m"     // 밝은 파랑 배경
#define ANSI_BG_BRIGHT_Magenta /*        */ "\033[105m"     // 밝은 자홍 배경
#define ANSI_BG_BRIGHT_Cyan /*           */ "\033[106m"     // 밝은 청록 배경
#define ANSI_BG_BRIGHT_White /*          */ "\033[107m"     // 밝은 흰색 배경
/* Style */                                                 //
#define ANSI_ST_RESET /*                 */ "\033[0m"       // 모든 스타일 초기화
#define ANSI_ST_BOLD /*                  */ "\033[1m"       // 굵은 텍스트
#define ANSI_ST_DIM /*                   */ "\033[2m"       // 어두운 텍스트
#define ANSI_ST_ITALIC /*                */ "\033[3m"       // 기울임 텍스트
#define ANSI_ST_UNDERLINE /*             */ "\033[4m"       // 밑줄 텍스트
#define ANSI_ST_BLINK /*                 */ "\033[5m"       // 깜빡이는 텍스트
#define ANSI_ST_INVERTED /*              */ "\033[7m"       // 배경/전경 색상 반전
#define ANSI_ST_HIDDEN /*                */ "\033[8m"       // 숨겨진 텍스트
#define ANSI_ST_STRIKE /*                */ "\033[9m"       // 텍스트

// Special Function Key codes (상위 ESC 파서에서 매핑해서 내려줄 값)
#define KEY_F1 (0x80)
#define KEY_F2 (0x81)
#define KEY_F3 (0x82)
#define KEY_F4 (0x83)
#define KEY_F5 (0x84)
#define KEY_F6 (0x85)
#define KEY_F7 (0x86)
#define KEY_F8 (0x87)
#define KEY_F9 (0x88)
#define KEY_F10 (0x89)
#define KEY_F11 (0x8A)
#define KEY_F12 (0x8B)

typedef struct
{
    char command[__CLI_MAX_CMD_LENGTH];
} tsXCommandHistory;

extern char inputBuffer[__CLI_MAX_CMD_LENGTH]; // 현재 입력 상태
extern uint16_t inputIndex;                    //

//==============================================================================
VOID Task_USBCommandLineInterface(void *pvParameters); // cli, for debugging
void CLI_Send_USB(const char *msg, uint16_t size);
void CLI_AddToHistory(const char *cmd);
void CLI_ClearCurrentInput(void);
const char *CLI_GetPrevHistory(void);
const char *CLI_GetNextHistory(void);
void CLI_UpdateStatusPanel(void);

#define COL_CONTENTS_WIDTH 17 //
#define COL_VALUE_WIDTH 37    //
#define COL_LABEL_WIDTH 64    //
void CLI_PrintAligned(int width, int index, const char *contentsFmt, const char *valueFmt, const char *label, ...);
//==============================================================================

//==============================================================================
// [Saving control]
//==============================================================================
extern int gWait_SaveConfirm;
extern TickType_t gSaveConfirmTick;
#define SAVE_CONFIRM_TIMEOUT_MS (5000)
bool CLI_HandleSave(char c);
void CLI_CheckSaveConfirmTimeout(void);
//==============================================================================

/** ****************************************************************************
 * @brief [Debugging Code 6] 시스템 정보
 * @note    - 메모리 사용량 분석
 * ******************************************************************************/
U32 HW_Get_RAM_usage(void);
U32 HW_Get_Flash_usage(void);
//==============================================================================

//==============================================================================
int Init_Debug(int index);
//==============================================================================


#endif /* __DEBUG_XDEBUG_H__ */
