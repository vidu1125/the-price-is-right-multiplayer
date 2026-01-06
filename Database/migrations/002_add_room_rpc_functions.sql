-- Migration: Add RPC functions for room operations
-- Date: 2026-01-02
-- Description: PostgreSQL functions callable via Supabase RPC

-- ============================================================================
-- 1. UPDATE ROOM RULES
-- ============================================================================
CREATE OR REPLACE FUNCTION update_room_rules(
    p_room_id INT,
    p_mode VARCHAR(20),
    p_max_players INT,
    p_round_time INT,
    p_wager_mode BOOLEAN
) RETURNS VOID AS $$
BEGIN
    UPDATE rooms
    SET 
        mode = p_mode,
        max_players = p_max_players,
        round_time = p_round_time,
        wager_mode = p_wager_mode,
        updated_at = NOW()
    WHERE id = p_room_id
      AND status = 'waiting'; -- Only allow updates in waiting state
END;
$$ LANGUAGE plpgsql;

-- ============================================================================
-- 2. KICK MEMBER
-- ============================================================================
CREATE OR REPLACE FUNCTION kick_member(
    p_room_id INT,
    p_account_id INT
) RETURNS VOID AS $$
BEGIN
    DELETE FROM room_members
    WHERE room_id = p_room_id
      AND account_id = p_account_id;
END;
$$ LANGUAGE plpgsql;

-- ============================================================================
-- 3. CLOSE ROOM
-- ============================================================================
CREATE OR REPLACE FUNCTION close_room(
    p_room_id INT
) RETURNS VOID AS $$
BEGIN
    -- Update room status
    UPDATE rooms
    SET status = 'closed',
        updated_at = NOW()
    WHERE id = p_room_id;
    
    -- Remove all members
    DELETE FROM room_members
    WHERE room_id = p_room_id;
END;
$$ LANGUAGE plpgsql;

-- ============================================================================
-- 4. LEAVE ROOM
-- ============================================================================
CREATE OR REPLACE FUNCTION leave_room(
    p_room_id INT,
    p_account_id INT
) RETURNS VOID AS $$
BEGIN
    DELETE FROM room_members
    WHERE room_id = p_room_id
      AND account_id = p_account_id;
      
    -- Optional: If host leaves, close room
    -- IF EXISTS (SELECT 1 FROM rooms WHERE id = p_room_id AND host_id = p_account_id) THEN
    --     PERFORM close_room(p_room_id);
    -- END IF;
END;
$$ LANGUAGE plpgsql;

-- ============================================================================
-- VERIFY FUNCTIONS CREATED
-- ============================================================================
SELECT routine_name, routine_type
FROM information_schema.routines
WHERE routine_schema = 'public'
  AND routine_name IN ('update_room_rules', 'kick_member', 'close_room', 'leave_room')
ORDER BY routine_name;
