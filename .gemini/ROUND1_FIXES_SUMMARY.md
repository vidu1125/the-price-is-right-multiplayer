# Round 1 Fixes Summary

## 🎯 Objectives

1. **Fix Race Condition**: Ensure bonus round triggers correctly without client getting stuck
2. **Fix Scoring System**: Implement time-based scoring formula

---

## 🔧 Fix 1: Race Condition (Bonus Round Trigger)

### Problem
When Round 1 ends with tied players:
- Backend triggers bonus round
- Client doesn't receive `OP_S2C_ROUND1_ALL_FINISHED` message
- Client gets stuck waiting

### Solution
Modified `r1_finalize_round()` to return bonus_triggered flag via output parameter, eliminating race condition with `is_bonus_active()` check.

### Backend Changes

**File**: `/app/backend/Network/src/handlers/round1_handler.c`

#### Change 1: Update `r1_finalize_round()` signature
```c
// Line 418 - OLD:
static bool r1_finalize_round(MessageHeader *req);

// NEW:
static bool r1_finalize_round(MessageHeader *req, bool *out_bonus_triggered);
```

#### Change 2: Initialize and set output parameter
```c
// Line 422 - ADD after match_id declaration:
// Initialize output parameter
if (out_bonus_triggered) *out_bonus_triggered = false;

// Line 451-456 - MODIFY:
bool bonus_triggered = perform_elimination();
if (bonus_triggered) {
    // BONUS ROUND: bonus handler sẽ tự điều hướng
    if (out_bonus_triggered) *out_bonus_triggered = true;
    printf("[Round1] Bonus triggered, setting output flag\n");
    return false;
}
```

#### Change 3: Use bonus_triggered flag in `advance_to_next_question()`
```c
// Line 901-939 - REPLACE entire block:
bool bonus_triggered = false;
bool match_ended = r1_finalize_round(req, &bonus_triggered);

printf("[Round1] After finalize: match_ended=%d, bonus_triggered=%d\n", match_ended, bonus_triggered);

if (!match_ended) {
    printf("[Round1] Building round end JSON...\n");
    char *json = build_round_end_json();
    if (json) {
        // Add bonus_triggered flag if bonus was triggered
        if (bonus_triggered) {
            printf("[Round1] Bonus round triggered - adding bonus_triggered flag to summary\n");
            
            // Parse JSON, add bonus_triggered flag, then re-serialize
            cJSON *obj = cJSON_Parse(json);
            if (obj) {
                cJSON_AddBoolToObject(obj, "bonus_triggered", true);
                free(json);
                json = cJSON_PrintUnformatted(obj);
                cJSON_Delete(obj);
                printf("[Round1] Updated JSON with bonus_triggered flag\n");
            }
        }
        
        printf("[Round1] Broadcasting OP_S2C_ROUND1_ALL_FINISHED: %s\n", json);
        broadcast_json(req, OP_S2C_ROUND1_ALL_FINISHED, json);
        free(json);
    } else {
        printf("[Round1] ERROR: Failed to build round end JSON!\n");
    }
} else {
    printf("[Round1] Match ended, not sending ALL_FINISHED\n");
}
```

### Client Changes

**File**: `/app/client/flutter_app/lib/ui/screens/game_container_screen.dart`

#### Add bonus_triggered check (Line 187-194)
```dart
// Check if bonus round is triggered
final bool bonusTriggered = data?['bonus_triggered'] == true;
if (bonusTriggered) {
  print("[GameContainer] Bonus round triggered - waiting for bonus notifications");
  // Don't advance to next round - bonus handler will send notifications
  break;
}
```

---

## 🔧 Fix 2: Time-Based Scoring System

### Formula
```
Score = 200 − (Answer Time / 15) × 100
Final score is rounded down to the nearest ten
```

### Examples
- Answer at 0s: 200 - (0/15)*100 = 200 points
- Answer at 5s: 200 - (5/15)*100 = 166.67 → 160 points
- Answer at 10s: 200 - (10/15)*100 = 133.33 → 130 points
- Answer at 15s: 200 - (15/15)*100 = 100 points

### Backend Changes

**File**: `/app/backend/Network/src/handlers/round1_handler.c`

#### Update scoring calculation (Line ~1250)
```c
// OLD:
int delta = correct ? 200 : 0;

// NEW:
int delta = 0;
if (correct) {
    // Time-based scoring: Score = 200 - (time_ms / 15000) * 100
    // Round down to nearest 10
    float time_penalty = ((float)time_ms / 15000.0f) * 100.0f;
    float raw_score = 200.0f - time_penalty;
    delta = ((int)raw_score / 10) * 10;  // Round down to nearest 10
    
    // Ensure minimum score of 100 for correct answers
    if (delta < 100) delta = 100;
    
    printf("[Round1] Time-based scoring: time=%dms, penalty=%.2f, raw=%.2f, final=%d\n",
           time_ms, time_penalty, raw_score, delta);
}
```

### Client Changes

**File**: `/app/client/flutter_app/lib/ui/screens/game_container_screen.dart`

#### Restore time calculation (Line ~654)
```dart
void _handleRound1AnswerSelected(int choiceIndex) {
  if (_currentQuestion == null) return;
  
  int questionIndex = _currentQuestion?['question_idx'] ?? 0;
  
  // Calculate time taken (Total time 15s - Remaining)
  int timeTakenMs = (15 - _timeRemaining) * 1000;
  // Safety check
  if (timeTakenMs < 0) timeTakenMs = 0;
  if (timeTakenMs > 15000) timeTakenMs = 15000;
  
  print("[GameContainer] Answer selected: question=$questionIndex, choice=$choiceIndex, time=${timeTakenMs}ms");
  
  ServiceLocator.gameStateService.submitRound1Answer(
    _matchId,
    questionIndex,
    choiceIndex,
    timeTakenMs: timeTakenMs,  // RESTORE THIS PARAMETER
  );
  
  _countdownTimer?.cancel();
}
```

---

## 📋 Rebuild Instructions

### 1. Stop all running services
```bash
# Stop all flutter instances
pkill -f "flutter run"

# Stop docker-compose
cd /home/thowo/networkprog/the-price-is-right-multiplayer/app/backend
docker-compose down
```

### 2. Rebuild backend with no cache
```bash
cd /home/thowo/networkprog/the-price-is-right-multiplayer/app/backend
docker-compose build --no-cache network
docker-compose up
```

### 3. Run client
```bash
cd /home/thowo/networkprog/the-price-is-right-multiplayer/app/client/flutter_app
flutter run
```

---

## ✅ Verification Checklist

### Backend Logs Should Show:
```
[Round1] All questions completed
[Round1] After finalize: match_ended=0, bonus_triggered=1
[Round1] Building round end JSON...
[Round1] Bonus round triggered - adding bonus_triggered flag to summary
[Round1] Updated JSON with bonus_triggered flag
[Round1] Broadcasting OP_S2C_ROUND1_ALL_FINISHED: {...,"bonus_triggered":true}
```

### Client Logs Should Show:
```
[GameContainer] Round 1 finished: {...}
[GameContainer] Bonus round triggered - waiting for bonus notifications
[GameContainer] BONUS ROUND STARTING!
```

### Scoring Should Show:
```
[Round1] Time-based scoring: time=5000ms, penalty=33.33, raw=166.67, final=160
```

---

## 🐛 Common Issues

### Issue: Backend not rebuilding
**Solution**: Use `docker-compose build --no-cache network`

### Issue: Client still sending time=0ms
**Solution**: Check that `timeTakenMs` parameter is restored in `submitRound1Answer()` call

### Issue: Still getting stuck at last question
**Solution**: Verify backend logs show the new debug messages. If not, backend wasn't rebuilt.

---

## 📊 Expected Flow

1. Players answer all 5 questions
2. Backend detects tie at lowest score
3. Backend sends `OP_S2C_ROUND1_ALL_FINISHED` with `bonus_triggered: true`
4. Client receives message, sees flag, DOES NOT auto-advance
5. Backend sends `OP_S2C_BONUS_PARTICIPANT`/`SPECTATOR`
6. Client transitions to `RoundBonusWidget`
7. Bonus round proceeds normally
