#include "../core/http_core.h"
#include "http_handlers.h"

#include "../html/generated/app_js.h"
#include "../html/generated/index_html.h"
#include "../html/generated/main_css.h"

#include "SWRTC.h"
#include "XSystemInfo.h"
#include "bootloader.h"

#include <stdio.h>
#include <string.h>

extern SW_DateTime_t gSWRTC;
extern tsXSystemInfo xSystemInfo;

void http_get_root(const http_request_ctx_t *ctx)
{
    http_response_set(ctx->resp, 200, "text/html; charset=utf-8",
                      (const char *)index_html, index_html_len);
}

void http_get_css(const http_request_ctx_t *ctx)
{
    http_response_set(ctx->resp, 200, "text/css; charset=utf-8",
                      (const char *)main_css, main_css_len);
}

void http_get_js(const http_request_ctx_t *ctx)
{
    http_response_set(ctx->resp, 200, "application/javascript; charset=utf-8",
                      (const char *)app_js, app_js_len);
}

void http_get_status(const http_request_ctx_t *ctx)
{
    /* static: response body pointer must remain valid until TX completes */
    static char body[128];
    const char *status;
    int         len;

    status = (BootParamInfo_GetAppConfirm() == APP_CONFIRMED) ? "YES" : "NO";

    len = snprintf(body, sizeof(body),
                   "model=%s\nstatus=%s\ntime=20%02u-%02u-%02u %02u:%02u:%02u\n",
                   MODEL_NAME_STR, status,
                   gSWRTC.year, gSWRTC.month, gSWRTC.day,
                   gSWRTC.hour, gSWRTC.min, gSWRTC.sec);

    if (len < 0 || len >= (int)sizeof(body))
        len = (int)sizeof(body) - 1;

    http_response_set(ctx->resp, 200, "text/plain", body, (uint32_t)len);
}

void http_get_getNetwork(const http_request_ctx_t *ctx)
{
    /* static: response body pointer must remain valid until TX completes */
    static char body[128];
    uint8_t     ip[4];
    uint8_t     subnet[4];
    uint8_t     gateway[4];

    memcpy(ip,      xSystemInfo.network.ip,     4);
    memcpy(subnet,  xSystemInfo.network.subnet, 4);
    memcpy(gateway, xSystemInfo.network.gw,     4);

    snprintf(body, sizeof(body),
             "ip=%u.%u.%u.%u\n"
             "subnet=%u.%u.%u.%u\n"
             "gateway=%u.%u.%u.%u\n",
             ip[0],      ip[1],      ip[2],      ip[3],
             subnet[0],  subnet[1],  subnet[2],  subnet[3],
             gateway[0], gateway[1], gateway[2], gateway[3]);

    http_response_set(ctx->resp, 200, "text/plain", body, (uint32_t)strlen(body));
}
