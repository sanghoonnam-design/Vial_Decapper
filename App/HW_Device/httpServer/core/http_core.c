#include "http_core.h"
#include "../user/http_handlers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * HTTP 코어 – W5300 소켓 0, 6 동시 지원 (포트 80)
 *
 * ┌──────────┬────────────────────────────────────────────────┐
 * │ 소켓 번호│ 역할                                           │
 * ├──────────┼────────────────────────────────────────────────┤
 * │    0     │ 파일 업로드 / 다운로드 (Keep-Alive 유지)       │
 * │    6     │ 상태폴링 / 일반 요청 (업로드 중 병렬 처리)     │
 * └──────────┴────────────────────────────────────────────────┘
 *
 * W5300은 동일 포트에 여러 소켓이 동시에 LISTEN 할 수 있으므로
 * 두 소켓 모두 포트 80 으로 열면 연결을 자동 분배한다.
 */

#define HTTP_CORE_MAX_SOCKETS   2
#define HTTP_CORE_RX_BUF_SIZE   16384
#define HTTP_CORE_MAX_BODY_SIZE 16000

/* W5300 소켓 번호 → 내부 배열 인덱스 (S0=0, S6=1) */
static const int8_t s_sock_map[8] = { 0, -1, -1, -1, -1, -1, 1, -1 };

static int sock_to_idx(int sock)
{
    if (sock < 0 || sock >= 8)
        return -1;
    return (int)s_sock_map[sock];
}

typedef enum {
    RX_IDLE = 0,
    RX_WAIT_HEADER,
    RX_WAIT_BODY
} rx_state_t;

typedef struct
{
    uint8_t    buf[HTTP_CORE_RX_BUF_SIZE + 1];
    uint32_t   len;
    uint32_t   header_len;
    uint32_t   content_length;
    rx_state_t state;

    http_response_t pending_resp;
    uint8_t         resp_pending;
} sock_state_t;

static sock_state_t       sock_states[HTTP_CORE_MAX_SOCKETS];
static http_request_ctx_t s_req_ctx[HTTP_CORE_MAX_SOCKETS];
static http_response_t    s_resp[HTTP_CORE_MAX_SOCKETS];

static const char *http_reason_phrase(int status)
{
    switch (status)
    {
    case 200: return "OK";
    case 204: return "No Content";
    case 400: return "Bad Request";
    case 404: return "Not Found";
    case 408: return "Request Timeout";
    case 413: return "Payload Too Large";
    case 500: return "Internal Server Error";
    case 503: return "Service Unavailable";
    default:  return "ERR";
    }
}

static void http_sock_reset(sock_state_t *s)
{
    s->len            = 0;
    s->header_len     = 0;
    s->content_length = 0;
    s->state          = RX_IDLE;
    s->resp_pending   = 0;
    memset(&s->pending_resp, 0, sizeof(s->pending_resp));
    s->buf[0] = 0;
}

void http_core_reset_socket(int sock)
{
    int idx = sock_to_idx(sock);
    if (idx < 0)
        return;

    http_sock_reset(&sock_states[idx]);
}

void http_response_init(http_response_t *resp)
{
    if (!resp)
        return;

    memset(resp, 0, sizeof(*resp));
    resp->type = HTTP_RESP_NONE;
}

void http_response_set(http_response_t *resp, int status,
                       const char *content_type,
                       const char *body, uint32_t body_len)
{
    if (!resp)
        return;

    resp->status               = status;
    resp->content_type         = content_type ? content_type : "application/octet-stream";
    resp->extra_headers        = NULL;
    resp->type                 = HTTP_RESP_BUFFER;
    resp->data.buffer.body     = (uint8_t *)body;
    resp->data.buffer.body_len = body_len;
}

void http_response_set_stream(http_response_t *resp, int status,
                               const char *content_type,
                               void *stream_ctx,
                               http_stream_read_fn  read_fn,
                               http_stream_close_fn close_fn,
                               uint32_t total_len)
{
    if (!resp)
        return;

    resp->status                = status;
    resp->content_type          = content_type ? content_type : "application/octet-stream";
    resp->extra_headers         = NULL;
    resp->type                  = HTTP_RESP_STREAM;
    resp->data.stream.ctx       = stream_ctx;
    resp->data.stream.read      = read_fn;
    resp->data.stream.close     = close_fn;
    resp->data.stream.total_len = total_len;
}

void http_response_set_extra_headers(http_response_t *resp, const char *extra_headers)
{
    if (!resp)
        return;

    resp->extra_headers = extra_headers;
}

void http_response_set_keep_alive(http_response_t *resp)
{
    if (!resp)
        return;

    resp->keep_alive = 1;
}

static int http_parse_request(uint8_t *rx_buf, uint32_t rx_len,
                               http_request_ctx_t *ctx)
{
    char    *buf;
    char    *p;
    char    *e;
    char    *body;
    uint32_t m_len;
    uint32_t url_len;

    if (!rx_buf || !ctx || rx_len == 0)
        return 0;

    memset(ctx->method, 0, sizeof(ctx->method));
    memset(ctx->url,    0, sizeof(ctx->url));

    buf = (char *)rx_buf;

    p = strchr(buf, ' ');
    if (!p)
        return 0;

    m_len = (uint32_t)(p - buf);
    if (m_len == 0)
        return 0;
    if (m_len >= sizeof(ctx->method))
        m_len = sizeof(ctx->method) - 1;
    memcpy(ctx->method, buf, m_len);
    ctx->method[m_len] = 0;

    p++;

    e = strchr(p, ' ');
    if (!e)
        return 0;

    url_len = (uint32_t)(e - p);
    if (url_len == 0)
        return 0;
    if (url_len >= sizeof(ctx->url))
        url_len = sizeof(ctx->url) - 1;
    memcpy(ctx->url, p, url_len);
    ctx->url[url_len] = 0;

    body = strstr(buf, "\r\n\r\n");
    if (body)
    {
        body += 4;
        ctx->body     = (uint8_t *)body;
        ctx->body_len = rx_len - (uint32_t)(body - buf);
    }
    else
    {
        ctx->body     = NULL;
        ctx->body_len = 0;
    }

    return 1;
}

static const struct
{
    const char    *method;
    const char    *url;
    http_handler_t handler;
} routes[] = {
#define X(m, u, h) {m, u, h},
    HTTP_ROUTE_TABLE
#undef X
};

static void http_route_dispatch(const http_request_ctx_t *ctx)
{
    int count;
    int i;

    if (!ctx || !ctx->resp)
        return;

    count = (int)(sizeof(routes) / sizeof(routes[0]));

    for (i = 0; i < count; i++)
    {
        if ((strcmp(routes[i].method, ctx->method) == 0) &&
            (strcmp(routes[i].url,    ctx->url)    == 0))
        {
            routes[i].handler(ctx);
            return;
        }
    }

    http_response_set(ctx->resp, 404, "text/plain", "Not Found", 9);
}

static uint32_t http_find_content_length(const char *buf)
{
    const char *cl = strstr(buf, "\r\nContent-Length:");
    if (!cl)
        cl = strstr(buf, "Content-Length:");

    if (!cl)
        return 0;

    cl = strchr(cl, ':');
    if (!cl)
        return 0;

    cl++;

    while (*cl == ' ')
        cl++;

    return (uint32_t)atoi(cl);
}

int http_handle_request(int sock, uint8_t *data, uint32_t data_len)
{
    int           idx = sock_to_idx(sock);
    sock_state_t *s;
    uint32_t      body_rx;

    if (idx < 0)
        return 0;

    s = &sock_states[idx];

    if (s->resp_pending)
        return 0;

    if (!data || data_len == 0)
        return 0;

    if ((s->len + data_len) > HTTP_CORE_RX_BUF_SIZE)
    {
        http_sock_reset(s);
        http_response_init(&s_resp[idx]);
        http_response_set(&s_resp[idx], 413, "text/plain", "Payload Too Large", 17);
        s->pending_resp = s_resp[idx];
        s->resp_pending = 1;
        return 1;
    }

    memcpy(s->buf + s->len, data, data_len);
    s->len += data_len;
    s->buf[s->len] = 0;

    if (s->state == RX_IDLE || s->state == RX_WAIT_HEADER)
    {
        char *end = strstr((char *)s->buf, "\r\n\r\n");
        if (!end)
        {
            s->state = RX_WAIT_HEADER;
            return 0;
        }

        s->header_len     = (uint32_t)((end + 4) - (char *)s->buf);
        s->content_length = http_find_content_length((char *)s->buf);

        if (s->content_length > HTTP_CORE_MAX_BODY_SIZE)
        {
            http_sock_reset(s);
            http_response_init(&s_resp[idx]);
            http_response_set(&s_resp[idx], 413, "text/plain", "Payload Too Large", 17);
            s->pending_resp = s_resp[idx];
            s->resp_pending = 1;
            return 1;
        }

        s->state = (s->content_length == 0) ? RX_IDLE : RX_WAIT_BODY;
    }

    if (s->state == RX_WAIT_BODY)
    {
        if (s->len < s->header_len)
            return 0;

        body_rx = s->len - s->header_len;
        if (body_rx < s->content_length)
            return 0;
    }

    s_req_ctx[idx].sock = (uint8_t)sock;   /* W5300 실제 소켓 번호 보존 */
    http_response_init(&s_resp[idx]);
    s_req_ctx[idx].resp = &s_resp[idx];

    if (!http_parse_request(s->buf, s->len, &s_req_ctx[idx]))
    {
        http_sock_reset(s);
        http_response_init(&s_resp[idx]);
        http_response_set(&s_resp[idx], 400, "text/plain", "Bad Request", 11);
        s->pending_resp = s_resp[idx];
        s->resp_pending = 1;
        return 1;
    }

    http_route_dispatch(&s_req_ctx[idx]);

    s->pending_resp = s_resp[idx];
    s->resp_pending = 1;

    s->len            = 0;
    s->header_len     = 0;
    s->content_length = 0;
    s->state          = RX_IDLE;
    s->buf[0]         = 0;

    return 1;
}

int http_response_build_header(const http_response_t *resp,
                                char *buf, uint32_t buf_len)
{
    uint32_t    content_len;
    int         hlen;
    const char *extra;

    if (!resp || !buf || buf_len == 0)
        return -1;

    if (resp->type == HTTP_RESP_BUFFER)
        content_len = resp->data.buffer.body_len;
    else if (resp->type == HTTP_RESP_STREAM)
        content_len = resp->data.stream.total_len;
    else
        content_len = 0;

    extra = resp->extra_headers ? resp->extra_headers : "";

    hlen = snprintf(buf, buf_len,
                    "%s %d %s\r\n"
                    "Content-Type: %s\r\n"
                    "Content-Length: %lu\r\n"
                    "Connection: %s\r\n"
                    "Cache-Control: no-store\r\n"
                    "%s"
                    "\r\n",
                    resp->keep_alive ? "HTTP/1.1" : "HTTP/1.0",
                    resp->status,
                    http_reason_phrase(resp->status),
                    resp->content_type ? resp->content_type : "text/plain",
                    (unsigned long)content_len,
                    resp->keep_alive ? "keep-alive" : "close",
                    extra);

    if (hlen < 0 || (uint32_t)hlen >= buf_len)
        return -1;

    return hlen;
}

int http_core_response_available(int sock)
{
    int idx = sock_to_idx(sock);
    if (idx < 0)
        return 0;

    return sock_states[idx].resp_pending ? 1 : 0;
}

int http_core_get_response(int sock, http_response_t *out)
{
    int           idx = sock_to_idx(sock);
    sock_state_t *s;

    if (idx < 0 || !out)
        return 0;

    s = &sock_states[idx];

    if (!s->resp_pending)
        return 0;

    *out            = s->pending_resp;
    s->resp_pending = 0;
    memset(&s->pending_resp, 0, sizeof(s->pending_resp));
    return 1;
}
