#ifndef SESSION_CONTEXT_H
#define SESSION_CONTEXT_H

#include <stdbool.h>

// Simple in-memory mapping of client_fd -> session/account
void set_client_session(int client_fd, const char *session_id, int account_id);
void clear_client_session(int client_fd);
bool has_client_session(int client_fd);
const char *get_client_session(int client_fd);
int get_client_account(int client_fd);

#endif // SESSION_CONTEXT_H