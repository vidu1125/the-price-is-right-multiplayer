# app/routes/room_routes.py
from flask import Blueprint, request, jsonify
from app.services.room_service import RoomService
from functools import wraps

room_bp = Blueprint('room', __name__, url_prefix='/api/room')

def require_auth(f):
    """Require X-Account-ID header"""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        account_id = request.headers.get('X-Account-ID')
        if not account_id:
            return jsonify({'success': False, 'error': 'Authentication required'}), 401
        try:
            account_id = int(account_id)
        except ValueError:
            return jsonify({'success': False, 'error': 'Invalid account id'}), 400
        return f(account_id, *args, **kwargs)
    return decorated_function


@room_bp.route('/create', methods=['POST'])
@require_auth
def create_room(account_id):
    data = request.get_json(silent=True) or {}
    if not data.get('name'):
        return jsonify({'success': False, 'error': 'Room name is required'}), 400

    result = RoomService.create_room(account_id, data)
    return (jsonify(result), 201) if result.get('success') else (jsonify(result), 400)


@room_bp.route('/<int:room_id>', methods=['DELETE'])
@require_auth
def delete_room(account_id, room_id):
    result = RoomService.delete_room(room_id, account_id)
    return (jsonify(result), 200) if result.get('success') else (jsonify(result), 403)


@room_bp.route('/<int:room_id>', methods=['GET'])
def get_room_details(room_id):
    result = RoomService.get_room_details(room_id)
    return (jsonify(result), 200) if result.get('success') else (jsonify(result), 404)


@room_bp.route('/list', methods=['GET'])
def list_rooms():
    status = request.args.get('status', 'waiting')
    visibility = request.args.get('visibility', 'public')

    result = RoomService.list_rooms(status=status, visibility=visibility)
    return (jsonify(result), 200) if result.get('success') else (jsonify(result), 500)

@room_bp.route('/rules', methods=['PUT'])
@require_auth
def update_rules(account_id):
    """Host updates room rules"""
    data = request.get_json()
    room_id = data.get('room_id')
    rules = data.get('rules')
    
    result = RoomService.update_rules(room_id, account_id, rules)
    return (jsonify(result), 200) if result.get('success') else (jsonify(result), 403)

@room_bp.route('/kick', methods=['POST'])
@require_auth
def kick_member(account_id):
    """Host kicks a member"""
    data = request.get_json()
    room_id = data.get('room_id')
    target_id = data.get('target_id')
    
    result = RoomService.kick_member(room_id, account_id, target_id)
    return (jsonify(result), 200) if result.get('success') else (jsonify(result), 403)

@room_bp.route('/<int:room_id>/leave', methods=['POST'])
@require_auth
def leave_room(account_id, room_id):
    """Member leaves room"""
    result = RoomService.leave_room(room_id, account_id)
    return (jsonify(result), 200) if result.get('success') else (jsonify(result), 400)

# Export bp for auto-registration
bp = room_bp
