#pragma once
#include <stdint.h>

#include "protocol/protocol.h"


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

void forward_response(
    int client_fd,
    MessageHeader *req,
    uint16_t cmd,
    const char *payload,
    uint32_t payload_len
);
