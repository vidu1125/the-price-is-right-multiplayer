#ifndef HISTORY_HANDLER_H
#define HISTORY_HANDLER_H

#include "protocol/protocol.h"


typedef struct PACKED {
    uint8_t limit;     
    uint8_t reserved; 
} HistRequestPayload;

typedef struct PACKED {
    uint32_t match_id;      
    uint8_t  player_count;  
    uint8_t  is_winner;      
    int32_t  final_score;    
    uint64_t ended_at;      
} HistoryListItem;

typedef struct {
    int match_id;
    int score;
    int ranking;
} HistoryRecord;

typedef struct PACKED {
    uint8_t count;              
    uint8_t reserved;           
    HistoryRecord records[];  
} HistResponsePayload;

typedef struct PACKED {
    uint32_t match_id;   // trận cần xem chi tiết
} MatchDetailRequestPayload;

typedef struct PACKED {
    uint32_t match_id;
    uint8_t  mode;        
    uint8_t  max_players;
    uint16_t reserved;
    uint64_t started_at;
    uint64_t ended_at;
} MatchDetailHeader;

typedef struct PACKED {
    uint32_t player_id;    // match_players.id
    int32_t  score;
    uint8_t  eliminated;
    uint8_t  forfeited;
    uint8_t  winner;
} MatchPlayerInfo;

typedef struct PACKED {
    uint32_t question_id;  // match_question.id
    uint8_t  round_no;
    uint8_t  round_type;   // enum: QUIZ/BID/WHEEL
    uint16_t question_len;
    char     question_data[]; // JSON UTF-8 (chỉ phần question)
} MatchQuestionInfo;


typedef struct PACKED {
    uint32_t question_id;
    uint32_t player_id;
    int32_t  score_delta;
    uint8_t  action_idx;
    uint16_t answer_len;
    char     answer_data[];  // JSON UTF-8
} MatchAnswerInfo;


void handle_history(
    int client_fd,
    MessageHeader *req_header,
    const char *payload,
    int32_t account_id
);

#endif
