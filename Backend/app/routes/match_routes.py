# app/routes/match_routes.py
from flask import Blueprint, request, jsonify
from app.services.match_service import MatchService
from functools import wraps

bp = Blueprint('match', __name__, url_prefix='/api/match')

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


@bp.route('/start', methods=['POST'])
@require_auth
def start_game(account_id):
    """
    Host starts the game
    Phase 1: Create match (NO questions returned)
    """
    data = request.get_json(silent=True) or {}
    room_id = data.get('room_id')
    
    if not room_id:
        return jsonify({'success': False, 'error': 'room_id required'}), 400
    
    result = MatchService.start_game(room_id, account_id)
    return (jsonify(result), 200) if result.get('success') else (jsonify(result), 400)


@bp.route('/<int:match_id>/round/<int:round_no>/questions', methods=['GET'])
def get_round_questions(match_id, round_no):
    """
    Get questions for a specific round
    Called by C Server after countdown
    """
    result = MatchService.get_round_questions(match_id, round_no)
    return (jsonify(result), 200) if result.get('success') else (jsonify(result), 404)
