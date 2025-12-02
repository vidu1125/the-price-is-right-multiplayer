import sys
import os
import time
from sqlalchemy.exc import OperationalError

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from flask import Flask, jsonify
from flask_cors import CORS
from app.models import db

# ⭐ Tạo Flask app
app = Flask(__name__)
CORS(app, resources={r"/api/*": {"origins": "*"}})

# ==================== DATABASE CONFIGURATION ====================
app.config['SQLALCHEMY_DATABASE_URI'] = os.getenv(
    'DATABASE_URL',
    'postgresql://gameuser:gamepass@db:5432/tpir_db'
)
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
app.config['SQLALCHEMY_ECHO'] = False  # Tắt log SQL để dễ đọc

# Initialize database
db.init_app(app)

# ⭐ Import models TRỰC TIẾP (KHÔNG dùng import app.models)
from app.models import Account, Profile, Room, RoomMember, Match, MatchSettings, MatchPlayer, Round

# ==================== REGISTER BLUEPRINTS ====================
from app.routes.room_routes import room_bp
app.register_blueprint(room_bp)

# ==================== HEALTH CHECK ====================
@app.route('/health')
def health():
    return jsonify({'status': 'healthy', 'service': 'backend'}), 200

@app.route('/api/game')
def game():
    return jsonify({'rounds': 5, 'mode': 'elimination'}), 200

# ==================== INIT DB FUNCTION ====================
def init_db():
    """Khởi tạo Database với retry logic"""
    with app.app_context():
        max_retries = 10
        for attempt in range(max_retries):
            try:
                db.create_all()
                print("✅ Database tables created successfully")

                # Seed test account
                if not Account.query.get(1):
                    test_account = Account(
                        id=1,
                        email='test@example.com',
                        password='hashed_password',
                        provider='local',
                        status='active'
                    )
                    db.session.add(test_account)
                    db.session.flush()

                    test_profile = Profile(
                        account_id=1,
                        name='Test User',
                        bio='Test account for development'
                    )
                    db.session.add(test_profile)
                    db.session.commit()
                    print("✅ Seeded Account id=1 with profile")
                else:
                    print("ℹ️  Account id=1 already exists")

                return True

            except OperationalError:
                if attempt == max_retries - 1:
                    print(f"❌ Could not connect to database after {max_retries} attempts.")
                    return False
                print(f"⏳ Database not ready, retrying in 2s... ({attempt+1}/{max_retries})")
                time.sleep(2)

            except Exception as e:
                print(f"❌ Error creating tables: {e}")
                import traceback
                traceback.print_exc()
                return False

# ==================== MAIN ENTRY ====================
if __name__ == '__main__':
    print("🚀 Backend API starting on port 5000...")
    init_db()
    app.run(host='0.0.0.0', port=5000, debug=True, use_reloader=False)