/*******************************************************************************
 * XParser.h
 *
 *  Created on: 2024.10.14
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#ifndef _XPARSER_H_
#define _XPARSER_H_

#include "XGlobal.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_CMD_LEN (20)         // 최대 명령어 길이
#define MAX_PARAMS (10)          // 최대 파라미터 개수
#define BUFFER_SIZE_RX_MSG (256) // 링 버퍼 크기  // TODO

typedef enum
{
    PARSER_ERR_SUCCESS = 0,                  // no error,
    PARSER_ERR_INVALID_CMD = 1,              // error,
    PARSER_ERR_NOISE_DETECTED = 2,           // error, 노이즈 감지
    PARSER_ERR_TIMEOUT = 3,                  // error, time out
    PARSER_ERR_HELP_COMMAND_EXPLANATION = 4, // help, 명령어 설명
    PARSER_ERR_HELP_COMMAND_LIST_CALLED = 5, // help, 명령어 리스트 설명
    PARSER_ERR_CODE_COUNT
} teXParsingErrorCode;

typedef enum
{
    PARAM_TYPE_NONE /* */ = 0, // 입력 없음
    PARAM_TYPE_INT /*  */ = 1, // int 타입 파라미터
    PARAM_TYPE_FLOAT /**/ = 2  // float 타입 파라미터
} ParamType;

typedef struct
{
    ParamType type;   // 파라미터의 타입
                      //
    union             // 공용체를 사용함.
    {                 //
        int _int;     // int 값
        float _float; // float 값
    } value;          //
} tsParam;

typedef struct
{
    char RawBuffer[50];
    char Command[MAX_CMD_LEN];
    tsParam Params[MAX_PARAMS]; // 파라미터 배열 (int 또는 float 지원)
    int ParamCount;
} tsXParsedData;

extern tsXParsedData xParsedData_RS232C;  // serial
extern tsXParsedData xParsedData_Network; // network
extern tsXParsedData xParsedData_USB;     // usb

extern U08 DB_isPrintParsingDataEnabled; // 디버깅용

teXParsingErrorCode xParser_ProcessReceivedData(const char *rxBuffer, tsXParsedData *parsedData, bool useTCP); // 시리얼 데이터를 처리하고 노이즈 필터링하는 함수
void xParser_HandleError(const char *functionName, teXParsingErrorCode errorCode, U08 position);               // 에러 메시지를 출력하는 함수

#endif /*_XPARSER_H_*/
