# Backend Round Handler Fix Plan

## Issue
Round handlers không follow đúng flow theo specification. Cần sửa để:
1. Check active players sau elimination
2. Trigger end_game đúng cách
3. Handle MODE_SCORING vs MODE_ELIMINATION correctly

## Flow cần implement (theo user requirement):

```
END ROUND
   ↓
CHECK DISCONNECTED
   ↓
ACTIVE PLAYERS < 2 ?
   ├─ YES → END GAME
   └─ NO
        ↓
MODE_SCORING ?
   ├─ YES → NEXT ROUND / END GAME
   └─ NO (ELIMINATION)
        ↓
CHECK LOWEST SCORE
   ├─ TIE ≥ 2 → BONUS ROUND
   └─ TIE = 1 → ELIMINATE
                    ↓
            ACTIVE PLAYERS < 2 ?
                ├─ YES → END GAME
                └─ NO → NEXT ROUND
```

## Files to fix:

### ✅ DONE: round1_handler.c
- Already has r1_finalize_round() following correct flow
- Fixed end_game_now() to use trigger_end_game()
- Added end_game_handler.h include

### 🔧 TODO: round2_handler.c
Location: advance_to_next_product() function (lines 910-955)

Current problems:
- No check for active players after elimination
- No call to trigger_end_game()
- Just advances round without checking game end conditions

Need to:
1. Extract finalization logic to r2_finalize_round() similar to round1
2. Add active player checks
3. Use trigger_end_game() for game ending
4. Add end_game_handler.h include

### 🔧 TODO: round3_handler.c  
Location: check_all_finished() function (around line 497-640)

Current: Already calls trigger_end_game() but need to verify flow consistency

## Implementation Steps:

1. Create r2_finalize_round() in round2_handler.c
2. Update advance_to_next_product() to call r2_finalize_round()
3. Verify round3_handler.c follows same flow
4. Test all scenarios:
   - Normal round progression
   - Elimination with active < 2
   - Bonus round triggered
   - MODE_SCORING vs MODE_ELIMINATION

## Expected behavior after fix:

When player is eliminated:
1. Server calls eliminate_player()
2. eliminate_player() sends NTF_ELIMINATION with player_id + room_id
3. eliminate_player() calls session_mark_lobby()
4. After all eliminations, check active players < 2
5. If yes, trigger_end_game() → sends proper end game notifications
6. Client receives NTF_ELIMINATION → navigates to room
7. Client may also receive end game notification
