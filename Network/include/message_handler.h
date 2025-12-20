#ifndef MESSAGE_HANDLER_H
#define MESSAGE_HANDLER_H

#include "protocol.h"
#include <stdint.h>

//==============================================================================
// AUTH HANDLERS
//==============================================================================
void handle_login(int client_idx, char *payload, uint32_t length);
void handle_register(int client_idx, char *payload, uint32_t length);
void handle_logout(int client_idx);
void handle_reconnect(int client_idx, char *payload, uint32_t length);

//==============================================================================
// ROOM HANDLERS
//==============================================================================
void handle_create_room(int client_idx, char *payload, uint32_t length);
void handle_join_room(int client_idx, char *payload, uint32_t length);
void handle_leave_room(int client_idx);
void handle_ready(int client_idx, char *payload, uint32_t length);
void handle_kick(int client_idx, char *payload, uint32_t length);

//==============================================================================
// GAME HANDLERS
//==============================================================================
void handle_start_game(int client_idx);
void handle_answer_quiz(int client_idx, char *payload, uint32_t length);
void handle_bid(int client_idx, char *payload, uint32_t length);
void handle_spin(int client_idx);
void handle_forfeit(int client_idx);

//==============================================================================
// SOCIAL HANDLERS
//==============================================================================
void handle_chat(int client_idx, char *payload, uint32_t length);
void handle_history(int client_idx);
void handle_leaderboard(int client_idx);

//==============================================================================
// SYSTEM HANDLERS
//==============================================================================
void handle_heartbeat(int client_idx);

//==============================================================================
// COMMON HANDLER UTILITIES
//==============================================================================

// State validation
int validate_client_state(int client_idx, ConnectionState state);
int require_authentication(int client_idx);
int require_room(int client_idx);
int require_playing(int client_idx);
int require_host(int client_idx);

// Response helpers
void send_error(int client_idx, uint16_t error_code, const char *message);
void send_success(int client_idx, const char *message);

// Payload validation
int validate_payload_size(int client_idx, uint32_t actual_size, 
                          uint32_t expected_size, const char *payload_name);

// Logging helpers
void log_command(int client_idx, uint16_t command, const char *description);
void log_error(int client_idx, const char *context, const char *error);
void log_info(int client_idx, const char *message);

#endif // MESSAGE_HANDLER_H