import os
import time
from dotenv import load_dotenv
from flask import Flask
from flask_cors import CORS
from sqlalchemy.exc import OperationalError

from app.models import db
from app.routes import register_routes

load_dotenv()

def create_app():
    app = Flask(__name__)
    CORS(app, resources={r"/api/*": {"origins": "*"}})

    # ================= CONFIG =================
    DATABASE_URL = os.getenv("DATABASE_URL") or \
        "postgresql://gameuser:gamepass@db:5432/tpir_db"

    if DATABASE_URL.startswith("postgres://"):
        DATABASE_URL = DATABASE_URL.replace("postgres://", "postgresql://", 1)

    app.config.update(
        SQLALCHEMY_DATABASE_URI=DATABASE_URL,
        SQLALCHEMY_TRACK_MODIFICATIONS=False,
        SQLALCHEMY_ECHO=False,
    )

    # ================= INIT =================
    db.init_app(app)
    register_routes(app)

    return app


def init_db(app):
    with app.app_context():
        max_retries = 10
        for i in range(max_retries):
            try:
                db.create_all()
                print("✅ Database ready")
                return
            except OperationalError:
                print(f"⏳ DB not ready ({i+1}/{max_retries})")
                time.sleep(2)


if __name__ == "__main__":
    print("🚀 Backend API starting on port 5000...")
    app = create_app()
    init_db(app)
    app.run(host="0.0.0.0", port=5000, debug=True, use_reloader=False)
