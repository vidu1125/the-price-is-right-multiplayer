from flask import Flask, jsonify, request
from flask_cors import CORS
from app.models import db
import os

app = Flask(__name__)
CORS(app)

# ==================== DATABASE CONFIGURATION ====================
app.config['SQLALCHEMY_DATABASE_URI'] = os.getenv(
    'DATABASE_URL',
    'postgresql://gameuser:gamepass@db:5432/tpir_db'
)
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
app.config['SQLALCHEMY_ECHO'] = True  # Log SQL queries

# Initialize database
db.init_app(app)

# ==================== REGISTER BLUEPRINTS ====================
from app.routes.room_routes import room_bp
app.register_blueprint(room_bp)

# ==================== EXISTING CODE ====================
@app.route('/health')
def health():
    return jsonify({'status': 'healthy', 'service': 'backend'}), 200

@app.route('/api/game')
def game():
    return jsonify({'rounds': 5, 'mode': 'elimination'}), 200

# ==================== CREATE TABLES ====================
with app.app_context():
    try:
        db.create_all()
        print("✅ Database tables created successfully")
    except Exception as e:
        print(f"❌ Error creating tables: {e}")

if __name__ == '__main__':
    print("🚀 Backend API starting on port 5000...")
    app.run(host='0.0.0.0', port=5000, debug=True)