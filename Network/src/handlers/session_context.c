#include "handlers/session_context.h"
#include <string.h>

// Naive fixed-size table; sufficient for small number of concurrent clients.
// If capacity is exceeded, oldest entries will be overwritten.

#define SESSION_SLOT_MAX 1024

typedef struct {
    int fd;
    int account_id;
    char session_id[37];
    int generation; // for simple LRU overwrite if needed
} session_slot_t;

static session_slot_t slots[SESSION_SLOT_MAX];
static int current_gen = 1;

static session_slot_t *find_slot(int client_fd) {
    for (int i = 0; i < SESSION_SLOT_MAX; i++) {
        if (slots[i].fd == client_fd) {
            return &slots[i];
        }
    }
    return NULL;
}

static session_slot_t *alloc_slot(int client_fd) {
    // reuse empty slot
    for (int i = 0; i < SESSION_SLOT_MAX; i++) {
        if (slots[i].fd == 0) return &slots[i];
    }
    // fallback: overwrite oldest
    int oldest_idx = 0;
    for (int i = 1; i < SESSION_SLOT_MAX; i++) {
        if (slots[i].generation < slots[oldest_idx].generation) {
            oldest_idx = i;
        }
    }
    return &slots[oldest_idx];
}

void set_client_session(int client_fd, const char *session_id, int account_id) {
    if (!session_id) return;
    session_slot_t *slot = find_slot(client_fd);
    if (!slot) slot = alloc_slot(client_fd);
    slot->fd = client_fd;
    slot->account_id = account_id;
    strncpy(slot->session_id, session_id, sizeof(slot->session_id) - 1);
    slot->session_id[sizeof(slot->session_id) - 1] = '\0';
    slot->generation = current_gen++;
    if (current_gen == 0) current_gen = 1; // avoid wrap to 0
}

void clear_client_session(int client_fd) {
    session_slot_t *slot = find_slot(client_fd);
    if (!slot) return;
    memset(slot, 0, sizeof(*slot));
}

bool has_client_session(int client_fd) {
    return find_slot(client_fd) != NULL;
}

const char *get_client_session(int client_fd) {
    session_slot_t *slot = find_slot(client_fd);
    return slot ? slot->session_id : NULL;
}

int get_client_account(int client_fd) {
    session_slot_t *slot = find_slot(client_fd);
    return slot ? slot->account_id : 0;
}
