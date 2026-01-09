#include "utils/json_utils.h"
#include <string.h>

/**
 * Escape JSON string: " → \", \ → \\, \n → \\n
 * Returns escaped string in static buffer (NOT thread-safe)
 */
const char* json_escape_string(const char *input) {
    static char buffer[512];
    const char *src = input;
    char *dst = buffer;
    char *end = buffer + sizeof(buffer) - 1;
    
    while (*src && dst < end) {
        switch (*src) {
            case '"':
                if (dst + 1 < end) {
                    *dst++ = '\\';
                    *dst++ = '"';
                }
                break;
            case '\\':
                if (dst + 1 < end) {
                    *dst++ = '\\';
                    *dst++ = '\\';
                }
                break;
            case '\n':
                if (dst + 1 < end) {
                    *dst++ = '\\';
                    *dst++ = 'n';
                }
                break;
            case '\r':
                if (dst + 1 < end) {
                    *dst++ = '\\';
                    *dst++ = 'r';
                }
                break;
            case '\t':
                if (dst + 1 < end) {
                    *dst++ = '\\';
                    *dst++ = 't';
                }
                break;
            case '\b':
                if (dst + 1 < end) {
                    *dst++ = '\\';
                    *dst++ = 'b';
                }
                break;
            case '\f':
                if (dst + 1 < end) {
                    *dst++ = '\\';
                    *dst++ = 'f';
                }
                break;
            default:
                *dst++ = *src;
                break;
        }
        src++;
    }
    *dst = '\0';
    return buffer;
}
