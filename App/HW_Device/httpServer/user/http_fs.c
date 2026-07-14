#include "http_fs.h"
#include "ff.h"
#include "ISD.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define FS_LIST_BUF_SIZE  2048
#define FS_FULL_PATH_MAX  72   /* "0:/" + 64 chars + null */

/* ---------------------------------------------------------------
 * Stream context for file read responses.
 * Static is safe because HTTP_MAX_SOCKETS = 1 (single connection).
 * --------------------------------------------------------------- */
typedef struct {
    FIL     file;
    uint8_t is_open;
} fs_read_ctx_t;

static fs_read_ctx_t s_read_ctx;
static char          s_list_buf[FS_LIST_BUF_SIZE];
static char          s_content_disp[200]; /* Content-Disposition header line */

/* ---------------------------------------------------------------
 * Parse "path=<dir>" for directory listing.
 * Accepts empty body or "path=/" as root "0:/".
 * Rejects paths containing "..".
 * Always succeeds (falls back to root on invalid input).
 * --------------------------------------------------------------- */
static void parse_dir_path(const char *body, uint32_t body_len,
                            char *out, uint32_t out_size)
{
    const char *p;
    uint32_t    plen;

    if (!body || body_len < 6 || strncmp(body, "path=", 5) != 0)
        goto root;

    p    = body + 5;
    plen = 0;

    while (p[plen] && p[plen] != '\r' && p[plen] != '\n' && plen < out_size - 4)
        plen++;

    /* Strip leading '/' */
    while (plen > 0 && p[0] == '/') { p++; plen--; }
    /* Strip trailing '/' */
    while (plen > 0 && p[plen - 1] == '/') plen--;

    if (plen == 0 || strstr(p, "..") != NULL)
        goto root;

    out[0] = '0';
    out[1] = ':';
    out[2] = '/';
    memcpy(out + 3, p, plen);
    out[3 + plen] = '\0';
    return;

root:
    strncpy(out, "0:/", out_size - 1);
    out[out_size - 1] = '\0';
}

/* ---------------------------------------------------------------
 * Parse "path=<name>" from request body.
 * Prepends "0:/" and rejects paths containing "..".
 * Returns 1 on success, 0 on failure.
 * --------------------------------------------------------------- */
static int parse_path(const char *body, uint32_t body_len, char *out, uint32_t out_size)
{
    const char *p;
    uint32_t    plen;

    if (!body || body_len < 6)
        return 0;

    if (strncmp(body, "path=", 5) != 0)
        return 0;

    p    = body + 5;
    plen = 0;

    while (p[plen] && p[plen] != '\r' && p[plen] != '\n' && plen < out_size - 4)
        plen++;

    if (plen == 0)
        return 0;

    if (strstr(p, "..") != NULL)
        return 0;

    out[0] = '0';
    out[1] = ':';
    out[2] = '/';
    memcpy(out + 3, p, plen);
    out[3 + plen] = '\0';

    return 1;
}

/* ---------------------------------------------------------------
 * Stream callbacks
 * --------------------------------------------------------------- */
static int fs_stream_read(void *ctx, uint8_t *buf, uint32_t max_len)
{
    fs_read_ctx_t *fctx = (fs_read_ctx_t *)ctx;
    UINT           br   = 0;

    if (!fctx->is_open)
        return 0;

    if (f_read(&fctx->file, buf, max_len, &br) != FR_OK)
        return -1;

    return (int)br;
}

static void fs_stream_close(void *ctx)
{
    fs_read_ctx_t *fctx = (fs_read_ctx_t *)ctx;

    if (fctx->is_open) {
        f_close(&fctx->file);
        fctx->is_open = 0;
    }
}

/* ---------------------------------------------------------------
 * POST /fs/list
 * Request body (optional): path=<directory>   (default: root)
 * Response (text/plain): one line per entry
 *   <name>,<size>,F    (file)
 *   <name>/,0,D        (directory)
 * --------------------------------------------------------------- */
void http_post_fs_list(const http_request_ctx_t *ctx)
{
    DIR     dir;
    FILINFO fno;
    int     len = 0;
    int     remaining;
    char    dir_path[FS_FULL_PATH_MAX];

    if (BSP_SD_IsDetected() == SD_NOT_PRESENT) {
        http_response_set(ctx->resp, 503, "text/plain", "SD card not present", 19);
        return;
    }

    parse_dir_path((const char *)ctx->body, ctx->body_len,
                   dir_path, sizeof(dir_path));

    if (f_opendir(&dir, dir_path) != FR_OK) {
        http_response_set(ctx->resp, 500, "text/plain", "Cannot open directory", 21);
        return;
    }

    memset(s_list_buf, 0, sizeof(s_list_buf));

    while (1) {
        if (f_readdir(&dir, &fno) != FR_OK || fno.fname[0] == '\0')
            break;

        remaining = (int)sizeof(s_list_buf) - len - 1;
        if (remaining <= 0)
            break;

        if (fno.fattrib & AM_DIR) {
            len += snprintf(s_list_buf + len, (size_t)remaining,
                            "%s/,0,D\n", fno.fname);
        } else {
            len += snprintf(s_list_buf + len, (size_t)remaining,
                            "%s,%lu,F\n", fno.fname, (unsigned long)fno.fsize);
        }
    }

    f_closedir(&dir);

    http_response_set(ctx->resp, 200, "text/plain", s_list_buf, (uint32_t)len);
}

/* ---------------------------------------------------------------
 * POST /fs/read
 * Request body: path=<filename>
 * Response:     file content (streamed)
 * --------------------------------------------------------------- */
void http_post_fs_read(const http_request_ctx_t *ctx)
{
    char    path[FS_FULL_PATH_MAX];
    FILINFO fno;

    if (!ctx->body || ctx->body_len == 0) {
        http_response_set(ctx->resp, 400, "text/plain", "Missing path", 12);
        return;
    }

    if (BSP_SD_IsDetected() == SD_NOT_PRESENT) {
        http_response_set(ctx->resp, 503, "text/plain", "SD card not present", 19);
        return;
    }

    if (!parse_path((const char *)ctx->body, ctx->body_len, path, sizeof(path))) {
        http_response_set(ctx->resp, 400, "text/plain", "Invalid path", 12);
        return;
    }

    if (f_stat(path, &fno) != FR_OK) {
        http_response_set(ctx->resp, 404, "text/plain", "File not found", 14);
        return;
    }

    if (s_read_ctx.is_open)
        fs_stream_close(&s_read_ctx);

    if (f_open(&s_read_ctx.file, path, FA_READ) != FR_OK) {
        http_response_set(ctx->resp, 500, "text/plain", "Cannot open file", 16);
        return;
    }

    s_read_ctx.is_open = 1;

    http_response_set_stream(ctx->resp, 200, "text/plain",
                             &s_read_ctx,
                             fs_stream_read,
                             fs_stream_close,
                             (uint32_t)fno.fsize);
}

/* ---------------------------------------------------------------
 * POST /fs/download
 * Request body: path=<filename>
 * Response:     file content (streamed, application/octet-stream)
 *               Content-Disposition: attachment  →  browser saves file
 * --------------------------------------------------------------- */
void http_post_fs_download(const http_request_ctx_t *ctx)
{
    char        path[FS_FULL_PATH_MAX];
    const char *fname;
    FILINFO     fno;

    if (!ctx->body || ctx->body_len == 0) {
        http_response_set(ctx->resp, 400, "text/plain", "Missing path", 12);
        return;
    }

    if (BSP_SD_IsDetected() == SD_NOT_PRESENT) {
        http_response_set(ctx->resp, 503, "text/plain", "SD card not present", 19);
        return;
    }

    if (!parse_path((const char *)ctx->body, ctx->body_len, path, sizeof(path))) {
        http_response_set(ctx->resp, 400, "text/plain", "Invalid path", 12);
        return;
    }

    if (f_stat(path, &fno) != FR_OK) {
        http_response_set(ctx->resp, 404, "text/plain", "File not found", 14);
        return;
    }

    if (s_read_ctx.is_open)
        fs_stream_close(&s_read_ctx);

    if (f_open(&s_read_ctx.file, path, FA_READ) != FR_OK) {
        http_response_set(ctx->resp, 500, "text/plain", "Cannot open file", 16);
        return;
    }

    s_read_ctx.is_open = 1;

    /* Extract bare filename (skip "0:/dir/" prefix) */
    fname = strrchr(path, '/');
    fname = fname ? fname + 1 : path;

    snprintf(s_content_disp, sizeof(s_content_disp),
             "Content-Disposition: attachment; filename=\"%s\"\r\n", fname);

    http_response_set_stream(ctx->resp, 200, "application/octet-stream",
                             &s_read_ctx,
                             fs_stream_read,
                             fs_stream_close,
                             (uint32_t)fno.fsize);
    http_response_set_extra_headers(ctx->resp, s_content_disp);
}

/* ---------------------------------------------------------------
 * POST /fs/write
 * Request body: path=<filename>\n<content>
 * Response:     "OK" or error
 * Max content limited by HTTP_CORE_MAX_BODY_SIZE (4096 bytes).
 * --------------------------------------------------------------- */
void http_post_fs_write(const http_request_ctx_t *ctx)
{
    char        path[FS_FULL_PATH_MAX];
    const char *body_str;
    const char *content_start;
    uint32_t    path_header_len;
    uint32_t    content_len;
    FIL         file;
    UINT        bw;

    if (!ctx->body || ctx->body_len == 0) {
        http_response_set(ctx->resp, 400, "text/plain", "Missing body", 12);
        return;
    }

    if (BSP_SD_IsDetected() == SD_NOT_PRESENT) {
        http_response_set(ctx->resp, 503, "text/plain", "SD card not present", 19);
        return;
    }

    body_str = (const char *)ctx->body;

    if (!parse_path(body_str, ctx->body_len, path, sizeof(path))) {
        http_response_set(ctx->resp, 400, "text/plain", "Invalid path", 12);
        return;
    }

    /* Find the newline after "path=<name>" to locate content start */
    content_start = strchr(body_str, '\n');
    if (!content_start) {
        content_start = body_str + ctx->body_len;
        content_len   = 0;
    } else {
        content_start++;
        path_header_len = (uint32_t)(content_start - body_str);
        content_len     = (ctx->body_len > path_header_len)
                          ? ctx->body_len - path_header_len
                          : 0;
    }

    if (f_open(&file, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
        http_response_set(ctx->resp, 500, "text/plain", "Cannot create file", 18);
        return;
    }

    if (content_len > 0) {
        if (f_write(&file, content_start, content_len, &bw) != FR_OK) {
            f_close(&file);
            http_response_set(ctx->resp, 500, "text/plain", "Write failed", 12);
            return;
        }
    }

    f_close(&file);
    http_response_set(ctx->resp, 200, "text/plain", "OK", 2);
}

/* ---------------------------------------------------------------
 * POST /fs/upload
 * Body: path=<name>\noffset=<N>\n<binary data>
 * Response: "OK" or error
 * --------------------------------------------------------------- */
void http_post_fs_upload(const http_request_ctx_t *ctx)
{
    char        path[FS_FULL_PATH_MAX];
    const char *bs;
    const char *p;
    const char *nl;
    const char *data_start;
    uint32_t    path_len;
    uint32_t    offset   = 0;
    uint32_t    data_len = 0;
    uint32_t    i;
    FIL         file;
    UINT        bw;
    uint8_t     mode;

    if (!ctx->body || ctx->body_len < 6) {
        http_response_set(ctx->resp, 400, "text/plain", "Missing body", 12);
        return;
    }

    if (BSP_SD_IsDetected() == SD_NOT_PRESENT) {
        http_response_set(ctx->resp, 503, "text/plain", "SD card not present", 19);
        return;
    }

    bs = (const char *)ctx->body;

    /* Parse "path=<name>\n" */
    if (strncmp(bs, "path=", 5) != 0) {
        http_response_set(ctx->resp, 400, "text/plain", "Missing path", 12);
        return;
    }

    p  = bs + 5;
    nl = (const char *)memchr(p, '\n', ctx->body_len - 5u);
    if (!nl) {
        http_response_set(ctx->resp, 400, "text/plain", "Bad format", 10);
        return;
    }

    path_len = (uint32_t)(nl - p);
    if (path_len > 0 && p[path_len - 1] == '\r')
        path_len--;

    if (path_len == 0 || path_len > (uint32_t)(FS_FULL_PATH_MAX - 4)) {
        http_response_set(ctx->resp, 400, "text/plain", "Invalid path", 12);
        return;
    }

    for (i = 0; i + 1u < path_len; i++) {
        if (p[i] == '.' && p[i + 1u] == '.') {
            http_response_set(ctx->resp, 400, "text/plain", "Invalid path", 12);
            return;
        }
    }

    path[0] = '0'; path[1] = ':'; path[2] = '/';
    memcpy(path + 3, p, path_len);
    path[3 + path_len] = '\0';

    p = nl + 1;

    /* Parse "offset=<N>\n" */
    if ((uint32_t)(p - bs) < ctx->body_len && strncmp(p, "offset=", 7) == 0) {
        offset = (uint32_t)atoi(p + 7);
        nl = (const char *)memchr(p, '\n', ctx->body_len - (uint32_t)(p - bs));
        if (!nl) {
            http_response_set(ctx->resp, 400, "text/plain", "Bad format", 10);
            return;
        }
        p = nl + 1;
    }

    data_start = p;
    data_len   = ctx->body_len - (uint32_t)(data_start - bs);

    mode = (offset == 0u) ? (FA_WRITE | FA_CREATE_ALWAYS) : (FA_WRITE | FA_OPEN_EXISTING);
    if (f_open(&file, path, mode) != FR_OK) {
        http_response_set(ctx->resp, 500, "text/plain", "Cannot open file", 16);
        return;
    }

    if (offset > 0u && f_lseek(&file, (FSIZE_t)offset) != FR_OK) {
        f_close(&file);
        http_response_set(ctx->resp, 500, "text/plain", "Seek failed", 11);
        return;
    }

    if (data_len > 0u) {
        if (f_write(&file, data_start, data_len, &bw) != FR_OK || bw != data_len) {
            f_close(&file);
            http_response_set(ctx->resp, 500, "text/plain", "Write failed", 12);
            return;
        }
    }

    f_close(&file);
    http_response_set(ctx->resp, 200, "text/plain", "OK", 2);
    http_response_set_keep_alive(ctx->resp);
}

/* ---------------------------------------------------------------
 * POST /fs/delete
 * Request body: path=<filename>
 * Response:     "OK" or error
 * --------------------------------------------------------------- */
void http_post_fs_delete(const http_request_ctx_t *ctx)
{
    char path[FS_FULL_PATH_MAX];

    if (!ctx->body || ctx->body_len == 0) {
        http_response_set(ctx->resp, 400, "text/plain", "Missing path", 12);
        return;
    }

    if (BSP_SD_IsDetected() == SD_NOT_PRESENT) {
        http_response_set(ctx->resp, 503, "text/plain", "SD card not present", 19);
        return;
    }

    if (!parse_path((const char *)ctx->body, ctx->body_len, path, sizeof(path))) {
        http_response_set(ctx->resp, 400, "text/plain", "Invalid path", 12);
        return;
    }

    if (f_unlink(path) != FR_OK) {
        http_response_set(ctx->resp, 500, "text/plain", "Delete failed", 13);
        return;
    }

    http_response_set(ctx->resp, 200, "text/plain", "OK", 2);
}
