// Network/server/src/backend_bridge.c
#include "backend_bridge.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define SOCKET_PATH "/tmp/tpir_backend.sock"
#define MAX_RESPONSE_SIZE 65536

static int backend_fd = -1;

bool backend_bridge_init(const char* socket_path) {
    struct sockaddr_un addr;
    
    if (backend_fd >= 0) {
        return true;  // Already connected
    }
    
    backend_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (backend_fd < 0) {
        perror("[Bridge] socket");
        return false;
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    
    const char* path = socket_path ? socket_path : SOCKET_PATH;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    
    if (connect(backend_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[Bridge] connect to backend");
        close(backend_fd);
        backend_fd = -1;
        return false;
    }
    
    printf("[Bridge] ✅ Connected to backend IPC at %s\n", path);
    return true;
}

char* backend_bridge_send(const char* json_request) {
    if (backend_fd < 0) {
        if (!backend_bridge_init(NULL)) {
            return NULL;
        }
    }
    
    uint32_t length = strlen(json_request);
    uint32_t network_length = htonl(length);
    
    // Send length (4 bytes, big-endian)
    if (send(backend_fd, &network_length, 4, 0) != 4) {
        perror("[Bridge] send length");
        close(backend_fd);
        backend_fd = -1;
        return NULL;
    }
    
    // Send JSON data
    int total_sent = 0;
    while (total_sent < length) {
        int n = send(backend_fd, json_request + total_sent, length - total_sent, 0);
        if (n <= 0) {
            perror("[Bridge] send data");
            close(backend_fd);
            backend_fd = -1;
            return NULL;
        }
        total_sent += n;
    }
    
    // Receive response length
    uint32_t response_length;
    if (recv(backend_fd, &response_length, 4, 0) != 4) {
        perror("[Bridge] recv length");
        close(backend_fd);
        backend_fd = -1;
        return NULL;
    }
    response_length = ntohl(response_length);
    
    if (response_length > MAX_RESPONSE_SIZE) {
        fprintf(stderr, "[Bridge] Response too large: %u bytes\n", response_length);
        return NULL;
    }
    
    // Receive response data
    char* response = (char*)malloc(response_length + 1);
    if (!response) {
        perror("[Bridge] malloc");
        return NULL;
    }
    
    int total_recv = 0;
    while (total_recv < response_length) {
        int n = recv(backend_fd, response + total_recv, response_length - total_recv, 0);
        if (n <= 0) {
            perror("[Bridge] recv data");
            free(response);
            close(backend_fd);
            backend_fd = -1;
            return NULL;
        }
        total_recv += n;
    }
    response[response_length] = '\0';
    
    return response;
}

char* backend_bridge_build_request(const char* action, uint32_t user_id, const char* data_json) {
    // Build JSON: {"action": "...", "user_id": ..., "data": {...}}
    size_t size = 256 + (data_json ? strlen(data_json) : 0);
    char* json = (char*)malloc(size);
    if (!json) return NULL;
    
    if (data_json && strlen(data_json) > 0) {
        snprintf(json, size, "{\"action\":\"%s\",\"user_id\":%u,\"data\":%s}",
                action, user_id, data_json);
    } else {
        snprintf(json, size, "{\"action\":\"%s\",\"user_id\":%u,\"data\":{}}",
                action, user_id);
    }
    
    return json;
}

void backend_bridge_cleanup() {
    if (backend_fd >= 0) {
        close(backend_fd);
        backend_fd = -1;
        printf("[Bridge] Disconnected from backend\n");
    }
}
