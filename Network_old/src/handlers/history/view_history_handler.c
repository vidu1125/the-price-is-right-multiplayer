// Network/src/handlers/history/view_history_handler.c
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../../../include/handlers/history/view_history_handler.h"
#include "../../../include/protocol/protocol.h"

/**
 * Handle CMD_HIST - View Game History
 * 
 * This is a basic implementation that returns dummy data.
 * TODO: Connect to database or Backend service to fetch real history
 */
int handle_view_history(int client_fd, MessageHeader* header, const char* payload) {
    printf("\n[VIEW_HISTORY] Processing request\n");
    printf("[VIEW_HISTORY] Client seq_num: %u\n", header->seq_num);
    printf("[VIEW_HISTORY] Payload length: %u bytes\n", header->length);
    
    // For now, return dummy data
    // TODO: Query SQLite database or call Backend API
    
    char history_buffer[2048];
    int len = snprintf(history_buffer, sizeof(history_buffer),
        "=== GAME HISTORY ===\n"
        "\n"
        "Game #1:\n"
        "  Date: 2025-01-15 14:30:00\n"
        "  Winner: Player1\n"
        "  Score: 1000 points\n"
        "\n"
        "Game #2:\n"
        "  Date: 2025-01-15 15:45:00\n"
        "  Winner: Player2\n"
        "  Score: 1200 points\n"
        "\n"
        "Game #3:\n"
        "  Date: 2025-01-15 16:20:00\n"
        "  Winner: Player3\n"
        "  Score: 950 points\n"
        "\n"
        "Total games played: 3\n"
        "Last updated: %ld\n",
        time(NULL)
    );
    
    if (len < 0 || len >= (int)sizeof(history_buffer)) {
        fprintf(stderr, "[VIEW_HISTORY] Buffer error\n");
        send_error(client_fd, RES_ERROR, header->seq_num, "Internal error");
        return -1;
    }
    
    // Send success response
    int result = send_response(client_fd, RES_SUCCESS, header->seq_num, 
                               history_buffer, len);
    
    if (result == 0) {
        printf("[VIEW_HISTORY] Response sent successfully (%d bytes)\n", len);
    } else {
        fprintf(stderr, "[VIEW_HISTORY] Failed to send response\n");
    }
    
    return result;
}