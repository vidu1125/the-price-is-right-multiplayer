import sys
import os
import time
from sqlalchemy.exc import OperationalError

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from flask import Flask, jsonify
from flask_cors import CORS
from dotenv import load_dotenv

load_dotenv()

# ========================== FLASK APP ==========================
app = Flask(__name__)
CORS(app, resources={r"/api/*": {"origins": "*"}})

# ========================== DATABASE CONFIG ==========================
from app.models import db  # db = SQLAlchemy() được định nghĩa trong models/__init__.py

DATABASE_URL = os.getenv("DATABASE_URL")
if not DATABASE_URL:
    # fallback giống nhánh main
    DATABASE_URL = "postgresql://gameuser:gamepass@db:5432/tpir_db"

# Fix legacy scheme
if DATABASE_URL.startswith("postgres://"):
    DATABASE_URL = DATABASE_URL.replace("postgres://", "postgresql://", 1)

app.config["SQLALCHEMY_DATABASE_URI"] = DATABASE_URL
app.config["SQLALCHEMY_TRACK_MODIFICATIONS"] = False
app.config["SQLALCHEMY_ECHO"] = False

# Initialize SQLAlchemy
db.init_app(app)

# ========================== IMPORT MODELS ==========================
# (giữ logic từ nhánh main)
from app.models import (
    Account, Profile, Room, RoomMember,
    Match, MatchPlayer
)

# ========================== REGISTER BLUEPRINTS ==========================
from app.routes.room_routes import room_bp
app.register_blueprint(room_bp)

# ========================== HEALTH CHECKS ==========================
@app.route('/health')
def health():
    return jsonify({'status': 'healthy', 'service': 'backend'}), 200


@app.route('/health/db')
def health_db():
    try:
        db.session.execute(db.text("SELECT 1"))
        return jsonify({"db": "connected"}), 200
    except Exception as e:
        return jsonify({"db": "error", "details": str(e)}), 500


@app.route('/api/game')
def game():
    return jsonify({'rounds': 5, 'mode': 'elimination'}), 200


# ========================== INIT DB FUNCTION (giữ từ main) ==========================
def init_db():
    """Khởi tạo Database với retry logic"""
    with app.app_context():
        max_retries = 10
        for attempt in range(max_retries):
            try:
                db.create_all()
                print("✅ Database tables created successfully")

                # Seed test account (logic từ main)
                if not db.session.get(Account, 1):
                    test_account = Account(
                        id=1,
                        email='test@example.com',
                        password='hashed_password'
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

# ========================== MAIN ENTRY ==========================
if __name__ == '__main__':
    print("🚀 Backend API starting on port 5000...")
    init_db()
    app.run(host='0.0.0.0', port=5000, debug=True, use_reloader=False)
