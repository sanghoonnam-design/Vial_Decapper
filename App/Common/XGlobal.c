/*******************************************************************************
 * XGlobal.c
 *
 *  Created on: 2024.10.14
 *      Author: RND. Kang PilSoon
 ******************************************************************************/
#include "XGlobal.h"
#include "XDebug.h"

/* task handle ===============================================================*/
//To do Tick의 Max시간을 기록하는 변수 추가작업 필요!! 250919 kyj

uint32_t gTick_LOG;
uint32_t gTick_UTT;
uint32_t gTick_USD;
uint32_t gTick_SDG;
uint32_t gTick_APC;
uint32_t gTick_SFZ;
//uint32_t gTick_NWPT;
//uint32_t gTick_SCPT;

TaskHandle_t gTaskHandle_LOG;     // Task_Handle for Program [L][O][G]o
TaskHandle_t gTaskHandle_UTT;  // Task_Handle for [U]padate [T]rigger [T]ime
TaskHandle_t gTaskHandle_USD;  // Task_Handle for [U]pdate [S]ignalBoard(ELMO+SSP) [D]ata
TaskHandle_t gTaskHandle_SDG;  // Task_Handle for [S]ignal [D]ia[G]nosis
TaskHandle_t gTaskHandle_APC;  // Task_Handle for [A][P]plication [C]ontrol
TaskHandle_t gTaskHandle_SFZ;  // Task_Handle for [D]is[P]osition [P]rocess
TaskHandle_t gTaskHandle_NWPT; // Task_Handle for [N]ewt[W]ork [P]ro[T]ocol
TaskHandle_t gTaskHandle_SCPT; // Task_Handle for [S]erial [C]ommand [P]ro[T]ocol
/*============================================================================*/

int Init_Global(int Index)
{
    //⚠️TODO
    return EXIT_SUCCESS;
}

void xDelay(uint32_t loopcnt)
{
    uint32_t i;

    for (i = 0; i < loopcnt; i++)
    {
        asm("   NOP");
    }
}

void xDelay2(unsigned int delayValue)
{
    volatile uint32_t delay1 = delayValue;
    while (delay1--)
        ;
}

void xDelay_u(uint32_t loopcnt)
{
    uint32_t i, k;

    for (i = 0; i < loopcnt; i++)
    {

        for (k = 0; k < 700; k++)
            asm("   NOP");
    }
}

// void UTIL_DelayMS(U16 wMS)
// {
//     register U16 i;

//     for (i=0; i<wMS; i++)
//         UTIL_DelayUS(1000);         // 1000us => 1ms
// }

// void UTIL_DelayUS(U16 wUS)
// {
//     volatile U32 Dly = (U32)wUS*8;
//     for(; Dly; Dly--);
// }

void swap(float *a, float *b)
{
    float temp;
    temp = *a;
    *a = *b;
    *b = temp;
}

char asctohex(char value)
{
    switch (value)
    {
    case 0x30:
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x37:
    case 0x38:
    case 0x39:
        return (value - '0');
    case 0x41:
    case 0x42:
    case 0x43:
    case 0x44:
    case 0x45:
    case 0x46:
        return (value - 'A' + 10);
    default:
        return 0xFF;
    }
}

void __xuint8ToBit_Fomat4bit(unsigned char v)
{
    unsigned char mask = 0x80; // 상위 비트 (1000 0000)
    int count = 0;

    while (mask)
    {
        if (count % 4 == 0 && count != 0)
            xcprintf(" "); // 4비트 간격으로 공백 추가

        xcprintf("%d", (v & mask) ? 1 : 0); 
        mask >>= 1;                       
        count++;
    }
    //printf("\n\r");
}

void __xuint32ToBit(unsigned int v)
{
    unsigned int ii, s;

    s = (unsigned int)1 << ((sizeof(v) << 3) - 1); // s = only significant bit at 1

    for (ii = s; ii; ii >>= 1)
    {
        xcprintf("%d", v & ii || 0);
    }
}

void __xuint32ToBit_Fomat4bit(unsigned int v)
{
    unsigned int ii, s;
    int count = 0;

    // sizeof(v) << 3은 v의 비트 수를 계산
    s = (unsigned int)1 << ((sizeof(v) << 3) - 1); // s = only significant bit at 1

    for (ii = s; ii; ii >>= 1)
    {
        if ((count % 4) == 0)
            xcprintf(" ");
        if ((count % 8) == 0 && count != 0)
            xcprintf(", ");

        xcprintf("%d", (v & ii) || 0);
        count++;
    }
    xcprintf("\n\r");
}

char upper_ch(char str)
{
    if (str > 96 && str < 123)
        str = str - 32;
    return str;
}

char lower_ch(char str)
{
    if (str > 64 && str < 91)
        str = str + 32;
    return str;
}

int f2i10(float value)
{
    return (int)(value * 10.0f);
}

int f2i100(float value)
{
    return (int)(value * 100.0f);
}

int f2i1000(float value)
{
    return (int)(value * 1000.0f);
}

int isInt10(float value)
{
    return (int)(value * 10.0f) ? 1 : 0;
}

int isInt100(float value)
{
    return (int)(value * 100.0f) ? 1 : 0;
}

int isInt1000(float value)
{
    return (int)(value * 1000.0f) ? 1 : 0;
}

int has_code(char *line, char chr)
{
#if 0
    char *line2 = (line + 1);
    char *p = strchr(line2, chr);

    if(p && *(p-1) != 0x20) {
        return 0;
    }
#endif

    return strchr(line, chr) != NULL;
}

const char *get_str(char *line, char chr)
{
#if 0
    char *line2 = (line+1);
    char *p = strchr(line2, chr) ;
    if(p && *(p-1) !=0x20) {
        return 0;
    }
#endif
    char *ptr = strchr(line, chr);
    return ptr ? ptr + 1 : NULL;
}

int get_int(char *line, char chr)
{
/*
ex)
#include <stdio.h>
#include <stdlib.h> // strtol

int main()
{
    char *s1 = "0xaf10 42 0x27C 9952"; // "0xaf10 42 0x27C 9952"
    int num1;
    int num2;
    int num3;
    int num4;
    char *end;    

    num1 = strtol(s1, &end, 16);     
    num2 = strtol(end, &end, 10);    
    num3 = strtol(end, &end, 16);    
    num4 = strtol(end, NULL, 10);    

    printf("%x\n", num1);    // af10
    printf("%d\n", num2);    // 42
    printf("%X\n", num3);    // 27C
    printf("%d\n", num4);    // 9952

    return 0;
}

[output]
af10
42
27C
9952

*/
#if 0
    char *line2 = (line+1);
    char *p = strchr(line2, chr) ;
    if(p && *(p-1) !=0x20) {
        return 0;
    }
#endif

    char *ptr = strchr(line, chr);
    return ptr ? strtol(ptr + 1, NULL, 10) : 0;
}

uint32_t get_uint(char *line, char chr)
{
#if 0
    char *line2 = (line+1);
    char *p = strchr(line2, chr) ;
    if(p && *(p-1) !=0x20) {
        return 0;
    }
#endif

    char *ptr = strchr(line, chr);
    return ptr ? strtoul(ptr + 1, NULL, 10) : 0;
}

float get_float(char *line, char chr)
{
#if 0
    char *line2 = (line+1);
    char *p = strchr(line2, chr) ;
    if(p && *(p-1) !=0x20) {
        return 0;
    }
#endif

    char *ptr = strchr(line, chr);
    return ptr ? strtod(ptr + 1, NULL) : 0;
}

uint32_t get_bool(char *line, char chr)
{
#if 0
    char *line2 = (line+1);
    char *p = strchr(line2, chr) ;
    if(p && *(p-1) !=0x20) {
        return 0;
    }
#endif

    return get_int(line, chr) ? 1 : 0;
}

#if 0
#define NUM_CONFIGS 4
  // 설정 파일에서 읽은 데이터
char *configData[] = {
    "timeout,100",
    "retry_count,5",
    "max_speed,2000",
    "min_speed,100"
};

// 설정을 저장할 변수들
uint32_t timeout = 0;
uint32_t retryCount = 0;
uint32_t maxSpeed = 0;
uint32_t minSpeed = 0;

int main() {
    char *key, *data;
    char *separator;
    uint32_t temp;
    char *ep;

    // 설정 데이터 파싱 및 파라미터 설정
    for (int i = 0; i < NUM_CONFIGS; i++) {
        key = strtok(configData[i], ",");   // ','로 구분된 첫 번째 부분 (key)
        data = strtok(NULL, ",");           // ','로 구분된 두 번째 부분 (data)

        // 설정 값에 따라 파라미터 설정
        CHECK_SET_PARAM("timeout", timeout);
        CHECK_SET_PARAM("retry_count", retryCount);
        CHECK_SET_PARAM("max_speed", maxSpeed);
        CHECK_SET_PARAM("min_speed", minSpeed);
    }

    // 설정 값 출력
    printf("Timeout: %u\n", timeout);
    printf("Retry Count: %u\n", retryCount);
    printf("Max Speed: %u\n", maxSpeed);
    printf("Min Speed: %u\n", minSpeed);

    return 0;
}
#endif

#if 0
int main() {
    char *key, *data;
    char *separator;
    
    // 설정 데이터 파싱 및 플래그 설정
    for (int i = 0; i < NUM_CONFIGS; i++) {
        key = strtok(configData[i], ","); // ','로 구분된 첫 번째 부분 (key)
        data = strtok(NULL, ",");         // ','로 구분된 두 번째 부분 (data)
        
        // 각 설정에 대해 플래그 설정
        CHECK_SET_FLAG("enable_logging", loggingEnabled);
        CHECK_SET_FLAG("enable_feature", featureEnabled);
        CHECK_SET_FLAG("auto_update", autoUpdateEnabled);
        CHECK_SET_FLAG("dark_mode", darkModeEnabled);
    }
    
    // 결과 출력
    printf("Logging Enabled: %d\n", loggingEnabled);
    printf("Feature Enabled: %d\n", featureEnabled);
    printf("Auto Update Enabled: %d\n", autoUpdateEnabled);
    printf("Dark Mode Enabled: %d\n", darkModeEnabled);

    return 0;
}
#endif
