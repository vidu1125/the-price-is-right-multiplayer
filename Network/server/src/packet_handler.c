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

bool recv_packet_header(int client_fd, PacketHeader* header) {
    if (!recv_full(client_fd, header, sizeof(PacketHeader))) {
        return false;
    }
    
    // Convert from network byte order
    header->magic = ntohl(header->magic);
    header->sequence = ntohl(header->sequence);
    header->payload_length = ntohl(header->payload_length);
    header->flags = ntohs(header->flags);
    
    // Verify magic number
    if (header->magic != MAGIC_NUMBER) {
        fprintf(stderr, "[Packet] Invalid magic: 0x%08x\n", header->magic);
        return false;
    }
    
    return true;
}

bool recv_packet_payload(int client_fd, uint32_t length, void** payload_out) {
    if (length == 0) {
        *payload_out = NULL;
        return true;
    }
    
    if (length > MAX_PACKET_SIZE) {
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
    PacketHeader header;
    
    // Build header
    header.magic = htonl(MAGIC_NUMBER);
    header.version = PROTOCOL_VERSION;
    header.type = response_code & 0xFF;
    header.flags = htons(0);
    header.sequence = htonl(0);  // TODO: track sequence
    header.payload_length = htonl(payload_size);
    
    // Send header
    ssize_t sent = send(client_fd, &header, sizeof(header), MSG_NOSIGNAL);
    if (sent != sizeof(header)) {
        perror("[Packet] send header");
        return false;
    }
    
    // Send payload if exists
    if (payload_size > 0 && payload) {
        sent = send(client_fd, payload, payload_size, MSG_NOSIGNAL);
        if (sent != (ssize_t)payload_size) {
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

// Helper functions
uint16_t packet_get_command(PacketHeader* header) {
    return header->type;
}

uint32_t packet_get_length(PacketHeader* header) {
    return header->payload_length;
}
