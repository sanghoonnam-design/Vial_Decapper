#ifndef HTTP_FS_H
#define HTTP_FS_H

#include "../core/http_core.h"

void http_post_fs_list(const http_request_ctx_t *ctx);
void http_post_fs_read(const http_request_ctx_t *ctx);
void http_post_fs_download(const http_request_ctx_t *ctx);
void http_post_fs_write(const http_request_ctx_t *ctx);
void http_post_fs_delete(const http_request_ctx_t *ctx);
void http_post_fs_upload(const http_request_ctx_t *ctx);

#endif
