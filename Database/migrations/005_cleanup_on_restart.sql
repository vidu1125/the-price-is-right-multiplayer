-- -- Cleanup zombie rooms on server restart
-- -- This migration should be run on every server startup

-- UPDATE rooms 
-- SET status = 'closed' 
-- WHERE status IN ('waiting', 'playing');

-- TRUNCATE room_members;
