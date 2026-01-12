
-- Create view to include player counts
CREATE OR REPLACE VIEW rooms_with_counts AS
SELECT 
    r.id,
    r.name,
    r.status,
    r.mode,
    r.max_players,
    r.visibility,
    r.wager_mode,
    r.created_at,
    (SELECT COUNT(*) FROM room_members rm WHERE rm.room_id = r.id) as current_players
FROM rooms r;
