#ifndef ACCOUNT_REPO_H
#define ACCOUNT_REPO_H

#include "db/models/model.h"
#include "db/core/db_error.h"
#include <stdbool.h>

/**
 * Account Repository
 * 
 * Provides database operations for the accounts table.
 * All functions use Supabase REST API through db_client.
 */

/**
 * Create a new account
 * 
 * @param email User's email address (must be unique)
 * @param password_hash Hashed password (bcrypt)
 * @param role User role (default: "user")
 * @param out_account Pointer to store created account (caller must free)
 * @return DB_SUCCESS on success, error code otherwise
 */
db_error_t account_create(
    const char *email,
    const char *password_hash,
    const char *role,
    account_t **out_account
);

/**
 * Find account by email
 * 
 * @param email Email address to search for
 * @param out_account Pointer to store found account (caller must free)
 * @return DB_SUCCESS if found, DB_ERROR_NOT_FOUND if not exists
 */
db_error_t account_find_by_email(
    const char *email,
    account_t **out_account
);

/**
 * Find account by ID
 * 
 * @param account_id Account ID to search for
 * @param out_account Pointer to store found account (caller must free)
 * @return DB_SUCCESS if found, DB_ERROR_NOT_FOUND if not exists
 */
db_error_t account_find_by_id(
    int32_t account_id,
    account_t **out_account
);

/**
 * Update account password
 * 
 * @param account_id Account ID
 * @param new_password_hash New hashed password
 * @return DB_SUCCESS on success, error code otherwise
 */
db_error_t account_update_password(
    int32_t account_id,
    const char *new_password_hash
);

/**
 * Update account role
 * 
 * @param account_id Account ID
 * @param new_role New role (e.g., "admin", "user")
 * @return DB_SUCCESS on success, error code otherwise
 */
db_error_t account_update_role(
    int32_t account_id,
    const char *new_role
);

/**
 * Check if email already exists
 * 
 * @param email Email to check
 * @return true if email exists, false otherwise
 */
bool account_email_exists(const char *email);

/**
 * Free account structure
 * 
 * @param account Account to free
 */
void account_free(account_t *account);

#endif // ACCOUNT_REPO_H