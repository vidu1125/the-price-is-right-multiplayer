-- ===============================
-- ADD GAME SETTINGS TO ROOMS
-- ===============================
ALTER TABLE rooms ADD COLUMN IF NOT EXISTS mode VARCHAR(20) DEFAULT 'scoring';
ALTER TABLE rooms ADD COLUMN IF NOT EXISTS max_players INT DEFAULT 5;
ALTER TABLE rooms ADD COLUMN IF NOT EXISTS wager_mode BOOLEAN DEFAULT TRUE;
