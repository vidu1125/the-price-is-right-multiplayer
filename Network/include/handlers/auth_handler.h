#ifndef AUTH_HANDLER_H
#define AUTH_HANDLER_H

#include "protocol/protocol.h"

/**
 * Authentication Handlers
 * 
 * These handlers process authentication-related commands:
 * - CMD_LOGIN_REQ (0x0100): User login with email/password
 * - CMD_REGISTER_REQ (0x0101): New user registration
 * - CMD_LOGOUT_REQ (0x0102): User logout and session cleanup
 * - CMD_RECONNECT (0x0103): Reconnect with existing session
 */

/**
 * Handle user login request
 * 
 * Expected JSON payload:
 * {
 *   "email": "user@example.com",
 *   "password": "password123"
 * }
 * 
 * Success response (RES_LOGIN_OK):
 * {
 *   "success": true,
 *   "account_id": 123,
 *   "session_id": "uuid-string",
 *   "profile": {
 *     "name": "Username",
 *     "avatar": "url",
 *     "points": 1000
 *   }
 * }
 * 
 * Error responses:
 * - ERR_BAD_REQUEST: Invalid JSON or missing fields
 * - ERR_INVALID_USERNAME: Invalid credentials
 */
void handle_login(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

/**
 * Handle user registration request
 * 
 * Expected JSON payload:
 * {
 *   "email": "user@example.com",
 *   "password": "password123",
 *   "name": "Display Name"
 * }
 * 
 * Success response (RES_SUCCESS):
 * {
 *   "success": true,
 *   "account_id": 123,
 *   "message": "Registration successful"
 * }
 * 
 * Error responses:
 * - ERR_BAD_REQUEST: Invalid JSON, missing fields, or invalid email format
 * - ERR_INVALID_USERNAME: Email already exists
 */
void handle_register(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

/**
 * Handle user logout request
 * 
 * Expected JSON payload:
 * {
 *   "session_id": "uuid-string"
 * }
 * 
 * Success response (RES_SUCCESS):
 * {
 *   "success": true,
 *   "message": "Logged out successfully"
 * }
 * 
 * This will:
 * - Mark session as disconnected
 * - Remove player from any active rooms
 * - Clean up server-side state
 */
void handle_logout(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

/**
 * Handle reconnect request with existing session
 * 
 * Expected JSON payload:
 * {
 *   "session_id": "uuid-string"
 * }
 * 
 * Success response (RES_LOGIN_OK):
 * {
 *   "success": true,
 *   "account_id": 123,
 *   "session_id": "uuid-string",
 *   "room_state": {...}  // if player was in a room
 * }
 * 
 * Error responses:
 * - ERR_BAD_REQUEST: Invalid session_id
 * - ERR_NOT_LOGGED_IN: Session not found or expired
 */
void handle_reconnect(
    int client_fd,
    MessageHeader *header,
    const char *payload
);

#endif // AUTH_HANDLER_H