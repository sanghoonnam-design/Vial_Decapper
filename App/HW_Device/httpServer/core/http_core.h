#ifndef HTTP_CORE_H
#define HTTP_CORE_H

#include <stdint.h>

typedef enum {
    HTTP_RESP_NONE = 0,
    HTTP_RESP_BUFFER,
    HTTP_RESP_STREAM
} http_resp_type_t;

typedef int (*http_stream_read_fn)(void* ctx, uint8_t* buf, uint32_t max_len);
typedef void (*http_stream_close_fn)(void* ctx);

typedef struct
{
    void*                ctx;
    http_stream_read_fn  read;
    http_stream_close_fn close;
    uint32_t             total_len;
} http_stream_t;

typedef struct
{
    int              status;
    const char*      content_type;
    const char*      extra_headers;
    uint8_t          keep_alive;   /* 1 → HTTP/1.1 Connection: keep-alive */
    http_resp_type_t type;

    union {
        struct
        {
            uint8_t* body;
            uint32_t body_len;
        } buffer;

        http_stream_t stream;
    } data;

} __attribute__((packed)) http_response_t;

typedef struct
{
    uint8_t          sock;
    char             method[8];
    char             url[64];
    uint8_t*         body;
    uint32_t         body_len;
    http_response_t* resp;
} __attribute__((packed)) http_request_ctx_t;

typedef void (*http_handler_t)(const http_request_ctx_t* ctx);

void http_response_init(http_response_t* resp);
void http_response_set(http_response_t* resp, int status, const char* content_type, const char* body, uint32_t body_len);
void http_response_set_stream(http_response_t* resp, int status, const char* content_type,
                              void*                stream_ctx,
                              http_stream_read_fn  read_fn,
                              http_stream_close_fn close_fn,
                              uint32_t             total_len);
void http_response_set_extra_headers(http_response_t* resp, const char* extra_headers);
void http_response_set_keep_alive(http_response_t* resp);

int http_response_build_header(const http_response_t* resp, char* buf, uint32_t buf_len);

int  http_handle_request(int sock, uint8_t* data, uint32_t data_len);
int  http_core_response_available(int sock);
int  http_core_get_response(int sock, http_response_t* out);
void http_core_reset_socket(int sock);

#endif
