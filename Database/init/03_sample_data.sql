-- ===============================
-- SAMPLE DATA
-- ===============================

-- Insert test accounts
INSERT INTO accounts (email, password, role) VALUES
('test1@example.com', '$2a$10$placeholder1', 'user'),
('test2@example.com', '$2a$10$placeholder2', 'user'),
('test3@example.com', '$2a$10$placeholder3', 'user'),
('admin@example.com', '$2a$10$placeholderadmin', 'admin');

-- Insert profiles
INSERT INTO profiles (account_id, name, points, matches, wins) VALUES
(1, 'Player 1', 1000, 10, 3),
(2, 'Player 2', 850, 8, 2),
(3, 'Player 3', 1200, 15, 5),
(4, 'Admin User', 0, 0, 0);

-- Insert sample questions (MCQ type)
INSERT INTO questions (type, data) VALUES
('mcq', '{"content": "What is the typical price of a high-end smartphone?", "category": "electronics", "choices": ["$200", "$500", "$1000", "$1500"], "correct_answer": 2, "image_url": null}'),
('mcq', '{"content": "What is the average cost of a good coffee maker?", "category": "home", "choices": ["$30", "$75", "$150", "$300"], "correct_answer": 1, "image_url": null}'),
('mcq', '{"content": "What is a typical price for designer sunglasses?", "category": "fashion", "choices": ["$50", "$150", "$300", "$500"], "correct_answer": 2, "image_url": null}'),
('mcq', '{"content": "What is the price of a mid-range laptop?", "category": "electronics", "choices": ["$300", "$700", "$1200", "$2000"], "correct_answer": 1, "image_url": null}'),
('mcq', '{"content": "What does a quality mattress typically cost?", "category": "furniture", "choices": ["$200", "$500", "$1000", "$2000"], "correct_answer": 2, "image_url": null}');

-- Insert sample questions (BID type)
INSERT INTO questions (type, data) VALUES
('bid', '{"content": "Bid on the price of this vintage watch", "category": "lifestyle", "actual_price": 2500, "image_url": null}'),
('bid', '{"content": "What is the price of this gaming console?", "category": "electronics", "actual_price": 499, "image_url": null}'),
('bid', '{"content": "Bid on this designer handbag", "category": "fashion", "actual_price": 1800, "image_url": null}');

-- Insert sample questions (WHEEL type)
INSERT INTO questions (type, data) VALUES
('wheel', '{"content": "Spin the wheel for this luxury car!", "category": "automotive", "segments": [100, 500, 1000, 2000, 5000, 10000], "image_url": null}'),
('wheel', '{"content": "Spin for vacation package!", "category": "travel", "segments": [100, 250, 500, 1000, 2500, 5000], "image_url": null}');
