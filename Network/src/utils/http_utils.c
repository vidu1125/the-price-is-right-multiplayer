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
