#ifndef SERVER_H
#define SERVER_H

#include <sys/select.h>
#include <time.h>
#include "protocol/protocol.h"  
//==============================================================================
// SERVER CONFIGURATION
//==============================================================================
#define SERVER_PORT 5500
#define MAX_CLIENTS 64
#define MAX_ROOMS 16
#define MAX_PLAYERS_PER_ROOM 8
#define HEARTBEAT_TIMEOUT 60
#define SELECT_TIMEOUT 30
#define RECONNECT_TIMEOUT 60

//==============================================================================
// LOG LEVELS
//==============================================================================
typedef enum {
    LOG_QUIET = 0,    // Chỉ errors
    LOG_NORMAL = 1,   // Connections + important events (DEFAULT)
    LOG_VERBOSE = 2   // Tất cả commands và details
} LogLevel;

//==============================================================================
// GAME DATA STRUCTURES
//==============================================================================

typedef struct {
    uint32_t user_id;
    int client_idx;
    char display_name[32];
    uint32_t score;
    uint8_t is_ready;
    uint8_t is_eliminated;
    uint8_t has_answered;
    uint32_t answer_data;
} Player;

typedef struct {
    uint32_t room_id;
    int host_client_idx;
    Player players[MAX_PLAYERS_PER_ROOM];
    int player_count;
    int max_players;
    GameMode game_mode;
    uint8_t is_playing;
    RoundType current_round;
    time_t round_start_time;
    uint16_t round_time_limit;
} GameRoom;

typedef struct {
    int sockfd;
    ConnectionState state;
    uint32_t user_id;
    uint32_t session_id;
    uint32_t balance;
    char username[32];
    char display_name[32];
    int room_id;
    
    char recv_buffer[BUFF_SIZE];
    int recv_offset;
    int expected_length;
    int header_received;
    MessageHeader current_header;
    
    time_t last_heartbeat;
    time_t disconnect_time;
} ClientConnection;

//==============================================================================
// GLOBAL VARIABLES (extern - defined in server.c)
//==============================================================================
extern ClientConnection clients[MAX_CLIENTS];
extern GameRoom rooms[MAX_ROOMS];
extern fd_set master_set, read_set;
extern int listen_fd, max_fd;
extern volatile int g_running;
extern LogLevel g_log_level;

//==============================================================================
// FUNCTION DECLARATIONS
//==============================================================================

// Server lifecycle
void initialize_server(void);
int create_listening_socket(void);
int main_loop(void);
void shutdown_server(void);
void signal_handler(int signum);

// Message processing
void handle_client_data(int client_idx);
void process_message(int client_idx, MessageHeader *header, char *payload);

#endif // SERVER_H