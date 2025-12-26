from flask import Blueprint, jsonify
from app.models import db

health_bp = Blueprint("health", __name__, url_prefix="/health")

@health_bp.route("/db")
def db_check():
    try:
        db.session.execute(db.text("SELECT 1"))
        return jsonify({"db": "connected"}), 200
    except Exception as e:
        return jsonify({"db": "error", "details": str(e)}), 500

__all__ = ["health_bp"]
