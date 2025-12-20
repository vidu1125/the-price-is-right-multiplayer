#include <time.h>
#include "../../include/message_handler.h"
#include "../../include/protocol.h"
#include "../../include/server.h"

//==============================================================================
// HEARTBEAT HANDLER
//==============================================================================

void handle_heartbeat(int client_idx) {
    ClientConnection *client = &clients[client_idx];
    client->last_heartbeat = time(NULL);
    send_response(client->sockfd, RES_HEARTBEAT_OK, NULL, 0);
}