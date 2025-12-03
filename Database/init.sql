-- =================================================================
-- CLEAN MINIMAL SCHEMA FOR THE PRICE IS RIGHT
-- Based on: accounts → profiles → rooms → matches → rounds
-- =================================================================

-- =================================================================
-- TABLE: accounts (Core authentication)
-- =================================================================
CREATE TABLE IF NOT EXISTS accounts (
    id SERIAL PRIMARY KEY,
    email VARCHAR(100) UNIQUE NOT NULL,
    password VARCHAR(255) NOT NULL,
    provider VARCHAR(20) DEFAULT 'local' CHECK (provider IN ('local', 'google', 'facebook')),
    status VARCHAR(20) DEFAULT 'active' CHECK (status IN ('active', 'banned', 'suspended')),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- =================================================================
-- TABLE: profiles (Player info & stats)
-- =================================================================
CREATE TABLE IF NOT EXISTS profiles (
    id SERIAL PRIMARY KEY,
    account_id INT UNIQUE NOT NULL,
    name VARCHAR(50) NOT NULL,
    avatar TEXT,
    bio TEXT,
    matches INT DEFAULT 0,
    wins INT DEFAULT 0,
    points INT DEFAULT 0,
    badges JSONB DEFAULT '[]'::jsonb,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE
);

-- =================================================================
-- TABLE: friends (Friend system)
-- =================================================================
CREATE TABLE IF NOT EXISTS friends (
    id SERIAL PRIMARY KEY,
    requester_id INT NOT NULL,
    addressee_id INT NOT NULL,
    status VARCHAR(20) DEFAULT 'pending' CHECK (status IN ('pending', 'accepted', 'blocked')),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (requester_id, addressee_id),
    FOREIGN KEY (requester_id) REFERENCES accounts(id) ON DELETE CASCADE,
    FOREIGN KEY (addressee_id) REFERENCES accounts(id) ON DELETE CASCADE
);

-- =================================================================
-- TABLE: rooms (Game lobbies)
-- =================================================================
CREATE TABLE IF NOT EXISTS rooms (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    visibility VARCHAR(20) DEFAULT 'public' CHECK (visibility IN ('public', 'private')),
    host_id INT NOT NULL,
    status VARCHAR(20) DEFAULT 'waiting' CHECK (status IN ('waiting', 'in_game', 'closed')),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (host_id) REFERENCES accounts(id) ON DELETE CASCADE
);

-- =================================================================
-- TABLE: sessions (Active connections)
-- =================================================================
CREATE TABLE IF NOT EXISTS sessions (
    id SERIAL PRIMARY KEY,
    account_id INT NOT NULL,
    room_id INT,
    ip VARCHAR(50),
    agent VARCHAR(200),
    connected BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    last_active TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE,
    FOREIGN KEY (room_id) REFERENCES rooms(id) ON DELETE SET NULL
);

-- =================================================================
-- TABLE: room_members (Players in rooms)
-- =================================================================
CREATE TABLE IF NOT EXISTS room_members (
    room_id INT NOT NULL,
    account_id INT NOT NULL,
    role VARCHAR(20) DEFAULT 'member' CHECK (role IN ('host', 'member')),
    ready BOOLEAN DEFAULT FALSE,
    kicked BOOLEAN DEFAULT FALSE,
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    left_at TIMESTAMP,
    PRIMARY KEY (room_id, account_id),
    FOREIGN KEY (room_id) REFERENCES rooms(id) ON DELETE CASCADE,
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE
);

-- =================================================================
-- TABLE: matches (Game instances)
-- =================================================================
CREATE TABLE IF NOT EXISTS matches (
    id SERIAL PRIMARY KEY,
    room_id INT NOT NULL,
    started_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    ended_at TIMESTAMP,
    status VARCHAR(20) DEFAULT 'playing' CHECK (status IN ('playing', 'finished', 'aborted')),
    FOREIGN KEY (room_id) REFERENCES rooms(id) ON DELETE CASCADE
);

-- =================================================================
-- TABLE: match_settings (Game configuration)
-- =================================================================
CREATE TABLE IF NOT EXISTS match_settings (
    match_id INT PRIMARY KEY,
    mode VARCHAR(20) DEFAULT 'scoring' CHECK (mode IN ('scoring', 'elimination')),
    max_players INT DEFAULT 4 CHECK (max_players BETWEEN 4 AND 8),
    wager BOOLEAN DEFAULT FALSE,
    round_time INT DEFAULT 15,
    visibility VARCHAR(20) DEFAULT 'public' CHECK (visibility IN ('public', 'private')),
    FOREIGN KEY (match_id) REFERENCES matches(id) ON DELETE CASCADE
);

-- =================================================================
-- TABLE: match_players (Players in a match)
-- =================================================================
CREATE TABLE IF NOT EXISTS match_players (
    id SERIAL PRIMARY KEY,
    match_id INT NOT NULL,
    account_id INT NOT NULL,
    score INT DEFAULT 0,
    eliminated BOOLEAN DEFAULT FALSE,
    forfeited BOOLEAN DEFAULT FALSE,
    winner BOOLEAN DEFAULT FALSE,
    connected BOOLEAN DEFAULT TRUE,
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (match_id) REFERENCES matches(id) ON DELETE CASCADE,
    FOREIGN KEY (account_id) REFERENCES accounts(id) ON DELETE CASCADE
);

-- =================================================================
-- TABLE: rounds (Game rounds)
-- =================================================================
CREATE TABLE IF NOT EXISTS rounds (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    match_id INT NOT NULL,
    number INT NOT NULL CHECK (number BETWEEN 1 AND 5),
    type VARCHAR(20) NOT NULL CHECK (type IN ('quiz', 'bid', 'updown', 'spin', 'fastpress')),
    question UUID,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (match_id) REFERENCES matches(id) ON DELETE CASCADE
);

-- =================================================================
-- ROUND 1: QUIZ
-- =================================================================
CREATE TABLE IF NOT EXISTS questions (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    text TEXT NOT NULL,
    image TEXT,
    a TEXT NOT NULL,
    b TEXT NOT NULL,
    c TEXT NOT NULL,
    d TEXT NOT NULL,
    correct VARCHAR(1) NOT NULL CHECK (correct IN ('A', 'B', 'C', 'D')),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS r1_items (
    id SERIAL PRIMARY KEY,
    round_id UUID NOT NULL,
    qid UUID NOT NULL,
    idx INT NOT NULL,
    time_sec INT DEFAULT 15,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (round_id) REFERENCES rounds(id) ON DELETE CASCADE,
    FOREIGN KEY (qid) REFERENCES questions(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS r1_answers (
    id SERIAL PRIMARY KEY,
    item_id INT NOT NULL,
    player_id INT NOT NULL,
    ans VARCHAR(1) CHECK (ans IN ('A', 'B', 'C', 'D')),
    time_ms INT NOT NULL,
    correct BOOLEAN NOT NULL,
    points INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (item_id, player_id),
    FOREIGN KEY (item_id) REFERENCES r1_items(id) ON DELETE CASCADE,
    FOREIGN KEY (player_id) REFERENCES match_players(id) ON DELETE CASCADE
);

-- =================================================================
-- ROUND 2: THE BID
-- =================================================================
CREATE TABLE IF NOT EXISTS products (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    description TEXT,
    image VARCHAR(255),
    price INT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    created_by INT,
    FOREIGN KEY (created_by) REFERENCES accounts(id) ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS r2_items (
    id SERIAL PRIMARY KEY,
    round_id UUID NOT NULL,
    product_id INT NOT NULL,
    idx INT NOT NULL,
    shown INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (round_id) REFERENCES rounds(id) ON DELETE CASCADE,
    FOREIGN KEY (product_id) REFERENCES products(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS r2_bids (
    id SERIAL PRIMARY KEY,
    item_id INT NOT NULL,
    player_id INT NOT NULL,
    bid INT NOT NULL,
    overbid BOOLEAN DEFAULT FALSE,
    winner BOOLEAN DEFAULT FALSE,
    points INT DEFAULT 0,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (item_id, player_id),
    FOREIGN KEY (item_id) REFERENCES r2_items(id) ON DELETE CASCADE,
    FOREIGN KEY (player_id) REFERENCES match_players(id) ON DELETE CASCADE
);

-- =================================================================
-- ROUND 3: UP OR DOWN
-- =================================================================
CREATE TABLE IF NOT EXISTS r3_items (
    id SERIAL PRIMARY KEY,
    round_id UUID NOT NULL,
    product_id INT NOT NULL,
    hint INT NOT NULL,
    pattern VARCHAR(20) NOT NULL,
    idx INT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (round_id) REFERENCES rounds(id) ON DELETE CASCADE,
    FOREIGN KEY (product_id) REFERENCES products(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS r3_guesses (
    id SERIAL PRIMARY KEY,
    item_id INT NOT NULL,
    player_id INT NOT NULL,
    guess VARCHAR(20) NOT NULL,
    correct INT DEFAULT 0,
    perfect BOOLEAN DEFAULT FALSE,
    points INT DEFAULT 0,
    time_ms INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (item_id, player_id),
    FOREIGN KEY (item_id) REFERENCES r3_items(id) ON DELETE CASCADE,
    FOREIGN KEY (player_id) REFERENCES match_players(id) ON DELETE CASCADE
);

-- =================================================================
-- ROUND 4: BONUS WHEEL
-- =================================================================
CREATE TABLE IF NOT EXISTS wheel (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    value INT NOT NULL CHECK (value BETWEEN 5 AND 100),
    weight INT DEFAULT 1,
    active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS bonus (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    round_id UUID NOT NULL,
    player_id INT NOT NULL,
    v1_id UUID NOT NULL,
    v1 INT NOT NULL,
    v2_id UUID,
    v2 INT,
    total INT NOT NULL,
    score INT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (round_id) REFERENCES rounds(id) ON DELETE CASCADE,
    FOREIGN KEY (player_id) REFERENCES match_players(id) ON DELETE CASCADE,
    FOREIGN KEY (v1_id) REFERENCES wheel(id) ON DELETE CASCADE,
    FOREIGN KEY (v2_id) REFERENCES wheel(id) ON DELETE CASCADE
);

-- =================================================================
-- ROUND 5: FAST PRESS (BONUS)
-- =================================================================
CREATE TABLE IF NOT EXISTS fp_round (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    round_id UUID NOT NULL,
    qid UUID NOT NULL,
    opt1 INT NOT NULL,
    opt2 INT NOT NULL,
    correct INT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (round_id) REFERENCES rounds(id) ON DELETE CASCADE,
    FOREIGN KEY (qid) REFERENCES questions(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS fp_results (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    round_id UUID NOT NULL,
    player_id INT NOT NULL,
    before INT NOT NULL,
    after INT NOT NULL,
    rank INT,
    choice INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (round_id) REFERENCES rounds(id) ON DELETE CASCADE,
    FOREIGN KEY (player_id) REFERENCES match_players(id) ON DELETE CASCADE
);

-- =================================================================
-- INDEXES
-- =================================================================
CREATE INDEX idx_accounts_email ON accounts(email);
CREATE INDEX idx_profiles_account ON profiles(account_id);
CREATE INDEX idx_rooms_host ON rooms(host_id);
CREATE INDEX idx_rooms_status ON rooms(status);
CREATE INDEX idx_room_members_account ON room_members(account_id);
CREATE INDEX idx_matches_room ON matches(room_id);
CREATE INDEX idx_match_players_match ON match_players(match_id);
CREATE INDEX idx_match_players_account ON match_players(account_id);
CREATE INDEX idx_rounds_match ON rounds(match_id);
CREATE INDEX idx_sessions_account ON sessions(account_id);
CREATE INDEX idx_friends_requester ON friends(requester_id);
CREATE INDEX idx_friends_addressee ON friends(addressee_id);

-- =================================================================
-- TEST DATA
-- =================================================================
INSERT INTO accounts (email, password, provider, status) VALUES
    ('alice@example.com', '$2b$12$hashed_alice', 'local', 'active'),
    ('bob@example.com', '$2b$12$hashed_bob', 'local', 'active'),
    ('charlie@example.com', '$2b$12$hashed_charlie', 'local', 'active'),
    ('diana@example.com', '$2b$12$hashed_diana', 'local', 'active')
ON CONFLICT (email) DO NOTHING;

INSERT INTO profiles (account_id, name, avatar, bio, matches, wins, points) VALUES
    (1, 'Alice', 'https://api.dicebear.com/7.x/avataaars/svg?seed=Alice', 'Game master', 0, 0, 0),
    (2, 'Bob', 'https://api.dicebear.com/7.x/avataaars/svg?seed=Bob', 'Casual player', 0, 0, 0),
    (3, 'Charlie', 'https://api.dicebear.com/7.x/avataaars/svg?seed=Charlie', 'Pro gamer', 0, 0, 0),
    (4, 'Diana', 'https://api.dicebear.com/7.x/avataaars/svg?seed=Diana', 'Newbie', 0, 0, 0)
ON CONFLICT (account_id) DO NOTHING;

INSERT INTO products (name, description, image, price, created_by) VALUES
    ('iPhone 15 Pro', 'Latest Apple smartphone', 'https://picsum.photos/400/300?random=1', 29990000, 1),
    ('Samsung TV 55"', '4K Smart TV', 'https://picsum.photos/400/300?random=2', 15990000, 1),
    ('Sony Headphones', 'Noise cancelling WH-1000XM5', 'https://picsum.photos/400/300?random=3', 5990000, 1),
    ('MacBook Air M2', '13-inch laptop', 'https://picsum.photos/400/300?random=4', 28990000, 1),
    ('AirPods Pro', 'Wireless earbuds with ANC', 'https://picsum.photos/400/300?random=5', 6990000, 1)
ON CONFLICT DO NOTHING;

INSERT INTO questions (text, a, b, c, d, correct) VALUES
    ('What is 2+2?', '3', '4', '5', '6', 'B'),
    ('Capital of Vietnam?', 'HCMC', 'Hanoi', 'Da Nang', 'Hue', 'B'),
    ('Python is a ...?', 'Snake', 'Language', 'Framework', 'Library', 'B'),
    ('Who created Linux?', 'Bill Gates', 'Steve Jobs', 'Linus Torvalds', 'Mark Zuckerberg', 'C'),
    ('HTTP stands for?', 'HyperText Transfer Protocol', 'High Transfer Protocol', 'Host Transfer Protocol', 'None', 'A')
ON CONFLICT DO NOTHING;

INSERT INTO wheel (value, weight, active) VALUES
    (5, 2, true), (10, 3, true), (15, 3, true),
    (20, 3, true), (25, 2, true), (30, 2, true),
    (40, 2, true), (50, 2, true), (75, 1, true),
    (100, 1, true)
ON CONFLICT DO NOTHING;

-- =================================================================
-- COMMENTS
-- =================================================================
COMMENT ON TABLE accounts IS 'User authentication';
COMMENT ON TABLE profiles IS 'Player profiles & stats';
COMMENT ON TABLE rooms IS 'Game lobbies';
COMMENT ON TABLE matches IS 'Active games';
COMMENT ON TABLE rounds IS 'Game rounds (1-5)';
COMMENT ON TABLE products IS 'Items for Round 2 & 3';
COMMENT ON TABLE questions IS 'Quiz questions for Round 1';
COMMENT ON TABLE wheel IS 'Bonus wheel values for Round 4';