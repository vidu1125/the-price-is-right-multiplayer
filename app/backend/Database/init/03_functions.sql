-- =============================================================================
-- ROOM RPC FUNCTIONS
-- =============================================================================

-- 1. update_room_rules: Update room settings
CREATE OR REPLACE FUNCTION update_room_rules(
    p_room_id INT,
    p_mode VARCHAR,
    p_max_players INT,
    p_wager_mode BOOLEAN
) RETURNS JSONB AS $$
DECLARE
    v_result JSONB;
BEGIN
    UPDATE rooms
    SET mode = p_mode,
        max_players = p_max_players,
        wager_mode = p_wager_mode,
        updated_at = NOW()
    WHERE id = p_room_id;

    IF FOUND THEN
        SELECT jsonb_build_object(
            'success', true,
            'room_id', p_room_id,
            'rules', jsonb_build_object(
                'mode', p_mode,
                'max_players', p_max_players,
                'wager_mode', p_wager_mode
            )
        ) INTO v_result;
    ELSE
        SELECT jsonb_build_object(
            'success', false,
            'error', 'Room not found'
        ) INTO v_result;
    END IF;

    RETURN v_result;
END;
$$ LANGUAGE plpgsql;

-- 2. kick_member: Remove player from room
CREATE OR REPLACE FUNCTION kick_member(
    p_room_id INT,
    p_account_id INT
) RETURNS JSONB AS $$
DECLARE
    v_result JSONB;
BEGIN
    DELETE FROM room_members
    WHERE room_id = p_room_id AND account_id = p_account_id;

    IF FOUND THEN
        SELECT jsonb_build_object(
            'success', true,
            'room_id', p_room_id,
            'kicked_account_id', p_account_id
        ) INTO v_result;
    ELSE
        SELECT jsonb_build_object(
            'success', false,
            'error', 'Member not found in room'
        ) INTO v_result;
    END IF;

    RETURN v_result;
END;
$$ LANGUAGE plpgsql;

-- 3. leave_room: Player leaves room
CREATE OR REPLACE FUNCTION leave_room(
    p_room_id INT,
    p_account_id INT
) RETURNS JSONB AS $$
DECLARE
    v_result JSONB;
BEGIN
    DELETE FROM room_members
    WHERE room_id = p_room_id AND account_id = p_account_id;

    IF FOUND THEN
        SELECT jsonb_build_object(
            'success', true,
            'room_id', p_room_id,
            'account_id', p_account_id
        ) INTO v_result;
    ELSE
        SELECT jsonb_build_object(
            'success', false,
            'error', 'Not a member of this room'
        ) INTO v_result;
    END IF;

    RETURN v_result;
END;
$$ LANGUAGE plpgsql;

-- 4. close_room: Host closes room
CREATE OR REPLACE FUNCTION close_room(
    p_room_id INT
) RETURNS JSONB AS $$
DECLARE
    v_result JSONB;
BEGIN
    -- 1. Update room status to closed
    UPDATE rooms
    SET status = 'closed',
        updated_at = NOW()
    WHERE id = p_room_id;

    IF FOUND THEN
        -- 2. Optional: Clean up members (cascade delete handles it if configured, 
        -- but we can explicitly clear if needed or keep for history)
        -- DELETE FROM room_members WHERE room_id = p_room_id;

        SELECT jsonb_build_object(
            'success', true,
            'room_id', p_room_id,
            'message', 'Room closed successfully'
        ) INTO v_result;
    ELSE
        SELECT jsonb_build_object(
            'success', false,
            'error', 'Room not found'
        ) INTO v_result;
    END IF;

    RETURN v_result;
END;
$$ LANGUAGE plpgsql;
