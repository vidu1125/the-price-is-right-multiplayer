#ifndef CLIENT_MANAGER_H
#define CLIENT_MANAGER_H

#include <stdint.h>
#include <time.h>

//==============================================================================
// FUNCTION DECLARATIONS
//==============================================================================

// Client initialization
void initialize_clients(void);

// Connection handling
void handle_new_connection(void);
void cleanup_client(int client_idx);

// Timeout management
void check_client_timeouts(void);

// Client lookup
int find_client_by_user_id(uint32_t user_id);
int find_client_by_username(const char *username);

// Client statistics
int get_active_client_count(void);
int get_authenticated_client_count(void);

// Client utilities
void broadcast_to_all_clients(uint16_t command, const char *payload, uint32_t length);
int is_user_logged_in(uint32_t user_id);
void update_client_balance(int client_idx, uint32_t new_balance);
void update_client_heartbeat(int client_idx);

#endif // CLIENT_MANAGER_H