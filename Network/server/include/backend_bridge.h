// Network/server/include/backend_bridge.h
#ifndef BACKEND_BRIDGE_H
#define BACKEND_BRIDGE_H

#include <stdbool.h>
#include <stdint.h>

// Initialize connection to backend IPC server
bool backend_bridge_init(const char* socket_path);

// Send JSON request to backend and receive response
char* backend_bridge_send(const char* json_request);

// Helper: Build JSON request
char* backend_bridge_build_request(const char* action, uint32_t user_id, const char* data_json);

// Cleanup
void backend_bridge_cleanup();

#endif // BACKEND_BRIDGE_H
