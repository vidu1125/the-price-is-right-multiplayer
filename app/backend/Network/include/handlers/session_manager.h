#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "protocol/protocol.h"

typedef enum {
    SESSION_UNAUTHENTICATED = 0,
    SESSION_LOBBY = 1,
    SESSION_PLAYING = 2,
    SESSION_PLAYING_DISCONNECTED = 3
} SessionState;

typedef struct {
    int socket_fd;
    int32_t account_id;
    char session_id[37];
    SessionState state;
    time_t last_active;
    time_t grace_deadline; // only for DISCONNECTED
} UserSession;

void session_manager_init(void);
UserSession* session_get_by_socket(int socket_fd);
UserSession* session_get_by_account(int32_t account_id);
UserSession* session_bind_after_login(int socket_fd, int32_t account_id, const char *session_id,
                                      MessageHeader *req);
void session_mark_playing(UserSession *s);
void session_mark_lobby(UserSession *s);
void session_mark_disconnected(UserSession *s);
void session_destroy(UserSession *s);
void session_touch(int socket_fd);
void session_cleanup_dead_sessions(void);

#endif // SESSION_MANAGER_H
