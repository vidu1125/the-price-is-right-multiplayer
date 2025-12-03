from flask import Blueprint, jsonify
from app import db
from app.routes.room_routes import room_bp

# Blueprint chỉ để check database
utils_bp = Blueprint("utils", __name__)

@utils_bp.route("/health/db")
def db_check():
    try:
        db.session.execute(db.text("SELECT 1"))
        return jsonify({"db": "connected"}), 200
    except Exception as e:
        return jsonify({"db": "error", "details": str(e)}), 500

__all__ = ["room_bp", "utils_bp"]
