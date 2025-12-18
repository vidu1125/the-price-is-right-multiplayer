/*
 * C Terminal Client - Test Host Features via RAW TCP SOCKET
 * 
 * Architecture:
 *   C Client → RAW TCP SOCKET → C Server (port 8888)
 *              (Binary Protocol)
 * 
 * This client demonstrates pure C socket programming
 * following APPLICATION DESIGN requirements.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

// Protocol constants
#define MAGIC_NUMBER 0x4347
#define PROTOCOL_VERSION 0x01
#define DEFAULT_FLAGS 0x00

// Command codes
#define CMD_CREATE_ROOM 0x0200
#define CMD_SET_RULE    0x0206
#define CMD_KICK        0x0204
#define CMD_DELETE_ROOM 0x0207
#define CMD_START_GAME  0x0300

// Response codes
#define RES_SUCCESS      0x00CA
#define RES_ERROR        0x00CB
#define RES_ROOM_CREATED 0x00CC
#define RES_GAME_STARTING 0x00CD

// Server config
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define BUFFER_SIZE 4096

// Packet header structure (16 bytes)
typedef struct __attribute__((packed)) {
    uint16_t magic;           // 0x4347
    uint16_t version;         // 0x01
    uint8_t  flags;           // 0x00
    uint8_t  command_high;    // High byte of command
    uint16_t command;         // Full command code
    uint16_t reserved;        // Reserved
    uint32_t sequence;        // Sequence number
    uint32_t payload_length;  // Payload size
} PacketHeader;

// CreateRoomRequest payload (72 bytes)
typedef struct __attribute__((packed)) {
    char     room_name[64];
    uint8_t  visibility;      // 0: public, 1: private
    uint8_t  mode;            // 0: scoring, 1: elimination
    uint8_t  max_players;     // 4-6
    uint16_t round_time;      // 15-60 seconds
    uint8_t  wager_enabled;
    uint8_t  padding[2];      // Alignment
} CreateRoomRequest;

// CreateRoomResponse payload
typedef struct __attribute__((packed)) {
    uint32_t room_id;
    char     room_code[11];
    uint32_t host_id;
    uint64_t created_at;
} CreateRoomResponse;

// Global sequence number
static uint32_t g_sequence = 0;

// Function prototypes
int connect_to_server();
void build_header(PacketHeader* header, uint16_t command, uint32_t payload_len);
int send_packet(int sock_fd, uint16_t command, void* payload, size_t payload_size);
int recv_response(int sock_fd, PacketHeader* header, void* payload, size_t max_payload);
void print_menu();
void handle_create_room(int sock_fd);
void handle_set_rule(int sock_fd);
void handle_kick_member(int sock_fd);
void handle_delete_room(int sock_fd);
void handle_start_game(int sock_fd);

int main() {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║    C CLIENT - HOST FEATURES TEST      ║\n");
    printf("║    Raw TCP Socket Communication       ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    while (1) {
        print_menu();
        
        int choice;
        printf("Enter choice: ");
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n'); // Clear input buffer
            printf("❌ Invalid input\n\n");
            continue;
        }
        getchar(); // Consume newline

        if (choice == 0) {
            printf("\n👋 Goodbye!\n");
            break;
        }

        // Connect to server for each request
        int sock_fd = connect_to_server();
        if (sock_fd < 0) {
            printf("❌ Failed to connect to server\n\n");
            continue;
        }

        switch (choice) {
            case 1:
                handle_create_room(sock_fd);
                break;
            case 2:
                handle_set_rule(sock_fd);
                break;
            case 3:
                handle_kick_member(sock_fd);
                break;
            case 4:
                handle_delete_room(sock_fd);
                break;
            case 5:
                handle_start_game(sock_fd);
                break;
            default:
                printf("❌ Invalid choice\n");
        }

        close(sock_fd);
        printf("\n");
    }

    return 0;
}

int connect_to_server() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sock_fd);
        return -1;
    }

    printf("🔌 Connecting to %s:%d...\n", SERVER_IP, SERVER_PORT);
    
    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock_fd);
        return -1;
    }

    printf("✅ Connected!\n\n");
    return sock_fd;
}

void build_header(PacketHeader* header, uint16_t command, uint32_t payload_len) {
    header->magic = htons(MAGIC_NUMBER);
    header->version = htons(PROTOCOL_VERSION);
    header->flags = DEFAULT_FLAGS;
    header->command_high = (command >> 8) & 0xFF;
    header->command = htons(command);
    header->reserved = 0;
    header->sequence = htonl(++g_sequence);
    header->payload_length = htonl(payload_len);
}

int send_packet(int sock_fd, uint16_t command, void* payload, size_t payload_size) {
    PacketHeader header;
    build_header(&header, command, payload_size);

    // Send header
    if (send(sock_fd, &header, sizeof(header), 0) != sizeof(header)) {
        perror("Failed to send header");
        return -1;
    }

    // Send payload
    if (payload_size > 0 && payload != NULL) {
        if (send(sock_fd, payload, payload_size, 0) != (ssize_t)payload_size) {
            perror("Failed to send payload");
            return -1;
        }
    }

    printf("📤 Sent command 0x%04X, payload: %zu bytes\n", command, payload_size);
    return 0;
}

int recv_response(int sock_fd, PacketHeader* header, void* payload, size_t max_payload) {
    // Receive header
    ssize_t received = recv(sock_fd, header, sizeof(PacketHeader), MSG_WAITALL);
    if (received != sizeof(PacketHeader)) {
        printf("❌ Failed to receive header (got %zd bytes)\n", received);
        return -1;
    }

    // Convert from network byte order
    header->magic = ntohs(header->magic);
    header->version = ntohs(header->version);
    header->command = ntohs(header->command);
    header->sequence = ntohl(header->sequence);
    header->payload_length = ntohl(header->payload_length);

    if (header->magic != MAGIC_NUMBER) {
        printf("❌ Invalid magic number: 0x%04X\n", header->magic);
        return -1;
    }

    printf("📥 Response received: command=0x%04X, payload=%u bytes\n", 
           header->command, header->payload_length);

    // Receive payload
    if (header->payload_length > 0 && payload != NULL && max_payload > 0) {
        size_t to_recv = (header->payload_length < max_payload) ? 
                         header->payload_length : max_payload;
        
        received = recv(sock_fd, payload, to_recv, MSG_WAITALL);
        if (received != (ssize_t)to_recv) {
            printf("❌ Failed to receive payload\n");
            return -1;
        }
    }

    return 0;
}

void print_menu() {
    printf("╔════════════════════════════════════════╗\n");
    printf("║           HOST FEATURES MENU           ║\n");
    printf("╠════════════════════════════════════════╣\n");
    printf("║  1. Create Room                        ║\n");
    printf("║  2. Set Rules                          ║\n");
    printf("║  3. Kick Member                        ║\n");
    printf("║  4. Delete Room                        ║\n");
    printf("║  5. Start Game                         ║\n");
    printf("║  0. Exit                               ║\n");
    printf("╚════════════════════════════════════════╝\n");
}

void handle_create_room(int sock_fd) {
    printf("=== CREATE ROOM ===\n");
    
    CreateRoomRequest req;
    memset(&req, 0, sizeof(req));

    printf("Room Name: ");
    fgets(req.room_name, sizeof(req.room_name), stdin);
    req.room_name[strcspn(req.room_name, "\n")] = 0; // Remove newline

    printf("Visibility (0=public, 1=private): ");
    scanf("%hhu", &req.visibility);

    printf("Mode (0=scoring, 1=elimination): ");
    scanf("%hhu", &req.mode);

    printf("Max Players (4-6): ");
    scanf("%hhu", &req.max_players);

    printf("Round Time (15-60 seconds): ");
    scanf("%hu", &req.round_time);
    req.round_time = htons(req.round_time); // Convert to network byte order

    printf("Enable Wager (0=no, 1=yes): ");
    scanf("%hhu", &req.wager_enabled);
    getchar(); // Consume newline

    // Send request
    if (send_packet(sock_fd, CMD_CREATE_ROOM, &req, sizeof(req)) < 0) {
        return;
    }

    // Receive response
    PacketHeader resp_header;
    CreateRoomResponse resp;
    
    if (recv_response(sock_fd, &resp_header, &resp, sizeof(resp)) < 0) {
        return;
    }

    if (resp_header.command == RES_ROOM_CREATED || resp_header.command == RES_SUCCESS) {
        resp.room_id = ntohl(resp.room_id);
        resp.host_id = ntohl(resp.host_id);
        resp.created_at = be64toh(resp.created_at);

        printf("\n✅ Room created successfully!\n");
        printf("   Room ID: %u\n", resp.room_id);
        printf("   Room Code: %s\n", resp.room_code);
        printf("   Host ID: %u\n", resp.host_id);
    } else {
        printf("❌ Failed to create room (response code: 0x%04X)\n", resp_header.command);
    }
}

void handle_set_rule(int sock_fd) {
    printf("=== SET RULES ===\n");
    
    typedef struct __attribute__((packed)) {
        uint32_t room_id;
        uint8_t  mode;
        uint8_t  max_players;
        uint16_t round_time;
        uint8_t  wager_enabled;
        uint8_t  padding[3];
    } SetRuleRequest;

    SetRuleRequest req;
    memset(&req, 0, sizeof(req));

    printf("Room ID: ");
    scanf("%u", &req.room_id);
    req.room_id = htonl(req.room_id);

    printf("Mode (0=scoring, 1=elimination): ");
    scanf("%hhu", &req.mode);

    printf("Max Players (4-6): ");
    scanf("%hhu", &req.max_players);

    printf("Round Time (15-60): ");
    scanf("%hu", &req.round_time);
    req.round_time = htons(req.round_time);

    printf("Enable Wager (0/1): ");
    scanf("%hhu", &req.wager_enabled);

    if (send_packet(sock_fd, CMD_SET_RULE, &req, sizeof(req)) < 0) {
        return;
    }

    PacketHeader resp_header;
    if (recv_response(sock_fd, &resp_header, NULL, 0) < 0) {
        return;
    }

    if (resp_header.command == RES_SUCCESS) {
        printf("\n✅ Rules updated successfully!\n");
    } else {
        printf("❌ Failed to update rules\n");
    }
}

void handle_kick_member(int sock_fd) {
    printf("=== KICK MEMBER ===\n");
    
    typedef struct __attribute__((packed)) {
        uint32_t room_id;
        uint32_t target_user_id;
    } KickRequest;

    KickRequest req;

    printf("Room ID: ");
    scanf("%u", &req.room_id);
    req.room_id = htonl(req.room_id);

    printf("Target User ID to kick: ");
    scanf("%u", &req.target_user_id);
    req.target_user_id = htonl(req.target_user_id);

    if (send_packet(sock_fd, CMD_KICK, &req, sizeof(req)) < 0) {
        return;
    }

    PacketHeader resp_header;
    if (recv_response(sock_fd, &resp_header, NULL, 0) < 0) {
        return;
    }

    if (resp_header.command == RES_SUCCESS) {
        printf("\n✅ Member kicked successfully!\n");
    } else {
        printf("❌ Failed to kick member\n");
    }
}

void handle_delete_room(int sock_fd) {
    printf("=== DELETE ROOM ===\n");
    
    typedef struct __attribute__((packed)) {
        uint32_t room_id;
    } DeleteRoomRequest;

    DeleteRoomRequest req;

    printf("Room ID: ");
    scanf("%u", &req.room_id);
    req.room_id = htonl(req.room_id);

    if (send_packet(sock_fd, CMD_DELETE_ROOM, &req, sizeof(req)) < 0) {
        return;
    }

    PacketHeader resp_header;
    if (recv_response(sock_fd, &resp_header, NULL, 0) < 0) {
        return;
    }

    if (resp_header.command == RES_SUCCESS) {
        printf("\n✅ Room deleted successfully!\n");
    } else {
        printf("❌ Failed to delete room\n");
    }
}

void handle_start_game(int sock_fd) {
    printf("=== START GAME ===\n");
    
    typedef struct __attribute__((packed)) {
        uint32_t room_id;
    } StartGameRequest;

    typedef struct __attribute__((packed)) {
        uint32_t match_id;
        uint32_t countdown_ms;
        uint64_t server_timestamp_ms;
        uint64_t game_start_timestamp_ms;
    } GameStartingNotification;

    StartGameRequest req;

    printf("Room ID: ");
    scanf("%u", &req.room_id);
    req.room_id = htonl(req.room_id);

    if (send_packet(sock_fd, CMD_START_GAME, &req, sizeof(req)) < 0) {
        return;
    }

    PacketHeader resp_header;
    GameStartingNotification resp;
    
    if (recv_response(sock_fd, &resp_header, &resp, sizeof(resp)) < 0) {
        return;
    }

    if (resp_header.command == RES_GAME_STARTING || resp_header.command == RES_SUCCESS) {
        resp.match_id = ntohl(resp.match_id);
        resp.countdown_ms = ntohl(resp.countdown_ms);
        resp.server_timestamp_ms = be64toh(resp.server_timestamp_ms);
        resp.game_start_timestamp_ms = be64toh(resp.game_start_timestamp_ms);

        printf("\n✅ Game starting!\n");
        printf("   Match ID: %u\n", resp.match_id);
        printf("   Countdown: %u ms\n", resp.countdown_ms);
        printf("   Server Time: %lu ms\n", resp.server_timestamp_ms);
        printf("   Game Start Time: %lu ms\n", resp.game_start_timestamp_ms);
    } else {
        printf("❌ Failed to start game\n");
    }
}
