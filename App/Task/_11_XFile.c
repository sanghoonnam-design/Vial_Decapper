#define __XFILE_C__
#include "_11_XFile.h"
#undef __XFILE_C__

#include "ff.h"
#include <string.h>
#include <stdarg.h>
#include "ISD.h"
#include "XDebug.h"

static void File_Task(void *pvParameters);

static FATFS SDFatFs; /* File system object for SD card logical drive */
static FIL MyFile;    /* File object */

TaskHandle_t gTaskHandle_File;
QueueHandle_t xQueue_FileLog; // 고정길이 큐 사용

static U32 FileWriteCount;

void XFile_Init(void)
{
    xQueue_FileLog = xQueueCreate(20, 100);
    xTaskCreate(File_Task, "File", 256, NULL, 2, &gTaskHandle_File);
}

static void File_Task(void *pvParameters)
{
    static uint8_t buf[100];

    f_mount(&SDFatFs, (TCHAR const *)"0:/", 0);

    while (1)
    {
        if (xQueueReceive(xQueue_FileLog, buf, RTOS_WAIT_FOREVER) == pdPASS)
        {
            U16 len = (U16)strlen((const char *)buf);

            if (BSP_SD_IsDetected() == SD_NOT_PRESENT)
            {
                // SD 카드가 없을 때 처리 (예: 로그를 다른 곳에 저장하거나, 오류 메시지 출력)
                LOG_MSG_SEND("No SD Card");
                FileWriteCount = 0;
                continue;
            }

            if (f_open(&MyFile, "log.csv", FA_WRITE | FA_OPEN_APPEND) != FR_OK)
                continue;
            if (f_write(&MyFile, buf, len, (void *)&len) != FR_OK)
                continue;
            if (f_close(&MyFile) == FR_OK)
                FileWriteCount++;
        }
    }
}

void XFile_Write(const char *fmt, ...)
{
    va_list args;
    static uint8_t buf[100];

    va_start(args, fmt);
    vsnprintf((char *)buf, sizeof(buf), fmt, args);
    va_end(args);

    xQueueSend(xQueue_FileLog, buf, pdMS_TO_TICKS(100));
}

U32 XFile_ReadWriteCount(void)
{
    return FileWriteCount;
}
