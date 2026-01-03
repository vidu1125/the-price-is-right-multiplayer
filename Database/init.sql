-- ============================================================
-- ACCOUNTS
-- ============================================================
CREATE TABLE accounts (
    id SERIAL PRIMARY KEY,
    email VARCHAR(100) UNIQUE NOT NULL,
    password VARCHAR(255),
    provider VARCHAR(20),
    role VARCHAR(20) NOT NULL DEFAULT 'user',
    status VARCHAR(20),
    created_at TIMESTAMP,
    updated_at TIMESTAMP
);

-- ============================================================
-- SESSIONS
-- ============================================================
CREATE TABLE sessions (
    account_id INT PRIMARY KEY REFERENCES accounts(id) ON DELETE CASCADE,
    session_id UUID,
    connected BOOLEAN,
    updated_at TIMESTAMP
);

-- ============================================================
-- PROFILES
-- ============================================================
CREATE TABLE profiles (
    id SERIAL PRIMARY KEY,
    account_id INT REFERENCES accounts(id) ON DELETE CASCADE,
    name VARCHAR(50),
    avatar TEXT,
    bio TEXT,
    matches INT,
    wins INT,
    points INT,
    badges JSONB,
    created_at TIMESTAMP,
    updated_at TIMESTAMP
);

-- ============================================================
-- FRIENDS
-- ============================================================
CREATE TABLE friends (
    id SERIAL PRIMARY KEY,
    requester_id INT REFERENCES accounts(id) ON DELETE CASCADE,
    addressee_id INT REFERENCES accounts(id) ON DELETE CASCADE,
    status VARCHAR(20),
    created_at TIMESTAMP,
    updated_at TIMESTAMP,
    CONSTRAINT unique_friend_pair UNIQUE (requester_id, addressee_id)
);

-- ============================================================
-- ROOMS
-- ============================================================
CREATE TABLE rooms (
    id SERIAL PRIMARY KEY,
    code VARCHAR(10) UNIQUE,          
    name VARCHAR(100),
    visibility VARCHAR(20),
    host_id INT REFERENCES accounts(id) ON DELETE SET NULL,
    status VARCHAR(20),
    created_at TIMESTAMP,
    updated_at TIMESTAMP
);

-- ============================================================
-- ROOM MEMBERS
-- ============================================================
CREATE TABLE room_members (
    room_id INT REFERENCES rooms(id) ON DELETE CASCADE,
    account_id INT REFERENCES accounts(id) ON DELETE CASCADE,
    joined_at TIMESTAMP,
    PRIMARY KEY (room_id, account_id)
);

-- ============================================================
-- MATCHES
-- ============================================================
CREATE TABLE matches (
    id SERIAL PRIMARY KEY,
    room_id INT REFERENCES rooms(id) ON DELETE SET NULL,
    mode VARCHAR(20),
    max_players INT,
    wager BOOLEAN,
    round_time INT,
    started_at TIMESTAMP,
    ended_at TIMESTAMP
);

-- ============================================================
-- MATCH PLAYERS
-- ============================================================
CREATE TABLE match_players (
    id SERIAL PRIMARY KEY,
    match_id INT REFERENCES matches(id) ON DELETE CASCADE,
    account_id INT REFERENCES accounts(id) ON DELETE CASCADE,
    score INT,
    eliminated BOOLEAN,
    forfeited BOOLEAN,
    winner BOOLEAN,
    joined_at TIMESTAMP
);

-- ============================================================
-- ROUNDS
-- ============================================================
CREATE TABLE rounds (
    id SERIAL PRIMARY KEY,
    match_id INT REFERENCES matches(id) ON DELETE CASCADE,
    number INT,
    type VARCHAR(20),
    created_at TIMESTAMP
);

-- ============================================================
-- ROUND 1 — QUESTIONS
-- ============================================================
CREATE TABLE questions (
    id SERIAL PRIMARY KEY,
    text TEXT,
    image TEXT,
    a TEXT,
    b TEXT,
    c TEXT,
    d TEXT,
    correct VARCHAR(1),
    active BOOLEAN,
    last_updated_by INT REFERENCES accounts(id) ON DELETE SET NULL,
    last_updated_at TIMESTAMP
);

CREATE TABLE r1_items (
    id SERIAL PRIMARY KEY,
    round_id INT REFERENCES rounds(id) ON DELETE CASCADE,
    idx INT,
    text TEXT,
    image TEXT,
    a TEXT,
    b TEXT,
    c TEXT,
    d TEXT,
    correct VARCHAR(1)
);

CREATE TABLE r1_answers (
    id SERIAL PRIMARY KEY,
    item_id INT REFERENCES r1_items(id) ON DELETE CASCADE,
    player_id INT REFERENCES match_players(id) ON DELETE CASCADE,
    ans VARCHAR(1),
    time_ms INT,
    correct BOOLEAN,
    points INT,
    CONSTRAINT unique_r1_answer UNIQUE (item_id, player_id)
);

-- ============================================================
-- ROUND 2 — BIDDING
-- ============================================================
CREATE TABLE products (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255),
    description TEXT,
    image VARCHAR(255),
    price INT,
    active BOOLEAN,
    last_updated_by INT REFERENCES accounts(id) ON DELETE SET NULL,
    last_updated_at TIMESTAMP
);

CREATE TABLE r2_items (
    id SERIAL PRIMARY KEY,
    round_id INT REFERENCES rounds(id) ON DELETE CASCADE,
    name VARCHAR(255),
    description TEXT,
    image VARCHAR(255),
    price INT,
    idx INT
);

CREATE TABLE r2_answers (
    id SERIAL PRIMARY KEY,
    item_id INT REFERENCES r2_items(id) ON DELETE CASCADE,
    player_id INT REFERENCES match_players(id) ON DELETE CASCADE,
    bid INT,
    overbid BOOLEAN,
    points INT,
    wager BOOLEAN,
    CONSTRAINT unique_r2_answer UNIQUE (item_id, player_id)
);

-- ============================================================
-- ROUND 3 — BONUS WHEEL
-- ============================================================
CREATE TABLE wheel (
    id SERIAL PRIMARY KEY,
    value INT,
    last_updated_by INT REFERENCES accounts(id) ON DELETE SET NULL,
    last_updated_at TIMESTAMP
);

CREATE TABLE r3_items (
    id SERIAL PRIMARY KEY,
    round_id INT REFERENCES rounds(id) ON DELETE CASCADE,
    value INT
);

CREATE TABLE r3_answer (
    id SERIAL PRIMARY KEY,
    round_id INT REFERENCES rounds(id) ON DELETE CASCADE,
    player_id INT REFERENCES match_players(id) ON DELETE CASCADE,
    spin1_value INT,
    spin2_value INT,
    points INT
);

-- ============================================================
-- FAST PRESS
-- ============================================================
CREATE TABLE fp_round (
    id SERIAL PRIMARY KEY,
    round_id INT REFERENCES rounds(id) ON DELETE CASCADE,
    text TEXT,
    correct INT,
    wrong INT
);

CREATE TABLE fp_answers (
    id SERIAL PRIMARY KEY,
    round_id INT REFERENCES rounds(id) ON DELETE CASCADE,
    player_id INT REFERENCES match_players(id) ON DELETE CASCADE,
    choice INT,
    correct BOOLEAN,
    score INT
);

-- ============================================================
-- PLAYER MATCH HISTORY
-- ============================================================
CREATE TABLE player_match_history (
    id SERIAL PRIMARY KEY,
    player_id INT REFERENCES accounts(id) ON DELETE CASCADE,
    match_id INT REFERENCES matches(id) ON DELETE CASCADE,
    created_at TIMESTAMP
);

-- Optional optimization index
CREATE INDEX idx_history_player ON player_match_history (player_id, created_at DESC);
