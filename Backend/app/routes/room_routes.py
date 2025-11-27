from flask import Blueprint, request, jsonify
from app.services.room_service import RoomService
from functools import wraps

# Create Blueprint
room_bp = Blueprint('room', __name__, url_prefix='/api/room')

# Auth decorator
def require_auth(f):
    """Decorator to require authentication"""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        user_id = request.headers.get('X-User-ID')
        if not user_id:
            return jsonify({'error': 'Authentication required'}), 401
        return f(user_id, *args, **kwargs)
    return decorated_function

# ==================== ROOM ROUTES ====================

@room_bp.route('/create', methods=['POST'])
@require_auth
def create_room(user_id):
    """
    POST /api/room/create
    Headers: X-User-ID: <user_id>
    Body: {
        "name": "My Room",
        "visibility": "public",
        "mode": "scoring",
        "max_players": 4,
        "wager_enabled": false,
        "round_time_sec": 15
    }
    """
    room_data = request.get_json()
    
    if not room_data or 'name' not in room_data:
        return jsonify({'error': 'Room name is required'}), 400
    
    result = RoomService.create_room(user_id, room_data)
    
    if result['success']:
        return jsonify(result), 201
    return jsonify(result), 400


@room_bp.route('/<int:room_id>', methods=['DELETE'])
@require_auth
def delete_room(user_id, room_id):
    """
    DELETE /api/room/{room_id}
    Headers: X-User-ID: <user_id>
    """
    result = RoomService.delete_room(room_id, user_id)
    
    if result['success']:
        return jsonify(result), 200
    return jsonify(result), 403


@room_bp.route('/<int:room_id>', methods=['GET'])
def get_room_details(room_id):
    """
    GET /api/room/{room_id}
    Get detailed information about a room
    """
    result = RoomService.get_room_details(room_id)
    
    if result['success']:
        return jsonify(result), 200
    return jsonify(result), 404


@room_bp.route('/list', methods=['GET'])
def list_rooms():
    """
    GET /api/room/list?status=waiting&visibility=public
    List all available rooms
    """
    status = request.args.get('status', 'waiting')
    visibility = request.args.get('visibility', 'public')
    
    result = RoomService.list_rooms(status, visibility)
    
    if result['success']:
        return jsonify(result), 200
    return jsonify(result), 500