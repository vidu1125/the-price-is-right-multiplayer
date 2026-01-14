#ifndef CRYPTO_H
#define CRYPTO_H

#include <stdbool.h>

/**
 * Crypto Utilities
 * 
 * Provides password hashing and verification using bcrypt.
 * Uses OpenSSL's bcrypt implementation.
 */

/**
 * Hash a password using bcrypt
 * 
 * @param password Plain text password to hash
 * @param out_hash Buffer to store the hash (must be at least 61 bytes)
 * @return true on success, false on error
 * 
 * Note: bcrypt output format: $2b$[cost]$[salt][hash]
 *       Example: $2b$12$R9h/cIPz0gi.URNNX3kh2OPST9/PgBkqquzi.Ss7KIUgO2t0jWMUW
 */
bool crypto_hash_password(const char *password, char *out_hash);

/**
 * Verify a password against a bcrypt hash
 * 
 * @param password Plain text password to verify
 * @param hash Bcrypt hash to verify against
 * @return true if password matches, false otherwise
 */
bool crypto_verify_password(const char *password, const char *hash);

/**
 * Generate a random UUID v4
 * 
 * @param out_uuid Buffer to store UUID (must be at least 37 bytes)
 * @return true on success, false on error
 * 
 * Format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
 * Example: 550e8400-e29b-41d4-a716-446655440000
 */
bool crypto_generate_uuid(char *out_uuid);

/**
 * Validate email format
 * 
 * @param email Email address to validate
 * @return true if valid format, false otherwise
 * 
 * Basic validation: checks for @ symbol and domain
 */
bool crypto_validate_email(const char *email);

#endif // CRYPTO_H