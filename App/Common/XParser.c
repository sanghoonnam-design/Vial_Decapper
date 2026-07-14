/*******************************************************************************
 * XParser.c
 *
 *  Created on: 2024.10.14
 *      Author: RND. Kang PilSoon.
 ******************************************************************************/
#include "XParser.h"
#include "XDebug.h"

tsXParsedData xParsedData_RS232C;  // Serial Command 파싱된 데이터
tsXParsedData xParsedData_Network;  // Network Command 파싱된 데이터
tsXParsedData xParsedData_USB;  // USB Command 파싱된 데이터

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
    return isalnum(c) || c == ' ' || c == ',' || c == '.' || c == '-' ||
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

#if 0 
// 노이즈가 있는지 체크하는 함수 (노이즈 감지)
static teXParsingErrorCode xParser_DetectNoise(const char *buffer)
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
        if (data->ParamCount == 0 && (!strcmp(token, "?") || !strcmp(token, "H") || !strcmp(token, "h") || !strcmp(token, "s") || !strcmp(token, "S"))) // 명령에 대한 help 파라미터 처리
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
        ERR_MSG_SEND_N("%s(%d): Invalid command format.", functionName, position);
        break;
    case PARSER_ERR_NOISE_DETECTED:
        ERR_MSG_SEND_N("%s(%d): Noise detected in input.", functionName, position);
        break;
    case PARSER_ERR_TIMEOUT:
        ERR_MSG_SEND_N("%s(%d): Time out.", functionName, position);
        break;
    case PARSER_ERR_HELP_COMMAND_EXPLANATION: // not used.
        // printf("Info: Help requested for a specific command.\n");
        break;
    case PARSER_ERR_HELP_COMMAND_LIST_CALLED: // not used.
        // printf("Info: Help command list has been called.\n");
        break;
    default:
        ERR_MSG_SEND_N("%s(%d): Unknown error.", functionName, position);
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
