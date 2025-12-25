#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <time.h>

#include "../include/server.h"
#include "../include/protocol/protocol.h"
#include "../include/handler/client_manager.h"
#include "../include/handler/room_manager.h"
#include "../include/database.h"
#include "../include/handler//message_handler.h"

//==============================================================================
// GLOBAL VARIABLES
//==============================================================================
ClientConnection clients[MAX_CLIENTS];
GameRoom rooms[MAX_ROOMS];
fd_set master_set, read_set;
int listen_fd, max_fd;
volatile int g_running = 1;

// ✅ LOG LEVEL (Mặc định: NORMAL)
LogLevel g_log_level = LOG_NORMAL;

//==============================================================================
// SERVER INITIALIZATION
//==============================================================================

int create_listening_socket() {
    struct sockaddr_in server_addr;
    int sockfd, opt = 1;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);
    
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    if (listen(sockfd, 10) < 0) {
        perror("Listen failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    return sockfd;
}

void initialize_server() {
    srand(time(NULL));
    initialize_clients();
    init_rooms();
    
    if (db_init() < 0) {
        fprintf(stderr, "Failed to initialize database\n");
        exit(EXIT_FAILURE);
    }
    
    listen_fd = create_listening_socket();
    
    FD_ZERO(&master_set);
    FD_SET(listen_fd, &master_set);
    max_fd = listen_fd;
    
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("Server started on port %d\n", SERVER_PORT);
    printf("Log level: %s\n", 
           g_log_level == LOG_QUIET ? "QUIET" :
           g_log_level == LOG_NORMAL ? "NORMAL" : "VERBOSE");
}

//==============================================================================
// MESSAGE PROCESSING
//==============================================================================

void process_message(int client_idx, MessageHeader *header, char *payload) {
    ClientConnection *client = &clients[client_idx];
    
    // ✅ CHỈ LOG Ở CHẾ ĐỘ VERBOSE
    if (g_log_level >= LOG_VERBOSE && header->command != CMD_HEARTBEAT) {
        printf("[VERBOSE] Processing command: 0x%04X from client %d (%s)\n", 
               header->command, 
               client_idx,
               client->username[0] ? client->username : "anonymous");
    }
    
    if (!validate_command_for_state(header->command, client->state)) {
        send_response(client->sockfd, ERR_NOT_LOGGED_IN, "Unauthorized", 12);
        return;
    }
    
    switch (header->command) {
        case CMD_LOGIN_REQ:
            handle_login(client_idx, payload, header->length);
            break;
        case CMD_REGISTER_REQ:
            handle_register(client_idx, payload, header->length);
            break;
        case CMD_LOGOUT_REQ:
            handle_logout(client_idx);
            break;
        case CMD_CREATE_ROOM:
            handle_create_room(client_idx, payload, header->length);
            break;
        case CMD_JOIN_ROOM:
            handle_join_room(client_idx, payload, header->length);
            break;
        case CMD_LEAVE_ROOM:
            handle_leave_room(client_idx);
            break;
        case CMD_READY:
            handle_ready(client_idx, payload, header->length);
            break;
        case CMD_KICK:
            handle_kick(client_idx, payload, header->length);
            break;
        case CMD_START_GAME:
            handle_start_game(client_idx);
            break;
        case CMD_ANSWER_QUIZ:
            handle_answer_quiz(client_idx, payload, header->length);
            break;
        case CMD_BID:
            handle_bid(client_idx, payload, header->length);
            break;
        case CMD_SPIN:
            handle_spin(client_idx);
            break;
        case CMD_FORFEIT:
            handle_forfeit(client_idx);
            break;
        case CMD_CHAT:
            handle_chat(client_idx, payload, header->length);
            break;
        case CMD_HIST:
            handle_history(client_idx);
            break;
        case CMD_LEAD:
            handle_leaderboard(client_idx);
            break;
        case CMD_HEARTBEAT:
            handle_heartbeat(client_idx);
            break;
        default:
            send_response(client->sockfd, ERR_BAD_REQUEST, "Unknown command", 15);
            break;
    }
}

void handle_client_data(int client_idx) {
    ClientConnection *client = &clients[client_idx];
    MessageHeader header;
    char payload[BUFF_SIZE];
    int result;
    
    if (!client->header_received) {
        result = receive_header(client_idx, &header);
        if (result < 0) {
            if (g_log_level >= LOG_NORMAL) {
                printf("Invalid header from client %d, closing connection\n", client_idx);
            }
            cleanup_client(client_idx);
            return;
        } else if (result == 0) {
            return;  // Need more data
        }
    }
    
    result = receive_payload(client_idx, payload, client->expected_length);
    if (result < 0) {
        if (g_log_level >= LOG_VERBOSE) {
            printf("Connection closed by client %d\n", client_idx);
        }
        cleanup_client(client_idx);
        return;
    } else if (result == 0) {
        return;  // Need more data
    }
    
    process_message(client_idx, &client->current_header, payload);
}

//==============================================================================
// MAIN LOOP
//==============================================================================

int main_loop() {
    int n_events;
    struct timeval timeout;
    
    while (g_running) {
        read_set = master_set;
        
        timeout.tv_sec = SELECT_TIMEOUT;
        timeout.tv_usec = 0;
        
        n_events = select(max_fd + 1, &read_set, NULL, NULL, &timeout);
        
        if (n_events < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }
        
        if (n_events == 0) {
            check_client_timeouts();
            continue;
        }
        
        if (FD_ISSET(listen_fd, &read_set)) {
            handle_new_connection();
            n_events--;
        }
        
        for (int i = 0; i < MAX_CLIENTS && n_events > 0; i++) {
            if (clients[i].sockfd != -1 && FD_ISSET(clients[i].sockfd, &read_set)) {
                handle_client_data(i);
                n_events--;
            }
        }
    }
    
    return 0;
}

//==============================================================================
// SHUTDOWN
//==============================================================================

void shutdown_server() {
    printf("Shutting down server...\n");
    
    // Disconnect all clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd != -1) {
            cleanup_client(i);
        }
    }
    
    // Close listening socket
    if (listen_fd != -1) {
        close(listen_fd);
    }
    
    // Close database
    db_close();
    
    printf("Server shutdown complete\n");
}