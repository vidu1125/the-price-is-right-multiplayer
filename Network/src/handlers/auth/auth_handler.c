#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "../../include/message_handler.h"
#include "../../include/protocol.h"
#include "../../include/server.h"
#include "../../include/database.h"
#include "../../include/utils.h"

//==============================================================================
// LOGIN HANDLER
//==============================================================================

void handle_login(int client_idx, char *payload, uint32_t length) {
    ClientConnection *client = &clients[client_idx];
    
    // ✅ CHỈ LOG Ở CHẾ ĐỘ VERBOSE
    if (g_log_level >= LOG_VERBOSE) {
        printf("[%s] Client %d: Login attempt\n", get_timestamp_string(), client_idx);
    }
    
    // Validate payload size
    if (length < sizeof(AuthRequestPayload)) {
        if (g_log_level >= LOG_NORMAL) {
            printf("[%s] Client %d: Invalid login payload size (%u bytes)\n", 
                   get_timestamp_string(), client_idx, length);
        }
        send_response(client->sockfd, ERR_BAD_REQUEST, "Invalid payload", 15);
        return;
    }
    
    // Check if already logged in
    if (client->state != STATE_UNAUTHENTICATED) {
        if (g_log_level >= LOG_VERBOSE) {
            printf("[%s] Client %d: Already authenticated\n", 
                   get_timestamp_string(), client_idx);
        }
        send_response(client->sockfd, ERR_BAD_REQUEST, "Already logged in", 17);
        return;
    }
    
    // Parse request
    AuthRequestPayload *req = (AuthRequestPayload *)payload;
    
    // Ensure null-terminated strings
    req->username[31] = '\0';
    req->password[63] = '\0';
    
    // Trim whitespace
    char username[32];
    char password[64];
    string_copy_safe(username, req->username, sizeof(username));
    string_copy_safe(password, req->password, sizeof(password));
    trim_string(username);
    trim_string(password);
    
    // Validate username format
    if (!validate_username(username)) {
        if (g_log_level >= LOG_NORMAL) {
            printf("[%s] Client %d: Invalid username format: '%s'\n", 
                   get_timestamp_string(), client_idx, username);
        }
        send_response(client->sockfd, ERR_INVALID_USERNAME, "Invalid username", 16);
        return;
    }
    
    // Hash password
    char password_hash[65];
    simple_hash(password, password_hash, sizeof(password_hash));
    
    // Authenticate with database
    uint32_t user_id;
    char display_name[32];
    uint32_t balance;
    
    int auth_result = db_authenticate_user(username, password_hash, 
                                           &user_id, display_name, &balance);
    
    if (auth_result < 0) {
        if (g_log_level >= LOG_NORMAL) {
            printf("[%s] Client %d: Authentication failed for '%s'\n", 
                   get_timestamp_string(), client_idx, username);
        }
        send_response(client->sockfd, ERR_NOT_LOGGED_IN, "Invalid credentials", 19);
        return;
    }
    
    // Check if user is already logged in from another connection
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (i != client_idx && 
            clients[i].sockfd != -1 && 
            clients[i].user_id == user_id &&
            clients[i].state != STATE_UNAUTHENTICATED) {
            
            if (g_log_level >= LOG_NORMAL) {
                printf("[%s] Client %d: User %u already logged in from client %d\n", 
                       get_timestamp_string(), client_idx, user_id, i);
            }
            send_response(client->sockfd, ERR_BAD_REQUEST, 
                         "User already logged in", 23);
            return;
        }
    }
    
    // Update client state
    client->state = STATE_AUTHENTICATED;
    client->user_id = user_id;
    client->session_id = generate_session_id();
    client->balance = balance;
    string_copy_safe(client->username, username, sizeof(client->username));
    string_copy_safe(client->display_name, display_name, sizeof(client->display_name));
    
    // Prepare response
    AuthSuccessPayload response;
    memset(&response, 0, sizeof(response));
    response.user_id = htonl(user_id);
    response.session_id = htonl(client->session_id);
    response.balance = htonl(balance);
    string_copy_safe(response.display_name, display_name, sizeof(response.display_name));
    
    // Send success response
    send_response(client->sockfd, RES_LOGIN_OK, (char *)&response, sizeof(response));
    
    // ✅ NORMAL: Log successful login
    if (g_log_level >= LOG_NORMAL) {
        printf("[%s] ✅ Client %d: Login successful - User: %s (ID: %u, Balance: %u)\n", 
               get_timestamp_string(), client_idx, username, user_id, balance);
    }
}

//==============================================================================
// REGISTER HANDLER
//==============================================================================

void handle_register(int client_idx, char *payload, uint32_t length) {
    ClientConnection *client = &clients[client_idx];
    
    // ✅ CHỈ LOG Ở CHẾ ĐỘ VERBOSE
    if (g_log_level >= LOG_VERBOSE) {
        printf("[%s] Client %d: Register attempt\n", get_timestamp_string(), client_idx);
    }
    
    // Validate payload size
    if (length < sizeof(AuthRequestPayload)) {
        if (g_log_level >= LOG_NORMAL) {
            printf("[%s] Client %d: Invalid register payload size (%u bytes)\n", 
                   get_timestamp_string(), client_idx, length);
        }
        send_response(client->sockfd, ERR_BAD_REQUEST, "Invalid payload", 15);
        return;
    }
    
    // Parse request
    AuthRequestPayload *req = (AuthRequestPayload *)payload;
    
    // Ensure null-terminated strings
    req->username[31] = '\0';
    req->password[63] = '\0';
    
    // Copy and trim
    char username[32];
    char password[64];
    string_copy_safe(username, req->username, sizeof(username));
    string_copy_safe(password, req->password, sizeof(password));
    trim_string(username);
    trim_string(password);
    
    // Validate username
    if (!validate_username(username)) {
        if (g_log_level >= LOG_NORMAL) {
            printf("[%s] Client %d: Invalid username format: '%s'\n", 
                   get_timestamp_string(), client_idx, username);
        }
        send_response(client->sockfd, ERR_INVALID_USERNAME, 
                     "Invalid username (3-31 chars, alphanumeric + underscore)", 57);
        return;
    }
    
    // Validate password
    if (!validate_password(password)) {
        if (g_log_level >= LOG_NORMAL) {
            printf("[%s] Client %d: Invalid password format (length: %zu)\n", 
                   get_timestamp_string(), client_idx, strlen(password));
        }
        send_response(client->sockfd, ERR_BAD_REQUEST, 
                     "Invalid password (6-63 chars required)", 39);
        return;
    }
    
    // Hash password
    char password_hash[65];
    simple_hash(password, password_hash, sizeof(password_hash));
    
    // Generate display name
    char display_name[32];
    generate_display_name(username, display_name, sizeof(display_name));
    
    // Register in database
    int reg_result = db_register_user(username, password_hash, display_name);
    
    if (reg_result < 0) {
        if (g_log_level >= LOG_NORMAL) {
            printf("[%s] Client %d: Registration failed for '%s' (username may exist)\n", 
                   get_timestamp_string(), client_idx, username);
        }
        send_response(client->sockfd, ERR_BAD_REQUEST, 
                     "Username already exists or database error", 42);
        return;
    }
    
    // Success
    send_response(client->sockfd, RES_SUCCESS, "Registration successful", 23);
    
    // ✅ NORMAL: Log successful registration
    if (g_log_level >= LOG_NORMAL) {
        printf("[%s] ✅ Client %d: Registration successful - Username: %s\n", 
               get_timestamp_string(), client_idx, username);
    }
}

//==============================================================================
// LOGOUT HANDLER
//==============================================================================

void handle_logout(int client_idx) {
    ClientConnection *client = &clients[client_idx];
    
    if (g_log_level >= LOG_VERBOSE) {
        printf("[%s] Client %d: Logout request from user '%s' (ID: %u)\n", 
               get_timestamp_string(), client_idx, 
               client->username[0] ? client->username : "anonymous", 
               client->user_id);
    }
    
    // Check if in a room
    if (client->room_id >= 0 && client->room_id < MAX_ROOMS) {
        if (g_log_level >= LOG_VERBOSE) {
            printf("[%s] Client %d: Leaving room %d before logout\n", 
                   get_timestamp_string(), client_idx, client->room_id);
        }
        handle_leave_room(client_idx);
    }
    
    // Store info for logging
    char username_copy[32];
    string_copy_safe(username_copy, client->username, sizeof(username_copy));
    uint32_t user_id = client->user_id;
    
    // Clear client state
    client->state = STATE_UNAUTHENTICATED;
    client->user_id = 0;
    client->session_id = 0;
    client->balance = 0;
    memset(client->username, 0, sizeof(client->username));
    memset(client->display_name, 0, sizeof(client->display_name));
    
    // Send success response
    send_response(client->sockfd, RES_SUCCESS, "Logged out successfully", 23);
    
    // ✅ NORMAL: Log successful logout
    if (g_log_level >= LOG_NORMAL) {
        printf("[%s] ✅ Client %d: Logout successful - Was user '%s' (ID: %u)\n", 
               get_timestamp_string(), client_idx, username_copy, user_id);
    }
}

//==============================================================================
// RECONNECT HANDLER (Optional - for future implementation)
//==============================================================================

void handle_reconnect(int client_idx, char *payload, uint32_t length) {
    ClientConnection *client = &clients[client_idx];
    
    if (g_log_level >= LOG_VERBOSE) {
        printf("[%s] Client %d: Reconnect attempt\n", 
               get_timestamp_string(), client_idx);
    }
    
    // TODO: Implement session-based reconnection
    
    send_response(client->sockfd, ERR_BAD_REQUEST, 
                 "Reconnect not implemented. Please login again", 46);
    
    (void)payload;
    (void)length;
}