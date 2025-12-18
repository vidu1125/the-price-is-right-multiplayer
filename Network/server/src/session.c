// Network/server/src/session.c
#include "session.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_SESSIONS 1000

static SessionContext sessions[MAX_SESSIONS];
static int session_count = 0;

bool session_manager_init() {
    memset(sessions, 0, sizeof(sessions));
    session_count = 0;
    printf("[Session] Manager initialized\n");
    return true;
}

SessionContext* session_create(int socket_fd) {
    if (session_count >= MAX_SESSIONS) {
        fprintf(stderr, "[Session] Max sessions reached\n");
        return NULL;
    }
    
    SessionContext* ctx = &sessions[session_count++];
    memset(ctx, 0, sizeof(SessionContext));
    
    ctx->socket_fd = socket_fd;
    ctx->state = STATE_NOT_AUTH;
    ctx->last_heartbeat = time(NULL);
    
    printf("[Session] Created session for fd=%d\n", socket_fd);
    return ctx;
}

SessionContext* session_get(int socket_fd) {
    for (int i = 0; i < session_count; i++) {
        if (sessions[i].socket_fd == socket_fd && sessions[i].socket_fd > 0) {
            return &sessions[i];
        }
    }
    return NULL;
}

SessionContext* session_get_by_user_id(uint32_t user_id) {
    for (int i = 0; i < session_count; i++) {
        if (sessions[i].user_id == user_id && sessions[i].socket_fd > 0) {
            return &sessions[i];
        }
    }
    return NULL;
}

void session_destroy(int socket_fd) {
    for (int i = 0; i < session_count; i++) {
        if (sessions[i].socket_fd == socket_fd) {
            printf("[Session] Destroyed session for fd=%d (user_id=%u)\n",
                   socket_fd, sessions[i].user_id);
            sessions[i].socket_fd = -1;  // Mark as destroyed
            return;
        }
    }
}

void session_cleanup_all() {
    session_count = 0;
    memset(sessions, 0, sizeof(sessions));
    printf("[Session] All sessions cleaned up\n");
}
