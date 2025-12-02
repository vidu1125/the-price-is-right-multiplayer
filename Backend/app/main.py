from flask import Flask, jsonify
from flask_cors import CORS
from flask_sqlalchemy import SQLAlchemy
from dotenv import load_dotenv
import os

# Load environment variables from .env
load_dotenv()

app = Flask(__name__)
CORS(app)

# ==========================
# Database configuration
# ==========================
DATABASE_URL = os.getenv("DATABASE_URL")

if not DATABASE_URL:
    raise Exception("❌ DATABASE_URL is missing in Backend/.env")

# Fix legacy postgres:// scheme
if DATABASE_URL.startswith("postgres://"):
    DATABASE_URL = DATABASE_URL.replace("postgres://", "postgresql://", 1)

app.config["SQLALCHEMY_DATABASE_URI"] = DATABASE_URL
app.config["SQLALCHEMY_TRACK_MODIFICATIONS"] = False

# Initialize SQLAlchemy
db = SQLAlchemy(app)

# ==========================
# Routes
# ==========================

@app.route('/health')
def health():
    return jsonify({'status': 'healthy'}), 200


@app.route('/health/db')
def health_db():
    try:
        # Simple query to check Supabase connection
        db.session.execute(db.text("SELECT 1"))
        return jsonify({"db": "connected"}), 200
    except Exception as e:
        return jsonify({"db": "error", "details": str(e)}), 500


@app.route('/api/game')
def game():
    return jsonify({'rounds': 5, 'mode': 'elimination'}), 200


# ==========================
# Entry Point
# ==========================
if __name__ == '__main__':
    print("Backend API starting on port 5000...")
    app.run(host='0.0.0.0', port=5000, debug=True)
