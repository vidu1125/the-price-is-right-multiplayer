CREATE TABLE accounts (
    id SERIAL PRIMARY KEY,
    email VARCHAR(100) UNIQUE NOT NULL,
    password VARCHAR(255),
    role VARCHAR(20) NOT NULL DEFAULT 'user',
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE sessions (
    account_id INT PRIMARY KEY
        REFERENCES accounts(id)
        ON DELETE CASCADE,
    session_id UUID,
    connected BOOLEAN,
    updated_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE profiles (
    id SERIAL PRIMARY KEY,
    account_id INT
        REFERENCES accounts(id)
        ON DELETE CASCADE,
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

CREATE TABLE friends (
    id SERIAL PRIMARY KEY,
    requester_id INT
        REFERENCES accounts(id)
        ON DELETE CASCADE,
    addressee_id INT
        REFERENCES accounts(id)
        ON DELETE CASCADE,
    status VARCHAR(20),
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW(),
    UNIQUE (requester_id, addressee_id)
);

CREATE TABLE rooms (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100),
    code VARCHAR(10) UNIQUE,
    visibility VARCHAR(20),
    host_id INT
        REFERENCES accounts(id),
    wager_mode BOOLEAN DEFAULT FALSE,
    max_players INT,
    mode VARCHAR,
    status VARCHAR(20),
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE room_members (
    room_id INT
        REFERENCES rooms(id)
        ON DELETE CASCADE,
    account_id INT
        REFERENCES accounts(id)
        ON DELETE CASCADE,
    joined_at TIMESTAMP DEFAULT NOW(),
    PRIMARY KEY (room_id, account_id)
);

CREATE TABLE matches (
    id SERIAL PRIMARY KEY,
    room_id INT
        REFERENCES rooms(id),
    mode VARCHAR(20),
    max_players INT,
    advanced JSONB,
    started_at TIMESTAMP,
    ended_at TIMESTAMP
);

CREATE TABLE match_players (
    id SERIAL PRIMARY KEY,
    match_id INT
        REFERENCES matches(id)
        ON DELETE CASCADE,
    account_id INT
        REFERENCES accounts(id),
    score INT DEFAULT 0,
    eliminated BOOLEAN DEFAULT FALSE,
    forfeited BOOLEAN DEFAULT FALSE,
    winner BOOLEAN DEFAULT FALSE,
    rank INT,
    joined_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE match_question (
    id SERIAL PRIMARY KEY,
    match_id INT
        REFERENCES matches(id)
        ON DELETE CASCADE,
    round_no INT,                 -- 1 | 2 | 3 | 4
    round_type VARCHAR(20),       -- MCQ | FILL | WHEEL | BONUS
    question_idx INT,             -- index trong round
    question JSONB,               -- snapshot câu hỏi
    created_at TIMESTAMP DEFAULT NOW(),
    UNIQUE (match_id, round_no, question_idx)
);

CREATE TABLE match_answer (
    id SERIAL PRIMARY KEY,
    question_id INT
        REFERENCES match_question(id)
        ON DELETE CASCADE,
    player_id INT
        REFERENCES match_players(id)
        ON DELETE CASCADE,
    answer JSONB,                 -- choice / value / spin
    score_delta INT,
    action_idx INT DEFAULT 1,     -- lượt trả lời / lượt quay
    created_at TIMESTAMP DEFAULT NOW(),
    UNIQUE (question_id, player_id, action_idx)
);

CREATE TABLE questions (
    id SERIAL PRIMARY KEY,
    type VARCHAR(20),             -- MCQ | PRICE | WHEEL
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
