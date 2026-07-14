#ifndef HTTP_HANDLERS_H
#define HTTP_HANDLERS_H

#include "../core/http_core.h"
#include "http_fs.h"

/* GET */
void http_get_root(const http_request_ctx_t* ctx);
void http_get_css(const http_request_ctx_t* ctx);
void http_get_js(const http_request_ctx_t* ctx);
void http_get_status(const http_request_ctx_t* ctx);
void http_get_getNetwork(const http_request_ctx_t* ctx);

/* POST */
void http_post_gotoboot(const http_request_ctx_t* ctx);
void http_post_appconfirm(const http_request_ctx_t* ctx);
void http_post_settime(const http_request_ctx_t* ctx);
void http_post_factorySet(const http_request_ctx_t* ctx);
void http_post_systemReset(const http_request_ctx_t* ctx);
void http_post_setNetwork(const http_request_ctx_t* ctx);

/*
 * method,        url,               handler
 */
/*
 * HTTP 라우트 테이블
 *
 * 형식: X("METHOD", "/url", handler_function)
 *
 * 새 엔드포인트 추가 방법:
 *   1. 아래 //add new func 위치에 X() 한 줄 추가
 *   2. http_get.c 또는 http_post.c 에 핸들러 구현
 *   3. 이 파일 상단에 핸들러 선언 추가
 */
#define HTTP_ROUTE_TABLE                                         \
    /* ── 정적 파일 ─────────────────────────────────────── */  \
    X("GET",  "/",           http_get_root)                      \
    X("GET",  "/main.css",   http_get_css)                       \
    X("GET",  "/app.js",     http_get_js)                        \
    /* ── 시스템 상태 / 설정 ────────────────────────────── */  \
    X("GET",  "/status",     http_get_status)                    \
    X("GET",  "/getNetwork", http_get_getNetwork)                \
    X("POST", "/gotoboot",   http_post_gotoboot)                 \
    X("POST", "/appconfirm", http_post_appconfirm)               \
    X("POST", "/settime",    http_post_settime)                  \
    X("POST", "/factorySet", http_post_factorySet)               \
    X("POST", "/systemReset",http_post_systemReset)              \
    X("POST", "/setNetwork", http_post_setNetwork)               \
    /* ── SD 카드 파일 시스템 ───────────────────────────── */  \
    X("POST", "/fs/list",    http_post_fs_list)                  \
    X("POST", "/fs/read",    http_post_fs_read)                  \
    X("POST", "/fs/download",http_post_fs_download)              \
    X("POST", "/fs/write",   http_post_fs_write)                 \
    X("POST", "/fs/delete",  http_post_fs_delete)               \
    X("POST", "/fs/upload",  http_post_fs_upload)               \
    /* ── //add new func: 새 엔드포인트를 여기에 추가 ───── */

#endif
