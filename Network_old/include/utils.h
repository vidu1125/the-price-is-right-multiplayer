#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>

//==============================================================================
// SESSION & ID GENERATION
//==============================================================================

uint32_t generate_session_id(void);
int random_int(int min, int max);
void generate_random_string(char *buffer, size_t length);

//==============================================================================
// VALIDATION FUNCTIONS
//==============================================================================

int validate_username(const char *username);
int validate_password(const char *password);

//==============================================================================
// HASHING (WARNING: Use proper library in production)
//==============================================================================

void simple_hash(const char *input, char *output, size_t output_size);

//==============================================================================
// STRING UTILITIES
//==============================================================================

void trim_string(char *str);
int string_copy_safe(char *dest, const char *src, size_t dest_size);
void generate_display_name(const char *username, char *display_name, size_t size);

//==============================================================================
// TIME UTILITIES
//==============================================================================

const char* get_timestamp_string(void);
uint64_t get_timestamp_ms(void);

#endif // UTILS_H