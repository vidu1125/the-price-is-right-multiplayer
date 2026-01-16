# ✅ CHECKLIST: Round 1 → Bonus Round Flow (2 người tie)

## 📊 BACKEND FLOW

### 1️⃣ **Round 1 kết thúc** (Câu hỏi cuối cùng)
- ✅ Player submit answer cuối cùng
- ✅ `handle_submit_answer()` gửi `OP_S2C_ROUND1_RESULT` với `correct_index`
- ✅ Nếu tất cả đã answer → gọi `advance_to_next_question()`

### 2️⃣ **advance_to_next_question()** 
```c
// Line 872-927
void advance_to_next_question(MessageHeader *req) {
    // ... check if more questions ...
    
    // END ROUND
    round->status = ROUND_ENDED;
    g_r1.is_active = false;
    
    bool match_ended = r1_finalize_round(req);  // ← GỌI FINALIZE
    
    if (!match_ended) {
        char *json = build_round_end_json();
        if (json) {
            // ✅ THÊM FLAG bonus_triggered nếu bonus active
            if (is_bonus_active(g_r1.match_id)) {
                cJSON *obj = cJSON_Parse(json);
                cJSON_AddBoolToObject(obj, "bonus_triggered", true);
                json = cJSON_PrintUnformatted(obj);
            }
            
            // ✅ LUÔN GỬI message này
            broadcast_json(req, OP_S2C_ROUND1_ALL_FINISHED, json);
        }
    }
}
```

### 3️⃣ **r1_finalize_round()** 
```c
// Line 418-476
static bool r1_finalize_round(MessageHeader *req) {
    // ... cleanup disconnected ...
    // ... check active players ...
    
    // MODE_ELIMINATION → check lowest score
    bool bonus_triggered = perform_elimination();  // ← GỌI PERFORM_ELIMINATION
    
    if (bonus_triggered) {
        return false;  // Bonus triggered - don't end match
    }
    
    // ... normal flow: advance to next round ...
}
```

### 4️⃣ **perform_elimination()** - Phát hiện TIE
```c
// Line 626-724
static bool perform_elimination(void) {
    // ... collect active players scores ...
    // ... sort by score ascending ...
    
    int lowest = scores[0].score;
    int tie_count = 0;
    for (int i = 0; i < count; i++) {
        if (scores[i].score == lowest) tie_count++;
    }
    
    // ✅ RULE: >=2 tied at lowest → BONUS ROUND
    if (tie_count >= 2) {
        printf("[Round1] %d players tied at lowest (%d), triggering bonus round\n", 
               tie_count, lowest);
        
        // Collect tied players
        int32_t tied_players[MAX_MATCH_PLAYERS];
        int tied_count = 0;
        for (int i = 0; i < count && tied_count < MAX_MATCH_PLAYERS; i++) {
            if (scores[i].score == lowest) {
                tied_players[tied_count++] = scores[i].account_id;
            }
        }
        
        // ✅ TRIGGER BONUS
        check_and_trigger_bonus(match->runtime_match_id, 1);
        return true;  // Bonus triggered
    }
    
    // ... normal elimination ...
}
```

### 5️⃣ **check_and_trigger_bonus()** - Khởi tạo Bonus
```c
// bonus_handler.c line 497-653
bool check_and_trigger_bonus(uint32_t match_id, int after_round) {
    // ... collect tied players ...
    // ... determine bonus type (ELIMINATION for round 1) ...
    
    // ✅ Initialize bonus context
    initialize_bonus(match_id, after_round, BONUS_TYPE_ELIMINATION, 
                    tied_players, tied_count);
    
    // ✅ GỬI NOTIFICATIONS
    notify_participants(&dummy_hdr);   // → OP_S2C_BONUS_PARTICIPANT (0x0671)
    notify_spectators(&dummy_hdr);     // → OP_S2C_BONUS_SPECTATOR (0x0672)
    
    return true;
}
```

## 📱 CLIENT FLOW

### 1️⃣ **Nhận OP_S2C_ROUND1_RESULT** (0x0614)
```dart
case GameEventType.round1Result:
  // ✅ Hiển thị đúng/sai (màu xanh/đỏ)
  final bool isCorrect = result?['is_correct'] == true;
  final int correctIndex = result?['correct_index'];
  
  // ✅ Update _currentQuestion với correctIndex
  _currentQuestion['correctIndex'] = correctIndex;
  
  // ✅ Show SnackBar
  ScaffoldMessenger.of(context).showSnackBar(
    SnackBar(
      content: Text(isCorrect ? "Correct!" : "Wrong!"),
      backgroundColor: isCorrect ? Colors.green : Colors.red,
    ),
  );
```

### 2️⃣ **Nhận OP_S2C_ROUND1_ALL_FINISHED** (0x0619)
```dart
case GameEventType.round1AllFinished:
  final data = event.data;
  
  // ✅ Update leaderboard
  _updateLeaderboard(data['players']);
  
  // ✅ CHECK BONUS FLAG
  final bool bonusTriggered = data?['bonus_triggered'] == true;
  if (bonusTriggered) {
    print("[GameContainer] Bonus round triggered - waiting for bonus notifications");
    break;  // ← KHÔNG advance, đợi bonus notifications
  }
  
  // Normal flow: advance to next round
  final nextRound = data?['next_round'] ?? 0;
  if (nextRound > 0) {
    Future.delayed(Duration(seconds: 3), () {
      setState(() {
        _currentRound = nextRound;
        // Send ready for next round...
      });
    });
  }
```

### 3️⃣ **Nhận OP_S2C_BONUS_PARTICIPANT** (0x0671) hoặc **OP_S2C_BONUS_SPECTATOR** (0x0672)
```dart
case GameEventType.bonusStart:
  print("[GameContainer] BONUS ROUND STARTING!");
  
  // ✅ Switch to bonus round
  _inBonusRound = true;
  _isLoading = false;
  _showingResult = false;
  
  // ✅ RoundBonusWidget sẽ được render
  // Widget sẽ nhận initialData từ event.data
```

### 4️⃣ **RoundBonusWidget** được hiển thị
```dart
// round_bonus_widget.dart
void initState() {
  super.initState();
  
  // ✅ Apply initial data từ OP_S2C_BONUS_PARTICIPANT/SPECTATOR
  if (widget.initialData != null) {
    _applyInitialData(widget.initialData!);
  }
  
  // ✅ Listen to bonus events
  _eventSub = ServiceLocator.bonusService.events.listen(_handleBonusEvent);
  
  // ✅ Send ready
  ServiceLocator.bonusService.sendBonusReady(widget.matchId);
}
```

## 🔄 MESSAGES TIMELINE

```
TIME | SENDER  | MESSAGE                          | OPCODE | CONTENT
-----|---------|----------------------------------|--------|----------------------------------
T+0  | Backend | OP_S2C_ROUND1_RESULT            | 0x0614 | {is_correct, correct_index, ...}
T+1  | Backend | OP_S2C_ROUND1_ALL_FINISHED      | 0x0619 | {players, bonus_triggered: true}
T+2  | Backend | OP_S2C_BONUS_PARTICIPANT        | 0x0671 | {role, bonus_type, participants}
T+2  | Backend | OP_S2C_BONUS_SPECTATOR          | 0x0672 | {role, bonus_type, participants}
T+3  | Client  | OP_C2S_BONUS_READY              | 0x0670 | {match_id}
```

## ✅ VERIFICATION CHECKLIST

- [x] Backend: `advance_to_next_question()` LUÔN gửi `OP_S2C_ROUND1_ALL_FINISHED`
- [x] Backend: Thêm field `bonus_triggered: true` khi bonus active
- [x] Backend: `perform_elimination()` phát hiện tie (>=2 players cùng điểm thấp)
- [x] Backend: `check_and_trigger_bonus()` gửi `OP_S2C_BONUS_PARTICIPANT`/`SPECTATOR`
- [x] Client: Hiển thị đúng/sai (màu xanh/đỏ) khi nhận `OP_S2C_ROUND1_RESULT`
- [x] Client: Check `bonus_triggered` flag trong `round1AllFinished` event
- [x] Client: KHÔNG auto-advance khi `bonus_triggered == true`
- [x] Client: Chuyển sang `RoundBonusWidget` khi nhận bonus notifications
- [x] Client: `RoundBonusWidget` nhận initial data và hiển thị UI

## 🐛 POTENTIAL ISSUES

### Issue 1: Client không nhận được bonus notifications
**Symptom**: Client bị treo sau khi nhận `OP_S2C_ROUND1_ALL_FINISHED`
**Fix**: Đã thêm `bonus_triggered` flag để client biết đợi

### Issue 2: Double-advance (câu hỏi bị skip)
**Symptom**: Câu hỏi mới được gửi 2 lần
**Fix**: Đã xóa auto-advance logic trong `handle_round1_disconnect()`

### Issue 3: Không hiển thị màu đúng/sai
**Symptom**: Client không thấy màu xanh/đỏ khi submit answer
**Fix**: `Round1Widget` sử dụng `correctIndex` từ `_currentQuestion`

## 🎯 EXPECTED BEHAVIOR

1. ✅ Player submit answer → thấy màu xanh (đúng) hoặc đỏ (sai) NGAY LẬP TỨC
2. ✅ Tất cả players answer → Round 1 kết thúc
3. ✅ Client nhận `OP_S2C_ROUND1_ALL_FINISHED` với `bonus_triggered: true`
4. ✅ Client KHÔNG tự động chuyển sang Round 2
5. ✅ Client nhận `OP_S2C_BONUS_PARTICIPANT` hoặc `OP_S2C_BONUS_SPECTATOR`
6. ✅ Client hiển thị `RoundBonusWidget` với card drawing UI
7. ✅ Players draw cards → reveal → eliminate 1 player
8. ✅ Transition to Round 2 với remaining players
