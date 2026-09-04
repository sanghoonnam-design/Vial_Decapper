/*******************************************************************************
 * XParser.c
 *
 *  Created on: 2024.10.14
 *      Author: RND. Kang PilSoon.
 *
 ******************************************************************************/
#include "XParser.h"
#include "XDebug.h"

tsXParsedData xParsedData_RS232C;  // Serial Command 파싱된 데이터
tsXParsedData xParsedData_Network; // Network Command 파싱된 데이터
tsXParsedData xParsedData_USB;     // USB Command 파싱된 데이터

static teXParsingErrorCode xParser_ParseCommand( // 명령어와 파라미터를 파싱하는 함수
    const char *buffer,                          //
    tsXParsedData *data);                        //
static int xParser_IsValidChar(char c);          // 유효한 문자인지 확인하는 함수
static void xParser_TrimNewline(char *str);      // CR/LF 문자를 제거하는 함수
#if 0
static teXParsingErrorCode xParser_DetectNoise(const char *buffer);  // 노이즈가 있는지 체크하는 함수 (노이즈 감지)
#endif
static void xParser_PrintParsedData(const tsXParsedData *data); // 파싱된 데이터를 출력하는 함수 (디버깅용)

U08 DB_isPrintParsingDataEnabled = NO; // 디버깅 코드: 파싱데이터를 display 하는 조건

// 노이즈 필터링 : 유효한 문자인지 확인하는 함수
static int xParser_IsValidChar(char c)
{
    return isalnum(c) || c == ' ' || c == ',' || c == '.' || c == '-' || c == '_' ||
           c == '\r' || c == '\n' || c == '?' || c == 'h' || c == 'H' ||
           c == 's' || c == 'S';
}

// CR/LF 문자를 제거하는 함수
static void xParser_TrimNewline(char *str)
{
    if (str == NULL || strlen(str) == 0)
        return;

    char *p = str + strlen(str) - 1;

    while (p >= str && (*p == '\r' || *p == '\n'))
    {
        *p-- = '\0';
    }
}

#if 1
// 노이즈가 있는지 체크하는 함수 (노이즈 감지)
__attribute__((unused)) teXParsingErrorCode xParser_DetectNoise(const char *buffer)
{
    for (int i = 0; buffer[i] != '\0'; i++)
    {
        if (!xParser_IsValidChar(buffer[i]))
        {
            return PARSER_ERR_NOISE_DETECTED;
        }
    }

    return PARSER_ERR_SUCCESS;
}
#endif

#if 0
static teXParsingErrorCode xParser_ParseCommand(const char *buffer, tsXParsedData *data)
{
    char tempBuffer[BUFFER_SIZE_RX_MSG];

    strncpy(tempBuffer, buffer, BUFFER_SIZE_RX_MSG);
    tempBuffer[BUFFER_SIZE_RX_MSG - 1] = '\0'; // 널 종료 추가, strncpy()함수 버그 보정
    xParser_TrimNewline(tempBuffer);           // CR/LF 제거

    char *token = strtok(tempBuffer, " ");
    if (token == NULL)
    {
        xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 1);
        return PARSER_ERR_INVALID_CMD; // 명령어가 유효하지 않음
    }

    for (int i = 0; token[i] != '\0'; i++) // 일단! 대문자로 변환해봐.
    {
        token[i] = UPCASE(token[i]);
    }

    // 명령어 복사 (널 종료 처리)
    strncpy(data->Command, token, MAX_CMD_LEN - 1);
    data->Command[MAX_CMD_LEN - 1] = '\0'; // 명령어에 널 종료 추가

    // 파라미터가 있는지 확인
    data->ParamCount = 0;
    token = strtok(NULL, " ,"); // 파라미터 파싱 시작

    // 파라미터가 없을 경우에도 에러 없이 처리
    while (token != NULL && data->ParamCount < MAX_PARAMS)
    {
        if (data->ParamCount == 0 && (!strcmp(token, "?") ||
                                      !strcmp(token, "H") || !strcmp(token, "h") ||
                                      !strcmp(token, "s") || !strcmp(token, "S"))) // 명령에 대한 help 파라미터 처리
        {
            data->Params[data->ParamCount].type = PARAM_TYPE_INT;
            data->Params[data->ParamCount].value._int = (int)*token;
            data->ParamCount++;
            break;
        }

        char *endptr;
        if (strchr(token, '.')) //[1]. 소숫점 처리
        {
            float value = strtof(token, &endptr);
            if (*endptr != '\0')
            {
                xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 4);
                return PARSER_ERR_INVALID_CMD;
            }
            data->Params[data->ParamCount].type = PARAM_TYPE_FLOAT;
            data->Params[data->ParamCount].value._float = value;
        }
        else //[2]. 정수 처리
        {
            int value = (int)strtol(token, &endptr, 10);
            if (*endptr != '\0')
            {
                xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 5);
                return PARSER_ERR_INVALID_CMD;
            }
            data->Params[data->ParamCount].type = PARAM_TYPE_INT;
            data->Params[data->ParamCount].value._int = value;
        }

        data->ParamCount++;
        token = strtok(NULL, " ,");
    }

    // 파라미터가 너무 많을 경우 에러 처리
    if (data->ParamCount >= MAX_PARAMS)
    {
        xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 6);
        return PARSER_ERR_INVALID_CMD; // 파라미터가 너무 많음
    }

    return PARSER_ERR_SUCCESS; // 파싱 성공
}
#else

static teXParsingErrorCode xParser_ParseCommand(const char *buffer, tsXParsedData *data)
{
    char tempBuffer[BUFFER_SIZE_RX_MSG];

    strncpy(tempBuffer, buffer, BUFFER_SIZE_RX_MSG);
    tempBuffer[BUFFER_SIZE_RX_MSG - 1] = '\0';
    xParser_TrimNewline(tempBuffer);

    // 파라미터 초기화
    for (int i = 0; i < MAX_PARAMS; i++)
    {
        data->Params[i].type = PARAM_TYPE_NONE;
        data->Params[i].value._int = 0;
    }
    data->ParamCount = 0;

    char *p = tempBuffer;

    // 앞 공백 스킵
    while (*p == ' ')
        p++;

    if (*p == '\0')
        return PARSER_ERR_INVALID_CMD;

    // ---------------------------------------------------------
    // COMMAND 파싱
    // ---------------------------------------------------------
    int cmdLen = 0;
    while (*p != ' ' && *p != '\0')
    {
        if (cmdLen < MAX_CMD_LEN - 1)
            data->Command[cmdLen++] = UPCASE(*p);
        p++;
    }
    data->Command[cmdLen] = '\0';

    // COMMAND 뒤 공백 스킵
    while (*p == ' ')
        p++;

    // 파라미터가 없는 경우
    if (*p == '\0')
        return PARSER_ERR_SUCCESS;

    // ---------------------------------------------------------
    // PARAM 파싱 시작
    // ---------------------------------------------------------
    int paramIndex = 0;
    char tokenBuf[64];
    int tLen = 0;
    bool hasDot = false;

    while (1)
    {
        char ch = *p;
        bool isEnd   = (ch == '\0');
        bool isComma = (ch == ',');
        bool isSpace = (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');

        // -----------------------------------------------------
        // 구분자(콤마/EOF/공백)에 도달한 경우 → 토큰 종료
        // -----------------------------------------------------
        if (isComma || isEnd || isSpace)
        {
            // 공백만 있는 토큰이면 tLen == 0일 수 있음
            while (tLen > 0 && tokenBuf[tLen - 1] == ' ')
                tLen--;

            if (tLen == 0)
            {
                // 빈 파라미터(NONE)
                data->Params[paramIndex].type = PARAM_TYPE_NONE;
                data->Params[paramIndex].value._int = 0;
            }
            else
            {
                // 토큰 종료
                tokenBuf[tLen] = '\0';

                // HELP 처리
                if (paramIndex == 0 &&
                    (!strcmp(tokenBuf, "?") || !strcmp(tokenBuf, "H") ||
                     !strcmp(tokenBuf, "h") || !strcmp(tokenBuf, "S") || !strcmp(tokenBuf, "s")))
                {
                    data->Params[0].type = PARAM_TYPE_INT;
                    data->Params[0].value._int = tokenBuf[0];
                    data->ParamCount = 1;
                    return PARSER_ERR_SUCCESS;
                }

                // 한 글자 영문 파라미터는 대문자 ASCII 값으로 저장
                if ((tLen == 1) && isalpha((unsigned char)tokenBuf[0]))
                {
                    data->Params[paramIndex].type = PARAM_TYPE_INT;
                    data->Params[paramIndex].value._int = UPCASE(tokenBuf[0]);
                }
                // FLOAT 또는 INT 파싱
                else
                {
                    char *endptr;
                    if (hasDot)
                    {
                        float fv = strtof(tokenBuf, &endptr);
                        if (*endptr != '\0')
                            return PARSER_ERR_INVALID_CMD;

                        data->Params[paramIndex].type = PARAM_TYPE_FLOAT;
                        data->Params[paramIndex].value._float = fv;
                    }
                    else
                    {
                        int iv = strtol(tokenBuf, &endptr, 10);
                        if (*endptr != '\0')
                            return PARSER_ERR_INVALID_CMD;

                        data->Params[paramIndex].type = PARAM_TYPE_INT;
                        data->Params[paramIndex].value._int = iv;
                    }
                }
            }

            // 파라미터 카운트 증가
            paramIndex++;
            data->ParamCount = paramIndex;

            // 파라미터 최대치 검사
            if (paramIndex >= MAX_PARAMS)
                break;

            // 마지막이면 종료
            if (isEnd)
                break;

            // 다음 토큰 준비
            tLen = 0;
            hasDot = false;

            // 공백의 경우는 단순 스킵
            if (isSpace)
            {
                p++;
                continue;
            }

            // 콤마인 경우 다음 문자로 이동
            if (isComma)
            {
                p++;
                continue;
            }
        }
        else
        {
            if (ch == '.')
                hasDot = true;

            if (!isSpace && tLen < (int)sizeof(tokenBuf) - 1)
            {
                tokenBuf[tLen++] = ch;
            }
        }

        p++;
    }

    return PARSER_ERR_SUCCESS;
}
#endif

// 시리얼 데이터를 처리하고 노이즈 필터링하는 함수: 2중 처리
teXParsingErrorCode xParser_ProcessReceivedData(const char *rxBuffer, tsXParsedData *parsedData, bool useTCP)
{
    char cleanBuffer[BUFFER_SIZE_RX_MSG] = {0};
    int bufferIndex = 0;
    BOOL crDetected = false; // CR(\r)을 감지했는지 여부


    /*[1]. 노이즈 필터링 및 CR-LF 시퀀스 확인 */
    for (int i = 0; rxBuffer[i] != '\0' && bufferIndex < BUFFER_SIZE_RX_MSG; i++)
    {
        char ch = rxBuffer[i];

        if (crDetected)
        {
            if (ch == '\n')
            {
                strncpy(parsedData->RawBuffer, cleanBuffer, bufferIndex);
                
                parsedData->RawBuffer[bufferIndex] = '\0';
                break;
            }
            else
            {
                xParser_HandleError(__func__, PARSER_ERR_NOISE_DETECTED, 1);
                return PARSER_ERR_NOISE_DETECTED; // CR 뒤에 LF가 오지 않으면 잘못된 시퀀스이므로 에러 처리
            }
        }

        // 유효한 문자인지 확인 : 노이즈 체크
        if (xParser_IsValidChar(ch))
        {
            if (ch == '\r') // CR을 감지하면 플래그 설정
            {
                crDetected = true; // CR 감지 플래그 설정
            }
            else
            {
                cleanBuffer[bufferIndex++] = ch; // 유효한 문자는 버퍼에 저장
            }
        }
        else
        {
            xParser_HandleError(__func__, PARSER_ERR_NOISE_DETECTED, 2);
            return PARSER_ERR_NOISE_DETECTED; // 유효하지 않은 문자는 노이즈로 처리
        }

        if (bufferIndex >= BUFFER_SIZE_RX_MSG)
        {
            xParser_HandleError(__func__, PARSER_ERR_INVALID_CMD, 3);
            return PARSER_ERR_INVALID_CMD; // 버퍼 오버플로우 방지
        }
    }

    // if (useTCP == COMM_RS232C && crDetected && bufferIndex == 0)  // CR-LF가 연속적으로 오지 않았다면 에러 처리
    if (crDetected && bufferIndex == 0) // CR-LF가 연속적으로 오지 않았다면 에러 처리
    {
        xParser_HandleError(__func__, PARSER_ERR_NOISE_DETECTED, 4);
        return PARSER_ERR_NOISE_DETECTED;
    }

    /*[2]. 명령어 파싱 성공 후 crDetected 초기화 */
    teXParsingErrorCode result = xParser_ParseCommand(cleanBuffer, parsedData);

    /** ***********************************************************
     *  @brief for debugging, 파싱 결과 확인할때 사용
     * ************************************************************/
    if (DB_isPrintParsingDataEnabled)
    {
        xParser_PrintParsedData(parsedData);
    }

    if (result == PARSER_ERR_SUCCESS)
    {
        crDetected = false; // 명령어가 성공적으로 처리되면 플래그 초기화
    }

    return result;
}

void xParser_HandleError(const char *functionName, teXParsingErrorCode errorCode, U08 position)
{
    vTaskDelay(100);

    switch (errorCode)
    {
    case PARSER_ERR_INVALID_CMD:
        ERR_MSG_SEND("%s(%d): Invalid command format.", functionName, position);
        break;
    case PARSER_ERR_NOISE_DETECTED:
        ERR_MSG_SEND("%s(%d): Noise detected in input.", functionName, position);
        break;
    case PARSER_ERR_TIMEOUT:
        ERR_MSG_SEND("%s(%d): Time out.", functionName, position);
        break;
    case PARSER_ERR_HELP_COMMAND_EXPLANATION: // not used.
        // printf("Info: Help requested for a specific command.\n");
        break;
    case PARSER_ERR_HELP_COMMAND_LIST_CALLED: // not used.
        // printf("Info: Help command list has been called.\n");
        break;
    default:
        ERR_MSG_SEND("%s(%d): Unknown error.", functionName, position);
        break;
    }
}

// 파싱된 데이터를 출력하는 함수 (디버깅용)
static void xParser_PrintParsedData(const tsXParsedData *data)
{
    xprintf("\tCommand     : %s", data->Command);
    xprintf("\tParam.Count : %d", data->ParamCount);
    char paramBuffer[256];
    int offset = 0;

    for (int i = 0; i < data->ParamCount; i++)
    {
        if (data->Params[i].type == PARAM_TYPE_INT)
        {
            offset += snprintf(paramBuffer + offset, sizeof(paramBuffer) - offset,
                               "%s%d", (i == 0 ? "" : ","), data->Params[i].value._int);
        }
        else if (data->Params[i].type == PARAM_TYPE_FLOAT)
        {
            offset += snprintf(paramBuffer + offset, sizeof(paramBuffer) - offset,
                               "%s%7.3f", (i == 0 ? "" : ","), data->Params[i].value._float);
        }

        // 버퍼 오버플로우 방지
        if (offset >= sizeof(paramBuffer) - 1)
        {
            break;
        }
    }

    xprintf("\tParameters  : %s\r\n", paramBuffer);
}
