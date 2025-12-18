// Network/server/src/packet_handler.c
#include "packet_handler.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

bool recv_full(int fd, void* buffer, size_t length) {
    size_t total_recv = 0;
    char* buf = (char*)buffer;
    
    while (total_recv < length) {
        ssize_t n = recv(fd, buf + total_recv, length - total_recv, 0);
        if (n <= 0) {
            return false;  // Connection closed or error
        }
        total_recv += n;
    }
    
    return true;
}

bool recv_packet_header(int client_fd, MessageHeader* header) {
    if (!recv_full(client_fd, header, sizeof(MessageHeader))) {
        return false;
    }
    
    // Verify magic number
    if (packet_get_magic(header) != PROTOCOL_MAGIC) {
        fprintf(stderr, "[Packet] Invalid magic: 0x%04x\n", packet_get_magic(header));
        return false;
    }
    
    return true;
}

bool recv_packet_payload(int client_fd, uint32_t length, void** payload_out) {
    if (length == 0) {
        *payload_out = NULL;
        return true;
    }
    
    if (length > MAX_PAYLOAD_SIZE) {
        fprintf(stderr, "[Packet] Payload too large: %u bytes\n", length);
        return false;
    }
    
    void* payload = malloc(length);
    if (!payload) {
        perror("[Packet] malloc");
        return false;
    }
    
    if (!recv_full(client_fd, payload, length)) {
        free(payload);
        return false;
    }
    
    *payload_out = payload;
    return true;
}

bool send_response(int client_fd, uint16_t response_code, void* payload, uint32_t payload_size) {
    MessageHeader header;
    packet_set_header(&header, response_code, payload_size);
    
    // Send header
    if (send(client_fd, &header, sizeof(header), MSG_NOSIGNAL) != sizeof(header)) {
        perror("[Packet] send header");
        return false;
    }
    
    // Send payload if exists
    if (payload_size > 0 && payload) {
        if (send(client_fd, payload, payload_size, MSG_NOSIGNAL) != payload_size) {
            perror("[Packet] send payload");
            return false;
        }
    }
    
    return true;
}

bool send_error(int client_fd, uint16_t error_code, const char* message) {
    ErrorResponse err;
    memset(&err, 0, sizeof(err));
    strncpy(err.message, message, sizeof(err.message) - 1);
    
    return send_response(client_fd, error_code, &err, sizeof(err));
}

bool send_notification(int client_fd, uint16_t notification_code, void* payload, uint32_t payload_size) {
    return send_response(client_fd, notification_code, payload, payload_size);
}
