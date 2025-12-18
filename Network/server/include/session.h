// Network/server/include/session.h
#ifndef SESSION_H
#define SESSION_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

typedef enum {
    STATE_NOT_AUTH = 0,
    STATE_LOBBY,
    STATE_IN_ROOM,
    STATE_PLAYING
} ClientState;

typedef struct {
    int socket_fd;
    uint32_t user_id;
    uint32_t session_id;
    ClientState state;
    uint32_t room_id;
    uint32_t match_id;
    bool is_host;
    time_t last_heartbeat;
    char username[32];
} SessionContext;

// Session management functions
bool session_manager_init();
SessionContext* session_create(int socket_fd);
SessionContext* session_get(int socket_fd);
SessionContext* session_get_by_user_id(uint32_t user_id);
void session_destroy(int socket_fd);
void session_cleanup_all();

#endif // SESSION_H
