-- RPC Functions implementing legacy logic from room_repo.c

-- CLOSE ROOM
CREATE OR REPLACE FUNCTION close_room(p_room_id INT)
RETURNS JSONB AS $$
BEGIN
    UPDATE rooms 
    SET status = 'closed', updated_at = NOW()
    WHERE id = p_room_id;
    
    RETURN '{"success": true}'::jsonb;
END;
$$ LANGUAGE plpgsql;

-- UPDATE ROOM RULES
CREATE OR REPLACE FUNCTION update_room_rules(
    p_room_id INT,
    p_mode VARCHAR,
    p_max_players INT,
    p_wager_mode BOOLEAN
)
RETURNS JSONB AS $$
BEGIN
    UPDATE rooms
    SET mode = p_mode,
        max_players = p_max_players,
        wager_mode = p_wager_mode,
        updated_at = NOW()
    WHERE id = p_room_id;
    
    RETURN '{"success": true}'::jsonb;
END;
$$ LANGUAGE plpgsql;

-- KICK MEMBER
CREATE OR REPLACE FUNCTION kick_member(p_room_id INT, p_account_id INT)
RETURNS JSONB AS $$
BEGIN
    DELETE FROM room_members
    WHERE room_id = p_room_id AND account_id = p_account_id;
    
    RETURN '{"success": true}'::jsonb;
END;
$$ LANGUAGE plpgsql;

-- LEAVE ROOM
CREATE OR REPLACE FUNCTION leave_room(p_room_id INT, p_account_id INT)
RETURNS JSONB AS $$
BEGIN
    DELETE FROM room_members
    WHERE room_id = p_room_id AND account_id = p_account_id;
    
    RETURN '{"success": true}'::jsonb;
END;
$$ LANGUAGE plpgsql;
