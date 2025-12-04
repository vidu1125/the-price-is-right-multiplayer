-- ============================================================
-- Enable UUID generation
-- ============================================================
CREATE EXTENSION IF NOT EXISTS "pgcrypto";

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
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- ============================================================
-- PROFILES
-- ============================================================
CREATE TABLE profiles (
    id SERIAL PRIMARY KEY,
    account_id INT REFERENCES accounts(id),
    name VARCHAR(50),
    avatar TEXT,
    bio TEXT,
    matches INT DEFAULT 0,
    wins INT DEFAULT 0,
    points INT DEFAULT 0,
    badges JSONB,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- ============================================================
-- FRIENDS
-- ============================================================
CREATE TABLE friends (
    id SERIAL PRIMARY KEY,
    requester_id INT REFERENCES accounts(id),
    addressee_id INT REFERENCES accounts(id),
    status VARCHAR(20),
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW(),
    CONSTRAINT unique_friend_pair UNIQUE (requester_id, addressee_id)
);

-- ============================================================
-- SESSIONS
-- ============================================================
CREATE TABLE sessions (
    id SERIAL PRIMARY KEY,
    account_id INT REFERENCES accounts(id),
    room_id INT,
    ip VARCHAR(50),
    agent VARCHAR(200),
    connected BOOLEAN,
    created_at TIMESTAMP DEFAULT NOW(),
    last_active TIMESTAMP DEFAULT NOW()
);

-- ============================================================
-- ROOMS
-- ============================================================
CREATE TABLE rooms (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100),
    visibility VARCHAR(20),
    host_id INT REFERENCES accounts(id),
    status VARCHAR(20),
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- ============================================================
-- ROOM MEMBERS
-- ============================================================
CREATE TABLE room_members (
    room_id INT REFERENCES rooms(id),
    account_id INT REFERENCES accounts(id),
    role VARCHAR(20),
    ready BOOLEAN DEFAULT FALSE,
    kicked BOOLEAN DEFAULT FALSE,
    joined_at TIMESTAMP DEFAULT NOW(),
    left_at TIMESTAMP,
    PRIMARY KEY (room_id, account_id)
);

-- ============================================================
-- MATCHES
-- ============================================================
CREATE TABLE matches (
    id SERIAL PRIMARY KEY,
    room_id INT REFERENCES rooms(id),
    started_at TIMESTAMP,
    ended_at TIMESTAMP,
    status VARCHAR(20)
);

-- ============================================================
-- MATCH SETTINGS
-- ============================================================
CREATE TABLE match_settings (
    match_id INT PRIMARY KEY REFERENCES matches(id),
    mode VARCHAR(20),
    max_players INT,
    wager BOOLEAN,
    round_time INT,
    visibility VARCHAR(20)
);

-- ============================================================
-- MATCH PLAYERS
-- ============================================================
CREATE TABLE match_players (
    id SERIAL PRIMARY KEY,
    match_id INT REFERENCES matches(id),
    account_id INT REFERENCES accounts(id),
    score INT DEFAULT 0,
    eliminated BOOLEAN DEFAULT FALSE,
    forfeited BOOLEAN DEFAULT FALSE,
    winner BOOLEAN DEFAULT FALSE,
    connected BOOLEAN DEFAULT TRUE,
    joined_at TIMESTAMP DEFAULT NOW()
);

-- ============================================================
-- ROUNDS
-- ============================================================
CREATE TABLE rounds (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    match_id INT REFERENCES matches(id),
    number INT,
    type VARCHAR(20),  -- quiz, bid, r3, fp
    question UUID,
    created_at TIMESTAMP DEFAULT NOW()
);

-- ============================================================
-- ROUND 1 - QUIZ
-- ============================================================
CREATE TABLE questions (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    text TEXT,
    image TEXT,
    a TEXT,
    b TEXT,
    c TEXT,
    d TEXT,
    correct VARCHAR(1),
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE r1_items (
    id SERIAL PRIMARY KEY,
    round_id UUID REFERENCES rounds(id),
    qid UUID REFERENCES questions(id),
    idx INT,
    time_sec INT,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE r1_answers (
    id SERIAL PRIMARY KEY,
    item_id INT REFERENCES r1_items(id),
    player_id INT REFERENCES match_players(id),
    ans VARCHAR(1),
    time_ms INT,
    correct BOOLEAN,
    points INT,
    created_at TIMESTAMP DEFAULT NOW(),
    CONSTRAINT unique_r1_answer UNIQUE (item_id, player_id)
);

-- ============================================================
-- ROUND 2 - BIDDING
-- ============================================================
CREATE TABLE products (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255),
    description TEXT,
    image VARCHAR(255),
    price INT,
    created_at TIMESTAMP DEFAULT NOW(),
    created_by INT REFERENCES accounts(id)
);

CREATE TABLE r2_items (
    id SERIAL PRIMARY KEY,
    round_id UUID REFERENCES rounds(id),
    product_id INT REFERENCES products(id),
    idx INT,
    shown INT,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE r2_bids (
    id SERIAL PRIMARY KEY,
    item_id INT REFERENCES r2_items(id),
    player_id INT REFERENCES match_players(id),
    bid INT,
    overbid BOOLEAN,
    winner BOOLEAN,
    points INT,
    created_at TIMESTAMP DEFAULT NOW(),
    CONSTRAINT unique_r2_bid UNIQUE (item_id, player_id)
);

-- ============================================================
-- ROUND 3 - GLOBAL BONUS WHEEL
-- ============================================================

-- Wheel configuration (global, used by all matches)
CREATE TABLE r3_items (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    value INT,
    weight INT,
    active BOOLEAN,
    created_at TIMESTAMP DEFAULT NOW()
);

-- Wheel results per player per match
CREATE TABLE r3_results (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    round_id UUID REFERENCES rounds(id),
    player_id INT REFERENCES match_players(id),

    item1_id UUID REFERENCES r3_items(id),
    v1 INT,

    item2_id UUID REFERENCES r3_items(id),
    v2 INT,

    total INT,
    score INT,
    created_at TIMESTAMP DEFAULT NOW()
);

-- ============================================================
-- FAST PRESS (Tie-break)
-- ============================================================
CREATE TABLE fp_round (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    round_id UUID REFERENCES rounds(id),
    qid UUID REFERENCES questions(id),
    opt1 INT,
    opt2 INT,
    correct INT,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE fp_results (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    round_id UUID REFERENCES rounds(id),
    player_id INT REFERENCES match_players(id),
    before INT,
    after INT,
    rank INT,
    choice INT,
    created_at TIMESTAMP DEFAULT NOW()
);
