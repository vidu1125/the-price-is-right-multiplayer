#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>



//==============================================================================
// SESSION ID GENERATION
//==============================================================================

uint32_t generate_session_id(void) {
    static uint32_t counter = 0;
    uint32_t timestamp = (uint32_t)time(NULL);
    uint32_t random = (uint32_t)rand();
    
    // Combine timestamp, random, and counter for uniqueness
    counter++;
    return (timestamp ^ random) + counter;
}

//==============================================================================
// USERNAME VALIDATION
//==============================================================================

int validate_username(const char *username) {
    if (!username) {
        return 0;
    }
    
    size_t len = strlen(username);
    
    // Username must be 3-31 characters
    if (len < 3 || len > 31) {
        return 0;
    }
    
    // First character must be letter
    if (!isalpha((unsigned char)username[0])) {
        return 0;
    }
    
    // Rest can be alphanumeric or underscore
    for (size_t i = 0; i < len && username[i] != '\0'; i++) {
        unsigned char c = (unsigned char)username[i];
        if (!isalnum(c) && c != '_') {
            return 0;
        }
    }
    
    return 1;
}

//==============================================================================
// PASSWORD VALIDATION
//==============================================================================

int validate_password(const char *password) {
    if (!password) {
        return 0;
    }
    
    size_t len = strlen(password);
    
    // Password must be 6-63 characters
    if (len < 6 || len > 63) {
        return 0;
    }
    
    // Check for printable characters only
    for (size_t i = 0; i < len && password[i] != '\0'; i++) {
        unsigned char c = (unsigned char)password[i];
        if (!isprint(c)) {
            return 0;
        }
    }
    
    return 1;
}

//==============================================================================
// SIMPLE HASHING (PRODUCTION: USE bcrypt/argon2)
//==============================================================================

void simple_hash(const char *input, char *output, size_t output_size) {
    // WARNING: This is NOT secure! Use for testing only!
    // In production, use bcrypt, argon2, or similar
    
    if (!input || !output || output_size < 65) {
        return;
    }
    
    unsigned long hash = 5381;
    int c;
    
    while ((c = *input++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    
    // Convert to hex string
    snprintf(output, output_size, "%016lx%016lx%016lx%016lx", 
             hash, hash ^ 0xDEADBEEF, hash ^ 0xCAFEBABE, hash ^ 0xFEEDFACE);
}

//==============================================================================
// STRING UTILITIES
//==============================================================================

void trim_string(char *str) {
    if (!str) return;
    
    // Trim leading spaces
    char *start = str;
    while (isspace((unsigned char)*start)) {
        start++;
    }
    
    // Trim trailing spaces
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) {
        end--;
    }
    *(end + 1) = '\0';
    
    // Move trimmed string to beginning
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

int string_copy_safe(char *dest, const char *src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return -1;
    }
    
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
    
    return 0;
}

//==============================================================================
// TIME UTILITIES
//==============================================================================

const char* get_timestamp_string(void) {
    static char buffer[32];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}

uint64_t get_timestamp_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
}

//==============================================================================
// RANDOM UTILITIES
//==============================================================================

int random_int(int min, int max) {
    if (min > max) {
        int temp = min;
        min = max;
        max = temp;
    }
    
    return min + (rand() % (max - min + 1));
}

void generate_random_string(char *buffer, size_t length) {
    static const char charset[] = 
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";
    
    if (!buffer || length == 0) {
        return;
    }
    
    for (size_t i = 0; i < length - 1; i++) {
        int key = rand() % (sizeof(charset) - 1);
        buffer[i] = charset[key];
    }
    buffer[length - 1] = '\0';
}

//==============================================================================
// DISPLAY NAME GENERATION
//==============================================================================

void generate_display_name(const char *username, char *display_name, size_t size) {
    if (!username || !display_name || size == 0) {
        return;
    }
    
    // Just copy username for now, can be enhanced later
    string_copy_safe(display_name, username, size);
    
    // Capitalize first letter
    if (islower((unsigned char)display_name[0])) {
        display_name[0] = toupper((unsigned char)display_name[0]);
    }
}