-- ===============================
-- ACCOUNTS
-- ===============================
CREATE TABLE accounts (
    id SERIAL PRIMARY KEY,
    email VARCHAR(100) UNIQUE NOT NULL,
    password VARCHAR(255),
    role VARCHAR(20) NOT NULL DEFAULT 'user',
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- ===============================
-- SESSIONS
-- ===============================
CREATE TABLE sessions (
    account_id INT PRIMARY KEY REFERENCES accounts(id) ON DELETE CASCADE,
    session_id UUID,
    connected BOOLEAN,
    updated_at TIMESTAMP DEFAULT NOW()
);

-- ===============================
-- PROFILES
-- ===============================
CREATE TABLE profiles (
    id SERIAL PRIMARY KEY,
    account_id INT REFERENCES accounts(id) ON DELETE CASCADE,
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

-- ===============================
-- FRIENDS
-- ===============================
CREATE TABLE friends (
    id SERIAL PRIMARY KEY,
    requester_id INT REFERENCES accounts(id),
    addressee_id INT REFERENCES accounts(id),
    status VARCHAR(20),
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW(),
    UNIQUE (requester_id, addressee_id)
);

-- ===============================
-- ROOMS
-- ===============================
CREATE TABLE rooms (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100),
    code VARCHAR(10) UNIQUE,
    visibility VARCHAR(20),
    host_id INT REFERENCES accounts(id),
    status VARCHAR(20),
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- ===============================
-- ROOM MEMBERS
-- ===============================
CREATE TABLE room_members (
    room_id INT REFERENCES rooms(id) ON DELETE CASCADE,
    account_id INT REFERENCES accounts(id) ON DELETE CASCADE,
    joined_at TIMESTAMP DEFAULT NOW(),
    PRIMARY KEY (room_id, account_id)
);

-- ===============================
-- MATCHES
-- ===============================
CREATE TABLE matches (
    id SERIAL PRIMARY KEY,
    room_id INT REFERENCES rooms(id),
    mode VARCHAR(20),
    max_players INT,
    advanced JSONB,
    round_time INT,
    started_at TIMESTAMP,
    ended_at TIMESTAMP
);

-- ===============================
-- MATCH PLAYERS
-- ===============================
CREATE TABLE match_players (
    id SERIAL PRIMARY KEY,
    match_id INT REFERENCES matches(id) ON DELETE CASCADE,
    account_id INT REFERENCES accounts(id),
    score INT DEFAULT 0,
    rank INT,
    eliminated BOOLEAN DEFAULT FALSE,
    forfeited BOOLEAN DEFAULT FALSE,
    winner BOOLEAN DEFAULT FALSE,
    joined_at TIMESTAMP DEFAULT NOW()
);

-- ===============================
-- MATCH QUESTIONS
-- ===============================
CREATE TABLE match_question (
    id SERIAL PRIMARY KEY,
    match_id INT REFERENCES matches(id) ON DELETE CASCADE,
    round_no INT,
    round_type VARCHAR(20),
    question_idx INT,
    question JSONB,
    created_at TIMESTAMP DEFAULT NOW(),
    UNIQUE (match_id, round_no, question_idx)
);

-- ===============================
-- MATCH ANSWERS
-- ===============================
CREATE TABLE match_answer (
    id SERIAL PRIMARY KEY,
    question_id INT REFERENCES match_question(id) ON DELETE CASCADE,
    player_id INT REFERENCES match_players(id),
    answer JSONB,
    score_delta INT,
    action_idx INT DEFAULT 1,
    created_at TIMESTAMP DEFAULT NOW(),
    UNIQUE (question_id, player_id, action_idx)
);

-- ===============================
-- QUESTIONS BANK
-- ===============================
CREATE TABLE questions (
    id SERIAL PRIMARY KEY,
    type VARCHAR(20),
    data JSONB,
    active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE match_events (
    id SERIAL PRIMARY KEY,

    match_id INT NOT NULL
        REFERENCES matches(id)
        ON DELETE CASCADE,

    player_id INT
        REFERENCES accounts(id)
        ON DELETE SET NULL,

    event_type VARCHAR(50) NOT NULL,
    round_no INT,
    question_idx INT,

    created_at TIMESTAMP DEFAULT NOW()
);
