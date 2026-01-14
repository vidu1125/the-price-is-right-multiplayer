# Kết nối PostgreSQL Database với DBeaver

## 📋 Thông tin kết nối

### Cấu hình từ docker-compose.yml:

| Thông tin | Giá trị |
|-----------|---------|
| **Host** | `localhost` |
| **Port** | `5432` |
| **Database** | `tpir` |
| **Username** | `postgresql` |
| **Password** | `password` |
| **Driver** | PostgreSQL |

---

## 🔧 Các bước kết nối DBeaver

### Bước 1: Mở DBeaver
- Khởi động DBeaver
- Click vào **Database** → **New Database Connection**

### Bước 2: Chọn Database Type
- Chọn **PostgreSQL**
- Click **Next**

### Bước 3: Điền thông tin kết nối

**Main Tab:**
```
Host:     localhost
Port:     5432
Database: tpir
Username: postgresql
Password: password
```

**Các tuỳ chọn khác (Optional):**
- **Show all databases**: ✅ (để xem tất cả databases)
- **Connect automatically**: ✅ (tự động kết nối khi mở DBeaver)

### Bước 4: Test Connection
- Click **Test Connection**
- Nếu yêu cầu, DBeaver sẽ tự động download PostgreSQL driver
- Nếu thành công sẽ hiện "Connected"

### Bước 5: Finish
- Click **Finish** để lưu connection

---

## 🗂️ Cấu trúc Database

Sau khi kết nối thành công, bạn sẽ thấy các tables sau:

### Core Tables:
- **accounts** - User accounts
- **sessions** - Active sessions
- **profiles** - User profiles

### Game Tables:
- **rooms** - Game rooms
- **room_members** - Room participants
- **matches** - Match records
- **match_players** - Player scores in matches
- **match_question** - Questions used in matches
- **match_answer** - Player answers
- **match_events** - Game events (forfeit, eliminated)

### Questions:
- **questions** - Question bank

---

## 🐳 Lưu ý Docker

### Kiểm tra PostgreSQL container đang chạy:
```bash
docker ps | grep postgres
```

Kết quả phải có:
```
tpir-postgres   postgres:15-alpine   0.0.0.0:5432->5432/tcp
```

### Nếu container chưa chạy:
```bash
cd /home/thowo/networkprog/the-price-is-right-multiplayer
docker-compose up -d postgres
```

### Kiểm tra logs của PostgreSQL:
```bash
docker logs tpir-postgres
```

### Kết nối trực tiếp qua psql (nếu cần):
```bash
docker exec -it tpir-postgres psql -U postgresql -d tpir
```

Các lệnh SQL hữu ích:
```sql
-- Xem tất cả tables
\dt

-- Xem structure của một table
\d accounts

-- Xem dữ liệu
SELECT * FROM accounts;
SELECT * FROM questions;
SELECT * FROM rooms;

-- Đếm số lượng records
SELECT COUNT(*) FROM accounts;
SELECT COUNT(*) FROM questions;
```

---

## 🔍 Quick SQL Queries

### Xem tất cả accounts:
```sql
SELECT id, email, role, created_at FROM accounts;
```

### Xem questions theo type:
```sql
SELECT id, type, data->>'category' as category 
FROM questions 
WHERE type = 'mcq';
```

### Xem active sessions:
```sql
SELECT s.*, a.email 
FROM sessions s
JOIN accounts a ON s.account_id = a.id
WHERE s.connected = true;
```

### Xem match history:
```sql
SELECT m.id, m.mode, m.started_at, m.ended_at,
       COUNT(mp.id) as player_count
FROM matches m
LEFT JOIN match_players mp ON m.id = mp.match_id
GROUP BY m.id
ORDER BY m.started_at DESC;
```

---

## ⚠️ Troubleshooting

### Lỗi: "Connection refused"
**Nguyên nhân:** PostgreSQL container chưa chạy
**Giải pháp:**
```bash
docker-compose up -d postgres
docker logs tpir-postgres
```

### Lỗi: "Password authentication failed"
**Nguyên nhân:** Sai password
**Giải pháp:** Đảm bảo password là `password` (chữ thường)

### Lỗi: "FATAL: role 'postgres' does not exist" hoặc "FATAL: role 'postgresql' does not exist"
**Nguyên nhân:** 
- Sai username trong DBeaver, HOẶC
- Volume PostgreSQL cũ đã được tạo với username khác (volume lưu trữ dữ liệu database)

**Giải pháp:**

**Bước 1: Kiểm tra username trong DBeaver**
- Đảm bảo **Username** là `postgresql` (không phải `postgres`)
- Username đúng: `postgresql`
- Password: `password`
- Database: `tpir`

**Bước 2: Nếu vẫn lỗi, xóa volume cũ và tạo lại container**
```bash
# Dừng và xóa container + volume
docker-compose down -v

# Xóa volume thủ công (nếu cần)
docker volume rm the-price-is-right-multiplayer_postgres_data

# Tạo lại container với cấu hình mới
docker-compose up -d postgres

# Kiểm tra logs để đảm bảo container khởi động đúng
docker logs tpir-postgres
```

**Bước 3: Giải pháp thay thế (nếu muốn giữ dữ liệu)**
Nếu bạn muốn giữ dữ liệu hiện tại, có thể tạo user mới trong database:
```bash
# Kết nối vào container với user hiện có (thường là 'postgres')
docker exec -it tpir-postgres psql -U postgres -d tpir

# Hoặc nếu user là 'postgresql' nhưng database khác
docker exec -it tpir-postgres psql -U postgres

# Trong psql, tạo user mới:
CREATE USER postgresql WITH PASSWORD 'password';
ALTER USER postgresql CREATEDB;
GRANT ALL PRIVILEGES ON DATABASE tpir TO postgresql;
\q
```

**Lưu ý:** 
- Việc xóa volume (Bước 2) sẽ **XÓA TẤT CẢ DỮ LIỆU** trong database. Chỉ làm khi bạn chấp nhận mất dữ liệu hoặc đang trong giai đoạn development.
- Giải pháp Bước 3 chỉ hoạt động nếu bạn biết user hiện tại của database (thường là `postgres`).

### Lỗi: "Port 5432 already in use"
**Nguyên nhân:** Có PostgreSQL instance khác đang chạy trên port 5432
**Giải pháp:**
1. **Option 1:** Dừng PostgreSQL local: `sudo service postgresql stop`
2. **Option 2:** Đổi port trong docker-compose.yml: `"5433:5432"`

### Database trống hoặc thiếu tables
**Nguyên nhân:** Init scripts chưa chạy
**Giải pháp:**
```bash
# Xoá volume cũ và rebuild
docker-compose down -v
docker-compose up --build
```

---

## 📊 Thêm dữ liệu test (nếu cần)

Nếu database trống, chạy các SQL sau trong DBeaver:

### Insert test account:
```sql
INSERT INTO accounts (email, password, role) 
VALUES ('test@example.com', '$2a$10$hashedpassword', 'user')
RETURNING id;
```

### Insert test profile:
```sql
INSERT INTO profiles (account_id, name, points, matches, wins)
VALUES (1, 'Test Player', 1000, 10, 3);
```

### View sample data:
```sql
SELECT * FROM accounts;
SELECT * FROM questions LIMIT 5;
```

---

**Connection String (nếu cần):**
```
jdbc:postgresql://localhost:5432/tpir?user=postgresql&password=password
```

**pgAdmin URI:**
```
postgresql://postgresql:password@localhost:5432/tpir
```
