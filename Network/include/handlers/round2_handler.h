#ifndef ROUND2_HANDLER_H
#define ROUND2_HANDLER_H

#include "../protocol/protocol.h"

/**
 * Round 2: The Bid
 * 
 * GAMEPLAY:
 * - Players see a product image
 * - Each player submits a price bid (exact number)
 * - 5 products, 15 seconds each
 * 
 * SCORING:
 * - 100 points: Closest bid WITHOUT going over the actual price
 * - 50 points: If ALL players overbid, closest overbid wins
 * - 0 points: All others
 * 
 * STATES USED:
 * - MatchPlayerState.score - READ/UPDATE
 * - MatchPlayerState.eliminated - READ (block eliminated players)
 * - UserSession.state - READ (check connection)
 * - RoundState - Own/Manage
 */

void handle_round2(
    int client_fd,
    MessageHeader *req_header,
    const char *payload
);

// Called when a client disconnects during Round 2
void handle_round2_disconnect(int client_fd);

#endif // ROUND2_HANDLER_H
