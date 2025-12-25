#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

#include "../include/handler/client_manager.h"
#include "../include/protocol/protocol.h"
#include "../include/server.h"

//==============================================================================
// CLIENT INITIALIZATION
//==============================================================================

void initialize_clients() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sockfd = -1;
        clients[i].state = STATE_UNAUTHENTICATED;
        clients[i].user_id = 0;
        clients[i].session_id = 0;
        clients[i].balance = 0;
        clients[i].room_id = -1;
        clients[i].recv_offset = 0;
        clients[i].expected_length = 0;
        clients[i].header_received = 0;
        clients[i].last_heartbeat = 0;
        clients[i].disconnect_time = 0;
        memset(clients[i].username, 0, sizeof(clients[i].username));
        memset(clients[i].display_name, 0, sizeof(clients[i].display_name));
        memset(clients[i].recv_buffer, 0, sizeof(clients[i].recv_buffer));
        memset(&clients[i].current_header, 0, sizeof(MessageHeader));
    }
}

//==============================================================================
// NEW CONNECTION HANDLING
//==============================================================================

void handle_new_connection() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int new_fd, idx = -1;
    
    new_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (new_fd < 0) {
        if (errno != EINTR) {
            perror("Accept failed");
        }
        return;
    }
    
    // Find empty slot
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd == -1) {
            idx = i;
            break;
        }
    }
    
    if (idx == -1) {
        if (g_log_level >= LOG_NORMAL) {
            printf("⚠️  Max clients reached, rejecting connection\n");
        }
        close(new_fd);
        return;
    }
    
    // Initialize client connection
    clients[idx].sockfd = new_fd;
    clients[idx].state = STATE_UNAUTHENTICATED;
    clients[idx].user_id = 0;
    clients[idx].session_id = 0;
    clients[idx].balance = 0;
    clients[idx].room_id = -1;
    clients[idx].recv_offset = 0;
    clients[idx].expected_length = 0;
    clients[idx].header_received = 0;
    clients[idx].last_heartbeat = time(NULL);
    clients[idx].disconnect_time = 0;
    memset(clients[idx].username, 0, sizeof(clients[idx].username));
    memset(clients[idx].display_name, 0, sizeof(clients[idx].display_name));
    memset(clients[idx].recv_buffer, 0, sizeof(clients[idx].recv_buffer));
    memset(&clients[idx].current_header, 0, sizeof(MessageHeader));
    
    // Add to select set
    FD_SET(new_fd, &master_set);
    if (new_fd > max_fd) {
        max_fd = new_fd;
    }
    
    // ✅ CHỈ LOG Ở CHẾ ĐỘ NORMAL TRỞ LÊN
    if (g_log_level >= LOG_NORMAL) {
        printf("🔌 New connection from %s:%d (slot %d)\n",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               idx);
    }
}

//==============================================================================
// CLIENT CLEANUP
//==============================================================================

void cleanup_client(int client_idx) {
    ClientConnection *client = &clients[client_idx];
    
    if (client->sockfd != -1) {
        // Get peer address for logging
        struct sockaddr_in peer_addr;
        socklen_t addr_len = sizeof(peer_addr);
        
        // ✅ CHỈ LOG Ở CHẾ ĐỘ NORMAL TRỞ LÊN
        if (g_log_level >= LOG_NORMAL) {
            if (getpeername(client->sockfd, (struct sockaddr *)&peer_addr, &addr_len) == 0) {
                printf("🔌 Connection closed: %s:%d (slot %d, user: %s)\n", 
                       inet_ntoa(peer_addr.sin_addr),
                       ntohs(peer_addr.sin_port),
                       client_idx,
                       client->username[0] ? client->username : "anonymous");
            } else {
                printf("🔌 Connection closed: client slot %d (user: %s)\n", 
                       client_idx,
                       client->username[0] ? client->username : "anonymous");
            }
        }
        
        // Remove from fd_set BEFORE closing socket
        FD_CLR(client->sockfd, &master_set);
        
        // Close socket
        close(client->sockfd);
        
        // Mark disconnect time
        client->disconnect_time = time(NULL);
    }
    
    // If client was in a room, handle room cleanup
    if (client->room_id >= 0 && client->room_id < MAX_ROOMS) {
        if (g_log_level >= LOG_VERBOSE) {
            printf("Client %d leaving room %d during cleanup\n", 
                   client_idx, client->room_id);
        }
        // Note: You may need to implement remove_player_from_room() in room_manager
        client->room_id = -1;
    }
    
    // Reset client state completely
    client->sockfd = -1;
    client->state = STATE_UNAUTHENTICATED;
    client->user_id = 0;
    client->session_id = 0;
    client->balance = 0;
    client->room_id = -1;
    client->recv_offset = 0;
    client->expected_length = 0;
    client->header_received = 0;
    client->last_heartbeat = 0;
    memset(client->username, 0, sizeof(client->username));
    memset(client->display_name, 0, sizeof(client->display_name));
    memset(client->recv_buffer, 0, sizeof(client->recv_buffer));
    memset(&client->current_header, 0, sizeof(MessageHeader));
}

//==============================================================================
// TIMEOUT CHECKING
//==============================================================================

void check_client_timeouts() {
    time_t now = time(NULL);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd != -1 && clients[i].last_heartbeat > 0) {
            time_t elapsed = now - clients[i].last_heartbeat;
            
            if (elapsed > HEARTBEAT_TIMEOUT) {
                if (g_log_level >= LOG_NORMAL) {
                    printf("⏱️  Client %d timeout (no heartbeat for %ld seconds)\n", 
                           i, elapsed);
                }
                cleanup_client(i);
            }
        }
    }
}

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

int find_client_by_user_id(uint32_t user_id) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd != -1 && clients[i].user_id == user_id) {
            return i;
        }
    }
    return -1;
}

int find_client_by_username(const char *username) {
    if (username == NULL || username[0] == '\0') {
        return -1;
    }
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd != -1 && 
            strcmp(clients[i].username, username) == 0) {
            return i;
        }
    }
    return -1;
}

int get_active_client_count() {
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd != -1) {
            count++;
        }
    }
    return count;
}

int get_authenticated_client_count() {
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd != -1 && 
            clients[i].state == STATE_AUTHENTICATED) {
            count++;
        }
    }
    return count;
}

void broadcast_to_all_clients(uint16_t command, const char *payload, uint32_t length) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sockfd != -1 && 
            clients[i].state == STATE_AUTHENTICATED) {
            send_response(clients[i].sockfd, command, payload, length);
        }
    }
}

int is_user_logged_in(uint32_t user_id) {
    return find_client_by_user_id(user_id) != -1;
}

void update_client_balance(int client_idx, uint32_t new_balance) {
    if (client_idx >= 0 && client_idx < MAX_CLIENTS && 
        clients[client_idx].sockfd != -1) {
        clients[client_idx].balance = new_balance;
    }
}

void update_client_heartbeat(int client_idx) {
    if (client_idx >= 0 && client_idx < MAX_CLIENTS && 
        clients[client_idx].sockfd != -1) {
        clients[client_idx].last_heartbeat = time(NULL);
    }
}