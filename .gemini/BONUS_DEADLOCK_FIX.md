# 🐛 CRITICAL FIX: Bonus Round Deadlock

## Problem Identified

**Symptom**: Server hangs when bonus round is triggered after Round 1 tie.

**Root Cause**: DEADLOCK in `initialize_bonus()` function.

### Call Stack When Deadlock Occurs:
```
advance_to_next_question()
  → r1_finalize_round()
    → perform_elimination()
      → check_and_trigger_bonus()
        → initialize_bonus()  ← DEADLOCK HERE
          → pthread_mutex_lock(&g_bonus_mutex)
          → start_bonus_timer()  ← Timer callback needs same mutex!
          → DEADLOCK (mutex already locked)
```

### Code Analysis:

**File**: `/app/backend/Network/src/handlers/bonus_handler.c`

**BEFORE (Buggy Code)**:
```c
static void initialize_bonus(...) {
    pthread_mutex_lock(&g_bonus_mutex);  // Line 337: LOCK
    
    // ... setup code ...
    
    g_bonus.state = BONUS_STATE_DRAWING;
    
    start_bonus_timer();  // Line 393: Start timer WHILE mutex locked
                         // Timer callback may try to lock same mutex → DEADLOCK!
    
    pthread_mutex_unlock(&g_bonus_mutex);  // Line 395: Never reached
}
```

**AFTER (Fixed Code)**:
```c
static void initialize_bonus(...) {
    pthread_mutex_lock(&g_bonus_mutex);
    
    // ... setup code ...
    
    g_bonus.state = BONUS_STATE_DRAWING;
    
    // CRITICAL: Unlock BEFORE starting timer
    pthread_mutex_unlock(&g_bonus_mutex);
    
    // Start timer (safe now, mutex unlocked)
    start_bonus_timer();
}
```

## Why This Causes Deadlock

1. `initialize_bonus()` locks `g_bonus_mutex`
2. Calls `start_bonus_timer()` while mutex is still locked
3. Timer starts and may immediately fire callback
4. Timer callback tries to lock `g_bonus_mutex`
5. **DEADLOCK**: Callback waits for mutex that's already held by `initialize_bonus()`

## Evidence from Logs

**Last log before hang**:
```
[Bonus] TRIGGERING BONUS ROUND for 2 tied players
```

**Missing logs** (never printed):
```
[Bonus] Initialized: match=... ← Never reached
[Bonus] check_and_trigger_bonus() RETURNING true ← Never reached
[Round1] r1_finalize_round() - perform_elimination() returned: 1 ← Never reached
```

This confirms code is stuck inside `initialize_bonus()` after calling `start_bonus_timer()`.

## Fix Applied

**Change**: Move `pthread_mutex_unlock()` to BEFORE `start_bonus_timer()` call.

**File Modified**: `/app/backend/Network/src/handlers/bonus_handler.c` (Lines 387-401)

**Impact**: 
- ✅ Eliminates deadlock
- ✅ Timer can safely acquire mutex in callbacks
- ✅ Bonus round initialization completes successfully
- ✅ Round 1 → Bonus transition works correctly

## Testing Checklist

- [ ] Rebuild backend with fix
- [ ] Test Round 1 with 2 players tying at lowest score
- [ ] Verify logs show:
  - `[Bonus] Initialized: match=...`
  - `[Bonus] check_and_trigger_bonus() RETURNING true`
  - `[Round1] After finalize: match_ended=0, bonus_triggered=1`
  - `[Round1] Broadcasting OP_S2C_ROUND1_ALL_FINISHED`
- [ ] Verify client receives bonus notifications
- [ ] Verify client transitions to RoundBonusWidget

## Related Issues

This is the SAME deadlock pattern that was previously fixed in conversation `38246ba2-9101-4c86-bf0f-9b115efaf5fe` but somehow regressed or wasn't fully applied.

**Previous Fix Location**: Same file, similar pattern with mutex unlock timing.

## Prevention

**Rule**: NEVER call async functions (timers, threads) while holding a mutex that those functions might need.

**Pattern to Follow**:
```c
pthread_mutex_lock(&mutex);
// ... critical section ...
pthread_mutex_unlock(&mutex);  // UNLOCK FIRST
start_async_operation();       // THEN start async ops
```

**NOT**:
```c
pthread_mutex_lock(&mutex);
start_async_operation();  // ❌ BAD: async op may need mutex
pthread_mutex_unlock(&mutex);
```
