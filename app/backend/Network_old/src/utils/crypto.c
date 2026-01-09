#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>
#include <uuid/uuid.h>
#include <crypt.h>
#include <time.h>
#include <unistd.h>

#include "utils/crypto.h"

// Bcrypt settings
#define BCRYPT_COST 10
#define BCRYPT_HASHSIZE 61

// Bcrypt using crypt() with $2b$ (blowfish)
// Format: $2b$rounds$salt+hash

static const char* generate_salt(int rounds) {
    static char salt[64];
    static const char saltchars[] = 
        "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    
    // Generate random salt
    unsigned char randbytes[16];
    FILE *fp = fopen("/dev/urandom", "r");
    if (fp) {
        fread(randbytes, 1, 16, fp);
        fclose(fp);
    } else {
        // Fallback to time-based seed
        srand(time(NULL) ^ getpid());
        for (int i = 0; i < 16; i++) {
            randbytes[i] = rand() & 0xFF;
        }
    }
    
    // Encode salt in base64-like format for bcrypt
    char encoded[23];
    for (int i = 0; i < 22; i++) {
        encoded[i] = saltchars[randbytes[i % 16] & 0x3F];
    }
    encoded[22] = '\0';
    
    // Format: $2b$rounds$salt
    snprintf(salt, sizeof(salt), "$2b$%02d$%s", rounds, encoded);
    return salt;
}

bool crypto_hash_password(const char *password, char *out_hash) {
    if (!password || !out_hash) {
        return false;
    }

    // Check password length
    if (strlen(password) == 0 || strlen(password) > 72) {
        // Bcrypt has a max password length of 72 characters
        return false;
    }

    const char *salt = generate_salt(BCRYPT_COST);
    
    // Use crypt_r for thread safety
    struct crypt_data data;
    data.initialized = 0;
    
    const char *hash = crypt_r(password, salt, &data);
    
    if (!hash || strlen(hash) < 20) {
        return false;
    }
    
    // Copy hash to output (max 60 chars + null)
    strncpy(out_hash, hash, BCRYPT_HASHSIZE - 1);
    out_hash[BCRYPT_HASHSIZE - 1] = '\0';
    
    return true;
}

bool crypto_verify_password(const char *password, const char *hash) {
    if (!password || !hash) {
        printf("[CRYPTO] Missing password or hash\n");
        return false;
    }

    printf("[CRYPTO] Verifying password against hash: %.20s...\n", hash);

    // Extract salt from hash for bcrypt
    // Hash format: $2b$10$22_char_salt_31_char_hash (total 60)
    // For crypt_r, we need the first 29 chars: $2b$10$salt_22_chars
    char salt[30];
    if (strlen(hash) < 29) {
        printf("[CRYPTO] Hash too short (expected 60, got %zu)\n", strlen(hash));
        return false;
    }
    
    strncpy(salt, hash, 29);
    salt[29] = '\0';
    
    printf("[CRYPTO] Extracted salt: %.20s...\n", salt);
    
    // Hash the password with the extracted salt
    struct crypt_data data;
    data.initialized = 0;
    
    const char *computed = crypt_r(password, salt, &data);
    
    if (!computed) {
        printf("[CRYPTO] crypt_r failed\n");
        return false;
    }
    
    // Constant-time comparison to prevent timing attacks
    size_t hash_len = strlen(hash);
    size_t comp_len = strlen(computed);
    
    printf("[CRYPTO] Hash length: %zu, Computed: %zu\n", hash_len, comp_len);
    printf("[CRYPTO] Stored:  %.30s...\n", hash);
    printf("[CRYPTO] Computed: %.30s...\n", computed);
    
    if (hash_len != comp_len) {
        printf("[CRYPTO] Password mismatch (length)\n");
        return false;
    }
    
    int result = 0;
    for (size_t i = 0; i < hash_len; i++) {
        result |= hash[i] ^ computed[i];
    }
    
    return result == 0;
}

bool crypto_generate_uuid(char *out_uuid) {
    if (!out_uuid) {
        return false;
    }

    uuid_t uuid;
    uuid_generate_random(uuid);
    uuid_unparse_lower(uuid, out_uuid);
    
    return true;
}

bool crypto_validate_email(const char *email) {
    if (!email) {
        return false;
    }

    // Basic email validation
    const char *at = strchr(email, '@');
    if (!at || at == email) {
        return false;  // No @ or @ is first character
    }

    const char *dot = strchr(at, '.');
    if (!dot || dot == at + 1 || *(dot + 1) == '\0') {
        return false;  // No dot after @ or dot is right after @ or at end
    }

    // Check for valid length
    size_t len = strlen(email);
    if (len < 3 || len > 100) {
        return false;
    }
    
    // Check for invalid characters before @
    for (const char *p = email; p < at; p++) {
        if (!(*p == '.' || *p == '-' || *p == '_' || *p == '+' ||
              (*p >= 'a' && *p <= 'z') ||
              (*p >= 'A' && *p <= 'Z') ||
              (*p >= '0' && *p <= '9'))) {
            return false;
        }
    }
    
    // Check domain part
    const char *domain_end = email + len;
    for (const char *p = at + 1; p < domain_end; p++) {
        if (!(*p == '.' || *p == '-' ||
              (*p >= 'a' && *p <= 'z') ||
              (*p >= 'A' && *p <= 'Z') ||
              (*p >= '0' && *p <= '9'))) {
            return false;
        }
    }

    return true;
}