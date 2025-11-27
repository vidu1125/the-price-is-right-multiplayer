-- =================================================================
-- TABLE: users
-- =================================================================
CREATE TABLE IF NOT EXISTS users (
    user_id VARCHAR(100) PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    email VARCHAR(100) UNIQUE NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- =================================================================
-- TABLE: rooms
-- =================================================================
CREATE TABLE IF NOT EXISTS rooms (
    room_id SERIAL PRIMARY KEY,
    host_id VARCHAR(100) NOT NULL,
    name VARCHAR(100) NOT NULL,
    visibility VARCHAR(20) DEFAULT 'public' CHECK (visibility IN ('public', 'private')),
    status VARCHAR(20) DEFAULT 'waiting' CHECK (status IN ('waiting', 'in_game', 'closed')),
    mode VARCHAR(20) DEFAULT 'scoring' CHECK (mode IN ('scoring', 'elimination')),
    max_players INT NOT NULL DEFAULT 4 CHECK (max_players BETWEEN 4 AND 8),
    wager_enabled BOOLEAN DEFAULT FALSE,
    round_time_sec INT DEFAULT 15,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP NULL,
    FOREIGN KEY (host_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- =================================================================
-- TABLE: room_members
-- =================================================================
CREATE TABLE IF NOT EXISTS room_members (
    room_id INT NOT NULL,
    user_id VARCHAR(100) NOT NULL,
    role VARCHAR(20) DEFAULT 'member' CHECK (role IN ('host', 'member')),
    is_ready BOOLEAN DEFAULT FALSE,
    is_kicked BOOLEAN DEFAULT FALSE,
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (room_id, user_id),
    FOREIGN KEY (room_id) REFERENCES rooms(room_id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- =================================================================
-- TABLE: products (for Round 2 & 3)
-- =================================================================
CREATE TABLE IF NOT EXISTS products (
    product_id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    description TEXT,
    image_url VARCHAR(255),
    true_price INT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_by VARCHAR(100),
    FOREIGN KEY (created_by) REFERENCES users(user_id) ON DELETE SET NULL
);

-- =================================================================
-- TABLE: matches
-- =================================================================
CREATE TABLE IF NOT EXISTS matches (
    match_id SERIAL PRIMARY KEY,
    room_id INT NOT NULL,
    mode VARCHAR(20) NOT NULL CHECK (mode IN ('scoring', 'elimination')),
    wager_enabled BOOLEAN NOT NULL DEFAULT FALSE,
    visibility VARCHAR(20) NOT NULL DEFAULT 'public' CHECK (visibility IN ('public', 'private')),
    started_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    ended_at TIMESTAMP NULL,
    status VARCHAR(20) DEFAULT 'playing' CHECK (status IN ('playing', 'finished', 'aborted')),
    FOREIGN KEY (room_id) REFERENCES rooms(room_id) ON DELETE CASCADE
);

-- =================================================================
-- TABLE: match_players
-- =================================================================
CREATE TABLE IF NOT EXISTS match_players (
    match_player_id SERIAL PRIMARY KEY,
    match_id INT NOT NULL,
    user_id VARCHAR(100) NOT NULL,
    total_score INT DEFAULT 0,
    is_eliminated BOOLEAN DEFAULT FALSE,
    is_forfeited BOOLEAN DEFAULT FALSE,
    is_winner BOOLEAN DEFAULT FALSE,
    joined_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (match_id) REFERENCES matches(match_id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- =================================================================
-- TABLE: rounds
-- =================================================================
CREATE TABLE IF NOT EXISTS rounds (
    round_id SERIAL PRIMARY KEY,
    match_id INT NOT NULL,
    round_number INT NOT NULL CHECK (round_number BETWEEN 1 AND 4),
    round_type VARCHAR(20) NOT NULL CHECK (round_type IN ('quiz', 'bid', 'updown', 'spin')),
    started_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    ended_at TIMESTAMP NULL,
    FOREIGN KEY (match_id) REFERENCES matches(match_id) ON DELETE CASCADE
);

-- =================================================================
-- TABLE: round2_items (The Bid)
-- =================================================================
CREATE TABLE IF NOT EXISTS round2_items (
    r2_item_id SERIAL PRIMARY KEY,
    round_id INT NOT NULL,
    product_id INT NOT NULL,
    displayed_price INT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (round_id) REFERENCES rounds(round_id) ON DELETE CASCADE,
    FOREIGN KEY (product_id) REFERENCES products(product_id) ON DELETE CASCADE
);

-- =================================================================
-- TABLE: round2_bids
-- =================================================================
CREATE TABLE IF NOT EXISTS round2_bids (
    bid_id SERIAL PRIMARY KEY,
    round_id INT NOT NULL,
    match_player_id INT NOT NULL,
    bid_value INT NOT NULL,
    is_winner BOOLEAN DEFAULT FALSE,
    points_awarded INT DEFAULT 0,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (round_id) REFERENCES round2_items(round_id) ON DELETE CASCADE,
    FOREIGN KEY (match_player_id) REFERENCES match_players(match_player_id) ON DELETE CASCADE
);

-- =================================================================
-- TABLE: round3_items (Up or Down)
-- =================================================================
CREATE TABLE IF NOT EXISTS round3_items (
    r3_item_id SERIAL PRIMARY KEY,
    round_id INT NOT NULL,
    product_id INT NOT NULL,
    hint_price INT NOT NULL,
    correct_sequence VARCHAR(20) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (round_id) REFERENCES rounds(round_id) ON DELETE CASCADE,
    FOREIGN KEY (product_id) REFERENCES products(product_id) ON DELETE CASCADE
);

-- =================================================================
-- TABLE: round3_guesses
-- =================================================================
CREATE TABLE IF NOT EXISTS round3_guesses (
    guess_id SERIAL PRIMARY KEY,
    round_id INT NOT NULL,
    match_player_id INT NOT NULL,
    sequence_guess VARCHAR(20) NOT NULL,
    correct_count INT NOT NULL DEFAULT 0,
    is_perfect BOOLEAN DEFAULT FALSE,
    points_awarded INT DEFAULT 0,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (round_id) REFERENCES round3_items(round_id) ON DELETE CASCADE,
    FOREIGN KEY (match_player_id) REFERENCES match_players(match_player_id) ON DELETE CASCADE
);

-- =================================================================
-- INDEXES for Performance
-- =================================================================
CREATE INDEX IF NOT EXISTS idx_rooms_status ON rooms(status);
CREATE INDEX IF NOT EXISTS idx_rooms_host ON rooms(host_id);
CREATE INDEX IF NOT EXISTS idx_rooms_deleted ON rooms(deleted_at);
CREATE INDEX IF NOT EXISTS idx_room_members_user ON room_members(user_id);
CREATE INDEX IF NOT EXISTS idx_matches_room ON matches(room_id);
CREATE INDEX IF NOT EXISTS idx_match_players_match ON match_players(match_id);
CREATE INDEX IF NOT EXISTS idx_match_players_user ON match_players(user_id);
CREATE INDEX IF NOT EXISTS idx_rounds_match ON rounds(match_id);

-- =================================================================
-- INSERT TEST DATA
-- =================================================================

-- Insert test users
INSERT INTO users (user_id, username, password_hash, email) 
VALUES 
    ('user_001', 'alice', '$2b$12$hashed_password_alice', 'alice@example.com'),
    ('user_002', 'bob', '$2b$12$hashed_password_bob', 'bob@example.com'),
    ('user_003', 'charlie', '$2b$12$hashed_password_charlie', 'charlie@example.com'),
    ('user_004', 'diana', '$2b$12$hashed_password_diana', 'diana@example.com')
ON CONFLICT (user_id) DO NOTHING;

-- Insert test products
INSERT INTO products (name, description, image_url, true_price, created_by) 
VALUES 
    ('iPhone 15 Pro', 'Latest Apple smartphone', 'https://example.com/iphone.jpg', 29990000, 'user_001'),
    ('Samsung TV 55"', '4K Smart TV', 'https://example.com/tv.jpg', 15990000, 'user_001'),
    ('Sony Headphones', 'Noise cancelling', 'https://example.com/headphones.jpg', 5990000, 'user_001'),
    ('MacBook Air M2', 'Laptop 13 inch', 'https://example.com/macbook.jpg', 28990000, 'user_001'),
    ('AirPods Pro', 'Wireless earbuds', 'https://example.com/airpods.jpg', 6990000, 'user_001')
ON CONFLICT DO NOTHING;

-- =================================================================
-- COMMENTS
-- =================================================================
COMMENT ON TABLE rooms IS 'Game rooms created by hosts';
COMMENT ON TABLE room_members IS 'Players in each room';
COMMENT ON TABLE matches IS 'Game matches (1 room can have multiple matches)';
COMMENT ON TABLE match_players IS 'Players participating in a match';
COMMENT ON TABLE rounds IS 'Game rounds (4 rounds per match)';
COMMENT ON TABLE products IS 'Products for Round 2 & 3';