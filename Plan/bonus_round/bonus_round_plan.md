# Bonus Round Implementation Plan

## 1. ĐỊNH NGHĨA BONUS ROUND

### 1.1. Mục đích
- Bonus Round là vòng chơi phụ được kích hoạt tự động khi có từ 2 người chơi trở lên cùng điểm số
- Sử dụng cơ chế rút thăm ngẫu nhiên để tìm ra đúng 1 người bị loại hoặc 1 người chiến thắng
- Không phải là round chính, chỉ là bước phân định kết quả

### 1.2. Điều kiện kích hoạt

| Case | Mode | Timing | Condition | Type |
|------|------|--------|-----------|------|
| Round 1/2 | ELIMINATION | Sau scoring | >= 2 người có điểm thấp nhất | `ELIMINATION` |
| Round 3 | ELIMINATION | Sau scoring | >= 2 người sống có điểm cao nhất | `WINNER_SELECTION` |
| Round 3 | SCORING | Sau scoring | >= 2 người có điểm cao nhất | `WINNER_SELECTION` |

## 2. OPCODES

```c
// Bonus Round - Tiebreaker (0x0660 - 0x067F)
#define OP_C2S_BONUS_READY          0x0660  // Client ready for bonus round
#define OP_C2S_BONUS_DRAW_CARD      0x0661  // Player draws a card

#define OP_S2C_BONUS_INIT           0x0670  // Server initializes bonus round
#define OP_S2C_BONUS_PARTICIPANT    0x0671  // Notify: you are a participant
#define OP_S2C_BONUS_SPECTATOR      0x0672  // Notify: you are a spectator
#define OP_S2C_BONUS_CARD_DRAWN     0x0673  // Card drawn confirmation
#define OP_S2C_BONUS_PLAYER_DREW    0x0674  // Broadcast: player drew card
#define OP_S2C_BONUS_REVEAL         0x0675  // Reveal all cards
#define OP_S2C_BONUS_RESULT         0x0676  // Final result
#define OP_S2C_BONUS_TRANSITION     0x0677  // Transition to next phase
```

## 3. STATE DEFINITIONS

### 3.1. BonusState
```c
typedef enum {
    BONUS_STATE_NONE = 0,
    BONUS_STATE_INITIALIZING,    // Server creating cards
    BONUS_STATE_DRAWING,         // Players drawing cards
    BONUS_STATE_REVEALING,       // Revealing results
    BONUS_STATE_APPLYING,        // Applying results
    BONUS_STATE_COMPLETED        // Done, transitioning
} BonusState;
```

### 3.2. PlayerBonusState
```c
typedef enum {
    PLAYER_BONUS_WAITING_TO_DRAW,
    PLAYER_BONUS_CARD_DRAWN,
    PLAYER_BONUS_REVEALED_SAFE,
    PLAYER_BONUS_REVEALED_ELIMINATED,
    PLAYER_BONUS_SPECTATING
} PlayerBonusState;
```

## 4. FLOW

### 4.1. Detection Flow
```
Round Scoring Complete
        ↓
Check match mode
        ↓
    ┌───┴───┐
    ↓       ↓
ELIMINATION SCORING
    ↓       ↓
R1/R2: Find lowest    R3: Find highest
R3: Find highest      
    ↓       ↓
Count tied players
    ↓
>= 2 tied? → TRIGGER BONUS
```

### 4.2. Bonus Round Flow
```
1. INITIALIZATION
   - Server identifies tied players (participants)
   - Server identifies other players (spectators)
   - Create N cards: 1 ELIMINATED, N-1 SAFE
   - Shuffle cards
   - Notify all players

2. DRAWING PHASE
   - Each participant clicks DRAW
   - Server pops card from stack, assigns to player
   - Card NOT revealed yet
   - Broadcast that player drew

3. REVEAL PHASE (after all drew + 2s delay)
   - Flip all cards simultaneously
   - Show who got ELIMINATED vs SAFE
   - Display result message

4. APPLY RESULTS
   - If ELIMINATION type: eliminate the player
   - If WINNER_SELECTION: mark winner
   - Save to database

5. TRANSITION
   - If after R1/R2: go to next round
   - If after R3: end match
```

## 5. FILES CREATED/MODIFIED

### Network (Backend)
- `include/protocol/opcode.h` - Added bonus opcodes
- `include/handlers/bonus_handler.h` - NEW: Header file
- `src/handlers/bonus_handler.c` - NEW: Implementation
- `src/handlers/dispatcher.c` - Added bonus routing

### Database
- `Database/init/01_schema.sql` - Added bonus_rounds, bonus_draws tables

### Frontend
- `src/services/bonusService.js` - NEW: Service handlers
- `src/components/Game/Round/RoundBonus.js` - UPDATED: Full implementation
- `src/components/Game/Round/RoundBonus.css` - UPDATED: Styles
- `src/components/Game/GameContainer.js` - Added bonus event listeners

## 6. API USAGE

### 6.1. Trigger Bonus (Called by round handlers)
```c
// After round scoring is complete:
bool triggered = check_and_trigger_bonus(match_id, round_number);
if (triggered) {
    // Bonus round started, don't proceed to next round yet
    return;
}
```

### 6.2. Frontend Events
```javascript
// Listen for bonus trigger
window.addEventListener('bonusParticipant', (e) => {
    // You are in the bonus round - show DRAW button
});

window.addEventListener('bonusSpectator', (e) => {
    // You are watching - show spectator UI
});

// Draw a card
import { drawCard } from '../services/bonusService';
drawCard(matchId);

// Listen for results
window.addEventListener('bonusReveal', (e) => {
    // Show card flip animation
});

window.addEventListener('bonusTransition', (e) => {
    // Move to next round or end match
});
```

## 7. DATABASE SCHEMA

### bonus_rounds
| Column | Type | Description |
|--------|------|-------------|
| id | SERIAL | Primary key |
| match_id | INT | FK to matches |
| after_round | INT | Round that triggered bonus (1,2,3) |
| bonus_type | VARCHAR(20) | 'elimination' or 'winner_selection' |
| status | VARCHAR(20) | 'active', 'completed', 'cancelled' |
| participant_ids | INT[] | Array of tied player IDs |
| eliminated_player_id | INT | Result: who was eliminated |
| winner_player_id | INT | Result: who won |
| created_at | TIMESTAMP | When bonus started |
| completed_at | TIMESTAMP | When bonus ended |

### bonus_draws
| Column | Type | Description |
|--------|------|-------------|
| id | SERIAL | Primary key |
| bonus_id | INT | FK to bonus_rounds |
| player_id | INT | FK to accounts |
| card_type | VARCHAR(20) | 'safe' or 'eliminated' |
| draw_order | INT | Order in which player drew |
| drawn_at | TIMESTAMP | When card was drawn |
| revealed | BOOLEAN | Has card been revealed |
| revealed_at | TIMESTAMP | When card was revealed |

## 8. INTEGRATION POINTS

### Round handlers need to call:
```c
// At end of round scoring (round1_handler.c, round2_handler.c, round3_handler.c):
#include "handlers/bonus_handler.h"

// After calculating scores and before moving to next round:
if (check_and_trigger_bonus(match_id, round_number)) {
    // Bonus triggered, wait for bonus to complete
    return;
}
// Continue to next round...
```

## 9. TESTING

### Test Cases:
1. Round 1 - 2 players tied at lowest → bonus eliminates 1
2. Round 2 - 3 players tied at lowest → bonus eliminates 1
3. Round 3 (Elimination) - 2 players tied at highest → bonus picks winner
4. Round 3 (Scoring) - 4 players tied at highest → bonus picks winner
5. Player disconnects during bonus → auto-draw for them
6. All safe cards drawn except last → last player gets eliminated card

### Manual Testing:
```bash
# Start services
docker-compose up -d

# Check logs
docker-compose logs -f network

# Look for:
# [Bonus] Checking for ties: match=X after_round=Y mode=Z
# [Bonus] TRIGGERING BONUS ROUND for N tied players
# [Bonus] Player X drew card: SAFE/ELIMINATED
# [Bonus] Revealing results...
# [Bonus] Player X ELIMINATED via bonus round
```
