-- ===============================
-- BOT ACCOUNTS
-- ===============================
INSERT INTO accounts (email, password, role) VALUES 
('test1@example.com', '$2a$10$placeholder1', 'user'),
('test2@example.com', '$2a$10$placeholder2', 'user'),
('test3@example.com', '$2a$10$placeholder3', 'user'),
('bot2@game.com', '$2b$10$paddinghashfortestbot2xxxxxxxxx', 'user'),
('bot3@game.com', '$2b$10$paddinghashfortestbot3xxxxxxxxx', 'user'),
('bot4@game.com', '$2b$10$paddinghashfortestbot4xxxxxxxxx', 'user')
ON CONFLICT (email) DO NOTHING;

-- ===============================
-- PROFILES
-- ===============================
INSERT INTO profiles (account_id, name, points, avatar) VALUES
(1, 'Player 1', 1000, 'ava1.jpg'),
(2, 'Player 2', 850, 'ava2.jpg'),
(3, 'Player 3', 1200, 'ava3.jpg')
ON CONFLICT DO NOTHING;

-- ===============================
-- QUESTIONS SEED
-- ===============================
INSERT INTO questions (type, data)
VALUES (
    'wheel',
    '{
      "label": "classic_wheel",
      "segments": [0, 10, 20, 30, 40, 50, -20, 100]
    }'
);


INSERT INTO questions (type, data) VALUES
('mcq', '{
  "type": "lifestyle",
  "question": "Một bộ chăn ga cotton 1m6x2m có giá khoảng bao nhiêu?",
  "choices": ["599.000", "899.000", "1.199.000", "1.599.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "lifestyle",
  "question": "Gối cao su non tiêu chuẩn có giá khoảng bao nhiêu?",
  "choices": ["199.000", "299.000", "399.000", "599.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "lifestyle",
  "question": "Nệm bông ép 1m6x2m có giá khoảng bao nhiêu?",
  "choices": ["1.990.000", "2.990.000", "3.990.000", "4.990.000"],
  "correct_answer": 1
}'),
('mcq', '{
  "type": "lifestyle",
  "question": "Nồi cơm điện 1.8L phổ thông có giá khoảng bao nhiêu?",
  "choices": ["699.000", "999.000", "1.299.000", "1.599.000"],
  "correct_answer": 1
}'),
('mcq', '{
  "type": "lifestyle",
  "question": "Nồi chiên không dầu 5–6 lít có giá khoảng bao nhiêu?",
  "choices": ["1.290.000", "1.690.000", "2.290.000", "2.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "lifestyle",
  "question": "Bộ dao nhà bếp inox 5 món có giá khoảng bao nhiêu?",
  "choices": ["299.000", "499.000", "699.000", "999.000"],
  "correct_answer": 1
}'),
('mcq', '{
  "type": "lifestyle",
  "question": "Máy xay sinh tố gia đình có giá khoảng bao nhiêu?",
  "choices": ["399.000", "699.000", "999.000", "1.399.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "lifestyle",
  "question": "Ấm siêu tốc inox 1.7L có giá khoảng bao nhiêu?",
  "choices": ["299.000", "499.000", "699.000", "899.000"],
  "correct_answer": 1
}'),
('mcq', '{
  "type": "lifestyle",
  "question": "Chảo chống dính 26cm có giá khoảng bao nhiêu?",
  "choices": ["199.000", "299.000", "399.000", "599.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "lifestyle",
  "question": "Hộp đựng thực phẩm thủy tinh bộ 5 hộp có giá khoảng bao nhiêu?",
  "choices": ["299.000", "499.000", "699.000", "899.000"],
  "correct_answer": 1
}');


INSERT INTO questions (type, data) VALUES
('mcq', '{
  "type": "furniture",
  "question": "Sofa vải 3 chỗ ngồi có giá khoảng bao nhiêu?",
  "choices": ["6.990.000", "8.990.000", "11.990.000", "14.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "furniture",
  "question": "Giường ngủ gỗ công nghiệp 1m6x2m có giá khoảng bao nhiêu?",
  "choices": ["3.990.000", "5.990.000", "7.990.000", "9.990.000"],
  "correct_answer": 1
}'),
('mcq', '{
  "type": "furniture",
  "question": "Bàn ăn gỗ 4 ghế có giá khoảng bao nhiêu?",
  "choices": ["4.990.000", "6.990.000", "8.990.000", "10.990.000"],
  "correct_answer": 1
}'),
('mcq', '{
  "type": "furniture",
  "question": "Tủ quần áo gỗ MDF 3 cánh có giá khoảng bao nhiêu?",
  "choices": ["5.990.000", "7.990.000", "9.990.000", "11.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "furniture",
  "question": "Kệ TV phòng khách loại phổ thông có giá khoảng bao nhiêu?",
  "choices": ["1.990.000", "2.990.000", "3.990.000", "4.990.000"],
  "correct_answer": 1
}'),
('mcq', '{
  "type": "furniture",
  "question": "Bàn làm việc gỗ công nghiệp có giá khoảng bao nhiêu?",
  "choices": ["1.290.000", "1.990.000", "2.990.000", "3.990.000"],
  "correct_answer": 1
}'),
('mcq', '{
  "type": "furniture",
  "question": "Ghế văn phòng xoay lưới có giá khoảng bao nhiêu?",
  "choices": ["999.000", "1.490.000", "1.990.000", "2.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "furniture",
  "question": "Tủ giày gỗ 3 tầng có giá khoảng bao nhiêu?",
  "choices": ["799.000", "1.199.000", "1.599.000", "1.999.000"],
  "correct_answer": 1
}'),
('mcq', '{
  "type": "furniture",
  "question": "Kệ sách đứng 5 tầng có giá khoảng bao nhiêu?",
  "choices": ["699.000", "1.099.000", "1.499.000", "1.999.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "furniture",
  "question": "Bàn trà phòng khách có giá khoảng bao nhiêu?",
  "choices": ["1.290.000", "1.990.000", "2.990.000", "3.990.000"],
  "correct_answer": 1
}');


INSERT INTO questions (type, data) VALUES
('mcq', '{
  "type": "electronics",
  "question": "Giá của iPhone 15 Pro tại Việt Nam là bao nhiêu?",
  "choices": ["25.990.000", "27.990.000", "29.990.000", "31.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "electronics",
  "question": "Giá MacBook Air M2 tại Việt Nam khoảng bao nhiêu?",
  "choices": ["24.990.000", "27.990.000", "29.990.000", "32.990.000"],
  "correct_answer": 1
}'),
('mcq', '{
  "type": "electronics",
  "question": "Samsung Galaxy S23 có giá khoảng bao nhiêu?",
  "choices": ["19.990.000", "21.990.000", "23.990.000", "25.990.000"],
  "correct_answer": 1
}'),
('mcq', '{
  "type": "electronics",
  "question": "Giá PlayStation 5 tại Việt Nam là bao nhiêu?",
  "choices": ["10.990.000", "12.990.000", "14.990.000", "16.990.000"],
  "correct_answer": 1
}'),
('mcq', '{
  "type": "electronics",
  "question": "Nintendo Switch OLED có giá khoảng bao nhiêu?",
  "choices": ["6.990.000", "7.990.000", "8.990.000", "9.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "electronics",
  "question": "AirPods Pro (Gen 2) giá khoảng bao nhiêu?",
  "choices": ["4.990.000", "5.490.000", "6.190.000", "6.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "electronics",
  "question": "Apple Watch Series 9 có giá từ bao nhiêu?",
  "choices": ["9.990.000", "10.990.000", "11.990.000", "12.990.000"],
  "correct_answer": 1
}'),
('mcq', '{
  "type": "electronics",
  "question": "iPad Pro M2 11 inch có giá khoảng bao nhiêu?",
  "choices": ["18.990.000", "20.990.000", "22.990.000", "24.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "electronics",
  "question": "Dell XPS 13 có giá khoảng bao nhiêu?",
  "choices": ["29.990.000", "32.990.000", "35.990.000", "38.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "electronics",
  "question": "Tai nghe Sony WH-1000XM5 có giá bao nhiêu?",
  "choices": ["7.990.000", "8.990.000", "9.990.000", "10.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "electronics",
  "question": "Canon EOS R50 có giá khoảng bao nhiêu?",
  "choices": ["15.990.000", "17.990.000", "19.990.000", "21.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "electronics",
  "question": "GoPro Hero 12 có giá khoảng bao nhiêu?",
  "choices": ["9.990.000", "10.990.000", "11.990.000", "12.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "electronics",
  "question": "TV LG OLED 55 inch có giá khoảng bao nhiêu?",
  "choices": ["29.990.000", "34.990.000", "39.990.000", "44.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "electronics",
  "question": "Máy hút bụi Dyson V15 có giá bao nhiêu?",
  "choices": ["15.990.000", "17.990.000", "19.990.000", "21.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "electronics",
  "question": "Loa JBL Charge 5 có giá khoảng bao nhiêu?",
  "choices": ["3.990.000", "4.490.000", "4.990.000", "5.490.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "electronics",
  "question": "Loa Bose SoundLink có giá khoảng bao nhiêu?",
  "choices": ["5.990.000", "6.990.000", "7.990.000", "8.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "electronics",
  "question": "Asus ROG Phone có giá khoảng bao nhiêu?",
  "choices": ["19.990.000", "22.990.000", "25.990.000", "28.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "electronics",
  "question": "Surface Pro 9 có giá khoảng bao nhiêu?",
  "choices": ["23.990.000", "26.990.000", "29.990.000", "32.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "electronics",
  "question": "Xiaomi 13 Pro có giá khoảng bao nhiêu?",
  "choices": ["19.990.000", "21.990.000", "23.990.000", "25.990.000"],
  "correct_answer": 2
}'),
('mcq', '{
  "type": "electronics",
  "question": "Oculus Quest 3 có giá khoảng bao nhiêu?",
  "choices": ["11.990.000", "12.990.000", "13.990.000", "14.990.000"],
  "correct_answer": 2
}');


INSERT INTO questions (type, data) VALUES
('bid', '{"type":"lifestyle","question":"Giá gốc của Instant Pot Duo 7-in-1 (USD)?","correct_answer":99}'),
('bid', '{"type":"lifestyle","question":"Giá gốc của Ninja Air Fryer AF101 (USD)?","correct_answer":129}'),
('bid', '{"type":"lifestyle","question":"Giá gốc của Vitamix Explorian Blender (USD)?","correct_answer":349}'),
('bid', '{"type":"lifestyle","question":"Giá gốc của Tefal Non-stick Cookware Set (USD)?","correct_answer":199}'),
('bid', '{"type":"lifestyle","question":"Giá gốc của Dyson Supersonic Hair Dryer (USD)?","correct_answer":429}'),
('bid', '{"type":"lifestyle","question":"Giá gốc của Braun Silk-épil 9 (USD)?","correct_answer":149}'),
('bid', '{"type":"lifestyle","question":"Giá gốc của Philips Electric Kettle (USD)?","correct_answer":59}'),
('bid', '{"type":"lifestyle","question":"Giá gốc của Xiaomi Electric Toothbrush (USD)?","correct_answer":49}'),
('bid', '{"type":"lifestyle","question":"Giá gốc của IKEA Bedding Set Queen Size (USD)?","correct_answer":129}'),
('bid', '{"type":"lifestyle","question":"Giá gốc của Joseph Joseph Kitchen Organizer Set (USD)?","correct_answer":89}');


INSERT INTO questions (type, data) VALUES
('bid', '{"type":"electronics","question":"Giá gốc của Kindle Paperwhite (USD)?","correct_answer":139}'),
('bid', '{"type":"electronics","question":"Giá gốc của Logitech MX Master 3S (USD)?","correct_answer":99}'),
('bid', '{"type":"electronics","question":"Giá gốc của Samsung T7 SSD 1TB (USD)?","correct_answer":99}'),
('bid', '{"type":"electronics","question":"Giá gốc của Philips Hue Starter Kit (USD)?","correct_answer":199}'),
('bid', '{"type":"electronics","question":"Giá gốc của Anker PowerCore 20000mAh (USD)?","correct_answer":59}'),
('bid', '{"type":"electronics","question":"Giá gốc của Amazon Echo Dot (5th Gen) (USD)?","correct_answer":49}'),
('bid', '{"type":"electronics","question":"Giá gốc của Xiaomi Mi Smart Air Fryer (USD)?","correct_answer":129}'),
('bid', '{"type":"electronics","question":"Giá gốc của Logitech C920 Webcam (USD)?","correct_answer":79}'),
('bid', '{"type":"electronics","question":"Giá gốc của Razer BlackWidow Mechanical Keyboard (USD)?","correct_answer":139}'),
('bid', '{"type":"electronics","question":"Giá gốc của DJI Osmo Mobile 6 (USD)?","correct_answer":159}');


INSERT INTO questions (type, data) VALUES
('bid', '{"type":"furniture","question":"Giá gốc của IKEA MALM Bed Frame (USD)?","correct_answer":299}'),
('bid', '{"type":"furniture","question":"Giá gốc của IKEA BILLY Bookcase (USD)?","correct_answer":79}'),
('bid', '{"type":"furniture","question":"Giá gốc của IKEA LINNMON Desk (USD)?","correct_answer":99}'),
('bid', '{"type":"furniture","question":"Giá gốc của IKEA POÄNG Armchair (USD)?","correct_answer":149}'),
('bid', '{"type":"furniture","question":"Giá gốc của IKEA KALLAX Shelf Unit (USD)?","correct_answer":89}'),
('bid', '{"type":"furniture","question":"Giá gốc của Herman Miller Sayl Chair (USD)?","correct_answer":725}'),
('bid', '{"type":"furniture","question":"Giá gốc của Secretlab Titan Evo Chair (USD)?","correct_answer":549}'),
('bid', '{"type":"furniture","question":"Giá gốc của IKEA LACK Coffee Table (USD)?","correct_answer":59}'),
('bid', '{"type":"furniture","question":"Giá gốc của IKEA PAX Wardrobe Frame (USD)?","correct_answer":399}'),
('bid', '{"type":"furniture","question":"Giá gốc của IKEA EKTORP Sofa (USD)?","correct_answer":699}');
