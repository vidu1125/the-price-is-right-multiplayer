#pragma once
#include <stdint.h>

#include "protocol/protocol.h"

//==============================================================================
// HTTP Response Structure
//==============================================================================

typedef struct {
    int status_code;        // HTTP status: 200, 400, 500, -1 if error
    int body_length;        // Length of body
    const char *body;       // Pointer to body start in response buffer
} HttpResponse;

//==============================================================================
// HTTP Request Functions (Raw)
//==============================================================================

int http_get(
    const char *host,
    int port,
    const char *path,
    char *out,
    size_t max_len
);

int http_post(
    const char *host,
    int port,
    const char *path,
    const char *body,
    char *out,
    size_t max_len
);

int http_put(
    const char *host,
    int port,
    const char *path,
    const char *body,
    char *out,
    size_t max_len
);

int http_delete(
    const char *host,
    int port,
    const char *path,
    char *out,
    size_t max_len
);

//==============================================================================
// HTTP Request Functions (Parsed - Recommended)
//==============================================================================

/**
 * HTTP POST with parsed response
 * @param resp_buf - Buffer to store response (will be modified)
 * @return HttpResponse with status_code and body pointer
 */
HttpResponse http_post_parse(
    const char *host,
    int port,
    const char *path,
    const char *body,
    char *resp_buf,
    size_t resp_buf_size
);

HttpResponse http_put_parse(
    const char *host,
    int port,
    const char *path,
    const char *body,
    char *resp_buf,
    size_t resp_buf_size
);

HttpResponse http_delete_parse(
    const char *host,
    int port,
    const char *path,
    char *resp_buf,
    size_t resp_buf_size
);

HttpResponse http_get_parse(
    const char *host,
    int port,
    const char *path,
    char *resp_buf,
    size_t resp_buf_size
);

//==============================================================================
// Protocol Helper
//==============================================================================

void forward_response(
    int client_fd,
    MessageHeader *req,
    uint16_t cmd,
    const char *payload,
    uint32_t payload_len
);
