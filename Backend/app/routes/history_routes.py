from flask import Blueprint, jsonify

bp = Blueprint("history", __name__, url_prefix="/history")

@bp.route("", methods=["GET"])
def history():
    return jsonify({
        "message": "hello from backend"
    })
