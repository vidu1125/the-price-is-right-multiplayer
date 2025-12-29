#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>          // snprintf
#include <netdb.h>          // gethostbyname, struct hostent

#include "utils/http_utils.h"
#include "protocol/protocol.h"


int http_get(
    const char *host,
    int port,
    const char *path,
    char *out,
    size_t max_len
) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct hostent *he = gethostbyname(host);
    if (!he) return -1;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    memcpy(&addr.sin_addr, he->h_addr, he->h_length);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        return -1;

    char req[512];
    snprintf(req, sizeof(req),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n\r\n",
        path, host
    );

    send(sock, req, strlen(req), 0);

    int total = 0;
    int n;
    while ((n = recv(sock, out + total, max_len - total - 1, 0)) > 0) {
        total += n;
        if (total >= (int)max_len - 1)
            break;
    }

    out[total] = '\0';
    close(sock);

    return total;
}

int http_post(
    const char *host,
    int port,
    const char *path,
    const char *body,
    char *out,
    size_t max_len
) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct hostent *he = gethostbyname(host);
    if (!he) {
        close(sock);
        return -1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    memcpy(&addr.sin_addr, he->h_addr, he->h_length);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    size_t body_len = body ? strlen(body) : 0;
    char req[2048];
    snprintf(req, sizeof(req),
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "X-Account-ID: 1\r\n"
        "Connection: close\r\n\r\n"
        "%s",
        path, host, body_len, body ? body : ""
    );

    send(sock, req, strlen(req), 0);

    int total = 0;
    int n;
    while ((n = recv(sock, out + total, max_len - total - 1, 0)) > 0) {
        total += n;
        if (total >= (int)max_len - 1)
            break;
    }

    out[total] = '\0';
    close(sock);

    return total;
}

int http_put(
    const char *host,
    int port,
    const char *path,
    const char *body,
    char *out,
    size_t max_len
) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct hostent *he = gethostbyname(host);
    if (!he) {
        close(sock);
        return -1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    memcpy(&addr.sin_addr, he->h_addr, he->h_length);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    size_t body_len = body ? strlen(body) : 0;
    char req[2048];
    snprintf(req, sizeof(req),
        "PUT %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "X-Account-ID: 1\r\n"
        "Connection: close\r\n\r\n"
        "%s",
        path, host, body_len, body ? body : ""
    );

    send(sock, req, strlen(req), 0);

    int total = 0;
    int n;
    while ((n = recv(sock, out + total, max_len - total - 1, 0)) > 0) {
        total += n;
        if (total >= (int)max_len - 1)
            break;
    }

    out[total] = '\0';
    close(sock);

    return total;
}

int http_delete(
    const char *host,
    int port,
    const char *path,
    char *out,
    size_t max_len
) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct hostent *he = gethostbyname(host);
    if (!he) {
        close(sock);
        return -1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    memcpy(&addr.sin_addr, he->h_addr, he->h_length);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    char req[512];
    snprintf(req, sizeof(req),
        "DELETE %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "X-Account-ID: 1\r\n"
        "Connection: close\r\n\r\n",
        path, host
    );

    send(sock, req, strlen(req), 0);

    int total = 0;
    int n;
    while ((n = recv(sock, out + total, max_len - total - 1, 0)) > 0) {
        total += n;
        if (total >= (int)max_len - 1)
            break;
    }

    out[total] = '\0';
    close(sock);

    return total;
}

//==============================================================================
// HTTP Parsed Response Functions
//==============================================================================

static HttpResponse parse_http_response(char *resp_buf, int resp_len) {
    HttpResponse result = {0};
    result.status_code = -1;
    result.body = NULL;
    result.body_length = 0;
    
    if (resp_len <= 0) {
        return result;
    }
    
    // Parse "HTTP/1.X STATUS_CODE ...\r\n"
    if (sscanf(resp_buf, "HTTP/1.%*d %d", &result.status_code) != 1) {
        return result;
    }
    
    // Find body (after \r\n\r\n)
    char *body_start = strstr(resp_buf, "\r\n\r\n");
    if (body_start) {
        result.body = body_start + 4;
        result.body_length = resp_len - (int)(body_start - resp_buf + 4);
    }
    
    return result;
}

HttpResponse http_post_parse(
    const char *host,
    int port,
    const char *path,
    const char *body,
    char *resp_buf,
    size_t resp_buf_size
) {
    int resp_len = http_post(host, port, path, body, resp_buf, resp_buf_size);
    return parse_http_response(resp_buf, resp_len);
}

HttpResponse http_put_parse(
    const char *host,
    int port,
    const char *path,
    const char *body,
    char *resp_buf,
    size_t resp_buf_size
) {
    int resp_len = http_put(host, port, path, body, resp_buf, resp_buf_size);
    return parse_http_response(resp_buf, resp_len);
}

HttpResponse http_delete_parse(
    const char *host,
    int port,
    const char *path,
    char *resp_buf,
    size_t resp_buf_size
) {
    int resp_len = http_delete(host, port, path, resp_buf, resp_buf_size);
    return parse_http_response(resp_buf, resp_len);
}

HttpResponse http_get_parse(
    const char *host,
    int port,
    const char *path,
    char *resp_buf,
    size_t resp_buf_size
) {
    int resp_len = http_get(host, port, path, resp_buf, resp_buf_size);
    return parse_http_response(resp_buf, resp_len);
}

//==============================================================================
// Protocol Helper
//==============================================================================

void forward_response(
    int client_fd,
    MessageHeader *req,
    uint16_t cmd,
    const char *payload,
    uint32_t payload_len
) {
    MessageHeader resp;
    memset(&resp, 0, sizeof(resp));

    resp.magic   = htons(MAGIC_NUMBER);
    resp.version = PROTOCOL_VERSION;
    resp.command = htons(cmd);
    resp.seq_num = htonl(req->seq_num);
    resp.length  = htonl(payload_len);

    send(client_fd, &resp, sizeof(resp), 0);
    if (payload_len > 0)
        send(client_fd, payload, payload_len, 0);
}
