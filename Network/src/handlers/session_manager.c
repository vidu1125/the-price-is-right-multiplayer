#include "handlers/session_manager.h"
#include "protocol/opcode.h"
#include "transport/room_manager.h"
#include "handlers/session_context.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "db/repo/session_repo.h"

#define MAX_SESSIONS 1024
#define IDLE_SESSION_TIMEOUT_SEC 360  // 5 minutes for idle session cleanup
#define RECONNECT_GRACE_SEC 300  // 5 minutes realistic reconnect window

static UserSession g_sessions[MAX_SESSIONS];

static UserSession* alloc_slot(void) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (g_sessions[i].socket_fd == 0 && g_sessions[i].account_id == 0) {
            return &g_sessions[i];
        }
    }
    // fallback overwrite
    return &g_sessions[0];
}

void session_manager_init(void) {
    memset(g_sessions, 0, sizeof(g_sessions));
}

UserSession* session_get_by_socket(int socket_fd) {
    if (socket_fd <= 0) return NULL;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (g_sessions[i].socket_fd == socket_fd) return &g_sessions[i];
    }
    return NULL;
}

UserSession* session_get_by_account(int32_t account_id) {
    if (account_id <= 0) return NULL;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (g_sessions[i].account_id == account_id) return &g_sessions[i];
    }
    return NULL;
}

static void send_msg(int fd, MessageHeader *req, uint16_t code, const char *msg) {
    if (fd <= 0) return;
    const char *payload = msg ? msg : "";
    forward_response(fd, req, code, payload, (uint32_t)strlen(payload));
}

// Helper to forcibly log out an old socket that is not currently playing
// Sends a message, removes from rooms, clears session binding and marks DB disconnected.
static void force_logout_old_socket(UserSession *existing, MessageHeader *req) {
    if (!existing) return;

    // Notify old client
    send_msg(existing->socket_fd, req, ERR_NOT_LOGGED_IN,
             "Account logged in from another device. You have been logged out.");

    // Remove from any rooms
    room_remove_member_all(existing->socket_fd);

    // Clear local binding so require_auth() will fail
    clear_client_session(existing->socket_fd);

    // Close socket (disconnect handler will update DB when socket closes)
    close(existing->socket_fd);
}

UserSession* session_bind_after_login(int socket_fd, int32_t account_id, const char *session_id,
                                      MessageHeader *req) {
    time_t now = time(NULL);
    UserSession *existing = session_get_by_account(account_id);
    if (!existing) {
        UserSession *s = alloc_slot();
        memset(s, 0, sizeof(*s));
        s->socket_fd = socket_fd;
        s->account_id = account_id;
        strncpy(s->session_id, session_id, sizeof(s->session_id) - 1);
        s->state = SESSION_LOBBY;
        s->last_active = now;
        s->grace_deadline = 0;
        return s;
    }

    // Conflict handling per session rule
    switch (existing->state) {
        case SESSION_LOBBY:
        case SESSION_UNAUTHENTICATED: {
            // Force logout old (not in match) per workflow: only one active session when idle
            // Only kick if it's a DIFFERENT socket (avoid kicking ourselves on FD reuse)
            if (existing->socket_fd != socket_fd) {
                force_logout_old_socket(existing, req);
            }
            // Rebind to new socket
            existing->socket_fd = socket_fd;
            strncpy(existing->session_id, session_id, sizeof(existing->session_id) - 1);
            existing->state = SESSION_LOBBY;
            existing->last_active = now;
            existing->grace_deadline = 0;
            return existing;
        }
        case SESSION_PLAYING: {
            // Block new login
            send_msg(socket_fd, req, ERR_BAD_REQUEST, "Account is currently in a match. Login blocked.");
            close(socket_fd);
            return NULL;
        }
        case SESSION_PLAYING_DISCONNECTED: {
            // Allow reconnect
            existing->socket_fd = socket_fd;
            existing->state = SESSION_PLAYING;
            existing->last_active = now;
            existing->grace_deadline = 0;
            // TODO: send GAME_STATE here
            return existing;
        }
        default:
            break;
    }

    return existing;
}

void session_mark_playing(UserSession *s) {
    if (!s) return;
    s->state = SESSION_PLAYING;
}

void session_mark_lobby(UserSession *s) {
    if (!s) return;
    s->state = SESSION_LOBBY;
}

void session_mark_disconnected(UserSession *s) {
    if (!s) return;
    if (s->state == SESSION_PLAYING) {
        s->state = SESSION_PLAYING_DISCONNECTED;
        s->grace_deadline = time(NULL) + RECONNECT_GRACE_SEC;
    }
}

void session_destroy(UserSession *s) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
}

void session_touch(int socket_fd) {
    UserSession *s = session_get_by_socket(socket_fd);
    if (s) s->last_active = time(NULL);
}

void session_cleanup_dead_sessions(void) {
    time_t now = time(NULL);
    for (int i = 0; i < MAX_SESSIONS; i++) {
        UserSession *s = &g_sessions[i];
        if (s->account_id == 0) continue;

        if (s->state == SESSION_LOBBY || s->state == SESSION_UNAUTHENTICATED) {
            // Only cleanup sessions idle for more than 6 minutes
            if (s->last_active && now - s->last_active > IDLE_SESSION_TIMEOUT_SEC) {
                // remove idle dead session
                if (strlen(s->session_id) > 0) {
                    session_delete(s->session_id);
                }
                session_destroy(s);
            }
        } else if (s->state == SESSION_PLAYING_DISCONNECTED) {
            if (s->grace_deadline && now > s->grace_deadline) {
                // TODO: broadcast FORFEIT to room and remove from room
                printf("[SESSION] Grace timeout → forfeit account_id=%d\n", s->account_id);
                // Remove from any rooms and notify members
                room_remove_member_all(s->socket_fd);
                if (strlen(s->session_id) > 0) {
                    session_delete(s->session_id);
                }
                session_destroy(s);
            }
        }
    }
}
