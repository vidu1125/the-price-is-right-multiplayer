from flask import Blueprint, jsonify
from app.models import db

db_bp = Blueprint("db", __name__, url_prefix="/health")

@db_bp.route("/db")
def health_db():
    try:
        db.session.execute(db.text("SELECT 1"))
        return jsonify({"db": "connected"}), 200
    except Exception as e:
        return jsonify({"db": "error", "detail": str(e)}), 500
