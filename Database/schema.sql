///////////////////////////////////////////////////////////////
// ACCOUNTS
///////////////////////////////////////////////////////////////
Table accounts {
  id int [pk, increment]
  email varchar(100) [unique, not null]
  password varchar(255)
  provider varchar(20)
  role varchar(20) [not null, default: 'user']
  status varchar(20)
  created_at timestamp
  updated_at timestamp
}

///////////////////////////////////////////////////////////////
// SESSIONS
///////////////////////////////////////////////////////////////
Table sessions {
  account_id int [pk, ref: > accounts.id]
  session_id uuid  
  connected boolean
  updated_at timestamp
}

///////////////////////////////////////////////////////////////
// PROFILES
///////////////////////////////////////////////////////////////
Table profiles {
  id int [pk, increment]
  account_id int [ref: > accounts.id]
  name varchar(50)
  avatar text
  bio text
  matches int
  wins int
  points int
  badges jsonb
  created_at timestamp
  updated_at timestamp
}

///////////////////////////////////////////////////////////////
// FRIENDS
///////////////////////////////////////////////////////////////
Table friends {
  id int [pk, increment]
  requester_id int [ref: > accounts.id]
  addressee_id int [ref: > accounts.id]
  status varchar(20)
  created_at timestamp
  updated_at timestamp
  Note: "UNIQUE (requester_id, addressee_id)"
}

///////////////////////////////////////////////////////////////
// ROOMS
///////////////////////////////////////////////////////////////
Table rooms {
  id int [pk, increment]
  name varchar(100)
  visibility varchar(20)
  host_id int [ref: > accounts.id]
  status varchar(20)
  created_at timestamp
  updated_at timestamp
}

///////////////////////////////////////////////////////////////
// ROOM MEMBERS
///////////////////////////////////////////////////////////////
Table room_members {
  room_id int [ref: > rooms.id]
  account_id int [ref: > accounts.id]
  ready boolean [default: false]
  joined_at timestamp
  Note: "PRIMARY KEY (room_id, account_id)"
}

///////////////////////////////////////////////////////////////
// MATCHES
///////////////////////////////////////////////////////////////
Table matches {
  id int [pk, increment]
  room_id int [ref: > rooms.id]
  mode varchar(20)
  max_players int
  wager boolean
  round_time int
  started_at timestamp
  ended_at timestamp
}

///////////////////////////////////////////////////////////////
// MATCH PLAYERS
///////////////////////////////////////////////////////////////
Table match_players {
  id int [pk, increment]
  match_id int [ref: > matches.id]
  account_id int [ref: > accounts.id]
  score int
  eliminated boolean
  forfeited boolean
  winner boolean
  joined_at timestamp
}

///////////////////////////////////////////////////////////////
// ROUNDS
///////////////////////////////////////////////////////////////
Table rounds {
  id int [pk, increment]
  match_id int [ref: > matches.id]
  number int
  type varchar(20)
  created_at timestamp

}

///////////////////////////////////////////////////////////////
// ROUND 1 — QUIZ
///////////////////////////////////////////////////////////////
Table questions {
  id int [pk, increment]
  text text
  image text
  a text
  b text
  c text
  d text
  correct varchar(1)
  active boolean
  last_updated_by int [ref: > accounts.id]
  last_updated_at timestamp
}

Table r1_items {
  id int [pk, increment]
  round_id int [ref: > rounds.id]
  idx int
  text text
  image text
  a text
  b text
  c text
  d text
  correct varchar(1)
}

Table r1_answers {
  id int [pk, increment]
  item_id int [ref: > r1_items.id]
  player_id int [ref: > match_players.id]
  ans varchar(1)
  time_ms int
  correct boolean
  points int
  Note: "UNIQUE (item_id, player_id)"
}

///////////////////////////////////////////////////////////////
// ROUND 2 — BIDDING
///////////////////////////////////////////////////////////////
Table products {
  id int [pk, increment]
  name varchar(255)
  description text
  image varchar(255)
  price int
  active boolean
  last_updated_by int [ref: > accounts.id]
  last_updated_at timestamp
}

Table r2_items {
  id int [pk, increment]
  round_id int [ref: > rounds.id]
  name varchar(255)
  description text
  image varchar(255)
  price int
  idx int
}

Table r2_answers {
  id int [pk, increment]
  item_id int [ref: > r2_items.id]
  player_id int [ref: > match_players.id]
  bid int
  overbid boolean
  points int
  wager boolean
  Note: "UNIQUE (item_id, player_id)"
}

///////////////////////////////////////////////////////////////
// ROUND 3 — BONUS WHEEL
///////////////////////////////////////////////////////////////
Table wheel {
  id int [pk, increment]
  value int
  last_updated_by int [ref: > accounts.id]
  last_updated_at timestamp
}

Table r3_items {
  id int [pk, increment]
  round_id int [ref: > rounds.id]
  value int
}

Table r3_answer {
  id int [pk, increment]
  round_id int [ref: > rounds.id]

  player_id int [ref: > match_players.id]
  
  spin1_value int
  spin2_value int [null]
  points int
}

///////////////////////////////////////////////////////////////
// FAST PRESS
///////////////////////////////////////////////////////////////
Table fp_round {
  id int [pk, increment]
  round_id int [ref: > rounds.id]

  text text

  correct int        
  wrong int         
}


Table fp_answers {
  id int [pk, increment]
  round_id int [ref: > rounds.id]
  player_id int [ref: > match_players.id]

  choice int       
  correct boolean   

  score int
}

///////////////////////////////////////////////////////////////
// PLAYER MATCH HISTORY
///////////////////////////////////////////////////////////////
Table player_match_history {
  id int [pk, increment]
  player_id int [ref: > accounts.id]
  match_id int [ref: > matches.id]
  created_at timestamp
}


