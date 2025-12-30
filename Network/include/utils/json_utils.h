#ifndef JSON_UTILS_H
#define JSON_UTILS_H

/**
 * json_utils.h - JSON string escaping utilities
 * RIÊNG - Each handler can use this
 */

/**
 * Escape JSON string: " → \", \ → \\, \n → \\n
 * Returns escaped string in static buffer (NOT thread-safe)
 * 
 * @param input - Input string to escape
 * @return Escaped string (valid until next call)
 */
const char* json_escape_string(const char *input);

#endif // JSON_UTILS_H
