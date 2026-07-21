#include "../core/http_core.h"
#include "XSystemInfo.h"
#include "bootloader.h"
#include "http_handlers.h"

#include "SWRTC.h"
#include "XDebug.h"
#include "http_server_task.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern tsXSystemInfo xSystemInfo;
extern SW_DateTime_t gSWRTC;

#define BUILD_DATETIME (__DATE__ " " __TIME__)

static int parse_ip(const char *s, uint8_t ip[4])
{
    long value;
    int i;
    char *endptr;

    if (!s || !ip)
        return 0;

    for (i = 0; i < 4; i++)
    {
        value = strtol(s, &endptr, 10);
        if (endptr == s || value < 0 || value > 255)
            return 0;

        ip[i] = (uint8_t)value;

        if (i < 3)
        {
            if (*endptr != '.')
                return 0;
            s = endptr + 1;
        }
        else
        {
            if (*endptr != '\0')
                return 0;
        }
    }

    return 1;
}

void http_post_gotoboot(const http_request_ctx_t *ctx)
{
    makeVersionString();
    BootInfo_Init((uint8_t *)MODEL_NAME_STR,
                  (uint8_t *)xSystemInfo.cd_FWVersion_str,
                  (uint8_t *)BUILD_DATETIME);
    BootInfo_SetFwUpdatFlag(BOOTLOADER_BOOT_MODE);
    http_response_set(ctx->resp, 200, "text/plain", NULL, 0);
}

void http_post_appconfirm(const http_request_ctx_t *ctx)
{
    BootParamInfo_SetAppConfirm(APP_CONFIRMED);
    http_response_set(ctx->resp, 200, "text/plain", NULL, 0);
}

void http_post_settime(const http_request_ctx_t *ctx)
{
    int year;
    int month;
    int day;
    int hour;
    int min;
    int sec;

    if (ctx->body &&
        sscanf((char *)ctx->body, "time=%d-%d-%d %d:%d:%d",
               &year, &month, &day, &hour, &min, &sec) == 6)
    {
        gSWRTC.year = (uint8_t)(year % 100);
        gSWRTC.month = (uint8_t)month;
        gSWRTC.day = (uint8_t)day;
        gSWRTC.hour = (uint8_t)hour;
        gSWRTC.min = (uint8_t)min;
        gSWRTC.sec = (uint8_t)sec;
        gSWRTC.ms = 0;

        SWRTC_SetParam(&gSWRTC);
        http_response_set(ctx->resp, 200, "text/plain", NULL, 0);
    }
    else
    {
        http_response_set(ctx->resp, 400, "text/plain", "Invalid time format", 19);
    }
}

void http_post_factorySet(const http_request_ctx_t *ctx)
{
    (void)ctx;

    XTimer_Stop();
    SYSPL_FactorySetting();
    LOG_MSG_SEND("Factory setting done.");
    XTimer_Start();

    http_response_set(ctx->resp, 200, "text/plain", NULL, 0);
}

void http_post_systemReset(const http_request_ctx_t *ctx)
{
    (void)ctx;

    xcprintf("   Bye Bye Bye!~   \r\n");

    RCC->BDCR |= RCC_BDCR_BDRST;
    RCC->BDCR &= ~RCC_BDCR_BDRST;

    http_server_SetSystemResetFlag();

    http_response_set(ctx->resp, 200, "text/plain", NULL, 0);
}

void http_post_setNetwork(const http_request_ctx_t *ctx)
{
    /* static: response body pointer must remain valid until TX completes */
    static char body[128];
    uint8_t ip[4];
    uint8_t subnet[4];
    uint8_t gateway[4];
    uint8_t ip_ok = 0;
    uint8_t subnet_ok = 0;
    uint8_t gateway_ok = 0;
    char *line;
    char *cr;

    if (!ctx || !ctx->resp || !ctx->body || ctx->body_len == 0)
    {
        http_response_set(ctx->resp, 400, "text/plain", "Empty body", 10);
        return;
    }

    memset(ip, 0, sizeof(ip));
    memset(subnet, 0, sizeof(subnet));
    memset(gateway, 0, sizeof(gateway));

    line = strtok((char *)ctx->body, "\n");

    while (line)
    {
        cr = strchr(line, '\r');
        if (cr)
            *cr = 0;

        if (strncmp(line, "ip=", 3) == 0)
            ip_ok = parse_ip(line + 3, ip);
        else if (strncmp(line, "subnet=", 7) == 0)
            subnet_ok = parse_ip(line + 7, subnet);
        else if (strncmp(line, "gateway=", 8) == 0)
            gateway_ok = parse_ip(line + 8, gateway);

        line = strtok(NULL, "\n");
    }

    if (!ip_ok || !subnet_ok || !gateway_ok)
    {
        http_response_set(ctx->resp, 400, "text/plain", "Invalid network format", 22);
        return;
    }

    memcpy(xSystemInfo.network.ip, ip, 4);
    memcpy(xSystemInfo.network.subnet, subnet, 4);
    memcpy(xSystemInfo.network.gw, gateway, 4);

    XTimer_Stop();
    // [YYMMDD] parameter를 EEPROM에 저장한 날짜 저장
    xPL.Header.UpdateDate = SWRTC_GetTime_YYMMDDHH();

    EEPROMPL_SaveToEEPROM();
    XTimer_Start();

    snprintf(body, sizeof(body),
             "ip=%u.%u.%u.%u\n"
             "subnet=%u.%u.%u.%u\n"
             "gateway=%u.%u.%u.%u\n",
             ip[0], ip[1], ip[2], ip[3],
             subnet[0], subnet[1], subnet[2], subnet[3],
             gateway[0], gateway[1], gateway[2], gateway[3]);

    http_response_set(ctx->resp, 200, "text/plain", body, (uint32_t)strlen(body));
}
