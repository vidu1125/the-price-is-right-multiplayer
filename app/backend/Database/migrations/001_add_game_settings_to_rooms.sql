-- Migration: Add game settings columns to rooms table
-- Date: 2025-12-29
-- Description: Add mode, max_players, wager_mode columns

-- Add columns with defaults
ALTER TABLE rooms 
  ADD COLUMN IF NOT EXISTS mode VARCHAR(20) DEFAULT 'scoring',
  ADD COLUMN IF NOT EXISTS max_players INT DEFAULT 5,
  ADD COLUMN IF NOT EXISTS wager_mode BOOLEAN DEFAULT FALSE;

-- Verify columns added
SELECT column_name, data_type, column_default 
FROM information_schema.columns 
WHERE table_name = 'rooms' 
  AND column_name IN ('mode', 'max_players', 'wager_mode')
ORDER BY column_name;
