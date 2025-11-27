from flask import Blueprint, request, jsonify
from app.services.room_service import RoomService
from functools import wraps

room_bp = Blueprint('room', __name__, url_prefix='/api/room')

def require_auth(f):
    """Decorator to require authentication"""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        account_id = request.headers.get('X-Account-ID')
        if not account_id:
            return jsonify({'error': 'Authentication required'}), 401
        
        try:
            account_id = int(account_id)
        except ValueError:
            return jsonify({'error': 'Invalid account ID'}), 400
        
        return f(account_id, *args, **kwargs)
    return decorated_function


@room_bp.route('/create', methods=['POST'])
@require_auth
def create_room(account_id):
    """
    POST /api/room/create
    Headers: X-Account-ID: <account_id>
    Body: {
        "name": "My Room",
        "visibility": "public"
    }
    """
    room_data = request.get_json()
    
    if not room_data or 'name' not in room_data:
        return jsonify({'error': 'Room name is required'}), 400
    
    result = RoomService.create_room(account_id, room_data)
    
    if result['success']:
        return jsonify(result), 201
    return jsonify(result), 400


@room_bp.route('/<int:room_id>', methods=['DELETE'])
@require_auth
def delete_room(account_id, room_id):
    """
    DELETE /api/room/{room_id}
    Headers: X-Account-ID: <account_id>
    """
    result = RoomService.delete_room(room_id, account_id)
    
    if result['success']:
        return jsonify(result), 200
    return jsonify(result), 403


@room_bp.route('/<int:room_id>', methods=['GET'])
def get_room_details(room_id):
    """GET /api/room/{room_id}"""
    result = RoomService.get_room_details(room_id)
    
    if result['success']:
        return jsonify(result), 200
    return jsonify(result), 404


@room_bp.route('/list', methods=['GET'])
def list_rooms():
    """GET /api/room/list?status=waiting&visibility=public"""
    status = request.args.get('status', 'waiting')
    visibility = request.args.get('visibility', 'public')
    
    result = RoomService.list_rooms(status, visibility)
    
    if result['success']:
        return jsonify(result), 200
    return jsonify(result), 500