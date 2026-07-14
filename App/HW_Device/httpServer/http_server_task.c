/*******************************************************************************
 * http_server_task.c
 *
 * WIZnet W5300 기반 이중 소켓 HTTP/1.0-1.1 서버 태스크.
 *
 * ─ 소켓 구성 ─────────────────────────────────────────────────
 *   소켓 0 (S0) : 포트 80, 파일 업로드/다운로드, Keep-Alive
 *   소켓 6 (S6) : 포트 80, 상태폴링/일반 요청, 업로드 중 병렬 처리
 *
 *   W5300은 동일 포트에 여러 소켓이 동시에 LISTEN 가능하므로
 *   두 소켓이 자동으로 들어오는 연결을 분담한다.
 *
 * ─ TX 상태 머신 ──────────────────────────────────────────────
 *   TX_IDLE → TX_HEADER → TX_BODY | TX_STREAM
 *
 * ─ Keep-Alive ────────────────────────────────────────────────
 *   resp.keep_alive == 1 이면 TX_BODY 완료 후 소켓을 유지하고
 *   다음 HTTP 요청을 동일 TCP 연결로 수신한다.
 *   업로드 청크가 모두 끝나면 상태폴링(GET /status)이 이 연결로
 *   오고, /status 응답(keep_alive=0)에서 연결이 정상 종료된다.
 ******************************************************************************/
#include "http_server_task.h"
#include "bootloader.h"
#include "core/http_core.h"
#include "project.h"
#include "socket.h"
#include "wizchip_conf.h"
#include "XDebug.h"

/* ──────────────────────────────────────────────────────
 * 설정 상수
 * ────────────────────────────────────────────────────── */
#define HTTP_PORT 80
#define HTTP_MAX_SOCKETS 2                 /* S0, S6 두 소켓 동시 운영   */
#define HTTP_IO_BUFFER_SIZE 8192           /* 수신/스트림 청크 버퍼 크기  */
#define HTTP_HEADER_BUFFER_SIZE 256        /* HTTP 응답 헤더 최대 크기    */
#define HTTP_REQ_TIMEOUT_MS 3000U          /* 요청 수신 타임아웃(ms)      */
#define HTTP_TX_PROGRESS_TIMEOUT_MS 10000U /* TX 무진행 타임아웃(ms)      */
#define HTTP_RESET_DELAY_MS 100U           /* 리셋 전 지연(ms)            */

/* W5300 소켓 번호 → 내부 배열 인덱스 (S0=0, S6=1) */
static const int8_t s_sock_map[8] = {0, -1, -1, -1, -1, -1, 1, -1};

static int sock_to_idx(int sock)
{
    if (sock < 0 || sock >= 8)
        return -1;
    return (int)s_sock_map[sock];
}

/* ──────────────────────────────────────────────────────
 * TX 상태
 * ────────────────────────────────────────────────────── */
typedef enum
{
    TX_IDLE = 0,
    TX_HEADER,
    TX_BODY,
    TX_STREAM
} tx_state_t;

/* ──────────────────────────────────────────────────────
 * 소켓별 TX 컨텍스트
 * ────────────────────────────────────────────────────── */
typedef struct
{
    tx_state_t state;
    http_response_t resp;

    char header[HTTP_HEADER_BUFFER_SIZE];
    uint32_t header_len;
    uint32_t header_offset;

    uint32_t body_offset;

    uint32_t stream_len;
    uint32_t stream_offset;

    uint8_t io_buffer[HTTP_IO_BUFFER_SIZE];

    uint32_t req_tick; /* 마지막 데이터 수신 시각 */
    uint32_t tx_tick;  /* 마지막 TX 진행 시각     */
    uint8_t connected;
} tx_ctx_t;

/* ──────────────────────────────────────────────────────
 * 내부 상태
 * ────────────────────────────────────────────────────── */
static tx_ctx_t tx_ctx[HTTP_MAX_SOCKETS];
static uint8_t systemResetFlag = 0;

/* ──────────────────────────────────────────────────────
 * 내부 함수 선언
 * ────────────────────────────────────────────────────── */
static void http_server_reset_ctx(int sock);
static void http_server_abort_response(int sock);
static void http_server_close_socket(int sock);
static int send_partial(uint8_t sn, const uint8_t *buf,
                        uint32_t *offset, uint32_t len);
static void process_rx(int sock);
static void process_tx(int sock);

/* ══════════════════════════════════════════════════════
 * Public API
 * ══════════════════════════════════════════════════════ */

void http_server_SetSystemResetFlag(void)
{
    systemResetFlag = 1;
}

/* ══════════════════════════════════════════════════════
 * 내부 헬퍼
 * ══════════════════════════════════════════════════════ */

static void http_server_reset_ctx(int sock)
{
    int idx = sock_to_idx(sock);
    if (idx < 0)
        return;

    memset(&tx_ctx[idx], 0, sizeof(tx_ctx_t));
    tx_ctx[idx].state = TX_IDLE;

    http_core_reset_socket(sock);
}

static void http_server_abort_response(int sock)
{
    int idx = sock_to_idx(sock);
    tx_ctx_t *ctx;

    if (idx < 0)
        return;

    ctx = &tx_ctx[idx];

    if ((ctx->state == TX_STREAM) &&
        (ctx->resp.type == HTTP_RESP_STREAM) &&
        ctx->resp.data.stream.close)
    {
        ctx->resp.data.stream.close(ctx->resp.data.stream.ctx);
    }

    memset(&ctx->resp, 0, sizeof(ctx->resp));
    ctx->state = TX_IDLE;
    ctx->header_len = 0;
    ctx->header_offset = 0;
    ctx->body_offset = 0;
    ctx->stream_len = 0;
    ctx->stream_offset = 0;
}

static void http_server_close_socket(int sock)
{
    int idx = sock_to_idx(sock);
    if (idx < 0)
        return;

    disconnect((uint8_t)sock);
    http_server_abort_response(sock);
    http_core_reset_socket(sock);
    tx_ctx[idx].connected = 0;
}

/*
 * send_partial : TX 버퍼 여유만큼 부분 전송
 *
 * 반환값
 *   1  : 전체 [offset, len) 전송 완료
 *   0  : TX 버퍼 부족 – 다음 호출에서 재시도
 *  -1  : 소켓 오류
 */
static int send_partial(uint8_t sn, const uint8_t *buf,
                        uint32_t *offset, uint32_t len)
{
    uint16_t free_sz;
    uint32_t remain;
    uint16_t chunk;
    int32_t sent;

    if (!buf || !offset)
        return -1;

    if (*offset >= len)
        return 1;

    free_sz = getSn_TX_FSR(sn);
    if (free_sz == 0)
        return 0;

    remain = len - *offset;
    chunk = (remain > (uint32_t)free_sz) ? free_sz : (uint16_t)remain;

    sent = send(sn, (uint8_t *)buf + *offset, chunk);
    if (sent <= 0)
        return -1;

    *offset += (uint32_t)sent;
    return (*offset >= len) ? 1 : 0;
}

/* ══════════════════════════════════════════════════════
 * RX 처리 – 요청 수신 및 파싱
 * ══════════════════════════════════════════════════════ */
static void process_rx(int sock)
{
    int idx = sock_to_idx(sock);
    tx_ctx_t *ctx;
    int rsize, to_read, n;

    if (idx < 0)
        return;

    ctx = &tx_ctx[idx];

    if (ctx->state != TX_IDLE)
        return;

    while (1)
    {
        rsize = getSn_RX_RSR((uint8_t)sock);
        if (rsize <= 0)
            break;

        to_read = (rsize > HTTP_IO_BUFFER_SIZE) ? HTTP_IO_BUFFER_SIZE : rsize;

        n = recv((uint8_t)sock, ctx->io_buffer, to_read);
        if (n < 0)
        {
            http_server_close_socket(sock);
            return;
        }
        if (n == 0)
            break;

        ctx->req_tick = HAL_GetTick();

        if (http_handle_request(sock, ctx->io_buffer, (uint32_t)n))
            break;

        if (http_core_response_available(sock))
            break;
    }
}

/* ══════════════════════════════════════════════════════
 * TX 처리 – 응답 전송 상태 머신
 * ══════════════════════════════════════════════════════ */
static void process_tx(int sock)
{
    int idx = sock_to_idx(sock);
    tx_ctx_t *ctx;
    int ret;

    if (idx < 0)
        return;

    ctx = &tx_ctx[idx];

    switch (ctx->state)
    {
    /* ── TX_IDLE: 응답 준비 ─────────────────────────── */
    case TX_IDLE:
        if (http_core_get_response(sock, &ctx->resp))
        {
            int hlen = http_response_build_header(
                &ctx->resp, ctx->header, sizeof(ctx->header));

            if (hlen <= 0)
            {
                http_server_close_socket(sock);
                return;
            }

            ctx->header_len = (uint32_t)hlen;
            ctx->header_offset = 0;
            ctx->body_offset = 0;
            ctx->stream_len = 0;
            ctx->stream_offset = 0;
            ctx->tx_tick = HAL_GetTick();
            ctx->state = TX_HEADER;
        }
        break;

    /* ── TX_HEADER: HTTP 헤더 전송 ──────────────────── */
    case TX_HEADER:
        ret = send_partial((uint8_t)sock, (const uint8_t *)ctx->header,
                           &ctx->header_offset, ctx->header_len);
        if (ret < 0)
        {
            http_server_close_socket(sock);
            return;
        }
        if (ret == 0)
            return;

        if ((BootInfo_GetFwUpdatFlag() == BOOTLOADER_BOOT_MODE) ||
            systemResetFlag)
        {
            HAL_Delay(HTTP_RESET_DELAY_MS);
            __disable_irq();
            __DSB();
            __ISB();
            NVIC_SystemReset();
        }

        ctx->tx_tick = HAL_GetTick();
        ctx->state = (ctx->resp.type == HTTP_RESP_BUFFER) ? TX_BODY : TX_STREAM;
        break;

    /* ── TX_BODY: 버퍼 응답 본문 전송 ──────────────── */
    case TX_BODY:
        ret = send_partial((uint8_t)sock,
                           ctx->resp.data.buffer.body,
                           &ctx->body_offset,
                           ctx->resp.data.buffer.body_len);
        if (ret < 0)
        {
            http_server_close_socket(sock);
            return;
        }
        if (ret == 0)
            return;

        /* 본문 전송 완료 */
        if (ctx->resp.keep_alive)
        {
            /*
             * Keep-Alive: 소켓 유지, TX 상태만 초기화.
             * 다음 HTTP 요청(상태폴링 포함)을 동일 연결로 수신.
             * /status 응답은 keep_alive=0 이므로 그 시점에 연결이 닫힌다.
             */
            http_server_abort_response(sock);
            http_core_reset_socket(sock);
            ctx->req_tick = HAL_GetTick();
        }
        else
        {
            http_server_close_socket(sock);
        }
        break;

    /* ── TX_STREAM: 스트림 응답 청크 전송 ──────────── */
    case TX_STREAM:
        if (ctx->stream_offset >= ctx->stream_len)
        {
            int rd;

            if (!ctx->resp.data.stream.read)
            {
                http_server_close_socket(sock);
                return;
            }

            rd = ctx->resp.data.stream.read(
                ctx->resp.data.stream.ctx,
                ctx->io_buffer,
                sizeof(ctx->io_buffer));

            if (rd <= 0)
            {
                http_server_close_socket(sock);
                return;
            }

            ctx->stream_len = (uint32_t)rd;
            ctx->stream_offset = 0;
        }

        ret = send_partial((uint8_t)sock,
                           ctx->io_buffer,
                           &ctx->stream_offset,
                           ctx->stream_len);
        if (ret < 0)
        {
            http_server_close_socket(sock);
            return;
        }
        if (ret == 0)
            return;

        ctx->tx_tick = HAL_GetTick();
        break;

    default:
        http_server_close_socket(sock);
        break;
    }
}

/* ══════════════════════════════════════════════════════
 * http_server_task – 단일 소켓 처리 (내부용)
 * ══════════════════════════════════════════════════════ */
void http_server_task(int sock)
{
    int idx = sock_to_idx(sock);
    tx_ctx_t *ctx;
    uint8_t sr;
    uint32_t now;

    if (idx < 0)
        return;

    ctx = &tx_ctx[idx];
    sr = getSn_SR((uint8_t)sock);
    now = HAL_GetTick();

    switch (sr)
    {
    case SOCK_CLOSED:
        http_server_reset_ctx(sock);
        socket((uint8_t)sock, Sn_MR_TCP, HTTP_PORT, 0);
        break;

    case SOCK_INIT:
        listen((uint8_t)sock);
        break;

    case SOCK_LISTEN:
        ctx->connected = 0;
        break;

    case SOCK_ESTABLISHED:
        if (!ctx->connected)
        {
            ctx->connected = 1;
            ctx->req_tick = now;
            ctx->tx_tick = now;
            BootInfo_SetFwUpdatFlag(BOOTLOADER_APP_MODE);
        }

        if ((ctx->state == TX_IDLE) && !http_core_response_available(sock))
        {
            if ((now - ctx->req_tick) > HTTP_REQ_TIMEOUT_MS)
            {
                http_server_close_socket(sock);
                break;
            }
            process_rx(sock);
        }

        if (ctx->state != TX_IDLE || http_core_response_available(sock))
        {
            if ((now - ctx->tx_tick) > HTTP_TX_PROGRESS_TIMEOUT_MS)
            {
                http_server_close_socket(sock);
                break;
            }
            process_tx(sock);
        }
        break;

    case SOCK_CLOSE_WAIT:
        http_server_close_socket(sock);
        break;

    default:
        break;
    }
}

/* ══════════════════════════════════════════════════════
 * http_server_task_all – 모든 HTTP 소켓 일괄 처리
 *
 * TASK_Network 에서 이 함수 하나만 호출하면 된다.
 * 소켓 0, 7 을 순서대로 처리하므로 단일 소켓 시절과
 * 사용법이 동일하고 HW_Network.c 변경이 최소화된다.
 * ══════════════════════════════════════════════════════ */
void http_server_task_all(void)
{
    http_server_task(0); /* S0: 업로드/다운로드 */
    http_server_task(7); /* S7: 상태폴링/일반 요청 (병렬) */
}
