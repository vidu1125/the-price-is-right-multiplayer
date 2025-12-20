#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "../include/protocol.h"
#include "../include/server.h"
#include <errno.h>

//==============================================================================
// PROTOCOL VALIDATION
//==============================================================================

int validate_header(MessageHeader *header) {
    if (header->magic != MAGIC_NUMBER) {
        printf("Invalid magic number: 0x%04X\n", header->magic);
        return 0;
    }
    
    if (header->length > BUFF_SIZE) {
        printf("Payload too large: %u bytes\n", header->length);
        return 0;
    }
    
    return 1;
}

int validate_command_for_state(uint16_t command, ConnectionState state) {
    if (state == STATE_UNAUTHENTICATED) {
        if (command != CMD_LOGIN_REQ && 
            command != CMD_REGISTER_REQ && 
            command != CMD_HEARTBEAT) {
            return 0;
        }
    }
    
    if (state == STATE_AUTHENTICATED) {
        if (command >= CMD_START_GAME && command <= CMD_BONUS) {
            return 0;
        }
    }
    
    return 1;
}

//==============================================================================
// MESSAGE RECEIVING
//==============================================================================

int receive_header(int client_idx, MessageHeader *header) {
    ClientConnection *client = &clients[client_idx];
    int remaining = HEADER_SIZE - client->recv_offset;
    
    int n = recv(client->sockfd, 
                 client->recv_buffer + client->recv_offset,
                 remaining, 0);
    
    if (n <= 0) {
        return n;
    }
    
    client->recv_offset += n;
    
    if (client->recv_offset >= HEADER_SIZE) {
        memcpy(header, client->recv_buffer, HEADER_SIZE);
        
        header->magic = ntohs(header->magic);
        header->command = ntohs(header->command);
        header->reserved = ntohs(header->reserved);
        header->seq_num = ntohl(header->seq_num);
        header->length = ntohl(header->length);
        
        if (!validate_header(header)) {
            return -1;
        }
        
        client->header_received = 1;
        client->expected_length = header->length;
        client->current_header = *header;
        client->recv_offset = 0;
        
        return 1;
    }
    
    return 0;
}

int receive_payload(int client_idx, char *payload, uint32_t length) {
    ClientConnection *client = &clients[client_idx];
    
    if (length == 0) {
        return 1;  // No payload expected
    }
    
    int remaining = length - client->recv_offset;
    
    int n = recv(client->sockfd,
                 client->recv_buffer + client->recv_offset,
                 remaining, 0);
    
    if (n <= 0) {
        return n;  // Connection closed or error
    }
    
    client->recv_offset += n;
    
    if ((uint32_t)client->recv_offset >= length) {
        memcpy(payload, client->recv_buffer, length);
        client->recv_offset = 0;
        client->header_received = 0;
        return 1;  // Complete
    }
    
    return 0;  // Need more data
}

//==============================================================================
// MESSAGE SENDING
//==============================================================================

int send_response(int sockfd, uint16_t command, const char *payload, uint32_t length) {
    MessageHeader header;
    char buffer[BUFF_SIZE];
    int total_size;
    
    header.magic = htons(MAGIC_NUMBER);
    header.version = PROTOCOL_VERSION;
    header.flags = 0;
    header.command = htons(command);
    header.reserved = 0;
    header.seq_num = 0;
    header.length = htonl(length);
    
    memcpy(buffer, &header, HEADER_SIZE);
    
    if (length > 0 && payload != NULL) {
        memcpy(buffer + HEADER_SIZE, payload, length);
    }
    
    total_size = HEADER_SIZE + length;
    
    int sent = 0;
    while (sent < total_size) {
        int n = send(sockfd, buffer + sent, total_size - sent, MSG_NOSIGNAL);
        if (n < 0) {
            // 🔇 SILENT ERROR - Không spam log nữa
            if (errno == EPIPE || errno == ECONNRESET) {
                // Client đã đóng kết nối - đây là bình thường
                return -1;
            }
            
            // Các lỗi khác thì vẫn log
            perror("Send failed");
            return -1;
        }
        sent += n;
    }
    
    return sent;
}