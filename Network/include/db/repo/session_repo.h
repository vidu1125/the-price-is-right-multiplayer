#ifndef SESSION_REPO_H
#define SESSION_REPO_H

#include "db/models/model.h"
#include "db/core/db_error.h"
#include <stdbool.h>

/**
 * Session Repository
 * 
 * Provides database operations for the sessions table.
 * Sessions track active user connections and enable reconnection.
 */

/**
 * Create a new session for an account
 * 
 * @param account_id Account ID to create session for
 * @param out_session Pointer to store created session (caller must free)
 * @return DB_SUCCESS on success, error code otherwise
 * 
 * Note: Automatically generates a new UUID for session_id
 */
db_error_t session_create(
    int32_t account_id,
    session_t **out_session
);

/**
 * Find session by session_id
 * 
 * @param session_id Session UUID to search for
 * @param out_session Pointer to store found session (caller must free)
 * @return DB_SUCCESS if found, DB_ERROR_NOT_FOUND if not exists
 */
db_error_t session_find_by_id(
    const char *session_id,
    session_t **out_session
);

/**
 * Find session by account_id
 * 
 * @param account_id Account ID to search for
 * @param out_session Pointer to store found session (caller must free)
 * @return DB_SUCCESS if found, DB_ERROR_NOT_FOUND if not exists
 */
db_error_t session_find_by_account(
    int32_t account_id,
    session_t **out_session
);

/**
 * Update session connection status
 * 
 * @param session_id Session UUID
 * @param connected true to mark as connected, false for disconnected
 * @return DB_SUCCESS on success, error code otherwise
 */
db_error_t session_update_connected(
    const char *session_id,
    bool connected
);

/**
 * Update session for reconnection
 * Updates both connection status and updated_at timestamp
 * 
 * @param session_id Session UUID
 * @return DB_SUCCESS on success, error code otherwise
 */
db_error_t session_reconnect(
    const char *session_id
);

/**
 * Delete session (logout)
 * 
 * @param session_id Session UUID to delete
 * @return DB_SUCCESS on success, error code otherwise
 */
db_error_t session_delete(
    const char *session_id
);

/**
 * Delete session by account_id
 * 
 * @param account_id Account ID whose session to delete
 * @return DB_SUCCESS on success, error code otherwise
 */
db_error_t session_delete_by_account(
    int32_t account_id
);

/**
 * Check if session is valid and connected
 * 
 * @param session_id Session UUID to check
 * @return true if session exists and connected, false otherwise
 */
bool session_is_valid(const char *session_id);

/**
 * Free session structure
 * 
 * @param session Session to free
 */
void session_free(session_t *session);

#endif // SESSION_REPO_H
