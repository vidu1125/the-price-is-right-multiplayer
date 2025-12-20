#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

#include "../../include/message_handler.h"
#include "../../include/protocol.h"
#include "../../include/server.h"
#include "../../include/room_manager.h"

//==============================================================================
// START GAME HANDLER
//==============================================================================

void handle_start_game(int client_idx) {
    ClientConnection *client = &clients[client_idx];
    
    if (client->room_id < 0 || client->room_id >= MAX_ROOMS) {
        send_response(client->sockfd, ERR_NOT_HOST, "Not in room", 11);
        return;
    }
    
    GameRoom *room = &rooms[client->room_id];
    
    if (room->host_client_idx != client_idx) {
        send_response(client->sockfd, ERR_NOT_HOST, "Not room host", 13);
        return;
    }
    
    // Check all players ready
    for (int i = 0; i < room->player_count; i++) {
        if (!room->players[i].is_ready) {
            send_response(client->sockfd, ERR_BAD_REQUEST, "Not all players ready", 21);
            return;
        }
    }
    
    room->is_playing = 1;
    room->current_round = ROUND_QUIZ;
    room->round_start_time = time(NULL);
    room->round_time_limit = 30;
    
    // Update all players to PLAYING state
    for (int i = 0; i < room->player_count; i++) {
        clients[room->players[i].client_idx].state = STATE_PLAYING;
    }
    
    // Broadcast round start
    char round_data[256];
    snprintf(round_data, sizeof(round_data), 
            "{\"round\":1,\"type\":\"quiz\",\"time\":30,\"question\":\"Sample question?\"}");
    
    broadcast_to_room(client->room_id, NTF_ROUND_START, round_data, strlen(round_data));
    
    send_response(client->sockfd, RES_SUCCESS, "Game started", 12);
    printf("Client %d started game in room %u\n", client_idx, room->room_id);
}

//==============================================================================
// ANSWER QUIZ HANDLER
//==============================================================================

void handle_answer_quiz(int client_idx, char *payload, uint32_t length) {
    ClientConnection *client = &clients[client_idx];
    
    if (client->room_id < 0 || client->room_id >= MAX_ROOMS) {
        send_response(client->sockfd, ERR_BAD_REQUEST, "Not in game", 11);
        return;
    }
    
    if (length < sizeof(QuizAnswerPayload)) {
        send_response(client->sockfd, ERR_BAD_REQUEST, "Invalid payload", 15);
        return;
    }
    
    GameRoom *room = &rooms[client->room_id];
    QuizAnswerPayload *answer = (QuizAnswerPayload *)payload;
    
    // Find player and record answer
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i].client_idx == client_idx) {
            room->players[i].has_answered = 1;
            room->players[i].answer_data = answer->answer_index;
            
            // Simple scoring (assuming correct answer is 0)
            if (answer->answer_index == 0) {
                room->players[i].score += 100;
            }
            
            send_response(client->sockfd, RES_SUCCESS, "Answer recorded", 15);
            printf("Client %d submitted answer: %d\n", client_idx, answer->answer_index);
            
            // Check if all answered
            int all_answered = 1;
            for (int j = 0; j < room->player_count; j++) {
                if (!room->players[j].has_answered) {
                    all_answered = 0;
                    break;
                }
            }
            
            if (all_answered) {
                // Build scoreboard
                char scoreboard[512] = "{\"scores\":[";
                for (int j = 0; j < room->player_count; j++) {
                    char entry[64];
                    snprintf(entry, sizeof(entry), 
                            "{\"name\":\"%s\",\"score\":%u}%s",
                            room->players[j].display_name,
                            room->players[j].score,
                            (j < room->player_count - 1) ? "," : "");
                    strcat(scoreboard, entry);
                }
                strcat(scoreboard, "]}");
                
                broadcast_to_room(client->room_id, NTF_SCOREBOARD, scoreboard, strlen(scoreboard));
            }
            
            return;
        }
    }
}

//==============================================================================
// BID HANDLER
//==============================================================================

void handle_bid(int client_idx, char *payload, uint32_t length) {
    ClientConnection *client = &clients[client_idx];
    
    if (client->room_id < 0 || client->room_id >= MAX_ROOMS) {
        send_response(client->sockfd, ERR_BAD_REQUEST, "Not in game", 11);
        return;
    }
    
    if (length < sizeof(BidPayload)) {
        send_response(client->sockfd, ERR_BAD_REQUEST, "Invalid payload", 15);
        return;
    }
    
    GameRoom *room = &rooms[client->room_id];
    BidPayload *bid = (BidPayload *)payload;
    uint32_t bid_price = ntohl(bid->bid_price);
    
    // Record bid
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i].client_idx == client_idx) {
            room->players[i].answer_data = bid_price;
            room->players[i].has_answered = 1;
            
            send_response(client->sockfd, RES_SUCCESS, "Bid placed", 10);
            printf("Client %d placed bid: %u\n", client_idx, bid_price);
            return;
        }
    }
}

//==============================================================================
// SPIN HANDLER
//==============================================================================

void handle_spin(int client_idx) {
    ClientConnection *client = &clients[client_idx];
    
    if (client->room_id < 0 || client->room_id >= MAX_ROOMS) {
        send_response(client->sockfd, ERR_BAD_REQUEST, "Not in game", 11);
        return;
    }
    
    // Generate random result
    int result = (rand() % 1000) - 200; // -200 to 800
    
    char response[64];
    snprintf(response, sizeof(response), "Spin result: %d", result);
    
    // Update score
    GameRoom *room = &rooms[client->room_id];
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i].client_idx == client_idx) {
            if (result >= 0) {
                room->players[i].score += result;
            } else if (room->players[i].score >= (uint32_t)(-result)) {
                room->players[i].score += result;
            } else {
                room->players[i].score = 0;
            }
            break;
        }
    }
    
    send_response(client->sockfd, RES_SUCCESS, response, strlen(response));
    printf("Client %d spun wheel: %d\n", client_idx, result);
}

//==============================================================================
// FORFEIT HANDLER
//==============================================================================

void handle_forfeit(int client_idx) {
    ClientConnection *client = &clients[client_idx];
    
    if (client->room_id < 0 || client->room_id >= MAX_ROOMS) {
        send_response(client->sockfd, ERR_BAD_REQUEST, "Not in game", 11);
        return;
    }
    
    GameRoom *room = &rooms[client->room_id];
    
    for (int i = 0; i < room->player_count; i++) {
        if (room->players[i].client_idx == client_idx) {
            room->players[i].is_eliminated = 1;
            
            char msg[64];
            snprintf(msg, sizeof(msg), "%s forfeited", client->display_name);
            broadcast_to_room(client->room_id, NTF_ELIMINATION, msg, strlen(msg));
            
            send_response(client->sockfd, RES_SUCCESS, "Forfeited", 9);
            printf("Client %d forfeited\n", client_idx);
            return;
        }
    }
}